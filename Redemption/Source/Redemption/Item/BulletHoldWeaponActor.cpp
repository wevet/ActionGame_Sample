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

using namespace CharacterDebug;

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
	if (!Character.IsValid())
	{
		return true;
	}

	if (IsCurrentAmmosEmpty())
	{
		if (Character->GetWvAbilitySystemComponent()->HasMatchingGameplayTag(TAG_Character_Action_GunReload))
		{
			// reloading..
			//UE_LOG(LogTemp, Warning, TEXT("reloading.. => %s"), *FString(__FUNCTION__));
			return false;
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("reload ability start => %s"), *FString(__FUNCTION__));
			Notify_ReloadOwner();
			return false;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("not empty => %s"), *FString(__FUNCTION__));
	return true;
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


float ABulletHoldWeaponActor::GetBulletInterval() const
{
	return BulletInterval; 
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
	const FVector MuzzleLocation = MuzzleTransform.GetLocation();
	FVector TraceEndPosition = FVector::ZeroVector;

	const FLocomotionEssencialVariables& LocomotionEssencialVariables = Character->GetLocomotionComponent()->GetLocomotionEssencialVariables();
	if (LocomotionEssencialVariables.LSRotationMode != ELSRotationMode::VelocityDirection)
	{
		if (const APlayerCharacter* PC = Cast<APlayerCharacter>(Character))
		{
			const FVector CameraLocation = PC->GetFollowCamera()->GetComponentLocation();
			const FVector CameraForward = PC->GetFollowCamera()->GetForwardVector();
			const FVector CameraTraceEnd = CameraLocation + (CameraForward * PawnAttackParam.TraceRange);

			FHitResult CameraHitResult(ForceInit);
			
			WEVET_COMMENT("Fixed Bullet Forward")
			// カメラで見た方向に「ターゲット地点」を正しく決める（画面中央＝ターゲット）
			const bool bHit = LineOfSight(CameraLocation, CameraTraceEnd, CameraHitResult);

			const FVector TargetPoint = bHit ? CameraHitResult.ImpactPoint : CameraTraceEnd;
			const FVector FireDirection = (TargetPoint - MuzzleLocation).GetSafeNormal();
			TraceEndPosition = MuzzleLocation + (FireDirection * PawnAttackParam.TraceRange);
			TraceEndPosition += TraceNoise;
		}
		else
		{
			const FVector Forward = LocomotionEssencialVariables.LookingRotation.Vector();
			FVector Offset = Forward * PawnAttackParam.TraceRange;
			Offset += TraceNoise;
			TraceEndPosition = MuzzleLocation + Offset;
		}
	}
	else
	{
		const FVector Forward = FRotator(MuzzleTransform.GetRotation()).Vector();
		FVector Offset = Forward * PawnAttackParam.TraceRange;
		Offset += TraceNoise;
		TraceEndPosition = MuzzleLocation + Offset;
	}
	return TraceEndPosition;
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
	TArray<AActor*> IgnoreActors({ this, Character.Get() });

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const EDrawDebugTrace::Type TraceType = (CVarDebugCombatSystem.GetValueOnGameThread() > 0) ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
#else
	const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;
#endif

	//const ETraceTypeQuery Query = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);
	
	TArray<FHitResult> HitResults;
	const bool bHitResult = UKismetSystemLibrary::LineTraceMulti(GetWorld(), TraceStart, TraceEndPosition,
		ASC_GLOBAL()->WeaponTraceChannel, false, IgnoreActors, TraceType, HitResults, true, FLinearColor::Red, FLinearColor::Green, 2.0f);

	OutHitResults = HitResults;
	return bHitResult;
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
	TArray<AActor*> IgnoreActors({ this, Character.Get() });

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const EDrawDebugTrace::Type TraceType = (CVarDebugCombatSystem.GetValueOnGameThread() > 0) ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
#else
	const EDrawDebugTrace::Type TraceType = EDrawDebugTrace::None;
#endif

	FHitResult HitResult(ForceInit);
	const bool bHitResult = UKismetSystemLibrary::LineTraceSingle(GetWorld(), TraceStart, TraceEnd,
		ASC_GLOBAL()->WeaponTraceChannel, false, IgnoreActors, TraceType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 2.0f);

	OutHitResult = HitResult;
	return bHitResult;
}
#pragma endregion

