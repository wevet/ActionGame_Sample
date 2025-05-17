// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WvAbilitySystemTypes.h"
#include "WvFoleyEventAnimNotify.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UWvFoleyEventAnimNotify : public UAnimNotify
{
	GENERATED_BODY()
	
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName = FName(TEXT("foot_l"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag FooleyTag{ FGameplayTag::EmptyTag };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TEnumAsByte<EPhysicalSurface> SurfaceTypeInEditor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float TraceBeginDistance = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float TraceEndDistance = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float Volume = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bIsDisableTrace{false};


protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;


private:
	void TraceFootDone(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum, USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);
	void TriggerEffect(AActor* Owner, UAnimSequenceBase* Animation, const FHitResult& HitResult);

	const bool IsInCrouch(const AActor* Owner);
	bool HasOwnerAttachedTag() const;

	void CalculateVolume(AActor* Owner);
	void SpawnNoTraceSoundAndEffect(AActor* Owner, USkeletalMeshComponent* MeshComp);
	void SpawnSound(USoundBase* InSound, AActor* Owner);
};
