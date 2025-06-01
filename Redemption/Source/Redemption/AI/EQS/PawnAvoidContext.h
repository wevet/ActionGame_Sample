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
	/** LineTrace �̊J�n�I�t�Z�b�g�i������j */
	UPROPERTY(EditAnywhere, Category = "EQS|PawnAvoid")
	float TraceUpHeight = 50.f;

	/** LineTrace �̏I���I�t�Z�b�g�i���������n�ʒT���̒����j */
	UPROPERTY(EditAnywhere, Category = "EQS|PawnAvoid")
	float TraceDownLength = 100.f;

	/** Pawn �����o�����ꍇ�ɉ����߂����� */
	UPROPERTY(EditAnywhere, Category = "EQS|PawnAvoid")
	float PushBackDistance = 100.f;


};
