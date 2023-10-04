// Copyright 2022 wevet works All Rights Reserved.


#include "WvAnimNotifyState_ComboEnable.h"
#include "Character/WvPlayerController.h"
#include "GameplayTagContainer.h"
#include "Component/WvInputEventComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Redemption.h"

UWvAnimNotifyState_ComboEnable::UWvAnimNotifyState_ComboEnable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 100, 200, 255);
#endif

	RequiredGameplayTags.AddTag(TAG_Character_ActionMelee_ComboRequire);
}

void UWvAnimNotifyState_ComboEnable::AbilityNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	UWvAbilityDataAsset* AbilityData = Ability->GetWvAbilityDataChecked();
	if (!AbilityData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ComboSkill] acitivation failed, AbilityDataAsset Not Exist"));
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTags(RequiredGameplayTags, 1);

	LastPressedTag = FGameplayTag::EmptyTag;
	CurTime = 0.f;

	TriggerTag = GetInputCombo(AbilityData);
	//UE_LOG(LogTemp, Log, TEXT("TriggerTag => %s, function => %s"), *TriggerTag.GetTagName().ToString(), *FString(__FUNCTION__));

	TArray<FGameplayTag> TagArray;
	OtherComboDA.GenerateKeyArray(TagArray);
	TagArray.Add(TriggerTag);
	FGameplayTagContainer EventTagContainer = FGameplayTagContainer::CreateFromArray(TagArray);

	IsImmediatelyExecute = (ExecuteTime <= 0.f);

	WaitReleaseTask = UWvAT_WaitKeyPress::WaitKeyPress(Ability, FName(TEXT("WaitKeyInput_ComboEnable")), EventTagContainer);
	WaitReleaseTask->OnActive.__Internal_AddDynamic(this, &UWvAnimNotifyState_ComboEnable::OnRelease, FName(TEXT("OnRelease")));
	WaitReleaseTask->ReadyForActivation();

	if (AWvPlayerController* Ctrl = Cast<AWvPlayerController>(AbilitySystemComponent->GetAvatarController()))
	{
		if (Ctrl->IsLocalPlayerController())
		{
			UWvInputEventComponent* InputComponent = Ctrl->GetInputEventComponent();
			if (IsValid(InputComponent))
			{
				InputComponent->TriggerCacheInputEvent(Ability);
			}
		}
	}
}

void UWvAnimNotifyState_ComboEnable::AbilityNotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	CurTime += FrameDeltaTime;
	if (!IsImmediatelyExecute && CurTime >= ExecuteTime)
	{
		TryCombo();
	}
}

void UWvAnimNotifyState_ComboEnable::AbilityNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (WaitReleaseTask)
	{
		WaitReleaseTask->EndTask();
		WaitReleaseTask = nullptr;
	}

	AbilitySystemComponent->RemoveLooseGameplayTags(RequiredGameplayTags, 1);
}

FGameplayTag UWvAnimNotifyState_ComboEnable::GetInputCombo(class UWvAbilityDataAsset* AbilityData)
{
	if (Ability->HasComboTrigger()) 
	{
		return Ability->GetComboTriggerTag();
	}
	return AbilityData->ActiveTriggerTag;
}

void UWvAnimNotifyState_ComboEnable::OnRelease(const FGameplayTag InTag, const bool bIsPressed)
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
	//pass lock.attack check
	//if (!AbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_StateMelee))
	//{ 
	//	AbilitySystemComponent->AddGameplayTag(TAG_Character_StateMelee, 1);
	//}

	bool Result = false;
	UWvAbilityDataAsset* NextDA = nullptr;

	if (LastPressedTag == TriggerTag)
	{
		NextDA = NextAbilityDA;
		Result = AbilitySystemComponent->TryActivateAbilityByDataAsset(NextDA);
	}
	else
	{
		auto FindRef = OtherComboDA.FindRef(LastPressedTag);
		if (FindRef)
		{
			NextDA = FindRef;
			if (NextDA)
			{
				Result = AbilitySystemComponent->TryActivateAbilityByDataAsset(NextDA);
			}
		}
	}

	//AbilitySystemComponent->RemoveGameplayTag(TAG_Character_StateMelee, 1);

	if (Result)
	{
		UWvAbilityBase* CurrentAbility = AbilitySystemComponent->FindAbilityFromDataAsset(NextDA);
		if (CurrentAbility)
		{
			CurrentAbility->SetComboTriggerTag(LastPressedTag);
			UE_LOG(LogTemp, Log, TEXT("CurrentAbility => %s, function => %s"), *CurrentAbility->GetName(), *FString(__FUNCTION__));
		}
	}
}

