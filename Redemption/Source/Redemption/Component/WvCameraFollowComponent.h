// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Locomotion/LocomotionSystemTypes.h"
#include "Components/ActorComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Engine/HitResult.h"
#include "Async/TaskGraphInterfaces.h"
#include "AsyncComponentInterface.h"
#include "Engine/StreamableManager.h"
#include "Engine/DataAsset.h"
#include "Logging/LogMacros.h"
#include "WvCameraFollowComponent.generated.h"

class APlayerCharacter;
class ULocomotionComponent;
class UWvSpringArmComponent;
class APlayerController;
class UUserWidget;
class UWidgetComponent;
class UHitTargetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FComponentOnTargetLockedOnOff, AActor*, LookOnTarget, UHitTargetComponent*, LookOnTargetComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FComponentSetRotation, AActor*, LookOnTarget, FRotator, ControlRotation);


UCLASS(BlueprintType)
class REDEMPTION_API UCameraTargetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCameraSettingsTarget CameraSettings;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FCameraSettings BattleCameraSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationSensitiveValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UCurveFloat* DefaultCameraCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UCurveFloat* AimCamreaCurve;

	// The Widget Class to use when locked on Target. If not defined, will fallback to a Text-rendered
	// widget with a single O character.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Widget")
	TSubclassOf<UUserWidget> LockedOnWidgetClass;

	// Whether or not the Target LockOn Widget indicator should be drawn and attached automatically.
	// When set to false, this allow you to manually draw the widget for further control on where you'd like it to appear.
	// OnTargetLockedOn and OnTargetLockedOff events can be used for this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Widget")
	bool bShouldDrawLockedOnWidget = true;

	// The Widget Draw Size for the Widget class to use when locked on Target.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Widget")
	float LockedOnWidgetDrawSize = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Widget")
	FName LockedOnWidgetParentSocket = FName("spine_05");

	// The Relative Location to apply on Target LockedOn Widget when attached to a target.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Widget")
	FVector LockedOnWidgetRelativeLocation = FVector(0.0f, 0.0f, 0.0f);


public:
	UCameraTargetDataAsset() {}
};

USTRUCT()
struct FCameraLerpInfo
{
	GENERATED_BODY()

public:
	TObjectPtr<class UCurveFloat> LerpCurve{ nullptr };
	FCameraSettings CurCameraSettings;
	bool IsBattle{ false };
};

USTRUCT()
struct FTargetInfo
{
	GENERATED_BODY()
public:

	bool bIsSwitching{ false };
	bool bIsDesireToSwitch{ false };
	float TargetRotatingStack{ 0.f };
};


DECLARE_LOG_CATEGORY_EXTERN(LogWvCameraFollow, All, All)


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UWvCameraFollowComponent : public UActorComponent, public IAsyncComponentInterface
{
	GENERATED_BODY()

public:	
	UWvCameraFollowComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void RequestAsyncLoad() override;

protected:
	virtual void BeginPlay() override;


protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class UWvSpringArmComponent> SpringArmComponent;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> CameraComponent;


#pragma region TargetParameter
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System")
	TSoftObjectPtr<UCameraTargetDataAsset> CameraTargetDA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System")
	TEnumAsByte<ECollisionChannel> TargetableCollisionChannel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System")
	float MinimumDistanceToEnable = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System")
	float MaintainDistanceToKeep = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System")
	bool bIgnoreLookInput = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System")
	bool bDrawDebug = true;

	// The amount of time to break line of sight when actor gets behind an Object.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System")
	float BreakLineOfSightDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System")
	float YawDeltaThreshold{75.0f};

	// Setting this to true will tell the Target System to adjust the Pitch Offset (the Y axis) when locked on,
	// depending on the distance to the target actor.
	// It will ensure that the Camera will be moved up vertically the closer this Actor gets to its target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System|Pitch Offset")
	bool bAdjustPitchBasedOnDistanceToTarget = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System|Pitch Offset")
	float PitchDistanceCoefficient = -0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System|Pitch Offset")
	float PitchDistanceOffset = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System|Pitch Offset")
	float PitchMin = -50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Target System|Pitch Offset")
	float PitchMax = -20.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Switch Actor")
	bool bEnableStickyTarget = false;

	// Lower this value is, easier it will be to switch new target on right or left. Must be < 1.0f if controlling with gamepad stick
	// When using Sticky Feeling feature, it has no effect (see StickyRotationThreshold)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Switch Actor")
	float StartRotatingThreshold = 0.75f;

	// This value gets multiplied to the AxisValue to check against StickyRotationThreshold.
	// Only used when Sticky Target is enabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Switch Actor", meta = (EditCondition = "bEnableStickyTarget"))
	float AxisMultiplier = 1.0f;

	// Lower this value is, easier it will be to switch new target on right or left.
	// This is similar to StartRotatingThreshold, but you should set this to a much higher value.
	// Only used when Sticky Target is enabled.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Switch Actor", meta = (EditCondition = "bEnableStickyTarget"))
	float StickyRotationThreshold = 30.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Switch Component")
	bool bEnableStickyTargetComponent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Switch Component")
	float StartRotatingComponentThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Switch Component", meta = (EditCondition = "bEnableStickyTargetComponent"))
	float AxisComponentMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System|Switch Component", meta = (EditCondition = "bEnableStickyTargetComponent"))
	float StickyRotationComponentThreshold = 20.0f;
#pragma endregion


	void OnCameraChange();

public:
	/*
	* Function to call to target a new actor.
	*/
	void TargetLockOn();

	/*
	* Function to call to manually untarget.
	*/
	void TargetLockOff(bool bIsForce = true);

	/**
	* Function to call to switch with X-Axis mouse / controller stick movement.
	* @param AxisValue Pass in the float value of your Input Axis
	*/
	void TargetActorWithAxisInput(const float AxisValue, const float DeltaSeconds);

	/**
	* Function to call to switch with Y-Axis mouse / controller stick movement.
	* @param AxisValue Pass in the float value of your Input Axis
	*/
	void TargetComponentWithAxisInput(const float AxisValue, const float DeltaSeconds);

	/*
	* Returns the reference to currently targeted Actor if any
	*/
	AActor* GetLockedOnTargetActor() const;

	/*
	* Returns true / false whether the system is targeting an actor
	*/
	bool IsLocked() const;

	void DoTick();

	UPROPERTY(BlueprintAssignable, Category = "Target System")
	FComponentOnTargetLockedOnOff OnTargetLockedOff;

	UPROPERTY(BlueprintAssignable, Category = "Target System")
	FComponentOnTargetLockedOnOff OnTargetLockedOn;

	// Setup the control rotation on Tick when a target is locked on.
	// If not implemented, will fallback to default implementation.
	// If this event is implemented, it lets you control the rotation of the character.
	UPROPERTY(BlueprintAssignable, Category = "Target System")
	FComponentSetRotation OnTargetSetRotation;

	UFUNCTION(BlueprintCallable)
	void UpdateCamera(class UCurveFloat* LerpCurve);


private:
	UPROPERTY()
	TObjectPtr<class APlayerCharacter> Character;

	UPROPERTY()
	TObjectPtr<class ULocomotionComponent> LocomotionComponent;

	UPROPERTY()
	TObjectPtr<class APlayerController> PlayerController;

	TArray<TWeakObjectPtr<class UHitTargetComponent>> HitTargetComponents;

	TWeakObjectPtr<class AActor> LockOnTarget;
	TWeakObjectPtr<class UWidgetComponent> TargetLockUIComponent;
	TWeakObjectPtr<class UHitTargetComponent> SelectHitTargetComponent;


	FTimerHandle LineOfSightBreakTimerHandle;
	FTimerHandle SwitchingTargetTimerHandle;
	FTimerHandle SwitchingTargetComponentTimerHandle;
	FTimerHandle CameraLerpTimerHandle;

	FGraphEventRef AsyncWork;
	float CameraLerpTimerTotalTime{0.f};
	float CameraLerpTimerCurTime{ 0.f };
	float RotationSensitiveValue{ 0.f };

	bool bTargetLocked = false;
	bool bIsBreakingLineOfSight = false;
	int32 FocusIndex = 0;
	int32 FocusLastIndex = 0;

	UPROPERTY()
	FTargetInfo ActorInfo;

	UPROPERTY()
	FTargetInfo ComponentInfo;

	UPROPERTY()
	FCameraLerpInfo CameraLerpInfo;

	UPROPERTY()
	TObjectPtr<UCameraTargetDataAsset> CameraTargetDAInstance;

	TSharedPtr<FStreamableHandle>  ComponentStreamableHandle;

	

	void OnDataAssetLoadComplete();
	void OnLoadCameraTargetSettingsDA();

	UFUNCTION()
	void LerpUpdateCameraTimerCallback();
	const bool LerpCameraSettings(const float LerpAlpha);
	const bool ChooseCameraSettings(FCameraSettings& CameraSettings);

	UFUNCTION()
	void LocomotionMoveStateChangeCallback();

	UFUNCTION()
	void LocomotionAimChangeCallback();

	//~ Actors search / trace
	void DoTick(const float DeltaTime);
	TArray<AActor*> GetAllTargetableOfClass() const;
	TArray<AActor*> FindTargetsInRange(TArray<AActor*> ActorsToLook, float RangeMin, float RangeMax) const;
	AActor* FindNearestTarget(const TArray<AActor*>& Actors) const;
	AActor* FindNearestDistanceTarget(const TArray<AActor*>& Actors) const;
	bool LineTrace(FHitResult& OutHitResult, const AActor* OtherActor, const TArray<AActor*>& ActorsToIgnore) const;
	bool ShouldBreakLineOfSight() const;
	float GetDistanceFromCharacter(const AActor* OtherActor) const;
	void BreakLineOfSight();

	//~ Actor rotation
	FRotator GetControlRotationOnTarget(const AActor* OtherActor) const;
	FRotator FindLookAtRotation(const FVector Start, const FVector Target) const;
	void SetControlRotationOnTarget(AActor* TargetActor) const;
	void ControlRotation(const bool bStrafeMovement) const;
	float GetAngleUsingCameraRotation(const AActor* ActorToLook) const;
	float GetAngleUsingCharacterRotation(const AActor* ActorToLook) const;

	//~ Widget
	void CreateAndAttachTargetLockedOnWidgetComponent(AActor* TargetActor, UHitTargetComponent* TargetComponent);
	//~ Targeting

	TArray<UHitTargetComponent*> GetTargetHitComponents() const;
	UHitTargetComponent* GetLockTargetComponent() const;

	void TargetLockOn(AActor* TargetToLockOn, UHitTargetComponent* TargetComponent);
	void ResetIsSwitchingTarget();
	void ResetIsSwitchingTargetComponent();
	const bool ShouldSwitchTargetActor(const float AxisValue, const float DeltaSeconds);
	const bool ShouldSwitchTargetComponent(const float AxisValue, const float DeltaSeconds);
	static bool TargetIsTargetable(const AActor* Actor);


	void LineOfSightBreakHandler();

	void ModifyHitTargetComponents();

	static float YawDeltaSigned(const FRotator A, const FRotator B);

	AActor* PickByScreenSide(const TArray<AActor*>& Candidates, float AxisValue) const;

	void ReleaseTargetLockOnWidget();
};


USTRUCT()
struct FCameraTargetPostPhysicsTickFunction : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	class UWvCameraFollowComponent* Target;

	/**
	 * Abstract function actually execute the tick.
	 * @param DeltaTime - frame time to advance, in seconds
	 * @param TickType - kind of tick for this frame
	 * @param CurrentThread - thread we are executing on, useful to pass along as new tasks are created
	 * @param MyCompletionGraphEvent - completion event for this task. Useful for holding the completion of this task until certain child tasks are complete.
	 **/
	virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;

	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	virtual FString DiagnosticMessage() override;

	/** Function used to describe this tick for active tick reporting. **/
	virtual FName DiagnosticContext(bool bDetailed) override;
};

template<>
struct TStructOpsTypeTraits<FCameraTargetPostPhysicsTickFunction> : public TStructOpsTypeTraitsBase2<FCameraTargetPostPhysicsTickFunction>
{
	enum
	{
		WithCopy = false
	};
};


class FWvCameraTargetTask
{
	UWvCameraFollowComponent* TargetComponent;
public:
	FORCEINLINE FWvCameraTargetTask(UWvCameraFollowComponent* InComponent) : TargetComponent(InComponent) {}
	~FWvCameraTargetTask() {}

	static FORCEINLINE TStatId GetStatId()
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FWvCameraTargetTask, STATGROUP_TaskGraphTasks);
	}

	static FORCEINLINE ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyHiPriThreadNormalTask;
	}

	static FORCEINLINE ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::TrackSubsequents;
	}

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompleteGraphEvent)
	{
		TargetComponent->DoTick();
	}
};

