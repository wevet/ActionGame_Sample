// Copyright 2022 wevet works All Rights Reserved.


#include "Animation/WvFoleyEventAnimNotify.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Misc/WvCommonUtils.h"
#include "WvAbilitySystemGlobals.h"
#include "Component/WvCharacterMovementComponent.h"
#include "GameExtension.h"
#include "Level/FieldInstanceSubsystem.h"

#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Perception/AISense_Hearing.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvFoleyEventAnimNotify)


void UWvFoleyEventAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;

	if (!OwnerActor)
	{
		return;
	}

	if (bIsDisableTrace)
	{
		SpawnNoTraceSoundAndEffect(OwnerActor, MeshComp);
		return;
	}

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
		TraceFootDelegate.BindUObject(this, &UWvFoleyEventAnimNotify::TraceFootDone, MeshComp, Animation);
		World->AsyncLineTraceByChannel(
			EAsyncTraceType::Single,
			TraceBegin, TraceEnd,
			ECC_Visibility, TraceParams,
			FCollisionResponseParams::DefaultResponseParam, &TraceFootDelegate);
	}
	else
	{

#if WITH_EDITOR
		FHitResult HitResult;
		if (World->LineTraceSingleByChannel(HitResult, TraceBegin, TraceEnd, ECC_Visibility, TraceParams))
		{
			FVector HitLocation = HitResult.Location;
			EPhysicalSurface HitSurfaceType = SurfaceTypeInEditor;
			//TriggerEffect(OwnerActor, Animation, HitResult);
		}
		else
		{
			DrawDebugLine(World, TraceBegin, TraceEnd, FColor::Red, false, 1.f);
		}
#endif

	}

}



void UWvFoleyEventAnimNotify::TraceFootDone(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum, USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (TraceDatum.OutHits.Num() == 0)
	{
		return;
	}

	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;

	if (IsValid(OwnerActor))
	{
		if (!UWvCommonUtils::IsInViewport(OwnerActor))
		{
			return;
		}

		const FHitResult& HitResult = TraceDatum.OutHits[0];

		TriggerEffect(OwnerActor, Animation, HitResult);
	}
}

void UWvFoleyEventAnimNotify::TriggerEffect(AActor* Owner, UAnimSequenceBase* Animation, const FHitResult& HitResult)
{
	const FVector HitLocation = HitResult.Location;
	const EPhysicalSurface HitSurfaceType = HitResult.PhysMaterial.IsValid() ? HitResult.PhysMaterial->SurfaceType.GetValue() : EPhysicalSurface::SurfaceType_Default;

	bool bIsFound = false;
	const FFoleyBaseAsset& Data = UFieldInstanceSubsystem::Get()->GetFoleyBaseAsset(FooleyTag, HitSurfaceType, bIsFound);

	if(!bIsFound)
	{
		return;
	}

	CalculateVolume(Owner);


	if (Data.Effect)
	{
		UWorld* World = Owner->GetWorld();
		const float Angle = UKismetMathLibrary::DegAcos(HitResult.ImpactNormal | FVector(0.0f, 0.0f, 1.0f));
		const FVector FaceDirection = (Owner->GetActorLocation() - HitLocation).GetSafeNormal();
		const FRotator FaceRotation = UKismetMathLibrary::MakeRotFromXZ(HitResult.ImpactNormal, Owner->GetActorForwardVector());
		const FVector Direction = FaceRotation.RotateVector(FVector::UpVector);
		//DrawDebugPoint(World, HitLocation, 20, FColor::Green, false, 1.0f);
		//DrawDebugLine(World, HitLocation, HitLocation + (Direction * 100), FColor::Red, false, 1.0f);
		//DrawDebugLine(World, HitLocation, HitLocation + (HitResult.ImpactNormal * 50), FColor::Green, false, 1.0f);

		//UE_LOG(LogTemp, Log, TEXT("Angle => %.3f"), Angle);
		//UE_LOG(LogTemp, Log, TEXT("FaceRotation => %s"), *FaceRotation.ToString());

		const FRotator EffectRot{ Angle, 0.f, 0.f };
		UWvCommonUtils::SpawnParticleAtLocation(World, Data.Effect, HitLocation, EffectRot, FVector::OneVector);

	}


	if (Data.Sound)
	{
		SpawnSound(Data.Sound, Owner);
	}

}

bool UWvFoleyEventAnimNotify::HasOwnerAttachedTag() const
{
	FGameplayTagContainer FoleyContainer;
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	FoleyContainer.AddTag(TagsManager.RequestGameplayTag(FName("Foley.Event.Run")));
	FoleyContainer.AddTag(TagsManager.RequestGameplayTag(FName("Foley.Event.RunBackwds")));
	FoleyContainer.AddTag(TagsManager.RequestGameplayTag(FName("Foley.Event.RunStrafe")));
	FoleyContainer.AddTag(TagsManager.RequestGameplayTag(FName("Foley.Event.Scuff")));
	FoleyContainer.AddTag(TagsManager.RequestGameplayTag(FName("Foley.Event.ScuffPivot")));
	FoleyContainer.AddTag(TagsManager.RequestGameplayTag(FName("Foley.Event.Walk")));
	FoleyContainer.AddTag(TagsManager.RequestGameplayTag(FName("Foley.Event.WalkBackwds")));

	return FoleyContainer.HasTag(FooleyTag);
}


void UWvFoleyEventAnimNotify::CalculateVolume(AActor* Owner)
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
}

const bool UWvFoleyEventAnimNotify::IsInCrouch(const AActor* Owner)
{
	if (IsValid(Owner))
	{
		auto LocomotionComponent = Owner->FindComponentByClass<ULocomotionComponent>();
		if (LocomotionComponent)
		{
			const auto& LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
			return LocomotionEssencialVariables.LSStance == ELSStance::Crouching;
		}
	}
	return false;
}


void UWvFoleyEventAnimNotify::SpawnNoTraceSoundAndEffect(AActor* Owner, USkeletalMeshComponent* MeshComp)
{
	bool bIsFound = false;
	const FFoleyBaseAsset& Data = UFieldInstanceSubsystem::Get()->GetFoleyBaseAsset(FooleyTag, EPhysicalSurface::SurfaceType_Default, bIsFound);

	if (!bIsFound)
	{
		return;
	}

	CalculateVolume(Owner);

	if (Data.Effect)
	{
		UWorld* World = Owner->GetWorld();
		const auto SocketTransform = MeshComp->GetSocketTransform(SocketName);

		//const FVector HitLocation = Owner->GetActorLocation();
		//const float Angle = UKismetMathLibrary::DegAcos(HitResult.ImpactNormal | FVector(0.0f, 0.0f, 1.0f));
		//const FVector FaceDirection = (Owner->GetActorLocation() - HitLocation).GetSafeNormal();
		//const FRotator FaceRotation = UKismetMathLibrary::MakeRotFromXZ(HitResult.ImpactNormal, Owner->GetActorForwardVector());
		//const FVector Direction = FaceRotation.RotateVector(FVector::UpVector);
		//const FRotator EffectRot{ Angle, 0.f, 0.f };
		//UWvCommonUtils::SpawnParticleAtLocation(World, Data.Effect, HitLocation, EffectRot, FVector::OneVector);
	}


	if (Data.Sound)
	{
		SpawnSound(Data.Sound, Owner);
	}
}


void UWvFoleyEventAnimNotify::SpawnSound(USoundBase* InSound, AActor* Owner)
{
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(Owner))
	{
		UWorld* World = Character->GetWorld();

		const FVector2D NoiseRange = ASC_GLOBAL()->BotConfig.HearingRange;

		const float Speed = Character->GetCharacterMovement()->Velocity.Size();
		const float MaxSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
		float Radius = UKismetMathLibrary::MapRangeClamped(Speed, 0.f, MaxSpeed, NoiseRange.X, NoiseRange.Y);

		if (IsInCrouch(Character))
		{
			constexpr float CrouchLoudness = 0.5f;
			Radius *= CrouchLoudness;
		}

		float RealVolume = Volume;
		const float PlayerDistThreshold = ASC_GLOBAL()->BotConfig.PlayerDistThreshold;
		if (Character->IsBotCharacter())
		{
			auto PC = Game::ControllerExtension::GetPlayer(Character->GetWorld(), 0);
			if (PC)
			{
				if (Character->GetDistanceTo(PC->GetPawn()) > PlayerDistThreshold)
				{
					RealVolume = 0.f;
				}
			}
		}

		Character->ReportNoiseEvent(FVector::ZeroVector, Volume, Radius);
		UGameplayStatics::PlaySoundAtLocation(World, InSound, Owner->GetActorLocation(), RealVolume, 1.0f, 0.0f, nullptr, nullptr);
	}
}


