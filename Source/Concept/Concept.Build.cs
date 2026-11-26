// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Concept : ModuleRules
{
	public Concept(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets every file include headers as "Proto/..." from the module root.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"PhysicsCore"
		});
	}
}
