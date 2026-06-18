#include "StateTree/Tasks/Player/PlayerAttackTask.h"

/**
	- Get all granted abilities from ASC (cooldown-aware)                                                                                                                        
	- Count enemies near current target (overlap query)                                                                                                                          
	- Pick: AoE if 3+ enemies, else highest priority single-target ability                                                                                                       
	- TryActivateAbilityByClass on the selected ability                                                                                                                          
	- Return Succeeded on activation, Failed if nothing available  
*/
EStateTreeRunStatus FPlayerAttackTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	return FOnsetStateTreeTask::EnterState(Context, Transition);
}

EStateTreeRunStatus FPlayerAttackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return FOnsetStateTreeTask::Tick(Context, DeltaTime);
}
