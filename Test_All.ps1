param(
    [int]$LoginPort = 7777,
    [int]$GamePort = 7778,
    [string]$EnginePath = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectPath = "E:\Unreal Projects\PortfolioTutorial\Project\Onset.uproject"
)

$EngineBin = Join-Path $EnginePath "Engine\Binaries\Win64\UnrealEditor.exe"
$ProjectDir = Split-Path $ProjectPath -Parent
$LogDir = "$ProjectDir\Saved\Logs"

if (!(Test-Path $EngineBin)) {
    Write-Host "UnrealEditor.exe not found at $EngineBin" -ForegroundColor Red
    exit 1
}
if (!(Test-Path $ProjectPath)) {
    Write-Host "Project file not found at $ProjectPath" -ForegroundColor Red
    exit 1
}
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

$LogFilter = "LogOnsetAuth Verbose, LogTemp Verbose, LogOnsetPlayerData Verbose, LogSteamAuth Verbose, Global Warning"

$LoginLog = "$LogDir\LoginServer.log"
$GameLog  = "$LogDir\GameServer.log"
$ClientLog = "$LogDir\Client.log"

$LoginCmd = "-project=`"$ProjectPath`" /Game/Maps/LoginServer -server -log -AuthMode=Direct -Port=$LoginPort -NOSTEAM" +
            " -AbsLog=`"$LoginLog`" -LogCmds=`"$LogFilter`""

$GameCmd = "-project=`"$ProjectPath`" /Game/DemoLevel -server -log -AuthMode=Token -Port=$GamePort -NOSTEAM" +
           " -AbsLog=`"$GameLog`" -LogCmds=`"$LogFilter`""

$ClientBaseCmd = "-project=`"$ProjectPath`" 127.0.0.1:$LoginPort -game -log -windowed -ResX=1280 -ResY=720 -NOSTEAM" +
                 " -AbsLog=`"{0}`" -LogCmds=`"$LogFilter`""

Write-Host "=== Phase 1: Launching Login Server (port $LoginPort) ===" -ForegroundColor Cyan
Write-Host "  $EngineBin $LoginCmd" -ForegroundColor Gray
$procs = @()
$procs += Start-Process -FilePath $EngineBin -ArgumentList $LoginCmd -WindowStyle Normal -PassThru
Start-Sleep 10

Write-Host "=== Phase 2: Launching Game Server (port $GamePort) ===" -ForegroundColor Cyan
Write-Host "  $EngineBin $GameCmd" -ForegroundColor Gray
$procs += Start-Process -FilePath $EngineBin -ArgumentList $GameCmd -WindowStyle Normal -PassThru
Start-Sleep 10

Write-Host "=== Phase 3: Launching First Client ===" -ForegroundColor Cyan
$clientCmd = $ClientBaseCmd -f $ClientLog
Write-Host "  $EngineBin $clientCmd" -ForegroundColor Gray
$procs += Start-Process -FilePath $EngineBin -ArgumentList $clientCmd -WindowStyle Normal -PassThru

$clientCount = 1

Write-Host "`nLog files:" -ForegroundColor Green
Write-Host "  LoginServer -> $LoginLog" -ForegroundColor Gray
Write-Host "  GameServer  -> $GameLog" -ForegroundColor Gray
Write-Host "  Client 1    -> $ClientLog" -ForegroundColor Gray

Write-Host "`nInteractive controls:" -ForegroundColor Green
Write-Host "  [Enter]  Launch another client" -ForegroundColor Gray
Write-Host "  [Q]      Quit and clean up all processes" -ForegroundColor Gray

try {
    do {
        Write-Host "`n> " -ForegroundColor Yellow -NoNewline
        $keyInfo = [System.Console]::ReadKey($true)
        if ($keyInfo.Key -eq [ConsoleKey]::Enter) {
            $clientCount++
            $clientLog = "$LogDir\Client_$clientCount.log"
            $clientCmd = $ClientBaseCmd -f $clientLog
            Write-Host "Launching client $clientCount..." -ForegroundColor Cyan
            Write-Host "  $EngineBin $clientCmd" -ForegroundColor Gray
            $procs += Start-Process -FilePath $EngineBin -ArgumentList $clientCmd -WindowStyle Normal -PassThru
            Write-Host "  Client $clientCount -> $clientLog" -ForegroundColor Gray
        }
    } while ($keyInfo.Key -ne [ConsoleKey]::Q)

    Write-Host "`nQuit requested." -ForegroundColor Yellow
}
catch {
    Write-Host "`nCaught exception: $_" -ForegroundColor Red
}
finally {
    Write-Host "`nCleaning up all processes..." -ForegroundColor Yellow
    foreach ($p in $procs) {
        if (!$p.HasExited) {
            Write-Host "  Stopping PID $($p.Id)..." -ForegroundColor Gray
            $p.Kill()
        }
    }
    Write-Host "Done." -ForegroundColor Green
}
