// Copyright 2022 wevet works All Rights Reserved.


#include "Item/BulletHoldWeaponActor.h"
#include "Character/BaseCharacter.h"
#include "Character/WvPlayerController.h"
#include "Character/PlayerCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "WvAbilitySystemGlobals.h"
#include "GameExtension.h"
#include "Locomotion/LocomotionComponent.h"
#include "Redemption.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "Camera/CameraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BulletHoldWeaponActor)

ABulletHoldWeaponActor::ABulletHoldWeaponActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Priority = 1;
}

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

	if (PawnAttackParam.IsEmptyAmmo())
	{
		UE_LOG(LogTemp, Warning, TEXT("IsEmptyAmmo => %s"), *FString(__FUNCTION__));
		return;
	}

	const float Loudness = PawnAttackParam.Loudness;
	const float Volume = PawnAttackParam.Volume;
	Character->ReportNoiseEvent(FVector::ZeroVector, Volume, Loudness);
	if (IsValid(FireAnimation))
	{
		SkeletalMeshComponent->PlayAnimation(FireAnimation, false);
	}
	PawnAttackParam.DecrementAmmos();
	DoFireReceived();

	// if ammo becomes empty as a result of bullet fire
	if (PawnAttackParam.IsEmptyCurrentAmmo())
	{
		Notify_AmmoEmpty();
	}

}

const bool ABulletHoldWeaponActor::HandleAttackPrepare()
{
	if (Character.IsValid())
	{
		if (IsCurrentAmmosEmpty())
		{
			if (Character->GetWvAbilitySystemComponent()->HasMatchingGameplayTag(TAG_Character_Action_GunReload))
			{
				// reloading..
				return false;
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("reload ability start => %s"), *FString(__FUNCTION__));
				Notify_ReloadOwner();
				return false;
			}
		}
		else
		{
			return true;
		}
	}
	return Super::HandleAttackPrepare();
}

bool ABulletHoldWeaponActor::IsCurrentAmmosEmpty() const
{
	return PawnAttackParam.IsEmptyCurrentAmmo();
}

void ABulletHoldWeaponActor::Notify_AmmoEmpty()
{
	if (Character.IsValid())
	{
		Character->GetWvAbilitySystemComponent()->AddGameplayTag(AmmoEmptyTag, 1);
	}
}

void ABulletHoldWeaponActor::Notify_AmmoReplenishment()
{
	if (Character.IsValid())
	{
		Character->GetWvAbilitySystemComponent()->RemoveGameplayTag(AmmoEmptyTag, 1);
	}
	PawnAttackParam.Replenishment();
}

void ABulletHoldWeaponActor::Notify_ReloadOwner()
{
	if (Character.IsValid())
	{
		Character->GetWvAbilitySystemComponent()->TryActivateAbilityByTag(TAG_Weapon_Gun_Reload);
	}
}

void ABulletHoldWeaponActor::DoReload()
{

	if (IsValid(ReloadAnimation))
	{
		SkeletalMeshComponent->PlayAnimation(ReloadAnimation, false);
	}
	DoReloadReceived();
}


#pragma region Fire
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

const FVector ABulletHoldWeaponActor::CalcTraceEndPosition()
{
	const FTransform MuzzleTransform = SkeletalMeshComponent->GetSocketTransform(PawnAttackParam.MuzzleSocketName);
	const FVector TraceStart = MuzzleTransform.GetLocation();
	FVector Forward = FRotator(MuzzleTransform.GetRotation()).Vector();

	FVector TraceEndPosition = FVector::ZeroVector;
	auto LocomotionEssencialVariables = Character->GetLocomotionComponent()->GetLocomotionEssencialVariables();
	if (LocomotionEssencialVariables.LSRotationMode != ELSRotationMode::VelocityDirection)
	{
		if (const APlayerCharacter* PC = Cast<APlayerCharacter>(Character))
		{
			const auto StartPosition = PC->GetFollowCamera()->GetComponentLocation();
			auto Pos = StartPosition;
			Forward = PC->GetFollowCamera()->GetForwardVector() * PawnAttackParam.TraceRange;
			Pos += Forward;

			TraceEndPosition = Pos;
			FHitResult LocalHitResult(ForceInit);
			const bool bIsHit = LineOfSight(StartPosition, TraceEndPosition, LocalHitResult);

			if (LocalHitResult.IsValidBlockingHit())
			{
				TraceEndPosition = LocalHitResult.ImpactPoint;
			}

			// hand noise
			TraceEndPosition += TraceNoise;
		}
		else
		{
			Forward = LocomotionEssencialVariables.LookingRotation.Vector();
			FVector Offset = Forward * PawnAttackParam.TraceRange;
			Offset += TraceNoise;
			TraceEndPosition = TraceStart + Offset;
		}
	}
	else
	{
		FVector Offset = Forward * PawnAttackParam.TraceRange;
		Offset += TraceNoise;
		TraceEndPosition = TraceStart + Offset;
	}
	return TraceEndPosition;
}


const bool ABulletHoldWeaponActor::LineOfSightOuter(FHitResult& OutHitResult)
{
	if (!Character->GetLocomotionComponent())
	{
		return false;
	}

	const bool bIsRandomize = Character->IsBotCharacter() || Character->IsHealthHalf();
	const FTransform MuzzleTransform = SkeletalMeshComponent->GetSocketTransform(PawnAttackParam.MuzzleSocketName);
	const FVector TraceStart = MuzzleTransform.GetLocation();
	//FVector Forward = FRotator(MuzzleTransform.GetRotation()).Vector();

	// make noise
	if (bIsRandomize)
	{
		const float Value = FMath::RandRange(Randmize * -1.0f, Randmize);
		TraceNoise = FVector(0.f, Value, Value);
	}
	else
	{
		TraceNoise = FVector::ZeroVector;
	}

	const FVector TraceEndPosition = CalcTraceEndPosition();
	return LineOfSight(TraceStart, TraceEndPosition, OutHitResult);
}

const bool ABulletHoldWeaponActor::LineOfSight(const FVector TraceStart, const FVector TraceEnd, FHitResult& OutHitResult)
{
	TArray<AActor*> IgnoreActors({ Character.Get(), });
	auto TraceType = ASC_GLOBAL()->bWeaponTraceDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(ASC_GLOBAL()->WeaponTraceChannel);

	FHitResult HitResult(ForceInit);
	const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), TraceStart, TraceEnd,
		ASC_GLOBAL()->WeaponTraceChannel, false, IgnoreActors, TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 2.0f);

	OutHitResult = HitResult;
	return bHitResult;
}

const bool ABulletHoldWeaponActor::LineOfSightOuterMulti(TArray<FHitResult>& OutHitResults)
{
	if (!Character->GetLocomotionComponent())
	{
		return false;
	}

	const bool bIsRandomize = Character->IsBotCharacter() || Character->IsHealthHalf();
	const FTransform MuzzleTransform = SkeletalMeshComponent->GetSocketTransform(PawnAttackParam.MuzzleSocketName);
	const FVector TraceStart = MuzzleTransform.GetLocation();
	//FVector Forward = FRotator(MuzzleTransform.GetRotation()).Vector();

	// make noise
	if (bIsRandomize)
	{
		const float Value = FMath::RandRange(Randmize * -1.0f, Randmize);
		TraceNoise = FVector(0.f, Value, Value);
	}
	else
	{
		TraceNoise = FVector::ZeroVector;
	}

	const FVector TraceEndPosition = CalcTraceEndPosition();
	return LineOfSightMulti(TraceStart, TraceEndPosition, OutHitResults);
}

const bool ABulletHoldWeaponActor::LineOfSightMulti(const FVector TraceStart, const FVector TraceEnd, TArray<FHitResult>& OutHitResults)
{
	TArray<AActor*> IgnoreActors({ Character.Get(), });
	auto TraceType = ASC_GLOBAL()->bWeaponTraceDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(ASC_GLOBAL()->WeaponTraceChannel);

	TArray<FHitResult> HitResults;
	const bool bHitResult = UKismetSystemLibrary::LineTraceMulti(GetWorld(), TraceStart, TraceEnd, 
		ASC_GLOBAL()->WeaponTraceChannel, false, IgnoreActors, TraceType, HitResults, true, FLinearColor::Red, FLinearColor::Green, 2.0f);

	OutHitResults = HitResults;
	return bHitResult;
}
#pragma endregion

