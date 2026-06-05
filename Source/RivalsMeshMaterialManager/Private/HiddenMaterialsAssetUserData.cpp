#include "HiddenMaterialsAssetUserData.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Internationalization/Regex.h"
#if WITH_EDITOR
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#endif

UHiddenMaterialsAssetUserData::UHiddenMaterialsAssetUserData()
{
}

void UHiddenMaterialsAssetUserData::RefreshSlots()
{
	EnsureHiddenMaterialsSynced();
	UE_LOG(LogTemp, Log, TEXT("HiddenMaterialsAssetUserData: Refreshed hidden materials slots"));

#if WITH_EDITOR
	if (UObject* Outer = GetOuter())
	{
		Outer->MarkPackageDirty();
	}
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.NotifyCustomizationModuleChanged();
#endif
}

void UHiddenMaterialsAssetUserData::EnsureHiddenMaterialsSynced()
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(GetOuter());
	if (!Mesh) return;

	const int32 LODCount = Mesh->GetLODNum();
	const int32 SlotCount = Mesh->GetMaterials().Num();

	// Resize LOD array to match mesh LOD count
	if (LODHiddenMaterials.Num() != LODCount)
	{
		TArray<FLODHiddenMaterials> NewLODArray;
		NewLODArray.SetNum(LODCount);

		// Preserve existing data for matching LOD indices
		for (int32 LodIdx = 0; LodIdx < FMath::Min(LODCount, LODHiddenMaterials.Num()); ++LodIdx)
		{
			NewLODArray[LodIdx] = LODHiddenMaterials[LodIdx];
		}

		LODHiddenMaterials = MoveTemp(NewLODArray);
	}

	// Resize each LOD's slot array to match material count
	for (int32 LodIdx = 0; LodIdx < LODCount; ++LodIdx)
	{
		LODHiddenMaterials[LodIdx].LODIndex = LodIdx;

		TArray<uint8>& Flags = LODHiddenMaterials[LodIdx].HiddenMaterials;
		if (Flags.Num() != SlotCount)
		{
			TArray<uint8> NewFlags;
			NewFlags.SetNum(SlotCount);

			// Preserve existing flags for matching slot indices
			for (int32 SlotIdx = 0; SlotIdx < FMath::Min(SlotCount, Flags.Num()); ++SlotIdx)
			{
				NewFlags[SlotIdx] = Flags[SlotIdx];
			}

			Flags = MoveTemp(NewFlags);
		}
	}
}

void UHiddenMaterialsAssetUserData::AutoMatchPresetFromMesh()
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(GetOuter());
	if (!Mesh) return;

	FString MeshName = Mesh->GetName();
	TArray<FString> Presets = GetPresetMeshNames();

	// Try exact match first
	for (const FString& P : Presets)
	{
		if (P.Equals(MeshName, ESearchCase::IgnoreCase))
		{
			PresetMeshName = P;
			return;
		}
	}

	// Try substring match
	for (const FString& P : Presets)
	{
		if (P.IsEmpty()) continue;
		if (MeshName.Contains(P) || P.Contains(MeshName))
		{
			PresetMeshName = P;
			return;
		}
	}

	// Try matching by character ID (e.g. "1048309" in both names)
	FRegexPattern Pattern(TEXT("(\\d{7})"));
	FRegexMatcher Matcher(Pattern, MeshName);
	if (Matcher.FindNext())
	{
		FString CharId = Matcher.GetCaptureGroup(1);
		for (const FString& P : Presets)
		{
			if (P.Contains(CharId))
			{
				PresetMeshName = P;
				return;
			}
		}
	}
}

void UHiddenMaterialsAssetUserData::ApplyPresetHiddenMaterials()
{
	if (PresetMeshName.IsEmpty()) return;

	FString IniPath = GetPresetIniPath();
	if (!FPaths::FileExists(IniPath)) return;

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *IniPath);

	FString SectionHeader = FString::Printf(TEXT("[%s]"), *PresetMeshName);
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

		// Key format: LOD_<N>_Hidden=0,1,0,1,...
		if (Key.StartsWith(TEXT("LOD_")) && Key.EndsWith(TEXT("_Hidden")))
		{
			FString LodNumStr = Key.Mid(4);
			LodNumStr.LeftInline(LodNumStr.Len() - 7); // Remove "_Hidden"
			int32 LodIdx = FCString::Atoi(*LodNumStr);

			if (!LODHiddenMaterials.IsValidIndex(LodIdx))
				continue;

			TArray<FString> Pieces;
			Value.ParseIntoArray(Pieces, TEXT(","));

			TArray<uint8>& Flags = LODHiddenMaterials[LodIdx].HiddenMaterials;
			for (int32 i = 0; i < FMath::Min(Pieces.Num(), Flags.Num()); ++i)
			{
				Flags[i] = FCString::Atoi(*Pieces[i].TrimStartAndEnd()) != 0 ? 1 : 0;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("HiddenMaterialsAssetUserData: Applied preset hidden materials for '%s'"), *PresetMeshName);
}

TArray<uint8> UHiddenMaterialsAssetUserData::GetPresetHiddenFlagsForLOD(int32 LODIndex) const
{
	TArray<uint8> Result;

	if (PresetMeshName.IsEmpty())
	{
		return Result;
	}

	FString IniPath = GetPresetIniPath();
	if (!FPaths::FileExists(IniPath))
	{
		return Result;
	}

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *IniPath);

	FString SectionHeader = FString::Printf(TEXT("[%s]"), *PresetMeshName);
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

		// Key format: LOD_<N>_Hidden=0,1,0,1,...
		if (Key.StartsWith(TEXT("LOD_")) && Key.EndsWith(TEXT("_Hidden")))
		{
			FString LodNumStr = Key.Mid(4);
			LodNumStr.LeftInline(LodNumStr.Len() - 7); // Remove "_Hidden"
			int32 LodIdx = FCString::Atoi(*LodNumStr);

			if (LodIdx != LODIndex)
				continue;

			TArray<FString> Pieces;
			Value.ParseIntoArray(Pieces, TEXT(","));
			Result.Reserve(Pieces.Num());
			for (const FString& Piece : Pieces)
			{
				Result.Add(FCString::Atoi(*Piece.TrimStartAndEnd()) != 0 ? 1 : 0);
			}
			break;
		}
	}

	return Result;
}

TArray<FString> UHiddenMaterialsAssetUserData::GetPresetMeshNames() const
{
	TArray<FString> Names;
	Names.Add(TEXT("")); // Empty option to clear selection

	FString IniPath = GetPresetIniPath();
	if (!FPaths::FileExists(IniPath))
	{
		return Names;
	}

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *IniPath);

	for (const FString& Line : Lines)
	{
		FString Trimmed = Line.TrimStartAndEnd();
		if (Trimmed.StartsWith(TEXT("[")) && Trimmed.EndsWith(TEXT("]")))
		{
			FString SectionName = Trimmed.Mid(1, Trimmed.Len() - 2);
			if (!SectionName.IsEmpty())
			{
				Names.Add(SectionName);
			}
		}
	}

	return Names;
}

FString UHiddenMaterialsAssetUserData::GetPresetIniPath()
{
	return FPaths::ProjectConfigDir() / TEXT("MaterialTagPresets.ini");
}

#if WITH_EDITOR
void UHiddenMaterialsAssetUserData::PostLoad()
{
	Super::PostLoad();
	EnsureHiddenMaterialsSynced();
}

void UHiddenMaterialsAssetUserData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropName = PropertyChangedEvent.GetPropertyName();
	bool bNeedsRefresh = false;

	// Handle auto-match toggle
	if (PropName == GET_MEMBER_NAME_CHECKED(UHiddenMaterialsAssetUserData, bAutoMatchPreset))
	{
		if (bAutoMatchPreset)
		{
			AutoMatchPresetFromMesh();
			ApplyPresetHiddenMaterials();
		}
		bNeedsRefresh = true;
	}

	// Handle preset name change
	if (PropName == GET_MEMBER_NAME_CHECKED(UHiddenMaterialsAssetUserData, PresetMeshName))
	{
		bNeedsRefresh = true;
	}

	if (bNeedsRefresh)
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	if (UObject* Outer = GetOuter())
	{
		Outer->MarkPackageDirty();
	}
}
#endif
