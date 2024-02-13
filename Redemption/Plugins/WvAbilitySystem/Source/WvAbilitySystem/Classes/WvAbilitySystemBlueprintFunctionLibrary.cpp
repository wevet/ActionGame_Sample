// Copyright 2020 wevet works All Rights Reserved.


#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "WvGameplayEffectContext.h"
#include "WvGameplayTargetData.h"
#include "WvAbilityDataAsset.h"
#include "WvAbilityAttributeSet.h"
#include "Interface/WvAbilityTargetInterface.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Components/CapsuleComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "Runtime/Launch/Resources/Version.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilitySystemBlueprintFunctionLibrary)

static const float KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE = 16.f;


UWvAbilityDataAsset* UWvAbilitySystemBlueprintFunctionLibrary::GetAbilityData(const UGameplayAbility* Ability)
{
	check(Ability)
	UObject* SourceObj = Ability->GetCurrentSourceObject();
	check(SourceObj)
	return CastChecked<UWvAbilityDataAsset>(SourceObj);
}

UGameplayEffect* UWvAbilitySystemBlueprintFunctionLibrary::NewModifyAttributeGE(FGameplayAttribute Attribute, float Magnitude, FGameplayTag GETag, UObject* Outer /*= nullptr*/)
{
	if (Outer == nullptr)
	{
		Outer = GetTransientPackage();
	}

	UGameplayEffect* InstanceGE = NewObject<UGameplayEffect>(Outer);
	const int32 Idx = InstanceGE->Modifiers.Num();
	InstanceGE->Modifiers.SetNum(Idx + 1);
	FGameplayModifierInfo& Info = InstanceGE->Modifiers[Idx];
	Info.ModifierMagnitude = FScalableFloat(Magnitude);
	Info.ModifierOp = EGameplayModOp::Additive;
	Info.Attribute = Attribute;

#if (ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION >= 3)
	FInheritedTagContainer Container;
	Container.AddTag(GETag);
	UAssetTagsGameplayEffectComponent& AssetTagsComponent = InstanceGE->FindOrAddComponent<UAssetTagsGameplayEffectComponent>();
	AssetTagsComponent.SetAndApplyAssetTagChanges(Container);
#else
	InstanceGE->InheritableGameplayEffectTags.AddTag(GETag);
#endif

	InstanceGE->DurationPolicy = EGameplayEffectDurationType::Instant;
	return InstanceGE;
}

void UWvAbilitySystemBlueprintFunctionLibrary::FilterOverlaps(const TArray<FWvOverlapResult>& Overlaps, TArray<FWvOverlapResult>& OutOverlaps, TScriptInterface<IWvAbilityTargetInterface> Source, const FWvTargetDataFilter& TargetDataFilter)
{
	IWvAbilityTargetInterface* SourceObj = Cast<IWvAbilityTargetInterface>(Source.GetObject());

	TMap<AActor*, FWvOverlapResult*> OverlapActors;
	for (const FWvOverlapResult& Overlap : Overlaps)
	{
		if (!Overlap.Actor.IsValid())
		{
			continue;
		}

		AActor* HitActor = Overlap.Actor.Get();

		FWvOverlapResult** findRes = OverlapActors.Find(HitActor);
		if (!findRes)
		{
			if (SourceObj == nullptr || TargetDataFilter.FilterPassesForActor(SourceObj, HitActor))
			{
				FWvOverlapResult& newItem = OutOverlaps.Add_GetRef(Overlap);
				OverlapActors.Add(HitActor, &newItem);
			}
		}
		else
		{
			if ((*findRes)->ItemIndex == INDEX_NONE && Overlap.ItemIndex != INDEX_NONE)
			{
				(*findRes)->Component = Overlap.GetComponent();
				(*findRes)->ItemIndex = Overlap.ItemIndex;
			}

		}
	}
}

void UWvAbilitySystemBlueprintFunctionLibrary::FilterActors(const TArray<AActor*>& Actors, TArray<AActor*>& OutActors, TScriptInterface<IWvAbilityTargetInterface> Source, const FWvTargetDataFilter& TargetDataFilter)
{
	IWvAbilityTargetInterface* SourceObj = Cast<IWvAbilityTargetInterface>(Source.GetObject());

	TSet<AActor*> HitActors;
	for (AActor* HitActor : Actors)
	{
		if (!HitActor)
		{
			continue;
		}

		if (!HitActors.Find(HitActor))
		{
			if (SourceObj == nullptr || TargetDataFilter.FilterPassesForActor(SourceObj, HitActor))
			{
				HitActors.Add(HitActor);
				OutActors.Add(HitActor);
			}
		}
	}
}

FGameplayAbilityTargetDataHandle UWvAbilitySystemBlueprintFunctionLibrary::MakeTargetDataHandleFromHitResults(const TArray<FHitResult>& Hits, FVector SourceLocation, AActor* AbilityOwner, FWvAbilityData EffectAbilityData, AActor* HitActor)
{
	return MakeTargetDataHandleFromHitResultsAndSlicePlaneNormal(Hits, SourceLocation, FVector(), EffectAbilityData, HitActor);
}

FGameplayAbilityTargetDataHandle UWvAbilitySystemBlueprintFunctionLibrary::MakeTargetDataHandleFromHitResultsAndSlicePlaneNormal(const TArray<FHitResult>& Hits, FVector SourceLocation, FVector SlicePlaneNormal, FWvAbilityData EffectAbilityData, AActor* HitActor)
{
	FGameplayAbilityTargetDataHandle TargetDataHandle;

	for (const FHitResult& Hit : Hits)
	{
		FWvGameplayAbilityTargetData_SingleTargetHit* NewData = new FWvGameplayAbilityTargetData_SingleTargetHit();
		NewData->HitResult = Hit;
		NewData->SourceLocation = SourceLocation;
		NewData->SlicePlaneNormal = SlicePlaneNormal;
		NewData->AbilityData = EffectAbilityData;
		NewData->HitActor = HitActor;
		TargetDataHandle.Add(NewData);
	}
	return TargetDataHandle;
}

FCollisionQueryParams UWvAbilitySystemBlueprintFunctionLibrary::ConfigureCollisionParams(const FName TraceTag, const bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, const bool bIgnoreSelf, UObject* WorldContextObject)
{
	FCollisionQueryParams Params(TraceTag, SCENE_QUERY_STAT_ONLY(KismetTraceUtils), bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	// Ask for face index, as long as we didn't disable globally
	Params.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable;
	Params.AddIgnoredActors(ActorsToIgnore);
	if (bIgnoreSelf)
	{
		AActor* IgnoreActor = Cast<AActor>(WorldContextObject);
		if (IgnoreActor)
		{
			Params.AddIgnoredActor(IgnoreActor);
		}
		else
		{
			// find owner
			UObject* CurrentObject = WorldContextObject;
			while (CurrentObject)
			{
				CurrentObject = CurrentObject->GetOuter();
				IgnoreActor = Cast<AActor>(CurrentObject);
				if (IgnoreActor)
				{
					Params.AddIgnoredActor(IgnoreActor);
					break;
				}
			}
		}
	}
	return Params;
}

void UWvAbilitySystemBlueprintFunctionLibrary::DrawDebugCapsuleTraceMulti(const UWorld* World, const FVector& Start, const FVector& End, const FQuat CapsuleQuat, const float Radius, const float HalfHeight, const int32 DrawDebugTypeIndex, bool bHit, const TArray<FHitResult>& OutHits, const FLinearColor TraceColor, const FLinearColor TraceHitColor, const float DrawTime)
{
	const EDrawDebugTrace::Type DrawDebugType = (EDrawDebugTrace::Type)DrawDebugTypeIndex;

	if (DrawDebugType != EDrawDebugTrace::None)
	{
		const bool bPersistent = DrawDebugType == EDrawDebugTrace::Persistent;
		const float LifeTime = (DrawDebugType == EDrawDebugTrace::ForDuration) ? DrawTime : 0.f;

		if (bHit && OutHits.Last().bBlockingHit)
		{
			// Red up to the blocking hit, green thereafter
			FVector const BlockingHitPoint = OutHits.Last().Location;
			::DrawDebugCapsule(World, Start, HalfHeight, Radius, CapsuleQuat, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugCapsule(World, BlockingHitPoint, HalfHeight, Radius, CapsuleQuat, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, Start, BlockingHitPoint, TraceColor.ToFColor(true), bPersistent, LifeTime);

			::DrawDebugCapsule(World, End, HalfHeight, Radius, CapsuleQuat, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, BlockingHitPoint, End, TraceHitColor.ToFColor(true), bPersistent, LifeTime);
		}
		else
		{
			// no hit means all red
			::DrawDebugCapsule(World, Start, HalfHeight, Radius, CapsuleQuat, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugCapsule(World, End, HalfHeight, Radius, CapsuleQuat, TraceColor.ToFColor(true), bPersistent, LifeTime);
			::DrawDebugLine(World, Start, End, TraceColor.ToFColor(true), bPersistent, LifeTime);
		}

		// draw hits
		for (int32 HitIdx = 0; HitIdx < OutHits.Num(); ++HitIdx)
		{
			FHitResult const& Hit = OutHits[HitIdx];
			::DrawDebugPoint(World, Hit.ImpactPoint, KISMET_TRACE_DEBUG_IMPACTPOINT_SIZE, (Hit.bBlockingHit ? TraceColor.ToFColor(true) : TraceHitColor.ToFColor(true)), bPersistent, LifeTime);
		}
	}
}

FWvGameplayAbilityTargetData* UWvAbilitySystemBlueprintFunctionLibrary::GetGameplayAbilityTargetData(FGameplayEffectContextHandle EffectContextHandle)
{
	FWvGameplayAbilityTargetData* TargetData = nullptr;
	FWvGameplayEffectContext* GEContext = static_cast<FWvGameplayEffectContext*>(EffectContextHandle.Get());
	if (GEContext)
	{
		FGameplayAbilityTargetDataHandle TargetDataHandle = GEContext->GetTargetDataHandle();
		TargetData = (FWvGameplayAbilityTargetData*)(TargetDataHandle.Get(0));
	}
	return TargetData;
}

FGameplayEffectContextHandle UWvAbilitySystemBlueprintFunctionLibrary::EffectContextSetEffectDataAsset(FGameplayEffectContextHandle EffectContextHandle, UWvAbilityEffectDataAsset* DataAsset, const int32 Index)
{
	FWvGameplayEffectContext* GEContext = static_cast<FWvGameplayEffectContext*>(EffectContextHandle.Get());
	if (GEContext)
	{
		GEContext->SetEffectDataAsset(DataAsset, Index);
	}
	return EffectContextHandle;
}

bool UWvAbilitySystemBlueprintFunctionLibrary::GetGameplayAbilityTargetData(FGameplayEffectContextHandle EffectContextHandle, FWvGameplayAbilityTargetData& TargetData)
{
	FWvGameplayEffectContext* GEContext = static_cast<FWvGameplayEffectContext*>(EffectContextHandle.Get());
	if (GEContext)
	{
		FGameplayAbilityTargetDataHandle TargetDataHandle = GEContext->GetTargetDataHandle();
		FWvGameplayAbilityTargetData* TargetDataPtr = (FWvGameplayAbilityTargetData*)(TargetDataHandle.Get(0));
		if (TargetDataPtr)
		{
			TargetData = *TargetDataPtr;
			return true;
		}
	}
	return false;
}

bool UWvAbilitySystemBlueprintFunctionLibrary::EffectContextGetEffectGroup(FGameplayEffectContextHandle EffectContextHandle, FOnceApplyEffect& EffectGroup)
{
	FWvGameplayEffectContext* GEContext = static_cast<FWvGameplayEffectContext*>(EffectContextHandle.Get());
	if (GEContext)
	{
		UWvAbilityEffectDataAsset* DataAsset = GEContext->GetAbilityEffectDataAsset();
		if (DataAsset)
		{
			const int32 EffectGroupIdx = GEContext->GetEffectGroupIdx();
			if (DataAsset->AbilityEffectGroup.IsValidIndex(EffectGroupIdx))
			{
				EffectGroup = DataAsset->AbilityEffectGroup[EffectGroupIdx];
				return true;
			}
		}
	}
	return false;
}

class UApplyEffectExData* UWvAbilitySystemBlueprintFunctionLibrary::GetOnecEffectExData(FOnceApplyEffect& ApplyEffect)
{
	if (!ApplyEffect.ExData)
	{
		return nullptr;
	}
	return Cast<UApplyEffectExData>(ApplyEffect.ExData);
}

void UWvAbilitySystemBlueprintFunctionLibrary::SetAttributeSetValue(UWvAbilitySystemComponentBase* ASC, FGameplayAttribute Attribute, const float InValue)
{
	if (!ASC_GLOBAL()->GE_DynamicValue)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContexHandle = ASC->MakeEffectContext();
	if (!EffectContexHandle.IsValid())
	{
		return;
	}

	UGameplayEffect* GE = ASC_GLOBAL()->GE_DynamicValue->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectSpec* GESpec = new FGameplayEffectSpec(GE, EffectContexHandle, 1);

	FGameplayModifierInfo ModifierInfo = FGameplayModifierInfo();
	ModifierInfo.Attribute = Attribute;
	ModifierInfo.ModifierOp = EGameplayModOp::Override;
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(InValue);
	GE->Modifiers.Empty();
	GE->Modifiers.Add(ModifierInfo);

	ASC->ApplyGameplayEffectSpecToSelf(*GESpec);
}

void UWvAbilitySystemBlueprintFunctionLibrary::KillMySelf(AActor* Actor)
{
	do
	{
		if (!Actor)
		{
			break;
		}
		IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Actor);
		if (!Interface)
		{
			break;
		}
		UWvAbilitySystemComponentBase* ASC = Cast<UWvAbilitySystemComponentBase>(Interface->GetAbilitySystemComponent());
		if (!ASC)
		{
			break;
		}

		if (!ASC->HasAttributeSetForAttribute(UWvAbilityAttributeSet::GetDamageAttribute()) ||
			!ASC->HasAttributeSetForAttribute(UWvAbilityAttributeSet::GetHPAttribute()))
		{
			break;
		}
		const float CurrentHealth = ASC->GetNumericAttribute(UWvAbilityAttributeSet::GetHPAttribute());
		SetAttributeSetValue(ASC, UWvAbilityAttributeSet::GetDamageAttribute(), CurrentHealth);

	} while (false);
}

void UWvAbilitySystemBlueprintFunctionLibrary::RecoverHP(AActor* Actor, const float Value)
{
	do
	{
		if (!Actor)
		{
			break;
		}
		IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Actor);
		if (!Interface)
		{
			break;
		}
		UWvAbilitySystemComponentBase* ASC = Cast<UWvAbilitySystemComponentBase>(Interface->GetAbilitySystemComponent());
		if (!ASC)
		{
			break;
		}
		if (!ASC->HasAttributeSetForAttribute(UWvAbilityAttributeSet::GetRecoverHPAttribute()))
		{
			break;
		}
		SetAttributeSetValue(ASC, UWvAbilityAttributeSet::GetRecoverHPAttribute(), Value);

	} while (false);
}

void UWvAbilitySystemBlueprintFunctionLibrary::RecoverPercentHP(AActor* Actor, const float Percent)
{
	do
	{
		if (!Actor)
		{
			break;
		}

		IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Actor);
		if (!Interface)
		{
			break;
		}
		UWvAbilitySystemComponentBase* ASC = Cast<UWvAbilitySystemComponentBase>(Interface->GetAbilitySystemComponent());
		if (!ASC)
		{
			break;
		}

		if (!ASC->HasAttributeSetForAttribute(UWvAbilityAttributeSet::GetRecoverHPAttribute()) ||
			!ASC->HasAttributeSetForAttribute(UWvAbilityAttributeSet::GetHPMaxAttribute()))
		{
			break;
		}

		const float MaxHealth = ASC->GetNumericAttribute(UWvAbilityAttributeSet::GetHPMaxAttribute());
		const float RecoverHp = MaxHealth * Percent;
		SetAttributeSetValue(ASC, UWvAbilityAttributeSet::GetRecoverHPAttribute(), RecoverHp);

	} while (false);
}

void UWvAbilitySystemBlueprintFunctionLibrary::SetDamage(AActor* Actor, const float Value)
{
	do
	{
		if (!Actor)
		{
			break;
		}
		IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Actor);
		if (!Interface)
		{
			break;
		}
		UWvAbilitySystemComponentBase* ASC = Cast<UWvAbilitySystemComponentBase>(Interface->GetAbilitySystemComponent());
		if (!ASC)
		{
			break;
		}
		if (!ASC->HasAttributeSetForAttribute(UWvAbilityAttributeSet::GetDamageAttribute()))
		{
			break;
		}
		SetAttributeSetValue(ASC, UWvAbilityAttributeSet::GetDamageAttribute(), Value);

	} while (false);
}

class UApplyEffectExData* UWvAbilitySystemBlueprintFunctionLibrary::GetEffectExData(FGameplayEffectContextHandle ContextHandle)
{
	FOnceApplyEffect OnceApplyEffect;
	if (EffectContextGetEffectGroup(ContextHandle, OnceApplyEffect))
	{
		return GetOnecEffectExData(OnceApplyEffect);
	}
	return nullptr;
}

EHitReactDirection UWvAbilitySystemBlueprintFunctionLibrary::EvaluteHitReactDirection(const float Angle)
{
	if (Angle > -45.f && Angle <= 45.f)
	{
		return EHitReactDirection::Back;
	}
	else if (Angle > 45.f && Angle <= 135.f)
	{
		return EHitReactDirection::Right;
	}
	else if (Angle <= -45.f && Angle >= -135.f)
	{
		return EHitReactDirection::Left;
	}
	return EHitReactDirection::Forward;
}

EHitVerticalDirection UWvAbilitySystemBlueprintFunctionLibrary::EvaluteHitVerticalDirection(const FVector Direction)
{
	if (Direction.Z >= 0.4f) //0.8f
	{
		return EHitVerticalDirection::Top;
	}
	else if (Direction.Z <= -0.4f) //-0.8f
	{
		return EHitVerticalDirection::Bottom;
	}
	return EHitVerticalDirection::Middle;
}

FVector UWvAbilitySystemBlueprintFunctionLibrary::GetAttackDirection(FGameplayEffectContextHandle EffectContextHandle, const FVector ActorLocation)
{
	FVector* AttackDirectionPtr = nullptr;

	FWvGameplayEffectContext* EffectContext = static_cast<FWvGameplayEffectContext*>(EffectContextHandle.Get());
	if (EffectContext)
	{
		FGameplayAbilityTargetDataHandle TargetDataHandle = EffectContext->GetTargetDataHandle();
		const FWvGameplayAbilityTargetData* TargetData = static_cast<FWvGameplayAbilityTargetData*>(TargetDataHandle.Get(0));

		if (TargetData)
		{
			FVector AttackDirection = FVector(TargetData->SourceLocation - ActorLocation).GetSafeNormal();
			AttackDirectionPtr = &AttackDirection;
		}
	}
	
	if (!AttackDirectionPtr)
	{
		const FHitResult* HitResult = EffectContextHandle.GetHitResult();
		if (HitResult)
		{
			FVector AttackDirection = HitResult->Normal;
			AttackDirectionPtr = &AttackDirection;
		}
		else
		{
			FVector AttackDirection = FVector(EffectContextHandle.GetOrigin() - ActorLocation).GetSafeNormal();
			AttackDirectionPtr = &AttackDirection;
		}
	}

	return *AttackDirectionPtr;
}


