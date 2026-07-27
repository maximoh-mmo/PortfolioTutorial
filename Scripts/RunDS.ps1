<#
.SYNOPSIS
    Launch Dedicated Server + optional client instances for Onset.
.DESCRIPTION
    Usage scenarios:
      .\RunDS.ps1                            # DS + 1 client (editor)
      .\RunDS.ps1 -ClientCount 2             # DS + 2 clients
      .\RunDS.ps1 -ClientCount 2 -Standalone # DS + 2 clients (standalone Game target)
      .\RunDS.ps1 -NoDS -ClientCount 2       # PIE-style: just 2 clients

    The DS uses the Game target (Onset.exe) with -server flag.
    Engine distribution does not support the Server target type.
.NOTES
    Standalone mode requires cooked content or a running Zen Storage Server.
    For development, prefer PIE multi-player from the editor.
#>

param(
    [int]$ClientCount = 1,
    [string]$MapPath = "/Game/DemoLevel",
    [string]$ProjectPath = "E:\Unreal Projects\PortfolioTutorial\Project\Onset.uproject",
    [switch]$NoDS,
    [switch]$Standalone,
    [ValidateSet("Game", "Lobby")]
    [string]$Mode = "Game"
)

$EngineDir = "C:\Program Files\Epic Games\UE_5.8"

if ($Standalone) {
    # Standalone Game target (Onset.exe) — needs Zen Server or cooked content
    $DSBinary = "E:\Unreal Projects\PortfolioTutorial\Project\Binaries\Win64\Onset.exe"
    $ClientBinary = "$EngineDir\Engine\Binaries\Win64\Onset.exe"

    # Start Zen Storage Server if not already running
    if (-not (Get-Process -Name "zenserver" -ErrorAction SilentlyContinue)) {
        Write-Host "Starting Zen Storage Server..." -ForegroundColor Yellow
        $ZenDataDir = "$env:LOCALAPPDATA\UnrealEngine\Common\Zen\Data"
        Start-Process -FilePath "$EngineDir\Engine\Binaries\Win64\zenserver.exe" `
            -ArgumentList "--port 8558 --data-dir ""$ZenDataDir"" --http asio --quiet" `
            -WindowStyle Hidden
        Start-Sleep -Seconds 3
    }

    if (-not $NoDS) {
        Write-Host "Launching Dedicated Server (standalone)..." -ForegroundColor Green
        $dsArgs = """$ProjectPath"" $MapPath -server -log"
        Start-Process -FilePath $DSBinary -ArgumentList $dsArgs -WindowStyle Normal
        Start-Sleep -Seconds 5
    }

    for ($i = 1; $i -le $ClientCount; $i++) {
        Write-Host "Launching Client $i (standalone)..." -ForegroundColor Cyan
        $clientArgs = """$ProjectPath"" $MapPath -game -ResX=1280 -ResY=720 -WinX=$((($i - 1) * 1300) % 2560) -WinY=0 -log"
        Start-Process -FilePath $ClientBinary -ArgumentList $clientArgs -WindowStyle Normal
        Start-Sleep -Seconds 3
    }
}
else {
    # Editor-based (UnrealEditor.exe) — simpler, auto-cooks
    $Binary = "$EngineDir\Engine\Binaries\Win64\UnrealEditor.exe"

if (-not $NoDS) {
    $ServerMap = "/Game/Maps/MainMenu"
    if ($Mode -eq "Game")
    {
        $ServerMap = $MapPath
    }

    Write-Host "Launching Dedicated Server ($Mode - $ServerMap)..." -ForegroundColor Green
    $dsArgs = """$ProjectPath"" $ServerMap?Listen -server -log -NoLiveCoding"
    Start-Process -FilePath $Binary -ArgumentList $dsArgs -WindowStyle Normal
    Start-Sleep -Seconds 8
}

for ($i = 1; $i -le $ClientCount; $i++) {
    Write-Host "Launching Client $i ($Mode)..." -ForegroundColor Cyan
    $clientArgs = """$ProjectPath"" /Game/Maps/MainMenu -game -ResX=1280 -ResY=720 -WinX=$((($i - 1) * 1300) % 2560) -WinY=0 -log -NoLiveCoding"
    Start-Process -FilePath $Binary -ArgumentList $clientArgs -WindowStyle Normal
    Start-Sleep -Seconds 3
}
}

Write-Host "Done. Mode=$Mode, $ClientCount client(s)" $(if (-not $NoDS) { "+ DS" }) -ForegroundColor Green
