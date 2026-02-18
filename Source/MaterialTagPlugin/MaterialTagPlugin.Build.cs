using UnrealBuildTool;

public class MaterialTagPlugin : ModuleRules
{
	public MaterialTagPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags"
			}
		);
		
		// Editor-only modules for property customization
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Slate",
					"SlateCore",
					"InputCore",
					"PropertyEditor",
					"UnrealEd"
				}
			);
		}
	}
}
