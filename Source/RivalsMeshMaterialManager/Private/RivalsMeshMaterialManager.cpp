#include "RivalsMeshMaterialManager.h"
#include "MaterialTagAssetUserData.h"
#include "HiddenMaterialsAssetUserData.h"

#if WITH_EDITOR
#include "PropertyEditorModule.h"
#include "MaterialSlotTagEntryCustomization.h"
#include "MaterialTagUserDataCustomization.h"
#include "LODHiddenMaterialsCustomization.h"
#include "HiddenMaterialsAssetUserDataDetails.h"
#endif

#define LOCTEXT_NAMESPACE "FRivalsMeshMaterialManagerModule"

void FRivalsMeshMaterialManagerModule::StartupModule()
{
#if WITH_EDITOR
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register custom property type customization for FMaterialSlotTagEntry
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FMaterialSlotTagEntry::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMaterialSlotTagEntryCustomization::MakeInstance)
	);

	// Register property type customization for FPresetTagDisplay (draggable tag pills)
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FPresetTagDisplay::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPresetTagDisplayCustomization::MakeInstance)
	);

	// Register property type customization for FLODHiddenMaterials (per-LOD hidden material checkboxes)
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FLODHiddenMaterials::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLODHiddenMaterialsCustomization::MakeInstance)
	);

	// Register class detail customization for UHiddenMaterialsAssetUserData (preset UI, slot grid, etc.)
	PropertyModule.RegisterCustomClassLayout(
		UHiddenMaterialsAssetUserData::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FHiddenMaterialsAssetUserDataDetails::MakeInstance)
	);

#endif
}

void FRivalsMeshMaterialManagerModule::ShutdownModule()
{
#if WITH_EDITOR
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(FMaterialSlotTagEntry::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FPresetTagDisplay::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FLODHiddenMaterials::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomClassLayout(UHiddenMaterialsAssetUserData::StaticClass()->GetFName());
	}
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRivalsMeshMaterialManagerModule, RivalsMeshMaterialManager)
