# 🎬 **Episode 41 — Steam ID Resolution & Save/Load Flow**

## **Episode Goal**
Extract the numeric SteamID from the auth ticket via `SteamGameServer()->BeginAuthSession()`, store it on `PlayerState`, and implement the full save/load RPC flow — account auto-create on first login, load character data on select.

---

## **Context & Dependencies**
- Requires Episode 40 (Database Architecture — `IPlayerDataStore`, `UOnsetPlayerDataSubsystem` working)
- Steam auth ticket flow from Episode 39 (`Server_SendAuthTicket`, `ValidateAuthTicket`)
- PlayerController RPC framework established

---

## **High‑Level Summary**
This episode bridges Steam auth to persistence. The server validates the auth ticket, then calls Steam's `BeginAuthSession()` to get the numeric SteamID64. That ID becomes the primary key for the account row. We add the platform fields to `PlayerState`, wire `PostLogin` to load the account, and implement the RPCs for character selection and creation.

---

## **Key Concepts Introduced**
- Steamworks SDK: `ISteamGameServer::BeginAuthSession()` for ticket → SteamID
- Platform-agnostic identity: `(Platform, PlatformID)` composite key
- Auto-create account on first login
- Server RPCs: `Server_SelectCharacter`, `Server_CreateCharacter`, `Server_SaveCharacter`
- Client RPCs: `Client_AccountData`, `Client_CharacterData`, `Client_SaveComplete`

---

## **Technical Breakdown**

### **1. Update `AOnsetPlayerState`**
**File:** `Source/Onset/Public/Player/OnsetPlayerState.h`
```cpp
UCLASS()
class ONSET_API AOnsetPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    // ... existing bIsPvPEnabled ...

    // Platform identity for persistence
    UPROPERTY()
    FString PlayerPlatformID;  // e.g. "76561197960265728"

    UPROPERTY()
    FString PlayerPlatform;    // e.g. "Steam"

    // Which character slot is active this session
    UPROPERTY()
    int32 SelectedCharacterSlot = INDEX_NONE;
};
```

### **2. Update `AOnsetGameModeBase` — SteamID Extraction**
**File:** `Source/Onset/Private/Multiplayer/OnsetGameModeBase.cpp`

Add Steamworks includes:
```cpp
#include "Steam/steam_api.h" // or your ThirdParty path
```

Update `ValidateAuthTicket`:
```cpp
void AOnsetGameModeBase::ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket)
{
    // Existing validation logic...
    
    // Extract SteamID from ticket using Steam Game Server API
    CSteamID SteamID;
    if (SteamGameServer() && SteamGameServer()->BeginAuthSession(TCHAR_TO_UTF8(*AuthTicket), AuthTicket.Len(), &SteamID) == k_EBeginAuthSessionResultOK)
    {
        AOnsetPlayerState* PS = NewPlayer->GetPlayerState<AOnsetPlayerState>();
        if (PS)
        {
            PS->PlayerPlatformID = FString::Printf(TEXT("%llu"), SteamID.ConvertToUint64());
            PS->PlayerPlatform = TEXT("Steam");
            UE_LOG(LogOnsetGameMode, Log, TEXT("Extracted SteamID: %s"), *PS->PlayerPlatformID);
        }
        // Important: EndAuthSession when done (on disconnect/logout)
        SteamGameServer()->EndAuthSession(SteamID);
    }
    else
    {
        // Fallback: hash the ticket (not unique per-player but prevents hard crash)
        PS->PlayerPlatformID = FString::FromInt(GetTypeHash(AuthTicket));
        PS->PlayerPlatform = TEXT("Steam_Fallback");
    }

    // Trigger account load
    if (UWorld* World = GetWorld())
    {
        if (UOnsetPlayerDataSubsystem* DataSub = World->GetSubsystem<UOnsetPlayerDataSubsystem>())
        {
            FOnsetAccountData AccountData = DataSub->GetAccountData(PS->PlayerPlatform, PS->PlayerPlatformID);
            // Send to client
            if (AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(NewPlayer))
            {
                PC->Client_AccountData(AccountData);
            }
        }
    }
}
```

### **3. Add Client/Server RPCs to `AOnsetPlayerController`**
**File:** `Source/Onset/Public/Player/OnsetPlayerController.h`
```cpp
public:
    // Client receives account overview (3 slots)
    UFUNCTION(Client, Reliable)
    void Client_AccountData(const FOnsetAccountData& AccountData);

    // Client receives full character data on select
    UFUNCTION(Client, Reliable)
    void Client_CharacterData(const FOnsetFullCharacterData& CharacterData);

    // Client requests to select a character slot
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SelectCharacter(int32 SlotIndex);

    // Client requests to create a new character in empty slot
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_CreateCharacter(int32 SlotIndex, const FString& CharacterName);

    // Manual save request
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SaveCharacter();

    // Save result
    UFUNCTION(Client, Reliable)
    void Client_SaveComplete(bool bSuccess);
```

**File:** `Source/Onset/Private/Player/OnsetPlayerController.cpp`
```cpp
void AOnsetPlayerController::Server_SelectCharacter_Implementation(int32 SlotIndex)
{
    if (!HasAuthority()) return;
    
    AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
    if (!PS || PS->PlayerPlatformID.IsEmpty()) return;

    if (UOnsetPlayerDataSubsystem* DataSub = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>())
    {
        FOnsetFullCharacterData CharData = DataSub->GetCharacterData(PS->PlayerPlatform, PS->PlayerPlatformID, SlotIndex);
        
        if (CharData.SlotIndex == SlotIndex && !CharData.CharacterName.IsEmpty())
        {
            PS->SelectedCharacterSlot = SlotIndex;
            
            // Spawn/respawn player at saved position
            RespawnAtSavedLocation(CharData);
            
            // Send full character data to client for UI update
            Client_CharacterData(CharData);
        }
    }
}

void AOnsetPlayerController::Server_CreateCharacter_Implementation(int32 SlotIndex, const FString& CharacterName)
{
    if (!HasAuthority()) return;
    if (SlotIndex < 0 || SlotIndex > 2) return;
    
    AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
    if (!PS || PS->PlayerPlatformID.IsEmpty()) return;

    if (UOnsetPlayerDataSubsystem* DataSub = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>())
    {
        // Verify slot is empty
        FOnsetAccountData AccountData = DataSub->GetAccountData(PS->PlayerPlatform, PS->PlayerPlatformID);
        if (SlotIndex < AccountData.Slots.Num() && AccountData.Slots[SlotIndex].bOccupied)
        {
            // Slot not empty, reject
            return;
        }

        // Create default character
        FOnsetFullCharacterData NewChar;
        NewChar.SlotIndex = SlotIndex;
        NewChar.CharacterName = CharacterName.IsEmpty() ? TEXT("Adventurer") : CharacterName;
        NewChar.Level = 1;
        NewChar.Experience = 0.0f;
        NewChar.MaxHealth = 100.0f;
        NewChar.Position = FVector(0, 0, 200);
        NewChar.RotationYaw = 0.0f;
        NewChar.InventoryJSON = TEXT("[]");
        NewChar.EquipmentJSON = TEXT("{}");
        NewChar.QuestsJSON = TEXT("{}");

        if (DataSub->SaveCharacterData(PS->PlayerPlatform, PS->PlayerPlatformID, SlotIndex, NewChar))
        {
            // Auto-select the new character
            Server_SelectCharacter(SlotIndex);
        }
    }
}

void AOnsetPlayerController::Server_SaveCharacter_Implementation()
{
    if (!HasAuthority()) return;
    
    AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
    if (!PS || PS->SelectedCharacterSlot == INDEX_NONE) return;

    if (AOnsetPlayerCharacter* Char = Cast<AOnsetPlayerCharacter>(GetPawn()))
    {
        FOnsetFullCharacterData SaveData = Char->BuildSaveData();
        SaveData.SlotIndex = PS->SelectedCharacterSlot;

        bool bSuccess = false;
        if (UOnsetPlayerDataSubsystem* DataSub = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>())
        {
            bSuccess = DataSub->SaveCharacterData(PS->PlayerPlatform, PS->PlayerPlatformID, PS->SelectedCharacterSlot, SaveData);
        }
        
        Client_SaveComplete(bSuccess);
    }
}

// Client RPC implementations (empty bodies, just receive data)
void AOnsetPlayerController::Client_AccountData_Implementation(const FOnsetAccountData& AccountData) { /* UI binds to this */ }
void AOnsetPlayerController::Client_CharacterData_Implementation(const FOnsetFullCharacterData& CharacterData) { /* UI updates */ }
void AOnsetPlayerController::Client_SaveComplete_Implementation(bool bSuccess) { /* show toast */ }
```

### **4. Add `BuildSaveData()` and `ApplySaveData()` to `AOnsetPlayerCharacter`**
**File:** `Source/Onset/Public/Player/OnsetPlayerCharacter.h`
```cpp
UFUNCTION(BlueprintCallable, Category = "Persistence")
FOnsetFullCharacterData BuildSaveData() const;

UFUNCTION(BlueprintCallable, Category = "Persistence")
void ApplySaveData(const FOnsetFullCharacterData& Data);
```

**File:** `Source/Onset/Private/Player/OnsetPlayerCharacter.cpp`
```cpp
FOnsetFullCharacterData AOnsetPlayerCharacter::BuildSaveData() const
{
    FOnsetFullCharacterData Data;
    Data.SlotIndex = 0; // Filled by caller
    Data.CharacterName = GetName(); // Or from PlayerState
    Data.Level = 1; // From future progression system
    Data.Experience = 0.0f;
    
    if (UOnsetAttributeSet* AttrSet = GetAttributeSet())
    {
        Data.MaxHealth = AttrSet->GetMaxHealth();
    }
    
    Data.Position = GetActorLocation();
    Data.RotationYaw = GetActorRotation().Yaw;
    Data.InventoryJSON = TEXT("[]"); // Future: serialize inventory
    Data.EquipmentJSON = TEXT("{}");
    Data.QuestsJSON = TEXT("{}");
    return Data;
}

void AOnsetPlayerCharacter::ApplySaveData(const FOnsetFullCharacterData& Data)
{
    SetActorLocation(Data.Position);
    SetActorRotation(FRotator(0, Data.RotationYaw, 0));
    
    if (UOnsetAttributeSet* AttrSet = GetAttributeSet())
    {
        AttrSet->SetMaxHealth(Data.MaxHealth);
        AttrSet->SetHealth(Data.MaxHealth); // Full health on login
    }
}
```

### **5. Save Triggers**
- **On disconnect:** `AOnsetPlayerController::EndPlay()` → call `Server_SaveCharacter()`
- **Auto-save timer:** `UOnsetPlayerDataSubsystem` 5-min timer iterates all connected players, calls `SaveCharacter()` on their controllers
- **On death:** Hook in `OnDeath()` → trigger save

---

## **How to Test**
1. Launch DS + 1 client
2. Complete Steam auth → client receives `Client_AccountData` (3 empty slots)
3. Client calls `Server_CreateCharacter(0, "Hero")` → slot 0 fills
3. Client calls `Server_SelectCharacter(0)` → pawn spawns at (0,0,200)
4. Move pawn, disconnect → reconnect → pawn at saved position
5. Check DB: `characters` table has row with correct position

---

## **Code Snippets**

```cpp
// OnsetPlayerState.h
UPROPERTY() FString PlayerPlatformID;
UPROPERTY() FString PlayerPlatform;
UPROPERTY() int32 SelectedCharacterSlot = INDEX_NONE;

// OnsetGameModeBase.cpp — SteamID extraction
CSteamID SteamID;
if (SteamGameServer()->BeginAuthSession(TCHAR_TO_UTF8(*AuthTicket), AuthTicket.Len(), &SteamID) == k_EBeginAuthSessionResultOK)
{
    PS->PlayerPlatformID = FString::Printf(TEXT("%llu"), SteamID.ConvertToUint64());
    SteamGameServer()->EndAuthSession(SteamID);
}

// OnsetPlayerController.cpp — Server_SelectCharacter
FOnsetFullCharacterData CharData = DataSub->GetCharacterData(Platform, PlatformID, SlotIndex);
RespawnAtSavedLocation(CharData);
Client_CharacterData(CharData);
```

---

## **Common Pitfalls**
- Forgetting `EndAuthSession()` → Steam leaks session
- Not checking `HasAuthority()` in server RPCs
- `SelectedCharacterSlot` not replicated → client doesn't know which slot
- Forgetting to call `Client_SaveComplete()` → UI stuck on saving spinner

---

## **Dependencies**
- Episode 40 (Database Architecture)
- Episode 39 (Steam Auth)

---

## **Next Episode Preview**
Next time we build the lobby map and character select UI — 3-slot WBP widget, create/select flow, and `ServerTravel` to the game map.

---

## **Episode Checklist**
- [ ] `PlayerState` has `PlayerPlatformID`, `PlayerPlatform`, `SelectedCharacterSlot`
- [ ] `GameMode::ValidateAuthTicket` extracts SteamID via `BeginAuthSession`
- [ ] `GameMode::PostLogin` triggers `LoadAccount` → `Client_AccountData`
- [ ] `Server_SelectCharacter` loads character, spawns pawn, applies save data
- [ ] `Server_CreateCharacter` creates default char, auto-selects
- [ ] `Server_SaveCharacter` snapshots pawn, saves via subsystem
- [ ] Auto-save timer fires every 5 min
- [ ] Save-on-disconnect works