// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class learnUnrealForOnce : ModuleRules
{
	public learnUnrealForOnce(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
