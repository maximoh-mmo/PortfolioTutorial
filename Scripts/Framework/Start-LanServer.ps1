[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackageRoot,
    [Parameter(Mandatory=$true)][string]$BindAddress,
    [Alias('Port')][ValidateRange(1024,65535)][int]$LoginPort = 17877,
    [ValidateRange(1024,65535)][int]$GamePort = 17878,
    [ValidateRange(1024,65535)][int]$LoginQueryPort = 17879,
    [ValidateRange(1024,65535)][int]$GameQueryPort = 17880
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $PackageRoot).Path
$exe = Join-Path $root 'Onset/Binaries/Win64/OnsetServer.exe'
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Server executable not found: $exe" }
$ip = $null
if (![Net.IPAddress]::TryParse($BindAddress, [ref]$ip) -or $ip.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork) { throw 'Provide a private LAN IPv4 bind address.' }
$bytes = $ip.GetAddressBytes()
if (!($bytes[0] -eq 10 -or $bytes[0] -eq 127 -or ($bytes[0] -eq 192 -and $bytes[1] -eq 168) -or ($bytes[0] -eq 172 -and $bytes[1] -ge 16 -and $bytes[1] -le 31))) { throw 'Private LAN or loopback addresses only.' }
$ports = @($LoginPort, $GamePort, $LoginQueryPort, $GameQueryPort)
if (@($ports | Select-Object -Unique).Count -ne 4) { throw 'The four ports must be distinct.' }
$occupiedPorts = [Net.NetworkInformation.IPGlobalProperties]::GetIPGlobalProperties().GetActiveUdpListeners().Port
foreach ($port in $ports) {
    if ($occupiedPorts -contains $port) { throw "UDP port $port is occupied" }
}
$run = Join-Path $root ('MHTestResults/' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Force -Path $run | Out-Null
$common = @('-unattended', '-NullRHI', '-nosound', '-nosplash', '-noanalytics',
    "-MULTIHOME=$BindAddress", '-OnsetDataStoreType=SQLite',
    "-OnsetDataStoreURL=$run/local-test.db", '-ini:Engine:[Onset.Auth]:AuthTokenSecret=mh-lan-test-only',
    "-ini:Engine:[Onset.Auth]:GameServerIP=$BindAddress", "-ini:Engine:[Onset.Auth]:GameServerPort=$GamePort",
    '-ini:Engine:[OnlineSubsystemSteam]:SteamDevAppId=480')
$owned = @()
try {
    foreach ($role in @(
        @{ Name='login'; Map='/Game/Maps/LoginServer'; Mode='Direct'; Port=$LoginPort; Query=$LoginQueryPort },
        @{ Name='game'; Map='/Game/Maps/DemoLevel'; Mode='Token'; Port=$GamePort; Query=$GameQueryPort }
    )) {
        $log = Join-Path $run ($role.Name + '.log')
        $launchArgs = @($role.Map) + $common + @("-Port=$($role.Port)", "-QueryPort=$($role.Query)", "-AuthMode=$($role.Mode)", "-abslog=$log")
        if ($launchArgs | Where-Object { $_ -match '["\r\n]' }) { throw 'Unsupported quote/newline in path' }
        $argumentLine = ($launchArgs | ForEach-Object { '"' + $_ + '"' }) -join ' '
        $process = Start-Process -FilePath $exe -ArgumentList $argumentLine -WorkingDirectory (Split-Path $exe) -WindowStyle Hidden -PassThru
        $owned += $process
        $deadline = [DateTime]::UtcNow.AddSeconds(120)
        $ready = $false
        while (!$process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
            if (Test-Path -LiteralPath $log) {
                $text = Get-Content -LiteralPath $log -Raw
                $ready = $text.Contains('initialized with SQLite (success=1)') -and
                    $text.Contains('Bringing up level for play took:') -and
                    $text.Contains("IpNetDriver listening on port $($role.Port)")
                if ($ready) { break }
            }
            Start-Sleep -Milliseconds 500
            $process.Refresh()
        }
        if (!$ready) { throw "$($role.Name) server did not start successfully; see $log" }
        Write-Host "$($role.Name) server PID $($process.Id): ${BindAddress}:$($role.Port)"
    }
} catch {
    foreach ($process in $owned) { if (!$process.HasExited) { $process.Kill(); $process.WaitForExit() } }
    throw
}
$ids = ($owned | ForEach-Object { $_.Id }) -join ','
Write-Host "Results: $run"
Write-Host 'This tests legacy DirectAuth-to-TokenAuth connectivity; it does not prove Steam ticket validation.'
Write-Host "Stop these servers with: Stop-Process -Id $ids"
