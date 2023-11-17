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
#include "Animation/BlendSpace.h"

#include "Component/WvCharacterMovementComponent.h"
#include "Locomotion/LocomotionComponent.h"
#include "Character/WvPlayerController.h"



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
	LSMovementMode = ELSMovementMode::None;

	bIsClimbing = false;
	//bIsFreeHang = false;
	//bIsLaddering = false;
	//bIsQTEActivate = false;
	//bIsStartMantling = false;
	//bIsMoveToNextLedgeMode = false;
	//bIsLockUpdatingHangingMode = false;

	bIsWallClimbing = false;
	bIsWallClimbingJumping = false;
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

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const IConsoleVariable* RelevantCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("wv.LocomotionSystem.Debug"));
	const int32 RelevantConsoleValue = RelevantCVar->GetInt();
	if (RelevantConsoleValue > 0)
	{
		if (Character.IsValid())
		{
			bOwnerPlayerController = bool(Cast<APlayerController>(Character->GetController()));
			DrawRelevantAnimation();
		}
	}
#endif
}

void UWvAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (Character.IsValid())
	{
		TrajectorySampleRange = Character->GetTrajectorySampleRange();
	}

	if (IsValid(CharacterMovementComponent))
	{
		bIsWallClimbing = CharacterMovementComponent->IsWallClimbing();
		bIsWallClimbingJumping = CharacterMovementComponent->IsClimbJumping();
	}

	if (LocomotionComponent.IsValid())
	{
		LocomotionEssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
		CharacterOverlayInfo.ChooseStanceMode(LocomotionEssencialVariables.LSStance == ELSStance::Standing);
		CharacterOverlayInfo.ModifyAnimCurveValue(this);
		Speed = LocomotionEssencialVariables.Velocity.Size();
		bHasVelocity = LocomotionEssencialVariables.bWasMoving;
		OverlayState = LocomotionEssencialVariables.OverlayState;

		LastVelocityRotation = LocomotionEssencialVariables.LastVelocityRotation;
		Direction = LocomotionEssencialVariables.Direction;

		CharacterRotation = LocomotionEssencialVariables.CharacterRotation;
		LookingRotation = LocomotionEssencialVariables.LookingRotation;
		bWasAiming = LocomotionEssencialVariables.bAiming;
		LSRotationMode = LocomotionEssencialVariables.LSRotationMode;
		LSGait = LocomotionEssencialVariables.LSGait;
		LSStance = LocomotionEssencialVariables.LSStance;
		const ELSMovementMode CurMovementMode = LocomotionEssencialVariables.LSMovementMode;
		if (LSMovementMode != CurMovementMode)
		{
			PrevLSMovementMode = LSMovementMode;
			LSMovementMode = CurMovementMode;
		}

		WalkingSpeed = LocomotionComponent->GetWalkingSpeed_Implementation();
		RunningSpeed = LocomotionComponent->GetRunningSpeed_Implementation();
		SprintingSpeed = LocomotionComponent->GetSprintingSpeed_Implementation();
	}

	switch (LSMovementMode)
	{
		case ELSMovementMode::Grounded:
		DoWhileGrounded();
		break;
		case ELSMovementMode::Falling:
		DoWhileFalling();
		break;
	}

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

#pragma region Grounded
void UWvAnimInstance::DoWhileGrounded()
{
	CalculateGaitValue();
	CalculateAimOffset();
	CalculateGroundedLeaningValues();
}

void UWvAnimInstance::CalculateGaitValue()
{
	const float WalkSpeed = UKismetMathLibrary::MapRangeClamped(Speed, 0.0f, WalkingSpeed, 0.0f, 1.0f);
	const float RunSpeed = UKismetMathLibrary::MapRangeClamped(Speed, WalkingSpeed, RunningSpeed, 1.0f, 2.0f);
	const float SprintSpeed = UKismetMathLibrary::MapRangeClamped(Speed, RunningSpeed, SprintingSpeed, 2.0f, 3.0f);
	const bool bWalkedGreater = (Speed > WalkingSpeed);
	const bool bRunnedGreater = (Speed > RunningSpeed);
	const float CurrentSpeed = bRunnedGreater ? SprintSpeed : bWalkedGreater ? RunSpeed : WalkSpeed;
	GaitValue = CurrentSpeed;
}

void UWvAnimInstance::CalculateAimOffset()
{
	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	constexpr float DefaultInterpSpeed = 4.0f;

	switch (LSRotationMode)
	{
		case ELSRotationMode::VelocityDirection:
		{
			AimOffset = UKismetMathLibrary::Vector2DInterpTo(AimOffset, FVector2D::ZeroVector, DeltaSeconds, DefaultInterpSpeed);
		}
		break;
		case ELSRotationMode::LookingDirection:
		{
			const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LookingRotation, CharacterRotation);
			FVector2D NewAimOffset = FVector2D(DeltaRot.Yaw, DeltaRot.Pitch);
			NewAimOffset.X = FMath::Clamp(NewAimOffset.X, -AimOffsetClampRange.X, AimOffsetClampRange.X);
			NewAimOffset.Y = FMath::Clamp(NewAimOffset.Y, -AimOffsetClampRange.Y, AimOffsetClampRange.Y);
			AimOffset = NewAimOffset;
		}
		break;
	}

	AimSweepTime = UKismetMathLibrary::MapRangeClamped(AimOffset.Y, -90.0f, 90.f, 1.0f, 0.0f);
}

void UWvAnimInstance::CalculateGroundedLeaningValues()
{
	if (!IsValid(CharacterMovementComponent))
	{
		return;
	}

	check(GetWorld());

	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	const float ClampedSpeed = UKismetMathLibrary::MapRangeClamped(Speed, WalkingSpeed, RunningSpeed, 0.0f, 1.0f);
	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LastVelocityRotation, PreviousVelocityRotation);
	DeltaVelocityDifference = (DeltaRot.Yaw / DeltaSeconds);
	PreviousVelocityRotation = LastVelocityRotation;
	const float DeltaVelocityClampValue = UKismetMathLibrary::MapRangeClamped(DeltaVelocityDifference, -200.0f, 200.0f, -1.0f, 1.0f);
	const float LeanRotation = (DeltaVelocityClampValue * ClampedSpeed);

	AccelerationDifference = (Speed - PreviousSpeed) / DeltaSeconds;
	PreviousSpeed = Speed;
	const float MaxAcceleration = CharacterMovementComponent->GetMaxAcceleration();
	const float BrakingDecelerationWalking = CharacterMovementComponent->GetMaxBrakingDeceleration();
	const float MaxAccelerationClamp = UKismetMathLibrary::MapRangeClamped(FMath::Abs(AccelerationDifference), 0.0f, MaxAcceleration, 0.0f, 1.0f);
	const float BrakingDecelerationClamp = UKismetMathLibrary::MapRangeClamped(FMath::Abs(AccelerationDifference), 0.0f, BrakingDecelerationWalking, 0.0f, -1.0f);
	const float LeanAcceleration = ClampedSpeed * UKismetMathLibrary::SelectFloat(MaxAccelerationClamp, BrakingDecelerationClamp, (AccelerationDifference > 0.0f));

	const FVector LeanPosition = FVector(LeanRotation, LeanAcceleration, 0.0f);
	const FVector AngleAxis = UKismetMathLibrary::RotateAngleAxis(LeanPosition, Direction, FVector(0.0f, 0.0f, -1.0f));
	LeanGrounded.X = AngleAxis.X;
	LeanGrounded.Y = AngleAxis.Y;
}
#pragma endregion


#pragma region Falling
void UWvAnimInstance::DoWhileFalling()
{
	if (IsValid(CharacterMovementComponent))
	{
		const FWvCharacterGroundInfo& GroundInfo = CharacterMovementComponent->GetGroundInfo();
		GroundDistance = GroundInfo.GroundDistance;
		LandPredictionAlpha = GroundInfo.LandPredictionAlpha;

		//UE_LOG(LogTemp, Log, TEXT("GroundDistance => %.3f, LandPredictionAlpha => %.3f"), GroundDistance, LandPredictionAlpha);
	}
}
#pragma endregion


#pragma region Utils
const TArray<UAnimInstance*> UWvAnimInstance::GetAllAnimInstances()
{
	TArray<UAnimInstance*> Instances;
	Instances.Add(this);

	if (const IAnimClassInterface* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		const TArray<FStructProperty*>& LinkedAnimLayerNodeProperties = AnimBlueprintClass->GetLinkedAnimLayerNodeProperties();
		for (const FStructProperty* LayerNodeProperty : LinkedAnimLayerNodeProperties)
		{
			const FAnimNode_LinkedAnimLayer* Layer = LayerNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimLayer>(this);
			UAnimInstance* TargetInstance = Layer->GetTargetInstance<UAnimInstance>();
			if (IsValid(TargetInstance))
			{
				Instances.AddUnique(TargetInstance);
			}
		}

		const TArray<FStructProperty*>& LinkedAnimGraphNodeProperties = AnimBlueprintClass->GetLinkedAnimGraphNodeProperties();
		for (const FStructProperty* LinkedAnimGraphNodeProperty : LinkedAnimGraphNodeProperties)
		{
			const FAnimNode_LinkedAnimGraph* LinkedAnimGraph = LinkedAnimGraphNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimGraph>(this);
			UAnimInstance* TargetInstance = LinkedAnimGraph->GetTargetInstance<UAnimInstance>();
			if (IsValid(TargetInstance))
			{
				Instances.AddUnique(TargetInstance);
			}
		}
	}
	return Instances;
}

const TMap<FName, FAnimGroupInstance>& UWvAnimInstance::GetSyncGroupMapRead() const
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().GetSyncGroupMapRead();
}

const TArray<FAnimTickRecord>& UWvAnimInstance::GetUngroupedActivePlayersRead()
{
	return GetProxyOnGameThread<FAnimInstanceProxy>().GetUngroupedActivePlayersRead();
}

void UWvAnimInstance::DrawRelevantAnimation()
{
	if (!bOwnerPlayerController)
		return;

	const UWorld* World = Character->GetWorld();

	// check sync group
	{
		const TMap<FName, FAnimGroupInstance>& SyncGroupMap = GetSyncGroupMapRead();
		const TArray<FAnimTickRecord>& UngroupedActivePlayers = GetUngroupedActivePlayersRead();

		// Sync Groups and Sequences
		const FString SynGroupsHeading = FString::Printf(TEXT("SyncGroups: %i"), SyncGroupMap.Num());
		UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*SynGroupsHeading), true, false, FColor::Green, 0.0f);

		for (const TTuple<FName, FAnimGroupInstance>& SyncGroupPair : SyncGroupMap)
		{
			const FAnimGroupInstance& SyncGroup = SyncGroupPair.Value;
			const FString GroupLabel = FString::Printf(TEXT("Group %s - Players %i"), *SyncGroupPair.Key.ToString(), SyncGroup.ActivePlayers.Num());
			UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*GroupLabel), true, false, FColor::Green, 0.0f);

			if (SyncGroup.ActivePlayers.Num() > 0)
			{
				check(SyncGroup.GroupLeaderIndex != -1);
				constexpr bool bFullBlendSpaceDisplay = true;
				RenderAnimTickRecords(SyncGroup.ActivePlayers, SyncGroup.GroupLeaderIndex, FColor::White, FColor::Green, FColor::Black, bFullBlendSpaceDisplay);
			}
		}
		const FString UngroupedHeading = FString::Printf(TEXT("Ungrouped: %i"), UngroupedActivePlayers.Num());
		UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*UngroupedHeading), true, false, FColor::Green, 0.0f);

		constexpr int HighlightIndex = -1;
		constexpr bool bFullBlendSpaceDisplay = true;
		RenderAnimTickRecords(UngroupedActivePlayers, HighlightIndex, FColor::White, FColor::Green, FColor::Black, bFullBlendSpaceDisplay);
	}

	// montage & anim notify entry
	{
		const TArray<UAnimInstance*> AnimInstances = GetAllAnimInstances();
		for (UAnimInstance* AnimInstance : AnimInstances)
		{
			if (!IsValid(AnimInstance))
			{
				continue;
			}

			const FString ABPHeading = FString::Printf(TEXT("ABP Name: %s"), *AnimInstance->GetName());
			UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*ABPHeading), true, false, FColor::Yellow, 0.0f);

			const int32 MontageInstancesCount = AnimInstance->MontageInstances.Num();
			const FString MontagesHeading = FString::Printf(TEXT("Montages: %i"), MontageInstancesCount);
			UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*MontagesHeading), true, false, FColor::Blue, 0.0f);
			for (FAnimMontageInstance* MontageInstance : AnimInstance->MontageInstances)
			{
				FColor ActiveColor = MontageInstance->IsActive() ? FColor::Green : FColor::Black;

				if (MontageInstance && MontageInstance->Montage)
				{
					const FString MontageEntry = FString::Printf(TEXT("%s, CurrSec: %s, NextSec: %s, Weight: %.3f"),
						*MontageInstance->Montage->GetName(),
						*MontageInstance->GetCurrentSection().ToString(),
						*MontageInstance->GetNextSection().ToString(),
						MontageInstance->GetWeight());

					UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*MontageEntry), true, false, ActiveColor, 0.0f);
				}
			}

			const int32 ActiveNotifiesCount = ActiveAnimNotifyState.Num();
			const FString AnimNotifyHeading = FString::Printf(TEXT("Active Notify States: %i"), ActiveNotifiesCount);
			UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*AnimNotifyHeading), true, false, FColor::Blue, 0.0f);
			for (const FAnimNotifyEvent& NotifyState : AnimInstance->ActiveAnimNotifyState)
			{
				const FString NotifyEntry = FString::Printf(TEXT("%s Dur:%.3f"), *NotifyState.NotifyName.ToString(), NotifyState.GetDuration());
				UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*NotifyEntry), true, false, FColor::Green, 0.0f);
			}
		}

	}

}

void UWvAnimInstance::RenderAnimTickRecords(const TArray<FAnimTickRecord>& Records, const int32 HighlightIndex, FColor TextColor, FColor HighlightColor, FColor InInactiveColor, bool bFullBlendSpaceDisplay) const
{
	const UWorld* World = Character->GetWorld();
	for (int32 PlayerIndex = 0; PlayerIndex < Records.Num(); ++PlayerIndex)
	{
		const FAnimTickRecord& Player = Records[PlayerIndex];
		FString PlayerEntry = FString::Printf(TEXT("%i) %s"), PlayerIndex, *Player.SourceAsset->GetName());

		float Progress = -1.f;
		// See if we have access to SequenceLength
		if (UAnimSequenceBase* AnimSeqBase = Cast<UAnimSequenceBase>(Player.SourceAsset))
		{
			if (Player.TimeAccumulator != nullptr)
			{
				Progress = *Player.TimeAccumulator / AnimSeqBase->GetPlayLength();
			}
		}

		if (Progress == -1.f)
		{
			PlayerEntry += FString::Printf(TEXT(" P(%.2f)"), Player.TimeAccumulator != nullptr ? *Player.TimeAccumulator : 0.f);
		}
		UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*PlayerEntry), true, false, HighlightColor, 0.0f);

		if (Progress >= 0.f)
		{
			UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*FString::Printf(TEXT("%.3f"), Progress)), true, false, HighlightColor, 0.0f);
		}

		if (const UBlendSpace* BlendSpace = Cast<UBlendSpace>(Player.SourceAsset))
		{
			if (bFullBlendSpaceDisplay && Player.BlendSpace.BlendSampleDataCache && Player.BlendSpace.BlendSampleDataCache->Num() > 0)
			{
				TArray<FBlendSampleData> SampleData = *Player.BlendSpace.BlendSampleDataCache;
				SampleData.Sort([](const FBlendSampleData& L, const FBlendSampleData& R) { return L.SampleDataIndex < R.SampleDataIndex; });

				const FVector BlendSpacePosition(Player.BlendSpace.BlendSpacePositionX, Player.BlendSpace.BlendSpacePositionY, 0.f);
				const FString BlendSpaceHeader = FString::Printf(TEXT("Blendspace Input (%s)"), *BlendSpacePosition.ToString());
				UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*BlendSpaceHeader), true, false, HighlightColor, 0.0f);

				const TArray<FBlendSample>& BlendSamples = BlendSpace->GetBlendSamples();

				int32 WeightedSampleIndex = 0;

				for (int32 SampleIndex = 0; SampleIndex < BlendSamples.Num(); ++SampleIndex)
				{
					const FBlendSample& BlendSample = BlendSamples[SampleIndex];

					float Weight = 0.f;
					for (; WeightedSampleIndex < SampleData.Num(); ++WeightedSampleIndex)
					{
						FBlendSampleData& WeightedSample = SampleData[WeightedSampleIndex];
						if (WeightedSample.SampleDataIndex == SampleIndex)
						{
							Weight += WeightedSample.GetClampedWeight();
						}
						else if (WeightedSample.SampleDataIndex > SampleIndex)
						{
							break;
						}
					}

					const FString SampleEntry = FString::Printf(TEXT("%s"), *BlendSample.Animation->GetName());

					const FColor CurColor = (Weight > 0.f) ? HighlightColor : InInactiveColor;
					UKismetSystemLibrary::PrintString(World, TCHAR_TO_ANSI(*SampleEntry), true, false, CurColor, 0.0f);
				}
			}
		}

	}
}
#pragma endregion


