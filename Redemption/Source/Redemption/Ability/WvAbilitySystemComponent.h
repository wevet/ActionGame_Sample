// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvGameplayAbility.h"
#include "WvAbilityDataAsset.h"
#include "WvAbilitySystemComponentBase.h"
#include "WvAbilitySystemComponent.generated.h"

struct FAnimatingAbilityNotify
{
	TWeakObjectPtr<class UWvGameplayAbility> Ability;
	TWeakObjectPtr<class UWvAnimNotifyState> Notify;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAbilityMontageBeginDelegate, UGameplayAbility*, Ability, UAnimMontage*, Montage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAbilityTagUpdateDelegate, FGameplayTag, Tag, bool, TagExists);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FActorAbilityTagUpdateDelegate, AActor*, Actor, FGameplayTag, Tag, bool, TagExists);

class ABaseCharacter;
class AWvPlayerController;


UCLASS(ClassGroup = (Ability), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API UWvAbilitySystemComponent : public UWvAbilitySystemComponentBase
{
	GENERATED_BODY()
	
public:
	UWvAbilitySystemComponent();
	virtual void OnTagUpdated(const FGameplayTag& Tag, bool TagExists) override;

public:
	UPROPERTY(BlueprintAssignable, BlueprintReadWrite, EditAnywhere)
	FAbilityMontageBeginDelegate AbilityMontageBeginDelegate;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite, EditAnywhere)
	FAbilityTagUpdateDelegate AbilityTagUpdateDelegate;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite, EditAnywhere)
	FActorAbilityTagUpdateDelegate ActorAbilityTagUpdateDelegate;

	TArray<FAnimatingAbilityNotify> AnimatingAbilityNotifys;

	void AbilityNotifyBegin(class UWvAnimNotifyState* Notify, class UWvGameplayAbility* DebugAbility = nullptr);
	void AbilityNotifyEnd(class UWvAnimNotifyState* Notify);
	int32 GetDefaultAbilityLevel() const;
	class ABaseCharacter* GetAvatarCharacter() const;


	const bool TryActivateAbilityByClassPressing(TSubclassOf<UGameplayAbility> InAbilityToActivate, bool bAllowRemoteActivation);


};


