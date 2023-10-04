// Copyright 2020 wevet works All Rights Reserved.

#include "WvGameplayTargetData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "WvAbilitySystemGlobals.h"
#include "Interface/WvAbilityTargetInterface.h"
#include "PhysicsEngine/PhysicsAsset.h"

#pragma region FWvGameplayAbilityTargetData
bool FWvGameplayAbilityTargetData::HasHitResult() const
{
	return true;
}

const FHitResult* FWvGameplayAbilityTargetData::GetHitResult() const
{
	return &TargetInfo.HitResult;
}

bool FWvGameplayAbilityTargetData::HasOrigin() const
{
	return true;
}

FTransform FWvGameplayAbilityTargetData::GetOrigin() const
{
	FTransform Origin = FTransform();
	Origin.SetLocation(SourceLocation);
	return Origin;
}

AActor* FWvGameplayAbilityTargetData::GetTargetActor() const
{
	return TargetInfo.Target;
}

bool FWvGameplayAbilityTargetData::GetSlicePlaneNormal(FVector& OutNormal) const
{
	return false;
}

UPrimitiveComponent* FWvGameplayAbilityTargetData::GetComponent() const
{
	return TargetInfo.HitResult.GetComponent();
}

int32 FWvGameplayAbilityTargetData::GetBoneIndex() const
{
	return -1;
}

TArray<TWeakObjectPtr<AActor>> FWvGameplayAbilityTargetData::GetActors() const
{
	TArray<TWeakObjectPtr<AActor>> Actors;
	AActor* HitActor = GetTargetActor();
	if (HitActor)
	{
		Actors.Push(HitActor);
	}
	return Actors;
}

TArray<FActiveGameplayEffectHandle> FWvGameplayAbilityTargetData::ApplyGameplayEffectSpec(FGameplayEffectSpec& InSpec, FPredictionKey PredictionKey /*= FPredictionKey()*/)
{
	TArray<FActiveGameplayEffectHandle>	AppliedHandles;

	if (!ensure(InSpec.GetContext().IsValid()))
	{
		return AppliedHandles;
	}

	TArray<TWeakObjectPtr<AActor>> Actors = GetActors();
	if (Actors.Num() <= 0)
	{
		return AppliedHandles;
	}

	FGameplayEffectSpec	SpecToApply(InSpec);
	FGameplayEffectContextHandle EffectContext = SpecToApply.GetContext().Duplicate();
	SpecToApply.SetContext(EffectContext);
	AddTargetDataToContext(EffectContext, true);

	AActor* TargetActor = Actors[0].Get();
	UAbilitySystemComponent* TargetComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	UAbilitySystemComponent* SourceComponent = EffectContext.GetInstigatorAbilitySystemComponent();
	if (TargetComponent && SourceComponent)
	{
		FActiveGameplayEffectHandle ActiveGEHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(SpecToApply, TargetComponent, PredictionKey);
		
		if(ActiveGEHandle.IsValid())
		{
			AppliedHandles.Add(ActiveGEHandle);
		}
	}
	return AppliedHandles;
}
#pragma endregion


#pragma region FWvGameplayAbilityTargetData_SingleTarget
bool FWvGameplayAbilityTargetData_SingleTarget::GetSlicePlaneNormal(FVector& OutNormal) const
{
	return false;
}

UPrimitiveComponent* FWvGameplayAbilityTargetData_SingleTarget::GetComponent() const
{
	return Overlap.GetComponent();
}

TArray<TWeakObjectPtr<AActor> > FWvGameplayAbilityTargetData_SingleTarget::GetActors() const
{
	TArray<TWeakObjectPtr<AActor>>	Actors;
	if (Overlap.GetActor())
	{
		Actors.Push(Overlap.GetActor());
	}
	return Actors;
}

AActor* FWvGameplayAbilityTargetData_SingleTarget::GetTargetActor() const
{
	return Overlap.GetActor();
}

int32 FWvGameplayAbilityTargetData_SingleTarget::GetBoneIndex() const
{
	return Overlap.ItemIndex;
}

bool FWvGameplayAbilityTargetData_SingleTarget::HasEndPoint() const
{
	return true;
}

FVector FWvGameplayAbilityTargetData_SingleTarget::GetEndPoint() const
{
	return Overlap.GetActor() ? Overlap.GetActor()->GetActorLocation() : FVector::ZeroVector;
}

bool FWvGameplayAbilityTargetData_SingleTarget::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Overlap.NetSerialize(Ar, Map, bOutSuccess);
	SourceLocation.NetSerialize(Ar, Map, bOutSuccess);
	bOutSuccess = true;
	return true;
}
#pragma endregion


#pragma region FWvGameplayAbilityTargetData_SingleTargetHit
TArray<TWeakObjectPtr<AActor> > FWvGameplayAbilityTargetData_SingleTargetHit::GetActors() const
{
	TArray<TWeakObjectPtr<AActor> >	Actors;
	if (IsValid(HitResult.GetActor()))
	{
		Actors.Push(HitResult.GetActor());
	}
	return Actors;
}

AActor* FWvGameplayAbilityTargetData_SingleTargetHit::GetTargetActor() const
{
	return HitResult.GetActor();
}

bool FWvGameplayAbilityTargetData_SingleTargetHit::HasHitResult() const
{
	return true;
}

const FHitResult* FWvGameplayAbilityTargetData_SingleTargetHit::GetHitResult() const
{
	return &HitResult;
}

bool FWvGameplayAbilityTargetData_SingleTargetHit::GetSlicePlaneNormal(FVector& OutNormal) const
{
	OutNormal = SlicePlaneNormal;
	return true;
}

UPrimitiveComponent* FWvGameplayAbilityTargetData_SingleTargetHit::GetComponent() const
{
	return HitResult.GetComponent();
}

int32 FWvGameplayAbilityTargetData_SingleTargetHit::GetBoneIndex() const
{
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(HitResult.GetComponent()))
	{
		UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset();
		if (PhysicsAsset)
		{
			return PhysicsAsset->FindBodyIndex(HitResult.BoneName);
		}		
	}
	return -1;
}

bool FWvGameplayAbilityTargetData_SingleTargetHit::HasEndPoint() const
{
	return true;
}

FVector FWvGameplayAbilityTargetData_SingleTargetHit::GetEndPoint() const
{
	return HitResult.Location;
}

bool FWvGameplayAbilityTargetData_SingleTargetHit::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	HitResult.NetSerialize(Ar, Map, bOutSuccess);
	SourceLocation.NetSerialize(Ar, Map, bOutSuccess);
	SlicePlaneNormal.NetSerialize(Ar, Map, bOutSuccess);
	return true;
}

TArray<FActiveGameplayEffectHandle> FWvGameplayAbilityTargetData_SingleTargetHit::ApplyGameplayEffectSpec(FGameplayEffectSpec& InSpec, FPredictionKey PredictionKey)
{
	TArray<FActiveGameplayEffectHandle>	AppliedHandles;
	
	if (!ensure(InSpec.GetContext().IsValid()))
	{
		return AppliedHandles;
	}

	TArray<TWeakObjectPtr<AActor> > Actors = GetActors();
	if (Actors.Num() <= 0)
	{
		return AppliedHandles;
	}

	FGameplayEffectSpec	SpecToApply(InSpec);
	FGameplayEffectContextHandle EffectContext = SpecToApply.GetContext().Duplicate();
	SpecToApply.SetContext(EffectContext);
	AddTargetDataToContext(EffectContext, true);
	AActor* TargetActor = Actors[0].Get();

	UWvAbilitySystemGlobals::Get()->SetCustomEffectContext(TargetActor,SpecToApply);
	
	UAbilitySystemComponent* TargetComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	UAbilitySystemComponent* SourceComponent = EffectContext.GetInstigatorAbilitySystemComponent();

	// TargetActorがAvatarActorと同じであることを確認し、弾丸のような召喚獣がヒットを受けたときにSourceASCにGEが送信されないようにする。
	AActor* AvatarActor = TargetComponent ? TargetComponent->GetAvatarActor_Direct() : nullptr;
	if (TargetComponent && SourceComponent && AvatarActor == TargetActor)
	{
		FActiveGameplayEffectHandle ActiveGEHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(SpecToApply, TargetComponent, PredictionKey);
		
		if(ActiveGEHandle.IsValid())
		{
			AppliedHandles.Add(ActiveGEHandle);
		}
	}

	return AppliedHandles;
}
#pragma endregion

