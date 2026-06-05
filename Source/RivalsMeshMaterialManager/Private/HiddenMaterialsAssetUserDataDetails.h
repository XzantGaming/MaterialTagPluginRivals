#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"
#include "Input/Reply.h"

class IDetailLayoutBuilder;
class IDetailCategoryBuilder;
class UHiddenMaterialsAssetUserData;

/**
 * Detail customization for UHiddenMaterialsAssetUserData.
 * Renders a LOD selector + per-slot checkbox grid for DefaultHiddenMaterials.
 */
class FHiddenMaterialsAssetUserDataDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
	int32 SelectedLODIndex = 0;

	void RebuildLODSelector(IDetailCategoryBuilder& Category, UHiddenMaterialsAssetUserData* Data);
	void RebuildSlotGrid(IDetailCategoryBuilder& Category, UHiddenMaterialsAssetUserData* Data);
	FReply OnToggleHidden(int32 SlotIndex, UHiddenMaterialsAssetUserData* Data);
	void OnCopyLOD0ToAll(UHiddenMaterialsAssetUserData* Data);
	void OnApplyPreset(UHiddenMaterialsAssetUserData* Data);
};

#endif // WITH_EDITOR
