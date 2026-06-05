#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"
#include "GameplayTagContainer.h"

class IPropertyHandle;
class FDetailWidgetRow;
class IDetailChildrenBuilder;
class SVerticalBox;

/**
 * Custom property type customization for FMaterialSlotTagEntry.
 * Shows slot name (read-only) + pill-shaped tags with X buttons. Accepts tag pill drops.
 */
class FMaterialSlotTagEntryCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	/** Get the slot name from the property handle (delegate-bound) */
	FText GetSlotDisplayName() const;

	/** Add a gameplay tag by name to this slot's GameplayTags array */
	void AddTagToSlot(const FString& TagName);

	/** Remove a gameplay tag by name from this slot's GameplayTags array */
	void RemoveTagFromSlot(const FString& TagName);

	/** Rebuild the pill widgets in the tag box */
	void RebuildTagPills();

	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyHandle> SlotNameHandle;
	TSharedPtr<IPropertyHandle> TagsHandle;

	/** Cached property utilities for forcing refresh */
	TSharedPtr<IPropertyUtilities> PropertyUtilities;

	/** Vertical box holding removable tag pills (one per line) */
	TSharedPtr<SVerticalBox> TagPillBox;
};

#endif // WITH_EDITOR
