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

	if (MeshName.IsEmpty())
	{
		HeaderRow
			.NameContent()
			[
				PropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NoPreset", "Select a Preset Mesh above"))
				.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			];
		return;
	}

	// Build full slot table from mesh + INI
	UMaterialTagAssetUserData* UserData = GetUserData();
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
			SNew(SHorizontalBox)
			// LEFT: Slot table
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0, 0, 8, 0)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TableText))
					.Font(MonoFont)
					.ColorAndOpacity(FLinearColor(0.85f, 0.75f, 0.5f))
				]
			]
			// RIGHT: Draggable tag pills
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
				.AutoHeight()
				[
					WrapBox
				]
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

	// Get tag->slots map from the selected preset INI
	TMap<FString, TArray<FString>> TagToSlots = GetTagToSlotsMap(MeshName);

	// Build reverse map: slot name -> tags
	TMap<FString, TArray<FString>> SlotToTags;
	for (const auto& Pair : TagToSlots)
	{
		OutUniqueTags.Add(Pair.Key);
		for (const FString& SlotName : Pair.Value)
		{
			SlotToTags.FindOrAdd(SlotName.TrimStartAndEnd()).AddUnique(Pair.Key);
		}
	}

	// Use the PRESET's full slot list from the INI (Slot_N keys)
	TArray<FString> PresetSlots = GetPresetSlotList(MeshName);

	for (int32 i = 0; i < PresetSlots.Num(); i++)
	{
		FPresetSlotInfo Info;
		Info.Index = i;
		Info.SlotName = PresetSlots[i];

		if (const TArray<FString>* Tags = SlotToTags.Find(Info.SlotName))
		{
			Info.Tags = FString::Join(*Tags, TEXT(", "));
		}

		OutSlots.Add(Info);
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
				Result[Idx] = Value;
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

		// Skip Slot_N and SlotCount keys (new format)
		if (Trimmed.StartsWith(TEXT("Slot_")) || Trimmed.StartsWith(TEXT("SlotCount")))
			continue;

		FString Key, Value;
		if (Trimmed.Split(TEXT("="), &Key, &Value))
		{
			Key = Key.TrimStartAndEnd();
			Value = Value.TrimStartAndEnd();

			TArray<FString> Slots;
			Value.ParseIntoArray(Slots, TEXT(","));
			for (FString& S : Slots) S = S.TrimStartAndEnd();

			Result.Add(Key, Slots);
		}
	}

	return Result;
}

FString FPresetTagDisplayCustomization::GetPresetIniPath()
{
	return FPaths::ProjectPluginsDir() / TEXT("MaterialTagPlugin") / TEXT("Config") / TEXT("MaterialTagPresets.ini");
}

#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR
