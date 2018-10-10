// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PinballEditor : ModuleRules
{
	public PinballEditor(ReadOnlyTargetRules Target)
		: base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UnrealEd", "InputCore"});
		PrivateDependencyModuleNames.AddRange(new string[] 
		{ 
			"PropertyEditor",
			"Pinball",
			"RenderCore",
			"Slate",
			"SlateCore",
			"EditorStyle"
		}
		);

		PrivateIncludePathModuleNames.AddRange(new string[] 
		{ 
			"PropertyEditor",
			"Pinball",
			"RenderCore",
			"Slate",
			"SlateCore",
		}
		);
	}
}
