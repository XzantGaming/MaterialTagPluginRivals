#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "HiddenMaterialDefaultsAssetUserData.generated.h"

/**
 * A single material slot's default-hidden state for one LOD.
 *
 * Maps directly to one entry of the SkeletalMesh's per-LOD DefaultHiddenMaterials
 * boolean array: false = visible, true = hidden.
 *
 * Rendered by FHiddenMaterialEntryCustomization as a slot name + checkbox
 * whose label shows "Visible" or "Hidden" based on bHidden.
 */
USTRUCT(BlueprintType)
struct RIVALSMESHMATERIALMANAGER_API FHiddenMaterialEntry
{
	GENERATED_BODY()

	/** Name of the material slot (display/reference only; order matches the mesh material array) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hidden Material Defaults")
	FName MaterialSlotName;

	/** false = visible (default), true = hidden */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hidden Material Defaults")
	bool bHidden = false;
};

/**
 * Hidden-material defaults for a single LOD of the mesh.
 * The Materials array is ordered to match the mesh's material slots, so it
 * serializes 1:1 with the DefaultHiddenMaterials bool array for that LOD.
 */
USTRUCT(BlueprintType)
struct RIVALSMESHMATERIALMANAGER_API FHiddenMaterialLOD
{
	GENERATED_BODY()

	/** LOD index this entry corresponds to */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hidden Material Defaults")
	int32 LODIndex = 0;

	/**
	 * One entry per material slot, in mesh material order.
	 * EditFixedSize: the array size is driven by the mesh's material count and
	 * cannot be edited manually (no +/- buttons).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hidden Material Defaults", meta=(TitleProperty="MaterialSlotName", EditFixedSize))
	TArray<FHiddenMaterialEntry> Materials;
};

/**
 * AssetUserData that stores per-LOD default hidden material flags for a SkeletalMesh.
 *
 * Marvel Rivals stores a DefaultHiddenMaterials boolean array per LOD (inside each
 * SkeletalMeshLODInfo) where false = visible and true = hidden. This data is serialized
 * with the mesh and read by UAssetTool to inject the flags during mod creation.
 *
 * Usage:
 * 1. Open your SkeletalMesh in the editor
 * 2. In the Details panel, find the "Asset User Data" array
 * 3. Click + and select "Hidden Material Defaults"
 * 4. The plugin auto-populates one entry per LOD, each with one
 *    Visible/Hidden checkbox per material slot
 * 5. Toggle the checkboxes that should default to hidden, then save the mesh
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta=(DisplayName="Hidden Material Defaults"))
class RIVALSMESHMATERIALMANAGER_API UHiddenMaterialDefaultsAssetUserData : public UAssetUserData
{
	GENERATED_BODY()

public:
	UHiddenMaterialDefaultsAssetUserData();

	/**
	 * Per-LOD hidden material defaults.
	 * One element per mesh LOD; each holds one toggle per material slot.
	 * EditFixedSize: size is driven by the mesh's LOD count (auto-populated).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hidden Material Defaults", meta=(EditFixedSize))
	TArray<FHiddenMaterialLOD> LODs;

	/**
	 * Rebuild the LODs array from the current mesh: one entry per LOD, each with
	 * one Visible/Hidden toggle per material slot. Preserves existing hidden flags
	 * where slot names still match.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hidden Material Defaults", meta=(CallInEditor="true"))
	void PopulateFromMesh();

	/** Ensure the LODs/material entries match the mesh (adds missing, keeps existing flags). */
	void EnsurePopulated();

	/** Ensure a specific LOD's materials array matches the mesh's material count. */
	void EnsureLODMaterialsPopulated(int32 LODIndex);

	/** Returns the hidden-flag array for a given LOD (false = visible, true = hidden). */
	UFUNCTION(BlueprintCallable, Category = "Hidden Material Defaults")
	TArray<bool> GetHiddenFlagsForLOD(int32 LODIndex) const;

#if WITH_EDITOR
	virtual void PostLoad() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
