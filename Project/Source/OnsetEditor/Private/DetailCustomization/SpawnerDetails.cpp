// Fill out your copyright notice in the Description page of Project Settings.


#include "DetailCustomization/SpawnerDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Spawning/OnsetSpawner.h"
#include "Spawning/SpawnPoint.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "SpawnerDetails"

TSharedRef<IDetailCustomization> FSpawnerDetails::MakeInstance()
{   
	return MakeShared<FSpawnerDetails>();
}

void FSpawnerDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailLayout.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() != 1)
	{
		return;
	}

	Spawner = Cast<AOnsetSpawner>(Objects[0].Get());

	if (!Spawner.IsValid())
	{
		return;
	}

	IDetailCategoryBuilder& Category =
		DetailLayout.EditCategory("Spawning");

	Category.AddCustomRow(
		LOCTEXT("AddSpawnPointSearch", "Add Spawn Point")
	)
	.WholeRowContent()
	[
		SNew(SButton)
		.Text(LOCTEXT("AddSpawnPoint", "Add Spawn Point"))
		.OnClicked_Lambda(
			[this]()
			{
				AddSpawnPoint();

				return FReply::Handled();
			}
		)
	];
}

void FSpawnerDetails::AddSpawnPoint()
{
	AOnsetSpawner* CurrentSpawner = Spawner.Get();

	if (!CurrentSpawner)
	{
		return;
	}

	UWorld* World = CurrentSpawner->GetWorld();

	if (!World)
	{
		return;
	}

	FScopedTransaction Transaction(
		LOCTEXT("AddSpawnPointTransaction", "Add Spawn Point")
	);

	CurrentSpawner->Modify();

	FActorSpawnParameters SpawnParams;

	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector Location =
		CurrentSpawner->GetActorLocation();

	const FRotator Rotation =
		CurrentSpawner->GetActorRotation();

	ASpawnPoint* NewSpawnPoint =
		World->SpawnActor<ASpawnPoint>(
			ASpawnPoint::StaticClass(),
			Location,
			Rotation,
			SpawnParams
		);

	if (!NewSpawnPoint)
	{
		return;
	}

	NewSpawnPoint->Modify();
	
	CurrentSpawner->SpawnPoints.Add(NewSpawnPoint);

	if (CurrentSpawner->Config.EnemyVisualProfile)
	{
		const UVisualProfile* Profile = 
			CurrentSpawner->Config.EnemyVisualProfile.Get();
		
		NewSpawnPoint->SetPreview(
			Profile->SkeletalMesh.LoadSynchronous(),
			Profile->OverrideMaterial.Get());
	}
	
	// Make sure the level knows it has changed.
	NewSpawnPoint->MarkPackageDirty();
	CurrentSpawner->MarkPackageDirty();
	
	// Select the newly created SpawnPoint.
	GEditor->SelectNone(false, true);

	GEditor->SelectActor(
		NewSpawnPoint,
		true,
		true
	);

	GEditor->NoteSelectionChange();
}
#undef LOCTEXT_NAMESPACE