#if WITH_EDITOR

#include "MaterialSlotTagEntryCustomization.h"
#include "MaterialTagAssetUserData.h"
#include "MaterialTagDragDrop.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAssetCommon.h"
#include "GameplayTagsManager.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "MaterialSlotTagEntryCustomization"

TSharedRef<IPropertyTypeCustomization> FMaterialSlotTagEntryCustomization::MakeInstance()
{
	return MakeShareable(new FMaterialSlotTagEntryCustomization());
}

void FMaterialSlotTagEntryCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;
	SlotNameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMaterialSlotTagEntry, MaterialSlotName));
	TagsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMaterialSlotTagEntry, GameplayTags));
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	// Build tag pill vertical box
	TagPillBox = SNew(SVerticalBox);
	RebuildTagPills();

	auto Self = this;

	// [SlotName read-only, delegate-bound] | [Tag pills vertically stacked + drop zone]
	HeaderRow
		.NameContent()
		[
			SNew(STagDropTarget)
			.OnTagDropped_Lambda([Self](const FString& TagName)
			{
				Self->AddTagToSlot(TagName);
			})
			[
				SNew(STextBlock)
				.Text(this, &FMaterialSlotTagEntryCustomization::GetSlotDisplayName)
			]
		]
		.ValueContent()
		.MinDesiredWidth(300.0f)
		[
			SNew(STagDropTarget)
			.OnTagDropped_Lambda([Self](const FString& TagName)
			{
				Self->AddTagToSlot(TagName);
			})
			[
				TagPillBox.ToSharedRef()
			]
		];
}

void FMaterialSlotTagEntryCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// No children — tags are shown as pills in the header with X buttons
}

void FMaterialSlotTagEntryCustomization::RebuildTagPills()
{
	if (!TagPillBox.IsValid() || !TagsHandle.IsValid()) return;

	TagPillBox->ClearChildren();

	TSharedPtr<IPropertyHandleArray> ArrayHandle = TagsHandle->AsArray();
	if (!ArrayHandle.IsValid()) return;

	uint32 NumElements = 0;
	ArrayHandle->GetNumElements(NumElements);

	if (NumElements == 0)
	{
		TagPillBox->AddSlot()
		.AutoHeight()
		.Padding(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DropHere", "Drop tags here..."))
			.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f))
		];
		return;
	}

	auto Self = this;

	for (uint32 i = 0; i < NumElements; i++)
	{
		TSharedRef<IPropertyHandle> ElementHandle = ArrayHandle->GetElement(i);
		TSharedPtr<IPropertyHandle> TagFieldHandle = ElementHandle->GetChildHandle(TEXT("Tag"));
		if (!TagFieldHandle.IsValid()) continue;

		TSharedPtr<IPropertyHandle> TagNameHandle = TagFieldHandle->GetChildHandle(TEXT("TagName"));
		if (!TagNameHandle.IsValid()) continue;

		FName TagFName;
		TagNameHandle->GetValue(TagFName);
		if (TagFName.IsNone()) continue;

		FString TagStr = TagFName.ToString();

		TagPillBox->AddSlot()
		.AutoHeight()
		.Padding(1.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SRemovableTagPill)
				.TagName(TagStr)
				.OnRemove_Lambda([Self](const FString& RemovedTag)
				{
					Self->RemoveTagFromSlot(RemovedTag);
				})
			]
		];
	}
}

FText FMaterialSlotTagEntryCustomization::GetSlotDisplayName() const
{
	if (!SlotNameHandle.IsValid()) return FText::FromString(TEXT("(unset)"));

	FName SlotFName;
	SlotNameHandle->GetValue(SlotFName);
	FString SlotStr = SlotFName.IsNone() ? TEXT("(unset)") : SlotFName.ToString();

	// Get array index from the struct property handle
	if (StructPropertyHandle.IsValid())
	{
		int32 Index = StructPropertyHandle->GetIndexInArray();
		if (Index != INDEX_NONE)
		{
			return FText::FromString(FString::Printf(TEXT("%d  %s"), Index, *SlotStr));
		}
	}

	return FText::FromString(SlotStr);
}

void FMaterialSlotTagEntryCustomization::AddTagToSlot(const FString& TagName)
{
	if (!TagsHandle.IsValid()) return;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
	if (!Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MaterialSlotTagEntry: Tag '%s' not found"), *TagName);
		return;
	}

	TSharedPtr<IPropertyHandleArray> ArrayHandle = TagsHandle->AsArray();
	if (!ArrayHandle.IsValid()) return;

	// Check for duplicates
	uint32 NumElements = 0;
	ArrayHandle->GetNumElements(NumElements);
	for (uint32 i = 0; i < NumElements; i++)
	{
		TSharedRef<IPropertyHandle> ElementHandle = ArrayHandle->GetElement(i);
		TSharedPtr<IPropertyHandle> TagFieldHandle = ElementHandle->GetChildHandle(TEXT("Tag"));
		if (TagFieldHandle.IsValid())
		{
			TSharedPtr<IPropertyHandle> TagNameHandle = TagFieldHandle->GetChildHandle(TEXT("TagName"));
			if (TagNameHandle.IsValid())
			{
				FName ExistingTagName;
				TagNameHandle->GetValue(ExistingTagName);
				if (ExistingTagName == FName(*TagName)) return;
			}
		}
	}

	FScopedTransaction Transaction(LOCTEXT("AddTagFromPreset", "Add Tag From Preset"));
	ArrayHandle->AddItem();

	uint32 NewCount = 0;
	ArrayHandle->GetNumElements(NewCount);
	if (NewCount == 0) return;

	TSharedRef<IPropertyHandle> NewElement = ArrayHandle->GetElement(NewCount - 1);
	TSharedPtr<IPropertyHandle> TagFieldHandle = NewElement->GetChildHandle(TEXT("Tag"));
	if (TagFieldHandle.IsValid())
	{
		TSharedPtr<IPropertyHandle> TagNameFieldHandle = TagFieldHandle->GetChildHandle(TEXT("TagName"));
		if (TagNameFieldHandle.IsValid())
		{
			TagNameFieldHandle->SetValue(FName(*TagName));
		}
	}

	if (PropertyUtilities.IsValid())
	{
		PropertyUtilities->ForceRefresh();
	}
}

void FMaterialSlotTagEntryCustomization::RemoveTagFromSlot(const FString& TagName)
{
	if (!TagsHandle.IsValid()) return;

	TSharedPtr<IPropertyHandleArray> ArrayHandle = TagsHandle->AsArray();
	if (!ArrayHandle.IsValid()) return;

	uint32 NumElements = 0;
	ArrayHandle->GetNumElements(NumElements);

	for (int32 i = (int32)NumElements - 1; i >= 0; i--)
	{
		TSharedRef<IPropertyHandle> ElementHandle = ArrayHandle->GetElement(i);
		TSharedPtr<IPropertyHandle> TagFieldHandle = ElementHandle->GetChildHandle(TEXT("Tag"));
		if (!TagFieldHandle.IsValid()) continue;

		TSharedPtr<IPropertyHandle> TagNameHandle = TagFieldHandle->GetChildHandle(TEXT("TagName"));
		if (!TagNameHandle.IsValid()) continue;

		FName ExistingTagName;
		TagNameHandle->GetValue(ExistingTagName);
		if (ExistingTagName == FName(*TagName))
		{
			FScopedTransaction Transaction(LOCTEXT("RemoveTag", "Remove Tag"));
			ArrayHandle->DeleteItem(i);
			break;
		}
	}

	if (PropertyUtilities.IsValid())
	{
		PropertyUtilities->ForceRefresh();
	}
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
