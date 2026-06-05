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
struct RIVALSMESHMATERIALMANAGER_API FGameplayTagEntry
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
struct RIVALSMESHMATERIALMANAGER_API FPresetTagDisplay
{
	GENERATED_BODY()

	/** Serialized info text (fallback display) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preset", meta=(MultiLine="true"))
	FString InfoText;
};

/**
 * Entry for a single material slot's gameplay tags.
 * Each slot maps to a sub-array of FGameplayTagEntry wrappers.
 * Tags are wrapped individually to prevent UE's automatic tag hierarchy merging.
 */
USTRUCT(BlueprintType)
struct RIVALSMESHMATERIALMANAGER_API FMaterialSlotTagEntry
{
	GENERATED_BODY()

	/** Material slot name (auto-populated from mesh, read-only in the UI) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Tags")
	FName MaterialSlotName;

	/** Gameplay tags assigned to this slot (drag from preset area or edit manually) */
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
 * AssetUserData that stores per-slot GameplayTag assignments for SkeletalMesh materials.
 * 
 * Serialized with the mesh asset. UAssetTool reads this data to inject tags into
 * FSkeletalMaterial::GameplayTagContainer during mod creation (--material-tags flag).
 * Marvel Rivals uses these tags for material visibility control (e.g., hiding weapons).
 * 
 * Usage:
 * 1. Open a SkeletalMesh in the editor
 * 2. In Details panel, find "Asset User Data" array
 * 3. Click + to add, select "Material Tag Data"
 * 4. Material slots are auto-populated from the mesh on load
 * 5. Select a preset to see reference tags, then drag tag pills onto slots
 * 6. Use "Refresh Material Slots" if new materials are added to the mesh
 * 7. Save the mesh
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta=(DisplayName="Material Tag Data"))
class RIVALSMESHMATERIALMANAGER_API UMaterialTagAssetUserData : public UAssetUserData
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
	 * If true, automatically populates material slot tags from the selected preset (by slot index).
	 * Ignores slot names - maps Slot_N from the INI directly to MaterialSlotTags[N].
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	bool bAutoPopulateTags = false;

	/**
	 * Select a mesh preset to see which tags belong to which slots.
	 * Populated from <Project>/Config/MaterialTagPresets.ini.
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
	 * Per-slot tag assignments (1:1 with mesh material indices).
	 * Drag tags from the preset area above, or expand a slot to edit tags manually.
	 * Slots are managed automatically — use "Refresh Material Slots" to sync with the mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material Tags", meta=(TitleProperty="MaterialSlotName"))
	TArray<FMaterialSlotTagEntry> MaterialSlotTags;

	/**
	 * Refresh the material slot list from the current mesh.
	 * Adds missing slots, keeps existing tag data intact.
	 */
	UFUNCTION(BlueprintCallable, Category = "Material Tags", meta=(CallInEditor="true"))
	void RefreshSlots();

	/**
	 * Reset and repopulate MaterialSlotTags from the mesh (clears all existing tag data).
	 */
	UFUNCTION(BlueprintCallable, Category = "Material Tags", meta=(CallInEditor="true"))
	void PopulateFromMesh();

	/**
	 * Rebuild the slot array to be 1:1 with the mesh's material list.
	 * Preserves existing tag data for matching slots.
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
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
#endif

private:
#if WITH_EDITOR
	/** Saved copy of MaterialSlotTags before any edit, to guard against user add/delete */
	TArray<FMaterialSlotTagEntry> SavedSlotTags;
#endif

	/** Load preset info text for the given mesh name from the INI */
	void UpdatePresetInfo();

	/** Auto-match: find the best preset name matching the owning mesh */
	void AutoMatchPresetFromMesh();

	/** Apply preset tags to slots by index (ignores slot names, uses Slot_N index from INI) */
	void ApplyPresetTagsByIndex();

	/** Get the path to the preset INI file */
	static FString GetPresetIniPath();
};
