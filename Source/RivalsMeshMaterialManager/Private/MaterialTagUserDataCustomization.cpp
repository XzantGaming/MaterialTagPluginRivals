#if WITH_EDITOR

#include "MaterialTagUserDataCustomization.h"
#include "MaterialTagAssetUserData.h"
#include "MaterialTagDragDrop.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Fonts/SlateFontInfo.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAssetCommon.h"

#define LOCTEXT_NAMESPACE "PresetTagDisplayCustomization"

TSharedRef<IPropertyTypeCustomization> FPresetTagDisplayCustomization::MakeInstance()
{
	return MakeShareable(new FPresetTagDisplayCustomization());
}

void FPresetTagDisplayCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;

	FString MeshName = GetPresetMeshName();

	// Get UserData for the Refresh button (needed regardless of preset selection)
	UMaterialTagAssetUserData* UserData = GetUserData();
	TWeakObjectPtr<UMaterialTagAssetUserData> WeakUserData = UserData;

	if (MeshName.IsEmpty())
	{
		HeaderRow
			.NameContent()
			[
				PropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoPreset", "Select a Preset Mesh above (optional)"))
					.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 6, 0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("RefreshSlotsNoPreset", "Refresh Material Slots"))
					.ToolTipText(LOCTEXT("RefreshSlotsTipNoPreset", "Reload material slots from the mesh. Adds missing slots, keeps existing tag data."))
					.OnClicked_Lambda([WeakUserData]() -> FReply
					{
						if (UMaterialTagAssetUserData* UD = WeakUserData.Get())
						{
							UD->RefreshSlots();
						}
						return FReply::Handled();
					})
				]
			];
		return;
	}

	// Build full slot table from mesh + INI
	TArray<FPresetSlotInfo> SlotTable;
	TSet<FString> UniqueTags;
	BuildSlotTable(UserData, MeshName, SlotTable, UniqueTags);

	if (SlotTable.Num() == 0 && UniqueTags.Num() == 0)
	{
		HeaderRow
			.NameContent()
			[
				PropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("No data for '%s'"), *MeshName)))
				.ColorAndOpacity(FLinearColor(0.7f, 0.4f, 0.4f))
			];
		return;
	}

	// Build the slot table text like MaterialTagReference.txt
	FString TableText;
	if (SlotTable.Num() > 0)
	{
		int32 MaxLen = 12;
		for (const auto& Slot : SlotTable)
		{
			MaxLen = FMath::Max(MaxLen, Slot.SlotName.Len());
		}

		TableText += FString::Printf(TEXT("  %-4s %-*s  %s\n"), TEXT("#"), MaxLen, TEXT("Slot Name"), TEXT("Tag"));
		TableText += FString::Printf(TEXT("  ---- %s  %s\n"), *FString::ChrN(MaxLen, TEXT('-')), *FString::ChrN(30, TEXT('-')));

		for (const auto& Slot : SlotTable)
		{
			FString TagDisplay = Slot.Tags.IsEmpty() ? TEXT("(none)") : Slot.Tags;
			TagDisplay.ReplaceInline(TEXT(","), TEXT(", "));
			TableText += FString::Printf(TEXT("  %-4d %-*s  %s\n"), Slot.Index, MaxLen, *Slot.SlotName, *TagDisplay);
		}
	}

	// Build tag pills from unique tags
	TSharedRef<SWrapBox> WrapBox = SNew(SWrapBox)
		.UseAllottedSize(true);

	TArray<FString> SortedTags = UniqueTags.Array();
	SortedTags.Sort();

	// Also get the tag->slots map for tooltip hints
	TMap<FString, TArray<FString>> TagToSlots = GetTagToSlotsMap(MeshName);

	for (const FString& TagName : SortedTags)
	{
		FString SlotHint;
		if (const TArray<FString>* Slots = TagToSlots.Find(TagName))
		{
			SlotHint = FString::Join(*Slots, TEXT(", "));
		}

		WrapBox->AddSlot()
		.Padding(2.0f)
		[
			SNew(STagPill)
			.TagName(TagName)
			.SlotHint(SlotHint)
		];
	}

	FSlateFontInfo MonoFont = FCoreStyle::GetDefaultFontStyle("Mono", 8);

	HeaderRow
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PresetTagsLabel", "Preset Tags"))
		]
		.ValueContent()
		.MaxDesiredWidth(1200.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.MaxDesiredHeight(400.0f)
				[
					SNew(SHorizontalBox)
					// LEFT: Slot table (scrollable both vertically and horizontally)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(0, 0, 8, 0)
					[
						SNew(SScrollBox)
						.Orientation(Orient_Vertical)
						+ SScrollBox::Slot()
						[
							SNew(SScrollBox)
							.Orientation(Orient_Horizontal)
							+ SScrollBox::Slot()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TableText))
								.Font(MonoFont)
								.ColorAndOpacity(FLinearColor(0.85f, 0.75f, 0.5f))
							]
						]
					]
					// RIGHT: Draggable tag pills (scrollable)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8, 0, 0, 0)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 4)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DragHint", "Drag onto slots below:"))
							.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								WrapBox
							]
						]
					]
				]
			]
			// Refresh Slots button below the preset area
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 6, 0, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("RefreshSlots", "Refresh Material Slots"))
				.ToolTipText(LOCTEXT("RefreshSlotsTip", "Reload material slots from the mesh. Adds missing slots, keeps existing tag data."))
				.OnClicked_Lambda([WeakUserData]() -> FReply
				{
					if (UMaterialTagAssetUserData* UD = WeakUserData.Get())
					{
						UD->RefreshSlots();
					}
					return FReply::Handled();
				})
			]
		];
}

void FPresetTagDisplayCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

FString FPresetTagDisplayCustomization::GetPresetMeshName() const
{
	if (!StructHandle.IsValid()) return FString();

	TSharedPtr<IPropertyHandle> ParentHandle = StructHandle->GetParentHandle();
	if (!ParentHandle.IsValid()) return FString();

	TSharedPtr<IPropertyHandle> MeshNameHandle = ParentHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(UMaterialTagAssetUserData, PresetMeshName));
	if (!MeshNameHandle.IsValid()) return FString();

	FString MeshName;
	MeshNameHandle->GetValue(MeshName);
	return MeshName;
}

UMaterialTagAssetUserData* FPresetTagDisplayCustomization::GetUserData() const
{
	if (!StructHandle.IsValid()) return nullptr;

	TSharedPtr<IPropertyHandle> ParentHandle = StructHandle->GetParentHandle();
	if (!ParentHandle.IsValid()) return nullptr;

	TArray<UObject*> OuterObjects;
	ParentHandle->GetOuterObjects(OuterObjects);

	for (UObject* Obj : OuterObjects)
	{
		if (UMaterialTagAssetUserData* UD = Cast<UMaterialTagAssetUserData>(Obj))
			return UD;
	}
	return nullptr;
}

void FPresetTagDisplayCustomization::BuildSlotTable(UMaterialTagAssetUserData* UserData, const FString& MeshName, TArray<FPresetSlotInfo>& OutSlots, TSet<FString>& OutUniqueTags)
{
	OutSlots.Empty();
	OutUniqueTags.Empty();

	// Parse the INI once, building per-INDEX slot name + tags directly.
	// Keying by index (not name) prevents duplicate slot names from merging their tag lists.
	FString IniPath = GetPresetIniPath();
	if (!FPaths::FileExists(IniPath)) return;

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *IniPath);

	FString SectionHeader = FString::Printf(TEXT("[%s]"), *MeshName);
	bool bInSection = false;
	int32 SlotCount = 0;

	for (const FString& Line : Lines)
	{
		FString Trimmed = Line.TrimStartAndEnd();

		if (Trimmed.StartsWith(TEXT("[")))
		{
			if (bInSection) break;
			if (Trimmed.Equals(SectionHeader, ESearchCase::IgnoreCase))
				bInSection = true;
			continue;
		}

		if (!bInSection) continue;
		if (Trimmed.IsEmpty() || Trimmed.StartsWith(TEXT(";"))) continue;

		FString Key, Value;
		if (!Trimmed.Split(TEXT("="), &Key, &Value)) continue;
		Key = Key.TrimStartAndEnd();
		Value = Value.TrimStartAndEnd();

		if (Key == TEXT("SlotCount"))
		{
			SlotCount = FCString::Atoi(*Value);
			OutSlots.SetNum(SlotCount);
			for (int32 i = 0; i < SlotCount; i++)
			{
				OutSlots[i].Index = i;
			}
		}
		else if (Key.StartsWith(TEXT("Slot_")))
		{
			int32 Idx = FCString::Atoi(*Key.Mid(5));
			if (!OutSlots.IsValidIndex(Idx))
			{
				// Grow to fit if SlotCount was missing/low
				int32 OldNum = OutSlots.Num();
				OutSlots.SetNum(Idx + 1);
				for (int32 k = OldNum; k <= Idx; k++)
				{
					OutSlots[k].Index = k;
				}
			}

			FString SlotName, TagsPart;
			if (Value.Split(TEXT("|"), &SlotName, &TagsPart))
			{
				OutSlots[Idx].SlotName = SlotName.TrimStartAndEnd();

				TArray<FString> Tags;
				TagsPart.ParseIntoArray(Tags, TEXT(","));
				TArray<FString> CleanTags;
				for (FString& Tag : Tags)
				{
					Tag = Tag.TrimStartAndEnd();
					if (!Tag.IsEmpty())
					{
						CleanTags.Add(Tag);
						OutUniqueTags.Add(Tag);
					}
				}
				OutSlots[Idx].Tags = FString::Join(CleanTags, TEXT(", "));
			}
			else
			{
				OutSlots[Idx].SlotName = Value;
			}
		}
	}
}

TArray<FString> FPresetTagDisplayCustomization::GetPresetSlotList(const FString& MeshName)
{
	TArray<FString> Result;

	FString IniPath = GetPresetIniPath();
	if (!FPaths::FileExists(IniPath))
		return Result;

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *IniPath);

	FString SectionHeader = FString::Printf(TEXT("[%s]"), *MeshName);
	bool bInSection = false;
	int32 SlotCount = 0;

	for (const FString& Line : Lines)
	{
		FString Trimmed = Line.TrimStartAndEnd();

		if (Trimmed.StartsWith(TEXT("[")))
		{
			if (bInSection) break;
			if (Trimmed.Equals(SectionHeader, ESearchCase::IgnoreCase))
				bInSection = true;
			continue;
		}

		if (!bInSection) continue;
		if (Trimmed.IsEmpty() || Trimmed.StartsWith(TEXT(";"))) continue;

		FString Key, Value;
		if (!Trimmed.Split(TEXT("="), &Key, &Value)) continue;
		Key = Key.TrimStartAndEnd();
		Value = Value.TrimStartAndEnd();

		if (Key == TEXT("SlotCount"))
		{
			SlotCount = FCString::Atoi(*Value);
			Result.SetNum(SlotCount);
		}
		else if (Key.StartsWith(TEXT("Slot_")))
		{
			int32 Idx = FCString::Atoi(*Key.Mid(5));
			if (Result.IsValidIndex(Idx))
			{
				// Format: SlotName|Tag1,Tag2 - extract only the slot name part
				FString SlotName, TagsPart;
				if (Value.Split(TEXT("|"), &SlotName, &TagsPart))
				{
					Result[Idx] = SlotName.TrimStartAndEnd();
				}
				else
				{
					Result[Idx] = Value;
				}
			}
		}
	}

	return Result;
}

TMap<FString, TArray<FString>> FPresetTagDisplayCustomization::GetTagToSlotsMap(const FString& MeshName)
{
	TMap<FString, TArray<FString>> Result;

	FString IniPath = GetPresetIniPath();
	if (!FPaths::FileExists(IniPath))
		return Result;

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *IniPath);

	FString SectionHeader = FString::Printf(TEXT("[%s]"), *MeshName);
	bool bInSection = false;

	for (const FString& Line : Lines)
	{
		FString Trimmed = Line.TrimStartAndEnd();

		if (Trimmed.StartsWith(TEXT("[")))
		{
			if (bInSection) break;
			if (Trimmed.Equals(SectionHeader, ESearchCase::IgnoreCase))
				bInSection = true;
			continue;
		}

		if (!bInSection) continue;
		if (Trimmed.IsEmpty() || Trimmed.StartsWith(TEXT(";"))) continue;

		FString Key, Value;
		if (!Trimmed.Split(TEXT("="), &Key, &Value)) continue;
		Key = Key.TrimStartAndEnd();
		Value = Value.TrimStartAndEnd();

		// Parse Slot_N=SlotName|Tag1,Tag2 format
		if (Key.StartsWith(TEXT("Slot_")))
		{
			FString SlotName, TagsPart;
			if (Value.Split(TEXT("|"), &SlotName, &TagsPart))
			{
				SlotName = SlotName.TrimStartAndEnd();
				// Parse comma-separated tags
				TArray<FString> Tags;
				TagsPart.ParseIntoArray(Tags, TEXT(","));
				for (FString& Tag : Tags)
				{
					Tag = Tag.TrimStartAndEnd();
					if (!Tag.IsEmpty())
					{
						Result.FindOrAdd(Tag).AddUnique(SlotName);
					}
				}
			}
		}
	}

	return Result;
}

FString FPresetTagDisplayCustomization::GetPresetIniPath()
{
	return FPaths::ProjectConfigDir() / TEXT("MaterialTagPresets.ini");
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
