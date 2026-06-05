#if WITH_EDITOR

#include "MaterialTagAssetUserDataDetails.h"
#include "MaterialTagAssetUserData.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

TSharedRef<IDetailCustomization> FMaterialTagAssetUserDataDetails::MakeInstance()
{
	return MakeShareable(new FMaterialTagAssetUserDataDetails());
}

void FMaterialTagAssetUserDataDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingEdited);

	// Get the MaterialSlotTags array property
	TSharedRef<IPropertyHandle> SlotTagsHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UMaterialTagAssetUserData, MaterialSlotTags));

	// Hide the default array rendering (which has add/delete/reorder)
	DetailBuilder.HideProperty(SlotTagsHandle);

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Material Tags");

	// Add a custom header row for the array
	uint32 NumElements = 0;
	TSharedPtr<IPropertyHandleArray> ArrayHandle = SlotTagsHandle->AsArray();
	if (ArrayHandle.IsValid())
	{
		ArrayHandle->GetNumElements(NumElements);
	}

	Category.AddCustomRow(NSLOCTEXT("MaterialTagDetails", "SlotTagsHeader", "Material Slot Tags"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("MaterialTagDetails", "SlotTagsLabel", "Material Slot Tags"))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%d slots"), NumElements)))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];

	// Add each element individually — this bypasses the array header with +/trash
	// and the per-element Insert/Delete/Duplicate context menu
	for (uint32 i = 0; i < NumElements; i++)
	{
		TSharedRef<IPropertyHandle> ElementHandle = ArrayHandle->GetElement(i);
		// Adding individual struct elements directly — they get their
		// IPropertyTypeCustomization (FMaterialSlotTagEntryCustomization) applied
		// but without array manipulation UI
		Category.AddProperty(ElementHandle);
	}
}

#endif // WITH_EDITOR
