$EngineDir = "C:\Program Files\Epic Games\UE_5.8"
$ProjectPath = "$PSScriptRoot\Project\Onset.uproject"
$ClientPath = "$PSScriptRoot\Package\Windows\Onset.exe"

Write-Host "=== Onset Launch Script ===" -ForegroundColor Cyan

# 1. Kill any lingering processes
Get-Process -Name "UnrealEditor-Cmd","Onset" -ErrorAction SilentlyContinue | Stop-Process -Force

# 2. Start Dedicated Server (editor Cmd process)
Write-Host "[1/2] Starting Dedicated Server..." -ForegroundColor Green
$ServerProc = Start-Process -FilePath "$EngineDir\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    -ArgumentList "`"$ProjectPath`" /Game/DemoLevel.DemoLevel?game=/Script/Onset.OnsetLobbyGameMode -server -log -PORT=7777" `
    -WindowStyle Normal -PassThru
Write-Host "  Server PID: $($ServerProc.Id)" -ForegroundColor Gray

# 3. Wait for server to initialize
Write-Host "[...] Waiting 12s for server to initialize..." -ForegroundColor Yellow
Start-Sleep -Seconds 12

# 4. Check if server is still alive
if (-not (Get-Process -Id $ServerProc.Id -ErrorAction SilentlyContinue)) {
    Write-Host "  Server exited prematurely! Check server log for errors." -ForegroundColor Red
    exit 1
}

# 5. Launch Client
Write-Host "[2/2] Launching Client..." -ForegroundColor Green
Start-Process -FilePath $ClientPath -ArgumentList "-windowed -ResX=1280 -ResY=720"

Write-Host "=== Done ===" -ForegroundColor Cyan
Write-Host "Server log window should be visible. Client will open in a new window."
Write-Host "Press Enter in the client to connect to the server."