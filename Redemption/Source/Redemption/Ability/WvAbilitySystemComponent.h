// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "WvAbilitySystemComponent.generated.h"

struct FAnimatingAbilityNotify
{
	TWeakObjectPtr<class UWvGameplayAbility> Ability;
	TWeakObjectPtr<class UWvAnimNotifyState> Notify;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAbilityMontageBeginDelegate, UGameplayAbility*, Ability, UAnimMontage*, Montage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAbilityTagUpdateDelegate, FGameplayTag, Tag, bool, TagExists);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FActorAbilityTagUpdateDelegate, AActor*, Actor, FGameplayTag, Tag, bool, TagExists);

class USkeletalMeshComponent;
/**
 * 
 */
UCLASS(ClassGroup = (Ability), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API UWvAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	UWvAbilitySystemComponent();

public:
	UPROPERTY(BlueprintAssignable, BlueprintReadWrite, EditAnywhere)
	FAbilityMontageBeginDelegate AbilityMontageBeginDelegate;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite, EditAnywhere)
	FAbilityTagUpdateDelegate AbilityTagUpdateDelegate;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite, EditAnywhere)
	FActorAbilityTagUpdateDelegate ActorAbilityTagUpdateDelegate;

	TArray<FAnimatingAbilityNotify> AnimatingAbilityNotifys;

	void GetActiveAbilitiesWithTags(const FGameplayTagContainer& GameplayTagContainer, TArray<UGameplayAbility*>& ActiveAbilities);

	int32 GetDefaultAbilityLevel() const;

	static UWvAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent = false);

	bool HasActivatingAbilitiesWithTag(const FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void AddGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void RemoveGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SetGameplayTagCount(const FGameplayTag& GameplayTag, int32 Count = 1);

	void AbilityNotifyBegin(class UWvAnimNotifyState* Notify, class UWvGameplayAbility* DebugAbility = nullptr);
	void AbilityNotifyTick(class UWvAnimNotifyState* Notify, USkeletalMeshComponent* MeshComp, float FrameDeltaTime);
	void AbilityNotifyEnd(class UWvAnimNotifyState* Notify, USkeletalMeshComponent* MeshComp);
};

