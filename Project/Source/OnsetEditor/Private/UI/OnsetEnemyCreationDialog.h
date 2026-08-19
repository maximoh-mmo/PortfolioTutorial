// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UOnsetEnemyCreationData;

/**
 * Modal dialog for authoring a new DT_EnemyStats row. Hosts a UDetailsView bound to
 * a UOnsetEnemyCreationData (name + auto-create profiles). Create/Cancel close the
 * owner window; AddDefinition reads the result afterwards.
 */
class SEnemyCreationDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEnemyCreationDialog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UOnsetEnemyCreationData* InData);

	void SetOwnerWindow(const TWeakPtr<SWindow>& InOwnerWindow);

	bool ShouldCreate() const { return bCreate; }

	UOnsetEnemyCreationData* GetData() const { return Data; }

private:
	FReply OnCreate();
	FReply OnCancel();
	void CloseOwnerWindow();

	UOnsetEnemyCreationData* Data = nullptr;
	TWeakPtr<SWindow> OwnerWindow;
	bool bCreate = false;
};