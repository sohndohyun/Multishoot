param(
    [string]$ServerPath = "$PSScriptRoot\..\bld\x64\Debug\MultishootServer\MultishootServer.exe",
    [string]$DatabaseName = 'multishoot_test'
)

$ErrorActionPreference = 'Stop'

function Read-Exact([System.IO.Stream]$stream, [int]$count) {
    $buffer = [byte[]]::new($count)
    for ($offset = 0; $offset -lt $count;) {
        $read = $stream.Read($buffer, $offset, $count - $offset)
        if ($read -eq 0) { throw 'Connection closed while reading a frame.' }
        $offset += $read
    }
    $buffer
}

function Read-Frame([System.Net.Sockets.TcpClient]$client) {
    $header = Read-Exact $client.GetStream() 8
    $size = [BitConverter]::ToInt32($header, 4)
    if ($header[0] -ne 12 -or $size -lt 0 -or $size -gt 504) {
        throw 'Invalid frame received from server.'
    }
    ,(Read-Exact $client.GetStream() $size)
}

function New-Frame([byte[]]$body) {
    $frame = [byte[]]::new(8 + $body.Length)
    $frame[0] = 12
    [BitConverter]::GetBytes($body.Length).CopyTo($frame, 4)
    $body.CopyTo($frame, 8)
    $frame
}

function New-ShootRequest {
    # ClientPacket.shoot_request (field 2, length-delimited empty message).
    New-Frame ([byte[]](0x12, 0x00))
}

function New-AuthRequest([ValidateSet('Login', 'Signup')][string]$kind,
                         [string]$username, [string]$password) {
    $usernameBytes = [Text.Encoding]::ASCII.GetBytes($username)
    $passwordBytes = [Text.Encoding]::ASCII.GetBytes($password)
    $request = [Collections.Generic.List[byte]]::new()
    $request.Add(0x0A)
    $request.Add([byte]$usernameBytes.Length)
    $request.AddRange($usernameBytes)
    $request.Add(0x12)
    $request.Add([byte]$passwordBytes.Length)
    $request.AddRange($passwordBytes)

    $body = [Collections.Generic.List[byte]]::new()
    $body.Add([byte]$(if ($kind -eq 'Login') { 0x1A } else { 0x22 }))
    $body.Add([byte]$request.Count)
    $body.AddRange($request)
    New-Frame ($body.ToArray())
}

function Connect-Client {
    $client = [System.Net.Sockets.TcpClient]::new('127.0.0.1', 3000)
    $client.ReceiveTimeout = 5000
    $client
}

function Resolve-Docker {
    $command = Get-Command docker.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $fallback = Join-Path $env:LOCALAPPDATA 'Programs\DockerDesktop\resources\bin\docker.exe'
    if (Test-Path -LiteralPath $fallback) {
        $env:PATH = "$(Split-Path -Parent $fallback);$env:PATH"
        return $fallback
    }
    $fallback = Join-Path $env:ProgramFiles 'Docker\Docker\resources\bin\docker.exe'
    if (Test-Path -LiteralPath $fallback) {
        $env:PATH = "$(Split-Path -Parent $fallback);$env:PATH"
        return $fallback
    }
    throw 'Docker CLI was not found.'
}

function Clear-TestDatabase {
    $docker = Resolve-Docker
    & $docker compose exec -T mysql mysql -uroot -pmultishoot_root_dev -D $DatabaseName -e 'TRUNCATE TABLE accounts;' | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Could not clear the network test database.' }
}

function Set-TestBestScore([string]$username, [int]$score) {
    $docker = Resolve-Docker
    $sql = "UPDATE accounts SET best_score = $score WHERE username = '$username';"
    & $docker compose exec -T mysql mysql -uroot -pmultishoot_root_dev -D $DatabaseName -e $sql | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Could not seed the network test score.' }
}

function Start-TestServer {
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = (Resolve-Path $ServerPath)
    $startInfo.WorkingDirectory = Split-Path $startInfo.FileName
    $startInfo.Arguments = "--db-name $DatabaseName"
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    [System.Diagnostics.Process]::Start($startInfo)
}

function Stop-TestServer([System.Diagnostics.Process]$process) {
    if (-not $process -or $process.HasExited) { return }
    $process.StandardInput.WriteLine('quit')
    if (-not $process.WaitForExit(5000)) { $process.Kill() }
    $process.Dispose()
}

function Invoke-Auth([System.Net.Sockets.TcpClient]$client,
                     [ValidateSet('Login', 'Signup')][string]$kind,
                     [string]$username, [string]$password, [int]$expected,
                     [int]$expectedBestScore = -1) {
    $request = New-AuthRequest $kind $username $password
    $client.GetStream().Write($request, 0, $request.Length)
    $body = Read-Frame $client
    if ($body.Length -lt 4 -or $body[0] -ne 0x52 -or $body[2] -ne 0x08 -or
        $body[3] -ne $expected) {
        throw "Unexpected auth result for ${kind}/${username}: $($body -join ',')"
    }
    if ($expectedBestScore -ge 0 -and
        ($body.Length -lt 6 -or $body[4] -ne 0x10 -or $body[5] -ne $expectedBestScore)) {
        throw "Unexpected best score for ${kind}/${username}: $($body -join ',')"
    }

    if ($expected -eq 1) {
        $join = Read-Frame $client
        if ($join.Length -eq 0 -or $join[0] -ne 0x0A) {
            throw 'LoginResponse did not follow successful authentication.'
        }
    }
}

function Drain-Cases([System.Net.Sockets.TcpClient]$client) {
    $cases = @()
    while ($client.Available -ge 8) {
        $body = Read-Frame $client
        if ($body.Length -gt 0) { $cases += ($body[0] -shr 3) }
    }
    $cases
}

Clear-TestDatabase
$server = Start-TestServer

try {
    Start-Sleep -Milliseconds 300

    $bad = [System.Net.Sockets.TcpClient]::new('127.0.0.1', 3000)
    $header = [byte[]]::new(8)
    $header[0] = 12
    [BitConverter]::GetBytes(10000).CopyTo($header, 4)
    $bad.GetStream().Write($header, 0, $header.Length)
    Start-Sleep -Milliseconds 200
    $bad.Close()
    if ($server.HasExited) { throw 'Server crashed on oversized frame.' }

    $unauthenticated = Connect-Client
    Start-Sleep -Milliseconds 150
    if ($unauthenticated.Available -ne 0) {
        throw 'Server created a player before authentication.'
    }
    $shoot = New-ShootRequest
    $unauthenticated.GetStream().Write($shoot, 0, $shoot.Length)
    Start-Sleep -Milliseconds 100
    if ($unauthenticated.Available -ne 0) {
        throw 'Unauthenticated game request produced a response.'
    }
    Invoke-Auth $unauthenticated Signup 'ab' 'password1' 2

    $a = $unauthenticated
    Invoke-Auth $a Signup 'player_a' 'password1' 1

    $duplicate = Connect-Client
    Invoke-Auth $duplicate Signup 'player_a' 'password1' 3

    $wrong = Connect-Client
    Invoke-Auth $wrong Login 'player_a' 'wrongpass' 4

    $inUse = Connect-Client
    Invoke-Auth $inUse Login 'player_a' 'password1' 5

    $a.Close()
    $a = $null
    Start-Sleep -Milliseconds 200

    if ($b) { $b.Close() }
    $b = $null
    if ($duplicate) { $duplicate.Close() }
    $duplicate = $null
    if ($wrong) { $wrong.Close() }
    $wrong = $null
    if ($inUse) { $inUse.Close() }
    $inUse = $null
    $unauthenticated = $null
    Set-TestBestScore 'player_a' 42
    Stop-TestServer $server
    $server = Start-TestServer
    Start-Sleep -Milliseconds 300
    $a = Connect-Client
    Invoke-Auth $a Login 'player_a' 'password1' 1 42

    $docker = Resolve-Docker
    & $docker compose stop mysql | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Could not stop MySQL for reconnect testing.' }
    Start-Sleep -Milliseconds 300
    $reconnect = Connect-Client
    Invoke-Auth $reconnect Login 'reconnect_probe' 'password1' 6
    & $docker compose up -d --wait | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Could not restart MySQL for reconnect testing.' }
    Invoke-Auth $reconnect Signup 'reconnect_user' 'password1' 1
    $reconnect.Close()
    $reconnect = $null

    $cancel = Connect-Client
    $cancelRequest = New-AuthRequest Signup 'cancel_user' 'password3'
    $cancel.GetStream().Write($cancelRequest, 0, $cancelRequest.Length)
    Start-Sleep -Milliseconds 50
    $cancel.Close()
    $cancel = $null
    Start-Sleep -Milliseconds 200
    $cancel = Connect-Client
    Invoke-Auth $cancel Login 'cancel_user' 'password3' 1

    $b = Connect-Client
    Invoke-Auth $b Signup 'player_b' 'password2' 1
    Start-Sleep -Milliseconds 150
    [void](Drain-Cases $a)
    [void](Drain-Cases $b)

    $emptyPacket = New-Frame ([byte[]]::new(0))
    $malformedPacket = New-Frame ([byte[]](0x80))
    $a.GetStream().Write($emptyPacket, 0, $emptyPacket.Length)
    $a.GetStream().Write($malformedPacket, 0, $malformedPacket.Length)
    Start-Sleep -Milliseconds 100
    if ((Drain-Cases $a) -contains 4) { throw 'Malformed protobuf produced a shot.' }
    if ($server.HasExited) { throw 'Server crashed on malformed protobuf.' }

    $a.GetStream().Write($shoot, 0, $shoot.Length)
    $a.GetStream().Write($shoot, 0, $shoot.Length)
    Start-Sleep -Milliseconds 100
    $shootCases = @(Drain-Cases $a | Where-Object { $_ -eq 4 })
    if ($shootCases.Count -ne 1) { throw "Cooldown accepted $($shootCases.Count) immediate shots." }

    Start-Sleep -Milliseconds 150
    $a.GetStream().Write($shoot, 0, 3)
    Start-Sleep -Milliseconds 20
    $a.GetStream().Write($shoot, 3, 5)
    Start-Sleep -Milliseconds 20
    $a.GetStream().Write($shoot, 8, $shoot.Length - 8)

    $receivedShoot = $false
    $deadline = [DateTime]::UtcNow.AddSeconds(2)
    do {
        if ($a.Available -ge 8) {
            $body = Read-Frame $a
            $receivedShoot = $body.Length -gt 0 -and ($body[0] -shr 3) -eq 4
        } else {
            Start-Sleep -Milliseconds 10
        }
    } until ($receivedShoot -or [DateTime]::UtcNow -ge $deadline)
    if (-not $receivedShoot) { throw 'Fragmented ShootRequest was not handled.' }

    'PASS oversized-frame disconnect'
    'PASS malformed-protobuf rejection'
    'PASS authentication gate and validation'
    'PASS duplicate and concurrent account rejection'
    'PASS account reuse after disconnect'
    'PASS authoritative shoot cooldown'
    'PASS fragmented protobuf frame reassembly'
}
finally {
    if ($a) { $a.Close() }
    if ($b) { $b.Close() }
    if ($reconnect) { $reconnect.Close() }
    if ($cancel) { $cancel.Close() }
    if ($duplicate) { $duplicate.Close() }
    if ($wrong) { $wrong.Close() }
    if ($inUse) { $inUse.Close() }
    if ($unauthenticated -and $unauthenticated -ne $a) { $unauthenticated.Close() }
    Stop-TestServer $server
}
