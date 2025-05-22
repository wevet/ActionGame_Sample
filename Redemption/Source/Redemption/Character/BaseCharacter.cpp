// Copyright 2022 wevet works All Rights Reserved.


#include "BaseCharacter.h"
#include "Redemption.h"
#include "Misc/WvCommonUtils.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/CharacterMovementHelperComponent.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Component/InventoryComponent.h"
#include "Component/CombatComponent.h"
#include "Component/StatusComponent.h"
#include "Component/WeaknessComponent.h"
#include "Component/TrailInteractionComponent.h"
#include "Animation/WvAnimInstance.h"
#include "Animation/WvFaceAnimInstance.h"
#include "Mission/MinimapMarkerComponent.h"
#include "Locomotion/LocomotionComponent.h"
#include "WvPlayerController.h"
#include "WvAIController.h"
#include "Game/WvGameInstance.h"
#include "Vehicle/WvWheeledVehiclePawn.h"
#include "Item/BulletHoldWeaponActor.h"
#include "GameExtension.h"
#include "Climbing/ClimbingComponent.h"
#include "Climbing/LadderComponent.h"
#include "Game/CharacterInstanceSubsystem.h"
#include "Level/FieldInstanceSubsystem.h"
#include "Significance/SignificanceComponent.h"

// plugin
#include "WvFoleyAssetTypes.h"
#include "PredictionFootIKComponent.h"
#include "Ability/WvInheritanceAttributeSet.h"
#include "WvAbilitySystemBlueprintFunctionLibrary.h"


#include "Components/LODSyncComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "MotionWarpingComponent.h"
#include "AI/Navigation/NavigationTypes.h"
#include "Net/UnrealNetwork.h"

#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UObjectBaseUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Prediction.h"
#include "AbilitySystemBlueprintLibrary.h"

// Misc
#include "Engine/SkeletalMeshSocket.h"
#include "Runtime/Launch/Resources/Version.h"
#include "IAnimationBudgetAllocator.h"
#include "SignificanceManager.h"
#include "Algo/Transform.h"
#include "ChooserFunctionLibrary.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(BaseCharacter)



namespace CharacterDebug
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	TAutoConsoleVariable<int32> CVarDebugCharacterStatus(TEXT("wv.CharacterStatus.Debug"), 0, TEXT("CharacterStatus Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarDebugFallEdgeSystem(TEXT("wv.FallEdgeSystem.Debug"), 0, TEXT("FallEdgeSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDebugMantlingSystem(TEXT("wv.MantlingSystem.Debug"), 0, TEXT("MantlingSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDebugWallClimbingSystem(TEXT("wv.WallClimbingSystem.Debug"), 0, TEXT("WallClimbingSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);

	TAutoConsoleVariable<int32> CVarDebugVaultingSystem(TEXT("wv.VaultingSystem.Debug"), 0, TEXT("VaultingSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDebugCombatSystem(TEXT("wv.CombatSystem.Debug"), 0, TEXT("CombatSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDebugLadderSystem(TEXT("wv.LadderSystem.Debug"), 0, TEXT("LadderSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
	TAutoConsoleVariable<int32> CVarDebugClimbingSystem(TEXT("wv.ClimbingSystem.Debug"), 0, TEXT("ClimbingSystem Debug .\n") TEXT("<=0: off\n") TEXT("  1: on\n"), ECVF_Default);
#endif
}



ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWvCharacterMovementComponent>(ACharacter::CharacterMovementComponentName) 
							 .SetDefaultSubobjectClass<UWvSkeletalMeshComponent>(ACharacter::MeshComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	SetNetCullDistanceSquared(900000000.0f);

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

	// face sk mesh
	Face = ObjectInitializer.CreateDefaultSubobject<UWvSkeletalMeshComponent>(this, TEXT("Face"));
	Face->SetupAttachment(WvMeshComp);
	Face->SetReceivesDecals(true);

	Body = ObjectInitializer.CreateDefaultSubobject<UWvSkeletalMeshComponent>(this, TEXT("Body"));
	Body->SetupAttachment(WvMeshComp);
	Body->SetReceivesDecals(true);

	Bottom = ObjectInitializer.CreateDefaultSubobject<UWvSkeletalMeshComponent>(this, TEXT("Bottom"));
	Bottom->SetupAttachment(WvMeshComp);
	Bottom->SetReceivesDecals(true);

	Top = ObjectInitializer.CreateDefaultSubobject<UWvSkeletalMeshComponent>(this, TEXT("Top"));
	Top->SetupAttachment(WvMeshComp);
	Top->SetReceivesDecals(true);

	Feet = ObjectInitializer.CreateDefaultSubobject<UWvSkeletalMeshComponent>(this, TEXT("Feet"));
	Feet->SetupAttachment(WvMeshComp);
	Feet->SetReceivesDecals(true);

	Other1 = ObjectInitializer.CreateDefaultSubobject<UWvSkeletalMeshComponent>(this, TEXT("Other1"));
	Other1->SetupAttachment(WvMeshComp);
	Other1->SetReceivesDecals(true);


	UWvCharacterMovementComponent* WvMoveComp = CastChecked<UWvCharacterMovementComponent>(GetCharacterMovement());
	WvMoveComp->GravityScale = 1.0f;
	WvMoveComp->BrakingFrictionFactor = 1.0f;

	WvMoveComp->bUseControllerDesiredRotation = false;
	WvMoveComp->bOrientRotationToMovement = true;
	WvMoveComp->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
	WvMoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	WvMoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	WvMoveComp->bCanWalkOffLedgesWhenCrouching = true;
	WvMoveComp->SetCrouchedHalfHeight(55.0f);
	WvMoveComp->JumpZVelocity = 500.f;
	WvMoveComp->AirControl = 0.35f;

	// set motion matching parameters
	WvMoveComp->BrakingDecelerationWalking = 1500.0f;
	WvMoveComp->GroundFriction = 5.0f;
	WvMoveComp->MinAnalogWalkSpeed = 150.f;
	WvMoveComp->MaxWalkSpeed = 500.0f;
	WvMoveComp->bIgnoreBaseRotation = false;
	WvMoveComp->MaxAcceleration = 800.0f;
	WvMoveComp->BrakingFriction = 0.0f;
	WvMoveComp->bUseSeparateBrakingFriction = true;

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

	// custom climbing
	ClimbingComponent = ObjectInitializer.CreateDefaultSubobject<UClimbingComponent>(this, TEXT("ClimbingComponent"));
	ClimbingComponent->bAutoActivate = 1;

	// lod sync
	LODSyncComponent = ObjectInitializer.CreateDefaultSubobject<ULODSyncComponent>(this, TEXT("LODSyncComponent"));
	LODSyncComponent->bAutoActivate = 1;
	LODSyncComponent->NumLODs = 8;

	// item attach helper
	HeldObjectRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("HeldObjectRoot"));
	HeldObjectRoot->bAutoActivate = 1;
	HeldObjectRoot->SetupAttachment(WvMeshComp);

	AccessoryObjectRoot = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("AccessoryObjectRoot"));
	AccessoryObjectRoot->bAutoActivate = 1;
	AccessoryObjectRoot->SetupAttachment(WvMeshComp);

	CharacterMovementHelperComponent = ObjectInitializer.CreateDefaultSubobject<UCharacterMovementHelperComponent>(this, TEXT("CharacterMovementHelperComponent"));
	CharacterMovementHelperComponent->bAutoActivate = 1;

	SignificanceComponent = ObjectInitializer.CreateDefaultSubobject<USignificanceComponent>(this, TEXT("SignificanceComponent"));
	SignificanceComponent->bAutoActivate = 1;

	MinimapMarkerComponent = ObjectInitializer.CreateDefaultSubobject<UMinimapMarkerComponent>(this, TEXT("MinimapMarkerComponent"));
	MinimapMarkerComponent->bAutoActivate = 1;

	MyTeamID = FGenericTeamId(0);
	CharacterTag = FGameplayTag::RequestGameplayTag(TAG_Character_Default.GetTag().GetTagName());

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Ignore);

	WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Block);
	WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	// sets Damage
	WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);
	WvMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// custom collision preset
	WvMeshComp->SetCollisionProfileName(K_CHARACTER_COLLISION_PRESET);


	Face->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Block);
	Face->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	Face->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	Face->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
	// sets Damage
	Face->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);

	Face->SetCollisionProfileName(K_CHARACTER_COLLISION_PRESET);
	Face->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	Body->SetCollisionProfileName(K_CHARACTER_COLLISION_PRESET);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Bottom->SetCollisionProfileName(K_CHARACTER_COLLISION_PRESET);
	Bottom->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Top->SetCollisionProfileName(K_CHARACTER_COLLISION_PRESET);
	Top->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Feet->SetCollisionProfileName(K_CHARACTER_COLLISION_PRESET);
	Feet->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Other1->SetCollisionProfileName(K_CHARACTER_COLLISION_PRESET);
	Other1->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WvMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Face->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Bottom->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Top->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Feet->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
}

void ABaseCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// cache transform
	USkeletalMeshComponent* SkelMesh = GetMesh();
	InitSkelMeshTransform.SetLocation(SkelMesh->GetRelativeLocation());
	InitSkelMeshTransform.SetRotation(FQuat(SkelMesh->GetRelativeRotation()));
	InitSkelMeshTransform.SetScale3D(SkelMesh->GetRelativeScale3D());
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	AnimInstance = Cast<UWvAnimInstance>(GetMesh()->GetAnimInstance());
	FaceAnimInstance = Cast<UWvFaceAnimInstance>(Face->GetAnimInstance());

	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->AddTickPrerequisiteActor(this);
	Face->AddTickPrerequisiteActor(this);

	CombatComponent->AddTickPrerequisiteActor(this);
	PawnNoiseEmitterComponent->AddTickPrerequisiteActor(this);
	ItemInventoryComponent->AddTickPrerequisiteActor(this);
	ClimbingComponent->AddTickPrerequisiteActor(this);
	PredictionFootIKComponent->AddTickPrerequisiteActor(this);
	HeldObjectRoot->AddTickPrerequisiteActor(this);
	WeaknessComponent->AddTickPrerequisiteActor(this);
	StatusComponent->AddTickPrerequisiteActor(this);
	MotionWarpingComponent->AddTickPrerequisiteActor(this);
	CharacterTrajectoryComponent->AddTickPrerequisiteActor(this);


	if (!IsValid(Other1->GetSkeletalMeshAsset()))
	{
		Other1->SetVisibility(false);
	}

	WEVET_COMMENT("Must Async Function CMC")
	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	CMC->OnTraversalBeginDelegate.AddDynamic(this, &ThisClass::OnTraversalBegin_Callback);
	CMC->OnTraversalEndDelegate.AddDynamic(this, &ThisClass::OnTraversalEnd_Callback);

	CMC->AddTickPrerequisiteComponent(CharacterMovementHelperComponent);

	LocomotionComponent->OnRotationModeChangeDelegate.AddDynamic(this, &ThisClass::OnRoationChange_Callback);
	LocomotionComponent->OnGaitChangeDelegate.AddDynamic(this, &ThisClass::OnGaitChange_Callback);
	LocomotionComponent->AddTickPrerequisiteActor(this);

	if (AWvAIController* AIC = Cast<AWvAIController>(GetController()))
	{
		AIC->GetMissionComponent()->SetSendMissionData(SendMissionIndex);
	}

	CMC->MaxStepHeight = GetDistanceFromToeToKnee();
	CMC->SetWalkableFloorAngle(K_WALKABLE_FLOOR_ANGLE);

	BuildOptimization();
	RequestAsyncLoad();

	HairStrandsLODSetUp();

	WEVET_COMMENT("CharacterInstanceSubsystem API")
	UCharacterInstanceSubsystem::Get()->AssignAICharacter(this);
}

void ABaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	WEVET_COMMENT("CharacterInstanceSubsystem API")
	UCharacterInstanceSubsystem::Get()->RemoveAICharacter(this);

	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.ClearTimer(Ragdoll_TimerHandle);

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	CMC->OnTraversalBeginDelegate.RemoveDynamic(this, &ThisClass::OnTraversalBegin_Callback);
	CMC->OnTraversalEndDelegate.RemoveDynamic(this, &ThisClass::OnTraversalEnd_Callback);

	LocomotionComponent->OnRotationModeChangeDelegate.RemoveDynamic(this, &ThisClass::OnRoationChange_Callback);
	LocomotionComponent->OnGaitChangeDelegate.RemoveDynamic(this, &ThisClass::OnGaitChange_Callback);

	WvAbilitySystemComponent->AbilityFailedCallbacks.Remove(AbilityFailedDelegateHandle);

	ResetFinisherAnimationData();

	CloseCombatDA = nullptr;
	FinisherSenderDA = nullptr;
	FinisherReceinerDA = nullptr;
	HitReactionDA = nullptr;
	CharacterVFXDA = nullptr;

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

	if (!Super::WasRecentlyRendered(0.2f))
	{
		return;
	}

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	if (IsValid(CMC))
	{
		const float Acc = CMC->GetCurrentAcceleration().Length();
		const float MaxAcc = CMC->GetMaxAcceleration();
		MovementInputAmount = Acc / MaxAcc;
		bHasMovementInput = (MovementInputAmount > 0.0f);

	}

	// Stopped due to adverse effect on ragdoll animation
#if false
	if (LocomotionComponent->IsInRagdolling())
	{
		LocomotionComponent->DoWhileRagdolling();
	}
#endif

	DrawDebug();

}


#if WITH_EDITOR
void ABaseCharacter::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	static const FName SpatiallyLoadedName = GET_MEMBER_NAME_CHECKED(ABaseCharacter, bIsSpatiallyLoaded);
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == SpatiallyLoadedName)
	{
		//bIsSpatiallyLoaded = false;
	}
}
#endif


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

	AbilityFailedDelegateHandle = WvAbilitySystemComponent->AbilityFailedCallbacks.AddUObject(this, &ThisClass::OnAbilityFailed_Callback);
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

	DOREPLIFETIME_CONDITION(ThisClass, TraversalActionData, COND_SimulatedOnly);
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

void ABaseCharacter::OnRep_TraversalActionData()
{
}

void ABaseCharacter::Traversal_Server_Implementation(const FTraversalActionData& TraversalActionDataRep)
{
	TraversalActionData = TraversalActionDataRep;

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	CMC->OnTraversalStart();
}

void ABaseCharacter::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

/// <summary>
/// change to team id
/// </summary>
/// <param name="TeamAgent"></param>
/// <param name="OldTeam"></param>
/// <param name="NewTeam"></param>
void ABaseCharacter::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	const FGenericTeamId OldTeamID = MyTeamID;
	MyTeamID = IntegerToGenericTeamId(NewTeam);
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
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

class UBehaviorTree* ABaseCharacter::GetBehaviorTree() const
{
	return BehaviorTree;
}

class UWvHitReactDataAsset* ABaseCharacter::GetHitReactDataAsset() const
{
	return HitReactionDA;
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

FName ABaseCharacter::GetAvatarName() const
{
	return NAME_None;
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
			UE_LOG(LogTemp, Error, TEXT("You can't set the team ID on a character (%s) except on the authority"), *GetNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("You can't set the team ID on a possessed character (%s); it's driven by the associated controller"), *GetNameSafe(this));
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
	OnTeamWeaknessHandleAttackDelegate.Broadcast(Actor, WeaknessName, Damage);
}

void ABaseCharacter::OnSendAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
	OnTeamHandleAttackDelegate.Broadcast(Actor, SourceInfo, Damage);
}

void ABaseCharacter::OnSendKillTarget(AActor* Actor, const float Damage)
{
	if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(GetController()))
	{
		Interface->OnSendKillTarget(Actor, Damage);
	}
	OnTeamHandleSendKillDelegate.Broadcast(Actor, Damage);
}

void ABaseCharacter::OnReceiveWeaknessAttack(AActor* Actor, const FName WeaknessName, const float Damage)
{
	OnTeamWeaknessHandleReceiveDelegate.Broadcast(Actor, WeaknessName, Damage);
}

void ABaseCharacter::OnReceiveAbilityAttack(AActor* Actor, const FWvBattleDamageAttackSourceInfo SourceInfo, const float Damage)
{
	if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(GetController()))
	{
		Interface->OnReceiveAbilityAttack(Actor, SourceInfo, Damage);
	}
	OnTeamHandleReceiveDelegate.Broadcast(Actor, SourceInfo, Damage);
}

void ABaseCharacter::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
	if (IWvAbilityTargetInterface* Interface = Cast<IWvAbilityTargetInterface>(GetController()))
	{
		Interface->OnReceiveKillTarget(Actor, Damage);
	}

	if (bIsDiedRemoveInventory)
	{
		GetInventoryComponent()->RemoveAllInventory();
	}

	OnTeamHandleReceiveKillDelegate.Broadcast(Actor, Damage);

	OnReceiveKillTarget_Callback();
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
	WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidTraversal, 1);

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
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Locomotion_ForbidTraversal, 1);

	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionMelee_Forbid, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionJump_Forbid, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionDash_Forbid, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionCrouch_Forbid, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_TargetLock_Forbid, 1);
}

bool ABaseCharacter::IsFreezing() const
{
	if (WvAbilitySystemComponent)
	{
		FGameplayTagContainer Container;
		Container.AddTag(TAG_Locomotion_ForbidMovement);
		Container.AddTag(TAG_Locomotion_ForbidClimbing);
		Container.AddTag(TAG_Locomotion_ForbidMantling);
		Container.AddTag(TAG_Locomotion_ForbidJump);
		Container.AddTag(TAG_Locomotion_ForbidTraversal);

		Container.AddTag(TAG_Character_ActionMelee_Forbid);
		Container.AddTag(TAG_Character_ActionJump_Forbid);
		Container.AddTag(TAG_Character_ActionDash_Forbid);
		Container.AddTag(TAG_Character_ActionCrouch_Forbid);
		Container.AddTag(TAG_Character_TargetLock_Forbid);

		return WvAbilitySystemComponent->HasAllMatchingGameplayTags(Container);
	}
	return false;
}

bool ABaseCharacter::IsSprintingMovement() const
{
	const FGameplayTag SprintTag = FGameplayTag::RequestGameplayTag(TEXT("Character.Locomotion.Gait.Sprinting"));
	//UE_LOG(LogTemp, Log, TEXT("IsSprinting => %s"), WvAbilitySystemComponent->HasMatchingGameplayTag(SprintTag) ? TEXT("true") : TEXT("false"));
	const bool bResult = IsValid(WvAbilitySystemComponent) ? WvAbilitySystemComponent->HasMatchingGameplayTag(SprintTag) : false;
	return bResult || LocomotionComponent->GetLSGaitMode_Implementation() == ELSGait::Sprinting;
}

/// <summary>
/// Default Melee Attack
/// </summary>
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

/// <summary>
/// Pistol or Rifle Attack
/// </summary>
void ABaseCharacter::DoBulletAttack()
{
	if (WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_ActionMelee_Forbid))
	{
		UE_LOG(LogTemp, Warning, TEXT("has tag TAG_Character_StateMelee_Forbid => %s"), *FString(__FUNCTION__));
		return;
	}

	if (ItemInventoryComponent->CanAimingWeapon())
	{
		const auto Weapon = ItemInventoryComponent->GetEquipWeapon();
		if (Weapon && Weapon->IsAvailable())
		{
			const FGameplayTag TriggerTag = Weapon->GetPluralInputTriggerTag();
			WvAbilitySystemComponent->TryActivateAbilityByTag(TriggerTag);
		}
	}
}

/// <summary>
/// Bomb Attack
/// </summary>
void ABaseCharacter::DoThrowAttack()
{
	if (WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_ActionMelee_Forbid))
	{
		UE_LOG(LogTemp, Warning, TEXT("has tag TAG_Character_StateMelee_Forbid => %s"), *FString(__FUNCTION__));
		return;
	}

	if (ItemInventoryComponent->CanThrowableWeapon())
	{
		const auto Weapon = ItemInventoryComponent->GetEquipWeapon();
		if (Weapon && Weapon->IsAvailable())
		{
			const FGameplayTag TriggerTag = Weapon->GetPluralInputTriggerTag();
			WvAbilitySystemComponent->TryActivateAbilityByTag(TriggerTag);
		}
	}
}

bool ABaseCharacter::IsAttackAllowed() const
{
	return !WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_AI_NotAllowed_Attack);
}

/// <summary>
/// @TODO
/// must change to child mesh component URO disable
/// </summary>
void ABaseCharacter::DoStartCinematic()
{
	Freeze();

	auto CMC = GetWvCharacterMovementComponent();
	if (CMC)
	{
		CMC->SetMovementMode(EMovementMode::MOVE_None);
	}

	HandleMeshUpdateRateOptimizations(false, GetMesh());

	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_Action_Cinematic, 1);
}

/// <summary>
/// @TODO
/// must change to child mesh component URO enable
/// </summary>
void ABaseCharacter::DoStopCinematic()
{
	UnFreeze();

	auto CMC = GetWvCharacterMovementComponent();
	if (CMC)
	{
		CMC->SetMovementMode(IsBotCharacter() ? EMovementMode::MOVE_NavWalking : EMovementMode::MOVE_Walking);
	}

	HandleMeshUpdateRateOptimizations(true, GetMesh());

	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_Action_Cinematic, 1);
}

bool ABaseCharacter::IsCinematic() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_Action_Cinematic);
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

	ActionStateChangeDelegate.Broadcast(AIActionState, PrevActionState);
}

EAIActionState ABaseCharacter::GetAIActionState() const
{
	return AIActionState;
}
#pragma endregion


const int32 ABaseCharacter::SIGNIFICANCE_LEVEL_LOWEST(5);

void ABaseCharacter::OnSignificanceLevelChanged_Implementation(int32 SignificanceLevel)
{

	if (ManuallySignificanceLevel != -1)
	{
		// SignificanceLevel が手動で設定されている場合は、設定値が使用される。
		SignificanceLevel = ManuallySignificanceLevel;
	}
	else if ((Controller != nullptr && Controller->IsLocalPlayerController()))
	{
		// シーケンサが操作するキャラクターやプレイヤーが操作するキャラクターはレベル 0 に固定
		SignificanceLevel = 0;
	}


	// 最大値を超えてはならない
	if (SignificanceLevel > MaxSignificanceLevel)
	{
		SignificanceLevel = MaxSignificanceLevel;
	}

	// 変更があった場合のみ更新
	if (SignificanceComponent->SignificanceLevel != SignificanceLevel)
	{
		int32 LastSingificanceLevel = SignificanceComponent->SignificanceLevel;
		SignificanceComponent->SignificanceLevel = SignificanceLevel;

		const FSignificanceConfigType2& SignificanceConfig = SignificanceComponent->SignificanceConfigArray[SignificanceComponent->SignificanceLevel];

		TArray<UActorComponent*> IgnoreComponents;
		IgnoreComponents.Add(GetMesh());

		UWvCommonUtils::SetActorAndComponentTickIntervalWithIgnore(this, SignificanceConfig.TickInterval, IgnoreComponents);
		UWvCommonUtils::UpdateMinLODBySignificanceLevel(this, SignificanceConfig.MinLOD);
		UWvCommonUtils::SetAIControllerTickInterval(this, SignificanceConfig.TickInterval);

		int32 FilterLevel = SignificanceComponent->Significance_CharacterBaseAIUnlockLevel;
		if (LastSingificanceLevel <= FilterLevel && SignificanceLevel > FilterLevel)
		{
			// avoid repeatedly set
			//TemporaryLockAIOverstack(true, true);
		}
		else if (LastSingificanceLevel > FilterLevel && SignificanceLevel <= FilterLevel)
		{
			//TemporaryLockAIOverstack(false, true);
		}

		FilterLevel = SignificanceComponent->Significance_CharacterBaseMeshVisibleLevel;
		if (LastSingificanceLevel <= FilterLevel && SignificanceLevel > FilterLevel)
		{
			// avoid repeatedly set
			GetMesh()->SetVisibility(false, true);
		}
		else if (LastSingificanceLevel > FilterLevel && SignificanceLevel <= FilterLevel)
		{
			GetMesh()->SetVisibility(true, true);
		}

		SetMeshBudgetLevel((float)(SIGNIFICANCE_LEVEL_LOWEST - SignificanceLevel));
	}

}

void ABaseCharacter::GetSignificanceBounds_Implementation(FVector& Origin, FVector& BoxExtent, float& SphereRadius)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp == nullptr)
	{
		Origin = GetActorLocation();
		UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
		float ScaledCapsuleRadius = CapsuleComp->GetScaledCapsuleRadius();
		BoxExtent.X = ScaledCapsuleRadius;
		BoxExtent.Y = ScaledCapsuleRadius;
		BoxExtent.Z = CapsuleComp->GetScaledCapsuleHalfHeight();
		SphereRadius = ScaledCapsuleRadius;
	}
	else
	{
		UKismetSystemLibrary::GetComponentBounds(MeshComp, Origin, BoxExtent, SphereRadius);
	}
}

void ABaseCharacter::SetMeshBudgetLevel(const float NewLevel)
{
	if (IAnimationBudgetAllocator* AnimationBudgetAllocator = IAnimationBudgetAllocator::Get(GetWorld()))
	{
		if (AnimationBudgetAllocator->GetEnabled())
		{
			AnimationBudgetAllocator->SetComponentSignificance(CastChecked<UWvSkeletalMeshComponent>(GetMesh()), NewLevel, NewLevel > 4.9f);
		}
	}
}

#pragma region Components
UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return WvAbilitySystemComponent;
}

UMotionWarpingComponent* ABaseCharacter::GetMotionWarpingComponent() const
{
	return MotionWarpingComponent; 
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

UClimbingComponent* ABaseCharacter::GetClimbingComponent() const
{
	return ClimbingComponent;
}

USceneComponent* ABaseCharacter::GetHeldObjectRoot() const
{
	return HeldObjectRoot;
}

UStaticMeshComponent* ABaseCharacter::GetAccessoryObjectRoot() const
{
	return AccessoryObjectRoot;
}

USkeletalMeshComponent* ABaseCharacter::GetFaceMeshComponent() const
{
	return Face;
}

UMinimapMarkerComponent* ABaseCharacter::GetMinimapMarkerComponent() const
{
	return MinimapMarkerComponent;
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
				CMC->SetTraversalPressed(true);
				if (!CMC->TryTraversalAction())
				{
					Super::Jump();
				}

			}
			break;
			case MOVE_Falling:
			{
				CMC->SetTraversalPressed(true);
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
					{
						//if (ClimbingComponent)
						//{
						//	ClimbingComponent->SetJumpInputPressed(true);
						//}
					}
					break;
					case CUSTOM_MOVE_Mantling:
					{
						//
					}
					case CUSTOM_MOVE_Traversal:
					{
						//
					}
					break;
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

		const auto EssencialVariables = LocomotionComponent->GetLocomotionEssencialVariables();
		switch (EssencialVariables.LSMovementMode)
		{
			case ELSMovementMode::Grounded:
			case ELSMovementMode::Falling:
			case ELSMovementMode::Climbing:
			{
				ClimbingComponent->SetJumpInputPressed(true);
			}
			break;
		}
	}

}

void ABaseCharacter::StopJumping()
{
	Super::StopJumping();

	ClimbingComponent->SetJumpInputPressed(false);

	UWvCharacterMovementComponent* CMC = GetWvCharacterMovementComponent();
	if (CMC)
	{
		CMC->SetTraversalPressed(false);
	}

	ULadderComponent* LadderComponent = Cast<ULadderComponent>(GetComponentByClass(ULadderComponent::StaticClass()));
	if (LadderComponent)
	{
		LadderComponent->SetJumpInputPressed(false);
	}

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
	auto LocomotionVariables = LocomotionComponent->GetLocomotionEssencialVariables();

	if (LocomotionVariables.bAiming)
	{
		DoStopAiming();
	}

	LocomotionComponent->SetSprintPressed(true);

	//AbortClimbing();
	//AbortLaddering();
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
	LocomotionComponent->SetLSAiming(true);
	AimingChangeDelegate.Broadcast(true);
}

void ABaseCharacter::DoStopAiming()
{
	//StrafeMovement();
	LocomotionComponent->SetLSAiming(false);
	AimingChangeDelegate.Broadcast(false);
}

void ABaseCharacter::DoTargetLockOn()
{
	if (IsValid(WvAbilitySystemComponent))
	{
		WvAbilitySystemComponent->AddGameplayTag(TAG_Character_TargetLocking, 1);
	}
}

void ABaseCharacter::DoTargetLockOff()
{
	if (IsValid(WvAbilitySystemComponent))
	{
		WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_TargetLocking, 1);
	}
}

void ABaseCharacter::OnTraversalBegin_Callback()
{
	UE_LOG(LogTemp, Log, TEXT("Character => %s, function => %s"), *GetName(), *FString(__FUNCTION__));
}

void ABaseCharacter::OnTraversalEnd_Callback()
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

void ABaseCharacter::AbortClimbing()
{
	auto CMC = GetWvCharacterMovementComponent();
	if (CMC)
	{
		if (CMC->IsClimbing())
		{
			ClimbingComponent->ApplyStopClimbingInput(0.3f, false);
		}
	}
}

void ABaseCharacter::AbortLaddering()
{
	auto CMC = GetWvCharacterMovementComponent();
	if (CMC && CMC->IsLaddering())
	{

	}

	ULadderComponent* LadderComponent = Cast<ULadderComponent>(GetComponentByClass(ULadderComponent::StaticClass()));
	if (LadderComponent && LadderComponent->IsLadderState())
	{
		LadderComponent->ExitLadderApply();
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

#pragma region Ragdoll
void ABaseCharacter::Test_StartRagdoll()
{
	DoForceKill();
}

void ABaseCharacter::Test_StopRagdoll()
{
	BeginAliveAction();
}

void ABaseCharacter::BeginDeathAction()
{
	if (!IsDead())
	{
		WvAbilitySystemComponent->AddGameplayTag(TAG_Character_StateDead, 1);
		WvAbilitySystemComponent->AddGameplayTag(TAG_Locomotion_ForbidMovement, 1);

		Face->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ABaseCharacter::EndDeathAction(const float Interval)
{
	FTimerManager& TM = GetWorld()->GetTimerManager();
	if (TM.IsTimerActive(Ragdoll_TimerHandle))
	{
		TM.ClearTimer(Ragdoll_TimerHandle);
	}

	TM.SetTimer(Ragdoll_TimerHandle, [this] 
	{
		CancelAnimatingAbility();
		LocomotionComponent->StartRagdollAction();
	}, 
	Interval, false);
}

void ABaseCharacter::BeginAliveAction()
{
	if (IsDead())
	{

		StatusComponent->DoAlive();
		LocomotionComponent->StopRagdollAction([this]() 
		{
			//
		});

		WvAbilitySystemComponent->TryActivateAbilityByTag(TAG_Character_StateAlive_Action);
	}
}

void ABaseCharacter::EndAliveAction()
{
	Face->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetGroomSimulation(true);

	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_StateDead, 1);
	WvAbilitySystemComponent->RemoveGameplayTag(TAG_Locomotion_ForbidMovement, 1);
}

void ABaseCharacter::InitSkelMeshLocation()
{
	GetMesh()->SetRelativeTransform(InitSkelMeshTransform);
	//auto Location = InitSkelMeshTransform.GetLocation();
	//GetMesh()->SetRelativeLocation(Location);
}

void ABaseCharacter::WakeUpPoseSnapShot()
{
	AnimInstance->WakeUpPoseSnapShot();
}

void ABaseCharacter::DoForceKill()
{
	const float KillDamage = StatusComponent->GetKillDamage();
	StatusComponent->DoKill();

	FGameplayEffectContextHandle Context;
	FGameplayEventData Payload{};
	Payload.ContextHandle = Context;
	Payload.Instigator = this;
	Payload.Target = nullptr;
	Payload.EventMagnitude = KillDamage;
	WvAbilitySystemComponent->HandleGameplayEvent(TAG_Common_PassiveAbilityTrigger_KillReact, &Payload);
}
#pragma endregion


/// <summary>
/// Animation overlay change from chooser table
/// </summary>
/// <param name="CurrentOverlay"></param>
const bool ABaseCharacter::OverlayStateChange(const ELSOverlayState CurrentOverlay)
{
	if (SelectableOverlayState == CurrentOverlay)
	{
		return false;
	}

	bool bIsOverlayChange = false;
	const ELSOverlayState PrevOverlay = SelectableOverlayState;

	SelectableOverlayState = CurrentOverlay;

	if (const UClass* OverlayAnimClass = UWvCommonUtils::FindClassInChooserTable(this, OverlayAnimationTable))
	{
		if (OverlayAnimClass->IsChildOf(UAnimInstance::StaticClass()))
		{
			TSubclassOf<UAnimInstance> Subclass = const_cast<UClass*>(OverlayAnimClass);
			GetMesh()->LinkAnimClassLayers(Subclass);
			OverlayChangeDelegate.Broadcast(CurrentOverlay);
			bIsOverlayChange = true;
		}
	}


	if (!bIsOverlayChange)
	{
		SelectableOverlayState = PrevOverlay;
		UE_LOG(LogTemp, Error, TEXT("Overlay Change Failed, function: [%s]"), *FString(__FUNCTION__));
	}

	return bIsOverlayChange;
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

bool ABaseCharacter::IsLeader() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_AI_Leader);
}

void ABaseCharacter::HandleAllowAttack(const bool InAllow)
{
	if (InAllow)
	{
		WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_AI_NotAllowed_Attack, 1);
	}
	else
	{
		WvAbilitySystemComponent->AddGameplayTag(TAG_Character_AI_NotAllowed_Attack, 1);
	}
}

void ABaseCharacter::HandleLookAtTag(const bool bIsAddTag)
{
	if (bIsAddTag)
	{
		WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_ActionLookAt, 1);
	}
	else
	{
		WvAbilitySystemComponent->AddGameplayTag(TAG_Character_ActionLookAt, 1);
	}
}

void ABaseCharacter::SetLeaderTag()
{
	WvAbilitySystemComponent->AddGameplayTag(TAG_Character_AI_Leader, 1);
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
	//UE_LOG(LogTemp, Error, TEXT("Ability: [%s] \n function:[%s]"), *GetNameSafe(Ability), *FString(__FUNCTION__));
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

/// <summary>
/// get currenthealth
/// x => min
/// y => current
/// z => max
/// </summary>
/// <param name="OutHealth"></param>
void ABaseCharacter::GetCharacterHealth(FVector& OutHealth)
{
	StatusComponent->GetCharacterHealth(OutHealth);
}

bool ABaseCharacter::IsMeleePlaying() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_StateMelee);
}


bool ABaseCharacter::HasComboTrigger() const
{
	const UWvAbilityBase* AnimAbility = Cast<UWvAbilityBase>(WvAbilitySystemComponent->GetAnimatingAbility());
	if (IsValid(AnimAbility))
	{
		return AnimAbility->HasComboTrigger();
	}
	return false;
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
	const FVector To = Target->GetActorLocation();
	FindNearlestTarget(To, SyncPointWeight);
}

void ABaseCharacter::FindNearlestTarget(const FVector TargetPosition, const float SyncPointWeight)
{
	const FVector From = GetActorLocation();
	const FVector To = TargetPosition;
	const float Weight = SyncPointWeight;
	ResetNearlestTarget();
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
#if 1
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

	// Sort by distance
	UWvCommonUtils::OrderByDistance(this, HitTargets, true);


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
	if (!IsValid(FinisherSenderDA))
	{
		FinisherSenderDA = OnAsyncLoadDataAsset<UFinisherDataAsset>(TAG_Game_Asset_FinisherSender);
		return false;
	}
	return true;
}

const bool ABaseCharacter::CanFiniherReceiver()
{
	if (!IsValid(FinisherReceinerDA))
	{
		FinisherReceinerDA = OnAsyncLoadDataAsset<UFinisherDataAsset>(TAG_Game_Asset_FinisherReceiver);
		return false;
	}
	return true;
}

bool ABaseCharacter::HasFinisherIgnoreTag(const ABaseCharacter* Target, const FGameplayTag RequireTag) const
{
	auto ASC = Target->GetAbilitySystemComponent();
	if (ASC)
	{
		if (RequireTag == TAG_Weapon_Finisher)
		{
			return ASC->HasMatchingGameplayTag(TAG_Weapon_Finisher_Ignore);
		}
		else if (RequireTag == TAG_Weapon_HoldUp)
		{
			return ASC->HasMatchingGameplayTag(TAG_Weapon_HoldUp_Ignore);
		}
		else if (RequireTag == TAG_Weapon_KnockOut)
		{
			return ASC->HasMatchingGameplayTag(TAG_Weapon_KnockOut_Ignore);
		}
	}
	return false;
}

void ABaseCharacter::BuildFinisherAbility(const FGameplayTag RequireTag)
{
	if (!CanFiniherSender())
	{
		return;
	}

	auto Target = FindNearlestTarget(FinisherConfig.NearlestDistance, FinisherConfig.AngleThreshold, false);
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

	if (!TargetCharacter->CanFiniherReceiver())
	{
		return;
	}

	if (HasFinisherIgnoreTag(TargetCharacter, RequireTag))
	{
		// has ignore tag
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
	const bool bIsResult = TargetCharacter->BuildFinisherAnimationReceiver(RequireTag, OutIndex, Receiver);

	if (!bIsResult)
	{
		UE_LOG(LogTemp, Error, TEXT("invalid BuildFinisherAnimationReceiver => %s"), *FString(__FUNCTION__));
		return;
	}

	TargetCharacter->BuildFinisherAnimationData(Receiver.Montage, Receiver.IsTurnAround, this);
	UE_LOG(LogTemp, Log, TEXT("Finisher Sender => %s, Receiver => %s, Acos => %.3f, Dot => %.3f"), *GetPathName(Sender.Montage), *GetPathName(Receiver.Montage), Acos, Dot);

	// sender only
	{
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
	}

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
	if (!IsValid(FinisherSenderDA))
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid FinisherSender"));
		return;
	}

	const FFinisherAnimationContainer AnimationContainer = FinisherSenderDA->FindContainer(RequireTag);
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

const bool ABaseCharacter::BuildFinisherAnimationReceiver(const FGameplayTag RequireTag, const int32 Index, FFinisherAnimation& OutFinisherAnimation)
{
	if (!IsValid(FinisherReceinerDA))
	{
		UE_LOG(LogTemp, Warning, TEXT("not valid FinisherReceiner"));
		return false;
	}

	FFinisherAnimationContainer AnimationContainer = FinisherReceinerDA->FindContainer(RequireTag);
	if (AnimationContainer.Montages.Num() <= 0)
	{
		return false;
	}
	OutFinisherAnimation = AnimationContainer.Montages[Index];
	return true;
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
void ABaseCharacter::RequestAsyncLoad()
{
	OnSyncLoadCompleteHandler();

	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();

	TArray<FSoftObjectPath> TempPaths;
	for (TPair<FGameplayTag, TSoftObjectPtr<UDataAsset>>Pair : GameDataAssets)
	{
		if (Pair.Value.IsNull())
		{
			continue;
		}
		TempPaths.Add(Pair.Value.ToSoftObjectPath());
	}

	AsyncLoadStreamer = StreamableManager.RequestAsyncLoad(TempPaths, [this]
	{
		this->OnAsyncLoadCompleteHandler();
	});

	RequestComponentsAsyncLoad();
}

void ABaseCharacter::RequestComponentsAsyncLoad()
{
	if (!bIsAllowAsyncLoadComponentAssets)
	{
		return;
	}

	/*
	* InventoryComponent
	* ClimbingComponent
	* LadderComponent
	* WvCharacterMovementComponent
	* QTEActionComponent (player only)
	*/

	auto Components = Game::ComponentExtension::GetComponentsArray<UActorComponent>(this);
	for (UActorComponent* ActComp : Components)
	{
		if (IAsyncComponentInterface* Interface = Cast<IAsyncComponentInterface>(ActComp))
		{
			Interface->RequestAsyncLoad();
		}
	}
}

void ABaseCharacter::OnAsyncLoadCompleteHandler()
{
	FinisherReceinerDA = OnAsyncLoadDataAsset<UFinisherDataAsset>(TAG_Game_Asset_FinisherReceiver);
	CloseCombatDA = OnAsyncLoadDataAsset<UCloseCombatAnimationDataAsset>(TAG_Game_Asset_CloseCombat);
	HitReactionDA = OnAsyncLoadDataAsset<UWvHitReactDataAsset>(TAG_Game_Asset_HitReaction);

	// player only load
	CharacterVFXDA = OnAsyncLoadDataAsset<UCharacterVFXDataAsset>(TAG_Game_Asset_CharacterVFX);

	AsyncLoadStreamer.Reset();
	AsyncLoadCompleteDelegate.Broadcast();
}

void ABaseCharacter::OnSyncLoadCompleteHandler()
{

}

template<typename T>
T* ABaseCharacter::OnAsyncLoadDataAsset(const FGameplayTag Tag) const
{
	T* DataAsset = nullptr;
	if (GameDataAssets.Contains(Tag))
	{
		bool bIsResult = false;
		do
		{
			auto Inst = GameDataAssets[Tag].LoadSynchronous();
			DataAsset = Cast<T>(Inst);
			if (DataAsset)
			{
				bIsResult = (IsValid(DataAsset));
			}
			else
			{
				break;
			}

		} while (!bIsResult);

		if (bIsResult)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s Asset Load Complete %s => [%s]"), *GetNameSafe(this), *GetNameSafe(DataAsset), *FString(__FUNCTION__));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s Asset Load Fail %s => [%s]"), *GetNameSafe(this), *GetNameSafe(DataAsset), *FString(__FUNCTION__));
		}

	}
	return DataAsset;
}

template<typename T>
T* ABaseCharacter::OnSyncLoadDataAsset(const FGameplayTag Tag) const
{
	T* DataAsset = nullptr;

	if (GameDataAssets.Contains(Tag))
	{
		auto Obj = UKismetSystemLibrary::LoadAsset_Blocking(GameDataAssets[Tag]);
		DataAsset = Cast<T>(Obj);
	}

	if (DataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s Asset Load Complete %s => [%s]"), *GetNameSafe(this), *GetNameSafe(DataAsset), *FString(__FUNCTION__));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s Asset Load Fail %s => [%s]"), *GetNameSafe(this), *GetNameSafe(DataAsset), *FString(__FUNCTION__));
	}

	return DataAsset;
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
	Face->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	GetCombatComponent()->VisibilityCurrentWeapon(true);
	WvAbilitySystemComponent->AddGameplayTag(TAG_Vehicle_State_Drive, 1);
}

void ABaseCharacter::EndVehicleAction()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Face->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//GetMesh()->SetVisibility(true);

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


#pragma region CutScene
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
#pragma endregion


#pragma region CloseCombat
int32 ABaseCharacter::GetCombatAnimationIndex() const
{
	if (IsValid(CloseCombatDA))
	{
		auto BodyShape = GetBodyShapeType();
		return CloseCombatDA->GetCombatAnimationIndex(BodyShape);
	}
	return INDEX_NONE;
}

int32 ABaseCharacter::CloseCombatMaxComboCount(const int32 Index) const
{
	if (IsValid(CloseCombatDA))
	{
		auto BodyShape = GetBodyShapeType();
		return CloseCombatDA->CloseCombatMaxComboCount(BodyShape, Index);
	}
	return INDEX_NONE;
}

UAnimMontage* ABaseCharacter::GetCloseCombatAnimMontage(const int32 Index, const FGameplayTag Tag) const
{
	if (IsValid(CloseCombatDA))
	{
		auto BodyShape = GetBodyShapeType();
		return CloseCombatDA->GetAnimMontage(BodyShape, Index, Tag);
	}
	return nullptr;
}

float ABaseCharacter::CalcurateBodyShapePlayRate() const
{
	if (IsValid(CloseCombatDA))
	{
		auto BodyShape = GetBodyShapeType();
		return CloseCombatDA->CalcurateBodyShapePlayRate(BodyShape);
	}
	return 1.0f;
}
#pragma endregion


void ABaseCharacter::CancelAnimatingAbility()
{
	auto AnimAbility = WvAbilitySystemComponent->GetAnimatingAbility();
	if (IsValid(AnimAbility))
	{
		WvAbilitySystemComponent->CancelAbility(AnimAbility);
		UE_LOG(LogWvAI, Log, TEXT("Cancel Ability => %s"), *GetNameSafe(AnimAbility));
	}
}

void ABaseCharacter::CancelAnimatingAbilityMontage()
{
	auto AnimMontage = WvAbilitySystemComponent->GetCurrentMontage();
	if (IsValid(AnimMontage))
	{
		constexpr float LocalInterval = 0.2f;
		AnimInstance->Montage_Stop(LocalInterval, AnimMontage);
		UE_LOG(LogWvAI, Log, TEXT("Cancel AnimMontage => %s"), *GetNameSafe(AnimMontage));
	}
}

void ABaseCharacter::FriendlyActionAbility()
{
	WvAbilitySystemComponent->TryActivateAbilityByTag(TAG_Character_AI_Friend_Action);
}

void ABaseCharacter::CancelFriendlyActionAbility()
{
	CencelActiveAbilities(TAG_Character_AI_Friend_Action);
}

void ABaseCharacter::CencelActiveAbilities(const FGameplayTag InTag)
{
	FGameplayTagContainer Container;
	Container.AddTag(InTag);
	CencelActiveAbilities(Container);
}

void ABaseCharacter::CencelActiveAbilities(const FGameplayTagContainer Container)
{
	TArray<UGameplayAbility*> Abilities;
	WvAbilitySystemComponent->GetActiveAbilitiesWithTags(Container, Abilities);

	Abilities.RemoveAll([](UGameplayAbility* Ability)
	{
		return Ability == nullptr;
	});

	for (UGameplayAbility* Ability : Abilities)
	{
		WvAbilitySystemComponent->CancelAbility(Ability);
		UE_LOG(LogWvAI, Log, TEXT("Cancel Ability => %s"), *GetNameSafe(Ability));
	}

}


#pragma region Skill
/// <summary>
/// if skill max statuscomponent apply 
/// disable gameplay ability ended apply
/// </summary>
/// <param name="IsEnable"></param>
void ABaseCharacter::SkillEnableAction(const bool IsEnable)
{
	if (IsEnable)
	{
		if (!WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_StateSkill_Enable))
		{
			WvAbilitySystemComponent->AddGameplayTag(TAG_Character_StateSkill_Enable, 1);
			OnSkillEnableDelegate.Broadcast(IsEnable);
		}

	}
	else
	{
		if (WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_StateSkill_Enable))
		{
			WvAbilitySystemComponent->RemoveGameplayTag(TAG_Character_StateSkill_Enable, 1);
			OnSkillEnableDelegate.Broadcast(IsEnable);
		}
	}
}

void ABaseCharacter::SkillAction()
{
	WvAbilitySystemComponent->TryActivateAbilityByTag(TAG_Character_StateSkill_Trigger);
}

float ABaseCharacter::GetSkillToWidget() const
{
	return StatusComponent->GetSkillToWidget();
}

/// <summary>
/// debug code
/// </summary>
void ABaseCharacter::SetFullSkill()
{
	if (StatusComponent->SetFullSkill())
	{
		//SkillEnableAction(true);
	}
}
#pragma endregion


bool ABaseCharacter::IsFriendlyActionPlaying() const
{
	return WvAbilitySystemComponent->HasMatchingGameplayTag(TAG_Character_AI_Friend_Action_Playing);
}

bool ABaseCharacter::HasAccelerating() const
{
	return bHasMovementInput;
}

bool ABaseCharacter::IsStrafeMovementMode() const
{
	const FGameplayTag LookingDirectionTag = FGameplayTag::RequestGameplayTag(TEXT("Character.Locomotion.RotationMode.LookingDirection"));
	return WvAbilitySystemComponent->HasMatchingGameplayTag(LookingDirectionTag);
}

bool ABaseCharacter::IsQTEActionPlaying() const
{
	return false;
}

#pragma region URO
void ABaseCharacter::BuildOptimization()
{
	if (!bIsAllowOptimization)
	{
		return;
	}

	auto Components = Game::ComponentExtension::GetComponentsArray<UWvSkeletalMeshComponent>(this);

	for (UWvSkeletalMeshComponent* SkelMesh : Components)
	{
		HandleMeshUpdateRateOptimizations(true, SkelMesh);
		SkelMesh->UpdateRateOptimizations();
	}
}

void ABaseCharacter::BuildRestoreOptimization()
{
	auto Components = Game::ComponentExtension::GetComponentsArray<UWvSkeletalMeshComponent>(this);

	for (UWvSkeletalMeshComponent* SkelMesh : Components)
	{
		HandleMeshUpdateRateOptimizations(false, SkelMesh);
		SkelMesh->RestoreUpdateRateOptimization();
	}
}

void ABaseCharacter::HandleMeshUpdateRateOptimizations(const bool IsInEnableURO, USkeletalMeshComponent* SkelMesh)
{
	if (IsValid(SkelMesh))
	{
		if (IsInEnableURO)
		{
			// Enable URO for sk mesh
			SkelMesh->bEnableUpdateRateOptimizations = true;
			SkelMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		}
		else
		{
			SkelMesh->bEnableUpdateRateOptimizations = false;
			SkelMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		}
	}
}
#pragma endregion

void ABaseCharacter::SetIsDespawnCheck(const bool NewIsDespawnCheck)
{
	bIsDespawnCheck = NewIsDespawnCheck;
}

bool ABaseCharacter::GetIsDespawnCheck() const
{
	return bIsDespawnCheck;
}


bool ABaseCharacter::IsMotionMatchingEnable() const
{
	if (GetLocomotionComponent())
	{
		return GetLocomotionComponent()->bIsMotionMatchingEnable;
	}
	return false;
}


void ABaseCharacter::PreTickLocomotion()
{
	if (!IsMotionMatchingEnable())
	{
		return;
	}

	auto CMC = GetWvCharacterMovementComponent();

	if (!IsValid(CMC) || !IsValid(LocomotionComponent))
	{
		return;
	}

	if (CMC->IsFalling())
	{
		CMC->RotationRate = FRotator(0.f, 200.f, 0.0f);
	}
	else
	{
		CMC->RotationRate = FRotator(0.f, -1.f, 0.0f);
	}


	CMC->MaxAcceleration = LocomotionComponent->ChooseMaxAcceleration();
	CMC->BrakingDecelerationWalking = LocomotionComponent->ChooseBrakingDeceleration();
	CMC->GroundFriction = LocomotionComponent->ChooseGroundFriction();

	CMC->MaxWalkSpeed = LocomotionComponent->ChooseMaxWalkSpeed();
	CMC->MaxWalkSpeedCrouched = LocomotionComponent->ChooseMaxWalkSpeedCrouched();

}


#pragma region Traversal
FTraversalActionData ABaseCharacter::GetTraversalActionData() const
{
	return TraversalActionData;
}

void ABaseCharacter::ResetTraversalActionData()
{
	TraversalActionData.Reset();

	//UE_LOG(LogTemp, Error, TEXT("[%s]"), *FString(__FUNCTION__));
}


bool ABaseCharacter::CanPlayFoleySounds() const
{
	auto CMC = GetWvCharacterMovementComponent();
	if (IsValid(CMC))
	{
		return CMC->IsMovingOnGround() || CMC->IsTraversaling();
	}
	return false;
}
#pragma endregion



