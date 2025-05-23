// Copyright 2022 wevet works All Rights Reserved.

#include "Component/WvInputEventComponent.h"
#include "Character/WvPlayerController.h"
#include "GameFramework/InputSettings.h"
#include "Redemption.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvInputEventComponent)

UWvInputEventComponent::UWvInputEventComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	ClearCacheInput_TimerHandle.Invalidate();

	InputMode = EWvInputMode::GameOnly;
}

void UWvInputEventComponent::BeginPlay()
{
	Super::BeginPlay();
	bPermanentCacheInput = false;
	PlayerController = Cast<AWvPlayerController>(GetOuter());
}

void UWvInputEventComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.ClearTimer(ClearCacheInput_TimerHandle);

	PlayerCharacter.Reset();
	ASC.Reset();
	Super::EndPlay(EndPlayReason);
}

void UWvInputEventComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCachePluralInput();

	if (IsNeedForceRebuildKeymaps)
	{
		IsNeedForceRebuildKeymaps = false;
		InputSettingsCDO->ForceRebuildKeymaps();
	}
}

void UWvInputEventComponent::PostASCInitialize(UAbilitySystemComponent* NewASC)
{
	if (ASC.IsValid())
	{
		ASC.Reset();
	}

	ASC = Cast<UWvAbilitySystemComponent>(NewASC);

	if (RestrictInputEventTags.IsEmpty())
	{
		//RestrictInputEventTags = ABILITY_GLOBAL()->RestrictInputEventTags;
	}

	PlayerCharacter = Cast<APlayerCharacter>(ASC->GetAvatarActor());
}

void UWvInputEventComponent::BindInputEvent(UInputComponent* InInputComponent)
{
	InputComponent = InInputComponent;
	if (!InputComponent)
	{
		return;
	}

	InputSettingsCDO = GetMutableDefault<UInputSettings>();

	TArray<UDataTable*> InputTables;
	InputTables.Add(GeneralActionEventTable);
	InputTables.Add(FieldActionEventTable);
	InputTables.Add(BattleActionEventTable);
	InputTables += CustomActionEventTable;

	for (int32 Index = 0; Index < InputTables.Num(); Index++)
	{
		UDataTable* InputTable = InputTables[Index];
		if (!InputTable)
		{
			continue;
		}

		AllRegistTables.Add(InputTable);
		Name2RegistTableDict.Add(InputTable->GetName(), InputTable);

		TArray<FWvInputEvent*> InputEvents;
		InputTable->GetAllRows<FWvInputEvent>("FWvInputEvent", InputEvents);

		TArray<FName> InputEventNames = InputTable->GetRowNames();
		for (int32 JIndex = 0; JIndex < InputEvents.Num(); ++JIndex)
		{
			FWvInputEvent InputEvent = *InputEvents[JIndex];
			FName TotalKey = FName(InputTable->GetName() + ";" + InputEventNames[JIndex].ToString());
			BindTableInput(TotalKey, InputEvent);
			BindTablePluralInput(TotalKey, InputEvent);
		}
	}

	if (WaitDynamicRegistInputDict.Num() > 0)
	{
		for (TPair<FName, FWvInputEvent>Pair : WaitDynamicRegistInputDict)
		{
			DynamicRegistInputKey(Pair.Key, Pair.Value);
		}
		WaitDynamicRegistInputDict.Empty();
	}

	IsNeedForceRebuildKeymaps = true;
}

void UWvInputEventComponent::DynamicRegistInputKey(const FName Name, FWvInputEvent& InputEvent)
{
	if (!InputEvent.EventTag.IsValid())
	{
		return;
	}

	if (!InputComponent)
	{
		if (WaitDynamicRegistInputDict.Contains(Name))
		{
			WaitDynamicRegistInputDict[Name] = InputEvent;
		}
		else
		{
			WaitDynamicRegistInputDict.Add(Name, InputEvent);
		}
		return;
	}

	if (DynamicRegistInputDict.Contains(Name))
	{
		DynamicRegistInputDict[Name] = InputEvent;
	}
	else
	{
		DynamicRegistInputDict.Add(Name, InputEvent);
	}

	BindTableInput(Name, InputEvent);
	BindTablePluralInput(Name, InputEvent);
}

void UWvInputEventComponent::DynamicUnRegistInputKey(const FName Name)
{
	FWvInputEvent* InputEventPtr;
	InputEventPtr = WaitDynamicRegistInputDict.Find(Name);
	if (InputEventPtr)
	{
		WaitDynamicRegistInputDict.Remove(Name);
		return;
	}

	InputEventPtr = DynamicRegistInputDict.Find(Name);
	if (!InputEventPtr)
	{
		return;
	}

	DynamicRegistInputDict.Remove(Name);
	FWvInputEvent InputEvent = *InputEventPtr;
	TArray<FInputActionKeyMapping> ActionKeyMappings = InputEvent.GetActionKeyMappings();
	for (int32 Index = 0; Index < ActionKeyMappings.Num(); ++Index)
	{
		InputSettingsCDO->RemoveActionMapping(ActionKeyMappings[Index], false);
	}

	TArray<int32> BindIndexs = InputEvent.GetBindingIndexs();
	for (int32 Index = 0; Index < BindIndexs.Num(); ++Index)
	{
		InputComponent->RemoveActionBinding(BindIndexs[Index]);
	}

	IsNeedForceRebuildKeymaps = true;
}

void UWvInputEventComponent::BindTableInput(const FName Key, FWvInputEvent& InputEvent)
{
	if (!InputEvent.TriggerKey.IsValid() && !InputEvent.GamepadKey.IsValid())
	{
		return;
	}

	FName TagName = InputEvent.EventTag.GetTagName();
	if (InputEvent.GetIsUseExtend())
	{
		TagName = FName(InputEvent.GetEventTagNameWithExtend());
	}

	//register
	if (InputEvent.TriggerKey.IsValid())
	{
		FInputActionKeyMapping KeyboardMap = FInputActionKeyMapping(TagName, InputEvent.TriggerKey);
		InputSettingsCDO->AddActionMapping(KeyboardMap, false);
		InputEvent.AddInputActionKeyMapping(KeyboardMap);
	}
	if (InputEvent.GamepadKey.IsValid())
	{
		FInputActionKeyMapping GamepadMap = FInputActionKeyMapping(TagName, InputEvent.GamepadKey);
		InputSettingsCDO->AddActionMapping(GamepadMap, false);
		InputEvent.AddInputActionKeyMapping(GamepadMap);
	}

	//bind event
	{
		FInputActionBinding ActionBinding(TagName, IE_Pressed);
		FInputActionHandlerWithKeySignature ActionHandler;
		ActionHandler.BindLambda([this, Key](FKey inputKey) 
		{
			InputCallBack(inputKey, Key, true); 
		});
		ActionBinding.ActionDelegate = ActionHandler;
		ActionBinding.bConsumeInput = InputEvent.bConsumeInput;
		InputEvent.AddBindingIndex(InputComponent->AddActionBinding(ActionBinding).GetHandle());
	}
	{
		FInputActionBinding ActionBinding(TagName, IE_Released);
		FInputActionHandlerWithKeySignature ActionHandler;
		ActionHandler.BindLambda([this, Key](FKey inputKey) 
		{
			InputCallBack(inputKey, Key, false); 
		});
		ActionBinding.ActionDelegate = ActionHandler;
		ActionBinding.bConsumeInput = InputEvent.bConsumeInput;
		InputEvent.AddBindingIndex(InputComponent->AddActionBinding(ActionBinding).GetHandle());
	}

	IsNeedForceRebuildKeymaps = true;
}

void UWvInputEventComponent::BindTablePluralInput(const FName Key, FWvInputEvent& InputEvent)
{
	const int32 PluralInputEventsCount = InputEvent.PluralInputEvent.PrepositionInputKey.Num();
	FWvKey TriggerInputKey = InputEvent.PluralInputEvent.TriggerInputKey;

	if (PluralInputEventsCount == 0 && !TriggerInputKey.GamepadKey.IsValid() && !TriggerInputKey.TriggerKey.IsValid())
	{
		return;
	}

	UWvInputEventCallbackInfo* IECallbackInfo = NewObject<UWvInputEventCallbackInfo>();
	IECallbackInfo->PluralInputEventCount = PluralInputEventsCount;
	IECallbackInfo->CurInputEventCount = 0;
	IECallbackInfo->LastPressedTime = FDateTime::Now();
	IECallbackInfo->IsHoldAction = InputEvent.bHoldAction;
	IECallbackInfo->HoldTimer = InputEvent.Delay;
	InputEventCallbackInfoMap.Add(Key, IECallbackInfo);

	for (int32 Index = 0; Index < PluralInputEventsCount; ++Index)
	{
		FWvKey WvKey = InputEvent.PluralInputEvent.PrepositionInputKey[Index];
		AddRegisterInputKey(Key, WvKey);
	}
	AddRegisterInputKey(Key, TriggerInputKey);
}

void UWvInputEventComponent::AddRegisterInputKey(const FName Key, const FWvKey KeyInfo)
{
	if (KeyInfo.TriggerKey.IsValid())
	{
		TArray<FName>* NameList = RegisterInputKeyMap.Find(KeyInfo.TriggerKey);
		if (NameList == nullptr)
		{
			TArray<FName> KeyList = { Key };
			RegisterInputKeyMap.Add(KeyInfo.TriggerKey, KeyList);
		}
		else
		{
			NameList->Add(Key);
		}
	}

	if (KeyInfo.GamepadKey.IsValid())
	{
		TArray<FName>* NameList = RegisterInputKeyMap.Find(KeyInfo.GamepadKey);
		if (NameList == nullptr)
		{
			TArray<FName> KeyList = { Key };
			RegisterInputKeyMap.Add(KeyInfo.GamepadKey, KeyList);
		}
		else
		{
			NameList->Add(Key);
		}
	}
}

void UWvInputEventComponent::TriggerCacheInputEvent(UGameplayAbility* CallFromAbility)
{
	//only local has valid data
	if (!ASC.IsValid() || !PlayerController.Get())
	{
		return;
	}

	if (CacheInput == FGameplayTag::EmptyTag)
	{
		//UE_LOG(LogTemp, Warning, TEXT("empty cache input => %s"), *FString(__FUNCTION__));
		return;
	}

	const bool bHasStillPressing = InputKeyDownControl(CacheInput);
	ASC->PressTriggerInputEvent(CacheInput, true, bHasStillPressing);
	PlayerController->OnInputEventGameplayTagTrigger_Game.Broadcast(CacheInput, true);
	const UGameplayAbility* AnimatingAbility = ASC->GetAnimatingAbility();

	if (AnimatingAbility != CallFromAbility)
	{
		//comsumed, cache finished
		UE_LOG(LogTemp, Verbose, TEXT("comsumed, cache finished AnimatingAbility => %s, CallFromAbility => %s, [%s]"), *GetNameSafe(AnimatingAbility), *GetNameSafe(CallFromAbility) , *FString(__FUNCTION__));
		ResetCacheInput();
		ResetWaitTillEnd();
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("same AnimatingAbility => %s, [%s]"), *GetNameSafe(AnimatingAbility), *FString(__FUNCTION__));

		if (CallFromAbility)
		{
			ResetWaitTillEnd(CallFromAbility);
		}
	}


}

void UWvInputEventComponent::InputCallBack(const FKey InputKey, const FName Key, const bool bPress)
{
	//Time filtering is required here to comprehensively consider key combination, current input mode and other issues
	//After that, proceed to message broadcast. Cuttently, above issues are not considered here and message will be broadcasted directly

	if (!PlayerController.Get())
	{
		return;
	}

	if (PlayerCharacter.Get())
	{
		if (PlayerCharacter->IsInputKeyDisable())
		{
			return;
		}
	}

	FWvInputEvent* InputEvent = FindInputEvent(Key);
	if (!InputEvent)
	{
		return;
	}
	if (InputEvent->GamepadKey != InputKey && InputEvent->TriggerKey != InputKey)
	{
		return;
	}

	// RestrictInput
	if (bPress && RestrictInputEventTags.HasTag(InputEvent->EventTag))
	{
		//UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerCharacter.Get());
		//if (ASC && ASC->HasMatchingGameplayTag(GetGameplayTag_Global_Buff_RestrictInput()))
		//{
		//	FGameplayEventData Payload;
		//	ASC->HandleGameplayEvent(GetGameplayTag_Trigger_Passive_RestrictInput(), &Payload);
		//	return;
		//}
	}

	if (InputEvent->bCheckStateTag)
	{
		if (ASC.IsValid() && ASC->HasMatchingGameplayTag(TAG_Character_ActionMelee_ComboRequire))
		{
			//UE_LOG(LogTemp, Warning, TEXT("Enable ComboRequire Tag => %s"), *FString(__FUNCTION__));
			//return;
		}
	}

	if (InputEvent->GetIsUseExtend())
	{
		ProcessGameEventExtend(InputEvent->GetEventTagNameWithExtend(), bPress);
		return;
	}

	PlayerController->OnInputEventGameplayTagTrigger_All.Broadcast(InputEvent->EventTag, bPress);
	const EWvInputMode InputCache = InputMode;

	if (bPress)
	{
		switch (InputCache)
		{
			case EWvInputMode::GameOnly:
			ProcessGameEvent(InputEvent->EventTag, true);
			break;
			case EWvInputMode::UIOnly:
			ProcessGameEventUI(InputEvent->EventTag, true);
			break;
			case EWvInputMode::GameAndUI:
			ProcessGameEvent(InputEvent->EventTag, true);
			ProcessGameEventUI(InputEvent->EventTag, true);
			break;
		}

	}
	else
	{
		ProcessGameEvent(InputEvent->EventTag, false);
		ProcessGameEventUI(InputEvent->EventTag, false);
	}
}

void UWvInputEventComponent::SetInputModeType(const EWvInputMode NewInputMode)
{
	InputMode = NewInputMode;
}

EWvInputMode UWvInputEventComponent::GetInputModeType() const
{
	return InputMode;
}

void UWvInputEventComponent::InputKey(const FInputKeyParams& Params)
{
	EInputEvent InputEvent = Params.Event;
	FKey InputKey = Params.Key;

	TArray<FName>* NameList = RegisterInputKeyMap.Find(InputKey);
	if (NameList == nullptr)
	{
		return;
	}

	if (InputEvent == EInputEvent::IE_Pressed || InputEvent == EInputEvent::IE_Released)
	{
		const bool bPress = InputEvent == EInputEvent::IE_Pressed;
		TArray<FName> PluralInputKeyNames = *NameList;
		for (int32 Index = 0; Index < PluralInputKeyNames.Num(); ++Index)
		{
			const FName Key = PluralInputKeyNames[Index];
			PluralInputCallBack(InputKey, Key, bPress);
		}
	}
}

void UWvInputEventComponent::PluralInputCallBack(const FKey InputKey, const FName Key, const bool bPress)
{
	if (PlayerCharacter.Get())
	{
		if (PlayerCharacter->IsInputKeyDisable())
		{
			return;
		}
	}

	if (!InputKey.IsValid())
	{
		return;
	}

	FWvInputEvent* InputEvent = FindInputEvent(Key);
	if (!InputEvent)
	{
		return;
	}

	UWvInputEventCallbackInfo** InfoPtr = InputEventCallbackInfoMap.Find(Key);
	if (!InfoPtr)
	{
		return;
	}

	UWvInputEventCallbackInfo* IECallbackInfo = *InfoPtr;
	FWvPluralInputEventInfo PluralInputEvent = InputEvent->PluralInputEvent;

	do
	{
		int32 InputEventIndex = INDEX_NONE;
		for (int32 Index = 0; Index < PluralInputEvent.PrepositionInputKey.Num(); ++Index)
		{
			const FWvKey CurInputKey = PluralInputEvent.PrepositionInputKey[Index];
			if (CurInputKey.GamepadKey == InputKey || CurInputKey.TriggerKey == InputKey)
			{
				InputEventIndex = Index;
				break;
			}
		}

		if (InputEventIndex == INDEX_NONE)
		{
			break;
		}

		const int32 StepInputEventIndex = InputEventIndex + 1;
		if (bPress)
		{
			if (IECallbackInfo->CurInputEventCount == StepInputEventIndex - 1)
			{
				IECallbackInfo->CurInputEventCount = StepInputEventIndex;
			}
		}
		else
		{
			if (IECallbackInfo->CurInputEventCount == StepInputEventIndex)
			{
				IECallbackInfo->CurInputEventCount = StepInputEventIndex - 1;
			}
			else if (IECallbackInfo->CurInputEventCount > StepInputEventIndex)
			{
				IECallbackInfo->CurInputEventCount = 0;
			}
		}
	} while (false);


	if (IECallbackInfo->CurInputEventCount != IECallbackInfo->PluralInputEventCount)
	{
		return;
	}

	const FWvKey TriggerInputKey = PluralInputEvent.TriggerInputKey;
	if (TriggerInputKey.GamepadKey != InputKey && TriggerInputKey.TriggerKey != InputKey)
	{
		return;
	}

	const EWvInputEventType TriggerInputEventType = PluralInputEvent.TriggerInputEventType;
	bool bIsValid = false;

	//UE_LOG(LogTemp, Log, TEXT("[%s] : EventTag => %s"), *FString(__FUNCTION__), *InputEvent->EventTag.GetTagName().ToString());

	if (TriggerInputEventType == EWvInputEventType::Pressed)
	{
		if (bPress)
		{
			IECallbackInfo->LastPressedTime = FDateTime::Now();
			IECallbackInfo->EventTag = InputEvent->EventTag;
			IECallbackInfo->IsPress = PluralInputEvent.TriggerInputEventType == EWvInputEventType::Pressed ? true : false;
			CachePluralInputArray.AddUnique(IECallbackInfo);
			return;
		}
		else
		{
			CachePluralInputArray.Remove(IECallbackInfo);
		}
	}
	else if (TriggerInputEventType == EWvInputEventType::Released)
	{
		if (!bPress)
		{
			bIsValid = true;
		}
	}
	else if (TriggerInputEventType == EWvInputEventType::Clicked)
	{
		if (bPress)
		{
			IECallbackInfo->LastPressedTime = FDateTime::Now();
		}
		else
		{
			const int64 CurTick = FDateTime::Now().GetTicks();
			const int64 LastTick = IECallbackInfo->LastPressedTime.GetTicks();
			const int64 CLICK_INTERVAL = ETimespan::TicksPerSecond / 3;
			if (CurTick - LastTick < CLICK_INTERVAL)
			{
				bIsValid = true;
			}

			//UE_LOG(LogTemp, Log, TEXT("CurTick - LastTick => %d, CLICK_INTERVAL => %d"), CurTick - LastTick, CLICK_INTERVAL);
		}
	}
	else if (TriggerInputEventType == EWvInputEventType::HoldPressed)
	{
		if (bPress)
		{
			IECallbackInfo->OnPressed(GetWorld());
			IECallbackInfo->EventTag = InputEvent->EventTag;
			IECallbackInfo->IsPress = true;
			IECallbackInfo->OnHoldingCallback.AddDynamic(this, &ThisClass::OnHoldingInputCallBackExecute);
			CachePluralInputArray.AddUnique(IECallbackInfo);
			return;
		}
		else
		{
			bIsValid = true;
			IECallbackInfo->OnReleased();
			IECallbackInfo->OnHoldingCallback.RemoveDynamic(this, &ThisClass::OnHoldingInputCallBackExecute);
			CachePluralInputArray.Remove(IECallbackInfo);
		}
	}
	else if (TriggerInputEventType == EWvInputEventType::DoubleClicked)
	{
		if (bPress)
		{
			bIsValid = IECallbackInfo->OnDoubleClickPressed();
			IECallbackInfo->EventTag = InputEvent->EventTag;
			IECallbackInfo->IsPress = true;
			IECallbackInfo->OnDoubleClickCallback.AddDynamic(this, &ThisClass::OnDoubleClickCallBackExecute);
			CachePluralInputArray.AddUnique(IECallbackInfo);
		}
		else
		{
			IECallbackInfo->OnDoubleClickReleased();
			IECallbackInfo->OnHoldingCallback.RemoveDynamic(this, &ThisClass::OnDoubleClickCallBackExecute);
			CachePluralInputArray.Remove(IECallbackInfo);
		}
	}

	if (!bIsValid)
	{
		return;
	}

	if (InputEvent->GetIsUseExtend())
	{
		ProcessGameEventExtend(InputEvent->GetEventTagNameWithExtend(), bPress);
		return;
	}

	const bool bWasReleased = (PluralInputEvent.TriggerInputEventType == EWvInputEventType::Released) ? false : true;
	PluralInputCallBackExecute(InputEvent->EventTag, bWasReleased);
}

void UWvInputEventComponent::PluralInputCallBackExecute(FGameplayTag EventTag, bool bPress)
{
	if (!PlayerController.Get() || !ASC.Get())
	{
		return;
	}

	//ASC->PluralInputTriggerInputEvent(EventTag);
	ASC->TryActivateAbilityByTag(EventTag);
	PlayerController->OnPluralInputEventTrigger.Broadcast(EventTag, bPress);
}

void UWvInputEventComponent::OnHoldingInputCallBackExecute(FGameplayTag EventTag, bool bPress)
{
	if (!PlayerController.Get() || !ASC.Get())
	{
		return;
	}

	//ASC->PluralInputTriggerInputEvent(EventTag);
	ASC->TryActivateAbilityByTag(EventTag);
	PlayerController->OnHoldingInputEventTrigger.Broadcast(EventTag, bPress);
}

void UWvInputEventComponent::OnDoubleClickCallBackExecute(FGameplayTag EventTag, bool bPress)
{
	if (!PlayerController.Get() || !ASC.Get())
	{
		return;
	}

	ASC->TryActivateAbilityByTag(EventTag);
	PlayerController->OnDoubleClickInputEventTrigger.Broadcast(EventTag, bPress);
}

void UWvInputEventComponent::UpdateCachePluralInput()
{
	if (CachePluralInputArray.Num() <= 0)
	{
		return;
	}

	TSet<UWvInputEventCallbackInfo*> NeedToRemove;
	for (UWvInputEventCallbackInfo* Info : CachePluralInputArray)
	{
		const int64 CurTick = FDateTime::Now().GetTicks();
		const int64 LaskTick = Info->LastPressedTime.GetTicks();
		do
		{
			if (Info->CurInputEventCount != Info->PluralInputEventCount)
			{
				UE_LOG(LogTemp, Error, TEXT("Release Input Failed! => %s"), *FString(__FUNCTION__));
				NeedToRemove.Add(Info);
				break;
			}

			if (CurTick - LaskTick >= ETimespan::TicksPerSecond * InputDelayTime)
			{
				PluralInputCallBackExecute(Info->EventTag, Info->IsPress);
				NeedToRemove.Add(Info);
			}
		} while (false);
	}

	for (UWvInputEventCallbackInfo* Info : NeedToRemove)
	{
		CachePluralInputArray.Remove(Info);
	}
}

FWvInputEvent* UWvInputEventComponent::FindInputEvent(const FName Key)
{
	FWvInputEvent* DynamicInputKeyPtr = DynamicRegistInputDict.Find(Key);
	if (DynamicInputKeyPtr)
	{
		return DynamicInputKeyPtr;
	}

	FString TableName;
	FString EventRowName;
	Key.ToString().Split(";", &TableName, &EventRowName);

	FWvInputEvent* InputEvent = nullptr;

	UDataTable** InputTablePtr = Name2RegistTableDict.Find(TableName);
	if (!InputTablePtr)
	{
		return InputEvent;
	}

	UDataTable* InputTable = *InputTablePtr;
	InputEvent = InputTable->FindRow<FWvInputEvent>(FName(EventRowName), TEXT("UWvInputEventComponent::FindInputEvent"));
	return InputEvent;
}

void UWvInputEventComponent::ProcessGameEvent(const FGameplayTag& Tag, const bool bPress)
{
	if (!PlayerController.Get() || !ASC.IsValid())
	{
		return;
	}

	if (!bPress)
	{
		ASC->ReleasedTriggerInputEvent(Tag);
	}
	else
	{
		UGameplayAbility* CacheAnimatingAbility = ASC->GetAnimatingAbility();
		const int32 TriggerAbilityCount = ASC->PressTriggerInputEvent(Tag);
		FScopeLock ScopeLock(&CacheInputMutex);

		//UE_LOG(LogTemp, Log, TEXT("TriggerAbilityCount => %d"), TriggerAbilityCount);

		//Only duel with playing animation
		if (CacheAnimatingAbility)
		{
			// Input Event failed then cache, otherwise clear
			if (CacheAnimatingAbility == ASC->GetAnimatingAbility())
			{
				CacheInput = Tag;
				//UE_LOG(LogTemp, Log, TEXT("Modify CacheInput => %s"), *CacheInput.GetTagName().ToString());
				ResetWaitTillEnd(CacheAnimatingAbility);

				if (!bPermanentCacheInput)
				{
					if (GetWorld())
					{
						FTimerDelegate TimerDelegate;
						TimerDelegate.BindUFunction(this, FName(TEXT("ResetCacheInput_ClearTimer")));
						// 0.01666f
						const float Frame = (1.0f / 60.0f);
						GetWorld()->GetTimerManager().SetTimer(ClearCacheInput_TimerHandle, TimerDelegate, Frame, false);
					}
				}

				//UE_LOG(LogTemp, Log, TEXT("Input Event failed then cache, otherwise clear"));
			}
			else
			{
				UWvAbilityBase* CurrentAbility = Cast<UWvAbilityBase>(ASC->GetAnimatingAbility());
				UWvAbilityBase* LastAbility = Cast<UWvAbilityBase>(CacheAnimatingAbility);
				const bool bIsAnimatingCombo = ASC->IsAnimatingCombo();
				const bool bIsComboTagEqual = (CurrentAbility && LastAbility) ? CurrentAbility->GetComboRequiredTag() == LastAbility->GetComboRequiredTag() : false;
				const bool bIsSameCombo = CurrentAbility && LastAbility && bIsAnimatingCombo && bIsComboTagEqual;

				if (bIsSameCombo)
				{
					// keep cache input, exchange wait cur ability end instead
					ResetWaitTillEnd(CurrentAbility);
				}
				else
				{
					// Already switched to new combos/abilities and cleared
					ResetCacheInput();
					ResetWaitTillEnd();
				}
			}
		}
	}

	// Broadcast the tag status of input event, which button is currently pressed on HUD, need to listen to this message
	PlayerController->OnInputEventGameplayTagTrigger_Game.Broadcast(Tag, bPress);
}

void UWvInputEventComponent::ProcessGameEventUI(const FGameplayTag& Tag, const bool bPress)
{
	if (IsValid(PlayerController))
	{
		PlayerController->OnInputEventGameplayTagTrigger_UI.Broadcast(Tag, bPress);
	}
}

void UWvInputEventComponent::ProcessGameEventExtend(const FString EventName, const bool bPress)
{
	if (IsValid(PlayerController))
	{
		PlayerController->InputEventGameplayTagExtendDelegate_All.Broadcast(EventName, bPress);
	}
}

void UWvInputEventComponent::ResetWaitTillEnd(UGameplayAbility* WaitEndAbility)
{
	FScopeLock ScopeLock(&WaitMutex);

	if (WaitCacheInputResetHandle.IsValid() && ASC.IsValid())
	{
		ASC->OnAbilityEnded.Remove(WaitCacheInputResetHandle);
		WaitCacheInputResetHandle.Reset();
	}

	if (WaitEndAbility && ASC.IsValid())
	{
		auto WaitEndAbilityPtr = MakeWeakObjectPtr<UGameplayAbility>(WaitEndAbility);
		WaitCacheInputResetHandle = ASC->OnAbilityEnded.AddLambda([this, WaitEndAbilityPtr](const FAbilityEndedData& EndedData)
		{
			if (WaitEndAbilityPtr.IsValid())
			{
				WaitAbilityEnd(WaitEndAbilityPtr.Get(), EndedData);
			}
		});
	}
}

void UWvInputEventComponent::WaitAbilityEnd(UGameplayAbility* CallFromAbility, const FAbilityEndedData& EndedData)
{
	if (!ASC.IsValid())
	{
		return;
	}

	UWvAbilityBase* CurrentAbility = Cast<UWvAbilityBase>(ASC->GetAnimatingAbility());
	const UWvAbilityBase* LastAbility = Cast<UWvAbilityBase>(CallFromAbility);

	if (EndedData.AbilityThatEnded == CallFromAbility)
	{
		const bool bIsAbilitySwitching = CurrentAbility && LastAbility;
		const bool bIsSameCombo = bIsAbilitySwitching && ASC->IsAnimatingCombo() &&
			CurrentAbility->GetComboRequiredTag() == LastAbility->GetComboRequiredTag();

		if (bIsAbilitySwitching && !bIsSameCombo)
		{
			ResetWaitTillEnd();
		}
		else
		{
			if (!CurrentAbility)
			{
				ResetCacheInput();
			}
			ResetWaitTillEnd(CurrentAbility);
		}
	}

}

void UWvInputEventComponent::ResetCacheInput()
{
	if (CacheInput != FGameplayTag::EmptyTag)
	{
		FScopeLock ScopeLock(&CacheInputMutex);
		CacheInput = FGameplayTag::EmptyTag;
	}
}

void UWvInputEventComponent::ResetCacheInput_ClearTimer()
{
	ResetCacheInput();
	if (GetWorld())
	{
		if (ClearCacheInput_TimerHandle.IsValid() && GetWorld()->IsValidLowLevel())
		{
			GetWorld()->GetTimerManager().ClearTimer(ClearCacheInput_TimerHandle);
			ClearCacheInput_TimerHandle.Invalidate();
		}
	}
}

bool UWvInputEventComponent::InputKeyDownControl(const FGameplayTag Tag) const
{
	if (!PlayerController)
	{
		return false;
	}

	bool bIsFoundKey = false;

	for (int32 Index = 0; Index < AllRegistTables.Num(); Index++)
	{
		UDataTable* InputTable = AllRegistTables[Index];

		TArray<FWvInputEvent*> InputEvents;
		InputTable->GetAllRows<FWvInputEvent>("FWvInputEvent", InputEvents);
		for (int32 JIndex = 0; JIndex < InputEvents.Num(); ++JIndex)
		{
			FWvInputEvent InputEvent = *InputEvents[JIndex];
			if (InputEvent.EventTag == Tag)
			{
				bIsFoundKey = (PlayerController->IsInputKeyDown(InputEvent.GamepadKey) || PlayerController->IsInputKeyDown(InputEvent.TriggerKey));
				break;
			}
		}
	}

	return bIsFoundKey;
}

void UWvInputEventComponent::SetPermanentCacheInput(bool Enable)
{
	bPermanentCacheInput = Enable;
	if (!Enable)
	{
		ResetCacheInput();
	}
}

FGameplayTag UWvInputEventComponent::GetCacheInput() const
{
	return CacheInput;
}

