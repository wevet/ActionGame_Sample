// Copyright 2022 wevet works All Rights Reserved.


#include "Ability/WvAbility_VehicleDrive.h"
#include "Redemption.h"
#include "WvGameplayEffectContext.h"
#include "WvGameplayTargetData.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"

#include "Character/BaseCharacter.h"
#include "Character/WvPlayerController.h"
#include "Vehicle/WvWheeledVehiclePawn.h"
#include "Misc/WvCommonUtils.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbility_VehicleDrive)

UWvAbility_VehicleDrive::UWvAbility_VehicleDrive(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	bRetriggerInstancedAbility = true;
}

bool UWvAbility_VehicleDrive::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags /*= nullptr*/, const FGameplayTagContainer* TargetTags /*= nullptr*/, OUT FGameplayTagContainer* OptionalRelevantTags /*= nullptr*/) const
{
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UWvAbility_VehicleDrive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TriggerEventData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : not valid TriggerEventData]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}


	Instigator = Cast<APawn>(TriggerEventData->Instigator);
	Target = Cast<APawn>(TriggerEventData->Target);
	if (!Instigator.IsValid() || !Target.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s : not valid Instigator or Target]"), *FString(__FUNCTION__));
		CancelAbility(Handle, ActorInfo, ActivationInfo, false);
		return;
	}

	Controller = Instigator->GetController();
	if (!Controller.IsValid())
	{
		Controller = Target->GetController();
	}

	Controller->UnPossess();

	UE_LOG(LogTemp, Log, TEXT("Instigator => %s, Target => %s, function => %s"), *GetNameSafe(Instigator.Get()), *GetNameSafe(Target.Get()), *FString(__FUNCTION__));

	// If operation is started, collision is immediately disabled; if operation is terminated, collision remains disabled.
	DriveStartAction();

	PlayMontage();
}

void UWvAbility_VehicleDrive::DriveStartAction()
{
	if (bIsDriveEndEvent)
	{
		return;
	}

	if (ABaseCharacter* Character = Cast<ABaseCharacter>(Instigator))
	{
		Character->BeginVehicleAction();
	}

	if (AWvWheeledVehiclePawn* VehiclePawn = Cast<AWvWheeledVehiclePawn>(Target))
	{
		VehiclePawn->SetDrivingByPawn(Instigator.Get());
		HandlePossess(VehiclePawn);
	}
}

void UWvAbility_VehicleDrive::DriveEndAction()
{
	if (!bIsDriveEndEvent)
	{
		return;
	}

	if (AWvWheeledVehiclePawn* VehiclePawn = Cast<AWvWheeledVehiclePawn>(Target))
	{
		VehiclePawn->UnSetDrivingByPawn();
	}
	if (ABaseCharacter* Character = Cast<ABaseCharacter>(Instigator))
	{
		Character->EndVehicleAction();
		HandlePossess(Character);
	}
}

void UWvAbility_VehicleDrive::HandlePossess(APawn* NewPawn)
{
	if (Controller.IsValid())
	{
		Controller->Possess(NewPawn);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[%s : not valid Controller.]"), *FString(__FUNCTION__));
	}
}

void UWvAbility_VehicleDrive::PlayMontage()
{
	if (!AnimationMontage)
	{
		K2_EndAbility();
		return;
	}

	if (MontageTask)
	{
		MontageTask->OnCompleted.Clear();
		MontageTask->OnCancelled.Clear();
		MontageTask->OnInterrupted.Clear();
		MontageTask->EndTask();
	}

	MontageTask = UWvAT_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		FName("Drive"),
		AnimationMontage,
		FGameplayTagContainer(),
		1.0,
		0.f,
		FName("Default"),
		true,
		1.0f);

	MontageTask->OnCancelled.AddDynamic(this, &UWvAbility_VehicleDrive::OnPlayMontageCompleted_Event);
	MontageTask->OnInterrupted.AddDynamic(this, &UWvAbility_VehicleDrive::OnPlayMontageCompleted_Event);
	MontageTask->OnCompleted.AddDynamic(this, &UWvAbility_VehicleDrive::OnPlayMontageCompleted_Event);
	MontageTask->ReadyForActivation();
}

void UWvAbility_VehicleDrive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	DriveEndAction();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWvAbility_VehicleDrive::OnPlayMontageCompleted_Event(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_EndAbility();
}

