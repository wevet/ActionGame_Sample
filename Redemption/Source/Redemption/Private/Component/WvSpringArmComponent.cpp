// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/WvSpringArmComponent.h"
#include "DrawDebugHelpers.h"
#include "PhysicsEngine/PhysicsSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvSpringArmComponent)

UWvSpringArmComponent::UWvSpringArmComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FVector UWvSpringArmComponent::BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething, float DeltaTime)
{
	if (bHitSomething)
	{
		FVector Interp = FMath::VInterpTo(PrevHitInterpLoc, TraceHitLocation, DeltaTime, HitInterpSpeed);
		PrevHitInterpLoc = Interp;
		//壁と衝突したのでCurrentHitReturnInterpTimeをHitReturnInterpTimeにリセット
		CurrentHitReturnInterpTime = HitReturnInterpTime;
		return Interp;
	}

	// 壁ぶつかり終了後の復帰補間
	if (CurrentHitReturnInterpTime > 0.0f)
	{
		CurrentHitReturnInterpTime -= DeltaTime;
		FVector Interp = FMath::VInterpTo(PrevHitInterpLoc, DesiredArmLocation, 1.0f - (CurrentHitReturnInterpTime / HitReturnInterpTime), 1.0f);
		PrevHitInterpLoc = Interp;
		return Interp;
	}

	// 上の条件に入らなかったのでArmは伸びきっている状態のはず
	PrevHitInterpLoc = DesiredArmLocation;
	return DesiredArmLocation;
}

