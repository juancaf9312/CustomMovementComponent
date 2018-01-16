// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MovementComponent : ModuleRules
{
	public MovementComponent(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "OculusHMD" });

        PublicIncludePathModuleNames.AddRange(new string[] { "OculusHMD" });

        PublicIncludePaths.AddRange(new string[] { "OculusHMD/Public" });
    }
}
