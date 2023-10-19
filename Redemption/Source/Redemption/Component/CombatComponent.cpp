// Copyright 2022 wevet works All Rights Reserved.

#include "CombatComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Redemption.h"
#include "WvAbilityBase.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"
#include "WvAbilityDataAsset.h"
#include "WvGameplayEffectContext.h"
#include "Character/BaseCharacter.h"
#include "Ability/WvAbilitySystemComponent.h"


#include "AbilitySystemGlobals.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarDebugCharacterCombatTrace(
	TEXT("wv.CharacterCombatDebugTrace"),
	0,
	TEXT("Charactermovement ledge end\n")
	TEXT("<=0: Debug off\n")
	TEXT(">=1: Debug on\n"),
	ECVF_Default);
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)


UCombatComponent::UCombatComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	AbilityTraceChannel = UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel3);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	Super::SetComponentTickEnabled(false);

	Character = Cast<ABaseCharacter>(GetOwner());
	ASC = Cast<UWvAbilitySystemComponent>(Character->GetAbilitySystemComponent());
}

void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Character.Reset();
	ASC.Reset();
	Super::EndPlay(EndPlayReason);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


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
	bIsDebugTrace = (CVarDebugCharacterCombatTrace.GetValueOnGameThread() > 0);
#else
	bIsDebugTrace = false;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)


	UKismetSystemLibrary::BoxTraceMulti(
		GetWorld(),
		Start, End, HalfSize, Orientation,
		AbilityTraceChannel, false, ActorsToIgnore,
		bIsDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		HitResults, true,
		FLinearColor::Red, FLinearColor::Green, DrawTime);

	HitResultEnemyFilter(HitResults, HitTargetInfos);
}

void UCombatComponent::CapsuleTraceMulti(TArray<FWvBattleDamageAttackTargetInfo>& HitTargetInfos, const FVector Start, const FVector End, const float Radius, const float HalfHeight, const FQuat CapsuleFquat, const TArray<AActor*>& ActorsToIgnore)
{
	TArray<FHitResult> HitResults;
	ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(AbilityTraceChannel);
	static const FName CapsuleTraceMultiName(TEXT("CapsuleTraceMulti"));
	const FCollisionQueryParams Params = UWvAbilitySystemBlueprintFunctionLibrary::ConfigureCollisionParams(CapsuleTraceMultiName, false, ActorsToIgnore, true, GetOwner());
	GetWorld()->SweepMultiByChannel(HitResults, Start, End, CapsuleFquat, CollisionChannel, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	bIsDebugTrace = (CVarDebugCharacterCombatTrace.GetValueOnGameThread() > 0);
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

	for (auto HitInfo : HitInfos)
	{
		FWvBattleDamageAttackTargetInfo HitTargetInfo = HitInfo;
		if (!IsValid(HitTargetInfo.Target))
		{
			continue;
		}

		if (HitTargetInfo.Target->GetClass()->IsChildOf(ABaseCharacter::StaticClass()))
		{
			ABaseCharacter* CastCharacter = Cast<ABaseCharacter>(HitTargetInfo.Target);
			if (CastCharacter && CastCharacter->IsDead())
			{
				continue;
			}
		}

		FGameplayAbilityTargetDataHandle TargetDataHandle;

		FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
		UWvAbilitySystemBlueprintFunctionLibrary::EffectContextSetEffectDataAsset(EffectContextHandle, EffectDA, EffectGroupIndex);

		UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
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


/// <summary>
/// @TODO
/// Weakness Component Create
/// </summary>
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
		FHitResult HitResult = Hits[Index];
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor)
		{
			continue;
		}

		FWvBattleDamageAttackTargetInfo* InfoPointer = TargetCharacterInfos.Find(HitActor);
		UPrimitiveComponent* HitComponent = HitResult.GetComponent();

		//UWeaknessEquipComponent* WeaknessEquipComponent = nullptr;
		//if (HitComponent && HitComponent->GetClass()->IsChildOf(UWeaknessEquipComponent::StaticClass()))
		//{
		//	WeaknessEquipComponent = Cast<UWeaknessEquipComponent>(HitComponent);
		//	if (!InfoPointer)
		//	{
		//		FBattleDamageAttackTargetInfo InfoInstance = FBattleDamageAttackTargetInfo();
		//		InfoPointer = &InfoInstance;
		//		TargetCharacterInfos.Add(HitActor, *InfoPointer);
		//	}
		//}

		if (!InfoPointer || !InfoPointer->Target)
		{
			if (!SourceObj || TargetDataFilter.FilterPassesForActor(SourceObj, HitActor, false))
			{
				if (!HitActor->GetClass()->IsChildOf(ABaseCharacter::StaticClass()))
				{
					//Character hitはPhysicsAssetのコリジョンボックスの使用を指定
					if (HitComponent == nullptr || !HitComponent->GetClass()->IsChildOf(USkeletalMeshComponent::StaticClass()))
					{
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
		}

		if (!InfoPointer)
		{
			continue;
		}

		//if (WeaknessEquipComponent)
		//{
		//	InfoPointer->WeaknessNames.Add(WeaknessEquipComponent->GetWeaknessName());
		//}

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
/// Attack from bullet infos
/// </summary>
const bool UCombatComponent::BulletTraceAttackToAbilitySystemComponent(const int32 WeaponID, class UWvAbilityEffectDataAsset* EffectDA, const int32 EffectGroupIndex, TArray<FHitResult>& Hits, const FVector SourceLocation)
{
	TArray<FWvBattleDamageAttackTargetInfo> HitTargetInfos;
	HitResultEnemyFilter(Hits, HitTargetInfos);

	if (HitTargetInfos.Num() == 0)
	{
		return false;
	}

	FWvBattleDamageAttackSourceInfo SourceInfo = FWvBattleDamageAttackSourceInfo();
	SourceInfo.SourceType = EWvBattleDamageAttackSourceType::Bullet;
	SourceInfo.WeaponID = WeaponID;

	if (!IsValid(EffectDA))
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid EffectDA => %s"), *FString(__FUNCTION__));
		return false;
	}

	AttackToASC(SourceInfo, HitTargetInfos, EffectDA, EffectGroupIndex, SourceLocation);
	return true;
}

#pragma endregion


