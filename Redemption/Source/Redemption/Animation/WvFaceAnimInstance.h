// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WvFaceAnimInstance.generated.h"


class ABaseCharacter;
enum class EBodyShapeType : uint8;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvFaceAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
	
public:
	UWvFaceAnimInstance(const FObjectInitializer& ObjectInitializer);
	

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeBeginPlay() override;

protected:
	UPROPERTY()
	TWeakObjectPtr<class ABaseCharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
	EBodyShapeType BodyShapeType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Status")
	FName OverMorphTargetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Status")
	FName UnderMorphTargetName;
};
