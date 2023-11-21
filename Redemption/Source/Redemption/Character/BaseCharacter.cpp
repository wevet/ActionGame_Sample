// Copyright 2022 wevet works All Rights Reserved.


#include "BaseCharacter.h"
#include "Redemption.h"
#include "Misc/WvCommonUtils.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Locomotion/LocomotionComponent.h"
#include "PredictionFootIKComponent.h"
#include "Component/InventoryComponent.h"
#include "Component/CombatComponent.h"
#include "Component/StatusComponent.h"
#include "Component/WeaknessComponent.h"
#include "WvPlayerController.h"
#include "Animation/WvAnimInstance.h"
#include "Ability/WvInheritanceAttributeSet.h"
#include "Game/WvGameInstance.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "MotionWarpingComponent.h"
#include "AI/Navigation/NavigationTypes.h"
#include "Delegates/Delegate.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameplayTagContainer.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "Misc/AssertionMacros.h"
#include "Net/UnrealNetwork.h"
#include "Templates/Casts.h"
#include "Trace/Detail/Channel.h"
#include "UObject/CoreNetTypes.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UObjectBaseUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Animation/AnimInstance.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Prediction.h"

// Misc
#include "Engine/SkeletalMeshSocket.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(BaseCharacter)


TSubclassOf<UAnimInstance> UOverlayAnimInstanceDataAsset::FindAnimInstance(const ELSOverlayState InOverlayState) const
{
	auto FindItemData = OverlayAnimInstances.FindByPredicate([&](FOverlayAnimInstance Item)
	{
		return (Item.OverlayState == InOverlayState);
	});

	if (FindItemData)
	{
		return FindItemData->AnimInstanceClass;
	}
	return UnArmedAnimInstanceClass;
}

ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWvCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	NetCullDistanceSquared = 900000000.0f;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));  // Rotate mesh to be X forward since it is exported as Y forward.

	UWvCharacterMovementComponent* WvMoveComp = CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement());
	WvMoveComp->GravityScale = 1.0f;
	WvMoveComp->MaxAcceleration = 2400.0f;
	WvMoveComp->BrakingFrictionFactor = 1.0f;
	WvMoveComp->BrakingFriction = 6.0f;
	WvMoveComp->GroundFriction = 8.0f;
	WvMoveComp->BrakingDecelerationWalking = 1400.0f;
	WvMoveComp->bUseControllerDesiredRotation = false;
	WvMoveComp->bOrientRotationToMovement = true;
	WvMoveComp->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
	WvMoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	WvMoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	WvMoveComp->bCanWalkOffLedgesWhenCrouching = true;
	WvMoveComp->SetCrouchedHalfHeight(55.0f);

	WvMoveComp->JumpZVelocity = 500.f;
	WvMoveComp->AirControl = 0.35f;
	WvMoveComp->MinAnalogWalkSpeed = 20.f;


	SetReplicateMovement(true);

	MotionWarpingComponent = ObjectInitializer.CreateDefaultSubobject<UMotionWarpingComponent>(this, TEXT("MotionWarpingComponent"));
	MotionWarpingComponent->bSearchForWindowsInAnimsWithinMontages = true;

	CharacterMovementTrajectoryComponent = ObjectInitializer.CreateDefaultSubobject<UCharacterMovementTrajectoryComponent>(this, TEXT("CharacterMovementTrajectoryComponent"));

	PredictionFootIKComponent = ObjectInitializer.CreateDefaultSubobject<UPredictionFootIKComponent>(this, TEXT("PredictionFootIKComponent"));
	PredictionFootIKComponent->bAutoActivate = 1;

	WvAbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UWvAbilitySystemComponent>(this, TEXT("WvAbilitySystemComponent"));
	WvAbilitySystemComponent->bAutoActivate = 1;

	LocomotionComponent = ObjectInitializer.CreateDefaultSubobject<ULocomotionComponent>(this, TEXT("LocomotionComponent"));
	LocomotionComponent->bAutoActivate = 1;

	// managed item, weapon class
	ItemInventoryComponent = ObjectInitializer.CreateDefaultSubobject<UInventoryComponent>(this, TEXT("InventoryComponent"));
	ItemInventoryComponent->bAutoActivate = 1;

	// managed combat system
	CombatComponent = ObjectInitializer.CreateDefaultSubobject<UCombatComponent>(this, TEXT("CombatComponent"));
	CombatComponent->bAutoActivate = 1;

	// managed character health and more
	StatusComponent = ObjectInitializer.CreateDefaultSubobject<UStatusComponent>(this, TEXT("StatusComponent"));
	StatusComponent->bAutoActivate = 1;

	// managed character combat weakness
	WeaknessComponent = ObjectInitializer.CreateDefaultSubobject<UWeaknessComponent>(this, TEXT("WeaknessComponent"));
	WeaknessComponent->bAutoActivate = 1;

	// noise emitter
	PawnNoiseEmitterComponent = ObjectInitializer.CreateDefaultSubobject<UPawnNoiseEmitterComponent>(this, TEXT("PawnNoiseEmitterComponent"));
	PawnNoiseEmitterComponent->bAutoActivate = 1;

	HeldObjectRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("HeldObjectRoot"));
	HeldObjectRoot->bAutoActivate = 1;
	HeldObjectRoot->SetupAttachment(GetMesh());

	MyTeamID = FGenericTeamId(0);
	CharacterTag = FGameplayTag::RequestGameplayTag(TAG_Character_Default.GetTag().GetTagName());
	CharacterRelation = ECharacterRelation::Friend;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	// sets Damage
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->AddTickPrerequisiteActor(this);
	CombatComponent->AddTickPrerequisiteActor(this);
	PawnNoiseEmitterComponent->AddTickPrerequisiteActor(this);

	AnimInstance = Cast<UWvAnimInstance>(GetMesh()->GetAnimInstance());

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	CMC->OnWallClimbingBeginDelegate.AddDynamic(this, &ABaseCharacter::OnWallClimbingBegin_Callback);
	CMC->OnWallClimbingEndDelegate.AddDynamic(this, &ABaseCharacter::OnWallClimbingEnd_Callback);

	RequestAsyncLoad();
}

void ABaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.ClearTimer(Ragdoll_TimerHandle);

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	CMC->OnWallClimbingBeginDelegate.RemoveDynamic(this, &ABaseCharacter::OnWallClimbingBegin_Callback);
	CMC->OnWallClimbingEndDelegate.RemoveDynamic(this, &ABaseCharacter::OnWallClimbingEnd_Callback);

	ResetFinisherAnimationData();

	OverlayAnimDAInstance = nullptr;

	FinisherSender = nullptr;
	FinisherReceiner = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
	const FGenericTeamId OldTeamID = MyTeamID;

	Super::PossessedBy(NewController);

	// Grab the current team ID and listen for future changes
	if (IWvAbilityTargetInterface* ControllerAsTeamProvider = Cast<IWvAbilityTargetInterface>(NewController))
	{
		MyTeamID = ControllerAsTeamProvider->GetGenericTeamId();
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
	}

	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
	InitAbilitySystemComponent();
}

void ABaseCharacter::UnPossessed()
{
	AController* const OldController = Controller;

	// Stop listening for changes from the old controller
	const FGenericTeamId OldTeamID = MyTeamID;
	if (IWvAbilityTargetInterface* ControllerAsTeamProvider = Cast<IWvAbilityTargetInterface>(OldController))
	{
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
	}

	Super::UnPossessed();

	// @TODO
	// Determine what the new team ID should be afterwards
	MyTeamID = DetermineNewTeamAfterPossessionEnds(OldTeamID);
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	auto CMC = GetCharacterMovement();
	if (IsValid(CMC))
	{
		const float Acc = CMC->GetCurrentAcceleration().Length();
		const float MaxAcc = CMC->GetMaxAcceleration();
		MovementInputAmount = Acc / MaxAcc;
		bHasMovementInput = (MovementInputAmount > 0.0f);
	}

	if (IsValid(CharacterMovementTrajectoryComponent))
	{
		TrajectorySampleRange = CharacterMovementTrajectoryComponent->GetTrajectory();
	}
}

void ABaseCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	ILocomotionInterface::Execute_SetLSStanceMode(LocomotionComponent, ELSStance::Crouching);
}

void ABaseCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	ILocomotionInterface::Execute_SetLSStanceMode(LocomotionComponent, ELSStance::Standing);
}

void ABaseCharacter::InitAbilitySystemComponent()
{
	WvAbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (GetLocalRole() == ROLE_Authority)
	{
		WvAbilitySystemComponent->AddStartupGameplayAbilities();
	}

	auto* PC = Cast<AWvPlayerController>(Controller);
	if (PC && IsLocallyControlled())
	{
		PC->PostAscInitialize(WvAbilitySystemComponent);
	}

}

void ABaseCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedAcceleration, COND_SimulatedOnly);
	DOREPLIFETIME(ThisClass, MyTeamID)
}

void ABaseCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		const double MaxAccel = MovementComponent->MaxAcceleration;
		const FVector CurrentAccel = MovementComponent->GetCurrentAcceleration();
		double AccelXYRadians, AccelXYMagnitude;
		FMath::CartesianToPolar(CurrentAccel.X, CurrentAccel.Y, AccelXYMagnitude, AccelXYRadians);

		ReplicatedAcceleration.AccelXYRadians = FMath::FloorToInt((AccelXYRadians / TWO_PI) * 255.0);     // [0, 2PI] -> [0, 255]
		ReplicatedAcceleration.AccelXYMagnitude = FMath::FloorToInt((AccelXYMagnitude / MaxAccel) * 255.0);	// [0, MaxAccel] -> [0, 255]
		ReplicatedAcceleration.AccelZ = FMath::FloorToInt((CurrentAccel.Z / MaxAccel) * 127.0);   // [-MaxAccel, MaxAccel] -> [-127, 127]
	}
}

void ABaseCharacter::OnRep_ReplicatedAcceleration()
{
	if (UWvCharacterMovementComponent* WvCharacterMovementComponent = Cast<UWvCharacterMovementComponent>(GetCharacterMovement()))
	{
		// Decompress Acceleration
		const double MaxAccel = WvCharacterMovementComponent->MaxAcceleration;
		const double AccelXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0; // [0, 255] -> [0, MaxAccel]
		const double AccelXYRadians = double(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;     // [0, 255] -> [0, 2PI]

		FVector UnpackedAcceleration(FVector::ZeroVector);
		FMath::PolarToCartesian(AccelXYMagnitude, AccelXYRadians, UnpackedAcceleration.X, UnpackedAcceleration.Y);
		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0; // [-127, 127] -> [-MaxAccel, MaxAccel]
		WvCharacterMovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
	}
}

void ABaseCharacter::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void ABaseCharacter::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	const FGenericTeamId MyOldTeamID = MyTeamID;
	MyTeamID = IntegerToGenericTeamId(NewTeam);
	ConditionalBroadcastTeamChanged(this, MyOldTeamID, MyTeamID);
}

void ABaseCharacter::NotifyControllerChanged()
{
	const FGenericTeamId OldTeamId = GetGenericTeamId();

	Super::NotifyControllerChanged();

	// Update our team ID based on the controller
	if (HasAuthority() && (Controller != nullptr))
	{
		if (IWvAbilityTargetInterface* ControllerWithTeam = Cast<IWvAbilityTargetInterface>(Controller))
		{
			MyTeamID = ControllerWithTeam->GetGenericTeamId();
			ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamID);
		}
	}
}

#pragma region IWvAbilitySystemAvatarInterface
const FWvAbilitySystemAvatarData& ABaseCharacter::GetAbilitySystemData()
{
	return AbilitySystemData;
}

const FCustomWvAbilitySystemAvatarData& ABaseCharacter::GetCustomWvAbilitySystemData()
{
	return AbilitySystemData;
}

void ABaseCharacter::InitAbilitySystemComponentByData(class UWvAbilitySystemComponentBase* ASC)
{
	IWvAbilitySystemAvatarInterface::InitAbilitySystemComponentByData(ASC);

	// Read DataTable of locomotion system
	const FCustomWvAbilitySystemAvatarData& Data = GetCustomWvAbilitySystemData();

	TArray<TSoftObjectPtr<UDataTable>> AbilityTables;
	AbilityTables.Add(Data.LocomotionAbilityTable);
	AbilityTables.Add(Data.FieldAbilityTable);
	AbilityTables += Data.FunctionAbilityTables;

	for (int32 Index = 0; Index < AbilityTables.Num(); Index++)
	{
		TSoftObjectPtr<UDataTable> SoftAbilityTable = AbilityTables[Index];

		if (SoftAbilityTable.IsNull())
		{
			continue;
		}

		const FSoftObjectPath TablePath = SoftAbilityTable.ToSoftObjectPath();
		const FString TablePathString = TablePath.ToString();

		UDataTable* AbilityTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *TablePathString));
		if (!AbilityTable)
		{
			continue;
		}

		TArray<FWvAbilityRow*> Rows;
		AbilityTable->GetAllRows(SoftAbilityTable.GetAssetName(), Rows);

		for (int32 JIndex = 0; JIndex < Rows.Num(); ++JIndex)
		{
			const FWvAbilityRow* AbilityRow = Rows[JIndex];
			WvAbilitySystemComponent->AddRegisterAbilityDA(AbilityRow->AbilityData);
		}
	}
	WvAbilitySystemComponent->GiveAllRegisterAbility();
}
#pragma endregion

#pragma region IWvAbilityTargetInterface
FGenericTeamId ABaseCharacter::GetGenericTeamId() const
{
	return MyTeamID;
}

void ABaseCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (!GetController())
	{
		if (HasAuthority())
		{
			const FGenericTeamId OldTeamID = MyTeamID;
			MyTeamID = NewTeamID;
			ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("You can't set the team ID on a character (%s) except on the authority"), *GetPathNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("You can't set the team ID on a possessed character (%s); it's driven by the associated controller"), *GetPathNameSafe(this));
	}
}

FOnTeamIndexChangedDelegate* ABaseCharacter::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

ECharacterRelation ABaseCharacter::GetRelationWithSelfImpl(const IWvAbilityTargetInterface* Other) const
{
	return CharacterRelation;
}

FGameplayTag ABaseCharacter::GetAvatarTag() const
{
	return CharacterTag;
}

USceneComponent* ABaseCharacter::GetOverlapBaseComponent()
{
	return GetMesh();
}

void ABaseCharacter::OnSendWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage)
{
}

void ABaseCharacter::OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
	//UE_LOG(LogTemp, Log, TEXT("Owner => %s, Actor => %s, function => %s"), *GetPathName(this), *GetPathName(Actor), *FString(__FUNCTION__));
}

void ABaseCharacter::OnSendKillTarget(AActor* Actor, const float Damage)
{
	if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(GetController()))
	{
		Interface->OnSendKillTarget(Actor, Damage);
	}
	UE_LOG(LogTemp, Log, TEXT("Owner => %s, Actor => %s, function => %s"), *GetPathNameSafe(this), *GetPathNameSafe(Actor), *FString(__FUNCTION__));
}

void ABaseCharacter::OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage)
{
}

void ABaseCharacter::OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
}

void ABaseCharacter::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
	if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(GetController()))
	{
		Interface->OnReceiveKillTarget(Actor, Damage);
	}
}

void ABaseCharacter::OnReceiveHitReact(FGameplayEffectContextHandle Context, const bool IsInDead, const float Damage)
{
	CombatComponent->StartHitReact(Context, IsInDead, Damage);
}

bool ABaseCharacter::IsDead() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_StateDead);
}

bool ABaseCharacter::IsTargetable() const
{
	return !IsDead();
}

bool ABaseCharacter::IsInBattled() const
{
	if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(GetController()))
	{
		Interface->IsInBattled();
	}
	return false;
}

void ABaseCharacter::Freeze()
{
	if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(GetController()))
	{
		Interface->Freeze();
	}

	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidMovement, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidMantling, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidJump, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_ActionMelee_Forbid, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_ActionJump_Forbid, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_ActionDash_Forbid, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_ActionCrouch_Forbid, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_TargetLock_Forbid, 1);
}

void ABaseCharacter::UnFreeze()
{
	if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(GetController()))
	{
		Interface->UnFreeze();
	}

	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Locomotion_ForbidMovement, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Locomotion_ForbidClimbing, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Locomotion_ForbidMantling, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Locomotion_ForbidJump, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionMelee_Forbid, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionJump_Forbid, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionDash_Forbid, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionCrouch_Forbid, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_TargetLock_Forbid, 1);
}
#pragma endregion

#pragma region IWvAIActionStateInterface
void ABaseCharacter::SetAIActionState(const EAIActionState NewAIActionState)
{
	if (AIActionState == NewAIActionState)
	{
		return;
	}

	const auto PrevActionState = AIActionState;
	AIActionState = NewAIActionState;

	if (AIActionStateDA)
	{
		const FGameplayTag PrevStateTag = AIActionStateDA->FindActionStateTag(PrevActionState);
		if (PrevStateTag.IsValid())
		{
			WvAbilitySystemComponent->RemoveGameplayTag(PrevStateTag, 1);
		}

		const FGameplayTag CurStateTag = AIActionStateDA->FindActionStateTag(AIActionState);
		if (CurStateTag.IsValid())
		{
			WvAbilitySystemComponent->AddGameplayTag(CurStateTag, 1);
		}
	}
}

EAIActionState ABaseCharacter::GetAIActionState() const
{
	return AIActionState;
}
#pragma endregion

#pragma region Components
UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return WvAbilitySystemComponent;
}

UMotionWarpingComponent* ABaseCharacter::GetMotionWarpingComponent() const
{
	return MotionWarpingComponent; 
}

UCharacterMovementTrajectoryComponent* ABaseCharacter::GetCharacterMovementTrajectoryComponent() const
{
	return CharacterMovementTrajectoryComponent; 
}

UWvCharacterMovementComponent* ABaseCharacter::GetWvCharacterMovementComponent() const
{
	return CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement());
}

UWvAbilitySystemComponent* ABaseCharacter::GetWvAbilitySystemComponent() const
{
	return CastChecked<UWvAbilitySystemComponent>(WvAbilitySystemComponent);
}

ULocomotionComponent* ABaseCharacter::GetLocomotionComponent() const
{
	return LocomotionComponent;
}

UCombatComponent* ABaseCharacter::GetCombatComponent() const
{
	return CombatComponent;
}

UInventoryComponent* ABaseCharacter::GetInventoryComponent() const
{
	return ItemInventoryComponent;
}

UWeaknessComponent* ABaseCharacter::GetWeaknessComponent() const
{
	return WeaknessComponent;
}

USceneComponent* ABaseCharacter::GetHeldObjectRoot() const
{
	return HeldObjectRoot;
}
#pragma endregion

#pragma region Action
void ABaseCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	LocomotionComponent->OnLanded();
}

void ABaseCharacter::Jump()
{
	if (GetCharacterMovement()->IsCrouching())
	{
		UnCrouch();
	}
	else
	{
		UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
		const EMovementMode MovementMode = CMC->MovementMode;
		switch (MovementMode)
		{
			case MOVE_None:
			break;
			case MOVE_Walking:
			case MOVE_NavWalking:
			{
				if (bHasMovementInput)
				{
					const bool bResult = CMC->GroundMantling();
					if (!bResult)
					{
						Super::Jump();
					}
				}
				else
				{
					Super::Jump();
				}
			}
			break;
			case MOVE_Falling:
			{
				CMC->FallingMantling();
			}
			break;
			case MOVE_Flying:
			break;
			case MOVE_Swimming:
			break;
			case MOVE_Custom:
			{
				const ECustomMovementMode CustomMovementMode = (ECustomMovementMode)CMC->CustomMovementMode;
				switch (CustomMovementMode)
				{
					case CUSTOM_MOVE_Climbing:
					break;
					case CUSTOM_MOVE_WallClimbing:
					CMC->TryClimbJumping();
					break;
					case CUSTOM_MOVE_Mantling:
					break;
				}
			}
			break;
		}
	}
}

void ABaseCharacter::StopJumping()
{
	Super::StopJumping();
}

void ABaseCharacter::DoAttack()
{
	if (WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_ActionMelee_Forbid))
	{
		UE_LOG(LogTemp, Warning, TEXT("has tag TAG_Character_StateMelee_Forbid => %s"), *FString(__FUNCTION__));
		return;
	}

	const auto Weapon = ItemInventoryComponent->GetEquipWeapon();
	if (Weapon)
	{
		const FGameplayTag TriggerTag = Weapon->GetPluralInputTriggerTag();
		WvAbilitySystemComponent->TryActivateAbilityByTag(TriggerTag);
	}
}

void ABaseCharacter::DoResumeAttack()
{
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionMelee_Forbid, 1);
}

void ABaseCharacter::DoStopAttack()
{
	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_ActionMelee_Forbid, 1);
}

void ABaseCharacter::VelocityMovement()
{
	if (LocomotionComponent->GetLockUpdatingRotation())
	{
		return;
	}

	auto CMC = GetWvCharacterMovementComponent();

	if (CMC)
	{
		CMC->bUseControllerDesiredRotation = false;
		CMC->bOrientRotationToMovement = true;
	}

	ILocomotionInterface::Execute_SetLSRotationMode(LocomotionComponent, ELSRotationMode::VelocityDirection);
}

void ABaseCharacter::StrafeMovement()
{
	if (LocomotionComponent->GetLockUpdatingRotation())
	{
		return;
	}

	auto CMC = GetWvCharacterMovementComponent();

	if (CMC)
	{
		CMC->bUseControllerDesiredRotation = true;
		CMC->bOrientRotationToMovement = false;
	}

	ILocomotionInterface::Execute_SetLSRotationMode(LocomotionComponent, ELSRotationMode::LookingDirection);
}

void ABaseCharacter::DoSprinting()
{
	LocomotionComponent->SetSprintPressed(true);
}

void ABaseCharacter::DoStopSprinting()
{
	LocomotionComponent->SetSprintPressed(false);
}

void ABaseCharacter::DoWalking()
{
	LocomotionComponent->SetLSGaitMode_Implementation(ELSGait::Walking);
}

void ABaseCharacter::DoStopWalking()
{
	LocomotionComponent->SetLSGaitMode_Implementation(ELSGait::Running);
}

void ABaseCharacter::DoStartCrouch()
{
	if (bIsCrouched)
	{
		Super::UnCrouch();
		return;
	}
	Super::Crouch();
}

void ABaseCharacter::DoStopCrouch()
{
	if (bIsCrouched)
	{
		Super::UnCrouch();
	}
}

void ABaseCharacter::DoStartAiming()
{
	StrafeMovement();
	DoWalking();
	LocomotionComponent->SetLSAiming_Implementation(true);
}

void ABaseCharacter::DoStopAiming()
{
	LocomotionComponent->SetLSAiming_Implementation(false);
}

void ABaseCharacter::OnWallClimbingBegin_Callback()
{
	UE_LOG(LogTemp, Log, TEXT("Character => %s, function => %s"), *GetName(), *FString(__FUNCTION__));
}

void ABaseCharacter::OnWallClimbingEnd_Callback()
{
	UE_LOG(LogTemp, Log, TEXT("Character => %s, function => %s"), *GetName(), *FString(__FUNCTION__));
}

/// <summary>
/// ref BTT_DoCover
/// </summary>
void ABaseCharacter::HandleCrouchAction(const bool bCanCrouch)
{
	if (bCanCrouch)
	{
		DoStartCrouch();
	}
	else
	{
		DoStopCrouch();
	}
}

void ABaseCharacter::HandleGuardAction(const bool bGuardEnable)
{
	if (bGuardEnable)
	{
		WvAbilitySystemComponent->AddGameplayTag(TAG_Character_DamageBlock, 1);
	}
	else
	{
		WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_DamageBlock, 1);
	}
}
#pragma endregion

/// <summary>
/// ref BTT_DoCover
/// </summary>
FTransform ABaseCharacter::GetChestTransform(const FName BoneName) const
{
	return GetMesh()->GetSocketTransform(BoneName);
}

FTrajectorySampleRange ABaseCharacter::GetTrajectorySampleRange() const
{
	return TrajectorySampleRange;
}

FVector2D ABaseCharacter::GetInputAxis() const
{
	return InputAxis;
}

FVector ABaseCharacter::GetLedgeInputVelocity() const
{
	return GetForwardMoveDir(-GetActorUpVector()) * InputAxis.Y + GetRightMoveDir(-GetActorUpVector()) * InputAxis.X;
}

FVector ABaseCharacter::GetRightMoveDir(FVector CompareDir) const
{
	const FRotator ControllRotation = GetControlRotation();
	FVector CameraRight = UKismetMathLibrary::GetRightVector(ControllRotation);
	const float Angle = UWvCommonUtils::GetAngleBetweenVector(CameraRight, CompareDir);
	const float HalfAngle = (180 - Angle);
	if (Angle < InputDirVerThreshold)
	{
		CameraRight = UKismetMathLibrary::GetUpVector(ControllRotation);
	}
	else if (HalfAngle < InputDirVerAngleThreshold)
	{
		CameraRight = FVector::ZeroVector - UKismetMathLibrary::GetUpVector(ControllRotation);
	}
	CameraRight = UKismetMathLibrary::ProjectVectorOnToPlane(CameraRight, GetActorUpVector());
	CameraRight.Normalize();
	return CameraRight;
}

FVector ABaseCharacter::GetForwardMoveDir(FVector CompareDir) const
{
	const FRotator ControllRotation = GetControlRotation();
	FVector CameraForward = UKismetMathLibrary::GetForwardVector(ControllRotation);
	const float Angle = UWvCommonUtils::GetAngleBetweenVector(CameraForward, CompareDir);
	const float HalfAngle = (180 - Angle);
	if (Angle < InputDirVerThreshold)
	{
		CameraForward = UKismetMathLibrary::GetUpVector(ControllRotation);
	}
	else if (HalfAngle < InputDirVerAngleThreshold)
	{
		CameraForward = FVector::ZeroVector - UKismetMathLibrary::GetUpVector(ControllRotation);
	}
	CameraForward = UKismetMathLibrary::ProjectVectorOnToPlane(CameraForward, GetActorUpVector());
	CameraForward.Normalize();
	return CameraForward;
}

FVector ABaseCharacter::GetCharacterFeetLocation() const
{
	auto Position = GetActorLocation();
	const float Height = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Position.Z -= Height;
	return Position;
}

float ABaseCharacter::GetDistanceFromToeToKnee(FName KneeL, FName BallL, FName KneeR, FName BallR) const
{
	const FVector KneeLPosition = GetMesh()->GetSocketLocation(KneeL);
	const FVector BallLPosition = GetMesh()->GetSocketLocation(BallL);

	const FVector KneeRPosition = GetMesh()->GetSocketLocation(KneeR);
	const FVector BallRPosition = GetMesh()->GetSocketLocation(BallR);

	const float L = (KneeLPosition - BallLPosition).Size();
	const float R = (KneeRPosition - BallRPosition).Size();

	const float Result = FMath::Max(FMath::Abs(L), FMath::Abs(R));
	return FMath::Max(GetWvCharacterMovementComponent()->MaxStepHeight, Result);
}

void ABaseCharacter::BeginDeathAction()
{
	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_StateDead, 1);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidMovement, 1);
	UE_LOG(LogTemp, Log, TEXT("Owner => %s, function => %s"), *GetName(), *FString(__FUNCTION__));
}

void ABaseCharacter::EndDeathAction(const float Interval)
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(Ragdoll_TimerHandle))
	{
		TM.ClearTimer(Ragdoll_TimerHandle);
	}

	TM.SetTimer(Ragdoll_TimerHandle, this, &ABaseCharacter::EndDeathAction_Callback, Interval, false);
}

void ABaseCharacter::EndDeathAction_Callback()
{
	ILocomotionInterface::Execute_SetLSMovementMode(LocomotionComponent, ELSMovementMode::Ragdoll);
	LocomotionComponent->StartRagdollAction();

	if (IsBotCharacter())
	{
		//DetachFromControllerPendingDestroy();
		Super::SetLifeSpan(DEFAULT_LIFESPAN);
	}
	UE_LOG(LogTemp, Log, TEXT("Owner => %s, function => %s"), *GetName(), *FString(__FUNCTION__));
}

void ABaseCharacter::OverlayStateChange(const ELSOverlayState CurrentOverlay)
{
	if (!IsValid(OverlayAnimDAInstance))
	{
		OverlayAnimDAInstance = OverlayAnimInstanceDA.LoadSynchronous();
		return;
	}

	auto AnimInstanceClass = OverlayAnimDAInstance->FindAnimInstance(CurrentOverlay);
	if (AnimInstanceClass)
	{
		AnimInstance->LinkAnimClassLayers(AnimInstanceClass);
	}
}

bool ABaseCharacter::IsTargetLock() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_TargetLocking);
}

bool ABaseCharacter::IsBotCharacter() const
{
	return UWvCommonUtils::IsBot(GetController());
}

#pragma region AI
// AI Perception
// https://blog.gamedev.tv/ai-sight-perception-to-custom-points/
bool ABaseCharacter::CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible, int32* UserData) const
{
	check(GetMesh());
	static const FName NAME_AILineOfSight = FName(TEXT("TestPawnLineOfSight"));

	FHitResult HitResult;
	const TArray<USkeletalMeshSocket*> Sockets = GetMesh()->GetSkeletalMeshAsset()->GetActiveSocketList();

	for (int32 Index = 0; Index < Sockets.Num(); ++Index)
	{
		const FVector SocketLocation = GetMesh()->GetSocketLocation(Sockets[Index]->SocketName);
		const bool bHitSocket = GetWorld()->LineTraceSingleByObjectType(HitResult, ObserverLocation, SocketLocation,
			FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic) | ECC_TO_BITFIELD(ECC_Pawn)),
			FCollisionQueryParams(NAME_AILineOfSight, true, IgnoreActor));
		NumberOfLoSChecksPerformed++;

		if (bHitSocket || (HitResult.GetActor() && HitResult.GetActor()->IsOwnedBy(this)))
		{
			OutSeenLocation = SocketLocation;
			OutSightStrength = 1;
			return true;
		}
	}

	HitResult.Reset();
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(HitResult, ObserverLocation, GetActorLocation(),
		FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic) | ECC_TO_BITFIELD(ECC_Pawn)),
		FCollisionQueryParams(NAME_AILineOfSight, true, IgnoreActor));
	NumberOfLoSChecksPerformed++;

	if (bHit || (HitResult.GetActor() && HitResult.GetActor()->IsOwnedBy(this)))
	{
		OutSeenLocation = GetActorLocation();
		OutSightStrength = 1;
		return true;
	}

	OutSightStrength = 0;
	return false;
}

class UBehaviorTree* ABaseCharacter::GetBehaviorTree() const
{
	return BehaviorTree;
}
#pragma endregion


/// <summary>
/// Sound
/// Send Sound AI
/// </summary>
void ABaseCharacter::ReportNoiseEvent(const FVector Offset, const float Volume, const float Radius)
{
	FVector Location = GetActorLocation();
	Location += Offset;
	PawnNoiseEmitterComponent->MakeNoise(this, Volume, Location);
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), Location, Volume, this, Radius);
}

void ABaseCharacter::ReportPredictionEvent(AActor* PredictedActor, const float PredictionTime)
{
	UAISense_Prediction::RequestPawnPredictionEvent(this, PredictedActor, PredictionTime);
}

FVector ABaseCharacter::GetPredictionStopLocation(const FVector CurrentLocation) const
{
	const auto CMC = GetCharacterMovement();
	// Get and store current world delta seconds for later use
	const float DT = GetWorld()->GetDeltaSeconds();
	// Small number break loop when velocity is less than this value
	const float SmallVelocity = 10.f * FMath::Square(DT);
	// Current velocity at current frame in unit/frame
	FVector CurrentVelocityInFrame = CMC->Velocity * DT;
	// Store velocity direction for later use
	const FVector CurrentVelocityDirection = CMC->Velocity.GetSafeNormal2D();
	// Current deacceleration at current frame in unit/fame^2
	const FVector CurrentDeaccelerationInFrame = (CurrentVelocityDirection * CMC->BrakingDecelerationWalking) * FMath::Square(DT);
	// Calculate number of frames needed to reach zero velocity and gets its int value
	const int32 StopFrameCount = CurrentVelocityInFrame.Size() / CurrentDeaccelerationInFrame.Size();
	// float variable use to store distance to targeted stop location
	float StoppingDistance = 0.0f;
	// Do Stop calculation go through all frames and calculate stop distance in each frame and stack them
	for (int32 Index = 0; Index <= StopFrameCount; Index++)
	{
		CurrentVelocityInFrame -= CurrentDeaccelerationInFrame;

		// if velocity in XY plane is small break loop for safety
		if (CurrentVelocityInFrame.Size2D() <= SmallVelocity)
		{
			break;
		}
		// Calculate distance travel in current frame and add to previous distance
		StoppingDistance += CurrentVelocityInFrame.Size2D();
	}
	// return stopping distance from player position in previous frame
	return CurrentLocation + CurrentVelocityDirection * StoppingDistance;
}

#pragma region NearlestAction
void ABaseCharacter::FindNearlestTarget(const FAttackMotionWarpingData AttackMotionWarpingData)
{
	auto Target = FindNearlestTarget(AttackMotionWarpingData.NearlestDistance, AttackMotionWarpingData.AngleThreshold, false);
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("not found FindNearlestTarget => %s"), *FString(__FUNCTION__));
		return;
	}

	const FVector From = GetActorLocation();
	const FVector To = Target->GetActorLocation();
	const float Weight = AttackMotionWarpingData.TargetSyncPointWeight;
	MotionWarpingComponent->RemoveWarpTarget(NEARLEST_TARGET_SYNC_POINT);
	const FRotator TargetLookAt = UKismetMathLibrary::FindLookAtRotation(From, To);

	const FRotator Rotation = UKismetMathLibrary::RLerp(GetActorRotation(), TargetLookAt, Weight, true);
	FMotionWarpingTarget WarpingTarget;
	WarpingTarget.Name = NEARLEST_TARGET_SYNC_POINT;
	WarpingTarget.Location = UKismetMathLibrary::VLerp(From, To, Weight);
	WarpingTarget.Rotation = FRotator(0.f, Rotation.Yaw, 0.f);
	MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	DrawDebugSphere(GetWorld(), From, 20.f, 12, FColor::Blue, false, 2);
	DrawDebugSphere(GetWorld(), To, 20.f, 12, FColor::Blue, false, 2);
	DrawDebugDirectionalArrow(GetWorld(), From, To, 20.f, FColor::Red, false, 2);
#endif

}

const TArray<AActor*> ABaseCharacter::FindNearlestTargets(const float Distance, const float AngleThreshold)
{
	// WorldDynamic and Pawn object type
	static const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = { EObjectTypeQuery::ObjectTypeQuery2, EObjectTypeQuery::ObjectTypeQuery3 };

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	TArray<AActor*> HitTargets;
	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetActorLocation(), Distance, ObjectTypes, ABaseCharacter::StaticClass(), IgnoreActors, HitTargets);

	// Get the target with the smallest angle difference from the camera forward vector
	TArray<AActor*> FilterTargets;
	for (int32 Index = 0; Index < HitTargets.Num(); ++Index)
	{
		if (ABaseCharacter* Target = Cast<ABaseCharacter>(HitTargets[Index]))
		{
			if (!IsValid(Target) || Target->IsDead())
			{
				continue;
			}

			const FVector NormalizePos = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			const FVector Forward = GetActorForwardVector();
			const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct(Forward, NormalizePos));
			const bool bIsTargetInView = (FMath::Abs(Angle) < AngleThreshold);
			if (bIsTargetInView)
			{
				FilterTargets.Add(Target);

//#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#if false
				const FVector From = GetActorLocation();
				const FVector To = Target->GetActorLocation();
				DrawDebugSphere(GetWorld(), From, 20.f, 12, FColor::Blue, false, 2);
				DrawDebugSphere(GetWorld(), To, 20.f, 12, FColor::Blue, false, 2);
				DrawDebugDirectionalArrow(GetWorld(), From, To, 20.f, FColor::Red, false, 2);
#endif
			}
		}
	}
	return FilterTargets;
}

/// <summary>
/// Find Target, for example HoldUp. Finisher. KnockOut.
/// </summary>
AActor* ABaseCharacter::FindNearlestTarget(const float Distance, const float AngleThreshold, bool bTargetCheckBattled/* = true*/)
{
	TArray<AActor*> HitTargets = FindNearlestTargets(Distance, AngleThreshold);

	if (HitTargets.Num() <= 0)
	{
		return nullptr;
	}

	// Get the target with the smallest angle difference from the camera forward vector
	float ClosestDotToCenter = 0.f;
	ABaseCharacter* NearlestTarget = nullptr;

	for (int32 Index = 0; Index < HitTargets.Num(); ++Index)
	{
		if (ABaseCharacter* Target = Cast<ABaseCharacter>(HitTargets[Index]))
		{
			if (!IsValid(Target))
			{
				continue;
			}

			if (Target->IsDead())
			{
				continue;
			}

			if (bTargetCheckBattled && Target->IsInBattled())
			{
				continue;
			}

			const FVector NormalizePos = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			const FVector Forward = GetActorForwardVector();
			const float Dot = FVector::DotProduct(Forward, NormalizePos);
			if (Dot > ClosestDotToCenter)
			{
				ClosestDotToCenter = Dot;
				NearlestTarget = Target;
			}
		}
	}
	return NearlestTarget;
}

const bool ABaseCharacter::CanFiniherSender()
{
	if (!IsValid(FinisherSender))
	{
		OnLoadFinisherSender();
		return false;
	}
	return true;
}

const bool ABaseCharacter::CanFiniherReceiver()
{
	if (!IsValid(FinisherReceiner))
	{
		OnLoadFinisherReceiver();
		return false;
	}
	return true;
}

void ABaseCharacter::BuildFinisherAbility(const FGameplayTag RequireTag)
{
	if (!CanFiniherSender() || !CanFiniherReceiver())
	{
		return;
	}

	auto Target = FindNearlestTarget(FinisherConfig.NearlestDistance, FinisherConfig.AngleThreshold);
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("not found FindNearlestTarget => %s"), *FString(__FUNCTION__));
		return;
	}

	ABaseCharacter* TargetCharacter = Cast<ABaseCharacter>(Target);
	if (!TargetCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("cast faild TargetCharacter => %s"), *FString(__FUNCTION__));
		return;
	}

	int32 OutIndex = 0;
	FFinisherAnimation Sender;
	BuildFinisherAnimationSender(RequireTag, Sender, OutIndex);

	// Determine whether you are in front of or behind the enemy.
	const FRotator TargetRot = UKismetMathLibrary::FindLookAtRotation(TargetCharacter->GetActorLocation(), GetActorLocation());
	const float Dot = (TargetCharacter->GetActorForwardVector() | UKismetMathLibrary::GetForwardVector(TargetRot));
	const float Acos = (180.0) / UE_DOUBLE_PI * FMath::Acos(Dot);
	const bool bHasForward = UKismetMathLibrary::InRange_FloatFloat(Acos, 0.f, 90.0f, true, true);

	const EFinisherDirectionType Direction = Sender.FinisherDirectionType;
	switch (Direction)
	{
		case EFinisherDirectionType::Backward:
		{
			if (bHasForward)
			{
				UE_LOG(LogTemp, Warning, TEXT("target is front => %s"), *FString(__FUNCTION__));
				return;
			}
		}
		break;
		case EFinisherDirectionType::Forward:
		{
			if (!bHasForward)
			{
				UE_LOG(LogTemp, Warning, TEXT("target is backward => %s"), *FString(__FUNCTION__));
				return;
			}
		}
		break;
	}

	BuildFinisherAnimationData(Sender.Montage, false, nullptr);
	FFinisherAnimation Receiver;
	TargetCharacter->BuildFinisherAnimationReceiver(RequireTag, OutIndex, Receiver);
	TargetCharacter->BuildFinisherAnimationData(Receiver.Montage, Receiver.IsTurnAround, this);
	UE_LOG(LogTemp, Log, TEXT("Finisher Sender => %s, Receiver => %s, Acos => %.3f, Dot => %.3f"), *GetPathName(Sender.Montage), *GetPathName(Receiver.Montage), Acos, Dot);

	// setup warping
	MotionWarpingComponent->RemoveWarpTarget(FINISHER_TARGET_SYNC_POINT);
	const FRotator TargetLookAt = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TargetCharacter->GetActorLocation());
	const float CurDistance = (GetActorLocation() - TargetCharacter->GetActorLocation()).Size2D();
	FVector Forward = GetActorForwardVector() * FMath::Min(Sender.PushDistance, CurDistance);
	Forward += GetActorLocation();

	FMotionWarpingTarget WarpingTarget;
	WarpingTarget.Name = FINISHER_TARGET_SYNC_POINT;
	WarpingTarget.Location = Forward;
	WarpingTarget.Rotation = FRotator(0.f, TargetLookAt.Yaw, 0.f);
	MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);

	if (RequireTag == TAG_Weapon_Finisher)
	{
		WvAbilitySystemComponent->TryActivateAbilityByTag(TAG_Weapon_Finisher_Sender);
		TargetCharacter->GetWvAbilitySystemComponent()->TryActivateAbilityByTag(TAG_Weapon_Finisher_Receiver);
	}
	else if (RequireTag == TAG_Weapon_HoldUp)
	{
		WvAbilitySystemComponent->TryActivateAbilityByTag(TAG_Weapon_HoldUp_Sender);
		TargetCharacter->GetWvAbilitySystemComponent()->TryActivateAbilityByTag(TAG_Weapon_HoldUp_Receiver);
	}
	else if (RequireTag == TAG_Weapon_KnockOut)
	{
		WvAbilitySystemComponent->TryActivateAbilityByTag(TAG_Weapon_KnockOut_Sender);
		TargetCharacter->GetWvAbilitySystemComponent()->TryActivateAbilityByTag(TAG_Weapon_KnockOut_Receiver);
	}

}

void ABaseCharacter::BuildFinisherAnimationSender(const FGameplayTag RequireTag, FFinisherAnimation& OutFinisherAnimation, int32 &OutIndex)
{
	if (!IsValid(FinisherSender))
	{
		return;
	}

	FFinisherAnimationContainer AnimationContainer = FinisherSender->FindContainer(RequireTag);
	if (AnimationContainer.Montages.Num() <= 0)
	{
		return;
	}
	const int32 LastIndex = (AnimationContainer.Montages.Num() - 1);
	const int32 Index = FMath::RandRange(0, LastIndex);
	if (AnimationContainer.Montages.IsValidIndex(Index))
	{
		OutIndex = Index;
		OutFinisherAnimation = AnimationContainer.Montages[Index];
	}
}

void ABaseCharacter::BuildFinisherAnimationReceiver(const FGameplayTag RequireTag, const int32 Index, FFinisherAnimation& OutFinisherAnimation)
{
	if (!IsValid(FinisherReceiner))
	{
		return;
	}

	FFinisherAnimationContainer AnimationContainer = FinisherReceiner->FindContainer(RequireTag);
	if (AnimationContainer.Montages.Num() <= 0)
	{
		return;
	}
	OutFinisherAnimation = AnimationContainer.Montages[Index];
}

void ABaseCharacter::BuildFinisherAnimationData(UAnimMontage* InMontage, const bool IsTurnAround, AActor* TurnActor, float PlayRate/* = 1.0f*/)
{
	FinisherAnimationData.AnimMontage = InMontage;
	FinisherAnimationData.PlayRate = PlayRate;
	FinisherAnimationData.IsTurnAround = IsTurnAround;
	FinisherAnimationData.LookAtTarget = TurnActor;
	FinisherAnimationData.TimeToStartMontageAt = 0.f;
}

void ABaseCharacter::ResetFinisherAnimationData()
{
	FinisherAnimationData.AnimMontage = nullptr;
	FinisherAnimationData.PlayRate = 1.0f;
	FinisherAnimationData.LookAtTarget.Reset();
}

FRequestAbilityAnimationData ABaseCharacter::GetFinisherAnimationData() const
{
	return FinisherAnimationData;
}
#pragma endregion

#pragma region AsyncLoad
void ABaseCharacter::RequestAsyncLoad()
{
	OnABPAnimAssetLoad();
	OnFinisherAnimAssetLoad();
}

void ABaseCharacter::OnABPAnimAssetLoad()
{
	if (OverlayAnimInstanceDA.IsValid())
	{
		FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();
		const FSoftObjectPath ObjectPath = OverlayAnimInstanceDA.ToSoftObjectPath();
		ABPStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnABPAnimAssetLoadComplete));
	}
}

void ABaseCharacter::OnABPAnimAssetLoadComplete()
{
	OverlayAnimDAInstance = OverlayAnimInstanceDA.LoadSynchronous();
	ABPStreamableHandle.Reset();
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void ABaseCharacter::OnFinisherAnimAssetLoad()
{
	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();
	TArray<FSoftObjectPath> Paths;
	for (TPair<FGameplayTag, TSoftObjectPtr<UFinisherDataAsset>>Pair : FinisherDAList)
	{
		if (Pair.Value.IsNull())
		{
			continue;
		}
		Paths.Add(Pair.Value.ToSoftObjectPath());
	}
	FinisherStreamableHandle = StreamableManager.RequestAsyncLoad(Paths, FStreamableDelegate::CreateUObject(this, &ThisClass::OnFinisherAnimAssetLoadComplete));
}

void ABaseCharacter::OnFinisherAnimAssetLoadComplete()
{
	OnLoadFinisherSender();
	OnLoadFinisherReceiver();
	FinisherStreamableHandle.Reset();
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void ABaseCharacter::OnLoadFinisherSender()
{
	FinisherSender = FinisherDAList[TAG_Weapon_Finisher_Sender].LoadSynchronous();
}

void ABaseCharacter::OnLoadFinisherReceiver()
{
	FinisherReceiner = FinisherDAList[TAG_Weapon_Finisher_Receiver].LoadSynchronous();
}
#pragma endregion


