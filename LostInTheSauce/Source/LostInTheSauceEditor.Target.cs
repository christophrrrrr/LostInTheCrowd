using UnrealBuildTool;
using System.Collections.Generic;

public class LostInTheSauceEditorTarget : TargetRules
{
	public LostInTheSauceEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("LostInTheSauce");
	}
}
