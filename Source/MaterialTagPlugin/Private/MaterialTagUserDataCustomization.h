#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class FDetailWidgetRow;
class IDetailChildrenBuilder;
class UMaterialTagAssetUserData;

/** One slot in the full table: index, name, tag(s) */
struct FPresetSlotInfo
{
	int32 Index;
	FString SlotName;
	FString Tags; // comma-separated, empty if none
};

/**
 * Custom property type customization for FPresetTagDisplay.
 * Shows a full slot table (index, name, tag) from the mesh + INI, plus draggable tag pills.
 */
class FPresetTagDisplayCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	/** Parse the INI to get tag->slot(s) map for the given mesh */
	static TMap<FString, TArray<FString>> GetTagToSlotsMap(const FString& MeshName);

	/** Read the full ordered slot list (Slot_N keys) from the INI for the given mesh */
	static TArray<FString> GetPresetSlotList(const FString& MeshName);

	/** Build the full slot table from the mesh materials + INI tag data */
	static void BuildSlotTable(UMaterialTagAssetUserData* UserData, const FString& MeshName, TArray<FPresetSlotInfo>& OutSlots, TSet<FString>& OutUniqueTags);

	/** Get the path to the preset INI file */
	static FString GetPresetIniPath();

	/** Find the PresetMeshName from the parent UMaterialTagAssetUserData */
	FString GetPresetMeshName() const;

	/** Find the UMaterialTagAssetUserData from the property handle chain */
	UMaterialTagAssetUserData* GetUserData() const;

	TSharedPtr<IPropertyHandle> StructHandle;
};

#endif // WITH_EDITOR
