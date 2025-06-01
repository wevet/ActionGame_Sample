// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "PawnAvoidContext.generated.h"

/**
 *
 */
UCLASS()
class REDEMPTION_API UPawnAvoidContext : public UEnvQueryContext
{
	GENERATED_BODY()


public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;

protected:
	/** LineTrace の開始オフセット（上方向） */
	UPROPERTY(EditAnywhere, Category = "EQS|PawnAvoid")
	float TraceUpHeight = 50.f;

	/** LineTrace の終了オフセット（下方向＝地面探索の長さ） */
	UPROPERTY(EditAnywhere, Category = "EQS|PawnAvoid")
	float TraceDownLength = 100.f;

	/** Pawn を検出した場合に押し戻す距離 */
	UPROPERTY(EditAnywhere, Category = "EQS|PawnAvoid")
	float PushBackDistance = 100.f;


};
