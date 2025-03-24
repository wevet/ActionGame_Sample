// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SignificanceManager.h"
#include "SignificanceConfig.h"
#include "SignificanceComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API USignificanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USignificanceComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:
	static float K2_SignificanceFunction(USignificanceManager::FManagedObjectInfo* ManagedObjectInfo, const FTransform& Viewpoint);


public:
	UPROPERTY(EditDefaultsOnly)
	FName Tag = FName("Default");

	/** �d�v�x�F�L�����N�^�[�̉��Z�̎��𒲐�������̂ŁA���x�����Ⴂ�قǎ��������Ȃ�B */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int32 SignificanceLevel = -1;

	/** Significance ��ʋ�ԂŃL�����N�^�[���ǂꂾ���傫�������邩������Fruit */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	ESignificanceFruitType SignificanceFruitType = ESignificanceFruitType::Melon;

	/** �p�t�H�[�}���X�E���x���̈قȂ�\�� */
	UPROPERTY(BlueprintReadOnly)
	TArray<FSignificanceConfigType2> SignificanceConfigArray;

public:
	int32 Significance_CharacterBaseBattleMinLevel = 0;
	int32 Significance_CharacterBaseAIUnlockLevel = 0;
	int32 Significance_CharacterBaseMeshVisibleLevel = 0;
	int32 Significance_EnvironmentCreatureBaseBattleMinLevel = 0;
};
