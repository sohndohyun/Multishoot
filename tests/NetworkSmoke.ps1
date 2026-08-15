param(
    [string]$ServerPath = "$PSScriptRoot\..\x64\Debug\MultishootServer.exe"
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

function Connect-Player {
    $client = [System.Net.Sockets.TcpClient]::new('127.0.0.1', 3000)
    $client.ReceiveTimeout = 1500
    $body = Read-Frame $client
    if ($body.Length -eq 0 -or $body[0] -ne 0x0A) {
        throw 'LoginResponse was not first.'
    }
    $client
}

function Drain-Cases([System.Net.Sockets.TcpClient]$client) {
    $cases = @()
    while ($client.Available -ge 8) {
        $body = Read-Frame $client
        if ($body.Length -gt 0) { $cases += ($body[0] -shr 3) }
    }
    $cases
}

$startInfo = [System.Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = (Resolve-Path $ServerPath)
$startInfo.WorkingDirectory = Split-Path $startInfo.FileName
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $true
$startInfo.RedirectStandardInput = $true
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true
$server = [System.Diagnostics.Process]::Start($startInfo)

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

    $a = Connect-Player
    $b = Connect-Player
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

    $shoot = New-ShootRequest
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
    'PASS authoritative shoot cooldown'
    'PASS fragmented protobuf frame reassembly'
}
finally {
    if ($a) { $a.Close() }
    if ($b) { $b.Close() }
    if (-not $server.HasExited) {
        $server.StandardInput.WriteLine('quit')
        if (-not $server.WaitForExit(5000)) { $server.Kill() }
    }
    $server.Dispose()
}
