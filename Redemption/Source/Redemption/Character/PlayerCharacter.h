// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "InputActionValue.h"
#include "PlayerCharacter.generated.h"

class UWvSpringArmComponent;
class UWvCameraFollowComponent;
class UHitTargetComponent;

/**
 * 
 */
UCLASS()
class REDEMPTION_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	APlayerCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	bool IsInputKeyDisable() const;
	virtual bool IsTargetLock() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UWvSpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UWvCameraFollowComponent* WvCameraFollowComponent;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* StrafeAction;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Camera)
	float BaseLookUpRate;

protected:
	void Move(const FInputActionValue& Value);

private:
	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);
	void Look(const FInputActionValue& Value);
	void ToggleRotationMode(const FInputActionValue& Value);
	void ToggleAimMode(const FInputActionValue& Value);
	void ToggleStanceMode();
	void ToggleTargetLock();

	void HandleJump(const bool bIsPress);
	void HandleSprinting(const bool bIsPress);
	bool HasFinisherAction(const FGameplayTag Tag) const;

	UFUNCTION()
	void GameplayTagTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnPluralInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OverlayStateChange_Callback(const ELSOverlayState PrevOverlay, const ELSOverlayState CurrentOverlay);

	UFUNCTION()
	void OnTargetLockedOn_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent);

	UFUNCTION()
	void OnTargetLockedOff_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent);

public:
	FORCEINLINE class UWvSpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
