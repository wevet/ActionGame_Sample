// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAnimNotifyState.h"
#include "WvAbilityDataAsset.h"
#include "Ability/Task/WvAT_WaitKeyPress.h"
#include "WvAnimNotifyState_ComboEnable.generated.h"

class ABaseCharacter;

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
	float ExecuteTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer RequiredGameplayTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UWvAbilityDataAsset* NextAbilityDA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, UWvAbilityDataAsset*> OtherComboDA;

private:
	FGameplayTag GetInputCombo(const class UWvAbilityDataAsset* AbilityData) const;

	UFUNCTION()
	void OnPressed(const FGameplayTag InTag, const bool bIsPressed);
	void TryCombo();
	void PressedToCombo();

	FGameplayTag TriggerTag;
	UWvAT_WaitKeyPress* WaitReleaseTask;
	FGameplayTag LastPressedTag;
	float CurTime;
	bool IsImmediatelyExecute;

	UPROPERTY()
	TWeakObjectPtr<ABaseCharacter> Character;

};

