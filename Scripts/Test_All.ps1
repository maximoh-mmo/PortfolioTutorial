<#
.SYNOPSIS
    Comprehensive test runner for Onset project - validates all systems.
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

Write-Host @"
╔═══════════════════════════════════════════════════════════════╗
║               ONSET - COMPREHENSIVE TEST SUITE               ║
╠═══════════════════════════════════════════════════════════════╣
║  Phase 1: Quick PIE Validation         (~5 min)              ║
║  Phase 2: Direct Auth + Persistence    (~15 min)             ║
║  Phase 3: Token Auth + Login Server    (~20 min)             ║
║  Phase 4: Regression (Direct mode)     (~5 min)              ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Cyan

# ═══════════════════════════════════════════════════════════════
# PHASE 1 - Quick PIE Validation
# ═══════════════════════════════════════════════════════════════
Write-Step "PHASE 1: Quick PIE Validation" @"
  Open the editor and run PIE (standalone, no multiplayer).
  Verify these items manually in the PIE window:
"@

Write-Section "1.1 - Screen-Relative WASD"
Write-Host @"
  `[ `] W moves character toward TOP of screen (not where character faces)
  `[ `] S moves toward BOTTOM of screen
  `[ `] A moves LEFT on screen
  `[ `] D moves RIGHT on screen
  `[ `] Gamepad L-Stick behaves identically (up = top of screen)
  `[ `] WASD interrupts click-to-move pathfinding immediately
"@ -ForegroundColor Gray
Read-Key

Write-Section "1.2 - Click-to-Move + Targeting"
Write-Host @"
  `[ `] Click on ground - character moves to location
  `[ `] Click on enemy - enemy is targeted (target indicator visible)
  `[ `] Click on ground while targeting - move to location, targeting cleared
  `[ `] Tap (touch) behaves the same as click
"@ -ForegroundColor Gray
Read-Key

Write-Section "1.3 - Basic Combat"
Write-Host @"
  `[ `] Target an enemy - basic attack fires (ability activates)
  `[ `] NPC reacts (hit reaction, health decreases)
  `[ `] NPC dies - corpse appears - respawns after delay
  `[ `] Player AI autoplay (idle 20s) - auto-combat engages
  `[ `] WASD/click interrupts auto-combat
"@ -ForegroundColor Gray
Read-Key

# ═══════════════════════════════════════════════════════════════
# PHASE 2 - Lobby DS + Game DS (Two-Server Flow)
# ═══════════════════════════════════════════════════════════════
Write-Step "PHASE 2: Lobby DS + Game Server (Two-Server Auth Flow)" @"
  Tests the new architecture:
    Lobby Server (MainMenu, AuthMode=Direct) handles auth + char select
    Game Server (DemoLevel, AuthMode=Token) handles gameplay

  After character selection, the lobby server generates a session token
  and the client travels to the game server.
"@

Write-Section "2.0 - Verify Account API is Reachable"
$LambdaUrl = (Select-String -Path $ConfigPath -Pattern "ConnectionString=(.+)" | ForEach-Object { $_.Matches.Groups[1].Value.Trim() })
if ($LambdaUrl) {
    Write-Host "Account API URL from config: https://$LambdaUrl" -ForegroundColor Gray
    try {
        $response = Invoke-WebRequest -Uri "https://$LambdaUrl/health" -UseBasicParsing -TimeoutSec 5
        if ($response.StatusCode -eq 200) {
            Write-Host "  `[PASS`] Account API is reachable: $($response.Content)" -ForegroundColor Green
        }
    } catch {
        Write-Host "  `[WARN`] Account API not reachable: $_" -ForegroundColor Yellow
        Write-Host "  Tests will still work but will use the Lambda. Check CloudWatch logs." -ForegroundColor Yellow
    }
} else {
    Write-Host "  `[WARN`] Could not parse ConnectionString from config" -ForegroundColor Yellow
}
Read-Key

Write-Section "2.1 - Launch Lobby DS + Game Server + Client"
Write-Host "Launching Lobby DS (MainMenu)..." -ForegroundColor Green
$lobbyArgs = """$ProjectPath"" /Game/Maps/MainMenu?Listen -server -log -NoLiveCoding"
$lobbyProcess = Start-Process -FilePath $Binary -ArgumentList $lobbyArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

Write-Host "Launching Game Server (DemoLevel)..." -ForegroundColor Green
$gameArgs = """$ProjectPath"" /Game/DemoLevel?Listen -server -log -NoLiveCoding"
$gameProcess = Start-Process -FilePath $Binary -ArgumentList $gameArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

Write-Host "Launching client..." -ForegroundColor Green
$clientArgs = """$ProjectPath"" /Game/Maps/MainMenu -game -ResX=1280 -ResY=720 -WinX=100 -WinY=100 -log -NoLiveCoding"
$clientProcess = Start-Process -FilePath $Binary -ArgumentList $clientArgs -WindowStyle Normal -PassThru

Write-Host @"
  `[ `]  Lobby server window opened (MainMenu)
  `[ `]  Game server window opened (DemoLevel)
  `[ `]  Client window opened and connected to Lobby DS
"@ -ForegroundColor Gray
Read-Key

Write-Section "2.2 - First-Time Login on Lobby"
Write-Host @"
  Check the Lobby DS console/log for these lines:
  `[ `]  PostLogin: player ... platform=Steam, id=...
  `[ `]  FHttpStore initialized: BaseURL=https://...
  `[ `]  Request returned 404  (first login, account not found)
  `[ `]  PostLogin: auto-created account for Steam/...
  `[ `]  Client opens MainMenu - clicks Connect - Character Select screen appears

  Check Game Server console/log:
  `[ `]  Server is listening on DemoLevel
  `[ `]  No client connections yet (client is on lobby)

  If the Lambda URL is valid, also check CloudWatch:
  `[ `]  GET /account/Steam/{id} returns 404
  `[ `]  POST /account/Steam/{id} returns 201
"@ -ForegroundColor Gray
Read-Key

Write-Section "2.3 - Character Creation + Travel to Game Server"
Write-Host @"
  In the Character Select screen (on the lobby):
  `[ `]  Hover an empty slot - Create Character prompt appears
  `[ `]  Click/select empty slot - enter name - create

  After creation, the lobby server should:
  `[ `]  Log: Server_CreateCharacter: created '...' in slot N
  `[ `]  Log: Server_SelectCharacter: player ... selected slot N
  `[ `]  Client should travel to Game Server (DemoLevel)
  `[ `]  Client_TravelToGameServer RPC triggered (check client log)

  Check Game Server console:
  `[ `]  Client connects with token
  `[ `]  PreLogin: token validated for ...
  `[ `]  PostLogin: player ... with slot N
  `[ `]  HandleStartingNewPlayer: spawning pawn at default position
"@ -ForegroundColor Gray
Read-Key

Write-Section "2.4 - Gameplay Verification (on Game Server)"
Write-Host @"
  After traveling to Game Server, the player should be in DemoLevel:
  `[ `]  Player pawn spawns at position (0, 0, 150)
  `[ `]  WASD/click-to-move works immediately (no UI overlay)
  `[ `]  No MainMenu or Character Select UI visible
  `[ `]  Combat works: target enemy, basic attack fires, NPC reacts
"@ -ForegroundColor Gray
Read-Key

Write-Section "2.5 - Save-on-Disconnect (Game Server)"
Write-Host @"
  `[ `]  Move character to a new position
  `[ `]  Close the client window
  `[ `]  Game Server logs show EndPlay - save triggered
  `[ `]  Launch client again (same process)
  `[ `]  Client connects to Lobby → selects character → auto-travels to Game Server
  `[ `]  Character appears at last saved position
"@ -ForegroundColor Gray
Read-Key

# Close all Phase 2 processes
Write-Host "Closing Phase 2 processes..." -ForegroundColor Yellow
if ($clientProcess -and !$clientProcess.HasExited) { $clientProcess.Kill() }
if ($gameProcess -and !$gameProcess.HasExited) { $gameProcess.Kill() }
if ($lobbyProcess -and !$lobbyProcess.HasExited) { $lobbyProcess.Kill() }
Start-Sleep -Seconds 2

# ═══════════════════════════════════════════════════════════════
# PHASE 3 - Token Auth Mode (Login Server - Game Server)
# ═══════════════════════════════════════════════════════════════
Write-Step "PHASE 3: Token Auth Mode" @"
  Tests the full live-service auth flow:
    Client - Login Server (auth + token) - Game Server (validate + play)

  Requires AuthMode=Token in config.
  The test will temporarily modify DefaultEngine.ini and restore it after.
"@

Write-Section "3.0 - Config Check"
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
Write-Host "  `[DONE`] Config updated. Remember to restore after testing." -ForegroundColor Green

Write-Host @"
  Quick check: LoginServer map must exist at /Game/Maps/LoginServer
  `[ `]  LoginServer map exists with AOnsetLoginServerGameMode override
"@ -ForegroundColor Gray
Read-Key

Write-Section "3.1 - Launch Login Server"
Write-Host "Launching Login Server..." -ForegroundColor Green

$loginServerArgs = """$ProjectPath"" /Game/Maps/LoginServer?Listen -server -log -NoLiveCoding"
$loginServerProcess = Start-Process -FilePath $Binary -ArgumentList $loginServerArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

Write-Host @"
  Check Login Server console:
  `[ `]  AuthSubsystem initialized: AuthMode=Token
  `[ `]  Map loaded: LoginServer

  Expected behavior: Login Server accepts connections,
  validates Steam ticket, generates HMAC token, sends
  Client_SessionToken, then kicks player after 2s.
"@ -ForegroundColor Gray
Read-Key

Write-Section "3.2 - Launch Game Server"
Write-Host "Launching Game Server with AuthMode=Token..." -ForegroundColor Green

$gameServerArgs = """$ProjectPath"" /Game/DemoLevel?Listen -server -log -NoLiveCoding"
$gameServerProcess = Start-Process -FilePath $Binary -ArgumentList $gameServerArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

Write-Host @"
  Check Game Server console:
  `[ `]  AuthSubsystem initialized: AuthMode=Token
  `[ `]  Map loaded: DemoLevel

  Expected behavior: Game Server rejects connections
  without a valid ?Token= in the URL. Only accepts connections
  that pass PreLoginTokenAuth validation.
"@ -ForegroundColor Gray
Read-Key

Write-Section "3.3 - Launch Client"
Write-Host "Launching client..." -ForegroundColor Green

$tokenClientArgs = """$ProjectPath"" /Game/Maps/MainMenu -game -ResX=1280 -ResY=720 -WinX=200 -WinY=200 -log -NoLiveCoding"
$tokenClientProcess = Start-Process -FilePath $Binary -ArgumentList $tokenClientArgs -WindowStyle Normal -PassThru

Write-Host @"
  Check Login Server logs:
  `[ `]  PostLogin: player ... platform=Steam, id=...
  `[ `]  PostLogin: sent session token to ...
  `[ `]  LoginServer: kicking ... in 2.0s
  `[ `]  Client receives Client_SessionToken RPC (check client log if visible)

  Check Game Server logs (after client reconnects):
  `[ `]  PreLogin: token validated for ...
  `[ `]  PostLogin: using token auth - Steam/...
  `[ `]  PostLogin: auto-created account for Steam/...  (or loaded existing)
  `[ `]  Client_AccountData sent
"@ -ForegroundColor Gray
Read-Key

Write-Section "3.4 - Token Rejection Test"
Write-Host @"
  In the Game Server console, verify no connections are
  accepted without a valid token.

  If you can connect another client without a token:
  `[ `]  PreLogin: rejected connection ... - Missing session token
  `[ `]  Client stays at loading screen / gets connection error
"@ -ForegroundColor Gray
Read-Key

# Close Phase 3 processes
Write-Host "Closing Phase 3 processes..." -ForegroundColor Yellow
if ($tokenClientProcess -and !$tokenClientProcess.HasExited) { $tokenClientProcess.Kill() }
if ($gameServerProcess -and !$gameServerProcess.HasExited) { $gameServerProcess.Kill() }
if ($loginServerProcess -and !$loginServerProcess.HasExited) { $loginServerProcess.Kill() }
Start-Sleep -Seconds 2

# ═══════════════════════════════════════════════════════════════
# PHASE 4 - Regression (Direct Mode)
# ═══════════════════════════════════════════════════════════════
Write-Step "PHASE 4: Regression - Direct Mode" @"
  Restore config to AuthMode=Direct and verify the original flow
  still works perfectly. This is critical: Direct mode must be
  100% unaffected by the auth subsystem extraction.
"@

# Restore config from backup
Set-Content -Path $ConfigPath -Value $configBackup
Write-Host "  `[DONE`] Config restored to original (AuthMode=Direct)" -ForegroundColor Green

Write-Section "4.1 - Launch Lobby DS + Game DS + Client (Direct Mode)"
Write-Host "Re-launching Lobby DS (MainMenu) + Game DS (DemoLevel) + client..." -ForegroundColor Gray

$lobbyArgs = """$ProjectPath"" /Game/Maps/MainMenu?Listen -server -log -NoLiveCoding"
$regressionLobbyProcess = Start-Process -FilePath $Binary -ArgumentList $lobbyArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

$gameArgs = """$ProjectPath"" /Game/DemoLevel?Listen -server -log -NoLiveCoding"
$regressionGameProcess = Start-Process -FilePath $Binary -ArgumentList $gameArgs -WindowStyle Normal -PassThru
Start-Sleep -Seconds 8

$regressionClientArgs = """$ProjectPath"" /Game/Maps/MainMenu -game -ResX=1280 -ResY=720 -WinX=100 -WinY=100 -log -NoLiveCoding"
$regressionClientProcess = Start-Process -FilePath $Binary -ArgumentList $regressionClientArgs -WindowStyle Normal -PassThru

Write-Host @"
  `[ `]  Lobby DS launched (AuthMode=Direct, MainMenu)
  `[ `]  Game DS launched (DemoLevel)
  `[ `]  Client launched, connected to Lobby DS
"@ -ForegroundColor Gray
Read-Key

Write-Section "4.2 - Full Flow with Two Servers"
Write-Host @"
  In the Character Select screen:
  `[ `]  Select an occupied slot (or create a new one)
  `[ `]  Client_TravelToGameServer triggered (check client log)
  `[ `]  Lobby logs: Server_SelectCharacter + travel
  `[ `]  Game Server logs: PreLogin + PostLogin token auth
  `[ `]  Game Server logs: HandleStartingNewPlayer spawns pawn
  `[ `]  Player appears at saved position in DemoLevel
  `[ `]  WASD + click-to-move works from saved position
"@ -ForegroundColor Gray
Read-Key

# Close Phase 4
Write-Host "Closing Phase 4 processes..." -ForegroundColor Yellow
if ($regressionClientProcess -and !$regressionClientProcess.HasExited) { $regressionClientProcess.Kill() }
if ($regressionGameProcess -and !$regressionGameProcess.HasExited) { $regressionGameProcess.Kill() }
if ($regressionLobbyProcess -and !$regressionLobbyProcess.HasExited) { $regressionLobbyProcess.Kill() }
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
║ Phase 1 - Quick PIE Validation   `[  / 8`]                     ║
║ Phase 2 - Lobby DS + Game Server    `[  / 16`]                    ║
║ Phase 3 - Token Auth (Login Server) `[  / 12`]                    ║
║ Phase 4 - Regression (Two-Server)   `[  / 6`]                     ║
║                                                              ║
║ TOTAL                          `[  / 43`]                      ║
║                                                              ║
║ FAILURES:                                                     ║
║  - ...                                                        ║
║  - ...                                                        ║
║                                                              ║
╚═══════════════════════════════════════════════════════════════╝
"@ -ForegroundColor Cyan

Write-Host "Test suite complete." -ForegroundColor Green
