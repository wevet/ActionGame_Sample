// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "WvFootstepAnimNotify.generated.h"

USTRUCT(BlueprintType)
struct FFootStepTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UNiagaraSystem* NiagaraSystems;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class USoundBase* FootStepSound;
};

/**
 *
 */
UCLASS()
class REDEMPTION_API UWvFootstepAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UDataTable* FootStepDT;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FName SocketName = FName(TEXT("foot_l"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TEnumAsByte<EPhysicalSurface> SurfaceTypeInEditor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float TraceBeginDistance = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float TraceEndDistance = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float Volume = 1.0f;

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	FName GetSurfaceName(TEnumAsByte<EPhysicalSurface> SurfaceType) const;

	void TraceFoot(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);
	void TraceFootDone(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum, USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation);
	void TriggerEffect(AActor* Owner, UAnimSequenceBase* Animation, const FVector Location, const TEnumAsByte<EPhysicalSurface> SurfaceType);

	const ELSGait GetGaitMode(AActor* Owner);
};
