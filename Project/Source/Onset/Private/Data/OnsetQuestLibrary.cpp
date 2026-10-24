// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/OnsetQuestLibrary.h"

#include "Engine/DataTable.h"
#include "Misc/ConfigCacheIni.h"

namespace OnsetQuestLibraryInternal
{
	TStrongObjectPtr<UDataTable> CachedTable;
	TMap<FName, FOnsetQuestDefinition> FallbackQuests;
	bool bDefaultsInitialized = false;

	void EnsureFallbackQuests()
	{
		if (bDefaultsInitialized)
		{
			return;
		}
		bDefaultsInitialized = true;

		// =========================================================================
		// FirstBlood (Legacy / baseline tutorial mission)
		// =========================================================================
		{
			FOnsetQuestDefinition Quest;
			Quest.DisplayName = FText::FromString(TEXT("First Blood"));
			Quest.Description = FText::FromString(TEXT("Neutralize hostiles outside the perimeter wall to verify weapon systems."));
			Quest.XpReward = 25;

			FOnsetQuestStageDefinition Stage1;
			Stage1.Description = FText::FromString(TEXT("Neutralize Grunts on the perimeter."));
			FOnsetQuestObjectiveDefinition Obj;
			Obj.Type = EOnsetQuestObjectiveType::Kill;
			Obj.Target = FName(TEXT("Grunt"));
			Obj.Count = 3;
			Obj.Description = FText::FromString(TEXT("Neutralize Grunts (3)"));
			Stage1.Objectives.Add(Obj);
			Quest.Stages.Add(Stage1);

			FallbackQuests.Add(FName(TEXT("FirstBlood")), Quest);
		}

		// =========================================================================
		// Q1_CadetInduction (Level 1: Inside the Academy Hub)
		// =========================================================================
		{
			FOnsetQuestDefinition Quest;
			Quest.DisplayName = FText::FromString(TEXT("Cadet Induction"));
			Quest.Description = FText::FromString(TEXT("Cycle the security bulkhead, calibrate combat telemetry with Instructor Kael, and report to Commander Vane."));
			Quest.XpReward = 30;

			// Stage 1: Cycle Bulkhead & speak to Kael
			FOnsetQuestStageDefinition Stage1;
			Stage1.Description = FText::FromString(TEXT("Complete Induction Orientation in the Academy."));

			FOnsetQuestObjectiveDefinition ObjBulkhead;
			ObjBulkhead.Type = EOnsetQuestObjectiveType::Interact;
			ObjBulkhead.Target = FName(TEXT("Door_Induction"));
			ObjBulkhead.Count = 1;
			ObjBulkhead.Description = FText::FromString(TEXT("Cycle Induction Security Bulkhead"));
			Stage1.Objectives.Add(ObjBulkhead);

			FOnsetQuestObjectiveDefinition ObjKael;
			ObjKael.Type = EOnsetQuestObjectiveType::Interact;
			ObjKael.Target = FName(TEXT("InstructorKael"));
			ObjKael.Count = 1;
			ObjKael.Description = FText::FromString(TEXT("Calibrate Combat Telemetry with Instructor Kael"));
			Stage1.Objectives.Add(ObjKael);

			Quest.Stages.Add(Stage1);

			// Stage 2: Report to Commander Vane
			FOnsetQuestStageDefinition Stage2;
			Stage2.Description = FText::FromString(TEXT("Report to Commander Vane at the South Gate."));

			FOnsetQuestObjectiveDefinition ObjVane;
			ObjVane.Type = EOnsetQuestObjectiveType::Interact;
			ObjVane.Target = FName(TEXT("ElderVane"));
			ObjVane.Count = 1;
			ObjVane.Description = FText::FromString(TEXT("Report to Commander Vane"));
			Stage2.Objectives.Add(ObjVane);

			Quest.Stages.Add(Stage2);

			FallbackQuests.Add(FName(TEXT("Q1_CadetInduction")), Quest);
		}

		// =========================================================================
		// Q2_BioDomeContainment (Level 1 -> 2: Sector 1)
		// =========================================================================
		{
			FOnsetQuestDefinition Quest;
			Quest.DisplayName = FText::FromString(TEXT("Bio-Dome Containment"));
			Quest.Description = FText::FromString(TEXT("Clear biological mutations in the Sector 1 Bio-Dome Terraces and scan the breached telemetry monitor."));
			Quest.XpReward = 45;

			// Stage 1: Cull Bio-Stalkers
			FOnsetQuestStageDefinition Stage1;
			Stage1.Description = FText::FromString(TEXT("Neutralize Bio-Stalkers in Sector 1."));

			FOnsetQuestObjectiveDefinition ObjKill;
			ObjKill.Type = EOnsetQuestObjectiveType::Kill;
			ObjKill.Target = FName(TEXT("Academy_Lv1_BioStalker"));
			ObjKill.Count = 8;
			ObjKill.Description = FText::FromString(TEXT("Neutralize Bio-Stalkers (8)"));
			Stage1.Objectives.Add(ObjKill);

			Quest.Stages.Add(Stage1);

			// Stage 2: Download logs & rendezvous with Operative Tam
			FOnsetQuestStageDefinition Stage2;
			Stage2.Description = FText::FromString(TEXT("Download monitor telemetry and rendezvous with Operative Tam."));

			FOnsetQuestObjectiveDefinition ObjScan;
			ObjScan.Type = EOnsetQuestObjectiveType::Interact;
			ObjScan.Target = FName(TEXT("Trigger_BioMonitor"));
			ObjScan.Count = 1;
			ObjScan.Description = FText::FromString(TEXT("Download Telemetry from Breached Bio-Monitor"));
			Stage2.Objectives.Add(ObjScan);

			FOnsetQuestObjectiveDefinition ObjTam;
			ObjTam.Type = EOnsetQuestObjectiveType::Interact;
			ObjTam.Target = FName(TEXT("OperativeTam"));
			ObjTam.Count = 1;
			ObjTam.Description = FText::FromString(TEXT("Rendezvous with Operative Tam"));
			Stage2.Objectives.Add(ObjTam);

			Quest.Stages.Add(Stage2);

			FallbackQuests.Add(FName(TEXT("Q2_BioDomeContainment")), Quest);
		}

		// =========================================================================
		// Q3_CoolantMalfunction (Level 2 -> 3: Sector 2)
		// =========================================================================
		{
			FOnsetQuestDefinition Quest;
			Quest.DisplayName = FText::FromString(TEXT("Coolant Malfunction"));
			Quest.Description = FText::FromString(TEXT("Cycle the turbine floodgate console and eliminate corrosive rogue drones in Sector 2."));
			Quest.XpReward = 55;

			// Stage 1: Engage console
			FOnsetQuestStageDefinition Stage1;
			Stage1.Description = FText::FromString(TEXT("Drain the reservoir floodgate."));

			FOnsetQuestObjectiveDefinition ObjConsole;
			ObjConsole.Type = EOnsetQuestObjectiveType::Interact;
			ObjConsole.Target = FName(TEXT("Trigger_CoolantConsole"));
			ObjConsole.Count = 1;
			ObjConsole.Description = FText::FromString(TEXT("Cycle Coolant Flow Console"));
			Stage1.Objectives.Add(ObjConsole);

			Quest.Stages.Add(Stage1);

			// Stage 2: Destroy drones & reach Mira
			FOnsetQuestStageDefinition Stage2;
			Stage2.Description = FText::FromString(TEXT("Destroy rogue drones and reach Recon Officer Mira."));

			FOnsetQuestObjectiveDefinition ObjKillDrones;
			ObjKillDrones.Type = EOnsetQuestObjectiveType::Kill;
			ObjKillDrones.Target = FName(TEXT("Academy_Lv2_RogueDrone"));
			ObjKillDrones.Count = 12;
			ObjKillDrones.Description = FText::FromString(TEXT("Destroy Rogue Drones (12)"));
			Stage2.Objectives.Add(ObjKillDrones);

			FOnsetQuestObjectiveDefinition ObjMira;
			ObjMira.Type = EOnsetQuestObjectiveType::Interact;
			ObjMira.Target = FName(TEXT("ReconMira"));
			ObjMira.Count = 1;
			ObjMira.Description = FText::FromString(TEXT("Report to Recon Officer Mira"));
			Stage2.Objectives.Add(ObjMira);

			Quest.Stages.Add(Stage2);

			FallbackQuests.Add(FName(TEXT("Q3_CoolantMalfunction")), Quest);
		}

		// =========================================================================
		// Q4_SignalInterference (Level 3 -> 4: Sectors 3 & 4)
		// =========================================================================
		{
			FOnsetQuestDefinition Quest;
			Quest.DisplayName = FText::FromString(TEXT("Signal Interference"));
			Quest.Description = FText::FromString(TEXT("Reclaim the comms relay dish, restore the East Service Airlock, and stabilize the geothermal excavation pylons."));
			Quest.XpReward = 70;

			// Stage 1: Clear Outcasts and unlock East Airlock
			FOnsetQuestStageDefinition Stage1;
			Stage1.Description = FText::FromString(TEXT("Eliminate Outcast scavengers and engage the East Airlock shortcut."));

			FOnsetQuestObjectiveDefinition ObjKillRaiders;
			ObjKillRaiders.Type = EOnsetQuestObjectiveType::Kill;
			ObjKillRaiders.Target = FName(TEXT("Academy_Lv3_OutcastRaider"));
			ObjKillRaiders.Count = 12;
			ObjKillRaiders.Description = FText::FromString(TEXT("Eliminate Outcast Raiders (12)"));
			Stage1.Objectives.Add(ObjKillRaiders);

			FOnsetQuestObjectiveDefinition ObjShortcut;
			ObjShortcut.Type = EOnsetQuestObjectiveType::Interact;
			ObjShortcut.Target = FName(TEXT("Trigger_EastShortcut"));
			ObjShortcut.Count = 1;
			ObjShortcut.Description = FText::FromString(TEXT("Activate East Airlock Override Console"));
			Stage1.Objectives.Add(ObjShortcut);

			Quest.Stages.Add(Stage1);

			// Stage 2: Stabilize excavation pylons & destroy heavy mechs
			FOnsetQuestStageDefinition Stage2;
			Stage2.Description = FText::FromString(TEXT("Dismantle excavator mechs in the geothermal bore pit."));

			FOnsetQuestObjectiveDefinition ObjKillMechs;
			ObjKillMechs.Type = EOnsetQuestObjectiveType::Kill;
			ObjKillMechs.Target = FName(TEXT("Academy_Lv4_ExcavatorUnit"));
			ObjKillMechs.Count = 12;
			ObjKillMechs.Description = FText::FromString(TEXT("Dismantle Excavator Units (12)"));
			Stage2.Objectives.Add(ObjKillMechs);

			FOnsetQuestObjectiveDefinition ObjPylon;
			ObjPylon.Type = EOnsetQuestObjectiveType::Interact;
			ObjPylon.Target = FName(TEXT("Trigger_PsionicPylon"));
			ObjPylon.Count = 1;
			ObjPylon.Description = FText::FromString(TEXT("Cycle Psionic Stabilization Pylon"));
			Stage2.Objectives.Add(ObjPylon);

			Quest.Stages.Add(Stage2);

			FallbackQuests.Add(FName(TEXT("Q4_SignalInterference")), Quest);
		}

		// =========================================================================
		// Q5_ThePatternRevealed (Level 5 Finale: Sector 5)
		// =========================================================================
		{
			FOnsetQuestDefinition Quest;
			Quest.DisplayName = FText::FromString(TEXT("The Pattern Revealed"));
			Quest.Description = FText::FromString(TEXT("Unlock the West Containment Bulkhead, purge the Aberration Breach, and inspect the Frontier Slipstream Gate."));
			Quest.XpReward = 90;

			FOnsetQuestStageDefinition Stage1;
			Stage1.Description = FText::FromString(TEXT("Contain the Aberration Breach in Sector 5."));

			FOnsetQuestObjectiveDefinition ObjWestShortcut;
			ObjWestShortcut.Type = EOnsetQuestObjectiveType::Interact;
			ObjWestShortcut.Target = FName(TEXT("Trigger_WestShortcut"));
			ObjWestShortcut.Count = 1;
			ObjWestShortcut.Description = FText::FromString(TEXT("Engage West Bulkhead Override Console"));
			Stage1.Objectives.Add(ObjWestShortcut);

			FOnsetQuestObjectiveDefinition ObjPurge;
			ObjPurge.Type = EOnsetQuestObjectiveType::Kill;
			ObjPurge.Target = FName(TEXT("Academy_Lv5_RiftAberration"));
			ObjPurge.Count = 15;
			ObjPurge.Description = FText::FromString(TEXT("Purge Rift Aberrations (15)"));
			Stage1.Objectives.Add(ObjPurge);

			Quest.Stages.Add(Stage1);

			FallbackQuests.Add(FName(TEXT("Q5_ThePatternRevealed")), Quest);
		}
	}
}

FString UOnsetQuestLibrary::GetTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("QuestDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Data/DT_Quests.DT_Quests");
	}
	return Path;
}

UDataTable* UOnsetQuestLibrary::GetTable()
{
	if (OnsetQuestLibraryInternal::CachedTable)
	{
		return OnsetQuestLibraryInternal::CachedTable.Get();
	}

	const FString TablePath = GetTablePath();
	if (UDataTable* Table = Cast<UDataTable>(StaticFindObject(UDataTable::StaticClass(), nullptr, *TablePath)))
	{
		OnsetQuestLibraryInternal::CachedTable = TStrongObjectPtr<UDataTable>(Table);
		return Table;
	}

	const FString PkgName = FPackageName::ObjectPathToPackageName(TablePath);
	if (FPackageName::DoesPackageExist(PkgName))
	{
		if (UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath)))
		{
			OnsetQuestLibraryInternal::CachedTable = TStrongObjectPtr<UDataTable>(Table);
			return Table;
		}
	}
	return nullptr;
}

const FOnsetQuestDefinition* UOnsetQuestLibrary::GetQuest(FName RowName)
{
	if (RowName.IsNone())
	{
		return nullptr;
	}

	OnsetQuestLibraryInternal::EnsureFallbackQuests();

	if (UDataTable* Table = GetTable())
	{
		if (const FOnsetQuestDefinition* Row = Table->FindRow<FOnsetQuestDefinition>(RowName, nullptr, /*bWarnIfRowMissing=*/false))
		{
			return Row;
		}
	}

	return OnsetQuestLibraryInternal::FallbackQuests.Find(RowName);
}
