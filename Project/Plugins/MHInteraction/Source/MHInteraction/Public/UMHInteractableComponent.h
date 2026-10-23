#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IMHInteractable.h"
#include "UMHInteractableComponent.generated.h"

class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMHOnInteractedSignature, const FMHInteractionContext&, Context);
DECLARE_MULTICAST_DELEGATE_OneParam(FMHOnInteractedNativeSignature, const FMHInteractionContext& /*Context*/);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FMHCanInteractQueryDelegate, const FMHInteractionContext& /*Context*/, FText& /*OutFailureReason*/);
DECLARE_DELEGATE_OneParam(FMHModifyDefinitionDelegate, FMHInteractionDefinition& /*InOutDefinition*/);

/**
 * Drop-in component that makes its owning Actor interactable via IMHInteractable.
 * Supports explicit click/button interaction, overlap-driven triggers, single-use latches,
 * cooldowns, replication, and both C++ and Blueprint callbacks.
 */
UCLASS(ClassGroup = (MH), meta = (BlueprintSpawnableComponent))
class MHINTERACTION_API UMHInteractableComponent : public UActorComponent, public IMHInteractable
{
	GENERATED_BODY()

public:
	UMHInteractableComponent();

	/** Metadata and settings for this interactable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "MH|Interaction")
	FMHInteractionDefinition Definition;

	/** Whether this interactable is currently active and available for interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "MH|Interaction")
	bool bEnabled = true;

	/** If true, this interactable can only be successfully executed once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MH|Interaction")
	bool bSingleUse = false;

	/** Minimum seconds between consecutive interactions (0 = no cooldown). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MH|Interaction", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 0.0f;

	/** Broadcast in Blueprint when ExecuteInteraction succeeds. */
	UPROPERTY(BlueprintAssignable, Category = "MH|Interaction")
	FMHOnInteractedSignature OnInteracted;

	/** Broadcast in C++ when ExecuteInteraction succeeds. */
	FMHOnInteractedNativeSignature OnInteractedNative;

	/** Optional C++ predicate for additional owner-specific CanInteract rules. */
	FMHCanInteractQueryDelegate OnCanInteractQuery;

	/** Optional C++ callback allowing the owning actor to dynamically adjust the definition (e.g. "Open" vs "Close"). */
	FMHModifyDefinitionDelegate OnModifyDefinition;

	UFUNCTION(BlueprintCallable, Category = "MH|Interaction")
	void SetEnabled(bool bInEnabled);

	UFUNCTION(BlueprintPure, Category = "MH|Interaction")
	bool IsEnabled() const { return bEnabled; }

	UFUNCTION(BlueprintPure, Category = "MH|Interaction")
	bool HasBeenUsed() const { return bHasBeenUsed; }

	UFUNCTION(BlueprintCallable, Category = "MH|Interaction")
	void ResetUsage();

	/** Binds a primitive component on the owner to trigger overlap interactions when TriggerMode allows overlap. */
	UFUNCTION(BlueprintCallable, Category = "MH|Interaction")
	void BindOverlapPrimitive(UPrimitiveComponent* Primitive);

	// --- IMHInteractable ---
	virtual void GetInteractionDefinition_Implementation(FMHInteractionDefinition& OutDefinition) const override;
	virtual bool CanInteract_Implementation(const FMHInteractionContext& Context, FText& OutFailureReason) const override;
	virtual bool ExecuteInteraction_Implementation(const FMHInteractionContext& Context) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandlePrimitiveBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

private:
	UPROPERTY(Replicated)
	bool bHasBeenUsed = false;

	double LastInteractionWorldTime = -1000.0;

	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> BoundOverlapPrimitive;
};
