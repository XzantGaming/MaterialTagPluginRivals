#include "HiddenMaterialDefaultsAssetUserData.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAssetCommon.h"

UHiddenMaterialDefaultsAssetUserData::UHiddenMaterialDefaultsAssetUserData()
{
}

void UHiddenMaterialDefaultsAssetUserData::PopulateFromMesh()
{
	EnsurePopulated();

#if WITH_EDITOR
	if (UObject* Outer = GetOuter())
	{
		Outer->MarkPackageDirty();
	}
#endif
}

void UHiddenMaterialDefaultsAssetUserData::EnsurePopulated()
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(GetOuter());
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("HiddenMaterialDefaultsAssetUserData: Not attached to a SkeletalMesh"));
		return;
	}

	const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
	const int32 NumLODs = FMath::Max(1, Mesh->GetLODNum());
	const int32 NumMaterials = Materials.Num();

	// Drop any LOD entries beyond what the mesh has.
	if (LODs.Num() > NumLODs)
	{
		LODs.SetNum(NumLODs);
	}

	for (int32 LODIndex = 0; LODIndex < NumLODs; LODIndex++)
	{
		bool bIsNewLOD = false;
		if (!LODs.IsValidIndex(LODIndex))
		{
			LODs.AddDefaulted();
			bIsNewLOD = true;
		}

		FHiddenMaterialLOD& LODEntry = LODs[LODIndex];
		LODEntry.LODIndex = LODIndex;

		// If this is a newly added LOD, initialize materials from scratch
		if (bIsNewLOD || LODEntry.Materials.Num() == 0)
		{
			LODEntry.Materials.Reset();
			LODEntry.Materials.Reserve(NumMaterials);
			for (int32 i = 0; i < NumMaterials; i++)
			{
				FHiddenMaterialEntry Entry;
				Entry.MaterialSlotName = Materials[i].MaterialSlotName;
				Entry.bHidden = false; // Default to visible
				LODEntry.Materials.Add(Entry);
			}
		}
		else
		{
			// Preserve existing flags keyed by slot name before resizing.
			TMap<FName, bool> PreviousFlags;
			for (const FHiddenMaterialEntry& Existing : LODEntry.Materials)
			{
				if (!Existing.MaterialSlotName.IsNone())
				{
					PreviousFlags.Add(Existing.MaterialSlotName, Existing.bHidden);
				}
			}

			// Rebuild in mesh material order so the array maps 1:1 to DefaultHiddenMaterials.
			TArray<FHiddenMaterialEntry> Rebuilt;
			Rebuilt.Reserve(NumMaterials);
			for (int32 i = 0; i < NumMaterials; i++)
			{
				FHiddenMaterialEntry Entry;
				Entry.MaterialSlotName = Materials[i].MaterialSlotName;
				if (const bool* Prev = PreviousFlags.Find(Entry.MaterialSlotName))
				{
					Entry.bHidden = *Prev;
				}
				Rebuilt.Add(Entry);
			}
			LODEntry.Materials = MoveTemp(Rebuilt);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("HiddenMaterialDefaultsAssetUserData: Populated %d LOD(s) x %d material(s)"), NumLODs, NumMaterials);
}

void UHiddenMaterialDefaultsAssetUserData::EnsureLODMaterialsPopulated(int32 LODIndex)
{
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(GetOuter());
	if (!Mesh || !LODs.IsValidIndex(LODIndex))
	{
		return;
	}

	const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
	const int32 NumMaterials = Materials.Num();
	FHiddenMaterialLOD& LODEntry = LODs[LODIndex];

	// If materials array is empty or size doesn't match, rebuild it
	if (LODEntry.Materials.Num() != NumMaterials)
	{
		// Preserve existing flags
		TMap<FName, bool> PreviousFlags;
		for (const FHiddenMaterialEntry& Existing : LODEntry.Materials)
		{
			if (!Existing.MaterialSlotName.IsNone())
			{
				PreviousFlags.Add(Existing.MaterialSlotName, Existing.bHidden);
			}
		}

		// Rebuild to match current mesh materials
		LODEntry.Materials.Reset();
		LODEntry.Materials.Reserve(NumMaterials);
		for (int32 i = 0; i < NumMaterials; i++)
		{
			FHiddenMaterialEntry Entry;
			Entry.MaterialSlotName = Materials[i].MaterialSlotName;
			if (const bool* Prev = PreviousFlags.Find(Entry.MaterialSlotName))
			{
				Entry.bHidden = *Prev;
			}
			LODEntry.Materials.Add(Entry);
		}
	}
}

TArray<bool> UHiddenMaterialDefaultsAssetUserData::GetHiddenFlagsForLOD(int32 LODIndex) const
{
	TArray<bool> Flags;
	if (LODs.IsValidIndex(LODIndex))
	{
		Flags.Reserve(LODs[LODIndex].Materials.Num());
		for (const FHiddenMaterialEntry& Entry : LODs[LODIndex].Materials)
		{
			Flags.Add(Entry.bHidden);
		}
	}
	return Flags;
}

#if WITH_EDITOR
void UHiddenMaterialDefaultsAssetUserData::PostLoad()
{
	Super::PostLoad();
	EnsurePopulated();
}

void UHiddenMaterialDefaultsAssetUserData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// TEMPORARILY DISABLED - testing if this interferes with toggle
	/*
	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	FName MemberPropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;
	EPropertyChangeType::Type ChangeType = PropertyChangedEvent.ChangeType;

	bool bIsArrayStructureChange = (ChangeType == EPropertyChangeType::ArrayAdd ||
		ChangeType == EPropertyChangeType::ArrayRemove ||
		ChangeType == EPropertyChangeType::ArrayClear ||
		ChangeType == EPropertyChangeType::ArrayMove);

	if (bIsArrayStructureChange &&
		(PropertyName == GET_MEMBER_NAME_CHECKED(UHiddenMaterialDefaultsAssetUserData, LODs) ||
		 MemberPropertyName == GET_MEMBER_NAME_CHECKED(UHiddenMaterialDefaultsAssetUserData, LODs)))
	{
		EnsurePopulated();
	}

	if (bIsArrayStructureChange &&
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(FHiddenMaterialLOD, Materials))
	{
		for (int32 i = 0; i < LODs.Num(); i++)
		{
			EnsureLODMaterialsPopulated(i);
		}
	}
	*/

	if (UObject* Outer = GetOuter())
	{
		Outer->MarkPackageDirty();
	}
}
#endif
