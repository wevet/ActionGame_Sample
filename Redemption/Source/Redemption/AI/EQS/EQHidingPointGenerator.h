// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_BlueprintBase.h"
#include "EQHidingPointGenerator.generated.h"

/**
 *
 */
UCLASS()
class REDEMPTION_API UEQHidingPointGenerator : public UEnvQueryGenerator_BlueprintBase
{
	GENERATED_BODY()


public:
	UEQHidingPointGenerator();

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;


protected:
	UFUNCTION(BlueprintCallable, Category = AI)
	void UpdateHidingPoints(const TArray<FVector>& ContextLocations, TArray<FVector>& OutPositions) const;


	/** ���ˏ󃌃C�̍ő勗�� */
	UPROPERTY(EditAnywhere, Category = "Generator")
	float TraceRange = 1000.f;

	/** ���ˏ󃌃C�̊J�n�ʒu���������ɃI�t�Z�b�g (Z��) */
	UPROPERTY(EditAnywhere, Category = "Generator")
	float StartZOffset = -10.f;

	/** SphereTrace �̔��a */
	UPROPERTY(EditAnywhere, Category = "Generator")
	float TraceRadius = 50.f;

	/** �q�b�g�����ۂɖ@�������։����߂����� */
	UPROPERTY(EditAnywhere, Category = "Generator")
	float OffsetFromWall = 50.f;

	/** �����蔻��Ɏg���`�����l�� */
	UPROPERTY(EditAnywhere, Category = "Generator")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category = "Generator")
	bool bIsShowTrace{ false };


	UPROPERTY(EditAnywhere, Category = "Generator")
	float TraceTime = 5.0f;
};

