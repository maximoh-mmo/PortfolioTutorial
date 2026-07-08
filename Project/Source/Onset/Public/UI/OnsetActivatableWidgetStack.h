#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "OnsetActivatableWidgetStack.generated.h"
/**
 * Identifies which layer of WBP_RootLayout a screen or modal should be pushed onto.
 * Keep this in sync with the stacks exposed by UOnsetRootLayout.
 */
UENUM(BlueprintType)
enum class EOnsetUILayer : uint8
{
	Game,
	Menu,
	Modal
};

/**
 * Project-specific subclass of UCommonActivatableWidgetStack.
 * No behavior yet - exists so every stack in the project shares one type,
 * which makes it trivial to add shared transition/analytics hooks later
 * without re-pointing every Blueprint stack instance.
 */
UCLASS()
class ONSET_API UOnsetActivatableWidgetStack : public UCommonActivatableWidgetStack
{
	GENERATED_BODY()

public:
	/** Which UI layer this stack instance represents. Set per-instance in the WBP_RootLayout designer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Onset|UI")
	EOnsetUILayer Layer = EOnsetUILayer::Menu;
};