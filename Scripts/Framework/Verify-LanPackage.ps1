[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$PackageRoot,
    [Parameter(Mandatory=$true)][string]$InspectionDirectory
)
$ErrorActionPreference = 'Stop'
$package = (Resolve-Path -LiteralPath $PackageRoot).Path
$inspect = [IO.Path]::GetFullPath($InspectionDirectory)
if (Test-Path -LiteralPath $inspect) { throw 'Choose a new inspection directory.' }
$unrealPak = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealPak.exe'
if (!(Test-Path -LiteralPath $unrealPak)) { throw "UnrealPak is missing: $unrealPak" }
$paks = @(Get-ChildItem -LiteralPath (Join-Path $package 'Onset/Content/Paks') -Filter '*.pak' -File)
if (!$paks.Count) { throw 'No client pak found.' }
New-Item -ItemType Directory -Path $inspect | Out-Null
foreach ($pak in $paks) {
    $destination = Join-Path $inspect $pak.BaseName
    & $unrealPak $pak.FullName '-Extract' $destination '-Filter=*.ini' *> (Join-Path $inspect ($pak.BaseName + '.log'))
    if ($LASTEXITCODE -ne 0) { throw "Failed to inspect $($pak.Name)" }
}
$iniFiles = @(Get-ChildItem -LiteralPath $inspect -Filter '*.ini' -File -Recurse)
$iniFiles += @(Get-ChildItem -LiteralPath $package -Filter '*.ini' -File -Recurse)
if (!$iniFiles.Count) { throw 'No INI files were inspected; cannot verify credential filtering.' }
$badFiles = @()
foreach ($file in $iniFiles) {
    if (Select-String -LiteralPath $file.FullName -Pattern '^\s*[+.!-]?(AuthTokenSecret|APIKey|SecurityToken|ConnectionString|Password|ClientSecret|EncryptionKey)\s*=' -Quiet) {
        $badFiles += $file.Name
    }
}
if ($badFiles.Count) { throw "Credential keys remain in packaged config: $($badFiles -join ', '). Values withheld." }
if (Get-ChildItem -LiteralPath $package -Filter 'libpq.dll' -Recurse -File) { throw 'Unexpected PostgreSQL runtime in client-only package.' }
[pscustomobject]@{ Passed=$true; IniFilesInspected=$iniFiles.Count; PakCount=$paks.Count; Package=$package } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $inspect 'result.json')
Write-Host "PASS: inspected $($iniFiles.Count) packaged INI files; credential keys and PostgreSQL runtime absent."
