// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "OnsetPlayerDataTypes.h"
#include "Data/OnsetItemTypes.h"
#include "GameFramework/PlayerController.h"
#include "OnsetPlayerController.generated.h"

struct FOnsetAccountData;
struct FOnsetFullCharacterData;
class AOnsetPlayerAIController;
class AOnsetPlayerCharacter;
class UInteractionComponent;
class UGameplayAbility;
class UGamepadCursorWidget;
class UCursorManager;
class UInputAction;
class UInputMappingContext;
class UOnsetRootLayout;
class UOnsetScreenBase;
class UTargetingComponent;
class UHUDWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogGamepad, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnsetAccountDataChanged);

UCLASS()
class ONSET_API AOnsetPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AOnsetPlayerController();
		
	UFUNCTION(BlueprintCallable, Category="Steam")
	void RequestSteamAuth();

	UFUNCTION(BlueprintCallable, Category="Combat")
	void StartAutoAttack();
	
	UFUNCTION(BlueprintCallable, Category="Combat")
	void StopAutoAttack();
	
	void EnableAutoCombat();
	void DisableAutoCombat();
	const AController* GetActiveController() const;

	/** Enables/disables autoplay (AI possession of the pawn). Server RPC path. */
	UFUNCTION(BlueprintCallable, Category = "Auto Combat")
	void SetAutoCombatEnabled(bool bEnabled);

	/** Toggles whether the pawn keeps auto-combating after this player disconnects. */
	UFUNCTION(BlueprintCallable, Category = "Auto Combat")
	void SetContinueOnDisconnect(bool bEnabled);

	/** Returns the auto-combat AI controller if it exists (authority). */
	UFUNCTION(BlueprintCallable, Category = "Auto Combat")
	AOnsetPlayerAIController* GetAutoCombatController() const;

protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;
	
	virtual void OnPossess(APawn* InPawn) override;
	
	virtual void OnUnPossess() override;

	/** Fires on the local client when the replicated Pawn property updates; used to build the HUD and dismiss the loading screen. */
	virtual void OnRep_Pawn() override;

	/** Creates + binds the in-game HUD on the local client when a pawn is possessed. */
	void CreateHUD(APawn* InPawn);

	/**
	 * Called by the engine when a player leaves and their PC is being destroyed.
	 * Default destroys the pawn; here we optionally hand it to the auto-combat
	 * controller so it keeps fighting while the player is offline.
	 */
	virtual void PawnLeavingGame() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Onset|Auth")           
	FOnsetAccountData CachedAccountData; 
private:
	// --- Auto Combat ---
	
	/** PlayerAIC, spawned as world actor on Begin Play, responsible for its own destruction */
	UPROPERTY()
	TObjectPtr<AOnsetPlayerAIController> AutoCombatController;
	
	bool bAutoCombatEnabled = false;
	bool bIdleTimerInitialized = false;
	
	UPROPERTY(EditDefaultsOnly, Category="Auto Combat")
	float IdleAutoCombatDelay = 5.0f;
	
	FTimerHandle IdleAutoCombatTimerHandle;

	void ResetIdleTimer();	
	
	// --- Input Mapping Contexts ---
	
	/** Virtual joystick + tap + virtual ability buttons for touch input. */                                      
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* IMC_Touch;
	/** Mouse clicks + WASD + number keys for desktop input. */                                                   
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* IMC_KbMouse;
	/** L-Stick movement + R-Stick cursor + button abilities for gamepad. */                                      
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* IMC_Gamepad;
	
	// --- Input Actions ---                                                                                      
                                                                                                                     
	/** Movement input (2D axis): virtual joystick, WASD, gamepad L-Stick. */                                     
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")                    
	UInputAction* IA_Move;
	/** Gamepad R-Stick cursor emulation (2D axis). */                                                            
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")
	UInputAction* IA_Cursor;
	/** Primary interaction (digital): tap, left-click, R-Stick click, A button.                                  
	*  Context resolution branches on hit result: enemy → target, ground → move. etc. */    
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")
	UInputAction* IA_Primary;
	
	/** Ability Input Actions */
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")
	UInputAction* IA_Ability1;
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")
	UInputAction* IA_Ability2;
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")
	UInputAction* IA_Ability3;
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")                                                              
	UInputAction* IA_Ability4; 
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")                                                         
	UInputAction* IA_PvPToggle;                                                                                     
	UPROPERTY(EditDefaultsOnly, Category="Input|Actions")
	UInputAction* IA_Inventory;

	/** Screen class pushed when the inventory toggle is pressed (override with WBP_InventoryScreen). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UOnsetScreenBase> InventoryScreenClass;

	/** Root layout class to re-create when entering the game world (override with WBP_RootLayout). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UOnsetRootLayout> GameRootLayoutClass;                                                                                     
	
	// --- Cursor ---                                                                                         
	
	/** Unified cursor position from mouse, touch, or gamepad R-Stick. */                 
	UPROPERTY()
	UCursorManager* CursorManager;	
	/** Called to hide software cursor */
	void HideGamepadCursor();	
	/** Timer handle for idle hide cursor*/
	FTimerHandle CursorIdleTimerHandle;	
	/** Time in seconds after R-Stick stops moving to hide the software cursor */
	UPROPERTY(EditDefaultsOnly, Category="Cursor")                                                                
	float CursorIdleDelay = 1.5f;
	
	/** Virtual cursor for gamepad */
	UPROPERTY(EditDefaultsOnly, Category="Cursor")                                                                
	TSubclassOf<UGamepadCursorWidget> GamepadCursorWidgetClass;                                                     
	UPROPERTY()                                                                                                     
	TObjectPtr<UGamepadCursorWidget> GamepadCursorWidget;
	
	// --- Targeting ---
	
	/** Stores the current targeting component via OnPossess, clears on UnPossess. */          
	UPROPERTY()
	TObjectPtr<UTargetingComponent> TargetingComponent;

	// --- HUD ---

	/** Widget class used to create the in-game HUD on the local client. */
	UPROPERTY(EditDefaultsOnly, Category="HUD")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	/** The live in-game HUD instance (local client only). */
	UPROPERTY()
	TObjectPtr<UHUDWidget> HUDWidget;
	
	UPROPERTY()
	TObjectPtr<UInteractionComponent> InteractionComponent;
	
	// --- Combat ---
	UPROPERTY()
	FTimerHandle AutoAttackTimerHandle;

	/**
	 * Cooldown-echo poll interval for the auto attack. The basic attack's cooldown
	 * (weapon archetype base x haste x slow multiplier) is the real attack-rate gate;
	 * this timer just echoes TryActivateAbilityByClass quickly enough to fire the
	 * instant the cooldown expires (0.1s << the fastest weapon cooldown).
	 */
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	float AutoAttackInterval = 0.1f;
	
	UPROPERTY(EditDefaultsOnly, Category="Combat")
	TSubclassOf<UGameplayAbility> BasicAttackAbility;
	
	void OnAutoAttackTick();	
	
	
	// --- Input Handlers ---                                                                                     
                                                                                                                     
	/** Called when IA_Move triggers. Applies 2D movement input to the controlled pawn. */                        
	void OnMove(const FInputActionValue& Value);	                                                                                                                     
	/** Called when IA_Cursor triggers. Accumulates R-Stick delta into the gamepad cursor position. */            
	void OnCursorMove(const FInputActionValue& Value);
	/** Called when R-Stick stops moving for cursor emulation, starts timer to hide software cursor  */
	void OnCursorMoveEnded(const FInputActionValue& Value);	
	/** Called when IA_Primary triggers. Raycasts at cursor position and branches:                                
	 *  Enemy → TargetingComponent->SetTarget(), Ground → MoveToLocation(). */                                    
	void OnPrimaryInteraction(const FInputActionValue& Value);
	/** Ability handlers for each ability */
	void OnAbility1(const FInputActionValue& Value);
	void OnAbility2(const FInputActionValue& Value);
	void OnAbility3(const FInputActionValue& Value);
	void OnAbility4(const FInputActionValue& Value);
	
	// --- Touch bridge (BP widget injects ability input without subsystem access) ---
	UFUNCTION(BlueprintCallable, Category="Input")
	void InjectAbilityInput(int32 AbilityIndex, bool bPressed) const;
	
	// --- PVP toggling ---
	void OnPvPToggleTriggered(const FInputActionValue& Value);                                                      

	// --- Inventory screen toggling ---
	void OnInventoryToggleTriggered(const FInputActionValue& Value);                                                      

	UFUNCTION(Server, Reliable)
	void Server_SendAuthTicket(const FString& AuthTicket);

	UFUNCTION(Server, Reliable)
	void Server_SetPvPEnabled(bool bEnabled);
		
public:
	const FOnsetAccountData& GetCachedAccountData() const { return CachedAccountData; }

	/** Broadcast whenever new account data arrives from the server (see Client_AccountData). */
	UPROPERTY(BlueprintAssignable, Category = "Account")
	FOnsetAccountDataChanged OnAccountDataChanged;

	const FString& GetCachedSessionToken() const { return CachedSessionToken; }

	UFUNCTION(BlueprintCallable, Category="Auth")
	void ReconnectToGameServer(); 

	UFUNCTION(Client, Reliable)
	void Client_ShowMainMenuUI(TSubclassOf<UOnsetRootLayout> RootLayoutClass, TSubclassOf<UOnsetScreenBase> MainMenuClass);

	UFUNCTION(Client, Reliable)
	void Client_CleanupUI();

	UFUNCTION(Client, Reliable)
	void Client_AccountData(const FOnsetAccountData& AccountData);

	// --- Persistence RPCs ---

	UFUNCTION(Client, Reliable)
	void Client_CharacterData(const FOnsetFullCharacterData& CharacterData);

	UFUNCTION(Client, Reliable)
	void Client_SaveComplete(bool bSuccess);

	UFUNCTION(Client, Reliable)
	void Client_SessionToken(const FString& Token);

	UFUNCTION(Client, Reliable)
	void Client_SessionTokenFailed(const FString& Reason);

	UFUNCTION(Client, Reliable)
	void Client_TravelToGameServer(const FString& ServerIP, const FString& ServerPort, const FString& Token);

	UFUNCTION(Server, Reliable)
	void Server_OnClientPossessed();

	UFUNCTION(Server, Reliable)
	void Server_SelectCharacter(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void Server_CreateCharacter(int32 SlotIndex, const FString& CharacterName, EOnsetCharacterClass CharacterClass, int32 AppearancePresetIndex);

	UFUNCTION(Server, Reliable)
	void Server_DeleteCharacter(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void Server_SaveCharacter();


	UFUNCTION(Client, Reliable)
	void Client_ClearAuthTimeout();

	UFUNCTION(Server, Reliable)
	void Server_DisableAutoCombat();

	UFUNCTION(Server, Reliable)
	void Server_SetAutoCombatEnabled(bool bEnabled);

	UFUNCTION(Server, Reliable)
	void Server_SetContinueOnDisconnect(bool bEnabled);

	UFUNCTION(Server, Reliable)
	void Server_ProcessPrimaryInteraction(AActor* HitActor, FVector HitLocation);

	/** Shows the looted-items popup on the owning client after a successful loot. */
	UFUNCTION(Client, Reliable)
	void Client_ShowLootOverlay(const TArray<FOnsetInventoryEntry>& LootedItems);

	/** Debug: grants a DT_Equipment item to the possessed pawn's inventory (console: OnsetGrantItem <RowName>). */
	UFUNCTION(Exec)
	void OnsetGrantItem(const FString& RowName);

	UFUNCTION(Server, Reliable)
	void Server_GrantItem(const FString& RowName);

	/** Debug: accepts a DT_Quests quest on the possessed pawn (console: OnsetAcceptQuest <RowName>). */
	UFUNCTION(Exec)
	void OnsetAcceptQuest(const FString& QuestRowName);

	UFUNCTION(Server, Reliable)
	void Server_AcceptQuest(const FString& QuestRowName);

	void ClearAuthTimeout();

	/** Saves the given pawn's character data (if a slot is available), at most once per session.
	 *  Returns true if the save was performed and succeeded. Subsequent calls are no-ops. */
	bool SaveCurrentCharacter(APawn* InPawn = nullptr);

	// --- Character Appearance ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Character")
	void BP_ApplyAppearancePreset(AOnsetPlayerCharacter* PlayerChar, int32 PresetIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Account")
	void BP_OnAccountDataUpdated();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnAuthTimeout();
	FTimerHandle AuthTimeoutTimerHandle;

	UPROPERTY()
	TObjectPtr<APawn> CachedPlayerPawn;

	FString CachedSessionToken;

	/** True once this controller has flushed its character data to the data store (disconnect / shutdown). */
	bool bCharacterDataSaved = false;

	// --- Test harness: -AutoPlaySlot=N auto-enters an occupied character slot. ---
	FTimerHandle AutoPlayTimerHandle;
	int32 AutoPlaySlotIndex = -1;
	void AutoPlaySelectCharacter();
};
