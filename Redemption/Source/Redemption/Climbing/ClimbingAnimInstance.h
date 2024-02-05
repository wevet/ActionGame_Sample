// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "ClimbingAnimInstance.generated.h"

class ACharacter;
class ULocomotionComponent;
//class UClimbingComponent;
//class ULadderComponent;
class UWvCharacterMovementComponent;

/**
 * 
 */
UCLASS()
class REDEMPTION_API UClimbingAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UClimbingAnimInstance(const FObjectInitializer& ObjectInitializer);
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaTimeX) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaTimeX) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
	TObjectPtr<class ACharacter> Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class ULocomotionComponent> LocomotionComponent;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	//class UClimbingComponent* ClimbingComponent;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	//class ULadderComponent* LadderComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UWvCharacterMovementComponent> CharacterMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climbing")
	FVector RootOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climbing")
	FName CMCSnapShotPose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climbing")
	FClimbingLedge CachedClimbingLedge;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climbing")
	EClimbActionType ClimbingActionType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climbing")
	FName CMCIdleCurveName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climbing")
	bool bIsLockUpdatingHangingMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Climbing")
	bool bIsFreeHang;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
	ELSMovementMode PrevMovementMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
	EClimbNextLedgeJumpType ClimbNextLedgeJumpType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
	EClimbMovementDirectionType MovementDirectionType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
	bool bIsClimbing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Wall")
	bool bIsWallClimbing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Wall")
	bool bIsWallClimbingJumping;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
	bool bIsStartMantling;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Laddering")
	bool bIsLaddering;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
	int32 PrepareAnimTypeIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
	bool bIsMoveToNextLedgeMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing")
	bool bIsQTEActivate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDebugTrace;

public:
	void NotifyJumpBackEvent();

	UFUNCTION(BlueprintCallable, Category = "ABP_Climbing")
	void NotifyEndMantling();

	UFUNCTION(BlueprintCallable, Category = "ABP_Climbing")
	void NotifyStartFreeHangMoveForward(const FClimbingLedge InClimbingLedge);

	UFUNCTION(BlueprintCallable, Category = "ABP_Climbing")
	void NotifyStartCornerOuterEvent(const FClimbingLedge InClimbingLedge);

	UFUNCTION(BlueprintCallable, Category = "ABP_Climbing")
	void NotifyFreeHangStateEvent(const bool bDetectedIK);

	UFUNCTION(BlueprintCallable, Category = "ABP_Climbing", meta = (BlueprintThreadSafe))
	void NotifyStartJumpEvent(const FClimbingJumpInfo JumpInfo);

	void NotifyQTEActivate(const bool bWasQTEActivate);


private:
	FTimerHandle WaitAxisTimer;

	const bool FreeHangStateEvent_Internal(const bool bDetectedIK);
	void SetCharacterReferences();
	void PrepareToForwardMove(const FClimbingLedge CachedLedgeLS);
};

