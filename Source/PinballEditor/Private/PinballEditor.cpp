// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PinballEditor.h"
#include "../PropertyEditor/Public/PropertyEditorModule.h"
#include "WallActor.h"
#include "SplineActorDetailsCustomization.h"
 
class FPinballEditor : public IPinballEditorModule
{
public:
	FPinballEditor()
	{
	}

public:
	virtual void StartupModule() override
	{
		// Register the details customizations
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.RegisterCustomClassLayout(ASplineActor::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FSplineActorDetailsCustomization::MakeInstance));
			//PropertyModule.RegisterCustomPropertyTypeLayout(AWallActor::StaticClass()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWallActorDetailsCustomization::MakeInstance));

			PropertyModule.NotifyCustomizationModuleChanged();
		}
	}

	virtual void ShutdownModule() override
	{
		// Unregister the details customization
		//@TODO: Unregister them
	}
};

//IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, PinballEditor, "PinballEditor");

IMPLEMENT_MODULE(FPinballEditor, PinballEditor);
