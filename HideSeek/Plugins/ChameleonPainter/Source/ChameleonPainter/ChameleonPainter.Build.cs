using UnrealBuildTool;

public class ChameleonPainter : ModuleRules
{
	public ChameleonPainter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"EnhancedInput",
				"InputCore",
				"ProceduralMeshComponent",
				"UMG"
			});

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Slate",
				"SlateCore"
			});
	}
}
