// Copyright 2022 wevet works All Rights Reserved.

#include "WvSpringArmComponent.h"
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
		//壁と衝突したのでCurrentHitReturnInterpTimeをHitReturnInterpTimeにリセット
		FVector Interp = FMath::VInterpTo(PrevHitInterpLoc, TraceHitLocation, DeltaTime, HitInterpSpeed);
		PrevHitInterpLoc = Interp;
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

