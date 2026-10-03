[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$project = Join-Path $repo 'Project/Onset.uproject'
$engine = (Resolve-Path -LiteralPath $EngineRoot).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw 'Choose a new output directory to avoid mixing packages.' }
New-Item -ItemType Directory -Path $output | Out-Null
function RunStep([string]$File, [string[]]$Arguments, [string]$Log) {
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $File @Arguments 2>&1 | Tee-Object -FilePath $Log
        $stepExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($stepExit -ne 0) { throw "Step failed ($stepExit). See $Log" }
}
Push-Location -LiteralPath $repo
try {
    if (!$SkipBuild) {
        RunStep (Join-Path $engine 'Engine/Build/BatchFiles/Build.bat') @('OnsetClient', 'Win64', 'Development',
            $project, '-CustomConfig=MHLanTest', '-WaitMutex', '-NoHotReloadFromIDE', '-NoUBA') (Join-Path $output 'client-build.log')
    }
    $receipt = Get-Content -LiteralPath (Join-Path $repo 'Project/Binaries/Win64/OnsetClient.target') -Raw | ConvertFrom-Json
    if (!($receipt.AdditionalProperties | Where-Object { $_.Name -eq 'CustomConfig' -and $_.Value -eq 'MHLanTest' })) {
        throw 'Client receipt must use CustomConfig=MHLanTest so packaging applies the test credential filters.'
    }
    RunStep (Join-Path $engine 'Engine/Build/BatchFiles/RunUAT.bat') @("-ScriptsForProject=$project", 'BuildCookRun',
        "-project=$project", '-target=OnsetClient', '-platform=Win64', '-clientconfig=Development',
        '-noP4', '-unattended', '-utf8output', '-nocompileeditor', '-cook', '-stage', '-pak', '-archive',
        '-prereqs', '-nodebuginfo', '-AdditionalCookerOptions=-CustomConfig=MHLanTest',
        '-map=/Game/Maps/MainMenu+/Game/Maps/LoginServer+/Game/Maps/DemoLevel+/Engine/Maps/Entry',
        "-stagingdirectory=$output/stage", "-archivedirectory=$output/archive") (Join-Path $output 'package.log')
    $package = Join-Path $output 'archive/WindowsClient'
    if (!(Test-Path -LiteralPath (Join-Path $package 'Onset/Binaries/Win64/OnsetClient.exe'))) {
        throw 'Expected WindowsClient executable was not archived.'
    }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Start-LanClient.ps1') -Destination $package
    $redist = Join-Path $engine 'Engine/Extras/Redist/en-us/vc_redist.x64.exe'
    if (!(Test-Path -LiteralPath $redist)) { throw "Missing client prerequisite installer: $redist" }
    $redistDestination = Join-Path $package 'Engine/Extras/Redist/en-us'
    New-Item -ItemType Directory -Force -Path $redistDestination | Out-Null
    Copy-Item -LiteralPath $redist -Destination $redistDestination
    Copy-Item -LiteralPath (Join-Path $repo 'Docs/Framework/LanTesting.md') -Destination (Join-Path $package 'README-LAN.md')
    & (Join-Path $PSScriptRoot 'Verify-LanPackage.ps1') -EngineRoot $engine -PackageRoot $package -InspectionDirectory (Join-Path $output 'config-inspection')
    Write-Host "Packaged client: $package. Run Steam smoke before distribution."
} finally { Pop-Location }
