using UnrealBuildTool;

public class ChameleonPainterEditor : ModuleRules
{
	public ChameleonPainterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"AssetRegistry",
				"AssetTools",
				"BlueprintGraph",
				"ChameleonPainter",
				"Core",
				"CoreUObject",
				"EditorFramework",
				"Engine",
				"EngineSettings",
				"EnhancedInput",
				"InputCore",
				"InputEditor",
				"KismetCompiler",
				"MaterialEditor",
				"ProceduralMeshComponent",
				"Slate",
				"SlateCore",
				"UMG",
				"UnrealEd"
			});
	}
}
