// Copyright 2022 wevet works All Rights Reserved.


#include "WvAnimNotifyState_ComboEnable.h"
#include "Character/WvPlayerController.h"
#include "Character/WvAIController.h"
#include "Component/WvInputEventComponent.h"
#include "Component/InventoryComponent.h"
#include "Locomotion/LocomotionComponent.h"
#include "Item/WeaponBaseActor.h"
#include "Redemption.h"

#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
//#include "WvAbilitySystemTypes.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAnimNotifyState_ComboEnable)

/// <summary>
/// ComboRequreTag is not required for the last Combo
/// </summary>
UWvAnimNotifyState_ComboEnable::UWvAnimNotifyState_ComboEnable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 100, 200, 255);
#endif

	RequiredGameplayTags.AddTag(TAG_Character_ActionMelee_ComboRequire);
}

void UWvAnimNotifyState_ComboEnable::AbilityNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	const UWvAbilityDataAsset* AbilityData = Ability->GetWvAbilityDataChecked();
	if (!AbilityData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] acitivation failed, AbilityDataAsset Not Exist"), *FString(__FUNCTION__));
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTags(RequiredGameplayTags, 1);

	LastPressedTag = FGameplayTag::EmptyTag;
	CurTime = 0.f;
	TriggerTag = GetInputCombo(AbilityData);

	CombatInputData.Reset();

	TArray<FGameplayTag> TagArray;
	TagArray.Add(TriggerTag);
	TagArray.Add(TAG_Character_Player_Melee);

	Character = Cast<ABaseCharacter>(MeshComp->GetOwner());
	LocomotionComponent = MeshComp->GetOwner()->FindComponentByClass<ULocomotionComponent>();


	const FGameplayTagContainer EventTagContainer = FGameplayTagContainer::CreateFromArray(TagArray);
	IsImmediatelyExecute = (ExecuteTime <= 0.f);
	WaitReleaseTask = UWvAT_WaitKeyPress::WaitKeyPress(Ability, FName(TEXT("WaitKeyInput_ComboEnable")), EventTagContainer);

	WaitReleaseTask->OnActive.AddUniqueDynamic(this, &ThisClass::OnPressed);
	WaitReleaseTask->OnHoldingCallback.AddUniqueDynamic(this, &ThisClass::OnHolding);
	WaitReleaseTask->ReadyForActivation();


	if (AWvPlayerController* PC = Cast<AWvPlayerController>(AbilitySystemComponent->GetAvatarController()))
	{
		if (PC->IsLocalPlayerController())
		{
			UWvInputEventComponent* InputComponent = PC->GetInputEventComponent();
			if (IsValid(InputComponent))
			{
				InputComponent->TriggerCacheInputEvent(Ability);
			}
		}
	}

	if (AWvAIController* AIC = Cast<AWvAIController>(AbilitySystemComponent->GetAvatarController()))
	{
		AIC->NotifyCloseCombatBegin();
	}
}

void UWvAnimNotifyState_ComboEnable::AbilityNotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	CurTime += FrameDeltaTime;


	if (LocomotionComponent.IsValid())
	{
		auto& LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
		CombatInputData.SetBackwardInputResult(LocomotionEssencialVariables.bIsBackwardInputEnable);
	}

	if (!IsImmediatelyExecute && CurTime >= ExecuteTime)
	{
		TryCombo();
	}

	if (AWvAIController* AIC = Cast<AWvAIController>(AbilitySystemComponent->GetAvatarController()))
	{
		AIC->NotifyCloseCombatUpdate();
	}

}

void UWvAnimNotifyState_ComboEnable::AbilityNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (WaitReleaseTask)
	{
		WaitReleaseTask->OnActive.RemoveDynamic(this, &ThisClass::OnPressed);
		WaitReleaseTask->OnHoldingCallback.RemoveDynamic(this, &ThisClass::OnHolding);

		WaitReleaseTask->EndTask();
		WaitReleaseTask = nullptr;
	}

	if (AWvAIController* AIC = Cast<AWvAIController>(AbilitySystemComponent->GetAvatarController()))
	{
		AIC->NotifyCloseCombatEnd();
	}

	AbilitySystemComponent->RemoveLooseGameplayTags(RequiredGameplayTags, 1);

	Character.Reset();
	LocomotionComponent.Reset();

	if (Ability)
	{
		Ability->SetComboTriggerTag(FGameplayTag::EmptyTag);
	}
}

FGameplayTag UWvAnimNotifyState_ComboEnable::GetInputCombo(const class UWvAbilityDataAsset* AbilityData) const
{
	if (Ability->HasComboTrigger()) 
	{
		return Ability->GetComboTriggerTag();
	}
	return AbilityData->ActiveTriggerTag;
}

void UWvAnimNotifyState_ComboEnable::OnPressed(const FGameplayTag InTag, const bool bIsPressed)
{
	if (!bIsPressed)
	{
		return;
	}

	LastPressedTag = FGameplayTag(InTag);
	if (IsImmediatelyExecute)
	{
		PressedToCombo();
	}
}


void UWvAnimNotifyState_ComboEnable::OnHolding(const FGameplayTag InTag, const bool bIsPressed)
{
	if (!bIsPressed)
	{
		return;
	}

	LastPressedTag = FGameplayTag(InTag);
	CombatInputData.SetHoldResult(true);
	if (IsImmediatelyExecute)
	{
		PressedToCombo();
	}
}


void UWvAnimNotifyState_ComboEnable::TryCombo()
{
	if (LastPressedTag != FGameplayTag::EmptyTag)
	{
		PressedToCombo();
	}
	IsImmediatelyExecute = true;
}


void UWvAnimNotifyState_ComboEnable::PressedToCombo()
{
	// @TODO
	// migrate chooser
	auto SelectedDA = NextAbilityDA;
	if (IsValid(BackwardInputAbilityDA) && CombatInputData.bIsBackwardInputResult)
	{
		SelectedDA = BackwardInputAbilityDA;
		Character->CalculateBackwardInputRotation();
		UE_LOG(LogTemp, Warning, TEXT("backward attack result: [%s]"), *FString(__FUNCTION__));
	}

	const bool Result = AbilitySystemComponent->TryActivateAbilityByDataAsset(SelectedDA);
	const auto TagName = LastPressedTag.GetTagName().ToString();

	bool bIsFailed = false;
	if (Result)
	{
		UWvAbilityBase* CurrentAbility = AbilitySystemComponent->FindAbilityFromDataAsset(SelectedDA);
		if (CurrentAbility)
		{
			CurrentAbility->SetComboTriggerTag(LastPressedTag);
		}
		else
		{
			bIsFailed = true;
			UE_LOG(LogTemp, Warning, TEXT("nullptr CurrentAbility : [LastPressedTag => %s, function => %s]"), *TagName, *FString(__FUNCTION__));
		}
	}
	else
	{
		bIsFailed = true;
		UE_LOG(LogTemp, Error, TEXT("AbilityCall Failer : [LastPressedTag => %s, function => %s]"), *TagName, *FString(__FUNCTION__));
	}

	if (bIsFailed && Ability)
	{
		Ability->SetComboTriggerTag(FGameplayTag::EmptyTag);
	}
}


/// <summary>
/// for ai
/// </summary>
void UWvAnimNotifyState_ComboEnable::HandleAIRemoveDelegate()
{
	if (AWvAIController* AIC = Cast<AWvAIController>(AbilitySystemComponent->GetAvatarController()))
	{
		//if (AIC->OnInputEventGameplayTagTrigger.IsBound())
		//{
		//	AIC->OnInputEventGameplayTagTrigger.RemoveDynamic(this, &ThisClass::OnPressed);
		//}
	}
}


