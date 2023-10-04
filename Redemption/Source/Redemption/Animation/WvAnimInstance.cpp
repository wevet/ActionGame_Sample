// Copyright 2022 wevet works All Rights Reserved.

#include "WvAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimMontage.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Animation/AnimClassInterface.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_LinkedAnimLayer.h"

#include "Component/WvCharacterMovementComponent.h"
#include "Locomotion/LocomotionComponent.h"



#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAnimInstance)

#pragma region Proxy
void FBaseAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	Super::Initialize(InAnimInstance);
}

bool FBaseAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	return Super::Evaluate(Output);
}
#pragma endregion


#pragma region Overlay
void FCharacterOverlayInfo::ChooseStanceMode(const bool bIsStanding)
{
	BasePose_N = bIsStanding ? 1.0f : 0.0f;
	BasePose_CLF = bIsStanding ? 0.0f : 1.0f;
}

void FCharacterOverlayInfo::ModifyAnimCurveValue(const UAnimInstance* AnimInstance)
{
	Spine_Add = AnimInstance->GetCurveValue(FName(TEXT("Layering_Spine_Add")));
	Head_Add = AnimInstance->GetCurveValue(FName(TEXT("Layering_Head_Add")));
	Arm_L_Add = AnimInstance->GetCurveValue(FName(TEXT("Layering_Arm_L_Add")));
	Arm_R_Add = AnimInstance->GetCurveValue(FName(TEXT("Layering_Arm_R_Add")));
	Hand_L = AnimInstance->GetCurveValue(FName(TEXT("Layering_Hand_L")));
	Hand_R = AnimInstance->GetCurveValue(FName(TEXT("Layering_Hand_R")));
	Arm_L_LS = AnimInstance->GetCurveValue(FName(TEXT("Layering_Arm_L_LS")));
	Arm_R_LS = AnimInstance->GetCurveValue(FName(TEXT("Layering_Arm_R_LS")));
	Arm_L_MS = FMath::Clamp((1.0f - Arm_L_LS), 0.0f, 1.0f);
	Arm_R_MS = FMath::Clamp((1.0f - Arm_R_LS), 0.0f, 1.0f);
}
#pragma endregion


UWvAnimInstance::UWvAnimInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bHasVelocity = false;

	Speed = 0.0f;
	GaitValue = 0.0f;
	WalkingSpeed = 0.0f;
	RunningSpeed = 0.0f;
	SprintingSpeed = 0.0f;

	LandPredictionAlpha = 0.0f;

	OverlayState = ELSOverlayState::None;
}

void UWvAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ABaseCharacter>(TryGetPawnOwner());

	if (!Character.IsValid())
	{
		return;
	}

	CharacterMovementComponent = CastChecked<UWvCharacterMovementComponent>(Character->GetCharacterMovement());
	LocomotionComponent = Character->GetLocomotionComponent();
	CapsuleComponent = Character->GetCapsuleComponent();

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character.Get()))
	{
		InitializeWithAbilitySystem(ASC);
	}
}

void UWvAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Character.IsValid())
	{
		TrajectorySampleRange = Character->GetTrajectorySampleRange();
	}

	if (LocomotionComponent.IsValid())
	{
		LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();

		CharacterOverlayInfo.ChooseStanceMode(LocomotionEssencialVariables.LSStance == ELSStance::Standing);
		CharacterOverlayInfo.ModifyAnimCurveValue(this);
		Speed = LocomotionEssencialVariables.Velocity.Size();
		bHasVelocity = LocomotionEssencialVariables.bWasMoving;
		OverlayState = LocomotionEssencialVariables.OverlayState;
		WalkingSpeed = LocomotionComponent->GetWalkingSpeed_Implementation();
		RunningSpeed = LocomotionComponent->GetRunningSpeed_Implementation();
		SprintingSpeed = LocomotionComponent->GetSprintingSpeed_Implementation();
	}

	switch (LocomotionEssencialVariables.LSMovementMode)
	{
		case ELSMovementMode::Grounded:
		DoWhileGrounded();
		break;
		case ELSMovementMode::Falling:
		DoWhileFalling();
		break;
	}

}

void UWvAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

}

void UWvAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
}

void UWvAnimInstance::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
}

void UWvAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
}

FAnimInstanceProxy* UWvAnimInstance::CreateAnimInstanceProxy()
{
	return new FBaseAnimInstanceProxy(this);
}

void UWvAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
	GameplayTagPropertyMap.Initialize(this, ASC);
}

#if WITH_EDITOR
EDataValidationResult UWvAnimInstance::IsDataValid(TArray<FText>& ValidationErrors)
{
	Super::IsDataValid(ValidationErrors);
	GameplayTagPropertyMap.IsDataValid(this, ValidationErrors);
	return ((ValidationErrors.Num() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif

void UWvAnimInstance::DoWhileGrounded()
{
	CalculateGaitValue();
}

void UWvAnimInstance::CalculateGaitValue()
{
	const float MoveSpeed = UKismetMathLibrary::MapRangeClamped(Speed, 0.0f, WalkingSpeed, 0.0f, 1.0f);
	const float WalkSpeed = UKismetMathLibrary::MapRangeClamped(Speed, WalkingSpeed, RunningSpeed, 1.0f, 2.0f);
	const float RunSpeed = UKismetMathLibrary::MapRangeClamped(Speed, RunningSpeed, SprintingSpeed, 2.0f, 3.0f);
	const bool bWalkedGreater = (Speed > WalkingSpeed);
	const bool bRunnedGreater = (Speed > RunningSpeed);

	const float CurrentSpeed = bRunnedGreater ? RunSpeed : bWalkedGreater ? WalkSpeed : MoveSpeed;
	GaitValue = CurrentSpeed;
}

void UWvAnimInstance::DoWhileFalling()
{
	//CalculateLandPredictionAlpha();

	if (IsValid(CharacterMovementComponent))
	{
		const FWvCharacterGroundInfo& GroundInfo = CharacterMovementComponent->GetGroundInfo();
		GroundDistance = GroundInfo.GroundDistance;
		LandPredictionAlpha = GroundInfo.LandPredictionAlpha;

		//UE_LOG(LogTemp, Log, TEXT("GroundDistance => %.3f, LandPredictionAlpha => %.3f"), GroundDistance, LandPredictionAlpha);
	}

}

void UWvAnimInstance::CalculateLandPredictionAlpha()
{
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	float InterpSpeed = 10.f;

	FVector Velocity = LocomotionEssencialVariables.Velocity;
	if (Velocity.Z > 0.0f)
	{
		LandPredictionAlpha = UKismetMathLibrary::FInterpTo(LandPredictionAlpha, 0.0f, DeltaSeconds, InterpSpeed);
		return;
	}

	if (!Character.IsValid() || !CharacterMovementComponent || !CapsuleComponent.IsValid())
	{
		LandPredictionAlpha = UKismetMathLibrary::FInterpTo(LandPredictionAlpha, 0.0f, DeltaSeconds, InterpSpeed);
		return;
	}

	const FVector Location = Character->GetActorLocation();
	const float Radius = CapsuleComponent->GetScaledCapsuleRadius();
	const float OffsetZ = (Location.Z - CapsuleComponent->GetScaledCapsuleHalfHeight_WithoutHemisphere());
	const FVector StartLocation = FVector(Location.X, Location.Y, OffsetZ);

	constexpr float ClampMin = -2000.f;
	constexpr float ClampMax = 1000.f;
	constexpr float DrawTime = 1.0f;

	FVector EndLocation = UKismetMathLibrary::Normal(FVector(Velocity.X, Velocity.Y, FMath::Clamp(Velocity.Z, ClampMin, -200.f)));
	EndLocation *= UKismetMathLibrary::MapRangeClamped(Velocity.Z, 0.0f, ClampMin, 50.f, ClampMax);
	EndLocation += StartLocation;
	FHitResult HitData(ForceInit);
	TArray<AActor*> IgnoreActors;

	UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		StartLocation,
		EndLocation,
		Radius,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		IgnoreActors,
		EDrawDebugTrace::Type::None,
		HitData,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		DrawTime);

	const bool bWasHitNormalGreater = (HitData.ImpactNormal.Z >= CharacterMovementComponent->GetWalkableFloorZ());
	if (HitData.bBlockingHit && bWasHitNormalGreater)
	{
		InterpSpeed = 20.f;
		const float Value = UKismetMathLibrary::MapRangeClamped(HitData.Time, 0.0f, 1.0f, 1.0f, 0.0f);
		const float CurveValue = LandAlphaCurve ? LandAlphaCurve->GetFloatValue(Value) : Value;
		LandPredictionAlpha = UKismetMathLibrary::FInterpTo(LandPredictionAlpha, CurveValue, DeltaSeconds, InterpSpeed);
	}
	else
	{
		LandPredictionAlpha = UKismetMathLibrary::FInterpTo(LandPredictionAlpha, 0.0f, DeltaSeconds, InterpSpeed);
	}

	//UE_LOG(LogTemp, Log, TEXT("LandPredictionAlpha => %.3f"), LandPredictionAlpha);
}


