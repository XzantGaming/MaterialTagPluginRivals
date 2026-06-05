#if WITH_EDITOR

#include "HiddenMaterialDefaultsCustomizations.h"
#include "HiddenMaterialDefaultsAssetUserData.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "HiddenMaterialDefaultsCustomizations"

// ---------- FHiddenMaterialLODCustomization ----------

TSharedRef<IPropertyTypeCustomization> FHiddenMaterialLODCustomization::MakeInstance()
{
	return MakeShareable(new FHiddenMaterialLODCustomization());
}

void FHiddenMaterialLODCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;
	LODIndexHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHiddenMaterialLOD, LODIndex));

	// Ensure materials are populated for this LOD when the header is customized
	EnsureLODMaterialsPopulated();

	HeaderRow
		.NameContent()
		[
			SNew(STextBlock)
			.Text(this, &FHiddenMaterialLODCustomization::GetHeaderText)
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		];
}

void FHiddenMaterialLODCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Hide LODIndex (it's already shown in the header) and only expose Materials.
	TSharedPtr<IPropertyHandle> MaterialsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHiddenMaterialLOD, Materials));
	if (MaterialsHandle.IsValid())
	{
		ChildBuilder.AddProperty(MaterialsHandle.ToSharedRef());
	}
}

void FHiddenMaterialLODCustomization::EnsureLODMaterialsPopulated()
{
	if (!StructHandle.IsValid())
	{
		return;
	}

	// Get the outer UHiddenMaterialDefaultsAssetUserData
	TArray<UObject*> OuterObjects;
	StructHandle->GetOuterObjects(OuterObjects);

	for (UObject* Outer : OuterObjects)
	{
		if (UHiddenMaterialDefaultsAssetUserData* UserData = Cast<UHiddenMaterialDefaultsAssetUserData>(Outer))
		{
			// Force populate on the user data - this will create materials array entries
			UserData->EnsurePopulated();
			break;
		}
	}
}

FText FHiddenMaterialLODCustomization::GetHeaderText() const
{
	int32 LODIndex = 0;
	if (LODIndexHandle.IsValid())
	{
		LODIndexHandle->GetValue(LODIndex);
	}
	else if (StructHandle.IsValid())
	{
		// Fallback: derive from array index if the child handle isn't available
		const int32 ArrIdx = StructHandle->GetIndexInArray();
		if (ArrIdx != INDEX_NONE)
		{
			LODIndex = ArrIdx;
		}
	}
	return FText::FromString(FString::Printf(TEXT("LOD %d"), LODIndex));
}

// ---------- FHiddenMaterialDefaultsDetailCustomization ----------

TSharedRef<IDetailCustomization> FHiddenMaterialDefaultsDetailCustomization::MakeInstance()
{
	return MakeShareable(new FHiddenMaterialDefaultsDetailCustomization());
}

void FHiddenMaterialDefaultsDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Ensure the arrays match the mesh's current LOD/material count whenever the
	// details panel is built for this object. This is what makes the lists
	// "dynamic to the amount of materials the mesh actually has".
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	bool bNeedsRefresh = false;
	for (const TWeakObjectPtr<UObject>& Weak : Objects)
	{
		if (UHiddenMaterialDefaultsAssetUserData* UD = Cast<UHiddenMaterialDefaultsAssetUserData>(Weak.Get()))
		{
			// Start a transaction so the populate can be undone
			FScopedTransaction Transaction(LOCTEXT("PopulateMaterials", "Populate Hidden Materials"));
			UD->Modify();
			UD->EnsurePopulated();
			bNeedsRefresh = true;
		}
	}

	// Force the detail builder to rebuild if we populated data
	if (bNeedsRefresh)
	{
		DetailBuilder.ForceRefreshDetails();
	}
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
