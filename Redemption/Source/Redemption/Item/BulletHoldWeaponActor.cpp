// Copyright 2022 wevet works All Rights Reserved.


#include "Item/BulletHoldWeaponActor.h"
#include "Character/BaseCharacter.h"
#include "Character/WvPlayerController.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "WvAbilitySystemGlobals.h"
#include "GameExtension.h"
#include "Locomotion/LocomotionComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BulletHoldWeaponActor)

bool ABulletHoldWeaponActor::IsAvailable() const
{
	return !PawnAttackParam.IsEmptyAmmo();
}

void ABulletHoldWeaponActor::DoFire()
{
	if (!Character.IsValid())
	{
		return;
	}

	const float Loudness = PawnAttackParam.Loudness;
	const float Volume = PawnAttackParam.Volume;
	Character->ReportNoiseEvent(FVector::ZeroVector, Volume, Loudness);

	if (IsValid(FireAnimation))
	{
		SkeletalMeshComponent->PlayAnimation(FireAnimation, false);
	}

	DoFireReceived();
}

void ABulletHoldWeaponActor::DoReload()
{
	if (!Character.IsValid())
	{
		return;
	}

	DoReloadReceived();
}

float ABulletHoldWeaponActor::GetGunFireAnimationLength() const
{
	float Total = FireAnimationOffset;
	Total += IsValid(FireAnimation) ? FireAnimation->GetPlayLength() : 0.f;
	return Total;
}

void ABulletHoldWeaponActor::SetGunFirePrepareParameters(const float InRandmize)
{
	Randmize = InRandmize;
}

const bool ABulletHoldWeaponActor::LineOfSightOuter(FHitResult& OutHitResult, FVector& OutTraceEnd)
{
	const bool bIsRandomize = Character->IsBotCharacter();
	const FTransform MuzzleTransform = SkeletalMeshComponent->GetSocketTransform(PawnAttackParam.MuzzleSocketName);
	FVector Forward = FRotator(MuzzleTransform.GetRotation()).Vector();

	if (Character->GetLocomotionComponent())
	{
		auto LocomotionEssencialVariables = Character->GetLocomotionComponent()->GetLocomotionEssencialVariables();
		if (LocomotionEssencialVariables.LSRotationMode != ELSRotationMode::VelocityDirection)
		{
			Forward = LocomotionEssencialVariables.LookingRotation.Vector();
		}
	}

	return LineOfSight(MuzzleTransform.GetLocation(), Forward, bIsRandomize, Randmize, OutHitResult, OutTraceEnd);
}

const bool ABulletHoldWeaponActor::LineOfSight(const FVector TraceStart, const FVector Forward, const bool IsRandomize, const float RandomRadius, FHitResult& OutHitResult, FVector& OutTraceEnd)
{
	if (IsRandomize)
	{
		const float Value = FMath::RandRange(RandomRadius * -1.0f, RandomRadius);
		TraceNoise = FVector(0.f, Value, Value);
	}
	else
	{
		TraceNoise = FVector::ZeroVector;
	}

	FVector Offset = Forward * PawnAttackParam.TraceRange;
	Offset += TraceNoise;
	const FVector TraceEndPosition = TraceStart + Offset;

	TArray<AActor*> IgnoreActors({ Character.Get(), });
	auto TraceType = ABILITY_GLOBAL()->bWeaponTraceDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(ABILITY_GLOBAL()->WeaponTraceChannel);

	FHitResult HitResult(ForceInit);
	const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), TraceStart, TraceEndPosition, 
		ABILITY_GLOBAL()->WeaponTraceChannel, false, IgnoreActors, TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 5.0f);

#if false
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character.Get());
	QueryParams.bReturnPhysicalMaterial = true;
	const bool bHitResult = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEndPosition, CollisionChannel, QueryParams);
	if (ABILITY_GLOBAL()->bWeaponTraceDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEndPosition, FColor::Green, false, 5.0f);
	}
#endif

	OutHitResult = HitResult;
	OutTraceEnd = TraceEndPosition;
	return bHitResult;
}


