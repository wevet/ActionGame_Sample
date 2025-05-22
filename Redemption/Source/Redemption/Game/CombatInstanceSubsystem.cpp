// Copyright 2022 wevet works All Rights Reserved.


#include "Game/CombatInstanceSubsystem.h"
#include "WvAbilitySystemGlobals.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "WvGameplayEffectContext.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvGameplayCue_HitEnvironment.h"
#include "Game/WvGameInstance.h"

#include "Kismet/KismetMathLibrary.h"
#include "Perception/AISense_Hearing.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"



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
	LoadAllEnvironmentVFXAssets();
}


void UCombatInstanceSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

}


void UCombatInstanceSubsystem::LoadAllEnvironmentVFXAssets()
{
	UAssetManager& AM = UAssetManager::Get();

	const FPrimaryAssetId SingleId(
		FPrimaryAssetType("EnvironmentVFXDataAsset"),
		FName("DA_EnvironmentVFXCommon")
	);

	AM.LoadPrimaryAssets(
		{ SingleId }, {},
		FStreamableDelegate::CreateUObject(this, &UCombatInstanceSubsystem::OnEnvironmentVFXDataAssetsLoaded), 0);
}


void UCombatInstanceSubsystem::OnEnvironmentVFXDataAssetsLoaded()
{

	UAssetManager& AM = UAssetManager::Get();

	const FPrimaryAssetId SingleId(
		FPrimaryAssetType("EnvironmentVFXDataAsset"),
		FName("DA_EnvironmentVFXCommon")
	);

	EnvironmentVFXDataAsset = Cast<UEnvironmentVFXDataAsset>(AM.GetPrimaryAssetObject(SingleId));

	if (EnvironmentVFXDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Success EnvironmentVFXyDA: [%s]"), *FString(__FUNCTION__));
	}
}


void UCombatInstanceSubsystem::OnHitEnvironment(const AActor* Attacker, const FHitResult& HitResult)
{
	if (!EnvironmentVFXDataAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("not valid EnvironmentVFXDataAsset => %s"), *FString(__FUNCTION__));
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

	bool bIsAssetFound = false;
	const FVFXBaseAsset& VFXAsset = GetVFXAssets(FName(SurfaceName), FGameplayTag::EmptyTag, bIsAssetFound);

	UWorld* World = Attacker->GetWorld();

	if (bIsAssetFound && World)
	{
		const FVector HitLocation = HitResult.ImpactPoint;
		FRotator FaceRotation;

		const FEnvironmentConfig& EnvironmentConfig = ASC_GLOBAL()->EnvironmentConfig;
		const float SoundVolume = EnvironmentConfig.HitSoundVolume;
		const float Roudness = EnvironmentConfig.Roudness;

		if (IsValid(VFXAsset.Sound))
		{
			UGameplayStatics::PlaySoundAtLocation(World, VFXAsset.Sound, HitLocation, SoundVolume, 1.0f, 0.0f, nullptr, nullptr);
		}
		// Send nosie even if there is no sound asset
		UAISense_Hearing::ReportNoiseEvent(World, HitLocation, SoundVolume, const_cast<AActor*>(HitTarget), Roudness);


		if (IsValid(VFXAsset.Effect))
		{
			const FVector FaceDirection = (Attacker->GetActorLocation() - HitTarget->GetActorLocation()).GetSafeNormal();
			FaceRotation = UKismetMathLibrary::MakeRotFromX(UKismetMathLibrary::MirrorVectorByNormal(Attacker->GetActorForwardVector(), HitResult.ImpactNormal));
			UWvCommonUtils::SpawnParticleAtLocation(HitTarget, VFXAsset.Effect, HitLocation, FaceRotation, FVector::OneVector);
		}


		//DrawDebugPoint(World, HitLocation, 20, FColor::Blue, false, 3.0f);
		//const FVector Direction = FaceRotation.RotateVector(FVector::ForwardVector);
		//DrawDebugLine(World, HitLocation, HitLocation + Direction * 100, FColor::Blue, false, 3.0f);
	}

}

const FVFXBaseAsset& UCombatInstanceSubsystem::GetVFXAssets(const FName SurfaceName, const FGameplayTag InTag, bool& bOutFound) const
{
	bOutFound = false;

	if (!IsValid(EnvironmentVFXDataAsset))
	{
		static const FVFXBaseAsset DefaultAsset;
		return DefaultAsset;
	}

	const FEnvironmentVFXAssetContainer* Container = EnvironmentVFXDataAsset->DataMap.Find(SurfaceName);
	if (Container)
	{
		if (InTag.IsValid())
		{
			for (const FEnvironmentVFXAsset& Asset : Container->DataArray)
			{
				if (Asset.OverrideAssetTag == InTag)
				{
					bOutFound = true;
					return Asset;
				}
			}
		}

		bOutFound = true;
		return Container->DefaultBaseAsset;
	}

	static const FVFXBaseAsset DefaultAsset;
	return DefaultAsset;
}



