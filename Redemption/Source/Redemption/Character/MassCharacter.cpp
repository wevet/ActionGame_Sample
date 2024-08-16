// Copyright 2022 wevet works All Rights Reserved.


#include "Character/MassCharacter.h"
#include "Redemption.h"
#include "Misc/WvCommonUtils.h"
#include "Locomotion/LocomotionComponent.h"


#include "Components/PawnNoiseEmitterComponent.h"
#include "Components/SceneComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassCharacter)

AMassCharacter::AMassCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

void AMassCharacter::BeginPlay()
{
	Super::BeginPlay();

	// @NOTE
	// disable ticking components
	// 1. UCharacterTrajectoryComponent
	// 2. ULocomotionComponent
	// 3. UPawnNoiseEmitterComponent
	// 4. USceneComponent() HeldObjectRoot
	CharacterTrajectoryComponent->SetComponentTickEnabled(false);
	//LocomotionComponent->SetComponentTickEnabled(false);
	PawnNoiseEmitterComponent->SetComponentTickEnabled(false);
	HeldObjectRoot->SetComponentTickEnabled(false);


	// disable climbing mantling
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidMantling, 1);

}

void AMassCharacter::DoStartCinematic()
{
	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		// Check for valid optimization parameters
		if (FAnimUpdateRateParameters* AnimUpdateRateParams = SkeletalMesh->AnimUpdateRateParams)
		{
			SkeletalMesh->bEnableUpdateRateOptimizations = false;
		}
	}

	Super::DoStartCinematic();
}

void AMassCharacter::DoStopCinematic()
{
	// Check for valid sk mesh
	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		// Check for valid optimization parameters
		if (FAnimUpdateRateParameters* AnimUpdateRateParams = SkeletalMesh->AnimUpdateRateParams)
		{
			// Enable URO for sk mesh
			SkeletalMesh->bEnableUpdateRateOptimizations = true;

			// Create threshold table for MaxDistanceFactor
			static const float ThresholdTable[] = { 0.5f, 0.5f, 0.3f, 0.1f, 0.1f, 0.1f };
			static const int32 TableNum = UE_ARRAY_COUNT(ThresholdTable);

			// Set threshold tables as optimization parameters for skeletal mesh
			TArray<float>& Thresholds = AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds;
			Thresholds.Empty(TableNum);
			for (int32 Index = 0; Index < TableNum; ++Index)
			{
				Thresholds.Add(ThresholdTable[Index]);
			}

			// copy rendering threshold
			AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Empty();
			for (const float Threshold : Thresholds)
			{
				AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Add(Threshold);
			}

			// Number of frame skips where interpolation is applied
			AnimUpdateRateParams->MaxEvalRateForInterpolation = 4;

			// Specify the number of off-screen skipped frames
			// Specify 6 to process 1 frame and skip 5 frames
			AnimUpdateRateParams->BaseNonRenderedUpdateRate = 6;
		}
	}

	Super::DoStopCinematic();
}


