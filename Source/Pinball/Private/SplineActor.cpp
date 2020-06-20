// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SplineActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

ASplineActor::ASplineActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bUpdateSplineMeshes(true)
{
	SetFlags(RF_Transactional);

	SplineComponent = ObjectInitializer.CreateDefaultSubobject < USplineComponent >(this, TEXT("SplineComp"));
	//static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshOb(TEXT("/Game/Meshes/TemplateCube_Rounded.TemplateCube_Rounded"));
	USceneComponent* SceneComponent = ObjectInitializer.CreateDefaultSubobject < USceneComponent >(this, TEXT("SceneComp"));
	RootComponent = SceneComponent;

	SplineComponent->AttachToComponent(SceneComponent, FAttachmentTransformRules::KeepRelativeTransform);

	SplineComponent->AddSplinePoint(FVector(100, 100, 0), ESplineCoordinateSpace::Local);
	SplineComponent->AddSplinePoint(FVector(0, 100, 0), ESplineCoordinateSpace::Local);

	SplineComponent->SetClosedLoop(true);
	SplineComponent->bSplineHasBeenEdited = true;

	SceneComponent->SetMobility(EComponentMobility::Static);
	SplineComponent->SetMobility(EComponentMobility::Movable);

	PrimaryActorTick.bCanEverTick = false;
} 

// Sets default values
ASplineActor::ASplineActor()
	: bUpdateSplineMeshes(true)
{
	// turned this off to improve performance
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ASplineActor::BeginPlay()
{
	Super::BeginPlay();
}

void ASplineActor::PreInitializeComponents()
{
	SplineComponent->RegisterComponent();
}

USplineMeshComponent* ASplineActor::AddPinballSplineMeshComponent(bool bManualAttachment, const FTransform& RelativeTransform, const UObject* ComponentTemplateContex)
{
		bool bIsSceneComponent = false;
		UActorComponent* NewActorComp = NewObject<USplineMeshComponent>(this);
		if (NewActorComp != nullptr)
		{
			// Call function to notify component it has been created
			NewActorComp->OnComponentCreated();
	
			// The user has the option of doing attachment manually where they have complete control or via the automatic rule
			// that the first component added becomes the root component, with subsequent components attached to the root.
			USceneComponent* NewSceneComp = Cast<USceneComponent>(NewActorComp);
			if (NewSceneComp != nullptr)
			{
				if (!bManualAttachment)
				{
					if (RootComponent == nullptr)
					{
						RootComponent = NewSceneComp;
					}
					else
					{
						NewSceneComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
					}
				}
	
				NewSceneComp->SetRelativeTransform(RelativeTransform);
	
				bIsSceneComponent = true;
			}
	
			// Register component, which will create physics/rendering state, now component is in correct position
			NewActorComp->RegisterComponent();
	
			UWorld* World = GetWorld();
			if (!IsRunningUserConstructionScript() && World && bIsSceneComponent)
			{
				UPrimitiveComponent* NewPrimitiveComponent = Cast<UPrimitiveComponent>(NewActorComp);
				if (NewPrimitiveComponent)
				{
					World->UpdateCullDistanceVolumes(this, NewPrimitiveComponent);
				}
			}
		}

		USplineMeshComponent* SplineMeshComponent = Cast<USplineMeshComponent>(NewActorComp);

		SplineMeshComponent->SetMobility(EComponentMobility::Static);

		SplineMeshComponents.Add(SplineMeshComponent);
	
		return SplineMeshComponent;
}

void ASplineActor::DestroyAllSplineMeshes()
{
	for (int32 SplineMeshIndex = SplineMeshComponents.Num() - 1; SplineMeshIndex >= 0; --SplineMeshIndex)
	{
		USplineMeshComponent* SplineMeshComponent = SplineMeshComponents[SplineMeshIndex];
		SplineMeshComponents[SplineMeshIndex] = nullptr;
		if (SplineMeshComponent != nullptr)
		{
			SplineMeshComponent->DestroyComponent();
		}
	}

	SplineMeshComponents.Empty();
}

#if WITH_EDITOR
void ASplineActor::PreEditUndo()
{
	DestroyAllSplineMeshes();
}
#endif
