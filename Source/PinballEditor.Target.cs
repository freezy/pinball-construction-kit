// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class PinballEditorTarget : TargetRules
{
	public PinballEditorTarget(TargetInfo Target)
		: base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("Pinball");
		ExtraModuleNames.Add("PinballEditor");
	}
}
