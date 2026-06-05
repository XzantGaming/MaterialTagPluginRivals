#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"

class IDetailLayoutBuilder;

/**
 * Detail customization for UMaterialTagAssetUserData.
 * Hides add/delete/insert/duplicate on the MaterialSlotTags array
 * by manually building the array display without array manipulation widgets.
 */
class FMaterialTagAssetUserDataDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Cached objects being edited */
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingEdited;
};

#endif // WITH_EDITOR
