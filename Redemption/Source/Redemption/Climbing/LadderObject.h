// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "LadderObject.generated.h"

UCLASS()
class REDEMPTION_API ALadderObject : public AActor
{
	GENERATED_BODY()
	
public:
	ALadderObject();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LadderObject")
	float SpacingBetweenRungs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LadderObject")
	TArray<UPrimitiveComponent*> RungsArray;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LadderObject")
	int32 NumberOfRungs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LadderObject")
	float RandomHeightScale;

	UFUNCTION(BlueprintCallable, Category = "LadderObject")
	float GetSpacingBetweenRungs() const;

	UFUNCTION(BlueprintCallable, Category = "LadderObject")
	TArray<UPrimitiveComponent*> GetRungsArray() const;

};
