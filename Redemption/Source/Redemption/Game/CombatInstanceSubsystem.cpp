// Copyright 2022 wevet works All Rights Reserved.


#include "Game/CombatInstanceSubsystem.h"
#include "WvAbilitySystemGlobals.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "WvGameplayEffectContext.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvGameplayCue_HitEnvironment.h"


#include "Kismet/KismetMathLibrary.h"
#include "Perception/AISense_Hearing.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CombatInstanceSubsystem)

UCombatInstanceSubsystem* UCombatInstanceSubsystem::Instance = nullptr;

UCombatInstanceSubsystem::UCombatInstanceSubsystem()
{
	UCombatInstanceSubsystem::Instance = this;
}

UCombatInstanceSubsystem* UCombatInstanceSubsystem::Get()
{
	return Instance;
}

void UCombatInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	BeginHitImpactAssetsLoad();
}

void UCombatInstanceSubsystem::Deinitialize()
{
	HitEffectDTRawPtr.Reset();
}

void UCombatInstanceSubsystem::BeginHitImpactAssetsLoad()
{
	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();
	LoadSoftObjectPathes = FSoftObjectPath("/Game/Game/Blueprints/DataTables/DT_EnvironmentHit.DT_EnvironmentHit");
	StreamableHandle = StreamableManager.RequestAsyncLoad(LoadSoftObjectPathes, FStreamableDelegate::CreateUObject(this, &ThisClass::OnHitImpactAssetsLoadCompleted));
}

void UCombatInstanceSubsystem::OnHitImpactAssetsLoadCompleted()
{
	HitEffectDTRawPtr = TSoftObjectPtr<UDataTable>(LoadSoftObjectPathes).Get();
	HitEffectDTInstance = HitEffectDTRawPtr.Get();

	StreamableHandle.Reset();
	UE_LOG(LogTemp, Log, TEXT("Success Async load HitEffectDTInstance: [%s]"), *FString(__FUNCTION__));

}

void UCombatInstanceSubsystem::OnHitEnvironment(const AActor* Attacker, const FHitResult& HitResult)
{
	if (!HitEffectDTInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("not valid HitEffectDTInstance => %s"), *FString(__FUNCTION__));
		return;
	}

	if (!Attacker->WasRecentlyRendered())
	{
		//Has this actor been rendered "recently"?
		UE_LOG(LogTemp, Warning, TEXT("This actor does not appear to have been rendered recently. => %s"), *FString(__FUNCTION__));
		return;
	}

	const EPhysicalSurface HitSurfaceType = HitResult.PhysMaterial.IsValid() ? HitResult.PhysMaterial->SurfaceType.GetValue() : EPhysicalSurface::SurfaceType_Default;
	const AActor* HitTarget = HitResult.GetActor();
	const FString SurfaceName = UWvCommonUtils::GetSurfaceName(HitSurfaceType).ToString();
	//UE_LOG(LogTemp, Log, TEXT("found HitActor => %s, SurfaceName => %s, function => %s"), *GetNameSafe(HitTarget), *SurfaceName, *FString(__FUNCTION__));

	IWvEnvironmentInterface* Interface = Cast<IWvEnvironmentInterface>(const_cast<AActor*>(HitTarget));
	if (Interface)
	{
		Interface->OnReceiveAbilityAttack(const_cast<AActor*>(Attacker), HitResult);
	}

	FHitReactEnvironmentRow* RowData = HitEffectDTInstance->FindRow<FHitReactEnvironmentRow>(FName(SurfaceName), "");
	if (RowData)
	{
		UWorld* World = Attacker->GetWorld();

		const FVector HitLocation = HitResult.ImpactPoint;
		FRotator FaceRotation;

		const FEnvironmentConfig EnvironmentConfig = ASC_GLOBAL()->EnvironmentConfig;
		const float SoundVolume = EnvironmentConfig.HitSoundVolume;
		const float Roudness = EnvironmentConfig.Roudness;

		if (RowData->HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(World, RowData->HitSound, HitLocation, SoundVolume, 1.0f, 0.0f, nullptr, nullptr);
		}
		// Send nosie even if there is no sound asset
		UAISense_Hearing::ReportNoiseEvent(World, HitLocation, SoundVolume, const_cast<AActor*>(HitTarget), Roudness);


		if (RowData->HitEffect)
		{
			const FVector FaceDirection = (Attacker->GetActorLocation() - HitTarget->GetActorLocation()).GetSafeNormal();
			FaceRotation = UKismetMathLibrary::MakeRotFromX(UKismetMathLibrary::MirrorVectorByNormal(Attacker->GetActorForwardVector(), HitResult.ImpactNormal));
			UWvCommonUtils::SpawnParticleAtLocation(HitTarget, RowData->HitEffect, HitLocation, FaceRotation, FVector::OneVector);
		}


		//DrawDebugPoint(World, HitLocation, 20, FColor::Blue, false, 3.0f);
		//const FVector Direction = FaceRotation.RotateVector(FVector::ForwardVector);
		//DrawDebugLine(World, HitLocation, HitLocation + Direction * 100, FColor::Blue, false, 3.0f);
	}
}


