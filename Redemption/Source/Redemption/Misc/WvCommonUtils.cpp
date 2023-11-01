// Copyright 2022 wevet works All Rights Reserved.


#include "Misc/WvCommonUtils.h"
#include "Character/BaseCharacter.h"
#include "Component/CombatComponent.h"
#include "Component/InventoryComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"


float UWvCommonUtils::GetAngleBetweenVector(FVector Vec1, FVector Vec2)
{
	Vec1.Normalize();
	Vec2.Normalize();
	return FMath::RadiansToDegrees(FMath::Acos(Vec1 | Vec2));
}

float UWvCommonUtils::GetAngleBetween3DVector(FVector Vec1, FVector Vec2)
{
	Vec1.Normalize();
	Vec2.Normalize();
	float Angle = FMath::RadiansToDegrees(FMath::Acos(Vec1 | Vec2));
	if (FVector::DotProduct(FVector::CrossProduct(Vec1, Vec2), FVector::UpVector) < 0)
	{
		Angle = -Angle;
	}

	return Angle;
}

float UWvCommonUtils::GetAngleBetween3DVector(FVector Vec1, FVector Vec2, FVector RefUpVector)
{
	Vec1.Normalize();
	Vec2.Normalize();
	float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct(Vec1, Vec2));
	if (FVector::DotProduct(FVector::CrossProduct(Vec1, Vec2), RefUpVector) < 0)
	{
		Angle = -Angle;
	}
	return Angle;
}

FTransform UWvCommonUtils::TransformSubStract(const FTransform& TransformA, const FTransform& TransformB)
{
	FVector Location = TransformA.GetLocation() - TransformB.GetLocation();
	FVector Scale = TransformA.GetScale3D() - TransformB.GetScale3D();
	FQuat Rotation = TransformA.GetRotation() - TransformA.GetRotation();
	return FTransform(Rotation, Location, Scale);
}

FTransform UWvCommonUtils::TransformAdd(const FTransform& TransformA, const FTransform& TransformB)
{
	FVector Location = TransformA.GetTranslation() + TransformB.GetTranslation();
	FVector Scale = TransformA.GetScale3D() + TransformB.GetScale3D();
	FQuat Rotation = TransformA.GetRotation() + TransformB.GetRotation();
	return FTransform(Rotation, Location, Scale);
}

FLSComponentAndTransform UWvCommonUtils::ComponentWorldToLocal(const FLSComponentAndTransform WorldSpaceComponent)
{
	FLSComponentAndTransform LocalSpaceComponent;
	LocalSpaceComponent.Component = WorldSpaceComponent.Component;
	// DisplayName GetWorldTransform
	LocalSpaceComponent.Transform = WorldSpaceComponent.Transform * UKismetMathLibrary::InvertTransform(WorldSpaceComponent.Component->K2_GetComponentToWorld());
	return LocalSpaceComponent;
}

FLSComponentAndTransform UWvCommonUtils::ComponentLocalToWorld(const FLSComponentAndTransform LocalSpaceComponent)
{
	FLSComponentAndTransform WorldSpaceComponent;
	WorldSpaceComponent.Component = LocalSpaceComponent.Component;
	WorldSpaceComponent.Transform = LocalSpaceComponent.Transform * WorldSpaceComponent.Component->K2_GetComponentToWorld();
	return WorldSpaceComponent;
}

FTransform UWvCommonUtils::TransformMinus(const FTransform A, const FTransform B)
{
	FTransform Out = FTransform::Identity;
	Out.SetLocation(A.GetLocation() - B.GetLocation());
	Out.SetScale3D(A.GetScale3D() - B.GetScale3D());

	const float Roll = (A.GetRotation().Rotator().Roll - B.GetRotation().Rotator().Roll);
	const float Pitch = (A.GetRotation().Rotator().Pitch - B.GetRotation().Rotator().Pitch);
	const float Yaw = (A.GetRotation().Rotator().Yaw - B.GetRotation().Rotator().Yaw);
	Out.SetRotation(FQuat(FRotator(Pitch, Yaw, Roll)));
	return Out;
}

FTransform UWvCommonUtils::TransformPlus(const FTransform A, const FTransform B)
{
	FTransform Out = FTransform::Identity;
	Out.SetLocation(A.GetLocation() + B.GetLocation());
	Out.SetScale3D(A.GetScale3D() + B.GetScale3D());

	const float Roll = (A.GetRotation().Rotator().Roll + B.GetRotation().Rotator().Roll);
	const float Pitch = (A.GetRotation().Rotator().Pitch + B.GetRotation().Rotator().Pitch);
	const float Yaw = (A.GetRotation().Rotator().Yaw + B.GetRotation().Rotator().Yaw);
	Out.SetRotation(FQuat(FRotator(Pitch, Yaw, Roll)));
	return Out;
}

UFXSystemComponent* UWvCommonUtils::SpawnParticleAtLocation(const UObject* WorldContextObject, UFXSystemAsset* Particle, FVector Location, FRotator Rotation, FVector Scale)
{
	if (Particle)
	{
		if (Particle->IsA(UNiagaraSystem::StaticClass()))
		{
			return UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, Cast<UNiagaraSystem>(Particle), Location, Rotation, Scale);
		}
	}
	return nullptr;
}

UFXSystemComponent* UWvCommonUtils::SpawnParticleAttached(UFXSystemAsset* Particle, USceneComponent* Component, FName BoneName, FVector Location, FRotator Rotation, FVector Scale, EAttachLocation::Type LocationType)
{
	if (Particle)
	{
		if (Particle->IsA(UNiagaraSystem::StaticClass()))
		{
			return UNiagaraFunctionLibrary::SpawnSystemAttached(Cast<UNiagaraSystem>(Particle), Component, BoneName, Location, Rotation, Scale, LocationType, true, ENCPoolMethod::None);
		}
	}

	return nullptr;
}

bool UWvCommonUtils::GetBoneTransForm(const USkeletalMeshComponent* MeshComp, const FName BoneName, FTransform& OutBoneTransform)
{
	const int32 BoneIndex = MeshComp->GetBoneIndex(BoneName);
	if (BoneIndex != INDEX_NONE)
	{
		OutBoneTransform = MeshComp->GetBoneTransform(BoneIndex);
		return true;
	}
	else
	{
		USkeletalMeshSocket const* socket = MeshComp->GetSocketByName(BoneName);
		if (socket)
		{
			OutBoneTransform = MeshComp->GetSocketTransform(BoneName);
			return true;
		}
	}
	return false;
}

bool UWvCommonUtils::IsHost(const AController* Controller)
{
	if (!IsValid(Controller))
	{
		UE_LOG(LogTemp, Error, TEXT("UWvCommonUtils::IsHost->Controller is nullptr"));
		return false;
	}
	return (Controller->HasAuthority() && Controller->IsLocalController() && !IsBot(Controller));
}

bool UWvCommonUtils::IsBot(const AController* Controller)
{
	if (!IsValid(Controller))
	{
		UE_LOG(LogTemp, Error, TEXT("UWvCommonUtils::IsBot -> Controller is nullptr"));
		return false;
	}

	const UClass* Class = Controller->GetClass();
	if (!IsValid(Class))
	{
		UE_LOG(LogTemp, Error, TEXT("UWvCommonUtils::IsBot -> Class is nullptr"));
		return false;
	}
	//Is a BOT
	if (!Class->IsChildOf(AAIController::StaticClass())) 
		return false;
	return true;
}

FHitReactInfoRow* UWvCommonUtils::FindHitReactInfoRow(ABaseCharacter* Character)
{
	IWvAbilitySystemAvatarInterface* Avatar = Cast<IWvAbilitySystemAvatarInterface>(Character);
	if (!Avatar)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s : Avatar is null.]"), *FString(__FUNCTION__));
		return nullptr;
	}

	const FWvAbilitySystemAvatarData& AbilitySystemData = Avatar->GetAbilitySystemData();
	UWvHitReactDataAsset* HitReactDA = AbilitySystemData.HitReactData;
	UCombatComponent* CombatComponent = Character->GetCombatComponent();
	const UInventoryComponent* InventoryComponent = Character->GetInventoryComponent();

	if (!IsValid(CombatComponent) || !IsValid(HitReactDA) || !IsValid(InventoryComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s : CombatComponent or HitReactDA or InventoryComponent is null.]"), *FString(__FUNCTION__));
		return nullptr;
	}

	const FGameplayTag HitReactFeatureTag = CombatComponent->GetHitReactFeature();
	const bool bIsFixed = CombatComponent->GetIsFixedHitReactFeature();
	const FGameplayTag WeaknesshitReactFeatureTag = CombatComponent->GetWeaknessHitReactFeature();
	CombatComponent->SetWeaknessHitReactFeature(FGameplayTag::EmptyTag);

	const FName WeaponName = InventoryComponent->GetEquipWeaponName();

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);

	FHitReactInfoRow* HitReactInfo = nullptr;

	if (WeaknesshitReactFeatureTag.IsValid())
	{
		HitReactInfo = const_cast<FHitReactInfoRow*>(HitReactDA->GetHitReactInfoRow_Special(ASC, WeaknesshitReactFeatureTag));
	}

	if (!HitReactInfo && bIsFixed)
	{
		HitReactInfo = const_cast<FHitReactInfoRow*>(HitReactDA->GetHitReactInfoRow_Special(ASC, HitReactFeatureTag));
	}

	if (!HitReactInfo && !WeaponName.IsNone())
	{
		HitReactInfo = const_cast<FHitReactInfoRow*>(HitReactDA->GetHitReactInfoRow_Weapon(WeaponName, ASC, HitReactFeatureTag));
	}

	if (!HitReactInfo)
	{
		HitReactInfo = const_cast<FHitReactInfoRow*>(HitReactDA->GetHitReactInfoRow_Normal(ASC, HitReactFeatureTag));
	}

	return HitReactInfo;
}


