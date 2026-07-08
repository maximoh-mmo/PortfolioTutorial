// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
 
#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "OnsetButtonBase.generated.h"
 
class USoundBase;
 
/**
 * Base class for every button in Onset. Style (chamfered shape, colors,
 * fonts) is driven by UCommonButtonStyle data assets assigned per-Blueprint;
 * this class centralizes shared *behavior* - audio feedback today, anything
 * else later - so it's set once rather than wired per button instance.
 */
UCLASS(Abstract)
class ONSET_API UOnsetButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()
 
public:
	UOnsetButtonBase(const FObjectInitializer& ObjectInitializer);
 
protected:
	//~ Begin UCommonButtonBase interface
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnClicked() override;
	//~ End UCommonButtonBase interface
 
	UPROPERTY(EditDefaultsOnly, Category = "Onset|Button|Audio")
	TObjectPtr<USoundBase> HoverSound;
 
	UPROPERTY(EditDefaultsOnly, Category = "Onset|Button|Audio")
	TObjectPtr<USoundBase> ClickSound;
};
 