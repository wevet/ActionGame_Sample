// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAbilitySystemTypes.h"
#include "NiagaraSystem.h"
#include "WvAbilityType.generated.h"


USTRUCT(BlueprintType)
struct FCustomWvAbilitySystemAvatarData : public FWvAbilitySystemAvatarData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	TSoftObjectPtr<UDataTable> LocomotionAbilityTable;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	TSoftObjectPtr<UDataTable> FieldAbilityTable;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	//TSoftObjectPtr<UDataTable> BattleAbilityTable;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	TArray<TSoftObjectPtr<UDataTable>> FunctionAbilityTables;

public:

	FCustomWvAbilitySystemAvatarData()
	{
		LocomotionAbilityTable = nullptr;
		FieldAbilityTable = nullptr;
		//BattleAbilityTable = nullptr;
	}
};

USTRUCT(Blueprintable)
struct FCharacterBaseParameter
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Health = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float HealthMax = 0.0f;

	void Init()
	{
		Health = 0.0f;
		HealthMax = 0.0f;
	}
};


#pragma region GameplayCue
UENUM(BlueprintType)
enum class EParticleRotationMode : uint8
{
	// 設定された回転値を直接使用する
	None,
	// 衝突点の法線に重ねた回転値
	AddToNormal,
	// ダメージが発生した場所に向かって
	HitDirection,
	// エフェクトが攻撃側を向いた後の回転値をスタックする。
	FaceToAttacker,
	// 攻撃側の正面を突く
	AttackerForward,
	// 被弾者の逆向き
	BeHitedBackward,
};

USTRUCT(BlueprintType)
struct FParticleCueConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UNiagaraSystem* NiagaraSystem{ nullptr };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FVector PositionOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FRotator RotationOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FVector ParticleScale {
		FVector::OneVector
	};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool IsAttachInBone{ false };
};

USTRUCT(BlueprintType)
struct FWvAttackCueConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FParticleCueConfig Particle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FName FixedAttachBoneName;

	UPROPERTY(EditDefaultsOnly)
	EParticleRotationMode RotationMode {
		EParticleRotationMode::AttackerForward
	};

	UPROPERTY(EditDefaultsOnly)
	bool OnlyYaw{ true };
};

USTRUCT(BlueprintType)
struct FWvDamageCueConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FParticleCueConfig Particle;

	UPROPERTY(EditDefaultsOnly)
	EParticleRotationMode RotationMode;

	UPROPERTY(EditDefaultsOnly)
	bool OnlyYaw{ true };
};

USTRUCT(BlueprintType)
struct FWvAttackCueConfigRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AvatarTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag CueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FWvAttackCueConfig CueConfig;
};

USTRUCT(BlueprintType)
struct FWvDamageCueConfigRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AvatarTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag CueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FWvDamageCueConfig CueConfig;
};

UCLASS()
class UWvCueConfigDataAssest : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UDataTable* AttackCueConfigTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UDataTable* DamageCueConfigTable;

public:
	bool GetAttackCueConfigRow(const FGameplayTag& AvatarTag, const FGameplayTag& CueTag, FWvAttackCueConfigRow& CueConfigRow);
	bool GetDamageCueConfigRow(const FGameplayTag& AvatarTag, const FGameplayTag& CueTag, FWvDamageCueConfigRow& CueConfigRow);

};
#pragma endregion


