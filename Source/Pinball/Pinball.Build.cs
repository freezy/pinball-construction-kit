// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Pinball : ModuleRules
{
	public Pinball(ReadOnlyTargetRules Target)
		: base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore"}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"RenderCore",
				"Slate",
				"SlateCore",
				"Paper2D",
			}
		);
	}
}
