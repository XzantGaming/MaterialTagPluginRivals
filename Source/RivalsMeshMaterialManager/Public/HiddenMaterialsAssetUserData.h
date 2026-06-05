#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "HiddenMaterialsAssetUserData.generated.h"

/**
 * Per-LOD hidden material flags.
 * Index = material slot; true = hidden by default at this LOD.
 */
USTRUCT(BlueprintType)
struct RIVALSMESHMATERIALMANAGER_API FLODHiddenMaterials
{
	GENERATED_BODY()

	/** The LOD index this entry represents (auto-set during sync). */
	UPROPERTY(VisibleAnywhere)
	int32 LODIndex = 0;

	/** Per-slot hidden flags. Stored as uint8 to avoid UE's special TArray<bool> inline checkbox rendering. */
	UPROPERTY(EditAnywhere, meta = (Hidden))
	TArray<uint8> HiddenMaterials;
};

/**
 * AssetUserData that stores per-LOD DefaultHiddenMaterials for SkeletalMesh assets.
 *
 * Serialized with the mesh asset. UAssetTool reads this data and injects
 * the bool array into FSkeletalMeshLODInfo::DefaultHiddenMaterials during Zen conversion.
 */
UCLASS(BlueprintType, EditInlineNew, CollapseCategories, meta = (DisplayName = "Hidden Materials Data"))
class RIVALSMESHMATERIALMANAGER_API UHiddenMaterialsAssetUserData : public UAssetUserData
{
	GENERATED_BODY()

public:
	UHiddenMaterialsAssetUserData();

	/**
	 * If true, automatically selects the preset matching the mesh name.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	bool bAutoMatchPreset = false;

	/**
	 * Select a mesh preset to load hidden-material flags from.
	 * Populated from <Project>/Config/MaterialTagPresets.ini.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset", meta = (GetOptions = "GetPresetMeshNames", EditCondition = "!bAutoMatchPreset"))
	FString PresetMeshName;

	/**
	 * Per-LOD default hidden material flags.
	 * True = hidden by default. Array length should match mesh LOD count.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hidden Materials")
	TArray<FLODHiddenMaterials> LODHiddenMaterials;

	/**
	 * Refresh the hidden-materials arrays to match the current mesh state.
	 * Preserves existing flags where indices still align.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hidden Materials", meta = (CallInEditor = "true"))
	void RefreshSlots();

	/** Returns list of mesh names from the preset INI (for GetOptions dropdown) */
	UFUNCTION()
	TArray<FString> GetPresetMeshNames() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
#endif

	/** Resize LODHiddenMaterials to match the mesh's LOD and material counts */
	void EnsureHiddenMaterialsSynced();

	/** Read LOD_N_Hidden lines from the preset INI and populate LODHiddenMaterials */
	void ApplyPresetHiddenMaterials();

	/** Read preset hidden flags for a specific LOD without modifying current data */
	TArray<uint8> GetPresetHiddenFlagsForLOD(int32 LODIndex) const;

private:
	/** Auto-match: find the best preset name matching the owning mesh */
	void AutoMatchPresetFromMesh();

	/** Get the path to the preset INI file */
	static FString GetPresetIniPath();
};
