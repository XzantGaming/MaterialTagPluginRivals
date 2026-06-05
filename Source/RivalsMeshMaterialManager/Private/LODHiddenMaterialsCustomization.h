#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class IPropertyHandle;
class FDetailWidgetRow;
class IDetailChildrenBuilder;

/**
 * Custom property type customization for FLODHiddenMaterials.
 * Renders "LOD N" header + per-material-slot checkboxes.
 */
class FLODHiddenMaterialsCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	TSharedPtr<IPropertyHandle> StructPropertyHandle;
};

#endif // WITH_EDITOR
