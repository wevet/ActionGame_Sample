// Copyright 2022 wevet works All Rights Reserved.


#include "Climbing/InteractionObject.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionObject)


AInteractionObject::AInteractionObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComponent"));
	RootComponent = SceneComponent;

	StaticMeshComponent = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->bAutoActivate = 1;
}

void AInteractionObject::BeginPlay()
{
	Super::BeginPlay();
	Super::SetActorTickEnabled(false);

	StaticMeshComponent->AddTickPrerequisiteActor(this);
}

UStaticMeshComponent* AInteractionObject::GetStaticMeshComponent() const
{
	return StaticMeshComponent;
}


void AInteractionObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void AInteractionObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
}
#endif

