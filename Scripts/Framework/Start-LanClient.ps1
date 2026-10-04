[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackageRoot,
    [string]$ServerAddress,
    [ValidateRange(1024,65535)][int]$Port = 17877,
    [ValidateRange(1024,65535)][int]$GamePort = 17878,
    [string]$ResultsDirectory,
    [switch]$SteamSmoke,
    # Development-only regression control: replaces the packet-handler ticket with a malformed value.
    [switch]$PacketAuthBadKey
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $PackageRoot).Path
$exe = Join-Path $root 'Onset/Binaries/Win64/OnsetClient.exe'
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Client executable not found: $exe" }
$results = if ($ResultsDirectory) { [IO.Path]::GetFullPath($ResultsDirectory) } else { Join-Path $root 'MHTestResults' }
$run = Join-Path $results ((Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Force -Path $run | Out-Null
$launchArgs = @('-nosplash', '-noanalytics', '-CustomConfig=MHLanTest', "-abslog=$run/client.log")
if ($SteamSmoke) {
	if ($PacketAuthBadKey) { throw 'PacketAuthBadKey cannot be combined with SteamSmoke.' }
    $launchArgs = @('/Engine/Maps/Entry?game=/Script/Engine.GameModeBase') + $launchArgs + @('-unattended', '-NullRHI', '-nosound',
        '-MHSteamLiveTest', '-ExecCmds=Automation RunTests MH.Online.SteamLive',
        '-TestExit=Automation Test Queue Empty', "-ReportExportPath=$run")
} else {
    $ip = $null
    if (![Net.IPAddress]::TryParse($ServerAddress, [ref]$ip) -or $ip.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork) {
        throw 'Provide the server computer private LAN IPv4 address.'
    }
    $bytes = $ip.GetAddressBytes()
    $local = $bytes[0] -eq 10 -or $bytes[0] -eq 127 -or ($bytes[0] -eq 192 -and $bytes[1] -eq 168) -or ($bytes[0] -eq 172 -and $bytes[1] -ge 16 -and $bytes[1] -le 31)
    if (!$local) { throw 'This development launcher accepts private LAN or loopback addresses only.' }
    $launchArgs = @("${ServerAddress}:$Port") + $launchArgs + @('-windowed', '-ResX=1280', '-ResY=720', '-AuthMode=Direct',
        "-ini:Engine:[Onset.Auth]:GameServerIP=$ServerAddress", "-ini:Engine:[Onset.Auth]:GameServerPort=$GamePort")
	if ($PacketAuthBadKey) { $launchArgs += '-MHPacketAuthBadKey' }
}
# The development Steam SDK looks beside the real executable, not the bootstrap exe.
Set-Content -LiteralPath (Join-Path (Split-Path $exe) 'steam_appid.txt') -Value '480' -Encoding ascii
Write-Host "Launching client. Results: $run"
if ($launchArgs | Where-Object { $_ -match '["\r\n]' }) { throw 'Unsupported quote/newline in path' }
$argumentLine = ($launchArgs | ForEach-Object { '"' + $_ + '"' }) -join ' '
$startOptions = @{ FilePath=$exe; ArgumentList=$argumentLine; WorkingDirectory=(Split-Path $exe); Wait=$true; PassThru=$true }
if ($SteamSmoke) { $startOptions.WindowStyle = 'Hidden' }
$process = Start-Process @startOptions
$clientExit = $process.ExitCode
if ($clientExit -ne 0) { throw "Client exit code $clientExit. See $run" }
if ($SteamSmoke) {
    $report = Get-Content -LiteralPath (Join-Path $run 'index.json') -Raw | ConvertFrom-Json
    $tests = @($report.tests)
    if ($tests.Count -ne 1 -or $tests[0].state -ne 'Success') { throw "Steam smoke did not pass; see $run" }
    Write-Host 'PASS: Steam ticket acquisition/release. Remote server authentication was not tested.'
}
