// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "IDetailCustomization.h"

class FSpawnerDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) override;
	
private:
	void AddSpawnPoint();
	TWeakObjectPtr<class AOnsetSpawner> Spawner;
};
