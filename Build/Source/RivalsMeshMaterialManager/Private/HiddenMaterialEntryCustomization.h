#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class FDetailWidgetRow;
class IDetailChildrenBuilder;

/**
 * Custom property type customization for FHiddenMaterialEntry.
 *
 * Renders the material slot name plus a button that toggles between "Visible" and "Hidden".
 */
class FHiddenMaterialEntryCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	bool IsHidden() const;
	FText GetSlotDisplayName() const;
	FText GetButtonText() const;
	FReply OnToggleClicked();

	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<IPropertyHandle> SlotNameHandle;
	TSharedPtr<IPropertyHandle> HiddenHandle;
};

#endif // WITH_EDITOR
