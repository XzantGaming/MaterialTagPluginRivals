#include "RivalsMeshMaterialManager.h"
#include "MaterialTagAssetUserData.h"
#include "HiddenMaterialDefaultsAssetUserData.h"

#if WITH_EDITOR
#include "PropertyEditorModule.h"
#include "MaterialSlotTagEntryCustomization.h"
#include "MaterialTagUserDataCustomization.h"
#include "HiddenMaterialEntryCustomization.h"
#include "HiddenMaterialDefaultsCustomizations.h"
#endif

#define LOCTEXT_NAMESPACE "FRivalsMeshMaterialManagerModule"

void FRivalsMeshMaterialManagerModule::StartupModule()
{
#if WITH_EDITOR
	// Register custom property type customization for FMaterialSlotTagEntry
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FMaterialSlotTagEntry::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMaterialSlotTagEntryCustomization::MakeInstance)
	);

	// Register property type customization for FPresetTagDisplay (draggable tag pills)
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FPresetTagDisplay::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPresetTagDisplayCustomization::MakeInstance)
	);

	// Register property type customization for FHiddenMaterialEntry (Visible/Hidden toggle button)
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FHiddenMaterialEntry::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FHiddenMaterialEntryCustomization::MakeInstance)
	);

	// Register property type customization for FHiddenMaterialLOD ("LOD N" header)
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FHiddenMaterialLOD::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FHiddenMaterialLODCustomization::MakeInstance)
	);

	// Register class detail customization for UHiddenMaterialDefaultsAssetUserData
	// (auto-populates the LOD/Materials arrays from the mesh when the panel opens)
	PropertyModule.RegisterCustomClassLayout(
		UHiddenMaterialDefaultsAssetUserData::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FHiddenMaterialDefaultsDetailCustomization::MakeInstance)
	);
#endif
}

void FRivalsMeshMaterialManagerModule::ShutdownModule()
{
#if WITH_EDITOR
	// Unregister custom property type customization
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(FMaterialSlotTagEntry::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FPresetTagDisplay::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FHiddenMaterialEntry::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FHiddenMaterialLOD::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomClassLayout(UHiddenMaterialDefaultsAssetUserData::StaticClass()->GetFName());
	}
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRivalsMeshMaterialManagerModule, RivalsMeshMaterialManager)
