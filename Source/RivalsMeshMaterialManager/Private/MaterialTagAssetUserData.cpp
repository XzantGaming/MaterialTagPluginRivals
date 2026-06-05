#include "MaterialTagAssetUserData.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Internationalization/Regex.h"
#if WITH_EDITOR
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#endif

UMaterialTagAssetUserData::UMaterialTagAssetUserData()
{
}

void UMaterialTagAssetUserData::PopulateFromMesh()
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(GetOuter());
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("MaterialTagAssetUserData: Not attached to a SkeletalMesh"));
		return;
	}

	const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
	
	// Clear existing entries
	MaterialSlotTags.Empty();
	
	// Create an entry for each material slot using the slot name
	for (int32 i = 0; i < Materials.Num(); i++)
	{
		FMaterialSlotTagEntry Entry;
		Entry.MaterialSlotName = Materials[i].MaterialSlotName;
		MaterialSlotTags.Add(Entry);
	}
	
	UE_LOG(LogTemp, Log, TEXT("MaterialTagAssetUserData: Populated %d material slot entries"), Materials.Num());
	
#if WITH_EDITOR
	if (UObject* Outer = GetOuter())
	{
		Outer->MarkPackageDirty();
	}
#endif
}

void UMaterialTagAssetUserData::RefreshSlots()
{
	EnsureAllSlotsPopulated();
	UE_LOG(LogTemp, Log, TEXT("MaterialTagAssetUserData: Refreshed material slots"));

#if WITH_EDITOR
	if (UObject* Outer = GetOuter())
	{
		Outer->MarkPackageDirty();
	}
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.NotifyCustomizationModuleChanged();
#endif
}

void UMaterialTagAssetUserData::EnsureAllSlotsPopulated()
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(GetOuter());
	if (!Mesh) return;

	const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
	const int32 MeshSlotCount = Materials.Num();

	// Build a new array that is 1:1 with the mesh material array (by index).
	// Preserve existing tag data for slots that match by index AND name.
	TArray<FMaterialSlotTagEntry> NewSlots;
	NewSlots.SetNum(MeshSlotCount);

	for (int32 i = 0; i < MeshSlotCount; i++)
	{
		NewSlots[i].MaterialSlotName = Materials[i].MaterialSlotName;

		// Try to carry over tag data from the old array at the same index
		if (MaterialSlotTags.IsValidIndex(i) && MaterialSlotTags[i].MaterialSlotName == Materials[i].MaterialSlotName)
		{
			NewSlots[i].GameplayTags = MaterialSlotTags[i].GameplayTags;
		}
		else
		{
			// Fallback: search old array by name (first match)
			for (const FMaterialSlotTagEntry& Old : MaterialSlotTags)
			{
				if (Old.MaterialSlotName == Materials[i].MaterialSlotName && Old.GameplayTags.Num() > 0)
				{
					NewSlots[i].GameplayTags = Old.GameplayTags;
					break;
				}
			}
		}
	}

	MaterialSlotTags = MoveTemp(NewSlots);
}

FGameplayTagContainer UMaterialTagAssetUserData::GetTagsForSlot(FName SlotName) const
{
	for (const FMaterialSlotTagEntry& Entry : MaterialSlotTags)
	{
		if (Entry.MaterialSlotName == SlotName)
		{
			return Entry.ToContainer();
		}
	}
	return FGameplayTagContainer();
}

bool UMaterialTagAssetUserData::HasTagsForSlot(FName SlotName) const
{
	for (const FMaterialSlotTagEntry& Entry : MaterialSlotTags)
	{
		if (Entry.MaterialSlotName == SlotName)
		{
			return Entry.Num() > 0;
		}
	}
	return false;
}

TArray<FString> UMaterialTagAssetUserData::GetPresetMeshNames() const
{
	TArray<FString> Names;
	Names.Add(TEXT(""));  // Empty option to clear selection

	FString IniPath = GetPresetIniPath();
	if (!FPaths::FileExists(IniPath))
	{
		return Names;
	}

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *IniPath);

	for (const FString& Line : Lines)
	{
		FString Trimmed = Line.TrimStartAndEnd();
		if (Trimmed.StartsWith(TEXT("[")) && Trimmed.EndsWith(TEXT("]")))
		{
			FString SectionName = Trimmed.Mid(1, Trimmed.Len() - 2);
			if (!SectionName.IsEmpty())
			{
				Names.Add(SectionName);
			}
		}
	}

	return Names;
}

void UMaterialTagAssetUserData::UpdatePresetInfo()
{
	if (PresetMeshName.IsEmpty())
	{
		PresetTags.InfoText = TEXT("");
		return;
	}

	FString IniPath = GetPresetIniPath();
	if (!FPaths::FileExists(IniPath))
	{
		PresetTags.InfoText = TEXT("Preset INI not found.\nExpected: ") + IniPath;
		return;
	}

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *IniPath);

	// Find the section for this mesh
	FString SectionHeader = FString::Printf(TEXT("[%s]"), *PresetMeshName);
	bool bInSection = false;
	FString InfoText;

	for (const FString& Line : Lines)
	{
		FString Trimmed = Line.TrimStartAndEnd();

		if (Trimmed.StartsWith(TEXT("[")))
		{
			if (bInSection)
			{
				break;  // Hit next section, done
			}
			if (Trimmed.Equals(SectionHeader, ESearchCase::IgnoreCase))
			{
				bInSection = true;
			}
			continue;
		}

		if (!bInSection) continue;
		if (Trimmed.IsEmpty() || Trimmed.StartsWith(TEXT(";"))) continue;

		// Parse Tag=Slot1, Slot2
		FString TagName, SlotList;
		if (Trimmed.Split(TEXT("="), &TagName, &SlotList))
		{
			if (!InfoText.IsEmpty())
			{
				InfoText += TEXT("\n");
			}
			InfoText += FString::Printf(TEXT("%s\n    Slots: %s"), *TagName.TrimStartAndEnd(), *SlotList.TrimStartAndEnd());
		}
	}

	if (InfoText.IsEmpty())
	{
		PresetTags.InfoText = FString::Printf(TEXT("No preset data found for '%s'"), *PresetMeshName);
	}
	else
	{
		PresetTags.InfoText = InfoText;
	}
}

void UMaterialTagAssetUserData::AutoMatchPresetFromMesh()
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(GetOuter());
	if (!Mesh) return;

	FString MeshName = Mesh->GetName();
	TArray<FString> Presets = GetPresetMeshNames();

	// Try exact match first
	for (const FString& P : Presets)
	{
		if (P.Equals(MeshName, ESearchCase::IgnoreCase))
		{
			PresetMeshName = P;
			return;
		}
	}

	// Try substring match (mesh name contains preset name or vice versa)
	for (const FString& P : Presets)
	{
		if (P.IsEmpty()) continue;
		if (MeshName.Contains(P) || P.Contains(MeshName))
		{
			PresetMeshName = P;
			return;
		}
	}

	// Try matching by character ID (e.g. "1014001" in both names)
	// Extract digits from mesh name
	FRegexPattern Pattern(TEXT("(\\d{7})"));
	FRegexMatcher Matcher(Pattern, MeshName);
	if (Matcher.FindNext())
	{
		FString CharId = Matcher.GetCaptureGroup(1);
		for (const FString& P : Presets)
		{
			if (P.Contains(CharId))
			{
				PresetMeshName = P;
				return;
			}
		}
	}
}

void UMaterialTagAssetUserData::ApplyPresetTagsByIndex()
{
	if (PresetMeshName.IsEmpty()) return;

	FString IniPath = GetPresetIniPath();
	if (!FPaths::FileExists(IniPath)) return;

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *IniPath);

	FString SectionHeader = FString::Printf(TEXT("[%s]"), *PresetMeshName);
	bool bInSection = false;

	// Parse Slot_N=SlotName|Tag1,Tag2 format and build index->tags map
	TMap<int32, TArray<FString>> IndexToTags;

	for (const FString& Line : Lines)
	{
		FString Trimmed = Line.TrimStartAndEnd();

		if (Trimmed.StartsWith(TEXT("[")))
		{
			if (bInSection) break;
			if (Trimmed.Equals(SectionHeader, ESearchCase::IgnoreCase))
				bInSection = true;
			continue;
		}

		if (!bInSection) continue;
		if (Trimmed.IsEmpty() || Trimmed.StartsWith(TEXT(";"))) continue;

		FString Key, Value;
		if (!Trimmed.Split(TEXT("="), &Key, &Value)) continue;
		Key = Key.TrimStartAndEnd();
		Value = Value.TrimStartAndEnd();

		if (Key.StartsWith(TEXT("Slot_")))
		{
			int32 Idx = FCString::Atoi(*Key.Mid(5));
			
			// Parse SlotName|Tag1,Tag2 format
			FString SlotName, TagsPart;
			if (Value.Split(TEXT("|"), &SlotName, &TagsPart))
			{
				TArray<FString> Tags;
				TagsPart.ParseIntoArray(Tags, TEXT(","));
				for (FString& Tag : Tags)
				{
					Tag = Tag.TrimStartAndEnd();
					if (!Tag.IsEmpty())
					{
						IndexToTags.FindOrAdd(Idx).Add(Tag);
					}
				}
			}
		}
	}

	// Apply tags to MaterialSlotTags by index
	for (const auto& Pair : IndexToTags)
	{
		int32 SlotIndex = Pair.Key;
		const TArray<FString>& Tags = Pair.Value;

		if (MaterialSlotTags.IsValidIndex(SlotIndex))
		{
			// Clear existing tags for this slot
			MaterialSlotTags[SlotIndex].GameplayTags.Empty();

			// Add each tag
			for (const FString& TagName : Tags)
			{
				FGameplayTagEntry Entry;
				Entry.Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
				if (Entry.Tag.IsValid())
				{
					MaterialSlotTags[SlotIndex].GameplayTags.Add(Entry);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("MaterialTagAssetUserData: Tag '%s' not found in GameplayTags registry"), *TagName);
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("MaterialTagAssetUserData: Applied preset tags by index for '%s'"), *PresetMeshName);
}

FString UMaterialTagAssetUserData::GetPresetIniPath()
{
	return FPaths::ProjectConfigDir() / TEXT("MaterialTagPresets.ini");
}

#if WITH_EDITOR
void UMaterialTagAssetUserData::PostLoad()
{
	Super::PostLoad();
	EnsureAllSlotsPopulated();
	SavedSlotTags = MaterialSlotTags;
}

void UMaterialTagAssetUserData::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	// Save the array state before any edit so we can restore if user tries to add/delete
	SavedSlotTags = MaterialSlotTags;
}

void UMaterialTagAssetUserData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropName = PropertyChangedEvent.GetPropertyName();
	FName MemberName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// Guard the TOP-LEVEL MaterialSlotTags array against user add/delete/insert/duplicate/clear.
	// Only block if the changed property IS MaterialSlotTags itself (not a nested sub-property like GameplayTags).
	if (MemberName == GET_MEMBER_NAME_CHECKED(UMaterialTagAssetUserData, MaterialSlotTags)
		&& PropName == GET_MEMBER_NAME_CHECKED(UMaterialTagAssetUserData, MaterialSlotTags))
	{
		int32 ChangeType = PropertyChangedEvent.ChangeType;
		bool bArrayStructureChanged = (ChangeType & (EPropertyChangeType::ArrayAdd | EPropertyChangeType::ArrayRemove | EPropertyChangeType::ArrayClear | EPropertyChangeType::Duplicate)) != 0;

		if (bArrayStructureChanged && SavedSlotTags.Num() > 0)
		{
			MaterialSlotTags = SavedSlotTags;
			UE_LOG(LogTemp, Warning, TEXT("MaterialTagAssetUserData: Slot add/delete blocked. Use 'Refresh Slots' button instead."));

			FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.NotifyCustomizationModuleChanged();
			return;
		}
	}

	bool bNeedsRefresh = false;

	// Handle auto-match toggle
	if (PropName == GET_MEMBER_NAME_CHECKED(UMaterialTagAssetUserData, bAutoMatchPreset))
	{
		if (bAutoMatchPreset)
		{
			AutoMatchPresetFromMesh();
		}
		bNeedsRefresh = true;
	}

	// Handle auto-populate toggle
	if (PropName == GET_MEMBER_NAME_CHECKED(UMaterialTagAssetUserData, bAutoPopulateTags))
	{
		if (bAutoPopulateTags)
		{
			ApplyPresetTagsByIndex();
		}
		bNeedsRefresh = true;
	}

	// Update preset info when PresetMeshName changes
	if (PropName == GET_MEMBER_NAME_CHECKED(UMaterialTagAssetUserData, PresetMeshName))
	{
		UpdatePresetInfo();
		bNeedsRefresh = true;
	}

	// Force the details panel to rebuild so FPresetTagDisplay customization refreshes
	if (bNeedsRefresh)
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	// Mark the owning asset as modified
	if (UObject* Outer = GetOuter())
	{
		Outer->MarkPackageDirty();
	}
}
#endif
