// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "SplineActor.generated.h"

class USplineComponent;
class USplineMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class PINBALL_API ASplineActor : public AActor
{
	GENERATED_UCLASS_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASplineActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PreInitializeComponents() override;
	
	/** Spline that represents our shape */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	USplineComponent* SplineComponent;
	
	/** Duplicates the logic from AActor::AddComponent() because that cannot be called from a BP Macro Library or BP Function Library */
	UFUNCTION(BlueprintCallable, Category = Spline)
	USplineMeshComponent* AddPinballSplineMeshComponent(bool bManualAttachment, const FTransform& RelativeTransform, const UObject* ComponentTemplateContex);

	/** Remove all previous spline meshes*/
	UFUNCTION(BlueprintCallable, Category = Spline)
	void DestroyAllSplineMeshes();

	/** Spline meshes that represents our shape */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	TArray<USplineMeshComponent*> SplineMeshComponents;

	/** Whether or not to update the spline meshes every time a property is changed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	bool bUpdateSplineMeshes;

	/** Boolean to reset the spline to your custom default state as defined in the blueprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spline)
	bool bResetSplineToDefault;

#if WITH_EDITOR
	virtual void PreEditUndo() override;
#endif
};
