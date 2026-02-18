# Material Tag Plugin

An Unreal Engine 5.3 plugin for embedding GameplayTagContainer data per material slot in SkeletalMesh assets. Designed for Marvel Rivals modding in combination with [UAssetToolRivals](https://github.com/XzantGaming/UAssetToolRivals).

## Overview

Marvel Rivals uses `FGameplayTagContainer` fields in `FSkeletalMaterial` structs to control material visibility (e.g., hiding weapons, equipment toggles). This plugin provides an editor interface to define these tags per material slot, which are then read by UAssetTool during mod creation and injected into the final IoStore package.

## Features

- **Per-slot tag assignment**: Assign GameplayTags to individual material slots on SkeletalMesh assets
- **Auto-populate**: One-click button to create entries for all material slots in a mesh
- **Preset system**: Load tag configurations from INI presets for common mesh types
- **Drag-and-drop**: Draggable tag pills from presets to material slot entries
- **Auto-match**: Automatically select presets based on mesh name

## Installation

### From Release (Recommended)

1. Download the latest release ZIP for your UE version
2. Extract to your project's `Plugins/` folder
3. Restart the Unreal Editor
4. Enable the plugin in Edit → Plugins if not auto-enabled

### From Source

1. Clone this repository into your project's `Plugins/` folder:
   ```
   git clone https://github.com/YOUR_USERNAME/MaterialTagPlugin.git Plugins/MaterialTagPlugin
   ```
2. Regenerate project files
3. Build the project

## Usage

1. Open your SkeletalMesh asset in the editor
2. In the Details panel, find **Asset User Data** array
3. Click **+** to add a new entry, select **Material Tag Data**
4. Click **Populate From Mesh** to auto-create entries for all material slots
5. For each slot that needs tags, expand the **GameplayTags** array and add entries
6. Save the mesh

### Common Marvel Rivals Material Tags

| Tag | Description |
|-----|-------------|
| `MaterialTag.装备.武器` | Equipment.Weapon - Used for weapon visibility |
| `MaterialTag.装备` | Equipment - General equipment tag |

## Preset Configuration

Create `Config/MaterialTagPresets.ini` in your project to define reusable tag configurations:

```ini
[SK_CharacterName]
+Slots=(SlotName="M_Weapon",Tags=("MaterialTag.装备.武器"))
+Slots=(SlotName="M_Body",Tags=("MaterialTag.装备"))
```

## Integration with UAssetTool

When you cook your SkeletalMesh asset, the `UMaterialTagAssetUserData` is serialized with it. UAssetTool's `create_mod_iostore` command:

1. Detects the MaterialTagAssetUserData export
2. Reads the slot-tag mappings
3. Injects tags into each `FSkeletalMaterial::GameplayTagContainer`
4. Strips the UserData export (game doesn't need it)
5. Remaps `/Script/MaterialTagPlugin` imports to `/Script/Engine`

## Requirements

- Unreal Engine 5.3.x
- GameplayTags plugin (included with UE)

## Building from Source

### Prerequisites

- Visual Studio 2022 with C++ game development workload
- Unreal Engine 5.3 source or installed build

### Build Steps

1. Place the plugin in an UE5.3 project's `Plugins/` folder
2. Generate project files (right-click .uproject → Generate Visual Studio project files)
3. Open the solution and build in Development Editor configuration

## License

MIT License - see [LICENSE](LICENSE) for details.

## Credits

- **Xzant** - Plugin development
- Built for use with [UAssetToolRivals](https://github.com/XzantGaming/UAssetToolRivals)
