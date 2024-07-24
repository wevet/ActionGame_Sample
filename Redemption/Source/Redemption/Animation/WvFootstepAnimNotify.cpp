// Copyright 2022 wevet works All Rights Reserved.


#include "WvFootstepAnimNotify.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Misc/WvCommonUtils.h"
#include "WvAbilitySystemGlobals.h"
#include "Component/WvCharacterMovementComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Perception/AISense_Hearing.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvFootstepAnimNotify)

void UWvFootstepAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (OwnerActor)
	{
		if (ACharacter* Character = Cast<ACharacter>(OwnerActor))
		{
			if (!Character->GetCharacterMovement()->IsMovingOnGround())
			{
				return;
			}
		}
		else if (APawn* Pawn = Cast<APawn>(OwnerActor))
		{
			if (Pawn->GetMovementComponent()->IsFalling())
			{
				return;
			}
		}
	}

	TraceFoot(MeshComp, Animation);
}

void UWvFootstepAnimNotify::TraceFoot(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		// Distance to the foot downward is asset-dependent, so it is made into a property.
		const FVector SocketLocation = MeshComp->GetSocketLocation(SocketName);
		// Float the starting point a little to account for the sinking into the ground.
		const FVector TraceBegin = SocketLocation + FVector::UpVector * TraceBeginDistance;
		const FVector TraceEnd = SocketLocation + FVector::DownVector * TraceEndDistance;

		FCollisionQueryParams TraceParams(NAME_None, false, OwnerActor);
		TraceParams.bReturnPhysicalMaterial = true;

		UWorld* World = OwnerActor->GetWorld();
		if (World->IsGameWorld())
		{
			FTraceDelegate TraceFootDelegate;
			TraceFootDelegate.BindUObject(this, &ThisClass::TraceFootDone, MeshComp, Animation);
			World->AsyncLineTraceByChannel(
				EAsyncTraceType::Single, 
				TraceBegin, 
				TraceEnd,
				ECC_Visibility, 
				TraceParams,
				FCollisionResponseParams::DefaultResponseParam,
				&TraceFootDelegate);

			//DrawDebugLine(World, TraceBegin, TraceEnd, FColor::Red, false, 1.f);
		}
#if WITH_EDITOR
		else
		{
			FHitResult HitResult;
			if (World->LineTraceSingleByChannel(HitResult, TraceBegin, TraceEnd, ECC_Visibility, TraceParams))
			{
				FVector HitLocation = HitResult.Location;
				EPhysicalSurface HitSurfaceType = SurfaceTypeInEditor;
				TriggerEffect(OwnerActor, Animation, HitResult);
			}
			else
			{
				DrawDebugLine(World, TraceBegin, TraceEnd, FColor::Red, false, 1.f);
			}
		}
#endif
	}
}

void UWvFootstepAnimNotify::TraceFootDone(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum, USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (TraceDatum.OutHits.Num() == 0)
	{
		return;
	}

	if (AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		if (!UWvCommonUtils::IsInViewport(OwnerActor))
		{
			return;
		}

		const FHitResult& HitResult = TraceDatum.OutHits[0];
		TriggerEffect(OwnerActor, Animation, HitResult);
	}
}

void UWvFootstepAnimNotify::TriggerEffect(AActor* Owner, UAnimSequenceBase* Animation, const FHitResult& HitResult)
{
	const FVector HitLocation = HitResult.Location;
	const EPhysicalSurface HitSurfaceType = HitResult.PhysMaterial.IsValid() ? HitResult.PhysMaterial->SurfaceType.GetValue() : EPhysicalSurface::SurfaceType_Default;

	FString SurfaceName = UWvCommonUtils::GetSurfaceName(HitSurfaceType).ToString();

	const ELSGait GaitMode = GetGaitMode(Owner);
	FString GaitModeName = TEXT(".Running");
	if (GaitMode == ELSGait::Walking)
	{
		GaitModeName = TEXT(".Walking");
	}

	SurfaceName.Append(GaitModeName);

	if (FootStepDT)
	{
		auto RowData = FootStepDT->FindRow<FFootStepTableRow>(FName(SurfaceName), "");
		if (RowData)
		{
			Volume = 1.0f;
			if (UWvCommonUtils::IsBotPawn(Owner))
			{
				const float BotVolume = ASC_GLOBAL()->BotConfig.FootStepMaxVolume;
				constexpr float Threshold = 5000.0f;
				const float Distance = UWvCommonUtils::PlayerPawnToDistance(Owner);
				Volume = UKismetMathLibrary::MapRangeClamped(Distance, 0.f, Threshold, 1.0f, 0.f);
				Volume = FMath::Clamp(Volume, 0.f, BotVolume);
			}

			if (IsInCrouch(Owner))
			{
				constexpr float CrouchThreshold = 10.0f;
				Volume = Volume / CrouchThreshold;
			}

			UWorld* World = Owner->GetWorld();

			if (RowData->NiagaraSystems)
			{
				const float Angle = UKismetMathLibrary::DegAcos(HitResult.ImpactNormal | FVector(0.0f, 0.0f, 1.0f));
				const FVector FaceDirection = (Owner->GetActorLocation() - HitLocation).GetSafeNormal();
				const FRotator FaceRotation = UKismetMathLibrary::MakeRotFromXZ(HitResult.ImpactNormal, Owner->GetActorForwardVector());

				const FVector Direction = FaceRotation.RotateVector(FVector::UpVector);
				DrawDebugPoint(World, HitLocation, 20, FColor::Green, false, 1.0f);
				DrawDebugLine(World, HitLocation, HitLocation + (Direction * 100), FColor::Red, false, 1.0f);
				DrawDebugLine(World, HitLocation, HitLocation + (HitResult.ImpactNormal * 50), FColor::Green, false, 1.0f);

				UE_LOG(LogTemp, Log, TEXT("Angle => %.3f"), Angle);
				UE_LOG(LogTemp, Log, TEXT("FaceRotation => %s"), *FaceRotation.ToString());

				const FRotator EffectRot{Angle, 0.f, 0.f};
				UWvCommonUtils::SpawnParticleAtLocation(World, RowData->NiagaraSystems, HitLocation, EffectRot, FVector::OneVector);

			}

			if (RowData->FootStepSound)
			{
				UGameplayStatics::PlaySoundAtLocation(World, RowData->FootStepSound, HitLocation, Volume, 1.0f, 0.0f, nullptr, nullptr);
				const FVector2D NoiseRange = ASC_GLOBAL()->BotConfig.HearingRange;

				if (ABaseCharacter* Character = Cast<ABaseCharacter>(Owner))
				{
					const float Speed = Character->GetCharacterMovement()->Velocity.Size();
					const float MaxSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
					float Radius = UKismetMathLibrary::MapRangeClamped(Speed, 0.f, MaxSpeed, NoiseRange.X, NoiseRange.Y);

					if (IsInCrouch(Character))
					{
						constexpr float CrouchLoudness = 0.2f;
						Radius *= CrouchLoudness;
					}
					Character->ReportNoiseEvent(FVector::ZeroVector, Volume, Radius);
				}
			}


		}
	}

	//UE_LOG(LogTemp, Log, TEXT("SurfaceName => %s"), *SurfaceName);
}


const ELSGait UWvFootstepAnimNotify::GetGaitMode(AActor* Owner)
{
	if (IsValid(Owner))
	{
		auto LocomotionComponent = Owner->FindComponentByClass<ULocomotionComponent>();
		if (LocomotionComponent)
		{
			return LocomotionComponent->GetLSGaitMode_Implementation();
		}
	}
	return ELSGait::Running;
}

const bool UWvFootstepAnimNotify::IsInCrouch(AActor* Owner)
{
	if (IsValid(Owner))
	{
		auto LocomotionComponent = Owner->FindComponentByClass<ULocomotionComponent>();
		if (LocomotionComponent)
		{
			auto LocomotionEssencialVariabled = LocomotionComponent->GetLocomotionEssencialVariables();
			return LocomotionEssencialVariabled.LSStance == ELSStance::Crouching;
		}
	}
	return false;
}


