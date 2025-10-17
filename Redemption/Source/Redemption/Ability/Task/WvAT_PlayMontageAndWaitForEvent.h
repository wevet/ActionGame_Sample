// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ability/Task/WvAbilityTask.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/AnimMontage.h"
#include "WvAT_PlayMontageAndWaitForEvent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayMontageAndWaitForEventDelegate, FGameplayTag, EventTag, FGameplayEventData, EventData);

/**
 *
 */
UCLASS()
class REDEMPTION_API UWvAT_PlayMontageAndWaitForEvent : public UWvAbilityTask
{
	GENERATED_BODY()

public:
	UWvAT_PlayMontageAndWaitForEvent(const FObjectInitializer& ObjectInitializer);

	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual FString GetDebugString() const override;
	virtual void OnDestroy(bool AbilityEnded) override;

	/** The montage completely finished playing */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForEventDelegate OnCompleted;

	/** The montage started blending out */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForEventDelegate OnBlendOut;

	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForEventDelegate OnBlendIn;

	/** The montage was interrupted */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForEventDelegate OnInterrupted;

	/** The ability task was explicitly cancelled by another ability */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForEventDelegate OnCancelled;

	/** One of the triggering gameplay events happened */
	UPROPERTY(BlueprintAssignable)
	FPlayMontageAndWaitForEventDelegate EventReceived;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UWvAT_PlayMontageAndWaitForEvent* PlayMontageAndWaitForEvent(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		UAnimMontage* MontageToPlay,
		FGameplayTagContainer EventTags,
		float Rate = 1.0f,
		float StartTimeSeconds = 0.0f,
		FName StartSection = NAME_None,
		bool bStopWhenAbilityEnds = true,
		float AnimRootMotionTranslationScale = 1.0f,
		float StartingPosition = 0.0f);

public:
	/** Montage that is playing */
	UPROPERTY()
	TObjectPtr<class UAnimMontage> MontageToPlay;

	/** List of tags to match against gameplay events */
	UPROPERTY()
	FGameplayTagContainer EventTags{ FGameplayTagContainer::EmptyContainer };

	/** Playback rate */
	UPROPERTY()
	float Rate{ 1.0f };

	/** Section to start montage from */
	UPROPERTY()
	FName StartSection{ NAME_None };

	UPROPERTY()
	float StartTimeSeconds{ 0.f };

	/** Modifies how root motion movement to apply */
	UPROPERTY()
	float AnimRootMotionTranslationScale{ 1.0f };

	UPROPERTY()
	float StartingPosition{ 0.f };

	/** Rather montage should be aborted if ability ends */
	UPROPERTY()
	bool bStopWhenAbilityEnds{ true };

private:
	/** Checks if the ability is playing a montage and stops that montage, returns true if a montage was stopped, false if not. */
	bool StopPlayingMontage();

	/** Returns our ability system component */
	UWvAbilitySystemComponent* GetTargetAbilitySystemComponent();

	void OnMontageBlendedIn(UAnimMontage* Montage);
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	void OnAbilityCancelled();

	FOnMontageBlendingOutStarted BlendingOutDelegate;

	FOnMontageBlendedInEnded BlendedingInDelegate;

	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle CancelledHandle;
	FDelegateHandle EventHandle;
};
