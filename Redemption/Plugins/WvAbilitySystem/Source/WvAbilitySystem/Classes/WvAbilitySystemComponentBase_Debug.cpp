// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilitySystemComponentBase.h"
#include "WvAbilitySystemGlobals.h"
#include "WvAbilityDataAsset.h"
#include "Engine/Canvas.h"


void UWvAbilitySystemComponentBase::Debug_Internal(struct FAbilitySystemComponentDebugInfo& Info)
{
	// Draw title at top of screen (default HUD debug text starts at 50 ypos, we can position this on top)*
	//   *until someone changes it unknowingly
	{
		FString DebugTitle("");
		// Category
		if (Info.bShowAbilities)
		{
			DebugTitle += TEXT("ABILITIES ");
		}
		if (Info.bShowAttributes)
		{
			DebugTitle += TEXT("ATTRIBUTES ");
		}
		if (Info.bShowGameplayEffects)
		{
			DebugTitle += TEXT("GAMEPLAYEFFECTS ");
		}
		AActor* LocalAvatarActor = GetAvatarActor_Direct();
		AActor* LocalOwnerActor = GetOwnerActor();

		// Avatar info
		if (LocalAvatarActor)
		{
			const ENetRole AvatarRole = LocalAvatarActor->GetLocalRole();
			DebugTitle += FString::Printf(TEXT("for avatar %s "), *LocalAvatarActor->GetName());
			if (AvatarRole == ROLE_AutonomousProxy)
			{
				DebugTitle += TEXT("(local player) ");
			}
			else if (AvatarRole == ROLE_SimulatedProxy)
			{
				DebugTitle += TEXT("(simulated) ");
			}
			else if (AvatarRole == ROLE_Authority)
			{
				DebugTitle += TEXT("(authority) ");
			}
		}
		// Owner info
		if (LocalOwnerActor && LocalOwnerActor != LocalAvatarActor)
		{
			const ENetRole OwnerRole = LocalOwnerActor->GetLocalRole();
			DebugTitle += FString::Printf(TEXT("for owner %s "), *LocalOwnerActor->GetName());
			if (OwnerRole == ROLE_AutonomousProxy)
			{
				DebugTitle += TEXT("(autonomous) ");
			}
			else if (OwnerRole == ROLE_SimulatedProxy)
			{
				DebugTitle += TEXT("(simulated) ");
			}
			else if (OwnerRole == ROLE_Authority)
			{
				DebugTitle += TEXT("(authority) ");
			}
		}

		if (Info.Canvas)
		{
			Info.Canvas->SetDrawColor(FColor::White);
			FFontRenderInfo RenderInfo = FFontRenderInfo();
			RenderInfo.bEnableShadow = true;
			Info.Canvas->DrawText(GEngine->GetLargeFont(), DebugTitle, Info.XPos + 4.f, 10.f, 1.5f, 1.5f, RenderInfo);
		}
		else
		{
			DebugLine(Info, DebugTitle, 0.f, 0.f);
		}
	}

	FGameplayTagContainer OwnerTags;
	GetOwnedGameplayTags(OwnerTags);

	if (Info.Canvas)
	{
		Info.Canvas->SetDrawColor(FColor::White);
	}

	FString TagsStrings;
	int32 TagCount = 1;
	const int32 NumTags = OwnerTags.Num();
	for (FGameplayTag Tag : OwnerTags)
	{
		TagsStrings.Append(FString::Printf(TEXT("%s (%d)"), *Tag.ToString(), GetTagCount(Tag)));

		if (TagCount++ < NumTags)
		{
			TagsStrings += TEXT(", ");
		}
	}
	DebugLine(Info, FString::Printf(TEXT("Owned Tags: %s"), *TagsStrings), 4.f, 0.f, 4);

	if (BlockedAbilityTags.GetExplicitGameplayTags().Num() > 0)
	{
		FString BlockedTagsStrings;
		int32 BlockedTagCount = 1;
		for (FGameplayTag Tag : BlockedAbilityTags.GetExplicitGameplayTags())
		{
			BlockedTagsStrings.Append(FString::Printf(TEXT("%s (%d)"), *Tag.ToString(), BlockedAbilityTags.GetTagCount(Tag)));

			if (BlockedTagCount++ < NumTags)
			{
				BlockedTagsStrings += TEXT(", ");
			}
		}
		DebugLine(Info, FString::Printf(TEXT("BlockedAbilityTags: %s"), *BlockedTagsStrings), 4.f, 0.f);
	}
	else
	{
		DebugLine(Info, FString::Printf(TEXT("BlockedAbilityTags: ")), 4.f, 0.f);
	}

	TSet<FGameplayAttribute> DrawAttributes;

	float MaxCharHeight = 10;
	if (GetOwner()->GetNetMode() != NM_DedicatedServer)
	{
		MaxCharHeight = GEngine->GetTinyFont()->GetMaxCharHeight();
	}

	// -------------------------------------------------------------
	if (Info.bShowAttributes)
	{
#if false
		// Draw the attribute aggregator map.
		for (auto It = ActiveGameplayEffects.AttributeAggregatorMap.CreateConstIterator(); It; ++It)
		{
			FGameplayAttribute Attribute = It.Key();
			const FAggregatorRef& AggregatorRef = It.Value();
			if (AggregatorRef.Get())
			{
				FAggregator& Aggregator = *AggregatorRef.Get();

				FAggregatorEvaluateParameters EmptyParams;

				TMap<EGameplayModEvaluationChannel, const TArray<FAggregatorMod>*> ModMap;
				Aggregator.EvaluateQualificationForAllMods(EmptyParams);
				Aggregator.GetAllAggregatorMods(ModMap);

				if (ModMap.Num() == 0)
				{
					continue;
				}

				float FinalValue = GetNumericAttribute(Attribute);
				float BaseValue = Aggregator.GetBaseValue();

				FString AttributeString = FString::Printf(TEXT("%s %.2f "), *Attribute.GetName(), GetNumericAttribute(Attribute));
				if (FMath::Abs<float>(BaseValue - FinalValue) > SMALL_NUMBER)
				{
					AttributeString += FString::Printf(TEXT(" (Base: %.2f)"), BaseValue);
				}

				if (Info.Canvas)
				{
					Info.Canvas->SetDrawColor(FColor::White);
				}

				DebugLine(Info, AttributeString, 4.f, 0.f);

				DrawAttributes.Add(Attribute);

				for (const auto& CurMapElement : ModMap)
				{
					const EGameplayModEvaluationChannel Channel = CurMapElement.Key;
					const TArray<FAggregatorMod>* ModArrays = CurMapElement.Value;

					const FString ChannelNameString = UAbilitySystemGlobals::Get().GetGameplayModEvaluationChannelAlias(Channel).ToString();
					for (int32 ModOpIdx = 0; ModOpIdx < EGameplayModOp::Max; ++ModOpIdx)
					{
						const TArray<FAggregatorMod>& CurModArray = ModArrays[ModOpIdx];
						for (const FAggregatorMod& Mod : CurModArray)
						{
							bool IsActivelyModifyingAttribute = Mod.Qualifies();
							if (Info.Canvas)
							{
								Info.Canvas->SetDrawColor(IsActivelyModifyingAttribute ? FColor::Yellow : FColor(128, 128, 128));
							}

							FActiveGameplayEffect* ActiveGE = ActiveGameplayEffects.GetActiveGameplayEffect(Mod.ActiveHandle);
							FString SrcName = ActiveGE ? ActiveGE->Spec.Def->GetName() : FString(TEXT(""));

							if (IsActivelyModifyingAttribute == false)
							{
								if (Mod.SourceTagReqs)
								{
									SrcName += FString::Printf(TEXT(" SourceTags: [%s] "), *Mod.SourceTagReqs->ToString());
								}
								if (Mod.TargetTagReqs)
								{
									SrcName += FString::Printf(TEXT("TargetTags: [%s]"), *Mod.TargetTagReqs->ToString());
								}
							}

							DebugLine(Info, FString::Printf(TEXT("   %s %s\t %.2f - %s"), *ChannelNameString, *EGameplayModOpToString(ModOpIdx), Mod.EvaluatedMagnitude, *SrcName), 7.f, 0.f);
							Info.NewColumnYPadding = FMath::Max<float>(Info.NewColumnYPadding, Info.YPos + Info.YL);
						}
					}
				}

				AccumulateScreenPos(Info);
			}
		}
#endif
	}

	// -------------------------------------------------------------
	if (Info.bShowGameplayEffects)
	{
		for (FActiveGameplayEffect& ActiveGE : &ActiveGameplayEffects)
		{
			if (Info.Canvas)
			{
				Info.Canvas->SetDrawColor(FColor::White);
			}

			FString DurationStr = TEXT("Infinite Duration ");
			if (ActiveGE.GetDuration() > 0.f)
			{
				DurationStr = FString::Printf(TEXT("Duration: %.2f. Remaining: %.2f (Start: %.2f / %.2f / %.2f) %s "), ActiveGE.GetDuration(), ActiveGE.GetTimeRemaining(GetWorld()->GetTimeSeconds()), ActiveGE.StartServerWorldTime, ActiveGE.CachedStartServerWorldTime, ActiveGE.StartWorldTime, ActiveGE.DurationHandle.IsValid() ? TEXT("Valid Handle") : TEXT("INVALID Handle"));
				if (ActiveGE.DurationHandle.IsValid())
				{
					DurationStr += FString::Printf(TEXT("(Local Duration: %.2f)"), GetWorld()->GetTimerManager().GetTimerRemaining(ActiveGE.DurationHandle));
				}
			}
			if (ActiveGE.GetPeriod() > 0.f)
			{
				DurationStr += FString::Printf(TEXT("Period: %.2f"), ActiveGE.GetPeriod());
			}

			FString StackString;
			if (ActiveGE.Spec.GetStackCount() > 1)
			{

				if (ActiveGE.Spec.Def->StackingType == EGameplayEffectStackingType::AggregateBySource)
				{
					StackString = FString::Printf(TEXT("(Stacks: %d. From: %s) "), ActiveGE.Spec.GetStackCount(), *GetNameSafe(ActiveGE.Spec.GetContext().GetInstigatorAbilitySystemComponent()->GetAvatarActor_Direct()));
				}
				else
				{
					StackString = FString::Printf(TEXT("(Stacks: %d) "), ActiveGE.Spec.GetStackCount());
				}
			}

			FString LevelString;
			if (ActiveGE.Spec.GetLevel() > 1.f)
			{
				LevelString = FString::Printf(TEXT("Level: %.2f"), ActiveGE.Spec.GetLevel());
			}

			FString PredictionString;
			if (ActiveGE.PredictionKey.IsValidKey())
			{
				if (ActiveGE.PredictionKey.WasLocallyGenerated())
				{
					PredictionString = FString::Printf(TEXT("(Predicted and Waiting)"));
				}
				else
				{
					PredictionString = FString::Printf(TEXT("(Predicted and Caught Up)"));
				}
			}

			if (Info.Canvas)
			{
				Info.Canvas->SetDrawColor(ActiveGE.bIsInhibited ? FColor(128, 128, 128) : FColor::White);
			}

			DebugLine(Info, FString::Printf(TEXT("%s %s %s %s %s"), *CleanupName(GetNameSafe(ActiveGE.Spec.Def)), *DurationStr, *StackString, *LevelString, *PredictionString), 4.f, 0.f);

			FGameplayTagContainer GrantedTags;
			ActiveGE.Spec.GetAllGrantedTags(GrantedTags);
			if (GrantedTags.Num() > 0)
			{
				DebugLine(Info, FString::Printf(TEXT("Granted Tags: %s"), *GrantedTags.ToStringSimple()), 7.f, 0.f);
			}

			for (int32 ModIdx = 0; ModIdx < ActiveGE.Spec.Modifiers.Num(); ++ModIdx)
			{
				if (ActiveGE.Spec.Def == nullptr)
				{
					DebugLine(Info, FString::Printf(TEXT("null def! (Backwards compat?)")), 7.f, 0.f);
					continue;
				}

				const FModifierSpec& ModSpec = ActiveGE.Spec.Modifiers[ModIdx];
				const FGameplayModifierInfo& ModInfo = ActiveGE.Spec.Def->Modifiers[ModIdx];

				DebugLine(Info, FString::Printf(TEXT("Mod: %s. %s. %.2f"), *ModInfo.Attribute.GetName(), *EGameplayModOpToString(ModInfo.ModifierOp), ModSpec.GetEvaluatedMagnitude()), 7.f, 0.f);

				if (Info.Canvas)
				{
					Info.Canvas->SetDrawColor(ActiveGE.bIsInhibited ? FColor(128, 128, 128) : FColor::White);
				}
			}

			AccumulateScreenPos(Info);
		}
	}

	// -------------------------------------------------------------
	if (Info.bShowAttributes)
	{
		if (Info.Canvas)
		{
			Info.Canvas->SetDrawColor(FColor::White);
		}
		for (UAttributeSet* Set : GetSpawnedAttributes())
		{
			if (!Set)
			{
				continue;
			}

			for (TFieldIterator<FProperty> It(Set->GetClass()); It; ++It)
			{
				FGameplayAttribute	Attribute(*It);

				if (DrawAttributes.Contains(Attribute))
					continue;

				if (Attribute.IsValid())
				{
					float Value = GetNumericAttribute(Attribute);

					DebugLine(Info, FString::Printf(TEXT("%s %.2f"), *Attribute.GetName(), Value), 4.f, 0.f);
				}
			}
		}
		AccumulateScreenPos(Info);
	}

	// -------------------------------------------------------------
	bool bShowAbilityTaskDebugMessages = true;
	if (Info.bShowAbilities)
	{
		for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
		{
			//sakura: use instance instead cdo

			UWvAbilityBase* InstancedAbility = Cast<UWvAbilityBase>(AbilitySpec.GetPrimaryInstance());
			UWvAbilityDataAsset* AbilityData = Cast<UWvAbilityDataAsset>(AbilitySpec.SourceObject);
			
			if (!AbilityData || InstancedAbility == nullptr)
				continue;

			FString StatusText;
			FColor AbilityTextColor = FColor::Green;
			FGameplayTagContainer FailureTags;
			const TArray<uint8>& LocalBlockedAbilityBindings = GetBlockedAbilityBindings();

			if (AbilitySpec.IsActive())
			{
				StatusText = FString::Printf(TEXT(" (Active %d)"), AbilitySpec.ActiveCount);
				AbilityTextColor = FColor::Yellow;
			}
			else if (LocalBlockedAbilityBindings.IsValidIndex(AbilitySpec.InputID) && LocalBlockedAbilityBindings[AbilitySpec.InputID])
			{
				StatusText = TEXT(" (InputBlocked)");
				AbilityTextColor = FColor::Red;
			}
			else if (InstancedAbility->AbilityTags.HasAny(BlockedAbilityTags.GetExplicitGameplayTags()))
			{
				StatusText = TEXT(" (TagBlocked)");
				AbilityTextColor = FColor::Red;
			}
			else if (InstancedAbility->CanActivateAbility(AbilitySpec.Handle, AbilityActorInfo.Get(), nullptr, nullptr, &FailureTags) == false)
			{
				StatusText = FString::Printf(TEXT(" (CantActivate %s)"), *FailureTags.ToString());
				AbilityTextColor = FColor::Red;

				float Cooldown = InstancedAbility->GetCooldownTimeRemaining(AbilityActorInfo.Get());
				if (Cooldown > 0.f)
				{
					StatusText += FString::Printf(TEXT("   Cooldown: %.2f\n"), Cooldown);
				}
			}

			FString InputPressedStr = AbilitySpec.InputPressed ? TEXT("(InputPressed)") : TEXT("");
			FString ActivationModeStr = AbilitySpec.IsActive() ? UEnum::GetValueAsString(TEXT("GameplayAbilities.EGameplayAbilityActivationMode"), AbilitySpec.ActivationInfo.ActivationMode) : TEXT("");

			if (Info.Canvas)
			{
				Info.Canvas->SetDrawColor(AbilityTextColor);
			}

			DebugLine(Info, FString::Printf(TEXT("%s %s %s %s"), *(AbilityData->AbilityDes.ToString()), *StatusText, *InputPressedStr, *ActivationModeStr), 4.f, 0.f);

			if (AbilitySpec.IsActive())
			{
				if (Info.Canvas)
				{
					Info.Canvas->SetDrawColor(FColor::White);
				}
				for (UGameplayTask* Task : InstancedAbility->ActiveTasks)
				{
					if (Task)
					{
						DebugLine(Info, FString::Printf(TEXT("%s"), *Task->GetDebugString()), 7.f, 0.f);

						if (bShowAbilityTaskDebugMessages)
						{
							for (FAbilityTaskDebugMessage& Msg : InstancedAbility->TaskDebugMessages)
							{
								if (Msg.FromTask == Task)
								{
									DebugLine(Info, FString::Printf(TEXT("%s"), *Msg.Message), 9.f, 0.f);
								}
							}
						}
					}
				}

				bool FirstTaskMsg = true;
				int32 MsgCount = 0;
				for (FAbilityTaskDebugMessage& Msg : InstancedAbility->TaskDebugMessages)
				{
					// Cap finished task msgs to 5 per ability if we are printing to screen (else things will scroll off)
					if (Info.Canvas && ++MsgCount > 5)
					{
						break;
					}

					if (InstancedAbility->ActiveTasks.Contains(Msg.FromTask) == false)
					{
						if (FirstTaskMsg)
						{
							DebugLine(Info, TEXT("[FinishedTasks]"), 7.f, 0.f);
							FirstTaskMsg = false;
						}

						DebugLine(Info, FString::Printf(TEXT("%s"), *Msg.Message), 9.f, 0.f);
					}
				}

				if (Info.Canvas)
				{
					Info.Canvas->SetDrawColor(FColor(128, 128, 128));
				}
				DebugLine(Info, FString::Printf(TEXT("--------")), 7.f, 0.f);
			}
		}
		AccumulateScreenPos(Info);
	}

	// -------------------------------------------------------------
	if (Info.XPos > Info.OriginalX)
	{
		// We flooded to new columns, returned YPos should be max Y (and some padding)
		Info.YPos = Info.MaxY + MaxCharHeight * 2.f;
	}
	Info.YL = MaxCharHeight;
}
