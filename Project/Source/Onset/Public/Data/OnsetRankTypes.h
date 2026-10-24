// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OnsetRankTypes.generated.h"

/**
 * Branches of the Confederate service and governance hierarchy.
 */
UENUM(BlueprintType)
enum class EOnsetHierarchyBranch : uint8
{
	Military		UMETA(DisplayName = "Confederate Military (Fleet & Ground Forces)"),
	Governance		UMETA(DisplayName = "Confederate Governance (Senate, Governors, Council)"),
	Substrate		UMETA(DisplayName = "Substrate & Science Directorate (Meridian Institute)"),
	Logistics		UMETA(DisplayName = "Logistics & Industry (House Aegis Quartermasters)"),
	SpecialOps		UMETA(DisplayName = "Special Operations (House Nova Recon & Intelligence)")
};

/**
 * Functional specialism within a branch.
 */
UENUM(BlueprintType)
enum class EOnsetSpecialism : uint8
{
	None					UMETA(DisplayName = "None / General"),
	// Military
	ShockAssault			UMETA(DisplayName = "Vanguard Shock Assault"),
	BastionDefense			UMETA(DisplayName = "Aegis Bastion Defense"),
	VoidInfantry			UMETA(DisplayName = "Void Marine Infantry"),
	// Special Ops
	ReconSniper				UMETA(DisplayName = "Nova Recon Sniper"),
	SignalsIntel			UMETA(DisplayName = "Signals & Infiltration Intelligence"),
	// Substrate & Science
	SubstratePhysics		UMETA(DisplayName = "Substrate Resonance & Topology"),
	PsionicCalibration		UMETA(DisplayName = "Psionic Field Calibration"),
	Xenobiology				UMETA(DisplayName = "Planetary Xenobiology"),
	// Logistics & Industry
	OrdnanceLogistics		UMETA(DisplayName = "Armory & Munitions Supply"),
	VoidcraftEngineering	UMETA(DisplayName = "Starship & Conduit Engineering"),
	// Governance
	CivilAdministration		UMETA(DisplayName = "Confederate Civil Law & Administration"),
	DiplomaticCorps			UMETA(DisplayName = "Senate Diplomatic Corps")
};

/**
 * Style of physical or ceremonial protocol used during interactions.
 */
UENUM(BlueprintType)
enum class EOnsetProtocolStyle : uint8
{
	MilitarySalute		UMETA(DisplayName = "Military Salute (Fist to Breastplate)"),
	DiplomaticBow		UMETA(DisplayName = "Diplomatic Bow (Hand Over Heart)"),
	ResonantNod			UMETA(DisplayName = "Resonant Nod (Meridian Palm Outward)"),
	CivilianGreeting	UMETA(DisplayName = "Civilian Greeting (Polite Nod)")
};

/**
 * Granular Confederate ranks spanning Precedence Tiers 0 to 12.
 */
UENUM(BlueprintType)
enum class EOnsetRankId : uint8
{
	// Tier 0: Unaffiliated / Recruits
	Civilian				UMETA(DisplayName = "Civilian"),
	UnenlistedRecruit		UMETA(DisplayName = "Unenlisted Recruit"),

	// Tier 1: Inductees & Apprentices
	CadetInductee			UMETA(DisplayName = "Cadet Inductee"),
	CivicApprentice			UMETA(DisplayName = "Civic Apprentice"),
	ResearchApprentice		UMETA(DisplayName = "Research Apprentice"),

	// Tier 2: Junior Operatives & Delegates
	FieldOperative			UMETA(DisplayName = "Field Operative"),
	CitizenDelegate			UMETA(DisplayName = "Citizen Delegate"),
	JuniorAnalyst			UMETA(DisplayName = "Junior Analyst"),

	// Tier 3: Senior Scouts & Liaisons
	SeniorScout				UMETA(DisplayName = "Senior Scout"),
	CivilLiaison			UMETA(DisplayName = "Civil Liaison"),
	TechnicianLead			UMETA(DisplayName = "Technician Lead"),

	// Tier 4: NCOs & Overseers
	MasterSpecialist		UMETA(DisplayName = "Master Specialist"),
	TribuneOverseer			UMETA(DisplayName = "Tribune Overseer"),
	SubstrateEngineer		UMETA(DisplayName = "Substrate Engineer"),

	// Tier 5: Junior Commissioned Officers & Bureau Chiefs
	LogisticsOfficer		UMETA(DisplayName = "Logistics Officer"),
	ConsularAttache			UMETA(DisplayName = "Consular Attache"),
	ResearchSpecialist		UMETA(DisplayName = "Research Specialist"),

	// Tier 6: Officers, Directors & Senior Analysts
	SeniorOfficer			UMETA(DisplayName = "Senior Officer (Captain)"),
	MunicipalDirector		UMETA(DisplayName = "Municipal Director"),
	SeniorAnalyst			UMETA(DisplayName = "Senior Analyst"),

	// Tier 7: Staff Commanders & Magistrates
	StaffCommander			UMETA(DisplayName = "Staff Commander (Major)"),
	DistrictMagistrate		UMETA(DisplayName = "District Magistrate"),
	ChiefScienceOfficer		UMETA(DisplayName = "Chief Science Officer"),

	// Tier 8: Field Commanders & Planetary Governors
	LegatusCommander		UMETA(DisplayName = "Legatus Commander"),
	PlanetaryGovernor		UMETA(DisplayName = "Planetary Governor"),
	LeadArchitect			UMETA(DisplayName = "Lead Substrate Architect"),

	// Tier 9: Fleet Commanders & High Magistrates
	FleetCommander			UMETA(DisplayName = "Fleet Commander"),
	SystemPrefect			UMETA(DisplayName = "System Prefect"),
	SeniorChiefResearcher	UMETA(DisplayName = "Senior Chief Researcher"),

	// Tier 10: Marshals, Planetary Chancellors & Directorate Chairs
	Marshal					UMETA(DisplayName = "Marshal"),
	PlanetaryChancellor		UMETA(DisplayName = "Planetary Chancellor"),
	DirectorateProvost		UMETA(DisplayName = "Directorate Provost"),

	// Tier 11: Sector Senators & High Admirals
	HighAdmiral				UMETA(DisplayName = "High Admiral"),
	SectorSenator			UMETA(DisplayName = "Sector Senator"),
	DirectorGeneral			UMETA(DisplayName = "Director-General of Science"),

	// Tier 12: High Command & Supreme Council
	GrandMarshal			UMETA(DisplayName = "Grand Marshal"),
	SupremeChancellor		UMETA(DisplayName = "Supreme Chancellor / High Councillor"),
	ArchSavant				UMETA(DisplayName = "Arch-Savant")
};

/**
 * Complete metadata for a single rank in the Confederate system.
 */
USTRUCT(BlueprintType)
struct ONSET_API FOnsetRankInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	EOnsetRankId RankId = EOnsetRankId::Civilian;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	EOnsetHierarchyBranch DefaultBranch = EOnsetHierarchyBranch::Governance;

	/** Precedence tier from 0 (lowest) to 12 (highest). Subordinate salutes superior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	uint8 PrecedenceTier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	FText DisplayTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	FText Honorific;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	EOnsetProtocolStyle PreferredStyle = EOnsetProtocolStyle::CivilianGreeting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	FText DefaultDismissal;
};

/**
 * Result of a protocol comparison between two individuals.
 */
UENUM(BlueprintType)
enum class EOnsetProtocolPrecedence : uint8
{
	InitiatorIsSubordinate		UMETA(DisplayName = "Initiator is Subordinate (Salutes First)"),
	InitiatorIsSuperior			UMETA(DisplayName = "Initiator is Superior (NPC Salutes First)"),
	Peers						UMETA(DisplayName = "Peers (Mutual Protocol)"),
	CivilianEtiquette			UMETA(DisplayName = "Civilian Etiquette (Standard Greeting)")
};

/**
 * Helper library for rank metadata, precedence resolution, and ceremonial greetings.
 */
UCLASS()
class ONSET_API UOnsetRankLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the full rank definition including precedence tier, honorific, and dismissal phrase. */
	UFUNCTION(BlueprintPure, Category = "Onset|Ranks")
	static FOnsetRankInfo GetRankInfo(EOnsetRankId RankId);

	/** Compares precedence between Actor A and Actor B. */
	UFUNCTION(BlueprintPure, Category = "Onset|Ranks")
	static EOnsetProtocolPrecedence ComparePrecedence(EOnsetRankId InitiatorRank, EOnsetRankId TargetRank);

	/** Generates the ceremonial protocol notification banner (e.g. "[Protocol] Cadet salutes Commander Vane"). */
	UFUNCTION(BlueprintPure, Category = "Onset|Ranks")
	static FText BuildProtocolBannerText(const FText& InitiatorName, EOnsetRankId InitiatorRank, const FText& TargetName, EOnsetRankId TargetRank);

	/** Generates the dismissal / acknowledgement spoken line by the superior. */
	UFUNCTION(BlueprintPure, Category = "Onset|Ranks")
	static FText GetAcknowledgementLine(EOnsetRankId SuperiorRank, EOnsetRankId SubordinateRank);
};
