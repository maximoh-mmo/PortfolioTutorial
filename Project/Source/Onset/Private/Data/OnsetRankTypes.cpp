// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/OnsetRankTypes.h"

FOnsetRankInfo UOnsetRankLibrary::GetRankInfo(EOnsetRankId RankId)
{
	FOnsetRankInfo Info;
	Info.RankId = RankId;

	switch (RankId)
	{
		// --- Tier 0 ---
		case EOnsetRankId::Civilian:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 0;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Civilian", "Citizen");
			Info.Honorific = NSLOCTEXT("OnsetRank", "CitizenHon", "Citizen");
			Info.PreferredStyle = EOnsetProtocolStyle::CivilianGreeting;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "CivDismiss", "Good day, Citizen.");
			break;

		case EOnsetRankId::UnenlistedRecruit:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 0;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Recruit", "Recruit");
			Info.Honorific = NSLOCTEXT("OnsetRank", "RecruitHon", "Recruit");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "RecruitDismiss", "Back to formation.");
			break;

		// --- Tier 1 ---
		case EOnsetRankId::CadetInductee:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 1;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Cadet", "Cadet Inductee");
			Info.Honorific = NSLOCTEXT("OnsetRank", "CadetHon", "Cadet");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "CadetDismiss", "At ease, Cadet.");
			break;

		case EOnsetRankId::CivicApprentice:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 1;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "CivAppr", "Civic Apprentice");
			Info.Honorific = NSLOCTEXT("OnsetRank", "CivApprHon", "Apprentice");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "CivApprDismiss", "Continue your duties, Apprentice.");
			break;

		case EOnsetRankId::ResearchApprentice:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 1;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "ResAppr", "Research Apprentice");
			Info.Honorific = NSLOCTEXT("OnsetRank", "ResApprHon", "Apprentice");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "ResApprDismiss", "Resume telemetry calibration.");
			break;

		// --- Tier 2 ---
		case EOnsetRankId::FieldOperative:
			Info.DefaultBranch = EOnsetHierarchyBranch::SpecialOps;
			Info.PrecedenceTier = 2;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Operative", "Field Operative");
			Info.Honorific = NSLOCTEXT("OnsetRank", "OperativeHon", "Operative");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "OperativeDismiss", "Eyes sharp, Operative.");
			break;

		case EOnsetRankId::CitizenDelegate:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 2;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Delegate", "Citizen Delegate");
			Info.Honorific = NSLOCTEXT("OnsetRank", "DelegateHon", "Delegate");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "DelegateDismiss", "Thank you for your representation, Delegate.");
			break;

		case EOnsetRankId::JuniorAnalyst:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 2;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "JrAnalyst", "Junior Analyst");
			Info.Honorific = NSLOCTEXT("OnsetRank", "JrAnalystHon", "Analyst");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "JrAnalystDismiss", "Compile the telemetry report.");
			break;

		// --- Tier 3 ---
		case EOnsetRankId::SeniorScout:
			Info.DefaultBranch = EOnsetHierarchyBranch::SpecialOps;
			Info.PrecedenceTier = 3;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "SrScout", "Senior Scout");
			Info.Honorific = NSLOCTEXT("OnsetRank", "SrScoutHon", "Scout");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "SrScoutDismiss", "Maintain your perimeter patrol, Scout.");
			break;

		case EOnsetRankId::CivilLiaison:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 3;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Liaison", "Civil Liaison");
			Info.Honorific = NSLOCTEXT("OnsetRank", "LiaisonHon", "Liaison");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "LiaisonDismiss", "Communicate with the sector council.");
			break;

		case EOnsetRankId::TechnicianLead:
			Info.DefaultBranch = EOnsetHierarchyBranch::Logistics;
			Info.PrecedenceTier = 3;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "TechLead", "Technician Lead");
			Info.Honorific = NSLOCTEXT("OnsetRank", "TechLeadHon", "Lead");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "TechLeadDismiss", "Ensure conduit pressure holds.");
			break;

		// --- Tier 4 ---
		case EOnsetRankId::MasterSpecialist:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 4;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "MstrSpec", "Master Specialist");
			Info.Honorific = NSLOCTEXT("OnsetRank", "MstrSpecHon", "Specialist");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "MstrSpecDismiss", "Stand easy, Specialist.");
			break;

		case EOnsetRankId::TribuneOverseer:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 4;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Tribune", "Tribune Overseer");
			Info.Honorific = NSLOCTEXT("OnsetRank", "TribuneHon", "Tribune");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "TribuneDismiss", "Maintain order in your district.");
			break;

		case EOnsetRankId::SubstrateEngineer:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 4;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "SubEng", "Substrate Engineer");
			Info.Honorific = NSLOCTEXT("OnsetRank", "SubEngHon", "Engineer");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "SubEngDismiss", "Calibrate the containment field.");
			break;

		// --- Tier 5 ---
		case EOnsetRankId::LogisticsOfficer:
			Info.DefaultBranch = EOnsetHierarchyBranch::Logistics;
			Info.PrecedenceTier = 5;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "LogOff", "Logistics Officer");
			Info.Honorific = NSLOCTEXT("OnsetRank", "LogOffHon", "Quartermaster");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "LogOffDismiss", "Keep requisition manifests verified.");
			break;

		case EOnsetRankId::ConsularAttache:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 5;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Attache", "Consular Attache");
			Info.Honorific = NSLOCTEXT("OnsetRank", "AttacheHon", "Attache");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "AttacheDismiss", "Deliver the dispatch to the embassy.");
			break;

		case EOnsetRankId::ResearchSpecialist:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 5;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "ResSpec", "Research Specialist");
			Info.Honorific = NSLOCTEXT("OnsetRank", "ResSpecHon", "Specialist");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "ResSpecDismiss", "Document anomalous flux patterns.");
			break;

		// --- Tier 6 ---
		case EOnsetRankId::SeniorOfficer:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 6;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "SrOff", "Senior Officer (Captain)");
			Info.Honorific = NSLOCTEXT("OnsetRank", "SrOffHon", "Captain");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "SrOffDismiss", "At ease, Officer.");
			break;

		case EOnsetRankId::MunicipalDirector:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 6;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "MunDir", "Municipal Director");
			Info.Honorific = NSLOCTEXT("OnsetRank", "MunDirHon", "Director");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "MunDirDismiss", "The Directorate acknowledges your service.");
			break;

		case EOnsetRankId::SeniorAnalyst:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 6;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "SrAnalyst", "Senior Analyst");
			Info.Honorific = NSLOCTEXT("OnsetRank", "SrAnalystHon", "Analyst");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "SrAnalystDismiss", "Our research parameters remain focused.");
			break;

		// --- Tier 7 ---
		case EOnsetRankId::StaffCommander:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 7;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "StaffCmdr", "Staff Commander (Major)");
			Info.Honorific = NSLOCTEXT("OnsetRank", "StaffCmdrHon", "Commander");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "StaffCmdrDismiss", "As you were, Commander.");
			break;

		case EOnsetRankId::DistrictMagistrate:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 7;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Magistrate", "District Magistrate");
			Info.Honorific = NSLOCTEXT("OnsetRank", "MagistrateHon", "Magistrate");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "MagistrateDismiss", "Confederate law prevails.");
			break;

		case EOnsetRankId::ChiefScienceOfficer:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 7;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "CSO", "Chief Science Officer");
			Info.Honorific = NSLOCTEXT("OnsetRank", "CSOHon", "Chief");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "CSODismiss", "Forward your conclusions to the Provost.");
			break;

		// --- Tier 8 ---
		case EOnsetRankId::LegatusCommander:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 8;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "LegatusCmdr", "Legatus Commander");
			Info.Honorific = NSLOCTEXT("OnsetRank", "LegatusCmdrHon", "Commander");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "LegatusCmdrDismiss", "At ease. Carry on the mission.");
			break;

		case EOnsetRankId::PlanetaryGovernor:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 8;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "PlGov", "Planetary Governor");
			Info.Honorific = NSLOCTEXT("OnsetRank", "PlGovHon", "Governor");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "PlGovDismiss", "The Governor extends greeting. You have the floor.");
			break;

		case EOnsetRankId::LeadArchitect:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 8;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "LeadArch", "Lead Substrate Architect");
			Info.Honorific = NSLOCTEXT("OnsetRank", "LeadArchHon", "Architect");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "LeadArchDismiss", "The resonance lattice is aligned.");
			break;

		// --- Tier 9 ---
		case EOnsetRankId::FleetCommander:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 9;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "FleetCmdr", "Fleet Commander");
			Info.Honorific = NSLOCTEXT("OnsetRank", "FleetCmdrHon", "Fleet Commander");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "FleetCmdrDismiss", "Fleet Command stands vigilant. Report.");
			break;

		case EOnsetRankId::SystemPrefect:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 9;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "SysPrefect", "System Prefect");
			Info.Honorific = NSLOCTEXT("OnsetRank", "SysPrefectHon", "Prefect");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "SysPrefectDismiss", "Order throughout the star system is maintained.");
			break;

		case EOnsetRankId::SeniorChiefResearcher:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 9;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "SrChiefRes", "Senior Chief Researcher");
			Info.Honorific = NSLOCTEXT("OnsetRank", "SrChiefResHon", "Senior Chief");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "SrChiefResDismiss", "Resonant discovery guides our houses.");
			break;

		// --- Tier 10 ---
		case EOnsetRankId::Marshal:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 10;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Marshal", "Marshal");
			Info.Honorific = NSLOCTEXT("OnsetRank", "MarshalHon", "Marshal");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "MarshalDismiss", "The Vanguard front line salutes you.");
			break;

		case EOnsetRankId::PlanetaryChancellor:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 10;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Chancellor", "Planetary Chancellor");
			Info.Honorific = NSLOCTEXT("OnsetRank", "ChancellorHon", "Chancellor");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "ChancellorDismiss", "The Planetary Chancellery receives you with honors.");
			break;

		case EOnsetRankId::DirectorateProvost:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 10;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Provost", "Directorate Provost");
			Info.Honorific = NSLOCTEXT("OnsetRank", "ProvostHon", "Provost");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "ProvostDismiss", "Serve with honor and clarity, Operative.");
			break;

		// --- Tier 11 ---
		case EOnsetRankId::HighAdmiral:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 11;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "HighAdmiral", "High Admiral");
			Info.Honorific = NSLOCTEXT("OnsetRank", "HighAdmiralHon", "Admiral");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "HighAdmiralDismiss", "Fleet armada is at ready status.");
			break;

		case EOnsetRankId::SectorSenator:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 11;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "Senator", "Sector Senator");
			Info.Honorific = NSLOCTEXT("OnsetRank", "SenatorHon", "Senator");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "SenatorDismiss", "The Confederate Senate commends your dedication.");
			break;

		case EOnsetRankId::DirectorGeneral:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 11;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "DirGen", "Director-General of Science");
			Info.Honorific = NSLOCTEXT("OnsetRank", "DirGenHon", "Director-General");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "DirGenDismiss", "Scientific truth is the foundation of the Confederate.");
			break;

		// --- Tier 12 ---
		case EOnsetRankId::GrandMarshal:
			Info.DefaultBranch = EOnsetHierarchyBranch::Military;
			Info.PrecedenceTier = 12;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "GrandMarshal", "Grand Marshal / Archon");
			Info.Honorific = NSLOCTEXT("OnsetRank", "GrandMarshalHon", "Grand Marshal");
			Info.PreferredStyle = EOnsetProtocolStyle::MilitarySalute;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "GrandMarshalDismiss", "The Confederate Military Bows to High Command.");
			break;

		case EOnsetRankId::SupremeChancellor:
			Info.DefaultBranch = EOnsetHierarchyBranch::Governance;
			Info.PrecedenceTier = 12;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "SupremeChancellor", "Supreme Chancellor (High Council)");
			Info.Honorific = NSLOCTEXT("OnsetRank", "SupremeChancellorHon", "Your Eminence");
			Info.PreferredStyle = EOnsetProtocolStyle::DiplomaticBow;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "SupremeChancellorDismiss", "The Confederate Council grants you audience.");
			break;

		case EOnsetRankId::ArchSavant:
			Info.DefaultBranch = EOnsetHierarchyBranch::Substrate;
			Info.PrecedenceTier = 12;
			Info.DisplayTitle = NSLOCTEXT("OnsetRank", "ArchSavant", "Arch-Savant");
			Info.Honorific = NSLOCTEXT("OnsetRank", "ArchSavantHon", "Arch-Savant");
			Info.PreferredStyle = EOnsetProtocolStyle::ResonantNod;
			Info.DefaultDismissal = NSLOCTEXT("OnsetRank", "ArchSavantDismiss", "The deep substrate answers only to mastery.");
			break;
	}

	return Info;
}

EOnsetProtocolPrecedence UOnsetRankLibrary::ComparePrecedence(EOnsetRankId InitiatorRank, EOnsetRankId TargetRank)
{
	const FOnsetRankInfo InitiatorInfo = GetRankInfo(InitiatorRank);
	const FOnsetRankInfo TargetInfo = GetRankInfo(TargetRank);

	if (InitiatorInfo.PrecedenceTier == 0 && TargetInfo.PrecedenceTier == 0)
	{
		return EOnsetProtocolPrecedence::CivilianEtiquette;
	}

	if (InitiatorInfo.PrecedenceTier < TargetInfo.PrecedenceTier)
	{
		return EOnsetProtocolPrecedence::InitiatorIsSubordinate;
	}
	else if (InitiatorInfo.PrecedenceTier > TargetInfo.PrecedenceTier)
	{
		return EOnsetProtocolPrecedence::InitiatorIsSuperior;
	}

	return EOnsetProtocolPrecedence::Peers;
}

FText UOnsetRankLibrary::BuildProtocolBannerText(const FText& InitiatorName, EOnsetRankId InitiatorRank, const FText& TargetName, EOnsetRankId TargetRank)
{
	const FOnsetRankInfo InitInfo = GetRankInfo(InitiatorRank);
	const FOnsetRankInfo TargetInfo = GetRankInfo(TargetRank);
	const EOnsetProtocolPrecedence Result = ComparePrecedence(InitiatorRank, TargetRank);

	switch (Result)
	{
		case EOnsetProtocolPrecedence::InitiatorIsSubordinate:
			return FText::Format(
				NSLOCTEXT("OnsetProtocol", "BannerSubordinate", "[Protocol] {0} ({1}) salutes {2} ({3})"),
				InitiatorName, InitInfo.DisplayTitle, TargetName, TargetInfo.DisplayTitle);

		case EOnsetProtocolPrecedence::InitiatorIsSuperior:
			return FText::Format(
				NSLOCTEXT("OnsetProtocol", "BannerSuperior", "[Protocol] {0} ({1}) snaps to attention for {2} ({3})"),
				TargetName, TargetInfo.DisplayTitle, InitiatorName, InitInfo.DisplayTitle);

		case EOnsetProtocolPrecedence::Peers:
			return FText::Format(
				NSLOCTEXT("OnsetProtocol", "BannerPeers", "[Protocol] Mutual ceremonial acknowledgement between {0} and {1} ({2})"),
				InitiatorName, TargetName, InitInfo.DisplayTitle);

		case EOnsetProtocolPrecedence::CivilianEtiquette:
		default:
			return FText::Format(
				NSLOCTEXT("OnsetProtocol", "BannerCiv", "[Etiquette] Civic exchange with {0}"),
				TargetName);
	}
}

FText UOnsetRankLibrary::GetAcknowledgementLine(EOnsetRankId SuperiorRank, EOnsetRankId SubordinateRank)
{
	const FOnsetRankInfo SupInfo = GetRankInfo(SuperiorRank);
	const FOnsetRankInfo SubInfo = GetRankInfo(SubordinateRank);

	if (!SupInfo.DefaultDismissal.IsEmpty())
	{
		return SupInfo.DefaultDismissal;
	}

	return FText::Format(NSLOCTEXT("OnsetProtocol", "DefaultAck", "At ease, {0}."), SubInfo.Honorific);
}
