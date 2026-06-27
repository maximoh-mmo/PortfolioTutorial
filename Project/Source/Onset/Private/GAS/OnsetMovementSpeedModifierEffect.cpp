#include "GAS/OnsetMovementSpeedModifierEffect.h"
#include "GAS/OnsetMovementAttributeSet.h"

UOnsetMovementSpeedModifierEffect::UOnsetMovementSpeedModifierEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UOnsetMovementAttributeSet::GetMovementSpeedAttribute();
	Modifier.ModifierOp = EGameplayModOp::MultiplyCompound;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("MoveSpeedMod");
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(Modifier);
}
