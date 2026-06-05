#if WITH_EDITOR

#include "LODHiddenMaterialsCustomization.h"
#include "HiddenMaterialsAssetUserData.h"
#include "Engine/SkeletalMesh.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "LODHiddenMaterialsCustomization"

TSharedRef<IPropertyTypeCustomization> FLODHiddenMaterialsCustomization::MakeInstance()
{
	return MakeShareable(new FLODHiddenMaterialsCustomization());
}

/** Get the raw FLODHiddenMaterials pointer from the property handle. */
static FLODHiddenMaterials* GetRawStructData(TSharedRef<IPropertyHandle> PropertyHandle)
{
	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	return (RawData.Num() > 0) ? (FLODHiddenMaterials*)RawData[0] : nullptr;
}

/** Find the owning UHiddenMaterialsAssetUserData from the property handle. */
static UHiddenMaterialsAssetUserData* GetOwnerAssetUserData(TSharedRef<IPropertyHandle> PropertyHandle)
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);
	for (UObject* Obj : OuterObjects)
	{
		if (UHiddenMaterialsAssetUserData* Data = Cast<UHiddenMaterialsAssetUserData>(Obj))
		{
			return Data;
		}
		if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Obj))
		{
			const TArray<UAssetUserData*>* UserDataArray = Mesh->GetAssetUserDataArray();
			if (UserDataArray)
			{
				for (UAssetUserData* UserData : *UserDataArray)
				{
					if (UHiddenMaterialsAssetUserData* Data = Cast<UHiddenMaterialsAssetUserData>(UserData))
					{
						return Data;
					}
				}
			}
		}
	}
	return nullptr;
}

void FLODHiddenMaterialsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructPropertyHandle = PropertyHandle;

	FLODHiddenMaterials* StructData = GetRawStructData(PropertyHandle);
	int32 LODIndex = StructData ? StructData->LODIndex : 0;
	int32 SlotCount = StructData ? StructData->HiddenMaterials.Num() : 0;

	UHiddenMaterialsAssetUserData* Data = GetOwnerAssetUserData(PropertyHandle);

	// Ensure arrays are sized correctly before we render
	if (Data)
	{
		Data->EnsureHiddenMaterialsSynced();
		StructData = GetRawStructData(PropertyHandle);
		LODIndex = StructData ? StructData->LODIndex : LODIndex;
		SlotCount = StructData ? StructData->HiddenMaterials.Num() : SlotCount;
	}

	HeaderRow
		.NameContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("LOD %d"), LODIndex)))
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Visibility((LODIndex == 0) ? EVisibility::Visible : EVisibility::Collapsed)
				.Text_Lambda([Data]() -> FText
				{
					if (Data && !Data->PresetMeshName.IsEmpty())
					{
						return FText::Format(LOCTEXT("ApplyPresetBtnActive", "Apply: {0}"), FText::FromString(Data->PresetMeshName));
					}
					return LOCTEXT("ApplyPresetBtnInactive", "No preset");
				})
				.ButtonColorAndOpacity_Lambda([Data]() -> FLinearColor
				{
					if (Data && !Data->PresetMeshName.IsEmpty())
					{
						return FLinearColor(0.2f, 0.7f, 0.3f, 1.0f); // Green when active
					}
					return FLinearColor(0.3f, 0.3f, 0.3f, 1.0f); // Gray when inactive
				})
				.OnClicked_Lambda([Data]() -> FReply
				{
					if (Data && !Data->PresetMeshName.IsEmpty())
					{
						FScopedTransaction Transaction(LOCTEXT("ApplyPresetHidden", "Apply Preset Hidden Materials"));
						Data->Modify();
						Data->ApplyPresetHiddenMaterials();
						Data->PostEditChange();
					}
					return FReply::Handled();
				})
			]
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%d slots"), SlotCount)))
		];
}

void FLODHiddenMaterialsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Suppress the default TArray<uint8> rendering so only our custom rows appear
	TSharedPtr<IPropertyHandle> HiddenMaterialsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FLODHiddenMaterials, HiddenMaterials));
	if (HiddenMaterialsHandle.IsValid())
	{
		ChildBuilder.AddProperty(HiddenMaterialsHandle.ToSharedRef()).Visibility(EVisibility::Collapsed);
	}

	FLODHiddenMaterials* StructData = GetRawStructData(PropertyHandle);
	if (!StructData) return;

	UHiddenMaterialsAssetUserData* Data = GetOwnerAssetUserData(PropertyHandle);
	if (Data)
	{
		Data->EnsureHiddenMaterialsSynced();
		StructData = GetRawStructData(PropertyHandle);
	}

	USkeletalMesh* Mesh = nullptr;
	if (Data)
	{
		Mesh = Cast<USkeletalMesh>(Data->GetOuter());
	}

	if (!Mesh) return;

	const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
	TArray<uint8>& Flags = StructData->HiddenMaterials;

	// Read preset hidden flags for this LOD (if a preset is selected)
	TArray<uint8> PresetFlags;
	if (Data)
	{
		PresetFlags = Data->GetPresetHiddenFlagsForLOD(StructData->LODIndex);
	}

	for (int32 i = 0; i < Flags.Num() && i < Materials.Num(); ++i)
	{
		FString SlotName = Materials[i].MaterialSlotName.ToString();
		FText SlotLabel = FText::FromString(FString::Printf(TEXT("%d  %s"), i, *SlotName));
		int32 SlotIdx = i;
		bool bHasPreset = PresetFlags.IsValidIndex(SlotIdx);
		bool bPresetHidden = bHasPreset && PresetFlags[SlotIdx] != 0;

		ChildBuilder.AddCustomRow(SlotLabel)
			.NameContent()
			[
				SNew(STextBlock)
				.Text(SlotLabel)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text_Lambda([StructData, SlotIdx]() -> FText
					{
						return StructData->HiddenMaterials.IsValidIndex(SlotIdx) && StructData->HiddenMaterials[SlotIdx] != 0
							? LOCTEXT("HiddenLabel", "Hidden")
							: LOCTEXT("VisibleLabel", "Visible");
					})
					.OnClicked_Lambda([StructData, SlotIdx, Data]() -> FReply
					{
						if (StructData && StructData->HiddenMaterials.IsValidIndex(SlotIdx))
						{
							FScopedTransaction Transaction(LOCTEXT("ToggleHiddenSlot", "Toggle Hidden Slot"));
							if (Data)
							{
								Data->Modify();
							}
							uint8 bCurrent = StructData->HiddenMaterials[SlotIdx];
							StructData->HiddenMaterials[SlotIdx] = (bCurrent == 0) ? 1 : 0;
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([bHasPreset, bPresetHidden]() -> FText
					{
						if (!bHasPreset)
						{
							return FText::GetEmpty();
						}
						return bPresetHidden
							? LOCTEXT("PresetHidden", "(Preset: Hidden)")
							: LOCTEXT("PresetVisible", "(Preset: Visible)");
					})
					.ColorAndOpacity_Lambda([StructData, SlotIdx, bHasPreset, bPresetHidden]() -> FSlateColor
					{
						if (!bHasPreset || !StructData->HiddenMaterials.IsValidIndex(SlotIdx))
						{
							return FSlateColor::UseForeground();
						}
						bool bCurrentHidden = StructData->HiddenMaterials[SlotIdx] != 0;
						if (bCurrentHidden == bPresetHidden)
						{
							return FSlateColor(FLinearColor(0.4f, 0.8f, 0.4f)); // Green when matching
						}
						return FSlateColor(FLinearColor(0.8f, 0.6f, 0.3f)); // Orange when different
					})
				]
			];
	}
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
