// Copyright 2022 wevet works All Rights Reserved.


#include "WvAnimNotifyState_ComboEnable.h"
#include "Character/WvPlayerController.h"
#include "Character/WvAIController.h"
#include "Component/WvInputEventComponent.h"
#include "Component/InventoryComponent.h"
#include "Item/WeaponBaseActor.h"
#include "Redemption.h"

#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"

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

	//if (Ability && Ability->HasComboTrigger())
	//{
	//	UE_LOG(LogTemp, Log, TEXT("playing combo melee Tag => %s, [%s]"), *Ability->GetComboTriggerTag().GetTagName().ToString(), *FString(__FUNCTION__));
	//}

	TArray<FGameplayTag> TagArray;
	OtherComboDA.GenerateKeyArray(TagArray);
	TagArray.Add(TriggerTag);
	TagArray.Add(TAG_Character_Player_Melee);


#if false
	auto Tags = RequiredGameplayTags.GetGameplayTagArray();
	for (FGameplayTag Tag : Tags)
	{
		UE_LOG(LogTemp, Log, TEXT("Added Tag => %s, func => %s"), *Tag.GetTagName().ToString(), *FString(__FUNCTION__));
	}
#endif


	const FGameplayTagContainer EventTagContainer = FGameplayTagContainer::CreateFromArray(TagArray);
	IsImmediatelyExecute = (ExecuteTime <= 0.f);
	WaitReleaseTask = UWvAT_WaitKeyPress::WaitKeyPress(Ability, FName(TEXT("WaitKeyInput_ComboEnable")), EventTagContainer);
	WaitReleaseTask->OnActive.__Internal_AddDynamic(this, &ThisClass::OnPressed, FName(TEXT("OnPressed")));
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
		WaitReleaseTask->EndTask();
		WaitReleaseTask = nullptr;
	}

	if (AWvAIController* AIC = Cast<AWvAIController>(AbilitySystemComponent->GetAvatarController()))
	{
		AIC->NotifyCloseCombatEnd();
	}

	AbilitySystemComponent->RemoveLooseGameplayTags(RequiredGameplayTags, 1);
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
	bool Result = false;
	UWvAbilityDataAsset* NextDA = nullptr;

	if (LastPressedTag == TriggerTag)
	{
		NextDA = NextAbilityDA;
		Result = AbilitySystemComponent->TryActivateAbilityByDataAsset(NextDA);
	}
	else
	{
		//const auto A = LastPressedTag.GetTagName().ToString();
		//const auto B = TriggerTag.GetTagName().ToString();
		//UE_LOG(LogTemp, Log, TEXT("diffrece Tag : [LastPressedTag => %s, TriggerTag => %s] function => [%s]"), *A, *B, *FString(__FUNCTION__));

		auto FindRef = OtherComboDA.FindRef(LastPressedTag);
		if (FindRef)
		{
			NextDA = FindRef;
			Result = AbilitySystemComponent->TryActivateAbilityByDataAsset(FindRef);
		}

	}

	const auto TagName = LastPressedTag.GetTagName().ToString();

	if (Result)
	{
		UWvAbilityBase* CurrentAbility = AbilitySystemComponent->FindAbilityFromDataAsset(NextDA);
		if (CurrentAbility)
		{
			CurrentAbility->SetComboTriggerTag(LastPressedTag);
			//UE_LOG(LogTemp, Log, TEXT("%s : [LastPressedTag => %s, function => %s]"), *GetNameSafe(CurrentAbility), *TagName, *FString(__FUNCTION__));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("nullptr CurrentAbility : [LastPressedTag => %s, function => %s]"), *TagName, *FString(__FUNCTION__));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AbilityCall Failer : [LastPressedTag => %s, function => %s]"), *TagName, *FString(__FUNCTION__));
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


