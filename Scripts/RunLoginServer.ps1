<#
.SYNOPSIS
    Launch the Login Server (OnsetLoginServer target).
.DESCRIPTION
    The Login Server is a minimal server that authenticates players via
    Steam auth ticket, issues a signed session token, then kicks the player.
    Clients reconnect to the Game Server using the token.
.NOTES
    The Login Server requires:
      - [Onset.Auth] AuthMode=Token in DefaultEngine.ini
      - A valid AuthTokenSecret configured
      - Steam AppID 480 (Spacewar) for dev
    Standalone mode requires cooked content or a running Zen Storage Server.
#>

param(
    [string]$MapPath = "/Game/Maps/LoginServer",
    [string]$ProjectPath = "E:\Unreal Projects\PortfolioTutorial\Project\Onset.uproject",
    [string]$EngineDir = "C:\Program Files\Epic Games\UE_5.8"
)

# Check which binary to use
$DSBinary = "E:\Unreal Projects\PortfolioTutorial\Project\Binaries\Win64\Onset.exe"
$EditorBinary = "$EngineDir\Engine\Binaries\Win64\UnrealEditor.exe"

if (Test-Path $DSBinary) {
    Write-Host "Launching Login Server (standalone)..." -ForegroundColor Green
    $args = """$ProjectPath"" $MapPath -server -log -NoLiveCoding"
    Start-Process -FilePath $DSBinary -ArgumentList $args -WindowStyle Normal
}
else {
    Write-Host "No standalone binary found at $DSBinary" -ForegroundColor Yellow
    Write-Host "Falling back to editor-based launch..." -ForegroundColor Yellow
    
    if (-not (Get-Process -Name "zenserver" -ErrorAction SilentlyContinue)) {
        Write-Host "Starting Zen Storage Server..." -ForegroundColor Yellow
        $ZenDataDir = "$env:LOCALAPPDATA\UnrealEngine\Common\Zen\Data"
        Start-Process -FilePath "$EngineDir\Engine\Binaries\Win64\zenserver.exe" `
            -ArgumentList "--port 8558 --data-dir ""$ZenDataDir"" --http asio --quiet" `
            -WindowStyle Hidden
        Start-Sleep -Seconds 3
    }

    $args = """$ProjectPath"" $MapPath?Listen -server -log -NoLiveCoding"
    Start-Process -FilePath $EditorBinary -ArgumentList $args -WindowStyle Normal
}

Write-Host "Login Server launched. Map: $MapPath" -ForegroundColor Green
