#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "GameplayTagContainer.h"
#include "Engine/SkeletalMesh.h"
#include "MaterialTagAssetUserData.generated.h"

/**
 * Wrapper for a single FGameplayTag.
 * Used inside TArray so each tag gets its own independent tag picker in the editor.
 * UE cannot merge tags across struct boundaries.
 */
USTRUCT(BlueprintType)
struct MATERIALTAGPLUGIN_API FGameplayTagEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Tags", meta=(Categories="MaterialTag"))
	FGameplayTag Tag;
};

/**
 * Wrapper struct for the preset tag display area.
 * Has a custom property type customization that renders draggable tag pills.
 */
USTRUCT(BlueprintType)
struct MATERIALTAGPLUGIN_API FPresetTagDisplay
{
	GENERATED_BODY()

	/** Serialized info text (fallback display) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preset", meta=(MultiLine="true"))
	FString InfoText;
};

/**
 * Entry for a single material slot's gameplay tags.
 * Each slot has a sub-array of tag entries (click + to add more).
 * Tags are stored in individual wrapper structs to prevent UE's
 * automatic tag hierarchy merging.
 */
USTRUCT(BlueprintType)
struct MATERIALTAGPLUGIN_API FMaterialSlotTagEntry
{
	GENERATED_BODY()

	/** Name of the material slot (must match exactly) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Tags")
	FName MaterialSlotName;

	/** Gameplay tags for this slot — click + to add more entries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Tags")
	TArray<FGameplayTagEntry> GameplayTags;

	/** Helper: build an FGameplayTagContainer from all entries */
	FGameplayTagContainer ToContainer() const
	{
		FGameplayTagContainer Container;
		for (const FGameplayTagEntry& Entry : GameplayTags)
		{
			if (Entry.Tag.IsValid())
			{
				Container.AddTagFast(Entry.Tag);
			}
		}
		return Container;
	}

	/** Helper: number of tag entries */
	int32 Num() const { return GameplayTags.Num(); }
};

/**
 * AssetUserData that stores GameplayTagContainer data for SkeletalMesh material slots.
 * 
 * This data is serialized with the mesh and can be read by UAssetTool to inject
 * the tags into the FSkeletalMaterial::GameplayTagContainer field during mod creation.
 * 
 * Marvel Rivals uses these tags for material visibility control (e.g., hiding weapons).
 * 
 * Usage:
 * 1. Open your SkeletalMesh in the editor
 * 2. In Details panel, find "Asset User Data" array
 * 3. Click + to add, select "Material Tag Data"
 * 4. Click "Populate From Mesh" to auto-create entries
 * 5. For each material slot that needs tags, add the appropriate GameplayTags
 * 6. Save the mesh
 * 
 * Common Marvel Rivals MaterialTags:
 * - MaterialTag.装备.武器 (Equipment.Weapon)
 * - MaterialTag.装备 (Equipment)
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta=(DisplayName="Material Tag Data"))
class MATERIALTAGPLUGIN_API UMaterialTagAssetUserData : public UAssetUserData
{
	GENERATED_BODY()

public:
	UMaterialTagAssetUserData();

	/**
	 * If true, automatically selects the preset matching the mesh name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	bool bAutoMatchPreset = false;

	/**
	 * Select a mesh preset to see which tags belong to which slots.
	 * Populated from Config/MaterialTagPresets.ini.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset", meta=(GetOptions="GetPresetMeshNames", EditCondition="!bAutoMatchPreset"))
	FString PresetMeshName;

	/**
	 * Displays draggable tag pills for the selected preset.
	 * Drag tags onto material slot entries below to assign them.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preset")
	FPresetTagDisplay PresetTags;

	/**
	 * Array of slot-tag pairs.
	 * Each entry maps one material slot to its tag sub-array.
	 * Click + on the GameplayTags sub-array to add more tags per slot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Tags", meta=(TitleProperty="MaterialSlotName"))
	TArray<FMaterialSlotTagEntry> MaterialSlotTags;

	/**
	 * Populate the MaterialSlotTags array from the current mesh materials.
	 * Creates an entry for each material slot with empty tags.
	 */
	UFUNCTION(BlueprintCallable, Category = "Material Tags", meta=(CallInEditor="true"))
	void PopulateFromMesh();

	/**
	 * Ensure all mesh material slots have entries (adds missing ones, keeps existing).
	 */
	void EnsureAllSlotsPopulated();

	/**
	 * Get all tags for a specific material slot (collected from all matching entries).
	 */
	UFUNCTION(BlueprintCallable, Category = "Material Tags")
	FGameplayTagContainer GetTagsForSlot(FName SlotName) const;

	/**
	 * Check if a slot has any tags assigned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Material Tags")
	bool HasTagsForSlot(FName SlotName) const;

	/** Returns list of mesh names from the preset INI (for GetOptions dropdown) */
	UFUNCTION()
	TArray<FString> GetPresetMeshNames() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
#endif

private:
	/** Load preset info text for the given mesh name from the INI */
	void UpdatePresetInfo();

	/** Auto-match: find the best preset name matching the owning mesh */
	void AutoMatchPresetFromMesh();

	/** Get the path to the preset INI file */
	static FString GetPresetIniPath();
};
