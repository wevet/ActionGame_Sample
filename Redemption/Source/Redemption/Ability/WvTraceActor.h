// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "WvTraceActor.generated.h"

UCLASS()
class REDEMPTION_API AWvTraceActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AWvTraceActor(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:	
	TArray<UShapeComponent*> ShapeComponents;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* CapsuleCom0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* CapsuleCom1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* CapsuleCom2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* CapsuleCom3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UCapsuleComponent* CapsuleCom4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* BoxCom5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* BoxCom6;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* BoxCom7;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* BoxCom8;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* BoxCom9;
};


