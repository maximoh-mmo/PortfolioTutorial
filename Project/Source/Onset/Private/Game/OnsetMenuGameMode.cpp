#include "Game/OnsetMenuGameMode.h"
#include "Engine/GameInstance.h"
#include "Subsystem/OnsetUISubsystem.h"
#include "UI/CharacterSelectScreen.h"
#include "UI/OnsetRootLayout.h"
#include "UI/OnsetScreenBase.h"
#include "UObject/ConstructorHelpers.h"

AOnsetMenuGameMode::AOnsetMenuGameMode()
{
	DefaultPawnClass = nullptr;
	static ConstructorHelpers::FClassFinder<UOnsetRootLayout> RootLayoutClassFinder(TEXT("/Game/UI/Core/WBP_RootLayout"));
	if (RootLayoutClassFinder.Succeeded())
	{
		RootLayoutClass = RootLayoutClassFinder.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find Root Layout Class"));
	}
	static ConstructorHelpers::FClassFinder<UOnsetScreenBase> MenuFinder(TEXT("/Game/UI/Screens/WBP_MainMenu"));                                                                                                              
	if (MenuFinder.Succeeded())                                                                                 
	{
		MainMenuScreenClass = MenuFinder.Class;                                                                 
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find Main Menu Screen Class"));
	}
	static ConstructorHelpers::FClassFinder<UCharacterSelectScreen> CharSelectFinder(TEXT("/Game/UI/Screens/WBP_CharacterSelect"));                                                                                         
	if (CharSelectFinder.Succeeded())                                                                               
	{                                                                                                               
		CharacterSelectScreenClass = CharSelectFinder.Class;                                                        
	}      
}           

void AOnsetMenuGameMode::StartPlay()
{
	Super::StartPlay();
	if (auto* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())                                        
	{                                                                                                           
		if (!UI->GetRootLayout() && RootLayoutClass)                                                            
			UI->InitializeRootLayout(RootLayoutClass);                                                          
		if (MainMenuScreenClass)                                                                                
			UI->PushScreen(EOnsetUILayer::Game, MainMenuScreenClass);                                           
	}     
}
