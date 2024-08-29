// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "InputActionValue.h"
#include "PlayerCharacter.generated.h"

class UWvSpringArmComponent;
class UWvCameraFollowComponent;
class UHitTargetComponent;
class UQTEActionComponent;
class AWvPlayerController;

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
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

public:
	virtual bool IsTargetLock() const override;
	virtual void RegisterMission_Callback(const int32 MissionIndex) override;
	virtual bool IsQTEActionPlaying() const override;
	virtual void BuildOptimization() override;


#pragma region IWvAbilityTargetInterface
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage) override;
	virtual void Freeze() override;
	virtual void UnFreeze() override;
#pragma endregion

	bool IsInputKeyDisable() const;
	void SetKeyInputDisable();
	void SetKeyInputEnable();

	FVector GetFollowCameraLocation() const;

protected:
	virtual void OnFinisherAnimAssetLoadComplete() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvSpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UWvCameraFollowComponent> WvCameraFollowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UQTEActionComponent> QTEActionComponent;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* StrafeAction;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LockTargetAction;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Camera)
	float BaseLookUpRate;

protected:
	void Move(const FInputActionValue& Value);
	void MouseWheelAxis(const FInputActionValue& Value);

private:
	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);
	void Look(const FInputActionValue& Value);

	void HandleStanceMode();
	void HandleTargetLock();
	void HandleRotationMode();
	void HandleAimMode();

	void HandleJump(const bool bIsPress);
	void HandleSprinting(const bool bIsPress);
	void HandleMeleeAction(const bool bIsPress);
	void HandleDriveAction(const bool bIsPress);
	void HandleAliveAction(const bool bIsPress);
	void HandleHoldAimAction(const bool bIsPress);
	void HandleFinisherAction(const FGameplayTag Tag, const bool bIsPress);
	bool HasFinisherAction(const FGameplayTag Tag) const;

	void HandleQTEAction(const bool bIsPress);

	UFUNCTION()
	void GameplayTagTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnPluralInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnHoldingInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OnDoubleClickInputEventTrigger_Callback(const FGameplayTag Tag, const bool bIsPress);

	UFUNCTION()
	void OverlayStateChange_Callback(const ELSOverlayState PrevOverlay, const ELSOverlayState CurrentOverlay);

	UFUNCTION()
	void OnTargetLockedOn_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent);

	UFUNCTION()
	void OnTargetLockedOff_Callback(AActor* LookOnTarget, UHitTargetComponent* TargetComponent);

	UFUNCTION()
	void OnQTEBegin_Callback();

	UFUNCTION()
	void OnQTEEnd_Callback(const bool bIsSuccess);

	UPROPERTY()
	TWeakObjectPtr<AWvPlayerController> P_Controller;

public:
	FORCEINLINE class UWvSpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE class UQTEActionComponent* GetQTEActionComponent() const { return QTEActionComponent; }
};
