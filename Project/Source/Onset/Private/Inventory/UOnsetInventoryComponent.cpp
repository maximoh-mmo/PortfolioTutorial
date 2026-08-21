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

	EquippedEntries.RemoveAll([Slot](const FOnsetEquippedEntry& Entry) { return Entry.Slot == Slot; });
	if (!RowName.IsNone())
	{
		FOnsetEquippedEntry NewEntry;
		NewEntry.Slot = Slot;
		NewEntry.RowName = RowName;
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

	const FName RowName = GetEquippedRow(Slot);
	if (RowName.IsNone())
	{
		return;
	}
	const int32 Removed = EquippedEntries.RemoveAll([Slot](const FOnsetEquippedEntry& Entry) { return Entry.Slot == Slot; });
	if (Removed <= 0)
	{
		return;
	}

	// Unequipping returns the item to the bag as a stackable Equipment entry.
	UOnsetItemLibrary::AddStacked(Items, EOnsetItemCategory::Equipment, RowName, 1);
	OnInventoryChanged.Broadcast();
}

bool UOnsetInventoryComponent::EquipFromInventory(FName RowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || RowName.IsNone())
	{
		return false;
	}

	const FOnsetEquipmentDefinition* Definition = UOnsetEquipmentLibrary::GetDefinition(RowName);
	if (!Definition)
	{
		return false;
	}
	if (!RemoveItem(EOnsetItemCategory::Equipment, RowName, 1))
	{
		return false;
	}

	EquipItem(Definition->Slot, RowName);
	return true;
}

const FOnsetEquipmentDefinition* UOnsetInventoryComponent::GetEquippedItem(EOnsetEquipmentSlot Slot) const
{
	const FOnsetEquippedEntry* Entry = EquippedEntries.FindByPredicate(
		[Slot](const FOnsetEquippedEntry& Candidate) { return Candidate.Slot == Slot; });
	if (!Entry || Entry->RowName.IsNone())
	{
		return nullptr;
	}
	return UOnsetEquipmentLibrary::GetDefinition(Entry->RowName);
}

FName UOnsetInventoryComponent::GetEquippedRow(EOnsetEquipmentSlot Slot) const
{
	const FOnsetEquippedEntry* Entry = EquippedEntries.FindByPredicate(
		[Slot](const FOnsetEquippedEntry& Candidate) { return Candidate.Slot == Slot; });
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
			const FName RowName = GetEquippedRow(Slot);
			Root->SetStringField(SlotEnum->GetDisplayValueAsText(Slot).ToString(), RowName.ToString());
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
		const FString RowName = Root->GetStringField(SlotEnum->GetDisplayValueAsText(Slot).ToString());
		if (!RowName.IsEmpty())
		{
			EquipItem(Slot, FName(*RowName));
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
			UOnsetItemLibrary::AddStacked(Items, Category, RowName, Count);
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