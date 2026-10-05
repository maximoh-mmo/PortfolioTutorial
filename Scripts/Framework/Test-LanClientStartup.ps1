[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackageRoot,
    [Parameter(Mandatory=$true)][string]$ResultsDirectory,
    [ValidateRange(30,600)][int]$StartupTimeoutSeconds = 180
)
$ErrorActionPreference = 'Stop'
$package = (Resolve-Path -LiteralPath $PackageRoot).Path
$exe = Join-Path $package 'Onset/Binaries/Win64/OnsetClient.exe'
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Packaged client missing: $exe" }
$results = [IO.Path]::GetFullPath($ResultsDirectory)
if (Test-Path -LiteralPath $results) { throw 'Choose a new startup-check results directory.' }
New-Item -ItemType Directory -Path $results | Out-Null
$log = Join-Path $results 'client.log'
# Exercise the real menu and its constructor-loaded Blueprint classes without a
# graphics device, Steam account, remote server, or existing player database.
$launchArgs = @('/Game/Maps/MainMenu', '-unattended', '-NullRHI', '-nosound', '-nosplash',
    '-nosteam', '-noanalytics', '-CustomConfig=MHLanTest', '-seconds=10', '-AuthMode=Direct',
    '-OnsetDataStoreType=SQLite', "-OnsetDataStoreURL=$results/startup.db",
    '-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=Null',
    '-ini:Engine:[OnlineSubsystemSteam]:bEnabled=false', "-abslog=$log")
if ($launchArgs | Where-Object { $_ -match '["\r\n]' }) { throw 'Unsupported quote/newline in path' }
$argumentLine = ($launchArgs | ForEach-Object { '"' + $_ + '"' }) -join ' '
$process = $null
try {
    $process = Start-Process -FilePath $exe -ArgumentList $argumentLine -PassThru -WindowStyle Hidden `
        -WorkingDirectory (Split-Path $exe)
    $deadline = [DateTime]::UtcNow.AddSeconds($StartupTimeoutSeconds)
    while (!$process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 500
        $process.Refresh()
    }
    if (!$process.HasExited) { throw "Client startup check timed out. See $log" }
    $process.WaitForExit()
    $logText = Get-Content -LiteralPath $log -Raw
    $checks = [ordered]@{
        CleanExit = ($process.ExitCode -eq 0)
        MainMenuLoaded = ($logText -match 'LoadMap: /Game/Maps/MainMenu')
        WorldStarted = $logText.Contains('Bringing up level for play took:')
        NoSerializationError = ($logText -notmatch 'ObjectSerializationError|Bad export index')
        NoFatalError = ($logText -notmatch 'Fatal error:|Assertion failed:|Unhandled Exception:')
        NoMissingPlugin = ($logText -notmatch 'Unable to load plugin')
    }
    $failed = @($checks.Keys | Where-Object { !$checks[$_] })
    [pscustomobject]@{ Passed=($failed.Count -eq 0); Checks=$checks; ExitCode=$process.ExitCode;
        Package=$package; Log=$log; Scope='Cooked menu startup only; Steam authentication and rendered UI require LAN testing' } |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $results 'result.json')
    if ($failed.Count) { throw "Client startup checks failed: $($failed -join ', '). See $log" }
    Write-Host "PASS: cooked client menu startup. Results: $results"
}
finally {
    # Only stop the process started by this check.
    if ($process -and !$process.HasExited) { $process.Kill(); $process.WaitForExit() }
    if ($process) { $process.Dispose() }
}
