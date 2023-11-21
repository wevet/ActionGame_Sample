// Copyright 2022 wevet works All Rights Reserved.

#include "Ability/Task/WvAT_ComponentDamage.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAT_ComponentDamage)

UWvAT_ComponentDamage::UWvAT_ComponentDamage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bTickingTask = true;
}

UWvAT_ComponentDamage* UWvAT_ComponentDamage::ComponentFrameAction(UGameplayAbility* OwningAbility, FName TaskName, float TotalDuration, float IntervalTime, FName BoneName, TArray<int32> EffectIdxs, TArray<int32> EffectCompIdxs, TArray<UShapeComponent*> Shapes)
{
	UWvAT_ComponentDamage* DamageTask = NewAbilityTask<UWvAT_ComponentDamage>(OwningAbility, TaskName);
	DamageTask->SourceName = BoneName;
	DamageTask->DurationTime = TotalDuration;
	DamageTask->IntervalTime = IntervalTime;
	DamageTask->EffectGroupIndexs = EffectIdxs;
	DamageTask->EffectCompIndexs = EffectCompIdxs;
	DamageTask->ShapeComponents = Shapes;
	return DamageTask;
}

bool UWvAT_ComponentDamage::GetCurTransForm(FTransform& OutBoneTransform, const FName BoneName)
{
	const int32 HitBoneIndex = SkelMeshComp->GetBoneIndex(BoneName);
	if (HitBoneIndex != INDEX_NONE)
	{
		OutBoneTransform = SkelMeshComp->GetBoneTransform(HitBoneIndex);
		return true;
	}
	else
	{
		USkeletalMeshSocket const* Socket = SkelMeshComp->GetSocketByName(BoneName);
		if (Socket)
		{
			OutBoneTransform = SkelMeshComp->GetSocketTransform(BoneName);
			return true;
		}
	}
	return false;
}

void UWvAT_ComponentDamage::Activate()
{
	if (ShapeComponents.Num() > 0)
	{
		NotifyTime = 0.0f;
		NextTraceTime = 0.0f;
		HitActors.Empty();
		ActorsToIgnore.Empty();
		ActorsToIgnore.Add(WvAbility->GetAvatarActorFromActorInfo());
		SkelMeshComp = WvAbility->GetOwningComponentFromActorInfo();

		CombatComponent = Cast<UCombatComponent>(WvAbility->GetAvatarActorFromActorInfo()->GetComponentByClass(UCombatComponent::StaticClass()));

		if (!GetCurTransForm(CurrentTransform, SourceName))
		{
			EndTask();
		}
	}
	else
	{
		EndTask();
	}
}

void UWvAT_ComponentDamage::TickTask(float DeltaTime)
{
	if (!WvAbility)
	{
		EndTask();
		return;
	}

	NotifyTime += DeltaTime;
	if (NotifyTime > DurationTime)
	{
		EndTask();
	}
	else if (NotifyTime > NextTraceTime)
	{
		Execute();
		NextTraceTime += IntervalTime;
	}
}


void UWvAT_ComponentDamage::Execute()
{
	FTransform BoneTransform(FTransform::Identity);

	if (!GetCurTransForm(BoneTransform, SourceName))
	{
		return;
	}

	for (int32 Index = 0; Index < EffectCompIndexs.Num(); ++Index)
	{
		const int32 EffectCompIndex = EffectCompIndexs[Index];
		if (!ShapeComponents.IsValidIndex(EffectCompIndexs[Index]))
		{
			continue;
		}

		UShapeComponent* ShapeComp = ShapeComponents[EffectCompIndexs[Index]];
		UCapsuleComponent* CapsuleComp = Cast<UCapsuleComponent>(ShapeComp);
		UBoxComponent* BoxComp = Cast<UBoxComponent>(ShapeComp);

		for (int32 JIndex = 0; JIndex < EffectGroupIndexs.Num(); ++JIndex)
		{
			const int32 EffectGroupIndex = EffectGroupIndexs[JIndex];
			if (CapsuleComp)
			{
				TempCapsuleExecute(BoneTransform, CapsuleComp, EffectGroupIndex, EffectCompIndex);
			}
			else if (BoxComp)
			{
				TempBoxExecute(BoneTransform, BoxComp, EffectGroupIndex, EffectCompIndex);
			}
		}
	}

	CurrentTransform = BoneTransform;
}

void UWvAT_ComponentDamage::TempCapsuleExecute(const FTransform BoneTransform, UCapsuleComponent* CapsuleComp, const int32 EffectGroupIndex, const int32 EffectCompIndex)
{
	if (!CombatComponent)
	{
		return;
	}

	const FVector StartPosition = CurrentTransform.TransformPositionNoScale(CapsuleComp->GetRelativeLocation());
	const FVector EndPosition = BoneTransform.TransformPositionNoScale(CapsuleComp->GetRelativeLocation());
	const FQuat CapsuleRotation = CapsuleComp->GetRelativeRotation().Quaternion();
	const FQuat BoneRotation = BoneTransform.GetRotation();
	const FQuat Rotation = BoneRotation * CapsuleRotation;
	const int32 CurEffectGroupIndex = EffectGroupIndexs.IsValidIndex(EffectGroupIndex) ? EffectGroupIndexs[EffectGroupIndex] : 0;
	CombatComponent->AbilityDamageCapsuleTrace(WvAbility, CurEffectGroupIndex, StartPosition, EndPosition, CapsuleComp->GetUnscaledCapsuleRadius(), CapsuleComp->GetUnscaledCapsuleHalfHeight(), Rotation, ActorsToIgnore);
}

void UWvAT_ComponentDamage::TempBoxExecute(const FTransform BoneTransform, UBoxComponent* BoxComp, const int32 EffectGroupIndex, const int32 EffectCompIndex)
{
	if (!CombatComponent)
	{
		return;
	}

	const FVector StartPosition = CurrentTransform.TransformPositionNoScale(BoxComp->GetRelativeLocation());
	const FVector EndPosition = BoneTransform.TransformPositionNoScale(BoxComp->GetRelativeLocation());
	const FQuat CapsuleRotation = BoxComp->GetRelativeRotation().Quaternion();
	const FQuat BoneRotation = BoneTransform.GetRotation();
	const FQuat Rotation = BoneRotation * CapsuleRotation;
	const int32 CurEffectGroupIndex = EffectGroupIndexs.IsValidIndex(EffectGroupIndex) ? EffectGroupIndexs[EffectGroupIndex] : 0;
	CombatComponent->AbilityDamageBoxTrace(WvAbility, CurEffectGroupIndex, StartPosition, EndPosition, BoxComp->GetUnscaledBoxExtent(), Rotation.Rotator(), ActorsToIgnore);
}


