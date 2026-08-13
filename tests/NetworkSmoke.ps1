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

function Connect-Player {
    $client = [System.Net.Sockets.TcpClient]::new('127.0.0.1', 3000)
    $client.ReceiveTimeout = 1500
    $body = Read-Frame $client
    if ([BitConverter]::ToInt32($body, 0) -ne 2) { throw 'LOGIN_RES was not first.' }
    @{ Client = $client; Id = [BitConverter]::ToUInt32($body, 4) }
}

function New-ShootRequest([uint32]$playerId) {
    $frame = [byte[]]::new(16)
    $frame[0] = 12
    [BitConverter]::GetBytes(8).CopyTo($frame, 4)
    [BitConverter]::GetBytes(1).CopyTo($frame, 8)
    [BitConverter]::GetBytes($playerId).CopyTo($frame, 12)
    $frame
}

function Drain-Types([System.Net.Sockets.TcpClient]$client) {
    $types = @()
    while ($client.Available -ge 8) {
        $types += [BitConverter]::ToInt32((Read-Frame $client), 0)
    }
    $types
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
    [void](Drain-Types $a.Client)
    [void](Drain-Types $b.Client)

    $spoof = New-ShootRequest $b.Id
    $a.Client.GetStream().Write($spoof, 0, $spoof.Length)
    Start-Sleep -Milliseconds 200
    $types = @(Drain-Types $a.Client) + @(Drain-Types $b.Client)
    if ($types -contains 5) { throw 'Spoofed SHOOT_REQ was accepted.' }

    $valid = New-ShootRequest $a.Id
    $a.Client.GetStream().Write($valid, 0, 3)
    Start-Sleep -Milliseconds 20
    $a.Client.GetStream().Write($valid, 3, 5)
    Start-Sleep -Milliseconds 20
    $a.Client.GetStream().Write($valid, 8, 8)

    $deadline = [DateTime]::UtcNow.AddSeconds(2)
    do {
        if ($a.Client.Available -ge 8) {
            $receivedShoot = [BitConverter]::ToInt32((Read-Frame $a.Client), 0) -eq 5
        } else {
            Start-Sleep -Milliseconds 10
        }
    } until ($receivedShoot -or [DateTime]::UtcNow -ge $deadline)
    if (-not $receivedShoot) { throw 'Fragmented SHOOT_REQ was not handled.' }

    'PASS oversized-frame disconnect'
    'PASS player-id spoof rejection'
    'PASS fragmented-frame reassembly'
}
finally {
    if ($a) { $a.Client.Close() }
    if ($b) { $b.Client.Close() }
    if (-not $server.HasExited) {
        $server.StandardInput.WriteLine('quit')
        if (-not $server.WaitForExit(5000)) { $server.Kill() }
    }
    $server.Dispose()
}
