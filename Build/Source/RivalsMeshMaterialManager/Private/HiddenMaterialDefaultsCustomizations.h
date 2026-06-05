#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"

class FDetailWidgetRow;
class IDetailChildrenBuilder;
class IDetailLayoutBuilder;

/**
 * Custom property type customization for FHiddenMaterialLOD.
 * Renders the LOD index as a header (e.g., "LOD 0") instead of generic array index.
 */
class FHiddenMaterialLODCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	void EnsureLODMaterialsPopulated();
	FText GetHeaderText() const;

	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<IPropertyHandle> LODIndexHandle;
};

/**
 * Class detail customization for UHiddenMaterialDefaultsAssetUserData.
 * Ensures LODs/Materials arrays are auto-populated from the mesh when the details panel opens.
 */
class FHiddenMaterialDefaultsDetailCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

#endif // WITH_EDITOR
