// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/OnsetCCDiminishingComponent.h"

float UOnsetCCDiminishingComponent::GetDiminishedDuration(FGameplayTag CCType, float BaseDuration)
{
	if (!CCType.IsValid() || BaseDuration <= 0.0f || !GetWorld())
	{
		return BaseDuration;
	}

	const double Now = GetWorld()->GetTimeSeconds();

	int32& Stack = CCStacks.FindOrAdd(CCType);
	const double* LastTime = LastCCApplicationTimes.Find(CCType);
	if (!LastTime || (Now - *LastTime) > CCDiminishingWindow)
	{
		// Chain broken: the next application starts back at full duration.
		Stack = 0;
	}

	LastCCApplicationTimes.Add(CCType, Now);
	++Stack;

	// 1st -> 100%, 2nd -> 50%, 3rd -> 25%, 4th+ -> immune (0).
	constexpr float DR[4] = { 1.0f, 0.5f, 0.25f, 0.0f };
	const int32 Index = FMath::Clamp(Stack - 1, 0, 3);
	return BaseDuration * DR[Index];
}

void UOnsetCCDiminishingComponent::ResetDiminishingReturns()
{
	LastCCApplicationTimes.Reset();
	CCStacks.Reset();
}