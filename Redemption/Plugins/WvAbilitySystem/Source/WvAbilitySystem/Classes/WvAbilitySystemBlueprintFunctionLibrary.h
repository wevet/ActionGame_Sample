// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interface/WvSummonActorInterface.h"
#include "Interface/WvAbilityTargetInterface.h"
#include "WvAbilitySystemTypes.h"
#include "Abilities/GameplayAbility.h"
#include "WvAbilitySystemComponentBase.h"
#include "WvGameplayTargetData.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CollisionQueryParams.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.generated.h"


UCLASS()
class WVABILITYSYSTEM_API UWvAbilitySystemBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static UWvAbilityDataAsset* GetAbilityData(const UGameplayAbility* Ability);

	static UGameplayEffect* NewModifyAttributeGE(FGameplayAttribute& Attribute, const float Magnitude, const FGameplayTag& GETag, UObject* Outer = nullptr);

	//--------filter start
	static void FilterOverlaps(const TArray<FWvOverlapResult>& Overlaps, TArray<FWvOverlapResult>& OutOverlaps, TScriptInterface<IWvAbilityTargetInterface> Source, const FWvTargetDataFilter& TargetDataFilter);
	static void FilterActors(const TArray<AActor*>& Actors, TArray<AActor*>& OutActors, TScriptInterface<IWvAbilityTargetInterface> Source, const FWvTargetDataFilter& TargetDataFilter);
	//--------filter end

	static FGameplayEffectContextHandle EffectContextSetEffectDataAsset(FGameplayEffectContextHandle EffectContextHandle, UWvAbilityEffectDataAsset* DataAsset, const int32 Index);
	static FGameplayAbilityTargetDataHandle MakeTargetDataHandleFromHitResults(const TArray<FHitResult>& Hits, FVector SourceLocation, AActor* AbilityOwner, FWvAbilityData EffectAbilityData, AActor* HitActor);
	static FGameplayAbilityTargetDataHandle MakeTargetDataHandleFromHitResultsAndSlicePlaneNormal(const TArray<FHitResult>& Hits, FVector SourceLocation, FVector SlicePlaneNormal, FWvAbilityData EffectAbilityData, AActor* HitActor);

	static FCollisionQueryParams ConfigureCollisionParams(const FName TraceTag, const bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, const bool bIgnoreSelf, UObject* WorldContextObject);
	static void DrawDebugCapsuleTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, const FQuat CapsuleQuat, const float Radius, const float HalfHeight, const int32 DrawDebugTypeIndex, bool bHit, const TArray<FHitResult>& OutHits, const FLinearColor TraceColor, const FLinearColor TraceHitColor, const float DrawTime);
	static FWvGameplayAbilityTargetData* GetGameplayAbilityTargetData(FGameplayEffectContextHandle EffectContextHandle);

	static bool GetGameplayAbilityTargetData(FGameplayEffectContextHandle EffectContextHandle, FWvGameplayAbilityTargetData& TargetData);
	static bool EffectContextGetEffectGroup(FGameplayEffectContextHandle EffectContextHandle, FOnceApplyEffect& EffectGroup);
	static class UApplyEffectExData* GetOnecEffectExData(FOnceApplyEffect& ApplyEffect);

	static void SetAttributeSetValue(UWvAbilitySystemComponentBase* ASC, FGameplayAttribute Attribute, const float InValue);
	static void KillMySelf(AActor* Actor);
	static void RecoverHP(AActor* Actor, const float Value);
	static void RecoverPercentHP(AActor* Actor, const float Percent);
	static void SetDamage(AActor* Actor, const float Value);

	static void FullSkill(AActor* Actor);

	static class UApplyEffectExData* GetEffectExData(FGameplayEffectContextHandle ContextHandle);

	static EHitVerticalDirection EvaluteHitVerticalDirection(const FVector Direction);

	static EHitReactDirection EvaluteHitReactDirection(const float Angle);

	static FVector GetAttackDirection(FGameplayEffectContextHandle EffectContextHandle, const FVector ActorLocation);


	/**
	 * TagContainer の全タグをログに出力します。
	 * @param Container  出力対象のタグコンテナ
	 */
	static void LogTagContainer(const FGameplayTagContainer& Container);


};

