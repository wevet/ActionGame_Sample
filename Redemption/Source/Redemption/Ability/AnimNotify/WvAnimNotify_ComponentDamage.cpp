// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/AnimNotify/WvAnimNotify_ComponentDamage.h"
#include "Character/BaseCharacter.h"
#include "Component/CombatComponent.h"
#include "Component/InventoryComponent.h"
#include "Item/WeaponBaseActor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAnimNotify_ComponentDamage)

UWvAnimNotify_ComponentDamage::UWvAnimNotify_ComponentDamage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TaskName = TEXT("ComponentFrameFire");
	AttackIntervalTime = 0.01f;

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 200, 200, 255);
#endif
}

void UWvAnimNotify_ComponentDamage::AbilityNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	auto Owner = MeshComp->GetOwner();
	IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(Owner);
	if (!Interface)
	{
		return;
	}

	if (TraceActorClass)
	{
		const int32 HitBoneIndex = MeshComp->GetBoneIndex(TraceBoneName);
		USkeletalMeshSocket const* Socket = MeshComp->GetSocketByName(TraceBoneName);

		AWvTraceActor* TracableActor = TraceActorClass->GetDefaultObject<AWvTraceActor>();
		if (!TraceBoneName.IsNone() && (HitBoneIndex != INDEX_NONE || Socket != nullptr) && TracableActor->ShapeComponents.Num() > 0)
		{
			BoneFrameTask = UWvAT_ComponentDamage::ComponentFrameAction(Ability,
				TaskName,
				FLT_MAX,
				AttackIntervalTime,
				TraceBoneName,
				GameplayEffectGroupIndexs,
				TraceActorComponentIndexs,
				TracableActor->ShapeComponents
			);

			//UE_LOG(LogTemp, Log, TEXT("TracableActor => %s, Animation => %s"), TracableActor ? *TracableActor->GetName() : TEXT("nullptr"), Animation ? *Animation->GetName() : TEXT("nullptr"));
			BoneFrameTask->ReadyForActivation();
			NotifyWeapon_Fire(MeshComp);
		}

	}
}

void UWvAnimNotify_ComponentDamage::AbilityNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (BoneFrameTask)
	{
		BoneFrameTask->EndTask();
	}
}

void UWvAnimNotify_ComponentDamage::NotifyWeapon_Fire(USkeletalMeshComponent* MeshComp)
{
	ABaseCharacter* Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
	if (Character)
	{
		if (Character->GetInventoryComponent())
		{
			auto Weapon = Character->GetInventoryComponent()->GetEquipWeapon();
			if (Weapon)
			{
				Weapon->DoFire();
			}
		}
	}
}


