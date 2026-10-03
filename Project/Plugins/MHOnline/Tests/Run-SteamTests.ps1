param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [Parameter(Mandatory = $true)][string]$ProjectFile,
    [switch]$Live
)
$ErrorActionPreference = 'Stop'
$ProjectFile = (Resolve-Path -LiteralPath $ProjectFile).Path
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (!(Test-Path -LiteralPath $Editor)) { throw "Editor executable not found: $Editor" }
$RunId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 8)
$Report = Join-Path (Split-Path -Parent $ProjectFile) "Saved\MHSteamTests\$RunId"
New-Item -ItemType Directory -Path $Report -Force | Out-Null
$TestName = if ($Live) { 'MH.Online.SteamLive' } else { 'MH.Online.SteamLifecycle' }
$EditorArgs = @($ProjectFile, '-unattended', '-nop4', '-NullRHI', '-nosound', '-nosplash',
    '-DisablePlugins=RiderLink', "-ExecCmds=Automation RunTests $TestName",
    '-TestExit=Automation Test Queue Empty', "-ReportExportPath=$Report", "-abslog=$Report\automation.log")
if ($Live) {
    # Avoid starting the host's player controller and unrelated login timers.
    $EditorArgs += @('/Engine/Maps/Entry?game=/Script/Engine.GameModeBase', '-game', '-MHSteamLiveTest')
}
& $Editor @EditorArgs
if ($LASTEXITCODE -ne 0) { throw "Editor exited with code $LASTEXITCODE; see $Report" }
$Result = Get-Content -LiteralPath (Join-Path $Report 'index.json') -Raw | ConvertFrom-Json
$Tests = @($Result.tests)
if ($Tests.Count -ne 1 -or $Tests[0].state -ne 'Success') {
    throw "Steam test failed or did not run; see $Report"
}
Write-Host "PASS: $TestName. Report: $Report"
