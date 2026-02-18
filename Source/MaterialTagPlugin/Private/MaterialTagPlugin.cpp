#include "MaterialTagPlugin.h"
#include "MaterialTagAssetUserData.h"

#if WITH_EDITOR
#include "PropertyEditorModule.h"
#include "MaterialSlotTagEntryCustomization.h"
#include "MaterialTagUserDataCustomization.h"
#endif

#define LOCTEXT_NAMESPACE "FMaterialTagPluginModule"

void FMaterialTagPluginModule::StartupModule()
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
#endif
}

void FMaterialTagPluginModule::ShutdownModule()
{
#if WITH_EDITOR
	// Unregister custom property type customization
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(FMaterialSlotTagEntry::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FPresetTagDisplay::StaticStruct()->GetFName());
	}
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMaterialTagPluginModule, MaterialTagPlugin)
