# Hidden Material Defaults - UAsset Storage Format

This document describes how the `HiddenMaterialDefaultsAssetUserData` is stored in cooked Unreal Engine 5.3 UAssets (`.uasset` / `.uexp` files).

## Overview

The plugin stores per-LOD default hidden material flags as **Asset User Data** attached to `SkeletalMesh` assets. This data serializes as a `UAssetUserData` object within the mesh's `AssetUserData` array.

## Data Structure

### Source Structures

```cpp
USTRUCT()
struct FHiddenMaterialEntry
{
    FName MaterialSlotName;    // Reference only - not used at runtime
    bool bHidden = false;      // false = visible, true = hidden
};

USTRUCT()
struct FHiddenMaterialLOD
{
    int32 LODIndex = 0;
    TArray<FHiddenMaterialEntry> Materials;  // One per material slot
};

UCLASS()
class UHiddenMaterialDefaultsAssetUserData : public UAssetUserData
{
    TArray<FHiddenMaterialLOD> LODs;  // One per mesh LOD
};
```

### Cooked Binary Format

When cooked, the data is stored in the `.uexp` export data for the SkeletalMesh as part of the `AssetUserData` array serialization.

#### Header
```
[AssetUserData Array Entry]
├─ Object Pointer: UHiddenMaterialDefaultsAssetUserData
├─ Class: UHiddenMaterialDefaultsAssetUserData
└─ Outer: (SkeletalMesh)
```

#### Data Serialization (UProperty Format)

The `LODs` array is serialized using standard UE5 property serialization:

```
LODs (TArray<FHiddenMaterialLOD>)
├─ Array Count: int32 (number of LODs)
└─ [For each LOD]:
   ├─ LODIndex: int32
   └─ Materials (TArray<FHiddenMaterialEntry>):
      ├─ Array Count: int32 (number of materials)
      └─ [For each Material]:
         ├─ MaterialSlotName: FName (string + index pair)
         └─ bHidden: bool (1 byte, 0 or 1)
```

#### Binary Layout Example

For a mesh with **2 LODs** and **3 materials per LOD**:

```
Offset  Type      Value                          Description
------  ----      -----                          -----------
+0x00   int32     2                              LODs.ArrayNum
+0x04   struct[0] LOD 0 data
        ├─ int32  0                              LODIndex
        ├─ int32  3                              Materials.ArrayNum
        └─ struct[0] Material 0
            ├─ FName "Material_Slot_0"            MaterialSlotName
            └─ byte   0x00                         bHidden (false = visible)
        └─ struct[1] Material 1
            ├─ FName "Material_Slot_1"
            └─ byte   0x01                         bHidden (true = hidden)
        └─ struct[2] Material 2
            ├─ FName "Material_Slot_2"
            └─ byte   0x00
+0xXX   struct[1] LOD 1 data
        ... (same structure as LOD 0)
```

## How to Read at Runtime (Cooked Build)

```cpp
// Get the SkeletalMesh
USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(...);

// Find the asset user data
UHiddenMaterialDefaultsAssetUserData* HiddenData = Mesh->GetAssetUserData<UHiddenMaterialDefaultsAssetUserData>();

// Read the hidden flags for a specific LOD
if (HiddenData)
{
    TArray<bool> HiddenFlags = HiddenData->GetHiddenFlagsForLOD(0);
    // HiddenFlags[0] = false (visible)
    // HiddenFlags[1] = true  (hidden)
    // etc.
}
```

## Relationship to Marvel Rivals

Marvel Rivals stores its per-LOD hidden material flags in a different location:
- **Game Location**: `SkeletalMesh->LODInfo[LODIndex].DefaultHiddenMaterials` (TArray<bool>)
- **Plugin bridges this gap** by:
  1. Reading from the cooked AssetUserData (this plugin's format)
  2. Writing to the game's native format during mod creation

## Key Points for Modding

1. **Persistent Storage**: The data survives cooking because it's part of the serialized UAssetUserData
2. **Slot Order**: The `Materials` array order MUST match the mesh's material slot order (0-indexed)
3. **LOD Correspondence**: `LODIndex` corresponds to `SkeletalMesh->GetLODInfo()[Index]`
4. **No Runtime Overhead**: The AssetUserData is only accessed during mod creation; no runtime performance cost

## File Location in Cooked Package

```
Cooked Package (.pak/.ucas):
└─ Exports
   └─ SkeletalMesh_Export
      └─ AssetUserData[0] = UHiddenMaterialDefaultsAssetUserData
         └─ LODs[]
            └─ Materials[].bHidden
```

The data is loaded automatically when the SkeletalMesh is loaded by the engine's asset serialization system.
