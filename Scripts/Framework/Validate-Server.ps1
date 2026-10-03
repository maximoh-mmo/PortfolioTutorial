[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [string]$PostgresRoot,
    # Supply an existing archived WindowsServer directory to repeat only the smoke test.
    [string]$PackageRoot,
    [ValidateRange(1024,65535)][int]$Port = 17877,
    [ValidateRange(60,600)][int]$StartupTimeoutSeconds = 180
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$projectFile = Join-Path $projectRoot 'Project/Onset.uproject'
$engine = (Resolve-Path -LiteralPath $EngineRoot).Path
$runId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8)
$runRoot = Join-Path $projectRoot "Project/Saved/MHServerValidation/$runId"
$buildScript = Join-Path $engine 'Engine/Build/BatchFiles/Build.bat'
$uatScript = Join-Path $engine 'Engine/Build/BatchFiles/RunUAT.bat'
foreach ($file in @($projectFile, $buildScript, $uatScript)) {
    if (!(Test-Path -LiteralPath $file -PathType Leaf)) { throw "Required file missing: $file" }
}
if (!$PostgresRoot) { $PostgresRoot = Join-Path $projectRoot 'Project/Source/Onset/ThirdParty/PostgreSQL' }
$PostgresRoot = (Resolve-Path -LiteralPath $PostgresRoot).Path
if (!(Test-Path -LiteralPath (Join-Path $PostgresRoot 'include/libpq-fe.h'))) { throw 'Invalid PostgreSQL SDK root' }
if ($PackageRoot) { $PackageRoot = (Resolve-Path -LiteralPath $PackageRoot).Path }
New-Item -ItemType Directory -Path $runRoot -Force | Out-Null
Write-Host "Validation output: $runRoot"

function Invoke-NativeStep([string]$File, [string[]]$Arguments, [string]$Log) {
    $oldPreference = $ErrorActionPreference
    try {
        # Windows PowerShell represents native stderr as ErrorRecords, including warnings.
        $ErrorActionPreference = 'Continue'
        & $File @Arguments 2>&1 | Tee-Object -FilePath $Log
        $nativeExit = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $oldPreference }
    if ($nativeExit -ne 0) { throw "Native step failed with exit $nativeExit. See $Log" }
}

$previousSdk = $env:MH_POSTGRES_ROOT
$serverProcess = $null
Push-Location -LiteralPath $projectRoot
try {
    $env:MH_POSTGRES_ROOT = $PostgresRoot
    if (!$PackageRoot) {
        # Cooking needs this project's editor modules built against the selected source engine.
        Invoke-NativeStep $buildScript @('OnsetEditor','Win64','Development',$projectFile,
            '-WaitMutex','-NoHotReloadFromIDE') (Join-Path $runRoot 'editor-build.log')
        $archive = Join-Path $runRoot 'archive'
        Invoke-NativeStep $uatScript @("-ScriptsForProject=$projectFile",'BuildCookRun',
            "-project=$projectFile",'-noP4','-unattended','-utf8output',
            '-server','-noclient','-serverplatform=Win64','-serverconfig=Development',
            '-build','-cook','-stage','-pak','-archive','-map=/Game/Maps/LoginServer+/Game/Maps/DemoLevel',
            "-stagingdirectory=$(Join-Path $runRoot 'stage')", "-archivedirectory=$archive") (Join-Path $runRoot 'build-cook-stage.log')
        $PackageRoot = Join-Path $archive 'WindowsServer'
    }

    $serverExe = Join-Path $PackageRoot 'Onset/Binaries/Win64/OnsetServer.exe'
    if (!(Test-Path -LiteralPath $serverExe -PathType Leaf)) { throw "Packaged executable missing: $serverExe" }
    if (!(Test-Path -LiteralPath (Join-Path (Split-Path $serverExe) 'libpq.dll'))) {
        throw 'Packaged libpq.dll missing from the server binary directory'
    }
    if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) { throw "UDP port $Port is occupied" }

    $serverLog = Join-Path $runRoot 'server.log'
    $database = Join-Path $runRoot 'startup-test.db'
    $arguments = @('/Game/Maps/DemoLevel','-unattended','-NullRHI','-nosound','-nosplash',
        '-nosteam','-noanalytics',"-Port=$Port",'-MULTIHOME=127.0.0.1','-seconds=20',
        '-OnsetDataStoreType=SQLite',"-OnsetDataStoreURL=$database",'-AuthMode=Token',
        '-ini:Engine:[Onset.Auth]:AuthTokenSecret=mh-local-startup-test-only',
        '-ini:Engine:[OnlineSubsystem]:DefaultPlatformService=Null',
        '-ini:Engine:[OnlineSubsystemSteam]:bEnabled=false',"-abslog=$serverLog")
    # Start-Process joins arguments on Windows; quote each known argument to preserve spaces.
    if ($arguments | Where-Object { $_ -match '["\r\n]' }) { throw 'Unsupported quote/newline in launch path' }
    $argumentLine = ($arguments | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $serverProcess = Start-Process -FilePath $serverExe -ArgumentList $argumentLine -PassThru `
        -WindowStyle Hidden -WorkingDirectory (Split-Path $serverExe)
    $deadline = [DateTime]::UtcNow.AddSeconds($StartupTimeoutSeconds)
    while (!$serverProcess.HasExited -and [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 500
        $serverProcess.Refresh()
    }
    if (!$serverProcess.HasExited) { throw "Server did not exit before the timeout. See $serverLog" }
    $serverProcess.WaitForExit()
    if ($serverProcess.ExitCode -ne 0) { throw "Server exit code $($serverProcess.ExitCode). See $serverLog" }
    $logText = Get-Content -LiteralPath $serverLog -Raw
    $checks = [ordered]@{
        LocalStore = $logText.Contains('initialized with SQLite (success=1)')
        TokenAuth = $logText.Contains('AuthSubsystem initialized: AuthMode=Token')
        Listening = $logText.Contains("IpNetDriver listening on port $Port")
        WorldStarted = $logText.Contains('Bringing up level for play took:')
        LocalDatabaseCreated = (Test-Path -LiteralPath $database -PathType Leaf)
        NoFallback = ($logText -notmatch 'falling back to SQLite')
        NoFatalError = ($logText -notmatch 'Fatal error:|Assertion failed:|Unhandled Exception:')
    }
    $failedChecks = @($checks.Keys | Where-Object { !$checks[$_] })
    [pscustomobject]@{ Passed=($failedChecks.Count -eq 0); Checks=$checks; Package=$PackageRoot;
        Log=$serverLog; Scope='Cooked server startup only; no client login or MH provider migration tested' } |
        ConvertTo-Json -Depth 4 | Set-Content (Join-Path $runRoot 'result.json')
    if ($failedChecks.Count) { throw "Startup checks failed: $($failedChecks -join ', '). See $serverLog" }
    Write-Host "PASS: cooked server startup. Results: $runRoot"
}
finally {
    # Only stop the process owned by this invocation, never other editors/servers.
    if ($serverProcess -and !$serverProcess.HasExited) { $serverProcess.Kill(); $serverProcess.WaitForExit() }
    if ($serverProcess) { $serverProcess.Dispose() }
    $env:MH_POSTGRES_ROOT = $previousSdk
    Pop-Location
}
