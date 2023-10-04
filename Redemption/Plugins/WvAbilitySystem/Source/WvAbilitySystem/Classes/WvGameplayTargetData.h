// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WvAbilitySystemTypes.h"
#include "WvGameplayTargetData.generated.h"


USTRUCT(BlueprintType)
struct WVABILITYSYSTEM_API FWvGameplayAbilityTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize SourceLocation;

public:
	UPROPERTY()
	FWvBattleDamageAttackTargetInfo TargetInfo;

	UPROPERTY()
	FWvBattleDamageAttackSourceInfo SourceInfo;

	FVector SourceActorLocation;
	float TotalDamage{ 0 };
	float WeaknessDamge{ 0 };


public:

	virtual bool HasHitResult() const override;
	virtual const FHitResult* GetHitResult() const override;

	virtual bool HasOrigin() const override;
	virtual FTransform GetOrigin() const override;

	virtual AActor* GetTargetActor() const;

	virtual bool GetSlicePlaneNormal(FVector& OutNormal) const;
	virtual TArray<TWeakObjectPtr<AActor> >	GetActors() const override;
	virtual UPrimitiveComponent* GetComponent() const;
	virtual int32 GetBoneIndex() const;

	virtual TArray<FActiveGameplayEffectHandle> ApplyGameplayEffectSpec(FGameplayEffectSpec& Spec, FPredictionKey PredictionKey = FPredictionKey()) override;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FWvGameplayAbilityTargetData::StaticStruct();
	}
};


USTRUCT(BlueprintType)
struct WVABILITYSYSTEM_API FWvGameplayAbilityTargetData_SingleTarget : public FWvGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	FWvOverlapResult Overlap;

public:

	virtual bool GetSlicePlaneNormal(FVector& OutNormal) const override;
	virtual UPrimitiveComponent* GetComponent() const override;
	virtual TArray<TWeakObjectPtr<AActor> >	GetActors() const override;
	virtual AActor* GetTargetActor() const override;
	virtual int32 GetBoneIndex() const override;
	virtual bool HasEndPoint() const override;
	virtual FVector GetEndPoint() const override;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FWvGameplayAbilityTargetData_SingleTarget::StaticStruct();
	}
};
template<>
struct TStructOpsTypeTraits<FWvGameplayAbilityTargetData_SingleTarget> : public TStructOpsTypeTraitsBase2<FWvGameplayAbilityTargetData_SingleTarget>
{
	enum
	{
		WithNetSerializer = true
	};
};


USTRUCT(BlueprintType)
struct WVABILITYSYSTEM_API FWvGameplayAbilityTargetData_SingleTargetHit : public FWvGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	FHitResult HitResult;

	UPROPERTY()
	FVector_NetQuantize SlicePlaneNormal;

	UPROPERTY()
	FWvAbilityData AbilityData;

	UPROPERTY()
	TWeakObjectPtr<AActor> HitActor;

public:

	virtual TArray<TWeakObjectPtr<AActor> >	GetActors() const override;
	virtual AActor* GetTargetActor() const override;
	virtual bool HasHitResult() const override;
	virtual const FHitResult* GetHitResult() const override;
	virtual bool GetSlicePlaneNormal(FVector& OutNormal) const override;
	virtual UPrimitiveComponent* GetComponent() const override;
	virtual int32 GetBoneIndex() const override;
	virtual bool HasEndPoint() const override;
	virtual FVector GetEndPoint() const override;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual TArray<FActiveGameplayEffectHandle> ApplyGameplayEffectSpec(FGameplayEffectSpec& Spec, FPredictionKey PredictionKey = FPredictionKey()) override;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FWvGameplayAbilityTargetData_SingleTargetHit::StaticStruct();
	}
};
template<>
struct TStructOpsTypeTraits<FWvGameplayAbilityTargetData_SingleTargetHit> : public TStructOpsTypeTraitsBase2<FWvGameplayAbilityTargetData_SingleTargetHit>
{
	enum
	{
		WithNetSerializer = true
	};
};

