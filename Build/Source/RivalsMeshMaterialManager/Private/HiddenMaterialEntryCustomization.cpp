#if WITH_EDITOR

#include "HiddenMaterialEntryCustomization.h"
#include "HiddenMaterialDefaultsAssetUserData.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "ScopedTransaction.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "HiddenMaterialEntryCustomization"

TSharedRef<IPropertyTypeCustomization> FHiddenMaterialEntryCustomization::MakeInstance()
{
	return MakeShareable(new FHiddenMaterialEntryCustomization());
}

void FHiddenMaterialEntryCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;
	SlotNameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHiddenMaterialEntry, MaterialSlotName));
	HiddenHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHiddenMaterialEntry, bHidden));

	HeaderRow
		.NameContent()
		[
			SNew(STextBlock)
			.Text(this, &FHiddenMaterialEntryCustomization::GetSlotDisplayName)
			.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
		]
		.ValueContent()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.ToolTipText(LOCTEXT("ToggleTooltip", "Click to toggle this material between Visible and Hidden"))
			.OnClicked(this, &FHiddenMaterialEntryCustomization::OnToggleClicked)
			.ContentPadding(FMargin(10.0f, 0.0f))
			[
				SNew(STextBlock)
				.Text(this, &FHiddenMaterialEntryCustomization::GetButtonText)
				.Font(FAppStyle::GetFontStyle("PropertyWindow.BoldFont"))
			]
		];
}

void FHiddenMaterialEntryCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// No children — the button in the header represents the whole entry.
}

bool FHiddenMaterialEntryCustomization::IsHidden() const
{
	bool bHidden = false;
	if (HiddenHandle.IsValid())
	{
		HiddenHandle->GetValue(bHidden);
	}
	return bHidden;
}

FText FHiddenMaterialEntryCustomization::GetSlotDisplayName() const
{
	FString SlotStr(TEXT("(unset)"));
	if (SlotNameHandle.IsValid())
	{
		FName SlotFName;
		SlotNameHandle->GetValue(SlotFName);
		if (!SlotFName.IsNone())
		{
			SlotStr = SlotFName.ToString();
		}
	}

	if (StructHandle.IsValid())
	{
		const int32 Index = StructHandle->GetIndexInArray();
		if (Index != INDEX_NONE)
		{
			return FText::FromString(FString::Printf(TEXT("%d  %s"), Index, *SlotStr));
		}
	}
	return FText::FromString(SlotStr);
}

FText FHiddenMaterialEntryCustomization::GetButtonText() const
{
	return IsHidden() ? LOCTEXT("Hidden", "Hidden") : LOCTEXT("Visible", "Visible");
}

FReply FHiddenMaterialEntryCustomization::OnToggleClicked()
{
	if (!HiddenHandle.IsValid())
	{
		return FReply::Handled();
	}

	const bool bCurrentValue = IsHidden();
	const bool bNewValue = !bCurrentValue;

	FScopedTransaction Transaction(LOCTEXT("ToggleHidden", "Toggle Material Visibility"));

	TArray<UObject*> OuterObjects;
	HiddenHandle->GetOuterObjects(OuterObjects);

	for (UObject* Outer : OuterObjects)
	{
		if (Outer)
		{
			Outer->Modify();
		}
	}

	// Direct memory write for performance (avoids expensive PostEditChange cascade)
	TArray<void*> RawDataPtrs;
	HiddenHandle->AccessRawData(RawDataPtrs);

	for (void* RawPtr : RawDataPtrs)
	{
		if (RawPtr)
		{
			*static_cast<bool*>(RawPtr) = bNewValue;
		}
	}

	for (UObject* Outer : OuterObjects)
	{
		if (Outer)
		{
			Outer->MarkPackageDirty();
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
