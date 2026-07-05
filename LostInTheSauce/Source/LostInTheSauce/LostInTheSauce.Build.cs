using UnrealBuildTool;

public class LostInTheSauce : ModuleRules
{
	public LostInTheSauce(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"AIModule",
			"NavigationSystem",
			"Slate",
			"SlateCore",
			"UMG"
		});
	}
}
