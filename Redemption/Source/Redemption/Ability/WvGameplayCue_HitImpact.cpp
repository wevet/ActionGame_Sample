// Copyright 2022 wevet works All Rights Reserved.


#include "WvGameplayCue_HitImpact.h"
#include "Character/BaseCharacter.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "Misc/WvCommonUtils.h"
#include "WvGameplayEffectContext.h"

#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvGameplayCue_HitImpact)

bool UWvGameplayCue_HitImpact::HandlesEvent(EGameplayCueEvent::Type EventType) const
{
	return EventType == EGameplayCueEvent::Executed;
}

void UWvGameplayCue_HitImpact::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	check(MyTarget);

	if (!CueConfigDataAssest)
	{
		return;
	}

	const FHitResult* HitResult = Parameters.EffectContext.GetHitResult();
	if (!HitResult)
	{
		return;
	}

	ABaseCharacter* AttackerActor = Cast<ABaseCharacter>(Parameters.EffectContext.GetEffectCauser());
	ABaseCharacter* BeHitActor = Cast<ABaseCharacter>(MyTarget);
	if (!BeHitActor || !AttackerActor)
	{
		return;
	}

	FOnceApplyEffect OnceApplyEffect;
	if (!UWvAbilitySystemBlueprintFunctionLibrary::EffectContextGetEffectGroup(Parameters.EffectContext, OnceApplyEffect))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("%s"), *FString(__FUNCTION__));
	HandleAttack(BeHitActor, AttackerActor, Parameters, OnceApplyEffect, HitResult);
	HandleBeHit(BeHitActor, AttackerActor, Parameters, OnceApplyEffect, HitResult);
}

void UWvGameplayCue_HitImpact::HandleAttack(ABaseCharacter* BeHitActor, ABaseCharacter* Attacker, const FGameplayCueParameters& Parameters, FOnceApplyEffect& OnceApplyEffect, const FHitResult* HitResult)
{
	UApplyEffectExData* ExData = UWvAbilitySystemBlueprintFunctionLibrary::GetOnecEffectExData(OnceApplyEffect);
	if (!ExData)
	{
		return;
	}

	FWvAttackCueConfigRow ConfigRow;

	const FGameplayTag AvatarTag = Attacker->GetAvatarTag();
	const FGameplayTag AttackCueTag = ExData->CueConfig.AttackCueTag;

	if (!CueConfigDataAssest->GetAttackCueConfigRow(AvatarTag, AttackCueTag, ConfigRow))
	{
		return;
	}

	FWvAttackCueConfig& Config = ConfigRow.CueConfig;
	FParticleCueConfig ParticleConfig = Config.Particle;

	FVector ParticlePosition;
	GetHitImpactParticleLocation(BeHitActor, Attacker, HitResult, Config.FixedAttachBoneName, Config.Particle.PositionOffset, ParticlePosition);

	FRotator ParticleRotation;
	GetHitImpactParticleRotation(BeHitActor, Attacker, Parameters, HitResult, Config.RotationMode, Config.OnlyYaw, Config.Particle.RotationOffset, ParticleRotation);
	const FVector ParticleScale = (ParticleConfig.ParticleScale == FVector::ZeroVector) ? FVector::OneVector : ParticleConfig.ParticleScale;

	if (ParticleConfig.NiagaraSystem)
	{
		if (ParticleConfig.IsAttachInBone)
		{
			UWvCommonUtils::SpawnParticleAttached(ParticleConfig.NiagaraSystem, BeHitActor->GetMesh(), HitResult->BoneName, ParticlePosition, ParticleRotation, ParticleScale, EAttachLocation::KeepWorldPosition);;
		}
		else
		{
			UWvCommonUtils::SpawnParticleAtLocation(Attacker, ParticleConfig.NiagaraSystem, ParticlePosition, ParticleRotation, ParticleScale);
		}
	}

	if (IsDebug)
	{
		DrawDebugPoint(GetWorld(), ParticlePosition, 20, FColor::Red, false, DebugTime);
		const FVector Direction = ParticleRotation.RotateVector(FVector::ForwardVector);
		DrawDebugLine(GetWorld(), ParticlePosition, ParticlePosition + Direction * 1000, FColor::Red, false, DebugTime);
	}
}

void UWvGameplayCue_HitImpact::HandleBeHit(ABaseCharacter* BeHitActor, ABaseCharacter* Attacker, const FGameplayCueParameters& Parameters, FOnceApplyEffect& OnceApplyEffect, const FHitResult* HitResult)
{
	UApplyEffectExData* ExData = UWvAbilitySystemBlueprintFunctionLibrary::GetOnecEffectExData(OnceApplyEffect);
	if (!ExData)
	{
		return;
	}

	const FGameplayTag AvatarTag = BeHitActor->GetAvatarTag();
	FGameplayTag BeHitCueTag = ExData->CueConfig.BeHitCueTag;

	if (HitResult->PhysMaterial.IsValid())
	{
		FGameplayTagContainer ResultTags{};

		//weapon tag
		ResultTags.AddTag(BeHitCueTag);

		for (auto& Tag : ResultTags)
		{
			FWvDamageCueConfigRow ConfigRow;

			if (!CueConfigDataAssest->GetDamageCueConfigRow(AvatarTag, Tag, ConfigRow))
			{
				continue;
			}

			FWvDamageCueConfig& Config = ConfigRow.CueConfig;
			FParticleCueConfig ParticleConfig = ConfigRow.CueConfig.Particle;

			if (!ParticleConfig.NiagaraSystem)
			{
				continue;
			}

			FVector ParticlePosition;
			GetHitImpactParticleLocation(BeHitActor, Attacker, HitResult, NAME_None, Config.Particle.PositionOffset, ParticlePosition);

			FRotator ParticleRotation;
			GetHitImpactParticleRotation(BeHitActor, Attacker, Parameters, HitResult, Config.RotationMode, Config.OnlyYaw, Config.Particle.RotationOffset, ParticleRotation);

			FVector ParticleScale = ParticleConfig.ParticleScale == FVector::ZeroVector ? FVector::OneVector : ParticleConfig.ParticleScale;

			if (ParticleConfig.NiagaraSystem)
			{
				if (ParticleConfig.IsAttachInBone)
				{
					UWvCommonUtils::SpawnParticleAttached(ParticleConfig.NiagaraSystem, BeHitActor->GetMesh(), HitResult->BoneName, ParticlePosition, ParticleRotation, ParticleScale, EAttachLocation::KeepWorldPosition);
				}
				else
				{
					UWvCommonUtils::SpawnParticleAtLocation(BeHitActor, ParticleConfig.NiagaraSystem, ParticlePosition, ParticleRotation, ParticleScale);
				}
			}

			if (IsDebug)
			{
				DrawDebugPoint(GetWorld(), ParticlePosition, 20, FColor::Green, false, DebugTime);
				const FVector Direction = ParticleRotation.RotateVector(FVector::ForwardVector);
				DrawDebugLine(GetWorld(), ParticlePosition, ParticlePosition + Direction * 1000, FColor::Green, false, DebugTime);
			}
		}
	}

	//BeHitActor->OnHitBone(Attacker->GetMesh(), *HitResult, true);
}

void UWvGameplayCue_HitImpact::GetHitImpactParticleLocation(ABaseCharacter* BeHitActor, ABaseCharacter* Attacker, const FHitResult* HitResult, FName FixedAttachBoneName, FVector LocationOffset, FVector& OutLocation)
{
	FVector* LocationPtr = nullptr;

	if (!FixedAttachBoneName.IsNone())
	{
		FTransform boneTransfrom;
		if (UWvCommonUtils::GetBoneTransForm(Attacker->GetMesh(), FixedAttachBoneName, boneTransfrom))
		{
			FVector Location = FVector(boneTransfrom.GetLocation());
			LocationPtr = &Location;
		}
	}

	if (!LocationPtr && !HitResult->BoneName.IsNone())
	{
		FTransform BoneTransfrom;
		if (UWvCommonUtils::GetBoneTransForm(BeHitActor->GetMesh(), HitResult->BoneName, BoneTransfrom))
		{
			FVector Location = FVector(BoneTransfrom.GetLocation());
			LocationPtr = &Location;
		}
	}

	if (!LocationPtr)
	{
		FVector Location = FVector(HitResult->Location);
		LocationPtr = &Location;
	}

	if (LocationPtr)
	{
		OutLocation = *LocationPtr;
	}
	else
	{
		OutLocation = FVector::ZeroVector;
	}
	OutLocation += LocationOffset;
}

void UWvGameplayCue_HitImpact::GetHitImpactParticleRotation(ABaseCharacter* BeHitActor, ABaseCharacter* Attacker, const FGameplayCueParameters& Parameters, const FHitResult* HitResult, EParticleRotationMode RotationMode, bool isOnlyYaw, FRotator RotatorOffset, FRotator& OutRotation)
{
	FRotator RotationOffset = RotatorOffset;

	if (RotationMode == EParticleRotationMode::None)
	{
		OutRotation = FRotator::ZeroRotator;
	}
	else if (RotationMode == EParticleRotationMode::AddToNormal)
	{
		OutRotation = UKismetMathLibrary::MakeRotFromX(HitResult->ImpactNormal);
	}
	else if (RotationMode == EParticleRotationMode::HitDirection)
	{
		FWvGameplayAbilityTargetData* TargetData = nullptr;
		FWvGameplayEffectContext* EffectContext = (FWvGameplayEffectContext*)(Parameters.EffectContext.Get());
		if (EffectContext)
		{
			FGameplayAbilityTargetDataHandle TargetDataHandle = EffectContext->GetTargetDataHandle();
			TargetData = static_cast<FWvGameplayAbilityTargetData*>(TargetDataHandle.Get(0));
		}

		bool bIsSuccess = false;
		if (TargetData)
		{
			FTransform BoneTransfrom;
			if (UWvCommonUtils::GetBoneTransForm(BeHitActor->GetMesh(), HitResult->BoneName, BoneTransfrom))
			{
				FVector Direction = FVector(TargetData->SourceLocation - BoneTransfrom.GetLocation()).GetSafeNormal();
				OutRotation = UKismetMathLibrary::MakeRotFromX(Direction);
				bIsSuccess = true;
			}
		}

		if (!bIsSuccess)
		{
			OutRotation = UKismetMathLibrary::MakeRotFromX(HitResult->ImpactNormal);
		}
	}
	else if (RotationMode == EParticleRotationMode::FaceToAttacker)
	{
		FVector FaceDirection = (Attacker->GetActorLocation() - BeHitActor->GetActorLocation()).GetSafeNormal();
		OutRotation = UKismetMathLibrary::MakeRotFromX(FaceDirection);
	}
	else if (RotationMode == EParticleRotationMode::AttackerForward)
	{
		FVector ForwardDirection = Attacker->GetActorForwardVector();
		OutRotation = UKismetMathLibrary::MakeRotFromX(ForwardDirection);
	}
	else if (RotationMode == EParticleRotationMode::BeHitedBackward)
	{
		FVector BackwardDirection = -BeHitActor->GetActorForwardVector();
		OutRotation = UKismetMathLibrary::MakeRotFromX(BackwardDirection);
	}

	if (isOnlyYaw)
	{
		OutRotation.Roll = 0;
		OutRotation.Pitch = 0;
	}
	OutRotation += RotationOffset;
}


