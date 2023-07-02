// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimationAsset.h"
#include "Character/BaseCharacter.h"
#include "GameplayEffectTypes.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Locomotion/LocomotionInterface.h"
#include "WvAnimInstance.generated.h"

class UAbilitySystemComponent;
class ULocomotionComponent;
class UWvCharacterMovementComponent;

USTRUCT()
struct REDEMPTION_API FBaseAnimInstanceProxy : public FAnimInstanceProxy
{

public:
	GENERATED_BODY()

	FBaseAnimInstanceProxy() {}

	FBaseAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance)
	{
	}
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
};


/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	UWvAnimInstance(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeBeginPlay() override;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

public:
	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);


protected:
	UPROPERTY()
	TWeakObjectPtr<class ABaseCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UWvCharacterMovementComponent* CharacterMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DistanceMatching")
	float GroundDistance = -1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "DistanceMatching")
	float GroundDistanceThreshold = 150.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	FTrajectorySampleRange TrajectorySampleRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FLocomotionEssencialVariables LocomotionEssencialVariables;

	// Blueprintの変数にマッピングできるGameplayTagです。Tagが追加または削除されると、変数が自動的に更新される。
	// GameplayTagを手動で照会する代わりに、これらを使用する必要があります。
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MotionMatching")
	FMotionMatchingSettings MotionMatchingSettings;


private:
	UPROPERTY()
	TWeakObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY()
	TWeakObjectPtr<class ULocomotionComponent> LocomotionComponent;

};


