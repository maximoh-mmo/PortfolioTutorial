// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/OnsetDialogueLibrary.h"

#include "Engine/DataTable.h"
#include "Misc/ConfigCacheIni.h"
#include "Quest/UOnsetQuestComponent.h"

namespace OnsetDialogueLibraryInternal
{
	TStrongObjectPtr<UDataTable> CachedDialogueTable;
	TStrongObjectPtr<UDataTable> CachedNPCTable;
	TMap<FName, FOnsetDialogueRow> FallbackDialogueRows;
	TMap<FName, FOnsetNPCDefinition> FallbackNPCDefinitions;
	bool bDefaultsInitialized = false;

	void AddRow(FName RowName, const FText& Speaker, const FText& DialogueText, const TArray<FOnsetDialogueChoice>& Choices)
	{
		FOnsetDialogueRow Row;
		Row.SpeakerName = Speaker;
		Row.DialogueText = DialogueText;
		Row.Choices = Choices;
		FallbackDialogueRows.Add(RowName, Row);
	}

	void EnsureDefaultFallbacks()
	{
		if (bDefaultsInitialized)
		{
			return;
		}
		bDefaultsInitialized = true;

		// =========================================================================
		// 1. DIRECTOR SOLEN (Induction Concourse - Academy Safe Hub)
		// =========================================================================
		{
			// Solen_Intro
			FOnsetDialogueChoice ChoiceAcceptQ1;
			ChoiceAcceptQ1.ResponseText = FText::FromString(TEXT("Cadet reporting for induction, Director. [Accept: Cadet Induction]"));
			ChoiceAcceptQ1.RequiredQuestNotStarted = FName(TEXT("Q1_CadetInduction"));
			ChoiceAcceptQ1.GrantQuestOnSelect = FName(TEXT("Q1_CadetInduction"));
			ChoiceAcceptQ1.NextDialogueRow = FName(TEXT("Solen_Accepted"));

			FOnsetDialogueChoice ChoiceLore;
			ChoiceLore.ResponseText = FText::FromString(TEXT("What is the mission of this facility?"));
			ChoiceLore.NextDialogueRow = FName(TEXT("Solen_Lore"));

			FOnsetDialogueChoice ChoiceDismiss;
			ChoiceDismiss.ResponseText = FText::FromString(TEXT("Standing by."));
			ChoiceDismiss.NextDialogueRow = NAME_None;

			AddRow(FName(TEXT("Solen_Intro")),
				FText::FromString(TEXT("Director Solen")),
				FText::FromString(TEXT("Welcome to the Academy Concourse, Cadet. Our forward outposts report anomalous energy surges along the perimeter. Before you step outside, you must calibrate your stillsuit telemetry.")),
				{ ChoiceAcceptQ1, ChoiceLore, ChoiceDismiss });

			// Solen_Lore
			FOnsetDialogueChoice ChoiceBack;
			ChoiceBack.ResponseText = FText::FromString(TEXT("Understood, Director. Ready for assignment."));
			ChoiceBack.NextDialogueRow = FName(TEXT("Solen_Intro"));

			AddRow(FName(TEXT("Solen_Lore")),
				FText::FromString(TEXT("Director Solen")),
				FText::FromString(TEXT("The public sees isolated skirmishes. Leadership sees a pattern. Here we forge operatives who understand both martial discipline and the psionic substrate that threatens our colonies.")),
				{ ChoiceBack, ChoiceDismiss });

			// Solen_Accepted
			FOnsetDialogueChoice ChoiceConfirm;
			ChoiceConfirm.ResponseText = FText::FromString(TEXT("On my way."));
			ChoiceConfirm.NextDialogueRow = NAME_None;

			AddRow(FName(TEXT("Solen_Accepted")),
				FText::FromString(TEXT("Director Solen")),
				FText::FromString(TEXT("Cycle the Induction Security Bulkhead, verify weapon calibration with Instructor Kael in the Combat Sim Arena, then report to Commander Vane at the South Gate.")),
				{ ChoiceConfirm });

			// Solen_Completed
			AddRow(FName(TEXT("Solen_Completed")),
				FText::FromString(TEXT("Director Solen")),
				FText::FromString(TEXT("Your induction telemetry is synced to Command network. Vanguard Operations has full jurisdiction over your perimeter sorties. Serve with honour.")),
				{ ChoiceDismiss });

			// Def
			FOnsetNPCDefinition SolenDef;
			SolenDef.DisplayName = FText::FromString(TEXT("Director Solen"));
			SolenDef.TitleOrRole = FText::FromString(TEXT("Academy Provost"));
			SolenDef.InteractionPrompt = FText::FromString(TEXT("Speak"));
			SolenDef.DefaultDialogueRow = FName(TEXT("Solen_Intro"));
			SolenDef.MilitaryRank = EOnsetRankId::DirectorateProvost;
			SolenDef.HierarchyBranch = EOnsetHierarchyBranch::Substrate;
			SolenDef.Specialism = EOnsetSpecialism::SubstratePhysics;
			SolenDef.ProtocolStyle = EOnsetProtocolStyle::ResonantNod;

			FOnsetNPCQuestDialogueOverride SolenOverride;
			SolenOverride.QuestRowName = FName(TEXT("Q1_CadetInduction"));
			SolenOverride.NotStartedDialogueRow = FName(TEXT("Solen_Intro"));
			SolenOverride.InProgressDialogueRow = FName(TEXT("Solen_Accepted"));
			SolenOverride.CompletedDialogueRow = FName(TEXT("Solen_Completed"));
			SolenDef.QuestDialogueOverrides.Add(SolenOverride);

			FallbackNPCDefinitions.Add(FName(TEXT("DirectorSolen")), SolenDef);
			FallbackNPCDefinitions.Add(FName(TEXT("NPC_DirectorSolen")), SolenDef);
		}

		// =========================================================================
		// 2. INSTRUCTOR KAEL (Combat Sim Arena & Armory)
		// =========================================================================
		{
			FOnsetDialogueChoice ChoiceSimCheck;
			ChoiceSimCheck.ResponseText = FText::FromString(TEXT("Calibrating weapon and shield systems for field trials."));
			ChoiceSimCheck.ReportGenericObjectiveTarget = FName(TEXT("InstructorKael_Talk"));
			ChoiceSimCheck.NextDialogueRow = FName(TEXT("Kael_Calibrated"));

			FOnsetDialogueChoice ChoiceKaelLore;
			ChoiceKaelLore.ResponseText = FText::FromString(TEXT("What tactical doctrine should I apply against perimeter hostiles?"));
			ChoiceKaelLore.NextDialogueRow = FName(TEXT("Kael_Lore"));

			FOnsetDialogueChoice ChoiceDismiss;
			ChoiceDismiss.ResponseText = FText::FromString(TEXT("At ease."));
			ChoiceDismiss.NextDialogueRow = NAME_None;

			AddRow(FName(TEXT("Kael_Intro")),
				FText::FromString(TEXT("Instructor Kael")),
				FText::FromString(TEXT("Watch your spacing, Cadet! In the field, hesitation gets you cleaved by a Bio-Stalker. Keep your guard up and strike in rhythm.")),
				{ ChoiceSimCheck, ChoiceKaelLore, ChoiceDismiss });

			AddRow(FName(TEXT("Kael_Calibrated")),
				FText::FromString(TEXT("Instructor Kael")),
				FText::FromString(TEXT("Telemetry confirms clean conduit flow. Your weapon-scaling matrix is active. Now move to the South Gate—Commander Vane is waiting for you.")),
				{ ChoiceDismiss });

			FOnsetDialogueChoice ChoiceBack;
			ChoiceBack.ResponseText = FText::FromString(TEXT("Understood, Instructor."));
			ChoiceBack.NextDialogueRow = FName(TEXT("Kael_Intro"));

			AddRow(FName(TEXT("Kael_Lore")),
				FText::FromString(TEXT("Instructor Kael")),
				FText::FromString(TEXT("Skirmish packs will probe your defenses. Swarm pockets will try to overwhelm your cooldowns. Use your defensive mitigation and position yourself so you aren't surrounded.")),
				{ ChoiceBack, ChoiceDismiss });

			FOnsetNPCDefinition KaelDef;
			KaelDef.DisplayName = FText::FromString(TEXT("Instructor Kael"));
			KaelDef.TitleOrRole = FText::FromString(TEXT("Combat Master (House Vanguard)"));
			KaelDef.InteractionPrompt = FText::FromString(TEXT("Spar / Train"));
			KaelDef.DefaultDialogueRow = FName(TEXT("Kael_Intro"));
			KaelDef.MilitaryRank = EOnsetRankId::SeniorOfficer;
			KaelDef.HierarchyBranch = EOnsetHierarchyBranch::Military;
			KaelDef.Specialism = EOnsetSpecialism::BastionDefense;
			KaelDef.ProtocolStyle = EOnsetProtocolStyle::MilitarySalute;

			FallbackNPCDefinitions.Add(FName(TEXT("InstructorKael")), KaelDef);
			FallbackNPCDefinitions.Add(FName(TEXT("NPC_InstructorKael")), KaelDef);
		}

		// =========================================================================
		// 3. QUARTERMASTER BREN (Armory & Issue Lockers)
		// =========================================================================
		{
			FOnsetDialogueChoice ChoiceSupplies;
			ChoiceSupplies.ResponseText = FText::FromString(TEXT("Requesting armor calibration and ration requisitions."));
			ChoiceSupplies.NextDialogueRow = FName(TEXT("Bren_Stock"));

			FOnsetDialogueChoice ChoiceDismiss;
			ChoiceDismiss.ResponseText = FText::FromString(TEXT("Just inspecting the armory."));
			ChoiceDismiss.NextDialogueRow = NAME_None;

			AddRow(FName(TEXT("Bren_Intro")),
				FText::FromString(TEXT("Quartermaster Bren")),
				FText::FromString(TEXT("Standard-issue composite weave, pressurized stillsuit liners, and high-frequency blades. What does House Aegis have on your manifest today, Cadet?")),
				{ ChoiceSupplies, ChoiceDismiss });

			AddRow(FName(TEXT("Bren_Stock")),
				FText::FromString(TEXT("Quartermaster Bren")),
				FText::FromString(TEXT("Complete combat assignments beyond the perimeter wall. Each sector breach cleared logs commendation vouchers into your personal requisition account.")),
				{ ChoiceDismiss });

			FOnsetNPCDefinition BrenDef;
			BrenDef.DisplayName = FText::FromString(TEXT("Quartermaster Bren"));
			BrenDef.TitleOrRole = FText::FromString(TEXT("Logistics Officer (House Aegis)"));
			BrenDef.InteractionPrompt = FText::FromString(TEXT("Requisition"));
			BrenDef.DefaultDialogueRow = FName(TEXT("Bren_Intro"));
			BrenDef.MilitaryRank = EOnsetRankId::LogisticsOfficer;
			BrenDef.HierarchyBranch = EOnsetHierarchyBranch::Logistics;
			BrenDef.Specialism = EOnsetSpecialism::OrdnanceLogistics;
			BrenDef.ProtocolStyle = EOnsetProtocolStyle::MilitarySalute;

			FallbackNPCDefinitions.Add(FName(TEXT("QuartermasterBren")), BrenDef);
			FallbackNPCDefinitions.Add(FName(TEXT("NPC_QuartermasterBren")), BrenDef);
		}

		// =========================================================================
		// 4. COMMANDER VANE (Central Quadrangle / South Main Gate)
		// =========================================================================
		{
			// Intro / First Blood / Q2 Offer
			FOnsetDialogueChoice AcceptQ2;
			AcceptQ2.ResponseText = FText::FromString(TEXT("Ready for perimeter sortie, Commander. [Accept: Bio-Dome Containment]"));
			AcceptQ2.RequiredQuestNotStarted = FName(TEXT("Q2_BioDomeContainment"));
			AcceptQ2.GrantQuestOnSelect = FName(TEXT("Q2_BioDomeContainment"));
			AcceptQ2.NextDialogueRow = FName(TEXT("Vane_Q2Accepted"));

			FOnsetDialogueChoice ChoiceFirstBlood;
			ChoiceFirstBlood.ResponseText = FText::FromString(TEXT("Reporting for duty, Commander. [Accept Mission: FirstBlood]"));
			ChoiceFirstBlood.RequiredQuestNotStarted = FName(TEXT("FirstBlood"));
			ChoiceFirstBlood.GrantQuestOnSelect = FName(TEXT("FirstBlood"));
			ChoiceFirstBlood.NextDialogueRow = FName(TEXT("ElderVane_Accepted"));

			FOnsetDialogueChoice ChoiceVaneLore;
			ChoiceVaneLore.ResponseText = FText::FromString(TEXT("What is the tactical situation beyond the perimeter wall?"));
			ChoiceVaneLore.NextDialogueRow = FName(TEXT("ElderVane_Lore"));

			FOnsetDialogueChoice ChoiceDismiss;
			ChoiceDismiss.ResponseText = FText::FromString(TEXT("Dismissed."));
			ChoiceDismiss.NextDialogueRow = NAME_None;

			AddRow(FName(TEXT("ElderVane_Intro")),
				FText::FromString(TEXT("Commander Vane")),
				FText::FromString(TEXT("Cadet, telemetry shows rising aberration signatures outside the South Blast Gate. Sector 1 hydroponic domes are overrun with Bio-Stalkers. Are you prepared for deployment?")),
				{ AcceptQ2, ChoiceFirstBlood, ChoiceVaneLore, ChoiceDismiss });

			AddRow(FName(TEXT("ElderVane_Lore")),
				FText::FromString(TEXT("Commander Vane")),
				FText::FromString(TEXT("Security bulkheads seal the inner sectors. Look for override consoles and power relays along the service corridors to unlock direct transit back to the Academy.")),
				{ ChoiceVaneLore, ChoiceDismiss });

			AddRow(FName(TEXT("ElderVane_Accepted")),
				FText::FromString(TEXT("Commander Vane")),
				FText::FromString(TEXT("Keep your stillsuit telemetry active and neutralize the hostiles. Report back to Command once the sector is secure.")),
				{ ChoiceDismiss });

			AddRow(FName(TEXT("Vane_Q2Accepted")),
				FText::FromString(TEXT("Commander Vane")),
				FText::FromString(TEXT("Step through the South Blast Gate into Sector 1. Cull the Bio-Stalker packs, extract logs from the Breached Bio-Monitor, and rendezvous with Operative Tam at the forward crossroads.")),
				{ ChoiceDismiss });

			AddRow(FName(TEXT("ElderVane_Completed")),
				FText::FromString(TEXT("Commander Vane")),
				FText::FromString(TEXT("Clean execution, Operative. Your service record has been updated with Academy Command.")),
				{ ChoiceDismiss });

			FOnsetNPCDefinition VaneDef;
			VaneDef.DisplayName = FText::FromString(TEXT("Commander Vane"));
			VaneDef.TitleOrRole = FText::FromString(TEXT("Vanguard Operations Officer"));
			VaneDef.InteractionPrompt = FText::FromString(TEXT("Briefing"));
			VaneDef.DefaultDialogueRow = FName(TEXT("ElderVane_Intro"));
			VaneDef.MilitaryRank = EOnsetRankId::LegatusCommander;
			VaneDef.HierarchyBranch = EOnsetHierarchyBranch::Military;
			VaneDef.Specialism = EOnsetSpecialism::ShockAssault;
			VaneDef.ProtocolStyle = EOnsetProtocolStyle::MilitarySalute;

			FOnsetNPCQuestDialogueOverride VaneOverride;
			VaneOverride.QuestRowName = FName(TEXT("Q2_BioDomeContainment"));
			VaneOverride.NotStartedDialogueRow = FName(TEXT("ElderVane_Intro"));
			VaneOverride.InProgressDialogueRow = FName(TEXT("Vane_Q2Accepted"));
			VaneOverride.CompletedDialogueRow = FName(TEXT("ElderVane_Completed"));
			VaneDef.QuestDialogueOverrides.Add(VaneOverride);

			FallbackNPCDefinitions.Add(FName(TEXT("ElderVane")), VaneDef);
			FallbackNPCDefinitions.Add(FName(TEXT("NPC_CommanderVane")), VaneDef);
		}

		// =========================================================================
		// 5. ANALYST LYRA (Central Quadrangle Holo-Plaza)
		// =========================================================================
		{
			FOnsetDialogueChoice ChoiceAberrationData;
			ChoiceAberrationData.ResponseText = FText::FromString(TEXT("Explain the elemental affinities of the perimeter threats."));
			ChoiceAberrationData.NextDialogueRow = FName(TEXT("Lyra_Affinities"));

			FOnsetDialogueChoice ChoiceDismiss;
			ChoiceDismiss.ResponseText = FText::FromString(TEXT("Thank you, Analyst."));
			ChoiceDismiss.NextDialogueRow = NAME_None;

			AddRow(FName(TEXT("Lyra_Intro")),
				FText::FromString(TEXT("Analyst Lyra")),
				FText::FromString(TEXT("The central holo-projector tracks anomalous field signatures across all five sectors. What tactical intel do you require, Operative?")),
				{ ChoiceAberrationData, ChoiceDismiss });

			AddRow(FName(TEXT("Lyra_Affinities")),
				FText::FromString(TEXT("Analyst Lyra")),
				FText::FromString(TEXT("Bio-Stalkers in Sector 1 are purely biological physical threats. Sector 2 drones leak corrosive poison coolant. Outcast scrap-rigs use cryogenic charges. Excavators channel lightning, and the Sector 5 breach tears psionic fire into reality.")),
				{ ChoiceDismiss });

			FOnsetNPCDefinition LyraDef;
			LyraDef.DisplayName = FText::FromString(TEXT("Analyst Lyra"));
			LyraDef.TitleOrRole = FText::FromString(TEXT("Substrate Researcher (House Meridian)"));
			LyraDef.InteractionPrompt = FText::FromString(TEXT("Consult Intel"));
			LyraDef.DefaultDialogueRow = FName(TEXT("Lyra_Intro"));
			LyraDef.MilitaryRank = EOnsetRankId::SeniorAnalyst;
			LyraDef.HierarchyBranch = EOnsetHierarchyBranch::Substrate;
			LyraDef.Specialism = EOnsetSpecialism::SubstratePhysics;
			LyraDef.ProtocolStyle = EOnsetProtocolStyle::ResonantNod;

			FallbackNPCDefinitions.Add(FName(TEXT("AnalystLyra")), LyraDef);
			FallbackNPCDefinitions.Add(FName(TEXT("NPC_AnalystLyra")), LyraDef);
		}

		// =========================================================================
		// 6. OPERATIVE TAM (Sector 1 Bio-Dome Crossroads)
		// =========================================================================
		{
			FOnsetDialogueChoice AcceptQ3;
			AcceptQ3.ResponseText = FText::FromString(TEXT("I'll investigate the coolant lines. [Accept: Coolant Malfunction]"));
			AcceptQ3.RequiredQuestNotStarted = FName(TEXT("Q3_CoolantMalfunction"));
			AcceptQ3.GrantQuestOnSelect = FName(TEXT("Q3_CoolantMalfunction"));
			AcceptQ3.NextDialogueRow = FName(TEXT("Tam_Q3Accepted"));

			FOnsetDialogueChoice ChoiceDismiss;
			ChoiceDismiss.ResponseText = FText::FromString(TEXT("Hold your position, Tam."));
			ChoiceDismiss.NextDialogueRow = NAME_None;

			AddRow(FName(TEXT("Tam_Intro")),
				FText::FromString(TEXT("Operative Tam")),
				FText::FromString(TEXT("Good to see friendly armor out here. The bio-growth was triggered by a leak from the Sector 2 Coolant Reservoirs. Automated maintenance drones have gone haywire down there.")),
				{ AcceptQ3, ChoiceDismiss });

			AddRow(FName(TEXT("Tam_Q3Accepted")),
				FText::FromString(TEXT("Operative Tam")),
				FText::FromString(TEXT("Follow the conduit causeway to the reservoir. Cycle the Coolant Flow Console to drain the floodgate, dismantle the rogue drones, and link up with Recon Officer Mira.")),
				{ ChoiceDismiss });

			FOnsetNPCDefinition TamDef;
			TamDef.DisplayName = FText::FromString(TEXT("Operative Tam"));
			TamDef.TitleOrRole = FText::FromString(TEXT("Field Scout (House Nova)"));
			TamDef.InteractionPrompt = FText::FromString(TEXT("Status Report"));
			TamDef.DefaultDialogueRow = FName(TEXT("Tam_Intro"));
			TamDef.MilitaryRank = EOnsetRankId::SeniorScout;
			TamDef.HierarchyBranch = EOnsetHierarchyBranch::SpecialOps;
			TamDef.Specialism = EOnsetSpecialism::ReconSniper;
			TamDef.ProtocolStyle = EOnsetProtocolStyle::MilitarySalute;

			FallbackNPCDefinitions.Add(FName(TEXT("OperativeTam")), TamDef);
			FallbackNPCDefinitions.Add(FName(TEXT("NPC_OperativeTam")), TamDef);
		}

		// =========================================================================
		// 7. RECON OFFICER MIRA (Sector 3 Outcast Salvage Yard Post)
		// =========================================================================
		{
			FOnsetDialogueChoice AcceptQ4;
			AcceptQ4.ResponseText = FText::FromString(TEXT("We'll take that relay back. [Accept: Signal Interference]"));
			AcceptQ4.RequiredQuestNotStarted = FName(TEXT("Q4_SignalInterference"));
			AcceptQ4.GrantQuestOnSelect = FName(TEXT("Q4_SignalInterference"));
			AcceptQ4.NextDialogueRow = FName(TEXT("Mira_Q4Accepted"));

			FOnsetDialogueChoice ChoiceDismiss;
			ChoiceDismiss.ResponseText = FText::FromString(TEXT("Eyes sharp, Officer."));
			ChoiceDismiss.NextDialogueRow = NAME_None;

			AddRow(FName(TEXT("Mira_Intro")),
				FText::FromString(TEXT("Recon Officer Mira")),
				FText::FromString(TEXT("Keep your head down! Outcast scavengers hijacked the high-frequency comms relay dish. They've also barricaded the East Service Airlock back into the Academy.")),
				{ AcceptQ4, ChoiceDismiss });

			AddRow(FName(TEXT("Mira_Q4Accepted")),
				FText::FromString(TEXT("Recon Officer Mira")),
				FText::FromString(TEXT("Clear out the Outcast raiders, activate the East Airlock Override Console to reconnect transit to the Quad, then push north into the Geo-Thermal Excavation crater.")),
				{ ChoiceDismiss });

			FOnsetNPCDefinition MiraDef;
			MiraDef.DisplayName = FText::FromString(TEXT("Recon Officer Mira"));
			MiraDef.TitleOrRole = FText::FromString(TEXT("Reconnaissance Lead"));
			MiraDef.InteractionPrompt = FText::FromString(TEXT("Rendezvous"));
			MiraDef.DefaultDialogueRow = FName(TEXT("Mira_Intro"));
			MiraDef.MilitaryRank = EOnsetRankId::FieldOperative;
			MiraDef.HierarchyBranch = EOnsetHierarchyBranch::SpecialOps;
			MiraDef.Specialism = EOnsetSpecialism::SignalsIntel;
			MiraDef.ProtocolStyle = EOnsetProtocolStyle::MilitarySalute;

			FallbackNPCDefinitions.Add(FName(TEXT("ReconMira")), MiraDef);
			FallbackNPCDefinitions.Add(FName(TEXT("NPC_ReconMira")), MiraDef);
		}
	}
}

FString UOnsetDialogueLibrary::GetDialogueTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("DialogueDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Data/DT_Dialogues.DT_Dialogues");
	}
	return Path;
}

FString UOnsetDialogueLibrary::GetNPCTablePath()
{
	FString Path;
	GConfig->GetString(TEXT("Onset.Gameplay"), TEXT("NPCDataTable"), Path, GEngineIni);
	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/Data/DT_NPCs.DT_NPCs");
	}
	return Path;
}

UDataTable* UOnsetDialogueLibrary::GetDialogueTable()
{
	if (OnsetDialogueLibraryInternal::CachedDialogueTable)
	{
		return OnsetDialogueLibraryInternal::CachedDialogueTable.Get();
	}

	const FString TablePath = GetDialogueTablePath();
	if (UDataTable* Table = Cast<UDataTable>(StaticFindObject(UDataTable::StaticClass(), nullptr, *TablePath)))
	{
		OnsetDialogueLibraryInternal::CachedDialogueTable = TStrongObjectPtr<UDataTable>(Table);
		return Table;
	}

	const FString PkgName = FPackageName::ObjectPathToPackageName(TablePath);
	if (FPackageName::DoesPackageExist(PkgName))
	{
		if (UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath)))
		{
			OnsetDialogueLibraryInternal::CachedDialogueTable = TStrongObjectPtr<UDataTable>(Table);
			return Table;
		}
	}
	return nullptr;
}

UDataTable* UOnsetDialogueLibrary::GetNPCTable()
{
	if (OnsetDialogueLibraryInternal::CachedNPCTable)
	{
		return OnsetDialogueLibraryInternal::CachedNPCTable.Get();
	}

	const FString TablePath = GetNPCTablePath();
	if (UDataTable* Table = Cast<UDataTable>(StaticFindObject(UDataTable::StaticClass(), nullptr, *TablePath)))
	{
		OnsetDialogueLibraryInternal::CachedNPCTable = TStrongObjectPtr<UDataTable>(Table);
		return Table;
	}

	const FString PkgName = FPackageName::ObjectPathToPackageName(TablePath);
	if (FPackageName::DoesPackageExist(PkgName))
	{
		if (UDataTable* Table = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePath)))
		{
			OnsetDialogueLibraryInternal::CachedNPCTable = TStrongObjectPtr<UDataTable>(Table);
			return Table;
		}
	}
	return nullptr;
}

const FOnsetDialogueRow* UOnsetDialogueLibrary::GetDialogueRow(FName RowName)
{
	if (RowName.IsNone())
	{
		return nullptr;
	}

	OnsetDialogueLibraryInternal::EnsureDefaultFallbacks();

	if (UDataTable* Table = GetDialogueTable())
	{
		if (const FOnsetDialogueRow* Row = Table->FindRow<FOnsetDialogueRow>(RowName, nullptr, /*bWarnIfRowMissing=*/false))
		{
			return Row;
		}
	}

	return OnsetDialogueLibraryInternal::FallbackDialogueRows.Find(RowName);
}

const FOnsetNPCDefinition* UOnsetDialogueLibrary::GetNPCDefinition(FName RowName)
{
	if (RowName.IsNone())
	{
		return nullptr;
	}

	OnsetDialogueLibraryInternal::EnsureDefaultFallbacks();

	if (UDataTable* Table = GetNPCTable())
	{
		if (const FOnsetNPCDefinition* Def = Table->FindRow<FOnsetNPCDefinition>(RowName, nullptr, /*bWarnIfRowMissing=*/false))
		{
			return Def;
		}
	}

	return OnsetDialogueLibraryInternal::FallbackNPCDefinitions.Find(RowName);
}

void UOnsetDialogueLibrary::RegisterFallbackDialogueRow(FName RowName, const FOnsetDialogueRow& Row)
{
	OnsetDialogueLibraryInternal::EnsureDefaultFallbacks();
	if (!RowName.IsNone())
	{
		OnsetDialogueLibraryInternal::FallbackDialogueRows.Add(RowName, Row);
	}
}

void UOnsetDialogueLibrary::RegisterFallbackNPCDefinition(FName RowName, const FOnsetNPCDefinition& Definition)
{
	OnsetDialogueLibraryInternal::EnsureDefaultFallbacks();
	if (!RowName.IsNone())
	{
		OnsetDialogueLibraryInternal::FallbackNPCDefinitions.Add(RowName, Definition);
	}
}

bool UOnsetDialogueLibrary::IsChoiceAvailable(const FOnsetDialogueChoice& Choice, const UOnsetQuestComponent* QuestComponent)
{
	if (!Choice.RequiredQuestNotStarted.IsNone())
	{
		if (QuestComponent && QuestComponent->HasQuest(Choice.RequiredQuestNotStarted))
		{
			return false;
		}
	}

	if (!Choice.RequiredQuestActive.IsNone())
	{
		if (!QuestComponent || !QuestComponent->IsQuestActive(Choice.RequiredQuestActive))
		{
			return false;
		}
	}

	if (!Choice.RequiredQuestComplete.IsNone())
	{
		if (!QuestComponent || !QuestComponent->IsQuestComplete(Choice.RequiredQuestComplete))
		{
			return false;
		}
	}

	return true;
}

FName UOnsetDialogueLibrary::ResolveEntryDialogueRow(const FOnsetNPCDefinition& NPCDef, const UOnsetQuestComponent* QuestComponent)
{
	if (QuestComponent)
	{
		for (const FOnsetNPCQuestDialogueOverride& Override : NPCDef.QuestDialogueOverrides)
		{
			if (Override.QuestRowName.IsNone())
			{
				continue;
			}

			if (QuestComponent->IsQuestComplete(Override.QuestRowName))
			{
				if (!Override.CompletedDialogueRow.IsNone())
				{
					return Override.CompletedDialogueRow;
				}
			}
			else if (QuestComponent->IsQuestActive(Override.QuestRowName))
			{
				if (!Override.InProgressDialogueRow.IsNone())
				{
					return Override.InProgressDialogueRow;
				}
			}
			else
			{
				if (!Override.NotStartedDialogueRow.IsNone())
				{
					return Override.NotStartedDialogueRow;
				}
			}
		}
	}

	return NPCDef.DefaultDialogueRow;
}
