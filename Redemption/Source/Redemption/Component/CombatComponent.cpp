// Copyright 2022 wevet works All Rights Reserved.

#include "CombatComponent.h"
#include "InventoryComponent.h"
#include "WeaknessComponent.h"
#include "HitTargetComponent.h"
#include "Locomotion/LocomotionComponent.h"

#include "Misc/WvCommonUtils.h"
#include "Redemption.h"
#include "GameExtension.h"
#include "WvAbilityBase.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "WvAbilityDataAsset.h"
//#include "WvAbilitySystemTypes.h"
#include "WvGameplayEffectContext.h"
#include "Character/BaseCharacter.h"
#include "Character/WvAIController.h"
#include "Ability/WvAbilitySystemComponent.h"
#include "WvAbilitySystemGlobals.h"
#include "WvGameplayCueManager.h"
#include "Game/CombatInstanceSubsystem.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/WidgetComponent.h"

// sence
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Damage.h"

#define K_CHAIN_COMPONENT_TAG FName(TEXT("Chain"))


#include UE_INLINE_GENERATED_CPP_BY_NAME(CombatComponent)

using namespace CharacterDebug;


UCombatComponent::UCombatComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	Super::SetComponentTickEnabled(false);

	Character = Cast<ABaseCharacter>(GetOwner());
	ASC = Cast<UWvAbilitySystemComponent>(Character->GetAbilitySystemComponent());

	if (ASC.IsValid())
	{
		ASC->AbilityTagUpdateDelegate.AddDynamic(this, &ThisClass::OnTagUpdate);
	}

	if (Character.IsValid())
	{
		Character->OnTeamHandleAttackDelegate.AddDynamic(this, &ThisClass::OnSendAbilityAttack);
		Character->OnTeamWeaknessHandleAttackDelegate.AddDynamic(this, &ThisClass::OnSendWeaknessAttack);
	}

	TArray<UActorComponent*> Components = GetOwner()->GetComponentsByTag(UWidgetComponent::StaticClass(), K_CHAIN_COMPONENT_TAG);
	for (UActorComponent* Component : Components)
	{
		if (!ChainWidgetComponent.IsValid())
		{
			ChainWidgetComponent = Cast<UWidgetComponent>(Component);
		}
	}

}

void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ASC.IsValid())
	{
		ASC->AbilityTagUpdateDelegate.RemoveDynamic(this, &ThisClass::OnTagUpdate);
	}

	if (Character.IsValid())
	{
		Character->OnTeamHandleAttackDelegate.RemoveDynamic(this, &ThisClass::OnSendAbilityAttack);
		Character->OnTeamWeaknessHandleAttackDelegate.RemoveDynamic(this, &ThisClass::OnSendWeaknessAttack);
	}

	ChainWidgetComponent.Reset();
	Character.Reset();
	ASC.Reset();
	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.ClearTimer(ShakeBone_TimerHandle);
	TM.ClearTimer(TimerHandle);

	Super::EndPlay(EndPlayReason);
}

void UCombatComponent::BeginDestroy()
{
	Super::BeginDestroy();
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ComboChainSystem.IsPlaying())
	{
		ComboChainSystem.Update(DeltaTime);
	}
	else
	{
		ShowChainWidgetComponent(false);
		Super::SetComponentTickEnabled(false);
	}
}


#pragma region ChainCombo
void UCombatComponent::OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
	HandleChainComboUpdate();
}

void UCombatComponent::OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage)
{
	HandleChainComboUpdate();
}

void UCombatComponent::HandleChainComboUpdate()
{
	if (!IsCloseCombatWeapon())
	{
		return;
	}

	if (ComboChainSystem.IsPlaying())
	{
		ComboChainSystem.Push();
		return;
	}

	ShowChainWidgetComponent(true);
	ComboChainSystem.Begin();
	Super::SetComponentTickEnabled(true);
}

float UCombatComponent::GetProgressValue() const
{
	return ComboChainSystem.GetProgressValue();
}

bool UCombatComponent::IsChainPlaying() const
{
	return ComboChainSystem.IsPlaying();
}

void UCombatComponent::ShowChainWidgetComponent(const bool NewVisibility)
{
	if (ChainWidgetComponent.IsValid())
	{
		bIsChainWidgetShown = NewVisibility;
		ChainWidgetComponent->SetVisibility(NewVisibility);
	}

	//UE_LOG(LogTemp, Error, TEXT("widget => %s, function => %s"), *GetNameSafe(ChainWidgetComponent.Get()), *FString(__FUNCTION__));
}
#pragma endregion


#pragma region AbilityDamage
bool UCombatComponent::AbilityDamageBoxTrace(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FVector Start, const FVector End, FVector HalfSize, const FRotator Orientation, TArray<AActor*>& ActorsToIgnore)
{
	TArray<FWvBattleDamageAttackTargetInfo> HitTargetInfos;
	BoxTraceMulti(HitTargetInfos, Start, End, HalfSize, Orientation, ActorsToIgnore);

	if (HitTargetInfos.Num() == 0)
	{
		return false;
	}

	for (int32 Index = 0; Index < HitTargetInfos.Num(); ++Index)
	{
		ActorsToIgnore.Add(HitTargetInfos[Index].Target);
	}
	AbilityTraceAttackToASC(Ability, EffectGroupIndex, HitTargetInfos, Start);
	return true;
}

bool UCombatComponent::AbilityDamageCapsuleTrace(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FVector Start, const FVector End, const float Radius, const float HalfHeight, const FQuat CapsuleQuat, TArray<AActor*>& ActorsToIgnore)
{
	TArray<FWvBattleDamageAttackTargetInfo> HitTargetInfos;
	CapsuleTraceMulti(HitTargetInfos, Start, End, Radius, HalfHeight, CapsuleQuat, ActorsToIgnore);

	if (HitTargetInfos.Num() == 0)
	{
		return false;
	}

	for (int32 Index = 0; Index < HitTargetInfos.Num(); ++Index)
	{
		ActorsToIgnore.Add(HitTargetInfos[Index].Target);
	}
	AbilityTraceAttackToASC(Ability, EffectGroupIndex, HitTargetInfos, Start);
	return true;
}

void UCombatComponent::BoxTraceMulti(TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos, const FVector Start, const FVector End, const FVector HalfSize, const FRotator Orientation, const TArray<AActor*>& ActorsToIgnore)
{
	TArray<FHitResult> HitResults;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	bIsDebugTrace = (CVarDebugCombatSystem.GetValueOnGameThread() > 0);
#else
	bIsDebugTrace = false;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)


	UKismetSystemLibrary::BoxTraceMulti(
		GetWorld(),
		Start, End, HalfSize, Orientation,
		ASC_GLOBAL()->WeaponTraceChannel, false, ActorsToIgnore,
		bIsDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		HitResults, true,
		FLinearColor::Red, FLinearColor::Green, DrawTime);

	HitResultEnemyFilter(HitResults, HitTargetInfos);
}

void UCombatComponent::CapsuleTraceMulti(TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos, const FVector Start, const FVector End, const float Radius, const float HalfHeight, const FQuat CapsuleFquat, const TArray<AActor*>& ActorsToIgnore)
{
	TArray<FHitResult> HitResults;
	ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(ASC_GLOBAL()->WeaponTraceChannel);
	static const FName CapsuleTraceMultiName(TEXT("CapsuleTraceMulti"));
	const FCollisionQueryParams Params = UWvAbilitySystemBlueprintFunctionLibrary::ConfigureCollisionParams(CapsuleTraceMultiName, false, ActorsToIgnore, true, GetOwner());
	GetWorld()->SweepMultiByChannel(HitResults, Start, End, CapsuleFquat, CollisionChannel, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	bIsDebugTrace = (CVarDebugCombatSystem.GetValueOnGameThread() > 0);
#else
	bIsDebugTrace = false;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bIsDebugTrace)
	{
		const int32 DrawIndex = (int32)EDrawDebugTrace::ForDuration;
		UWvAbilitySystemBlueprintFunctionLibrary::DrawDebugCapsuleTraceMulti(GetWorld(), Start, End, CapsuleFquat, Radius, HalfHeight, DrawIndex, HitResults.Num() > 0, HitResults, FLinearColor::Red, FLinearColor::Green, DrawTime);
	}
#endif

	HitResultEnemyFilter(HitResults, HitTargetInfos);
}

void UCombatComponent::HitResultEnemyFilter(TArray<FHitResult>& Hits, TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos)
{
	if (Hits.Num() < 0)
	{
		return;
	}

	IWvAbilityTargetInterface* SourceObj = Cast<IWvAbilityTargetInterface>(Character);
	FWvTargetDataFilter TargetDataFilter = FWvTargetDataFilter();

	TMap<AActor*, FWvBattleDamageAttackTargetInfo> TargetCharacterInfos;

	for (int32 Index = 0; Index < Hits.Num(); ++Index)
	{
		const FHitResult HitResult = Hits[Index];
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor)
		{
			continue;
		}

		FWvBattleDamageAttackTargetInfo* InfoPointer = TargetCharacterInfos.Find(HitActor);
		UPrimitiveComponent* HitComponent = HitResult.GetComponent();
		UHitTargetComponent* HitTargetComponent = nullptr;

		if (HitComponent && HitComponent->GetClass()->IsChildOf(UHitTargetComponent::StaticClass()))
		{
			HitTargetComponent = Cast<UHitTargetComponent>(HitComponent);
			if (!InfoPointer)
			{
				FWvBattleDamageAttackTargetInfo InfoInstance = FWvBattleDamageAttackTargetInfo();
				InfoPointer = &InfoInstance;
				TargetCharacterInfos.Add(HitActor, *InfoPointer);
			}
		}

		if (!InfoPointer || !InfoPointer->Target)
		{
			if (SourceObj == nullptr || TargetDataFilter.FilterPassesForActor(SourceObj, HitActor))
			{
				if (!HitActor->GetClass()->IsChildOf(ABaseCharacter::StaticClass()))
				{
					// Character hit specifies the use of PhysicsAsset collision box
					if (HitComponent == nullptr || !HitComponent->GetClass()->IsChildOf(USkeletalMeshComponent::StaticClass()))
					{
						UE_LOG(LogTemp, Warning, TEXT("Character hit specifies the use of PhysicsAsset collision box"));
						continue;
					}
				}

				if (!InfoPointer)
				{
					FWvBattleDamageAttackTargetInfo InfoInstance = FWvBattleDamageAttackTargetInfo();
					InfoPointer = &InfoInstance;
					TargetCharacterInfos.Add(HitActor, *InfoPointer);
				}

				InfoPointer->HitResult = HitResult;
				InfoPointer->Target = HitActor;
			}
			else
			{
				//UE_LOG(LogTemp, Warning, TEXT("not enemy ?? => %s, function => %s"), *GetNameSafe(HitActor), *FString(__FUNCTION__));
			}
		}

		if (!InfoPointer)
		{
			continue;
		}

		if (HitTargetComponent)
		{
			InfoPointer->WeaknessNames.Add(HitTargetComponent->GetAttachBoneName());
		}

		TargetCharacterInfos[HitActor] = *InfoPointer;
	}

	for (TPair<AActor*, FWvBattleDamageAttackTargetInfo>Pair : TargetCharacterInfos)
	{
		if (Pair.Value.Target)
		{
			HitTargetInfos.Add(Pair.Value);
		}

	}

}

/// <summary>
/// Call WvAT_BulletDamage
/// </summary>
const bool UCombatComponent::LineOfSightTraceOuter(class UWvAbilityBase* Ability, const int32 WeaponID, const int32 EffectGroupIndex, TArray<FHitResult>& Hits, const FVector SourceLocation)
{
	LineOfSightTraceOuterEnvironments(Ability, EffectGroupIndex, Hits, SourceLocation);
	return BulletTraceAttackToAbilitySystemComponent(Ability, WeaponID, EffectGroupIndex, Hits, SourceLocation);
}

/// <summary>
/// HitBy Environments
/// </summary>
/// <returns></returns>
const bool UCombatComponent::LineOfSightTraceOuterEnvironments(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const TArray<FHitResult>& HitResults, const FVector SourceLocation)
{
	for (const FHitResult HitResult : HitResults)
	{
		LineOfSightTraceOuterEnvironment(Ability, EffectGroupIndex, HitResult, SourceLocation);
		if (HasEnvironmentFilterClass(HitResult))
		{

		}
	}
	return true;
}

/// <summary>
/// HitBy Environment
/// </summary>
/// <returns></returns>
const bool UCombatComponent::LineOfSightTraceOuterEnvironment(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, const FHitResult& HitResult, const FVector SourceLocation)
{
	if (!IsValid(HitResult.GetActor()))
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid HitResult GetActor => %s"), *FString(__FUNCTION__));
		return false;
	}

	UCombatInstanceSubsystem::Get()->OnHitEnvironment(GetOwner(), HitResult);

#if false
	FGameplayCueParameters CueParameter;
	CueParameter.EffectContext = ASC->MakeEffectContext();
	CueParameter.Location = HitResult.ImpactPoint;
	CueParameter.Normal = HitResult.ImpactNormal;
	CueParameter.PhysicalMaterial = HitResult.PhysMaterial;
	CueParameter.Instigator = Character;
	ABILITY_GLOBAL()->GetGameplayCueManager()->HandleGameplayCue(HitResult.GetActor(),
		TAG_GameplayCue_HitImpact_Environment_BulletHit, EGameplayCueEvent::Type::Executed, CueParameter);

#endif
	return true;
}

/// <summary>
/// Hit Effect by Environment
/// </summary>
/// <param name="HitResult"></param>
/// <returns></returns>
const bool UCombatComponent::HasEnvironmentFilterClass(const FHitResult& HitResult)
{
	if (IsValid(HitResult.GetActor()))
	{
		const auto FindClass = ASC_GLOBAL()->BulletHitFilterClasses.FindByPredicate([&](UClass* Class)
		{
			return (HitResult.GetActor()->GetClass()->IsChildOf(Class));
		});

		if (FindClass)
		{
			return true;
		}
	}
	return false;
}

/// <summary>
/// Attack from bullet infos
/// </summary>
const bool UCombatComponent::BulletTraceAttackToAbilitySystemComponent(class UWvAbilityBase* Ability, const int32 WeaponID, const int32 EffectGroupIndex, TArray<FHitResult>& Hits, const FVector SourceLocation)
{
	TArray<FWvBattleDamageAttackTargetInfo> HitTargetInfos;
	HitResultEnemyFilter(Hits, HitTargetInfos);

	if (HitTargetInfos.Num() == 0)
	{
		return false;
	}

	FWvBattleDamageAttackSourceInfo SourceInfo = FWvBattleDamageAttackSourceInfo();
	SourceInfo.SourceType = EWvBattleDamageAttackSourceType::Bullet;
	SourceInfo.SourceAbility = Ability;
	SourceInfo.WeaponID = WeaponID;
	const UWvAbilityDataAsset* AbilityData = Ability->GetWvAbilityDataNoChecked();
	UWvAbilityEffectDataAsset* EffectDA = AbilityData->EffectDataAsset;

	if (!IsValid(EffectDA))
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid EffectDA => %s"), *FString(__FUNCTION__));
		return false;
	}

	AttackToASC(SourceInfo, HitTargetInfos, EffectDA, EffectGroupIndex, SourceLocation);
	return true;
}

void UCombatComponent::AbilityTraceAttackToASC(class UWvAbilityBase* Ability, const int32 EffectGroupIndex, TArray<FWvBattleDamageAttackTargetInfo> HitTargetInfos, const FVector SourceLocation)
{
	FWvBattleDamageAttackSourceInfo SourceInfo = FWvBattleDamageAttackSourceInfo();
	SourceInfo.SourceType = EWvBattleDamageAttackSourceType::BasicMelee;
	SourceInfo.SourceAbility = Ability;
	const UWvAbilityDataAsset* AbilityData = Ability->GetWvAbilityDataNoChecked();
	UWvAbilityEffectDataAsset* EffectDA = AbilityData->EffectDataAsset;

	if (!IsValid(EffectDA))
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid EffectDA => %s"), *FString(__FUNCTION__));
		return;
	}

	AttackToASC(SourceInfo, HitTargetInfos, EffectDA, EffectGroupIndex, SourceLocation);
}

void UCombatComponent::AttackToASC(const FWvBattleDamageAttackSourceInfo SourceInfo, TArray<FWvBattleDamageAttackTargetInfo> HitInfos, class UWvAbilityEffectDataAsset* EffectDA, const int32 EffectGroupIndex, const FVector SourceLocation)
{
	if (!ASC.IsValid() || !IsValid(EffectDA))
	{
		return;
	}

	for (auto& HitInfo : HitInfos)
	{
		FWvBattleDamageAttackTargetInfo HitTargetInfo = HitInfo;
		if (!IsValid(HitTargetInfo.Target))
		{
			continue;
		}

		IWvAbilityTargetInterface* ATI = Cast<IWvAbilityTargetInterface>(HitTargetInfo.Target);
		if (ATI && ATI->IsDead())
		{
			continue;
		}

		FGameplayAbilityTargetDataHandle TargetDataHandle;
		FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
		UWvAbilitySystemBlueprintFunctionLibrary::EffectContextSetEffectDataAsset(EffectContextHandle, EffectDA, EffectGroupIndex);
		FWvGameplayAbilityTargetData* TargetData = new FWvGameplayAbilityTargetData();
		TargetData->TargetInfo = HitTargetInfo;
		TargetData->SourceInfo = SourceInfo;
		TargetData->SourceLocation = SourceLocation;
		TargetData->SourceActorLocation = Character->GetActorLocation();
		TargetDataHandle.Add(TargetData);
		EffectContextHandle.AddHitResult(HitTargetInfo.HitResult);
		EffectContextHandle.AddOrigin(SourceLocation);
		ASC->MakeEffectToTargetData(EffectContextHandle, TargetDataHandle, FGameplayEffectQuery());
	}
}
#pragma endregion


#pragma region HitReaction
FGameplayTag UCombatComponent::GetHitReactFeature() const
{
	return HitReactFeature;
}

FGameplayTag UCombatComponent::GetWeaknessHitReactFeature() const
{
	return WeaknessHitReactFeature;
}

bool UCombatComponent::GetIsFixedHitReactFeature() const
{
	return IsFixedHitReactFeature;
}

bool UCombatComponent::HasBoneShaking() const
{
	return false;
}

void UCombatComponent::SetHitReactFeature(const FGameplayTag Tag, const bool bIsFixed)
{
	HitReactFeature = Tag;
	IsFixedHitReactFeature = bIsFixed;
}

void UCombatComponent::SetWeaknessHitReactFeature(const FGameplayTag Tag)
{
	WeaknessHitReactFeature = Tag;
}

TArray<class UBoneShakeExecuteData*> UCombatComponent::GetBoneShakeDatas() const
{
	if (SkeletalMeshBoneShakeExecuteData)
	{
		return SkeletalMeshBoneShakeExecuteData->BoneShakeDatas;
	}
	return {};
}

void UCombatComponent::OnTagUpdate(const FGameplayTag Tag, const bool bIsTagExists)
{
	//if (Tag == TAG_Character_StateDead)
	if (Tag == TAG_Common_PassiveAbilityTrigger_KillReact)
	{
		IsDead = bIsTagExists;
		if (IsDead)
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UCombatComponent::HandleDeath, 0.01, true);
			UE_LOG(LogTemp, Error, TEXT("HandleDeath => %s"), *FString(__FUNCTION__));
		}
	}
}

void UCombatComponent::HandleDeath()
{
	if (!IsDead)
	{
		return;
	}

	if (!ASC.IsValid())
	{
		return;
	}

	//if (ASC->HasMatchingGameplayTag(StateDead) && !ASC->HasMatchingGameplayTag(StateHitReact))
	//{
	//	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	//	FGameplayEventData Payload;
	//	ASC->HandleGameplayEvent(TAG_Character_StateDead_Action, &Payload);
	//}
}

void UCombatComponent::StartHitReact(FGameplayEffectContextHandle& Context, const bool bIsDeath, const float Damage)
{
	UApplyEffectExData* EffectExData = UWvAbilitySystemBlueprintFunctionLibrary::GetEffectExData(Context);

	FWvGameplayAbilityTargetData* TargetData = nullptr;
	FWvGameplayAbilityTargetData GameplayAbilityTargetData;
	if (UWvAbilitySystemBlueprintFunctionLibrary::GetGameplayAbilityTargetData(Context, GameplayAbilityTargetData))
	{
		TargetData = &GameplayAbilityTargetData;
	}

	AActor* Attacker = Context.GetInstigatorAbilitySystemComponent()->GetAvatarActor();

	bool bIsFixed = false;
	FGameplayTag SelectHitReactFeature;

	if (EffectExData)
	{
		bIsFixed = EffectExData->TargetHitReact.IsFixed;
		SelectHitReactFeature = bIsDeath ? EffectExData->TargetHitReact.FixFeatureTag : EffectExData->TargetHitReact.FeatureTag;
	}

	if (!SelectHitReactFeature.IsValid())
	{
		SelectHitReactFeature = bIsDeath ? TAG_Config_HitReactFeature_Dead : TAG_Config_HitReactFeature_Hit;
	}
	SetHitReactFeature(SelectHitReactFeature, bIsFixed);

	if (bIsDeath)
	{
		//if (Character->DeadAnimPlayModeType == EDeadAnimPlayModeType::Immediately)
		//{
		//	ASC->RemoveGameplayTag(StateHitReact);
		//}
		//UGameplayEffect* DeadGE = (ABILITY_GLOBAL()->DeadGE).GetDefaultObject();
		//if (DeadGE)
		//{
		//	FActiveGameplayEffectHandle DeadGEHandle = ASC->ApplyGameplayEffectToSelf(DeadGE, 1.0f, ASC->MakeEffectContext());
		//	ASC->SetDeadStateEffectHandle(DeadGEHandle);
		//}
		// UWvAbility_Death Applied
		//FGameplayEventData GameplayEventData;
		//GameplayEventData.ContextHandle = Context;
		//GameplayEventData.Instigator = Attacker;
		//GameplayEventData.Target = Character.Get();
		//GameplayEventData.EventMagnitude = Damage;
		//ASC->HandleGameplayEvent(TAG_Common_PassiveAbilityTrigger_KillReact, &GameplayEventData);
	}
	else
	{
		// UWvAbility_Repel Applied
		FGameplayEventData GameplayEventData;
		GameplayEventData.ContextHandle = Context;
		GameplayEventData.Instigator = Attacker;
		ASC->HandleGameplayEvent(TAG_Common_PassiveAbilityTrigger_HitReact, &GameplayEventData);

		// Send AIPerceptionComponent
		auto HitResult = Context.GetHitResult();
		const FVector EventLocation = Character->GetActorLocation();
		UAISense_Damage::ReportDamageEvent(GetWorld(), Character.Get(), Attacker, Damage, EventLocation, HitResult->ImpactPoint);
	}

	FName* WeaknessName = nullptr;
	FName* WeaknessBoneName = nullptr;
	FWvBattleDamageAttackSourceInfo* SourceInfoPtr = nullptr;
	EAttackWeaponState WeaponState = EAttackWeaponState::InValid;

	// DamageAccumulationInfo
	//if (DamageAccumulationInfo->IsTrigger())
	//{
	//}


	if (TargetData)
	{
		if (TargetData->TargetInfo.WeaknessNames.Num() > 0)
		{
			// @TODO
			UInventoryComponent* InventoryComp = Cast<UInventoryComponent>(Attacker->GetComponentByClass(UInventoryComponent::StaticClass()));
			if (InventoryComp)
			{
				WeaponState = InventoryComp->GetEquipWeaponType();
			}

			auto Local_WName = TargetData->TargetInfo.GetMaxDamageWeaknessName();
			const FName HitBoneName = Context.GetHitResult()->BoneName;
			WeaknessName = &Local_WName;
			const FCharacterWeaknessData WeaknessData = Character->GetWeaknessComponent()->FindCharacterWeaknessData(WeaponState, HitBoneName);

			for (FName BoneName : TargetData->TargetInfo.WeaknessNames)
			{
				UE_LOG(LogTemp, Log, TEXT("TargetInfo.WeaknessName => %s"), *BoneName.ToString());
			}
			//WeaknessBoneName = &AttachBoneName;
			//WeaknessHitReactEventCallbak(Attacker, *WeaknessName, Damage);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("empty weakness names => %s"), *FString(__FUNCTION__));
		}

		SourceInfoPtr = &TargetData->SourceInfo;
	}

	if (EffectExData)
	{
		if (EffectExData->CueConfig.CommitTargetDataExecuteCueTag.IsValid())
		{
			FGameplayCueParameters CueParam(Context);
			ASC->ExecuteGameplayCue(EffectExData->CueConfig.CommitTargetDataExecuteCueTag, CueParam);
		}

		const UWvHitReactDataAsset* HitReactDA = Character->GetHitReactDataAsset();
		FGameplayTag StrengthTag = WeaknessName ? EffectExData->BoneShakeConfig.WeaknessBoneShakeStrengthTag : EffectExData->BoneShakeConfig.DefaultBoneShakeStrengthTag;
		FGameplayTag DefaultStrengthTag = WeaknessName ? TAG_Character_HitReact_Default_Weakness : TAG_Character_HitReact_Default_Streangth;
		StrengthTag = StrengthTag != FGameplayTag::EmptyTag ? StrengthTag : DefaultStrengthTag;
		const FName HitBoneName = WeaknessBoneName ? *WeaknessBoneName : Context.GetHitResult()->BoneName;

		if (HitReactDA && StrengthTag != FGameplayTag::EmptyTag && !HitBoneName.IsNone())
		{
			if (!SkeletalMeshBoneShakeExecuteData)
			{
				SkeletalMeshBoneShakeExecuteData = NewObject<USkeletalMeshBoneShakeExecuteData>(GetWorld());
			}

			if (TargetData)
			{
				SkeletalMeshBoneShakeExecuteData->HitDirection = (Context.GetHitResult()->Location - TargetData->SourceLocation).GetSafeNormal();
			}
			if (SkeletalMeshBoneShakeExecuteData->HitDirection == FVector::ZeroVector && Attacker)
			{
				SkeletalMeshBoneShakeExecuteData->HitDirection = (Character->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal();
			}
			if (SkeletalMeshBoneShakeExecuteData->HitDirection == FVector::ZeroVector)
			{
				SkeletalMeshBoneShakeExecuteData->HitDirection = Context.GetHitResult()->Normal;
			}

			if (SkeletalMeshBoneShakeExecuteData->HitDirection.IsNearlyZero())
			{
				SkeletalMeshBoneShakeExecuteData->HitDirection = FVector(FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f), FMath::RandRange(-1.f, 1.f));
			}

			//if (SkeletalMeshBoneShakeExecuteData->HitDirection == FVector::ZeroVector)
			//{

			//}

			SkeletalMeshBoneShakeExecuteData->BoneShakeDatas.Reset();

			FGameplayTag TriggerTag = HitReactDA->BoneShakeTriggerTag;
			const FGameplayTag DefaultTriggerTag = TAG_Character_HitReact_Default_Trigger;
			TriggerTag = TriggerTag != FGameplayTag::EmptyTag ? TriggerTag : DefaultTriggerTag;

			StartBoneShake(HitBoneName, TriggerTag, StrengthTag);

			if (SkeletalMeshBoneShakeExecuteData->BoneShakeDatas.Num() > 0)
			{
				const float DT = GetWorld()->GetDeltaSeconds();
				FTimerManager& TM = GetWorld()->GetTimerManager();

				TM.SetTimer(ShakeBone_TimerHandle, this, &ThisClass::TickUpdateUpdateBoneShake, DT, true);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("unmatch condition, function => %s"), *FString(__FUNCTION__));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("nullptr EffectExData, function => %s"), *FString(__FUNCTION__));
	}
}

void UCombatComponent::WeaknessHitReactEventCallback(const AActor* AttackActor, const FName WeaknessName, const float HitValue)
{

}

void UCombatComponent::StartBoneShake(const FName HitBoneName, const FGameplayTag BoneShakeTriggerTag, const FGameplayTag BoneShakeStrengthTag)
{
	if (!HitReactBoneShakeDA || BoneShakeTriggerTag == FGameplayTag::EmptyTag || BoneShakeStrengthTag == FGameplayTag::EmptyTag)
	{
		return;
	}

	//UE_LOG(LogTemp, Log, TEXT("function => %s"), *FString(__FUNCTION__));
	FSkeletalMeshShakeData* SkeletalMeshShakeData = HitReactBoneShakeDA->SkeletalShakeData.Find(BoneShakeTriggerTag);
	if (!SkeletalMeshShakeData)
	{
		SkeletalMeshShakeData = HitReactBoneShakeDA->SkeletalShakeData.Find(TAG_Character_ShakeBone_Default_Trigger);
	}

	if (!SkeletalMeshShakeData)
	{
		UE_LOG(LogTemp, Error, TEXT("not valid SkeletalMeshShakeData => [%s]"), *FString(__FUNCTION__));
		return;
	}

	FHitReactBoneShakeStrengthConfig* HitReactBoneShakeStrengthConfig = SkeletalMeshShakeData->StrengthBoneShakeData.Find(BoneShakeStrengthTag);
	if (!HitReactBoneShakeStrengthConfig)
	{
		HitReactBoneShakeStrengthConfig = SkeletalMeshShakeData->StrengthBoneShakeData.Find(TAG_Character_ShakeBone_Default_Streangth);
	}

	if (!HitReactBoneShakeStrengthConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("not valid HitReactBoneShakeStrengthConfig => [%s]"), *FString(__FUNCTION__));
		return;
	}

	FNearestShakableBone* CurNearestShakableBone = SkeletalMeshShakeData->NearestShakableBoneData.Find(HitBoneName);
	if (!CurNearestShakableBone || CurNearestShakableBone->Bone.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("not valid CurNearestShakableBone => [%s]"), *FString(__FUNCTION__));
		return;
	}

	const float NearestShakableBoneStrength = CurNearestShakableBone->Weight;
	const float GloablStrength = HitReactBoneShakeStrengthConfig->Strength;
	const FName MainShakeBoneName = CurNearestShakableBone->Bone;

	FHitReactBoneShake* MainHitReactBoneShake = HitReactBoneShakeStrengthConfig->BoneShakeData.Find(MainShakeBoneName);
	if (!MainHitReactBoneShake)
	{
		UE_LOG(LogTemp, Error, TEXT("not valid MainHitReactBoneShake => [%s]"), *FString(__FUNCTION__));
		return;
	}

	TArray<FName> BoneNames;
	Character->GetMesh()->GetBoneNames(BoneNames);

	for (int32 Index = 0; Index < BoneNames.Num(); ++Index)
	{
		const FName BoneName = BoneNames[Index];
		FTransform BoneTransform;
		if (!UWvCommonUtils::GetBoneTransForm(Character->GetMesh(), BoneName, BoneTransform))
		{
			continue;
		}

		UBoneShakeExecuteData* ExecuteData = NewObject<UBoneShakeExecuteData>(GetWorld());
		ExecuteData->BoneName = BoneName;
		ExecuteData->SourceLocation = BoneTransform.GetLocation();

		if (!SkeletalMeshShakeData->LockBoneNams.Contains(BoneName))
		{
			// main bone
			if (MainShakeBoneName == BoneName)
			{
				ExecuteData->Strength = NearestShakableBoneStrength * GloablStrength * MainHitReactBoneShake->ShakeStrength;
				ExecuteData->TotalTime = MainHitReactBoneShake->ShakeDuration;
				ExecuteData->DampingCurve = MainHitReactBoneShake->DampingCurve;
				ExecuteData->Direction = FMath::RandRange(0.f, 1.f) > 0.5f ? 1 : -1;
			}
			else
			{
				// Rest of the trembling bones
				FNearestShakableBone* NearestShakableBone = SkeletalMeshShakeData->NearestShakableBoneData.Find(BoneName);
				if (NearestShakableBone)
				{
					FName NearestShakeBoneName = NearestShakableBone->Bone;

					// Jiggle Bone in Main
					if (NearestShakeBoneName == MainShakeBoneName)
					{
						ExecuteData->Strength = NearestShakableBoneStrength * GloablStrength * MainHitReactBoneShake->ShakeStrength * NearestShakableBone->Weight;
						ExecuteData->TotalTime = MainHitReactBoneShake->ShakeDuration;
						ExecuteData->DampingCurve = MainHitReactBoneShake->DampingCurve;
						ExecuteData->Direction = FMath::RandRange(0.f, 1.f) > 0.5f ? 1 : -1;
					}
					else
					{
						// Other Jiggle Bone
						float* TransmitStrength = MainHitReactBoneShake->Transmits.OtherBoneTransmitShakeStrength.Find(NearestShakeBoneName);
						if (TransmitStrength && *TransmitStrength > 0)
						{
							if (FHitReactBoneShake* BoneShakeDataPtr = HitReactBoneShakeStrengthConfig->BoneShakeData.Find(NearestShakeBoneName))
							{
								ExecuteData->Strength = NearestShakableBoneStrength * GloablStrength * BoneShakeDataPtr->ShakeStrength * (*TransmitStrength) * NearestShakableBone->Weight;
								ExecuteData->TotalTime = BoneShakeDataPtr->ShakeDuration;
								ExecuteData->DampingCurve = BoneShakeDataPtr->DampingCurve;
								ExecuteData->Direction = FMath::RandRange(0.f, 1.f) > 0.5f ? 1 : -1;
							}
						}
					}
				}
			}
		}

		if (ExecuteData->Strength == 0)
		{
			ExecuteData->TotalTime = 0;
		}

		SkeletalMeshBoneShakeExecuteData->BoneShakeDatas.Add(ExecuteData);
	}
}

void UCombatComponent::TickUpdateUpdateBoneShake()
{
	auto World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float DT = World->GetDeltaSeconds();
	if (!UpdateBoneShake(DT))
	{
		FTimerManager& TM = World->GetTimerManager();
		TM.ClearTimer(ShakeBone_TimerHandle);

		ShakeBone_TimerHandle.Invalidate();
	}
}

bool UCombatComponent::UpdateBoneShake(const float DeltaTime) const
{
	if (!SkeletalMeshBoneShakeExecuteData || SkeletalMeshBoneShakeExecuteData->BoneShakeDatas.Num() <= 0)
	{
		return false;
	}

	auto PC = Game::ControllerExtension::GetPlayer(GetWorld());
	if (!IsValid(PC))
	{
		return false;
	}

	FRotator PlayerControllerRotator = PC->GetControlRotation();

	bool bHasShakeing = false;
	for (int32 Index = 0; Index < SkeletalMeshBoneShakeExecuteData->BoneShakeDatas.Num(); ++Index)
	{
		UBoneShakeExecuteData* ExecuteData = SkeletalMeshBoneShakeExecuteData->BoneShakeDatas[Index];

		if (ExecuteData->CurTime == ExecuteData->TotalTime)
		{
			ExecuteData->ShakeOffsetLocation = FVector::ZeroVector;
			continue;
		}

		bHasShakeing = true;

		FVector ShakeDirection = SkeletalMeshBoneShakeExecuteData->HitDirection;
		ShakeDirection *= ExecuteData->Direction;
		ExecuteData->Direction *= -1;

		ShakeDirection = UWvCommonUtils::ChangePositonByRotation(PlayerControllerRotator.Yaw, ShakeDirection);

		ExecuteData->CurTime += DeltaTime;
		ExecuteData->CurTime = ExecuteData->CurTime > ExecuteData->TotalTime ? ExecuteData->TotalTime : ExecuteData->CurTime;
		const float Alpha = ExecuteData->DampingCurve ? ExecuteData->DampingCurve->GetFloatValue(ExecuteData->CurTime / ExecuteData->TotalTime) : 1.0f;
		const float Strength = Alpha * ExecuteData->Strength;
		ExecuteData->ShakeOffsetLocation = ShakeDirection * Strength;
	}

	return bHasShakeing;
}

#pragma endregion


#pragma region BattleCommand
/// <summary>
/// has blackboard target?
/// </summary>
/// <returns></returns>
bool UCombatComponent::HasAttackTarget() const
{
	AWvAIController* AIC = Cast<AWvAIController>(Character.Get()->GetController());
	if (IsValid(AIC))
	{
		return !AIC->IsTargetDead();
	}
	return false;
}

/// <summary>
/// apply to vehicle system etc
/// </summary>
/// <param name="InHidden"></param>
void UCombatComponent::VisibilityCurrentWeapon(const bool InHidden)
{
	auto Inventory = Character->GetInventoryComponent();
	if (Inventory)
	{
		auto Weapon = Inventory->GetEquipWeapon();
		if (Weapon)
		{
			Weapon->SetActorHiddenInGame(InHidden);
		}
	}
}

void UCombatComponent::UnEquipWeapon()
{
	Modify_Weapon(ELSOverlayState::None);
}

void UCombatComponent::EquipPistol()
{
	Modify_Weapon(ELSOverlayState::Pistol);
}

void UCombatComponent::EquipRifle()
{
	Modify_Weapon(ELSOverlayState::Rifle);
}

void UCombatComponent::EquipKnife()
{
	Modify_Weapon(ELSOverlayState::Knife);
}

void UCombatComponent::SetAiming(const bool InAiming)
{
	ULocomotionComponent* Comp = Character->GetLocomotionComponent();
	if (Comp)
	{
		Comp->SetLSAiming(InAiming);
	}
}

void UCombatComponent::Modify_Weapon(const ELSOverlayState LSOverlayState)
{
	auto Inventory = Character->GetInventoryComponent();
	if (Inventory)
	{
		bool bCanAttack = false;
		auto WeaponType = Inventory->ConvertWeaponState(LSOverlayState, bCanAttack);
		const bool bResult = Inventory->ChangeAttackWeapon(WeaponType);

		if (bResult)
		{
			Character->OverlayStateChange(LSOverlayState);
		}
	}
}

void UCombatComponent::EquipAvailableWeapon()
{
	auto Inventory = Character->GetInventoryComponent();
	if (Inventory)
	{
		AWeaponBaseActor* Weapon = Inventory->GetAvailableWeapon();

#if false
		switch (AttackWeaponState)
		{
		case EAttackWeaponState::EmptyWeapon:
			Weapon = Inventory->FindWeaponItem(ELSOverlayState::None);
			break;
		case EAttackWeaponState::Gun:
		case EAttackWeaponState::Rifle:
		case EAttackWeaponState::Bomb:
		case EAttackWeaponState::Knife:
			Weapon = Inventory->GetAvailableWeapon();
			break;
		}
#endif

		if (Weapon)
		{
			ELSOverlayState LSOverlayState;
			const bool bResult = Inventory->ChangeWeapon(Weapon, LSOverlayState);

			if (bResult)
			{
				Character->OverlayStateChange(LSOverlayState);
			}
		}
	}
}

void UCombatComponent::EquipAvailableWeaponToDistance(const float Threshold)
{
	auto Inventory = Character->GetInventoryComponent();
	if (Inventory)
	{
		AWeaponBaseActor* Weapon = Inventory->GetAvailableWeaponToDistance(Threshold);

		if (Weapon)
		{
			ELSOverlayState LSOverlayState;
			const bool bResult = Inventory->ChangeWeapon(Weapon, LSOverlayState);

			if (bResult)
			{
				Character->OverlayStateChange(LSOverlayState);
			}
		}
	}
}

bool UCombatComponent::IsCloseCombatWeapon() const
{
	auto Inventory = Character->GetInventoryComponent();
	const EAttackWeaponState WeaponType = Inventory->GetEquipWeaponType();
	return WeaponType == EAttackWeaponState::EmptyWeapon || WeaponType == EAttackWeaponState::Knife;
}
#pragma endregion


#pragma region Follow
void UCombatComponent::AddFollower(APawn* NewPawn)
{
	if (!IsValid(NewPawn))
	{
		return;
	}

	if (!Followers.Contains(NewPawn))
	{
		Followers.Add(NewPawn);

		AWvAIController* AIC = Cast<AWvAIController>(NewPawn->GetController());
		if (IsValid(AIC))
		{
			AIC->Notify_Follow();
		}
	}
}

void UCombatComponent::RemoveFollower(APawn* RemovePawn)
{
	if (!IsValid(RemovePawn))
	{
		return;
	}

	if (Followers.Contains(RemovePawn))
	{
		Followers.Remove(RemovePawn);

		AWvAIController* AIC = Cast<AWvAIController>(RemovePawn->GetController());
		if (IsValid(AIC))
		{
			AIC->Notify_UnFollow();
		}
	}
}

void UCombatComponent::RemoveAllFollowers()
{
	for (TWeakObjectPtr<APawn> Follower : Followers)
	{
		if (!Follower.IsValid())
		{
			continue;
		}

		AWvAIController* AIC = Cast<AWvAIController>(Follower.Get()->GetController());
		if (IsValid(AIC))
		{
			AIC->Notify_UnFollow(true);
		}
		Follower.Reset();
	}
	Followers.Reset(0);
}

const FVector UCombatComponent::GetFormationPoint(const APawn* InPawn)
{
	if (!IsValid(InPawn))
	{
		return Character->GetActorLocation();
	}

	const int32 Num = Followers.Num();
	int32 SelectIndex = 0;
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const TWeakObjectPtr<APawn> Follow = Followers[Index];
		if (Follow.Get() == InPawn)
		{
			SelectIndex = Index;
			break;
		}
	}

	TArray<FVector> Points;
	UWvCommonUtils::CircleSpawnPoints(Num, ASC_GLOBAL()->BotLeaderConfig.FormationRadius, Character->GetActorLocation(), Points);
	return Points[SelectIndex];
}

bool UCombatComponent::CanFollow() const
{
	const int32 StackCount = IsValid(ASC_GLOBAL()) ? ASC_GLOBAL()->BotLeaderConfig.FollowStackCount : K_FOLLOW_NUM;
	return Followers.Num() < StackCount;
}
#pragma endregion


