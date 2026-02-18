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

void UMaterialTagAssetUserData::EnsureAllSlotsPopulated()
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(GetOuter());
	if (!Mesh) return;

	const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();

	// Build set of existing slot names
	TSet<FName> ExistingSlots;
	for (const FMaterialSlotTagEntry& Entry : MaterialSlotTags)
	{
		ExistingSlots.Add(Entry.MaterialSlotName);
	}

	// Add missing slots
	bool bAdded = false;
	for (int32 i = 0; i < Materials.Num(); i++)
	{
		FName SlotName = Materials[i].MaterialSlotName;
		if (!ExistingSlots.Contains(SlotName))
		{
			FMaterialSlotTagEntry Entry;
			Entry.MaterialSlotName = SlotName;
			MaterialSlotTags.Add(Entry);
			bAdded = true;
		}
	}

	if (bAdded)
	{
		// Sort to match mesh order
		TMap<FName, int32> SlotOrder;
		for (int32 i = 0; i < Materials.Num(); i++)
		{
			SlotOrder.Add(Materials[i].MaterialSlotName, i);
		}
		MaterialSlotTags.Sort([&SlotOrder](const FMaterialSlotTagEntry& A, const FMaterialSlotTagEntry& B)
		{
			int32* OrderA = SlotOrder.Find(A.MaterialSlotName);
			int32* OrderB = SlotOrder.Find(B.MaterialSlotName);
			int32 IdxA = OrderA ? *OrderA : MAX_int32;
			int32 IdxB = OrderB ? *OrderB : MAX_int32;
			return IdxA < IdxB;
		});
	}
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

FString UMaterialTagAssetUserData::GetPresetIniPath()
{
	return FPaths::ProjectPluginsDir() / TEXT("MaterialTagPlugin") / TEXT("Config") / TEXT("MaterialTagPresets.ini");
}

#if WITH_EDITOR
void UMaterialTagAssetUserData::PostLoad()
{
	Super::PostLoad();
	EnsureAllSlotsPopulated();
}

void UMaterialTagAssetUserData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropName = PropertyChangedEvent.GetPropertyName();

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
