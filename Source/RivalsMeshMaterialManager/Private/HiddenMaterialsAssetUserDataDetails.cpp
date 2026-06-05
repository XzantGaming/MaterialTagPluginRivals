#if WITH_EDITOR

#include "HiddenMaterialsAssetUserDataDetails.h"
#include "HiddenMaterialsAssetUserData.h"
#include "Engine/SkeletalMesh.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Engine/SkinnedAssetCommon.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "HiddenMaterialsAssetUserDataDetails"

TSharedRef<IDetailCustomization> FHiddenMaterialsAssetUserDataDetails::MakeInstance()
{
	return MakeShareable(new FHiddenMaterialsAssetUserDataDetails());
}

void FHiddenMaterialsAssetUserDataDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingEdited);

	UHiddenMaterialsAssetUserData* Data = nullptr;
	for (TWeakObjectPtr<UObject> Obj : ObjectsBeingEdited)
	{
		if (UHiddenMaterialsAssetUserData* Casted = Cast<UHiddenMaterialsAssetUserData>(Obj.Get()))
		{
			Data = Casted;
			break;
		}
	}

	if (!Data) return;

	Data->EnsureHiddenMaterialsSynced();

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Hidden Materials");

	// Hide the raw TArray property — we render it manually
	TSharedRef<IPropertyHandle> LodArrayHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UHiddenMaterialsAssetUserData, LODHiddenMaterials));
	DetailBuilder.HideProperty(LodArrayHandle);

	RebuildLODSelector(Category, Data);
	RebuildSlotGrid(Category, Data);

	// --- Apply Preset button (green when preset selected, gray when not) ---
	Category.AddCustomRow(LOCTEXT("ApplyPresetRow", "Apply Preset"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ApplyPresetLabel", "Apply Preset"))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		.ValueContent()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.HAlign(HAlign_Fill)
			.Text_Lambda([Data]() -> FText
			{
				if (Data && !Data->PresetMeshName.IsEmpty())
				{
					return FText::Format(LOCTEXT("ApplyPresetBtnActive", "Apply: {0}"), FText::FromString(Data->PresetMeshName));
				}
				return LOCTEXT("ApplyPresetBtnInactive", "Select a preset first");
			})
			.ToolTipText(LOCTEXT("ApplyPresetTip", "Read LOD_N_Hidden from the preset INI and populate this asset"))
			.ButtonColorAndOpacity_Lambda([Data]() -> FLinearColor
			{
				if (Data && !Data->PresetMeshName.IsEmpty())
				{
					return FLinearColor(0.2f, 0.7f, 0.3f, 1.0f); // Green when active
				}
				return FLinearColor(0.3f, 0.3f, 0.3f, 1.0f); // Gray when inactive
			})
			.OnClicked_Lambda([this, Data]() -> FReply
			{
				if (Data && !Data->PresetMeshName.IsEmpty())
				{
					OnApplyPreset(Data);
				}
				return FReply::Handled();
			})
		];

	// --- Copy LOD 0 to All button ---
	Category.AddCustomRow(LOCTEXT("CopyLOD0Row", "Copy LOD 0"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CopyLOD0Label", "Copy LOD 0"))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		.ValueContent()
		[
			SNew(SButton)
			.Text(LOCTEXT("CopyLOD0Btn", "Copy LOD 0 -> All"))
			.ToolTipText(LOCTEXT("CopyLOD0Tip", "Replicate LOD 0 hidden flags to every other LOD"))
			.OnClicked_Lambda([this, Data]() -> FReply
			{
				OnCopyLOD0ToAll(Data);
				return FReply::Handled();
			})
		];

	// --- Validation label ---
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(Data->GetOuter());
	if (Mesh)
	{
		int32 ExpectedLODCount = Mesh->GetLODNum();
		int32 ExpectedSlotCount = Mesh->GetMaterials().Num();
		int32 ActualLODCount = Data->LODHiddenMaterials.Num();

		bool bValid = true;
		FString ValidationText = FString::Printf(TEXT("Mesh: %d LODs, %d slots  |  Data: %d LODs"),
			ExpectedLODCount, ExpectedSlotCount, ActualLODCount);

		if (ActualLODCount != ExpectedLODCount)
		{
			bValid = false;
			ValidationText += TEXT("  [MISMATCH: press Refresh Slots]");
		}
		else
		{
			for (int32 i = 0; i < ActualLODCount; ++i)
			{
				if (Data->LODHiddenMaterials[i].HiddenMaterials.Num() != ExpectedSlotCount)
				{
					bValid = false;
					ValidationText += FString::Printf(TEXT("  [LOD %d slot count mismatch]"), i);
					break;
				}
			}
		}

		Category.AddCustomRow(LOCTEXT("ValidationRow", "Validation"))
			.NameContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ValidationLabel", "Status"))
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
			]
			.ValueContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(ValidationText))
				.ColorAndOpacity(bValid ? FLinearColor(0.3f, 1.0f, 0.3f) : FLinearColor(1.0f, 0.3f, 0.3f))
			];
	}
}

void FHiddenMaterialsAssetUserDataDetails::RebuildLODSelector(IDetailCategoryBuilder& Category, UHiddenMaterialsAssetUserData* Data)
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(Data->GetOuter());
	int32 LODCount = Mesh ? Mesh->GetLODNum() : Data->LODHiddenMaterials.Num();
	if (LODCount <= 1) return;

	Category.AddCustomRow(LOCTEXT("LODSelectorRow", "LOD"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("LODSelectorLabel", "Selected LOD"))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("PrevLOD", "<"))
				.OnClicked_Lambda([this, LODCount]() -> FReply
				{
					if (SelectedLODIndex > 0)
					{
						SelectedLODIndex--;
						FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
						PropertyModule.NotifyCustomizationModuleChanged();
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("LOD %d"), SelectedLODIndex)))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("NextLOD", ">"))
				.OnClicked_Lambda([this, LODCount]() -> FReply
				{
					if (SelectedLODIndex < LODCount - 1)
					{
						SelectedLODIndex++;
						FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
						PropertyModule.NotifyCustomizationModuleChanged();
					}
					return FReply::Handled();
				})
			]
		];
}

void FHiddenMaterialsAssetUserDataDetails::RebuildSlotGrid(IDetailCategoryBuilder& Category, UHiddenMaterialsAssetUserData* Data)
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(Data->GetOuter());
	if (!Mesh) { return; }
	if (!Data->LODHiddenMaterials.IsValidIndex(SelectedLODIndex)) { return; }

	const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
	TArray<uint8>& Flags = Data->LODHiddenMaterials[SelectedLODIndex].HiddenMaterials;

	Category.AddCustomRow(LOCTEXT("SlotGridHeader", "Slots"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SlotGridHeaderLabel", "Slot"))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("HiddenHeaderLabel", "Hidden"))
			.Font(IDetailLayoutBuilder::GetDetailFontBold())
		];

	for (int32 i = 0; i < Materials.Num(); ++i)
	{
		FString SlotName = Materials[i].MaterialSlotName.ToString();
		bool bIsHidden = Flags.IsValidIndex(i) ? (Flags[i] != 0) : false;
		int32 SlotIndex = i;

		Category.AddCustomRow(FText::FromString(FString::Printf(TEXT("Slot_%d"), i)))
			.NameContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d  %s"), i, *SlotName)))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			[
				SNew(SButton)
				.Text(FText::FromString(bIsHidden ? TEXT("Hidden") : TEXT("Visible")))
				.OnClicked_Lambda([this, SlotIndex, Data]() -> FReply
				{
					OnToggleHidden(SlotIndex, Data);
					return FReply::Handled();
				})
			];
	}
}

FReply FHiddenMaterialsAssetUserDataDetails::OnToggleHidden(int32 SlotIndex, UHiddenMaterialsAssetUserData* Data)
{
	if (!Data) return FReply::Handled();
	if (!Data->LODHiddenMaterials.IsValidIndex(SelectedLODIndex)) return FReply::Handled();

	TArray<uint8>& Flags = Data->LODHiddenMaterials[SelectedLODIndex].HiddenMaterials;
	if (!Flags.IsValidIndex(SlotIndex)) return FReply::Handled();

	FScopedTransaction Transaction(LOCTEXT("ToggleHidden", "Toggle Hidden Material"));
	Data->Modify();
	Flags[SlotIndex] = !Flags[SlotIndex];
	Data->PostEditChange();
	return FReply::Handled();
}

void FHiddenMaterialsAssetUserDataDetails::OnCopyLOD0ToAll(UHiddenMaterialsAssetUserData* Data)
{
	if (!Data) return;
	if (!Data->LODHiddenMaterials.IsValidIndex(0)) return;

	FScopedTransaction Transaction(LOCTEXT("CopyLOD0", "Copy LOD 0 to All"));
	Data->Modify();

	const TArray<uint8>& LOD0Flags = Data->LODHiddenMaterials[0].HiddenMaterials;

	for (int32 LodIdx = 1; LodIdx < Data->LODHiddenMaterials.Num(); ++LodIdx)
	{
		TArray<uint8>& TargetFlags = Data->LODHiddenMaterials[LodIdx].HiddenMaterials;
		TargetFlags.SetNum(LOD0Flags.Num());
		for (int32 SlotIdx = 0; SlotIdx < LOD0Flags.Num(); ++SlotIdx)
		{
			TargetFlags[SlotIdx] = LOD0Flags[SlotIdx];
		}
	}

	Data->PostEditChange();
}

void FHiddenMaterialsAssetUserDataDetails::OnApplyPreset(UHiddenMaterialsAssetUserData* Data)
{
	if (!Data) return;
	FScopedTransaction Transaction(LOCTEXT("ApplyPresetHidden", "Apply Preset Hidden Materials"));
	Data->Modify();
	Data->ApplyPresetHiddenMaterials();
	Data->PostEditChange();
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
