<#
.SYNOPSIS
    Comprehensive test runner for Onset project — validates all systems.
.DESCRIPTION
    Runs through 4 phases, from simplest (PIE validation) to full
    three-server token auth flow. Follow prompts and check logs as directed.
    Estimated total time: ~30-45 minutes.
#>

param(
    [string]$ProjectPath = "E:\Unreal Projects\PortfolioTutorial\Project\Onset.uproject",
    [string]$EngineDir = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ConfigPath = "E:\Unreal Projects\PortfolioTutorial\Project\Config\DefaultEngine.ini"
)

$Binary = "$EngineDir\Engine\Binaries\Win64\UnrealEditor.exe"

function Write-Step($Title, $Body) {
    Write-Host "`n" ("=" * 80) -ForegroundColor Cyan
    Write-Host "  $Title" -ForegroundColor Cyan
    Write-Host ("=" * 80) -ForegroundColor Cyan
    Write-Host $Body -ForegroundColor White
}

function Write-Section($Title) {
    Write-Host "`n" ("-" * 60) -ForegroundColor Yellow
    Write-Host "  $Title" -ForegroundColor Yellow
    Write-Host ("-" * 60) -ForegroundColor Yellow
}

function Read-Key {
    Write-Host "`nPress any key when ready (or Q to quit)..." -ForegroundColor Green
    $key = [Console]::ReadKey($true)
    if ($key.KeyChar -eq 'q' -or $key.KeyChar -eq 'Q') {
        Write-Host "`nAborted by user." -ForegroundColor Red
        exit
    }
}

function WaitFor-Log($Process, $Keyword, $TimeoutSeconds = 30) {
    $elapsed = 0
    while ($elapsed -lt $TimeoutSeconds) {
        $lines = Get-Content $LogFile -Tail 20 -ErrorAction SilentlyContinue
        if ($lines -match $Keyword) {
            return $true
        }
        Start-Sleep -Seconds 1
        $elapsed++
    }
    return $false
}

Write-Host @"
╔═══════════════════════════════════════════════════════════════╗
║               ONSET — COMPREHENSIVE TEST SUITE               ║
╠═══════════════════════════════════════════════════════════════╣
║  Phase 1: Quick PIE Validation         (~5 min)              ║
║  Phase 2: Direct Auth + Persistence    (~15 min)             ║
║  Phase 3: Token Auth + Login Server    (~20 min)             ║
║  Phase 4: Regression (Direct mode)     (~5 min)              ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Cyan

# ═══════════════════════════════════════════════════════════════
# PHASE 1 — Quick PIE Validation
# ═══════════════════════════════════════════════════════════════
Write-Step "PHASE 1: Quick PIE Validation" @"
  Open the editor and run PIE (standalone, no multiplayer).
  Verify these items manually in the PIE window:
"@

Write-Section "1.1 — Screen-Relative WASD"
Write-Host @"
  [ ] W moves character toward TOP of screen (not where character faces)
  [ ] S moves toward BOTTOM of screen
  [ ] A moves LEFT on screen
  [ ] D moves RIGHT on screen
  [ ] Gamepad L-Stick behaves identically (up = top of screen)
  [ ] WASD interrupts click-to-move pathfinding immediately
"@ -ForegroundColor Gray
Read-Key

Write-Section "1.2 — Click-to-Move + Targeting"
Write-Host @"
  [ ] Click on ground → character moves to location
  [ ] Click on enemy → enemy is targeted (target indicator visible)
  [ ] Click on ground while targeting → move to location, targeting cleared
  [ ] Tap (touch) behaves the same as click
"@ -ForegroundColor Gray
Read-Key

Write-Section "1.3 — Basic Combat"
Write-Host @"
  [ ] Target an enemy → basic attack fires (ability activates)
  [ ] NPC reacts (hit reaction, health decreases)
  [ ] NPC dies → corpse appears → respawns after delay
  [ ] Player AI autoplay (idle 20s) → auto-combat engages
  [ ] WASD/click interrupts auto-combat
"@ -ForegroundColor Gray
Read-Key

# ═══════════════════════════════════════════════════════════════
# PHASE 2 — Direct Auth Mode (DS + Client)
# ═══════════════════════════════════════════════════════════════
Write-Step "PHASE 2: Direct Auth + Persistence" @"
  Tests the default flow: Steam auth → account load/create → character select → enter world → save on disconnect.

  Config is already set to:
    [Onset.Auth] AuthMode=Direct
    [Onset.DataStore] Type=HttpApi

  You'll need the Account API Lambda running (already deployed).
  Verify it's up with: curl https://<lambda-url>/health
"@

Write-Section "2.0 — Verify Account API is Reachable"
$LambdaUrl = (Select-String -Path $ConfigPath -Pattern "ConnectionString=(.+)" | ForEach-Object { $_.Matches.Groups[1].Value.Trim() })
if ($LambdaUrl) {
    Write-Host "Account API URL from config: https://$LambdaUrl" -ForegroundColor Gray
    try {
        $response = Invoke-WebRequest -Uri "https://$LambdaUrl/health" -UseBasicParsing -TimeoutSec 5
        if ($response.StatusCode -eq 200) {
            Write-Host "  [PASS] Account API is reachable: $($response.Content)" -ForegroundColor Green
        }
    } catch {
        Write-Host "  [WARN] Account API not reachable: $_" -ForegroundColor Yellow
        Write-Host "  Tests will still work but will use the Lambda. Check CloudWatch logs." -ForegroundColor Yellow
    }
} else {
    Write-Host "  [WARN] Could not parse ConnectionString from config" -ForegroundColor Yellow
}
Read-Key

Write-Section "2.1 — Launch DS + Client"
Write-Host @"
  Launching DS (Terminal 1) + 1 client (Terminal 2) via RunDS.ps1
  Wait 10 seconds for DS to start...
"@ -ForegroundColor Gray

Write-Host "Launching DS..." -ForegroundColor Green
$dsArgs = """$ProjectPath"" /Game/DemoLevel?Listen -server -log -NoLiveCoding"
$dsProcess = Start-Process -FilePath $Binary -ArgumentList $dsArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

Write-Host "Launching client..." -ForegroundColor Green
$clientArgs = """$ProjectPath"" /Game/Maps/MainMenu -game -ResX=1280 -ResY=720 -WinX=100 -WinY=100 -log -NoLiveCoding"
$clientProcess = Start-Process -FilePath $Binary -ArgumentList $clientArgs -WindowStyle Normal -PassThru

Write-Host @"
  [ ] DS window opened without errors
  [ ] Client window opened and connected to DS
"@ -ForegroundColor Gray
Read-Key

Write-Section "2.2 — First-Time Login"
Write-Host @"
  Check the DS console/log for these lines:
  [ ]  PostLogin: player ... platform=Steam, id=...
  [ ]  FHttpStore initialized: BaseURL=https://...
  [ ]  Request returned 404  (first login, account not found)
  [ ]  PostLogin: auto-created account for Steam/...
  [ ]  Client opens MainMenu → clicks Connect → Character Select screen appears

  If the Lambda URL is valid, also check CloudWatch:
  [ ]  GET /account/Steam/{id} → 404
  [ ]  POST /account/Steam/{id} → 201
"@ -ForegroundColor Gray
Read-Key

Write-Section "2.3 — Character Creation + Enter World"
Write-Host @"
  In the Character Select screen:
  [ ]  Hover an empty slot → "Create Character" prompt appears
  [ ]  Click/select empty slot → enter name → create
  [ ]  Log shows: Server_CreateCharacter: created '...' in slot N
  [ ]  Log shows: Server_SelectCharacter: player ... selected slot N
  [ ]  Pawn spawns at default position (0, 0, 150)
  [ ]  You're in the game world with WASD/click-to-move working
"@ -ForegroundColor Gray
Read-Key

Write-Section "2.4 — Save-on-Disconnect"
Write-Host @"
  [ ]  Move character to a new position
  [ ]  Close the client window
  [ ]  DS logs show EndPlay → save triggered
  [ ]  Launch client again (same process)
  [ ]  Character Select shows occupied slot with correct name/level
  [ ]  Select character → enter world at last saved position
  [ ]  Lambda CloudWatch shows GET /account → GET /account/Steam/{id}/character/{slot}
"@ -ForegroundColor Gray
Read-Key

# Close DS + client from Phase 2
Write-Host "Closing Phase 2 processes..." -ForegroundColor Yellow
if ($clientProcess -and !$clientProcess.HasExited) { $clientProcess.Kill() }
if ($dsProcess -and !$dsProcess.HasExited) { $dsProcess.Kill() }
Start-Sleep -Seconds 2

# ═══════════════════════════════════════════════════════════════
# PHASE 3 — Token Auth Mode (Login Server → Game Server)
# ═══════════════════════════════════════════════════════════════
Write-Step "PHASE 3: Token Auth Mode" @"
  Tests the full live-service auth flow:
    Client → Login Server (auth + token) → Game Server (validate + play)

  Requires AuthMode=Token in config.
  The test will temporarily modify DefaultEngine.ini and restore it after.
"@

Write-Section "3.0 — Config Check"
$currentAuthMode = (Select-String -Path $ConfigPath -Pattern "AuthMode=(\w+)" | ForEach-Object { $_.Matches.Groups[1].Value })
Write-Host "  Current AuthMode: $currentAuthMode" -ForegroundColor Gray

# Backup and override config for token mode
$configBackup = Get-Content $ConfigPath -Raw
Write-Host "  Backing up config and switching to AuthMode=Token..." -ForegroundColor Yellow
$newConfig = $configBackup -replace "AuthMode=\w+", "AuthMode=Token"
if ($newConfig -notmatch "AuthTokenSecret") {
    $newConfig += "`nAuthTokenSecret=test-secret-123`n"
}
Set-Content -Path $ConfigPath -Value $newConfig
Write-Host "  [DONE] Config updated. Remember to restore after testing." -ForegroundColor Green

Write-Host @"
  Quick check: LoginServer map must exist at /Game/Maps/LoginServer
  [ ]  LoginServer map exists with AOnsetLoginServerGameMode override
"@ -ForegroundColor Gray
Read-Key

Write-Section "3.1 — Launch Login Server (Terminal 1)"
Write-Host @"
  Launching Login Server...
  Port 7778 (console) or 7777 (UnrealEditor.exe)
"@ -ForegroundColor Gray

$loginServerArgs = """$ProjectPath"" /Game/Maps/LoginServer?Listen -server -log -NoLiveCoding"
$loginServerProcess = Start-Process -FilePath $Binary -ArgumentList $loginServerArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

Write-Host @"
  Check Login Server console:
  [ ]  AuthSubsystem initialized: AuthMode=Token
  [ ]  Map loaded: LoginServer

  Expected behavior: Login Server accepts connections,
  validates Steam ticket, generates HMAC token, sends
  Client_SessionToken, then kicks player after 2s.
"@ -ForegroundColor Gray
Read-Key

Write-Section "3.2 — Launch Game Server (Terminal 2)"
Write-Host @"
  Launching Game Server with AuthMode=Token...
  This is a normal DS but configured to accept token-authed connections.
"@ -ForegroundColor Gray

$gameServerArgs = """$ProjectPath"" /Game/DemoLevel?Listen -server -log -NoLiveCoding"
$gameServerProcess = Start-Process -FilePath $Binary -ArgumentList $gameServerArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

Write-Host @"
  Check Game Server console:
  [ ]  AuthSubsystem initialized: AuthMode=Token
  [ ]  Map loaded: DemoLevel

  Expected behavior: Game Server rejects connections
  without a valid ?Token= in the URL. Only accepts connections
  that pass PreLoginTokenAuth validation.
"@ -ForegroundColor Gray
Read-Key

Write-Section "3.3 — Launch Client (Terminal 3)"
Write-Host @"
  Launching client that connects to Login Server first, then
  automatically reconnects to Game Server with the token.
  For this initial test, the client connects normally.
"@ -ForegroundColor Gray

$tokenClientArgs = """$ProjectPath"" /Game/Maps/MainMenu -game -ResX=1280 -ResY=720 -WinX=200 -WinY=200 -log -NoLiveCoding"
$tokenClientProcess = Start-Process -FilePath $Binary -ArgumentList $tokenClientArgs -WindowStyle Normal -PassThru

Write-Host @"
  Check Login Server logs:
  [ ]  PostLogin: player ... platform=Steam, id=...
  [ ]  PostLogin: sent session token to ...
  [ ]  LoginServer: kicking ... in 2.0s
  [ ]  Client receives Client_SessionToken RPC (check client log if visible)

  Check Game Server logs (after client reconnects):
  [ ]  PreLogin: token validated for ...
  [ ]  PostLogin: using token auth — Steam/...
  [ ]  PostLogin: auto-created account for Steam/...  (or loaded existing)
  [ ]  Client_AccountData sent
"@ -ForegroundColor Gray
Read-Key

Write-Section "3.4 — Token Rejection Test"
Write-Host @"
  In the Game Server console, verify no connections are
  accepted without a valid token.

  If you can connect another client without a token:
  [ ]  PreLogin: rejected connection ... — Missing session token
  [ ]  Client stays at loading screen / gets connection error
"@ -ForegroundColor Gray
Read-Key

# Close Phase 3 processes
Write-Host "Closing Phase 3 processes..." -ForegroundColor Yellow
if ($tokenClientProcess -and !$tokenClientProcess.HasExited) { $tokenClientProcess.Kill() }
if ($gameServerProcess -and !$gameServerProcess.HasExited) { $gameServerProcess.Kill() }
if ($loginServerProcess -and !$loginServerProcess.HasExited) { $loginServerProcess.Kill() }
Start-Sleep -Seconds 2

# ═══════════════════════════════════════════════════════════════
# PHASE 4 — Regression (Direct Mode)
# ═══════════════════════════════════════════════════════════════
Write-Step "PHASE 4: Regression — Direct Mode" @"
  Restore config to AuthMode=Direct and verify the original flow
  still works perfectly. This is critical: Direct mode must be
  100% unaffected by the auth subsystem extraction.
"@

# Restore config from backup
Set-Content -Path $ConfigPath -Value $configBackup
Write-Host "  [DONE] Config restored to original (AuthMode=Direct)" -ForegroundColor Green

Write-Section "4.1 — Launch DS + Client"
Write-Host "Re-launching DS + 1 client (same as Phase 2)..." -ForegroundColor Gray
$regressionDSArgs = """$ProjectPath"" /Game/DemoLevel?Listen -server -log -NoLiveCoding"
$regressionDSProcess = Start-Process -FilePath $Binary -ArgumentList $regressionDSArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

$regressionClientArgs = """$ProjectPath"" /Game/Maps/MainMenu -game -ResX=1280 -ResY=720 -WinX=100 -WinY=100 -log -NoLiveCoding"
$regressionClientProcess = Start-Process -FilePath $Binary -ArgumentList $regressionClientArgs -WindowStyle Normal -PassThru

Write-Host @"
  [ ]  DS starts without errors
  [ ]  Client connects, Character Select appears
  [ ]  Slot shows previously created character (persisted from Phase 2)
  [ ]  Create a new character in a different slot (if available)
  [ ]  Enter world → full combat + movement works
  [ ]  Disconnect → reconnect → position saved
"@ -ForegroundColor Gray
Read-Key

# Close Phase 4
Write-Host "Closing Phase 4 processes..." -ForegroundColor Yellow
if ($regressionClientProcess -and !$regressionClientProcess.HasExited) { $regressionClientProcess.Kill() }
if ($regressionDSProcess -and !$regressionDSProcess.HasExited) { $regressionDSProcess.Kill() }
Start-Sleep -Seconds 2

# ═══════════════════════════════════════════════════════════════
# RESULTS SUMMARY
# ═══════════════════════════════════════════════════════════════
Write-Step "TEST RESULTS" @"
  All phases complete. Please tally your results below.
"@

Write-Host @"
╔═══════════════════════════════════════════════════════════════╗
║                    TEST RESULTS SHEET                        ║
╠═══════════════════════════════════════════════════════════════╣
║                                                              ║
║ Phase 1 — Quick PIE Validation   [  / 8]                     ║
║ Phase 2 — Direct Auth            [  / 18]                    ║
║ Phase 3 — Token Auth             [  / 12]                    ║
║ Phase 4 — Regression             [  / 5]                     ║
║                                                              ║
║ TOTAL                          [  / 43]                      ║
║                                                              ║
║ FAILURES:                                                     ║
║  - ...                                                        ║
║  - ...                                                        ║
║                                                              ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Cyan

Write-Host "Test suite complete." -ForegroundColor Green
