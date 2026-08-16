param(
    [int]$LoginPort = 7777,
    [int]$GamePort = 7778,
    [string]$EnginePath = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectPath = "E:\Unreal Projects\PortfolioTutorial\Project\Onset.uproject",
    [string]$DataStoreType = "HttpApi",
    [string]$DataStoreURL = "",
    [string]$DataStoreAPIKey = "",
    [string]$AuthTokenSecret = "",
    [switch]$Cloud,
    [int]$AutoPlaySlot = -1
)

$CloudEndpoint = "qnghmsigrompw56v5wrlojl7z40dwtqm.lambda-url.us-east-1.on.aws/"
$LocalEndpoint = "http://localhost:3000"

if ($Cloud) {
    if ([string]::IsNullOrWhiteSpace($DataStoreURL)) { $DataStoreURL = $CloudEndpoint }
    if ([string]::IsNullOrWhiteSpace($DataStoreAPIKey)) { $DataStoreAPIKey = "dev-api-key-change-me-in-production" }
    if ([string]::IsNullOrWhiteSpace($AuthTokenSecret)) { $AuthTokenSecret = "change-me-in-production" }
}
else {
    if ([string]::IsNullOrWhiteSpace($DataStoreURL)) { $DataStoreURL = $LocalEndpoint }
    if ([string]::IsNullOrWhiteSpace($DataStoreAPIKey)) { $DataStoreAPIKey = "dev-api-key-change-me-in-production" }
    if ([string]::IsNullOrWhiteSpace($AuthTokenSecret)) { $AuthTokenSecret = "local-dev-shared-secret" }
}

function Ensure-LocalDynamoDB {
    if (Get-NetTCPConnection -State Listen -LocalPort 8000 -ErrorAction SilentlyContinue) {
        Write-Host "DynamoDB Local already listening on :8000" -ForegroundColor DarkGray
        return
    }
    if (!(Get-Command docker -ErrorAction SilentlyContinue)) {
        Write-Host "WARNING: DynamoDB Local is not running and 'docker' is unavailable." -ForegroundColor Yellow
        Write-Host "  Start it yourself (see Project/AccountApi/LOCAL.md) or local data store tests will fail." -ForegroundColor Yellow
        return
    }

    $ComposeFile = Join-Path $PSScriptRoot "Project\AccountApi\.local\dynamodb-local\docker-compose.yml"
    if (Test-Path -LiteralPath $ComposeFile) {
        Write-Host "Starting DynamoDB Local via docker compose..." -ForegroundColor Cyan
        docker compose -f $ComposeFile up -d dynamodb-local
    }
    else {
        Write-Host "Compose file not found; starting DynamoDB Local container 'onset-ddb'..." -ForegroundColor Cyan
        $existing = docker ps -a --filter "name=^onset-ddb$" --format "{{.Names}}" | Select-Object -First 1
        if ($existing) {
            docker start onset-ddb | Out-Null
        }
        else {
            docker run -d --name onset-ddb -v onset-ddb-data:/home/dynamodblocal/data -p 8000:8000 amazon/dynamodb-local:latest | Out-Null
        }
    }

    for ($i = 0; $i -lt 60; $i++) {
        if (Get-NetTCPConnection -State Listen -LocalPort 8000 -ErrorAction SilentlyContinue) {
            Write-Host "DynamoDB Local ready on :8000 (after ${i}s)" -ForegroundColor Green
            return
        }
        Start-Sleep 1
    }
    Write-Host "WARNING: DynamoDB Local did not come up on :8000; local data store tests will fail." -ForegroundColor Yellow
}

function Ensure-LocalTable {
    if (!(Get-Command aws -ErrorAction SilentlyContinue)) {
        Write-Host "WARNING: 'aws' not found; cannot verify/create local table." -ForegroundColor Yellow
        Write-Host "  Run 'npm run db:local:create' in Project/AccountApi first." -ForegroundColor Yellow
        return
    }
    for ($i = 0; $i -lt 10; $i++) {
        aws dynamodb describe-table --endpoint-url http://localhost:8000 --table-name onset-accounts-dev *> $null
        if ($LASTEXITCODE -eq 0) { return }
        Start-Sleep 2
    }
    Write-Host "Creating local table 'onset-accounts-dev' (provisioned 25/25)..." -ForegroundColor Cyan
    for ($i = 0; $i -lt 15; $i++) {
        aws dynamodb create-table --endpoint-url http://localhost:8000 --table-name onset-accounts-dev `
            --attribute-definitions AttributeName=PK,AttributeType=S AttributeName=SK,AttributeType=S `
            --key-schema AttributeName=PK,KeyType=HASH AttributeName=SK,KeyType=RANGE `
            --billing-mode PROVISIONED `
            --provisioned-throughput ReadCapacityUnits=25,WriteCapacityUnits=25 *> $null
        if ($LASTEXITCODE -eq 0) { break }
        Start-Sleep 2
    }
    aws dynamodb describe-table --endpoint-url http://localhost:8000 --table-name onset-accounts-dev *> $null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Local table ready." -ForegroundColor Green
    }
    else {
        Write-Host "WARNING: could not create local table; run 'npm run db:local:create' in AccountApi." -ForegroundColor Yellow
    }
}

if (!$Cloud) {
    Ensure-LocalDynamoDB
    Ensure-LocalTable
}

$EngineBin = Join-Path $EnginePath "Engine\Binaries\Win64\UnrealEditor.exe"
$ProjectDir = Split-Path $ProjectPath -Parent
$LogDir = "$ProjectDir\Saved\Logs"
$ExitLog = "$LogDir\ProcessExitCodes.log"

if (!(Test-Path $EngineBin)) {
    Write-Host "UnrealEditor.exe not found at $EngineBin" -ForegroundColor Red
    exit 1
}
if (!(Test-Path $ProjectPath)) {
    Write-Host "Project file not found at $ProjectPath" -ForegroundColor Red
    exit 1
}
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

$LogFilter = "LogOnsetAuth Verbose, LogTemp Verbose, LogOnsetPlayerData Verbose, LogSteamAuth Verbose"

$LoginLog = "$LogDir\LoginServer.log"
$GameLog  = "$LogDir\GameServer.log"
$ClientLog = "$LogDir\Client.log"

$DataStoreArgs = " -OnsetDataStoreType=$DataStoreType -OnsetDataStoreURL=$DataStoreURL" +
                 " -OnsetDataStoreAPIKey=$DataStoreAPIKey -OnsetAuthTokenSecret=$AuthTokenSecret"

$DataStoreMode = if ($Cloud) { "CLOUD" } else { "LOCAL" }
Write-Host "DataStore mode: $DataStoreMode -> $DataStoreURL" -ForegroundColor Cyan

$LoginCmd = "-project=`"$ProjectPath`" /Game/Maps/LoginServer -server -log -AuthMode=Direct -Port=$LoginPort -NOSTEAM" +
            " -AbsLog=`"$LoginLog`" -LogCmds=`"$LogFilter`"" + $DataStoreArgs

$GameCmd = "-project=`"$ProjectPath`" /Game/Maps/DemoLevel -server -log -AuthMode=Token -Port=$GamePort -NOSTEAM" +
           " -AbsLog=`"$GameLog`" -LogCmds=`"$LogFilter`"" + $DataStoreArgs

$ClientBaseCmd = "-project=`"$ProjectPath`" 127.0.0.1:${LoginPort}?ClientIndex={1} -game -log -windowed -ResX=1280 -ResY=720 -NOSTEAM" +
                 " -AbsLog=`"{0}`" -LogCmds=`"$LogFilter`""
if ($AutoPlaySlot -ge 0) {
    $ClientBaseCmd += " -AutoPlaySlot=$AutoPlaySlot"
}

function Write-ExitLog {
    param([string]$Line)
    $stamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Add-Content -Path $ExitLog -Value "[$stamp] $Line"
}

function Describe-ExitCode {
    param([int]$Code)
    # Exit codes are signed Int32 (e.g. STATUS_CONTROL_C_EXIT == 0xC000013A == -1073741510).
    # PowerShell parses hex literals as signed Int32 too, so compare directly.
    switch ($Code) {
        0             { return "graceful exit" }
        -1            { return "killed (TerminateProcess)" }
        0xC0000005    { return "ACCESS_VIOLATION (crash)" }
        0xC0000409    { return "STACK_BUFFER_OVERRUN (crash)" }
        0xC0000135    { return "DLL_NOT_FOUND" }
        0xC000000D    { return "INVALID_PARAMETER" }
        0xC00000FD    { return "STACK_OVERFLOW (crash)" }
        0xC000001D    { return "ILLEGAL_INSTRUCTION (crash)" }
        0xC000013A    { return "CTRL_C_EXIT (console Ctrl+C or window close)" }
        default {
            $u = [int64]$Code
            if ($u -lt 0) { $u += 4294967296 }
            if ($u -ge 3221225472) { return "NTSTATUS 0x$($u.ToString('X8')) (crash)" }
            return "exit code $Code"
        }
    }
}

# Processes started so far and an exit-code watchdog that polls while we wait for input.
$procs = @()
$TrackedProcs = [System.Collections.Generic.List[object]]::new()

function Add-TrackedProc {
    param($Proc, [string]$Label)
    $started = Get-Date
    $TrackedProcs.Add([pscustomobject]@{ Proc = $Proc; Label = $Label; Started = $started })
    Write-ExitLog "START label=$Label pid=$($Proc.Id) started=$($started.ToString('HH:mm:ss'))"
    Write-Host "  Watching $Label (PID $($Proc.Id)) for exit code" -ForegroundColor DarkGray
}

function Update-TrackedProcs {
    if ($TrackedProcs.Count -eq 0) { return }
    $toRemove = [System.Collections.Generic.List[object]]::new()
    foreach ($t in $TrackedProcs) {
        if ($t.Proc.HasExited) {
            $code = $t.Proc.ExitCode
            $desc = Describe-ExitCode -Code $code
            Write-Host "  [exit] $($t.Label) (PID $($t.Proc.Id)) -> $code ($desc)" -ForegroundColor Yellow
            Write-ExitLog "EXIT  label=$($t.Label) pid=$($t.Proc.Id) code=$code desc=$desc started=$($t.Started.ToString('HH:mm:ss'))"
            $toRemove.Add($t)
        }
    }
    foreach ($t in $toRemove) { $TrackedProcs.Remove($t) }
}

Write-ExitLog "=== Test_All session started ==="

Write-Host "=== Phase 1: Launching Login Server (port $LoginPort) ===" -ForegroundColor Cyan
Write-Host "  $EngineBin $LoginCmd" -ForegroundColor Gray
$p = Start-Process -FilePath $EngineBin -ArgumentList $LoginCmd -WindowStyle Normal -PassThru
$procs += $p
Add-TrackedProc $p "LoginServer"
Start-Sleep 10

Write-Host "=== Phase 2: Launching Game Server (port $GamePort) ===" -ForegroundColor Cyan
Write-Host "  $EngineBin $GameCmd" -ForegroundColor Gray
$p = Start-Process -FilePath $EngineBin -ArgumentList $GameCmd -WindowStyle Normal -PassThru
$procs += $p
Add-TrackedProc $p "GameServer"
Start-Sleep 10

Write-Host "=== Phase 3: Launching First Client ===" -ForegroundColor Cyan
$clientCount = 1
$clientCmd = $ClientBaseCmd -f $ClientLog, $clientCount
Write-Host "  $EngineBin $clientCmd" -ForegroundColor Gray
$p = Start-Process -FilePath $EngineBin -ArgumentList $clientCmd -WindowStyle Normal -PassThru
$procs += $p
Add-TrackedProc $p "Client 1"

Write-Host "`nLog files:" -ForegroundColor Green
Write-Host "  LoginServer -> $LoginLog" -ForegroundColor Gray
Write-Host "  GameServer  -> $GameLog" -ForegroundColor Gray
Write-Host "  Client 1    -> $ClientLog" -ForegroundColor Gray
Write-Host "  Exit codes  -> $ExitLog" -ForegroundColor Gray

Write-Host "`nInteractive controls:" -ForegroundColor Green
Write-Host "  [Enter]  Launch another client" -ForegroundColor Gray
Write-Host "  [Q]      Quit and clean up all processes" -ForegroundColor Gray
Write-Host "  Exit codes are logged to ProcessExitCodes.log as processes die." -ForegroundColor DarkGray

try {
    $running = $true
    while ($running) {
        Update-TrackedProcs
        if ([Console]::KeyAvailable) {
            $keyInfo = [Console]::ReadKey($true)
            if ($keyInfo.Key -eq [ConsoleKey]::Enter) {
                $clientCount++
                $clientLog = "$LogDir\Client_$clientCount.log"
                $clientCmd = $ClientBaseCmd -f $clientLog, $clientCount
                Write-Host "Launching client $clientCount..." -ForegroundColor Cyan
                Write-Host "  $EngineBin $clientCmd" -ForegroundColor Gray
                $p = Start-Process -FilePath $EngineBin -ArgumentList $clientCmd -WindowStyle Normal -PassThru
                $procs += $p
                Add-TrackedProc $p "Client $clientCount"
                Write-Host "  Client $clientCount -> $clientLog" -ForegroundColor Gray
            }
            elseif ($keyInfo.Key -eq [ConsoleKey]::Q) {
                $running = $false
            }
        }
        Start-Sleep -Milliseconds 500
    }

    Write-Host "`nQuit requested." -ForegroundColor Yellow
}
catch {
    Write-Host "`nCaught exception: $_" -ForegroundColor Red
    Write-ExitLog "EXCEPTION $_"
}
finally {
    Write-Host "`nCleaning up all processes..." -ForegroundColor Yellow
    foreach ($p in $procs) {
        if (!$p.HasExited) {
            Write-Host "  Stopping PID $($p.Id)..." -ForegroundColor Gray
            $p.Kill()
            $p.WaitForExit(5000) | Out-Null
            $code = $p.ExitCode
            Write-ExitLog "KILLED pid=$($p.Id) code=$code desc=$(Describe-ExitCode -Code $code)"
        }
    }
    Update-TrackedProcs
    Write-ExitLog "=== Test_All session ended ==="
    Write-Host "Done." -ForegroundColor Green
}
