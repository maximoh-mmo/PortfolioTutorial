#pragma once
 
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/OnsetActivatableWidgetStack.h"
#include "OnsetRootLayout.generated.h"
 
/**
 * The single root UI widget, added to the viewport once at boot
 * (see UOnsetUIManagerSubsystem::InitializeRootLayout).
 *
 * Holds one UOnsetActivatableWidgetStack per layer. Build WBP_RootLayout
 * from this class and name the three child stack widgets to match the
 * BindWidget names below: GameLayerStack, MenuLayerStack, ModalLayerStack.
 */
UCLASS(Abstract)
class ONSET_API UOnsetRootLayout : public UUserWidget
{
	GENERATED_BODY()
 
public:
	/** Returns the stack widget for the given layer, or nullptr if not bound. */
	UOnsetActivatableWidgetStack* GetStackForLayer(EOnsetUILayer Layer) const;
 
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UOnsetActivatableWidgetStack> GameLayerStack;
 
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UOnsetActivatableWidgetStack> MenuLayerStack;
 
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UOnsetActivatableWidgetStack> ModalLayerStack;
};