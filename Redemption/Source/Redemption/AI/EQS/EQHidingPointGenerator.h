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


	/** 放射状レイの最大距離 */
	UPROPERTY(EditAnywhere, Category = "Generator")
	float TraceRange = 1000.f;

	/** 放射状レイの開始位置を下方向にオフセット (Z軸) */
	UPROPERTY(EditAnywhere, Category = "Generator")
	float StartZOffset = -10.f;

	/** SphereTrace の半径 */
	UPROPERTY(EditAnywhere, Category = "Generator")
	float TraceRadius = 50.f;

	/** ヒットした際に法線方向へ押し戻す距離 */
	UPROPERTY(EditAnywhere, Category = "Generator")
	float OffsetFromWall = 50.f;

	/** 当たり判定に使うチャンネル */
	UPROPERTY(EditAnywhere, Category = "Generator")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category = "Generator")
	bool bIsShowTrace{ false };


	UPROPERTY(EditAnywhere, Category = "Generator")
	float TraceTime = 5.0f;
};

