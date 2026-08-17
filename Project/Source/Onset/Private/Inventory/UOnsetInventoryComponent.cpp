// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/UOnsetInventoryComponent.h"

#include "Combat/OnsetEquipmentLibrary.h"
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

void UOnsetInventoryComponent::AddItem(FName RowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || RowName.IsNone())
	{
		return;
	}
	Items.Add(RowName);
	OnInventoryChanged.Broadcast();
}

void UOnsetInventoryComponent::AddItems(const TArray<FName>& RowNames)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	for (const FName& RowName : RowNames)
	{
		if (!RowName.IsNone())
		{
			Items.Add(RowName);
		}
	}
	if (RowNames.Num() > 0)
	{
		OnInventoryChanged.Broadcast();
	}
}

bool UOnsetInventoryComponent::RemoveItem(FName RowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	const int32 Index = Items.Find(RowName);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	Items.RemoveAt(Index);
	OnInventoryChanged.Broadcast();
	return true;
}

void UOnsetInventoryComponent::RemoveAllOfItem(FName RowName)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	const int32 Removed = Items.Remove(RowName);
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

void UOnsetInventoryComponent::SetItems(const TArray<FName>& RowNames)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Items = RowNames;
	OnInventoryChanged.Broadcast();
}

bool UOnsetInventoryComponent::HasItem(FName RowName) const
{
	return Items.Contains(RowName);
}

int32 UOnsetInventoryComponent::GetItemCount(FName RowName) const
{
	return Items.FilterByPredicate([RowName](const FName& Entry) { return Entry == RowName; }).Num();
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
	const int32 Removed = EquippedEntries.RemoveAll([Slot](const FOnsetEquippedEntry& Entry) { return Entry.Slot == Slot; });
	if (Removed > 0)
	{
		OnInventoryChanged.Broadcast();
	}
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
	if (!RemoveItem(RowName))
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
	for (const FName& RowName : Items)
	{
		Values.Add(MakeShareable(new FJsonValueString(RowName.ToString())));
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

	for (const TSharedPtr<FJsonValue>& Value : Values)
	{
		if (!Value.IsValid())
		{
			continue;
		}
		const FString RowName = Value->AsString();
		if (!RowName.IsEmpty())
		{
			Items.Add(FName(*RowName));
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