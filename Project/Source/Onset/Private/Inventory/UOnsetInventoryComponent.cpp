// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/UOnsetInventoryComponent.h"

#include "Combat/OnsetEquipmentLibrary.h"
#include "Data/OnsetItemLibrary.h"
#include "Dom/JsonObject.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UOnsetInventoryComponent::UOnsetInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Replicate Items + EquippedEntries whenever this component is registered on a replicated actor.
	SetIsReplicatedByDefault(true);
}

// --- Bag ---

void UOnsetInventoryComponent::AddItem(EOnsetItemCategory Category, FName RowName, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	// Check if this item already exists in the inventory.
	bool bExists = Items.ContainsByPredicate([Category, RowName](const FOnsetInventoryEntry& Entry)
	{
		return Entry.Category == Category && Entry.RowName == RowName;
	});
	
	// If it's a new item, verify we haven't hit the max distinct slots limit.
	if (!bExists && Items.Num() >= MaxInventorySlots)
	{
		// Optionally broadcast or log that the inventory is full.
		return;
	}
	
	UOnsetItemLibrary::AddStacked(Items, Category, RowName, Count);
	if (Count > 0)
	{
		OnItemAdded.Broadcast(Category, RowName, Count);
		OnInventoryChanged.Broadcast();
	}
}

void UOnsetInventoryComponent::AddItems(const TArray<FOnsetInventoryEntry>& Entries)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	for (const FOnsetInventoryEntry& Entry : Entries)
	{
		if (Entry.Count <= 0)
		{
			continue;
		}
		UOnsetItemLibrary::AddStacked(Items, Entry.Category, Entry.RowName, Entry.Count);
		OnItemAdded.Broadcast(Entry.Category, Entry.RowName, Entry.Count);
	}
	if (Entries.Num() > 0)
	{
		OnInventoryChanged.Broadcast();
	}
}

bool UOnsetInventoryComponent::RemoveItem(EOnsetItemCategory Category, FName RowName, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Count <= 0)
	{
		return false;
	}

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		FOnsetInventoryEntry& Entry = Items[Index];
		if (Entry.Category != Category || Entry.RowName != RowName)
		{
			continue;
		}
		const int32 Removed = FMath::Min(Entry.Count, Count);
		Entry.Count -= Removed;
		Count -= Removed;
		if (Entry.Count <= 0)
		{
			Items.RemoveAt(Index);
		}
		if (Count <= 0)
		{
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	return false;
}

void UOnsetInventoryComponent::RemoveAllOfItem(EOnsetItemCategory Category, FName RowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const int32 Removed = Items.RemoveAll([Category, RowName](const FOnsetInventoryEntry& Entry)
	{
		return Entry.Category == Category && Entry.RowName == RowName;
	});
	if (Removed > 0)
	{
		OnInventoryChanged.Broadcast();
	}
}

void UOnsetInventoryComponent::ClearItems()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (Items.Num() == 0)
	{
		return;
	}
	Items.Reset();
	OnInventoryChanged.Broadcast();
}

void UOnsetInventoryComponent::SetItems(const TArray<FOnsetInventoryEntry>& Entries)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Items = Entries;
	OnInventoryChanged.Broadcast();
}

bool UOnsetInventoryComponent::HasItem(EOnsetItemCategory Category, FName RowName) const
{
	return Items.ContainsByPredicate([Category, RowName](const FOnsetInventoryEntry& Entry)
	{
		return Entry.Category == Category && Entry.RowName == RowName;
	});
}

int32 UOnsetInventoryComponent::GetItemCount(EOnsetItemCategory Category, FName RowName) const
{
	int32 Total = 0;
	for (const FOnsetInventoryEntry& Entry : Items)
	{
		if (Entry.Category == Category && Entry.RowName == RowName)
		{
			Total += Entry.Count;
		}
	}
	return Total;
}

int32 UOnsetInventoryComponent::GetTotalItemCount() const
{
	int32 Total = 0;
	for (const FOnsetInventoryEntry& Entry : Items)
	{
		Total += Entry.Count;
	}
	return Total;
}

// --- Equipped ---

void UOnsetInventoryComponent::EquipItem(EOnsetEquipmentSlot Slot, FName RowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const FOnsetEquipmentDefinition* Def = UOnsetEquipmentLibrary::GetDefinition(RowName);
	const FOnsetEquipmentInstanceData Rolled = UOnsetEquipmentLibrary::RollNewEquipmentInstance(Def ? Def->Rarity : EOnsetItemRarity::Common);
	EquipItemWithInstance(Slot, RowName, Rolled);
}

void UOnsetInventoryComponent::EquipItemWithInstance(EOnsetEquipmentSlot Slot, FName RowName, const FOnsetEquipmentInstanceData& InstanceData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	EquippedEntries.RemoveAll([Slot](const FOnsetEquippedEntry& Entry) { return Entry.Slot == Slot; });
	if (!RowName.IsNone())
	{
		FOnsetEquippedEntry NewEntry;
		NewEntry.Slot = Slot;
		NewEntry.RowName = RowName;
		NewEntry.InstanceData = InstanceData;
		EquippedEntries.Add(NewEntry);
	}
	OnInventoryChanged.Broadcast();
}

void UOnsetInventoryComponent::UnequipSlot(EOnsetEquipmentSlot Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const int32 Index = EquippedEntries.IndexOfByPredicate([Slot](const FOnsetEquippedEntry& Entry) { return Entry.Slot == Slot; });
	if (Index == INDEX_NONE)
	{
		return;
	}

	const FOnsetEquippedEntry Removed = EquippedEntries[Index];
	EquippedEntries.RemoveAt(Index);

	if (!Removed.RowName.IsNone())
	{
		UOnsetItemLibrary::AddEquipment(Items, Removed.RowName, Removed.InstanceData);
	}
	OnInventoryChanged.Broadcast();
}

bool UOnsetInventoryComponent::EquipFromInventory(FName RowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || RowName.IsNone())
	{
		return false;
	}

	const int32 BagIndex = Items.IndexOfByPredicate([RowName](const FOnsetInventoryEntry& Entry)
	{
		return Entry.Category == EOnsetItemCategory::Equipment && Entry.RowName == RowName;
	});

	if (BagIndex == INDEX_NONE)
	{
		return false;
	}

	return EquipFromInventorySlot(BagIndex);
}

bool UOnsetInventoryComponent::EquipFromInventorySlot(int32 BagIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Items.IsValidIndex(BagIndex))
	{
		return false;
	}

	const FOnsetInventoryEntry BagEntry = Items[BagIndex];
	if (BagEntry.Category != EOnsetItemCategory::Equipment || BagEntry.RowName.IsNone())
	{
		return false;
	}

	const FOnsetEquipmentDefinition* Definition = UOnsetEquipmentLibrary::GetDefinition(BagEntry.RowName);
	if (!Definition)
	{
		return false;
	}

	// Unequip current item in that slot first (returns to bag with its instance data)
	UnequipSlot(Definition->Slot);

	// Remove from bag
	Items[BagIndex].Count -= 1;
	if (Items[BagIndex].Count <= 0)
	{
		Items.RemoveAt(BagIndex);
	}

	// Equip with original instance data
	EquipItemWithInstance(Definition->Slot, BagEntry.RowName, BagEntry.InstanceData);
	return true;
}

void UOnsetInventoryComponent::AddEquipmentInstance(FName RowName, const FOnsetEquipmentInstanceData& InstanceData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || RowName.IsNone())
	{
		return;
	}
	UOnsetItemLibrary::AddEquipment(Items, RowName, InstanceData);
	OnItemAdded.Broadcast(EOnsetItemCategory::Equipment, RowName, 1);
	OnInventoryChanged.Broadcast();
}

bool UOnsetInventoryComponent::GetItemAtSlot(int32 SlotIndex, FOnsetInventoryEntry& OutEntry) const
{
	if (!Items.IsValidIndex(SlotIndex))
	{
		return false;
	}
	OutEntry = Items[SlotIndex];
	return true;
}

const FOnsetEquippedEntry* UOnsetInventoryComponent::GetEquippedEntry(EOnsetEquipmentSlot Slot) const
{
	return EquippedEntries.FindByPredicate([Slot](const FOnsetEquippedEntry& Candidate) { return Candidate.Slot == Slot; });
}

bool UOnsetInventoryComponent::GetEquippedInstanceData(EOnsetEquipmentSlot Slot, FOnsetEquipmentInstanceData& OutData) const
{
	const FOnsetEquippedEntry* Entry = GetEquippedEntry(Slot);
	if (!Entry || Entry->RowName.IsNone())
	{
		return false;
	}
	OutData = Entry->InstanceData;
	return true;
}

const FOnsetEquipmentDefinition* UOnsetInventoryComponent::GetEquippedItem(EOnsetEquipmentSlot Slot) const
{
	const FOnsetEquippedEntry* Entry = GetEquippedEntry(Slot);
	if (!Entry || Entry->RowName.IsNone())
	{
		return nullptr;
	}
	return UOnsetEquipmentLibrary::GetDefinition(Entry->RowName);
}

FName UOnsetInventoryComponent::GetEquippedRow(EOnsetEquipmentSlot Slot) const
{
	const FOnsetEquippedEntry* Entry = GetEquippedEntry(Slot);
	return Entry ? Entry->RowName : NAME_None;
}

TMap<EOnsetEquipmentSlot, FName> UOnsetInventoryComponent::GetEquippedMap() const
{
	TMap<EOnsetEquipmentSlot, FName> Map;
	for (const FOnsetEquippedEntry& Entry : EquippedEntries)
	{
		if (!Entry.RowName.IsNone())
		{
			Map.Add(Entry.Slot, Entry.RowName);
		}
	}
	return Map;
}

// --- Durability & Repair ---

void UOnsetInventoryComponent::DegradeEquippedWeapon(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.0f)
	{
		return;
	}

	for (FOnsetEquippedEntry& Entry : EquippedEntries)
	{
		if (Entry.Slot == EOnsetEquipmentSlot::Weapon && !Entry.RowName.IsNone())
		{
			Entry.InstanceData.CurrentDurability = FMath::Max(0.0f, Entry.InstanceData.CurrentDurability - Amount);
			OnInventoryChanged.Broadcast();
			break;
		}
	}
}

void UOnsetInventoryComponent::DegradeEquippedArmor(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0.0f)
	{
		return;
	}

	bool bChanged = false;
	for (FOnsetEquippedEntry& Entry : EquippedEntries)
	{
		if (Entry.Slot != EOnsetEquipmentSlot::Weapon && !Entry.RowName.IsNone())
		{
			Entry.InstanceData.CurrentDurability = FMath::Max(0.0f, Entry.InstanceData.CurrentDurability - Amount);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		OnInventoryChanged.Broadcast();
	}
}

void UOnsetInventoryComponent::ApplyDurabilityLossOnDeath(float Percent)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Percent <= 0.0f)
	{
		return;
	}

	bool bChanged = false;
	const float Fraction = Percent / 100.0f;
	for (FOnsetEquippedEntry& Entry : EquippedEntries)
	{
		if (!Entry.RowName.IsNone())
		{
			const float Loss = Entry.InstanceData.MaxDurability * Fraction;
			Entry.InstanceData.CurrentDurability = FMath::Max(0.0f, Entry.InstanceData.CurrentDurability - Loss);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		OnInventoryChanged.Broadcast();
	}
}

bool UOnsetInventoryComponent::RepairEquippedItem(EOnsetEquipmentSlot Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	FOnsetEquippedEntry* Entry = EquippedEntries.FindByPredicate([Slot](const FOnsetEquippedEntry& E) { return E.Slot == Slot; });
	if (!Entry || Entry->RowName.IsNone())
	{
		return false;
	}

	if (Entry->InstanceData.CurrentDurability >= Entry->InstanceData.MaxDurability)
	{
		return false;
	}

	const FOnsetEquipmentDefinition* Def = UOnsetEquipmentLibrary::GetDefinition(Entry->RowName);
	int32 CreditsCost = 0;
	int32 ScrapsCost = 0;
	UOnsetEquipmentLibrary::CalculateRepairCost(Def, Entry->InstanceData, CreditsCost, ScrapsCost);

	const FName ScrapRow(TEXT("CompositeScrap"));
	const int32 AvailableScraps = GetItemCount(EOnsetItemCategory::Material, ScrapRow);
	if (AvailableScraps < ScrapsCost)
	{
		return false;
	}

	RemoveItem(EOnsetItemCategory::Material, ScrapRow, ScrapsCost);
	Entry->InstanceData.CurrentDurability = Entry->InstanceData.MaxDurability;
	OnInventoryChanged.Broadcast();
	return true;
}

bool UOnsetInventoryComponent::RepairAllEquipped()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const FName ScrapRow(TEXT("CompositeScrap"));
	bool bRepairedAny = false;

	for (FOnsetEquippedEntry& Entry : EquippedEntries)
	{
		if (Entry.RowName.IsNone() || Entry.InstanceData.CurrentDurability >= Entry.InstanceData.MaxDurability)
		{
			continue;
		}

		const FOnsetEquipmentDefinition* Def = UOnsetEquipmentLibrary::GetDefinition(Entry.RowName);
		int32 CreditsCost = 0;
		int32 ScrapsCost = 0;
		UOnsetEquipmentLibrary::CalculateRepairCost(Def, Entry.InstanceData, CreditsCost, ScrapsCost);

		if (GetItemCount(EOnsetItemCategory::Material, ScrapRow) >= ScrapsCost)
		{
			RemoveItem(EOnsetItemCategory::Material, ScrapRow, ScrapsCost);
			Entry.InstanceData.CurrentDurability = Entry.InstanceData.MaxDurability;
			bRepairedAny = true;
		}
	}

	if (bRepairedAny)
	{
		OnInventoryChanged.Broadcast();
	}
	return bRepairedAny;
}

bool UOnsetInventoryComponent::RepairBagItem(int32 BagSlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Items.IsValidIndex(BagSlotIndex))
	{
		return false;
	}

	FOnsetInventoryEntry& Entry = Items[BagSlotIndex];
	if (Entry.Category != EOnsetItemCategory::Equipment || Entry.RowName.IsNone())
	{
		return false;
	}

	if (Entry.InstanceData.CurrentDurability >= Entry.InstanceData.MaxDurability)
	{
		return false;
	}

	const FOnsetEquipmentDefinition* Def = UOnsetEquipmentLibrary::GetDefinition(Entry.RowName);
	int32 CreditsCost = 0;
	int32 ScrapsCost = 0;
	UOnsetEquipmentLibrary::CalculateRepairCost(Def, Entry.InstanceData, CreditsCost, ScrapsCost);

	const FName ScrapRow(TEXT("CompositeScrap"));
	if (GetItemCount(EOnsetItemCategory::Material, ScrapRow) < ScrapsCost)
	{
		return false;
	}

	RemoveItem(EOnsetItemCategory::Material, ScrapRow, ScrapsCost);
	Entry.InstanceData.CurrentDurability = Entry.InstanceData.MaxDurability;
	OnInventoryChanged.Broadcast();
	return true;
}

// --- Salvage & Infusion ---

bool UOnsetInventoryComponent::SalvageItem(int32 BagSlotIndex, TArray<FOnsetInventoryEntry>& OutYield)
{
	OutYield.Reset();
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Items.IsValidIndex(BagSlotIndex))
	{
		return false;
	}

	FOnsetInventoryEntry& Target = Items[BagSlotIndex];
	if (Target.Category != EOnsetItemCategory::Equipment || Target.RowName.IsNone())
	{
		return false;
	}

	const FOnsetEquipmentDefinition* Def = UOnsetEquipmentLibrary::GetDefinition(Target.RowName);
	UOnsetEquipmentLibrary::GetSalvageYield(Def, Target.InstanceData, OutYield);

	Target.Count -= 1;
	if (Target.Count <= 0)
	{
		Items.RemoveAt(BagSlotIndex);
	}

	AddItems(OutYield);
	OnInventoryChanged.Broadcast();
	return true;
}

bool UOnsetInventoryComponent::InfuseItem(int32 TargetSlotIndex, int32 CatalystSlotIndex, bool& bOutGradeUp, bool& bOutCriticalProc)
{
	bOutGradeUp = false;
	bOutCriticalProc = false;

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (TargetSlotIndex == CatalystSlotIndex || !Items.IsValidIndex(TargetSlotIndex) || !Items.IsValidIndex(CatalystSlotIndex))
	{
		return false;
	}

	FOnsetInventoryEntry& Target = Items[TargetSlotIndex];
	FOnsetInventoryEntry& Catalyst = Items[CatalystSlotIndex];

	if (Target.Category != EOnsetItemCategory::Equipment || Catalyst.Category != EOnsetItemCategory::Equipment)
	{
		return false;
	}
	if (Target.RowName != Catalyst.RowName)
	{
		return false;
	}

	if (Target.InstanceData.PrimaryStatGrade == EOnsetQualityGrade::GradeS &&
		Target.InstanceData.SecondaryStatGrade == EOnsetQualityGrade::GradeS)
	{
		return false;
	}

	const float ProgressGain = UOnsetEquipmentLibrary::GetInfusionProgressPerDuplicate(Target.InstanceData.PrimaryStatGrade);
	const bool bCrit = UOnsetEquipmentLibrary::RollCriticalResonance();
	bOutCriticalProc = bCrit;

	if (bCrit || (Target.InstanceData.InfusionProgress + ProgressGain >= 1.0f))
	{
		bOutGradeUp = true;
		Target.InstanceData.PrimaryStatGrade = UOnsetEquipmentLibrary::GetNextGrade(Target.InstanceData.PrimaryStatGrade);
		Target.InstanceData.SecondaryStatGrade = UOnsetEquipmentLibrary::GetNextGrade(Target.InstanceData.SecondaryStatGrade);
		Target.InstanceData.InfusionProgress = 0.0f;
		Target.InstanceData.InfusionLevel += 1;
		Target.InstanceData.CurrentDurability = Target.InstanceData.MaxDurability;
	}
	else
	{
		Target.InstanceData.InfusionProgress += ProgressGain;
	}

	Catalyst.Count -= 1;
	if (Catalyst.Count <= 0)
	{
		Items.RemoveAt(CatalystSlotIndex);
	}

	OnInventoryChanged.Broadcast();
	return true;
}

// --- Persistence ---

FString UOnsetInventoryComponent::SerializeEquipmentJSON() const
{
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	UEnum* SlotEnum = StaticEnum<EOnsetEquipmentSlot>();
	if (SlotEnum)
	{
		for (int32 Index = 0; Index < SlotEnum->NumEnums(); ++Index)
		{
			const EOnsetEquipmentSlot Slot = static_cast<EOnsetEquipmentSlot>(SlotEnum->GetValueByIndex(Index));
			const FOnsetEquippedEntry* Entry = GetEquippedEntry(Slot);
			if (Entry && !Entry->RowName.IsNone())
			{
				TSharedPtr<FJsonObject> SlotObj = MakeShareable(new FJsonObject);
				SlotObj->SetStringField(TEXT("r"), Entry->RowName.ToString());
				SlotObj->SetNumberField(TEXT("d"), Entry->InstanceData.CurrentDurability);
				SlotObj->SetNumberField(TEXT("md"), Entry->InstanceData.MaxDurability);
				SlotObj->SetNumberField(TEXT("pg"), static_cast<int32>(Entry->InstanceData.PrimaryStatGrade));
				SlotObj->SetNumberField(TEXT("sg"), static_cast<int32>(Entry->InstanceData.SecondaryStatGrade));
				SlotObj->SetNumberField(TEXT("ip"), Entry->InstanceData.InfusionProgress);
				SlotObj->SetNumberField(TEXT("il"), Entry->InstanceData.InfusionLevel);
				Root->SetObjectField(SlotEnum->GetDisplayValueAsText(Slot).ToString(), SlotObj);
			}
			else
			{
				Root->SetStringField(SlotEnum->GetDisplayValueAsText(Slot).ToString(), TEXT("None"));
			}
		}
	}

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return Out;
}

void UOnsetInventoryComponent::DeserializeEquipmentJSON(const FString& JSON)
{
	EquippedEntries.Reset();
	if (JSON.IsEmpty() || JSON == TEXT("{}"))
	{
		return;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSON);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return;
	}

	UEnum* SlotEnum = StaticEnum<EOnsetEquipmentSlot>();
	if (!SlotEnum)
	{
		return;
	}

	for (int32 Index = 0; Index < SlotEnum->NumEnums(); ++Index)
	{
		const EOnsetEquipmentSlot Slot = static_cast<EOnsetEquipmentSlot>(SlotEnum->GetValueByIndex(Index));
		const FString Key = SlotEnum->GetDisplayValueAsText(Slot).ToString();
		if (!Root->HasField(Key))
		{
			continue;
		}

		const TSharedPtr<FJsonValue> Field = Root->TryGetField(Key);
		if (!Field.IsValid())
		{
			continue;
		}

		if (Field->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> SlotObj = Field->AsObject();
			const FString RowName = SlotObj->GetStringField(TEXT("r"));
			if (!RowName.IsEmpty() && RowName != TEXT("None"))
			{
				FOnsetEquipmentInstanceData Instance;
				if (SlotObj->HasField(TEXT("d")))  Instance.CurrentDurability = SlotObj->GetNumberField(TEXT("d"));
				if (SlotObj->HasField(TEXT("md"))) Instance.MaxDurability = SlotObj->GetNumberField(TEXT("md"));
				if (SlotObj->HasField(TEXT("pg"))) Instance.PrimaryStatGrade = static_cast<EOnsetQualityGrade>(FMath::Clamp(static_cast<int32>(SlotObj->GetNumberField(TEXT("pg"))), 0, 4));
				if (SlotObj->HasField(TEXT("sg"))) Instance.SecondaryStatGrade = static_cast<EOnsetQualityGrade>(FMath::Clamp(static_cast<int32>(SlotObj->GetNumberField(TEXT("sg"))), 0, 4));
				if (SlotObj->HasField(TEXT("ip"))) Instance.InfusionProgress = SlotObj->GetNumberField(TEXT("ip"));
				if (SlotObj->HasField(TEXT("il"))) Instance.InfusionLevel = static_cast<int32>(SlotObj->GetNumberField(TEXT("il")));

				EquipItemWithInstance(Slot, FName(*RowName), Instance);
			}
		}
		else if (Field->Type == EJson::String)
		{
			const FString RowName = Field->AsString();
			if (!RowName.IsEmpty() && RowName != TEXT("None"))
			{
				EquipItem(Slot, FName(*RowName));
			}
		}
	}
}

FString UOnsetInventoryComponent::SerializeInventoryJSON() const
{
	TArray<TSharedPtr<FJsonValue>> Values;
	UEnum* CategoryEnum = StaticEnum<EOnsetItemCategory>();
	for (const FOnsetInventoryEntry& Entry : Items)
	{
		TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject);
		const FString CategoryName = CategoryEnum
			? CategoryEnum->GetNameStringByValue(static_cast<int64>(Entry.Category))
			: TEXT("Equipment");
		Obj->SetStringField(TEXT("c"), CategoryName);
		Obj->SetStringField(TEXT("r"), Entry.RowName.ToString());
		Obj->SetNumberField(TEXT("n"), Entry.Count);

		if (Entry.Category == EOnsetItemCategory::Equipment)
		{
			Obj->SetNumberField(TEXT("d"), Entry.InstanceData.CurrentDurability);
			Obj->SetNumberField(TEXT("md"), Entry.InstanceData.MaxDurability);
			Obj->SetNumberField(TEXT("pg"), static_cast<int32>(Entry.InstanceData.PrimaryStatGrade));
			Obj->SetNumberField(TEXT("sg"), static_cast<int32>(Entry.InstanceData.SecondaryStatGrade));
			Obj->SetNumberField(TEXT("ip"), Entry.InstanceData.InfusionProgress);
			Obj->SetNumberField(TEXT("il"), Entry.InstanceData.InfusionLevel);
		}

		Values.Add(MakeShareable(new FJsonValueObject(Obj)));
	}

	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Values, Writer);
	return Out;
}

void UOnsetInventoryComponent::DeserializeInventoryJSON(const FString& JSON)
{
	Items.Reset();
	if (JSON.IsEmpty() || JSON == TEXT("{}") || JSON == TEXT("[]"))
	{
		return;
	}

	TArray<TSharedPtr<FJsonValue>> Values;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSON);
	if (!FJsonSerializer::Deserialize(Reader, Values))
	{
		return;
	}

	UEnum* CategoryEnum = StaticEnum<EOnsetItemCategory>();
	for (const TSharedPtr<FJsonValue>& Value : Values)
	{
		if (!Value.IsValid())
		{
			continue;
		}

		const TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (Obj.IsValid())
		{
			EOnsetItemCategory Category = EOnsetItemCategory::Equipment;
			if (CategoryEnum)
			{
				const int64 Found = CategoryEnum->GetValueByNameString(Obj->GetStringField(TEXT("c")));
				Category = Found != INDEX_NONE ? static_cast<EOnsetItemCategory>(Found) : EOnsetItemCategory::Equipment;
			}
			const FName RowName = FName(*Obj->GetStringField(TEXT("r")));
			const int32 Count = FMath::Max(1, static_cast<int32>(Obj->GetNumberField(TEXT("n"))));

			if (Category == EOnsetItemCategory::Equipment)
			{
				FOnsetEquipmentInstanceData Instance;
				if (Obj->HasField(TEXT("d")))  Instance.CurrentDurability = Obj->GetNumberField(TEXT("d"));
				if (Obj->HasField(TEXT("md"))) Instance.MaxDurability = Obj->GetNumberField(TEXT("md"));
				if (Obj->HasField(TEXT("pg"))) Instance.PrimaryStatGrade = static_cast<EOnsetQualityGrade>(FMath::Clamp(static_cast<int32>(Obj->GetNumberField(TEXT("pg"))), 0, 4));
				if (Obj->HasField(TEXT("sg"))) Instance.SecondaryStatGrade = static_cast<EOnsetQualityGrade>(FMath::Clamp(static_cast<int32>(Obj->GetNumberField(TEXT("sg"))), 0, 4));
				if (Obj->HasField(TEXT("ip"))) Instance.InfusionProgress = Obj->GetNumberField(TEXT("ip"));
				if (Obj->HasField(TEXT("il"))) Instance.InfusionLevel = static_cast<int32>(Obj->GetNumberField(TEXT("il")));

				for (int32 i = 0; i < Count; ++i)
				{
					UOnsetItemLibrary::AddEquipment(Items, RowName, Instance);
				}
			}
			else
			{
				UOnsetItemLibrary::AddStacked(Items, Category, RowName, Count);
			}
		}
		else
		{
			// Legacy flat row-ID list: treat entries as Equipment.
			const FString RowName = Value->AsString();
			if (!RowName.IsEmpty())
			{
				UOnsetItemLibrary::AddStacked(Items, EOnsetItemCategory::Equipment, FName(*RowName), 1);
			}
		}
	}
}

// --- Replication ---

void UOnsetInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = false;
	Params.Condition = bReplicateToOwnerOnly ? COND_OwnerOnly : COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(UOnsetInventoryComponent, Items, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(UOnsetInventoryComponent, EquippedEntries, Params);
}

void UOnsetInventoryComponent::OnRep_Items()
{
	OnInventoryChanged.Broadcast();
}

void UOnsetInventoryComponent::OnRep_Equipped()
{
	OnInventoryChanged.Broadcast();
}