using UnrealBuildTool;

public class HideSeekEditor : ModuleRules
{
	public HideSeekEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AssetRegistry",
			"ChameleonPainter",
			"Core",
			"CoreUObject",
			"EditorFramework",
			"Engine",
			"EngineSettings",
			"HideSeek",
			"KismetCompiler",
			"NavigationSystem",
			"Niagara",
			"UnrealEd"
		});
	}
}
