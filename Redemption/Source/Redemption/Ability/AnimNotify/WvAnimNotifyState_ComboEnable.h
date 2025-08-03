// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAnimNotifyState.h"
#include "WvAbilityDataAsset.h"
#include "Ability/Task/WvAT_WaitKeyPress.h"
#include "WvAnimNotifyState_ComboEnable.generated.h"

class ABaseCharacter;
class ULocomotionComponent;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAnimNotifyState_ComboEnable : public UWvAnimNotifyState
{
	GENERATED_UCLASS_BODY()
	
protected:
	virtual void AbilityNotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void AbilityNotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void AbilityNotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	// Time required for combo (relative value)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ExecuteTime{ 0.1f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer RequiredGameplayTags;


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UWvAbilityDataAsset> NextAbilityDA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UWvAbilityDataAsset> BackwardInputAbilityDA;



private:
	FGameplayTag GetInputCombo(const class UWvAbilityDataAsset* AbilityData) const;

	UFUNCTION()
	void OnPressed(const FGameplayTag InTag, const bool bIsPressed);

	UFUNCTION()
	void OnHolding(const FGameplayTag InTag, const bool bIsPressed);

	void TryCombo();
	void PressedToCombo();

	FGameplayTag TriggerTag;
	FGameplayTag LastPressedTag;

	float CurTime{ 0.f };
	bool IsImmediatelyExecute{ false };

	TWeakObjectPtr<ABaseCharacter> Character;
	TWeakObjectPtr<ULocomotionComponent> LocomotionComponent;

	UPROPERTY()
	FCombatInputData CombatInputData;

	UPROPERTY()
	UWvAT_WaitKeyPress* WaitReleaseTask;


	void HandleAIRemoveDelegate();
};

