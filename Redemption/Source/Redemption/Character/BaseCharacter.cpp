// Copyright 2022 wevet works All Rights Reserved.


#include "BaseCharacter.h"
#include "Redemption.h"
#include "Misc/WvCommonUtils.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Locomotion/LocomotionComponent.h"
#include "PredictionFootIKComponent.h"
#include "Component/InventoryComponent.h"
#include "Component/CombatComponent.h"
#include "Component/StatusComponent.h"
#include "Component/WeaknessComponent.h"
#include "WvPlayerController.h"
#include "WvAIController.h"
#include "Animation/WvAnimInstance.h"
#include "Ability/WvInheritanceAttributeSet.h"
#include "Game/WvGameInstance.h"
#include "Vehicle/WvWheeledVehiclePawn.h"
#include "Item/BulletHoldWeaponActor.h"
#include "GameExtension.h"
#include "Climbing/ClimbingComponent.h"
#include "Climbing/LadderComponent.h"

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
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Prediction.h"
#include "AbilitySystemBlueprintLibrary.h"

// Misc
#include "Engine/SkeletalMeshSocket.h"

#include "WvAIController.h"

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
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWvCharacterMovementComponent>(ACharacter::CharacterMovementComponentName) 
							 .SetDefaultSubobjectClass<UWvSkeletalMeshComponent>(ACharacter::MeshComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	NetCullDistanceSquared = 900000000.0f;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	UWvSkeletalMeshComponent* WvMeshComp = CastChecked<UWvSkeletalMeshComponent>(GetMesh());
	check(WvMeshComp);
	// Rotate mesh to be X forward since it is exported as Y forward.
	WvMeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	WvMeshComp->SetReceivesDecals(true);

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

	// @NOTE
	// always ticking component
	// 1. UCharacterTrajectoryComponent
	// 2. ULocomotionComponent
	// 3. UPawnNoiseEmitterComponent
	// 4. USceneComponent() HeldObjectRoot

	// motion warping
	MotionWarpingComponent = ObjectInitializer.CreateDefaultSubobject<UMotionWarpingComponent>(this, TEXT("MotionWarpingComponent"));
	MotionWarpingComponent->bSearchForWindowsInAnimsWithinMontages = true;

	// motion matching
	CharacterTrajectoryComponent = ObjectInitializer.CreateDefaultSubobject<UCharacterTrajectoryComponent>(this, TEXT("CharacterTrajectoryComponent"));
	CharacterTrajectoryComponent->bAutoActivate = 1;

	// ik prediction component
	PredictionFootIKComponent = ObjectInitializer.CreateDefaultSubobject<UPredictionFootIKComponent>(this, TEXT("PredictionFootIKComponent"));
	PredictionFootIKComponent->bAutoActivate = 1;

	// asc
	WvAbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UWvAbilitySystemComponent>(this, TEXT("WvAbilitySystemComponent"));
	WvAbilitySystemComponent->bAutoActivate = 1;

	// movement managed component
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

	// item attach helper
	HeldObjectRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("HeldObjectRoot"));
	HeldObjectRoot->bAutoActivate = 1;
	HeldObjectRoot->SetupAttachment(WvMeshComp);

	MyTeamID = FGenericTeamId(0);
	CharacterTag = FGameplayTag::RequestGameplayTag(TAG_Character_Default.GetTag().GetTagName());

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Ignore);

	WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	// sets Damage
	WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);
	WvMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// custom collision preset
	WvMeshComp->SetCollisionProfileName(K_CHARACTER_COLLISION_PRESET);
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	AnimInstance = Cast<UWvAnimInstance>(GetMesh()->GetAnimInstance());

	USkeletalMeshComponent* SkelMesh = GetMesh();
	//SkelMesh->PrimaryComponentTick.AddPrerequisite(this, this->PrimaryActorTick);

	CombatComponent->PrimaryComponentTick.AddPrerequisite(this, this->PrimaryActorTick);
	PawnNoiseEmitterComponent->PrimaryComponentTick.AddPrerequisite(this, this->PrimaryActorTick);

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	CMC->OnWallClimbingBeginDelegate.AddDynamic(this, &ABaseCharacter::OnWallClimbingBegin_Callback);
	CMC->OnWallClimbingEndDelegate.AddDynamic(this, &ABaseCharacter::OnWallClimbingEnd_Callback);
	//CMC->AddTickPrerequisiteActor(this);

	LocomotionComponent->OnRotationModeChangeDelegate.AddDynamic(this, &ThisClass::OnRoationChange_Callback);
	LocomotionComponent->OnGaitChangeDelegate.AddDynamic(this, &ThisClass::OnGaitChange_Callback);
	LocomotionComponent->PrimaryComponentTick.AddPrerequisite(this, this->PrimaryActorTick);

	if (AWvAIController* AIC = Cast<AWvAIController>(GetController()))
	{
		AIC->GetMissionComponent()->SetSendMissionData(SendMissionData);
	}
	RequestAsyncLoad();

}

void ABaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.ClearTimer(Ragdoll_TimerHandle);

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	CMC->OnWallClimbingBeginDelegate.RemoveDynamic(this, &ABaseCharacter::OnWallClimbingBegin_Callback);
	CMC->OnWallClimbingEndDelegate.RemoveDynamic(this, &ABaseCharacter::OnWallClimbingEnd_Callback);

	LocomotionComponent->OnRotationModeChangeDelegate.RemoveDynamic(this, &ThisClass::OnRoationChange_Callback);
	LocomotionComponent->OnGaitChangeDelegate.RemoveDynamic(this, &ThisClass::OnGaitChange_Callback);

	WvAbilitySystemComponent->AbilityFailedCallbacks.Remove(AbilityFailedDelegateHandle);

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

	//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	if (IsValid(CMC))
	{
		const float Acc = CMC->GetCurrentAcceleration().Length();
		const float MaxAcc = CMC->GetMaxAcceleration();
		MovementInputAmount = Acc / MaxAcc;
		bHasMovementInput = (MovementInputAmount > 0.0f);

		if (CMC->IsFalling() && bHasMovementInput)
		{
			CMC->FallingMantling();
		}
	}

	//LocomotionComponent->DoTick(DeltaTime);
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

void ABaseCharacter::MoveBlockedBy(const FHitResult& Impact)
{
	//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void ABaseCharacter::InitAbilitySystemComponent()
{
	if (!bIsAbilityInitializeResult)
	{
		bIsAbilityInitializeResult = true;
		WvAbilitySystemComponent->InitAbilityActorInfo(this, this);
		if (GetLocalRole() == ROLE_Authority)
		{
			WvAbilitySystemComponent->AddStartupGameplayAbilities();
		}
	}

	AWvPlayerController* PC = Cast<AWvPlayerController>(GetController());
	if (PC && IsLocallyControlled())
	{
		PC->PostASCInitialize(WvAbilitySystemComponent);
	}

	AbilityFailedDelegateHandle = WvAbilitySystemComponent->AbilityFailedCallbacks.AddUObject(this, &ABaseCharacter::OnAbilityFailed_Callback);
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
	if (UWvCharacterMovementComponent* WvCharacterMovementComponent = CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement()))
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
	//UE_LOG(LogTemp, Log, TEXT("Owner => %s, Actor => %s, function => %s"), *GetPathNameSafe(this), *GetPathNameSafe(Actor), *FString(__FUNCTION__));
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
		if (Weapon->IsAvailable())
		{
			const FGameplayTag TriggerTag = Weapon->GetPluralInputTriggerTag();
			WvAbilitySystemComponent->TryActivateAbilityByTag(TriggerTag);
		}
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

	if (ActionStateChangeDelegate.IsBound())
	{
		ActionStateChangeDelegate.Broadcast(AIActionState, PrevActionState);
	}

	if (IsLeader())
	{
		// no change state color
		return;
	}

	UpdateDisplayTeamColor();
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

UCharacterTrajectoryComponent* ABaseCharacter::GetCharacterTrajectoryComponent() const
{
	return CharacterTrajectoryComponent; 
}

UWvCharacterMovementComponent* ABaseCharacter::GetWvCharacterMovementComponent() const
{
	return CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement());
}

UWvSkeletalMeshComponent* ABaseCharacter::GetWvSkeletalMeshComponent() const
{
	return CastChecked<UWvSkeletalMeshComponent>(GetMesh());
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
					{
						UClimbingComponent* ClimbingComponent = Cast<UClimbingComponent>(GetComponentByClass(UClimbingComponent::StaticClass()));
						if (ClimbingComponent)
						{
							ClimbingComponent->SetJumpInputPressed(true);
						}
					}
					break;
					case CUSTOM_MOVE_WallClimbing:
					{
						// wall climbing when jump action
						CMC->TryClimbJumping();
					}
					break;
					case CUSTOM_MOVE_Mantling:
					{
						//
					}
					break;
					case CUSTOM_MOVE_Ladder:
					{
						ULadderComponent* LadderComponent = Cast<ULadderComponent>(GetComponentByClass(ULadderComponent::StaticClass()));
						if (LadderComponent)
						{
							LadderComponent->SetJumpInputPressed(true);
						}
					}
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
	//StrafeMovement();
	DoWalking();
	LocomotionComponent->SetLSAiming(true);

	if (AimingChangeDelegate.IsBound())
	{
		AimingChangeDelegate.Broadcast(true);
	}
}

void ABaseCharacter::DoStopAiming()
{
	//StrafeMovement();
	DoStopWalking();
	LocomotionComponent->SetLSAiming(false);

	if (AimingChangeDelegate.IsBound())
	{
		AimingChangeDelegate.Broadcast(false);
	}
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

void ABaseCharacter::HandleGuardAction(const bool bGuardEnable, bool& OutResult)
{
	if (bGuardEnable)
	{
		if (!WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_DamageBlock))
		{
			WvAbilitySystemComponent->AddGameplayTag(TAG_Character_DamageBlock, 1);
			OutResult = true;
		}
	}
	else
	{
		if (WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_DamageBlock))
		{
			WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_DamageBlock, 1);
			OutResult = true;
		}
	}
}

const bool ABaseCharacter::HandleAttackPawnPrepare()
{
	const auto Weapon = ItemInventoryComponent->GetEquipWeapon();
	if (Weapon)
	{
		return Weapon->HandleAttackPrepare();
	}
	return false;
}
#pragma endregion

/// <summary>
/// ref BTT_DoCover
/// </summary>
FTransform ABaseCharacter::GetChestTransform(const FName BoneName) const
{
	return GetMesh()->GetSocketTransform(BoneName);
}

/// <summary>
/// overlay widget position
/// </summary>
/// <returns></returns>
FTransform ABaseCharacter::GetPivotOverlayTansform() const
{
	auto RootPos = GetMesh()->GetSocketLocation(TEXT("root"));
	auto HeadPos = GetMesh()->GetSocketLocation(TEXT("head"));
	TArray<FVector> Points({ RootPos, HeadPos, });
	auto AveragePoint = UKismetMathLibrary::GetVectorArrayAverage(Points);
	return FTransform(GetActorRotation(), AveragePoint, FVector::OneVector);
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
	if (IsBotCharacter())
	{

	}
	LocomotionComponent->StartRagdollAction();
}

void ABaseCharacter::BeginAliveAction()
{
	//LocomotionComponent->StopRagdollAction();

	if (IsDead())
	{
		WvAbilitySystemComponent->TryActivateAbilityByTag(TAG_Character_StateAlive_Action);
		if (!bIsCrouched)
		{
			DoStartCrouch();
		}
	}
}

void ABaseCharacter::EndAliveAction()
{
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_StateDead, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Locomotion_ForbidMovement, 1);
	StatusComponent->DoAlive();
}

void ABaseCharacter::DoForceKill()
{
	auto Pawn = Game::ControllerExtension::GetPlayerPawn(GetWorld(), 0);

	const float KillDamage = StatusComponent->GetKillDamage();
	//OnReceiveKillTarget(Pawn, StatusComponent->GetKillDamage());
	//StatusComponent->DoKill();

	FGameplayEffectContextHandle Context;

	FGameplayEventData Payload{};
	Payload.ContextHandle = Context;
	Payload.Instigator = this;
	Payload.Target = Pawn;
	Payload.EventMagnitude = KillDamage;
	WvAbilitySystemComponent->HandleGameplayEvent(TAG_Common_PassiveAbilityTrigger_KillReact, &Payload);
}

void ABaseCharacter::WakeUpPoseSnapShot()
{
	AnimInstance->WakeUpPoseSnapShot();
}

void ABaseCharacter::OverlayStateChange(const ELSOverlayState CurrentOverlay)
{
	if (!IsValid(OverlayAnimDAInstance))
	{
		OnLoadOverlayABP();
	}


	if (IsValid(OverlayAnimDAInstance))
	{
		auto AnimInstanceClass = OverlayAnimDAInstance->FindAnimInstance(CurrentOverlay);
		if (AnimInstanceClass)
		{
			AnimInstance->LinkAnimClassLayers(AnimInstanceClass);
		}

		if (OverlayChangeDelegate.IsBound())
		{
			OverlayChangeDelegate.Broadcast(CurrentOverlay);
		}
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

bool ABaseCharacter::IsLeader() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_AI_Character_Leader);
}

void ABaseCharacter::SetLeaderTag()
{
	WvAbilitySystemComponent->AddGameplayTag(TAG_AI_Character_Leader, 1);
	SetLeaderDisplay();
}

ABaseCharacter* ABaseCharacter::GetLeaderCharacterFromController() const
{
	AWvAIController* AIC = Cast<AWvAIController>(GetController());
	if (IsValid(AIC))
	{
		return Cast<ABaseCharacter>(AIC->GetBlackboardLeader());
	}
	return nullptr;
}

void ABaseCharacter::StartRVOAvoidance()
{
	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	CMC->SetRVOAvoidanceWeight(0.5f);
	CMC->SetAvoidanceEnabled(true);

	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	AvoidanceConsiderationRadius = CMC->AvoidanceConsiderationRadius;
	CMC->AvoidanceConsiderationRadius = FMath::Abs(Radius * 4.0f);
}

void ABaseCharacter::StopRVOAvoidance()
{
	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	CMC->SetRVOAvoidanceWeight(0.0f);
	CMC->SetAvoidanceEnabled(false);

	CMC->AvoidanceConsiderationRadius = AvoidanceConsiderationRadius;
}
#pragma endregion

void ABaseCharacter::DrawActionState()
{
	if (IsDead())
	{
		return;
	}
	const FString CurStateName = *FString::Format(TEXT("{0}"), { *GETENUMSTRING("/Script/WvAbilitySystem.EAIActionState", AIActionState) });
	auto ActorLoc = GetActorLocation();
	ActorLoc.Z += GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	UKismetSystemLibrary::DrawDebugString(GetWorld(), ActorLoc, CurStateName, nullptr, FColor::Red, 0.f);

}

void ABaseCharacter::OnRoationChange_Callback()
{
	//
}

void ABaseCharacter::OnGaitChange_Callback()
{
	const auto EssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();

	float Weight = 0.6f;
	EPredictionGait Gait = EPredictionGait::Run;
	switch (EssencialVariables.LSGait)
	{
	case ELSGait::Walking:
		Gait = EPredictionGait::Walk;
		Weight = 0.2f;
		break;
	case ELSGait::Sprinting:
		Gait = EPredictionGait::Dash;
		Weight = 0.8f;
		break;
	}

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	if (IsValid(CMC))
	{
		const float Acc = CMC->GetCurrentAcceleration().Length();

	}

	PredictionFootIKComponent->ChangeSpeedCurveValue(Gait, Weight, EssencialVariables.Velocity.Size());
}

void ABaseCharacter::OnAbilityFailed_Callback(const UGameplayAbility* Ability, const FGameplayTagContainer& GameplayTags)
{

}

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

float ABaseCharacter::GetHealthToWidget() const
{
	return StatusComponent->GetHealthToWidget();
}

bool ABaseCharacter::IsHealthHalf() const
{
	return StatusComponent->IsHealthHalf();
}

#pragma region NearlestAction
/// <summary>
/// call to melee ability
/// </summary>
/// <param name="SyncPointWeight"></param>
void ABaseCharacter::CalcurateNearlestTarget(const float SyncPointWeight)
{
	const FLocomotionEssencialVariables LocomotionEssencial = LocomotionComponent->GetLocomotionEssencialVariables();
	if (LocomotionEssencial.LookAtTarget.IsValid())
	{
		FindNearlestTarget(LocomotionEssencial.LookAtTarget.Get(), SyncPointWeight);
	}
}

void ABaseCharacter::ResetNearlestTarget()
{
	MotionWarpingComponent->RemoveWarpTarget(NEARLEST_TARGET_SYNC_POINT);
}

void ABaseCharacter::FindNearlestTarget(AActor* Target, const float SyncPointWeight)
{
	const FVector From = GetActorLocation();
	const FVector To = Target->GetActorLocation();
	const float Weight = SyncPointWeight;
	MotionWarpingComponent->RemoveWarpTarget(NEARLEST_TARGET_SYNC_POINT);
	const FRotator TargetLookAt = UKismetMathLibrary::FindLookAtRotation(From, To);

	const FRotator Rotation = UKismetMathLibrary::RLerp(GetActorRotation(), TargetLookAt, Weight, true);
	FMotionWarpingTarget WarpingTarget;
	WarpingTarget.Name = NEARLEST_TARGET_SYNC_POINT;
	WarpingTarget.Location = UKismetMathLibrary::VLerp(From, To, Weight);
	WarpingTarget.Rotation = FRotator(0.f, Rotation.Yaw, 0.f);
	MotionWarpingComponent->AddOrUpdateWarpTarget(WarpingTarget);

	//#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#if false
	DrawDebugSphere(GetWorld(), From, 20.f, 12, FColor::Blue, false, 2);
	DrawDebugSphere(GetWorld(), To, 20.f, 12, FColor::Blue, false, 2);
	DrawDebugDirectionalArrow(GetWorld(), From, To, 20.f, FColor::Red, false, 2);
#endif
}

void ABaseCharacter::FindNearlestTarget(const FAttackMotionWarpingData AttackMotionWarpingData)
{
	auto Target = FindNearlestTarget(AttackMotionWarpingData.NearlestDistance, AttackMotionWarpingData.AngleThreshold, false);
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("not found FindNearlestTarget => %s"), *FString(__FUNCTION__));
		return;
	}
	
	FindNearlestTarget(Target, AttackMotionWarpingData.TargetSyncPointWeight);

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
		AActor* Target = HitTargets[Index];
		if (!IsValid(Target))
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

	HitTargets.RemoveAll([](AActor* Actor)
	{
		return Actor == nullptr;
	});

	// Get the target with the smallest angle difference from the camera forward vector
	float ClosestDotToCenter = 0.f;
	ABaseCharacter* NearlestTarget = nullptr;

	for (int32 Index = 0; Index < HitTargets.Num(); ++Index)
	{
		if (ABaseCharacter* Target = Cast<ABaseCharacter>(HitTargets[Index]))
		{
			if (bTargetCheckBattled && Target->IsInBattled() || Target->IsDead())
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
		OnLoadFinisherAssets();
		return false;
	}
	return true;
}

const bool ABaseCharacter::CanFiniherReceiver()
{
	if (!IsValid(FinisherReceiner))
	{
		OnLoadFinisherAssets();
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
		UE_LOG(LogTemp, Warning, TEXT("not valid FinisherSender"));
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
		UE_LOG(LogTemp, Warning, TEXT("not valid FinisherReceiner"));
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
	FinisherAnimationData.IsTurnAround = IsTurnAround;
	FinisherAnimationData.LookAtTarget = TurnActor;
	FinisherAnimationData.PlayRate = PlayRate;
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
/// <summary>
/// Finisher Assets & ABP Asyncload
/// </summary>
void ABaseCharacter::RequestAsyncLoad()
{
	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();

	if (OverlayAnimInstanceDA.IsValid())
	{
		const FSoftObjectPath ObjectPath = OverlayAnimInstanceDA.ToSoftObjectPath();
		ABPStreamableHandle = StreamableManager.RequestAsyncLoad(ObjectPath, FStreamableDelegate::CreateUObject(this, &ThisClass::OnABPAnimAssetLoadComplete));
	}

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


void ABaseCharacter::OnABPAnimAssetLoadComplete()
{
	OnLoadOverlayABP();
	ABPStreamableHandle.Reset();
}

void ABaseCharacter::OnLoadOverlayABP()
{
	bool bIsResult = false;
	do
	{
		OverlayAnimDAInstance = OverlayAnimInstanceDA.LoadSynchronous();
		bIsResult = (IsValid(OverlayAnimDAInstance));

	} while (!bIsResult);
	UE_LOG(LogTemp, Log, TEXT("Complete OverlayAnimDAInstance => [%s]"), *FString(__FUNCTION__));
}

void ABaseCharacter::OnFinisherAnimAssetLoadComplete()
{
	OnLoadFinisherAssets();
	FinisherStreamableHandle.Reset();
}

void ABaseCharacter::OnLoadFinisherAssets()
{
	if (!IsValid(FinisherSender))
	{
		if (FinisherDAList.Contains(TAG_Weapon_Finisher_Sender))
		{
			bool bIsResult = false;
			do
			{
				FinisherSender = FinisherDAList[TAG_Weapon_Finisher_Sender].LoadSynchronous();
				bIsResult = (IsValid(FinisherSender));

			} while (!bIsResult);
			UE_LOG(LogTemp, Log, TEXT("Complete FinisherSender => [%s]"), *FString(__FUNCTION__));
		}
	}

	if (!IsValid(FinisherReceiner))
	{
		if (FinisherDAList.Contains(TAG_Weapon_Finisher_Receiver))
		{
			bool bIsResult = false;
			do
			{
				FinisherReceiner = FinisherDAList[TAG_Weapon_Finisher_Receiver].LoadSynchronous();
				bIsResult = (IsValid(FinisherReceiner));

			} while (!bIsResult);
			UE_LOG(LogTemp, Log, TEXT("Complete FinisherReceiner => [%s]"), *FString(__FUNCTION__));
		}
	}

}
#pragma endregion

#pragma region VehicleAction
bool ABaseCharacter::IsVehicleDriving() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Vehicle_State_Drive);
}

void ABaseCharacter::BeginVehicleAction()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetVisibility(false);

	GetCombatComponent()->VisibilityCurrentWeapon(true);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Vehicle_State_Drive, 1);
}

void ABaseCharacter::EndVehicleAction()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetVisibility(true);

	GetCombatComponent()->VisibilityCurrentWeapon(false);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Vehicle_State_Drive, 1);

}

void ABaseCharacter::HandleDriveAction()
{
	if (IsVehicleDriving())
	{
		return;
	}

	const FVehicleTraceConfig VehicleTraceConfig = ASC_GLOBAL()->VehicleTraceConfig;
	const float ClosestTargetDistance = VehicleTraceConfig.ClosestTargetDistance;
	const float NearestDistance = VehicleTraceConfig.NearestDistance;
	const FVector2D ViewRange = VehicleTraceConfig.ViewRange;

	TArray<AActor*> IgnoreActors({ this });
	TArray<AActor*> OutActors;

	// debug
	//UKismetSystemLibrary::DrawDebugSphere(GetWorld(), GetActorLocation(), ClosestTargetDistance, 12, FLinearColor::Blue, 4.0f, 1.0f);

	const bool bHitResult = UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetActorLocation(), ClosestTargetDistance,
		ASC_GLOBAL()->VehicleTraceChannel, AWvWheeledVehiclePawn::StaticClass(), IgnoreActors, OutActors);

	const AWvWheeledVehiclePawn* VehicleTarget = Cast<AWvWheeledVehiclePawn>(UWvCommonUtils::FindNearestDistanceTarget(this, OutActors, NearestDistance));

	if (!IsValid(VehicleTarget))
	{
		return;
	}

	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(VehicleTarget->GetActorRotation(), GetActorRotation());

	const FVector NormalizePos = (VehicleTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FVector Forward = GetActorForwardVector();
	const float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct(Forward, NormalizePos));
	const float Yaw = FMath::Abs(DeltaRot.Yaw);

	UE_LOG(LogTemp, Log, TEXT("Angle => %.3f"), Angle);
	UE_LOG(LogTemp, Log, TEXT("Yaw => %.3f"), Yaw);

	if (ViewRange.X <= Yaw && ViewRange.Y >= Yaw)
	{
		UE_LOG(LogTemp, Log, TEXT("DriveIn => %s"), *FString(__FUNCTION__));
		FGameplayEventData EventData;
		EventData.Instigator = this;
		EventData.Target = VehicleTarget;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, TAG_Vehicle_Drive, EventData);
	}
}
#pragma endregion

void ABaseCharacter::BeginCinematic()
{
	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	if (IsValid(CMC))
	{
		CMC->bUpdateOnlyIfRendered = false;
	}
}

void ABaseCharacter::EndCinematic()
{
	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	if (IsValid(CMC))
	{
		CMC->bUpdateOnlyIfRendered = true;
	}
}

void ABaseCharacter::SetDayNightPhase(const uint8 InDayNightPhase)
{
	DayNightPhase = InDayNightPhase;
}

uint8 ABaseCharacter::GetDayNightPhase() const
{
	return DayNightPhase;
}


