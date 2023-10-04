// Copyright 2022 wevet works All Rights Reserved.

#include "Ability/WvTraceActor.h"


AWvTraceActor::AWvTraceActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	ShapeComponents.Empty();
	CapsuleCom0 = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, "CapsuleCom0");
	this->SetRootComponent(CapsuleCom0);
	CapsuleCom0->CreationMethod = EComponentCreationMethod::Instance;
	ShapeComponents.Add(CapsuleCom0);

	CapsuleCom1 = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, "CapsuleCom1");
	CapsuleCom1->CreationMethod = EComponentCreationMethod::Instance;
	CapsuleCom1->SetupAttachment(RootComponent);
	ShapeComponents.Add(CapsuleCom1);

	CapsuleCom2 = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, "CapsuleCom2");
	CapsuleCom2->CreationMethod = EComponentCreationMethod::Instance;
	CapsuleCom2->SetupAttachment(RootComponent);
	ShapeComponents.Add(CapsuleCom2);

	CapsuleCom3 = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, "CapsuleCom3");
	CapsuleCom3->CreationMethod = EComponentCreationMethod::Instance;
	CapsuleCom3->SetupAttachment(RootComponent);
	ShapeComponents.Add(CapsuleCom3);

	CapsuleCom4 = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, "CapsuleCom4");
	CapsuleCom4->CreationMethod = EComponentCreationMethod::Instance;
	CapsuleCom4->SetupAttachment(RootComponent);
	ShapeComponents.Add(CapsuleCom4);

	BoxCom5 = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, "BoxCom5");
	BoxCom5->CreationMethod = EComponentCreationMethod::Instance;
	BoxCom5->SetupAttachment(RootComponent);
	ShapeComponents.Add(BoxCom5);

	BoxCom6 = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, "BoxCom6");
	BoxCom6->CreationMethod = EComponentCreationMethod::Instance;
	BoxCom6->SetupAttachment(RootComponent);
	ShapeComponents.Add(BoxCom6);

	BoxCom7 = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, "BoxCom7");
	BoxCom7->CreationMethod = EComponentCreationMethod::Instance;
	BoxCom7->SetupAttachment(RootComponent);
	ShapeComponents.Add(BoxCom7);

	BoxCom8 = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, "BoxCom8");
	BoxCom8->CreationMethod = EComponentCreationMethod::Instance;
	BoxCom8->SetupAttachment(RootComponent);
	ShapeComponents.Add(BoxCom8);

	BoxCom9 = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(this, "BoxCom9");
	BoxCom9->CreationMethod = EComponentCreationMethod::Instance;
	BoxCom9->SetupAttachment(RootComponent);
	ShapeComponents.Add(BoxCom9);
}

void AWvTraceActor::BeginPlay()
{
	Super::BeginPlay();
	Super::SetActorTickEnabled(false);
}

void AWvTraceActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

