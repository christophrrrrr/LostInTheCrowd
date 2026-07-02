using UnrealBuildTool;
using System.Collections.Generic;

public class LostInTheSauceTarget : TargetRules
{
	public LostInTheSauceTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("LostInTheSauce");
	}
}
