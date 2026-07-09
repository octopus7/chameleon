// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HideSeek : ModuleRules
{
	public HideSeek(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"ChameleonPainter",
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"GameplayTasks",
			"InputCore",
			"NavigationSystem",
			"Niagara",
			"Slate",
			"SlateCore",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
