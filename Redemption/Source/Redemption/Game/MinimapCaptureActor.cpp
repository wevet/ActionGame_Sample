// Copyright 2022 wevet works All Rights Reserved.


#include "Game/MinimapCaptureActor.h"
#include "Character/BaseCharacter.h"

#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UObjectBaseUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MinimapCaptureActor)

AMinimapCaptureActor::AMinimapCaptureActor()
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SceneComponent->bAutoActivate = 1;
	RootComponent = SceneComponent;

	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent"));
	SceneCaptureComponent->SetupAttachment(RootComponent, NAME_None);

	SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;

	SceneCaptureComponent->bCaptureEveryFrame = false;

	SceneCaptureComponent->ShowFlags.DisableAdvancedFeatures();
	SceneCaptureComponent->ShowFlags.SetPostProcessing(false);
	SceneCaptureComponent->ShowFlags.SetLighting(true);
	SceneCaptureComponent->ShowFlags.SetAntiAliasing(false);
	SceneCaptureComponent->ShowFlags.SetToneCurve(false);
	SceneCaptureComponent->ShowFlags.SetEyeAdaptation(false);
	SceneCaptureComponent->ShowFlags.SetBloom(false);
	SceneCaptureComponent->ShowFlags.SetLumenReflections(false);
	SceneCaptureComponent->PostProcessBlendWeight = 0.f;
	SceneCaptureComponent->CaptureSource = SCS_SceneDepth;
	SceneCaptureComponent->SetRelativeLocation(FVector::ZeroVector);
}

void AMinimapCaptureActor::BeginPlay()
{
	Super::BeginPlay();

	SceneCaptureComponent->OrthoWidth = OrthoWidth;

	if (GetWorld())
	{
		if (MinimapCollection)
		{
			MinimapCollectionInstance = GetWorld()->GetParameterCollectionInstance(MinimapCollection);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("not valid World : [%s]"), *FString(__FUNCTION__));
	}

	Super::SetActorTickEnabled(false);
}

void AMinimapCaptureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MinimapCollectionInstance.Reset();
	PlayerCharacter.Reset();
	Super::EndPlay(EndPlayReason);
}

void AMinimapCaptureActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMinimapCaptureActor::Initializer(APawn* NewPlayerCharacter)
{
	if (PlayerCharacter.IsValid())
	{
		PlayerCharacter.Reset();
	}

	PlayerCharacter = NewPlayerCharacter;
}


void AMinimapCaptureActor::UpdateCapture(const float DeltaTime)
{
	if (!MinimapCollectionInstance.IsValid() || !PlayerCharacter.IsValid())
	{
		return;
	}

	CaptureElapsed += DeltaTime;
	if (CaptureElapsed >= CaptureInterval)
	{
		UpdateCapture_Internal(DeltaTime);
		CaptureElapsed = 0.f;
	}
}

void AMinimapCaptureActor::UpdateCapture_Internal(const float DeltaTime)
{
	//
	const auto Location = PlayerCharacter->GetActorLocation();
	const auto Rotation = PlayerCharacter->GetActorRotation();
	float Yaw = Rotation.Yaw;
	if (Yaw < 0.0f)
	{
		Yaw += 360.0f;
	}

	CurrentYaw = FMath::FInterpTo(CurrentYaw, Yaw, DeltaTime, InterpSpeed);

	MinimapCollectionInstance->SetVectorParameterValue(PlayerPositionParamName, FLinearColor(Location.X, Location.Y, Location.Z, 1.f));
	MinimapCollectionInstance->SetScalarParameterValue(PlayerRotationParamName, CurrentYaw);

	SetActorLocation(Location + FVector(0.f, 0.f, HeightOffset));

	//const auto CurRot = GetActorRotation();
	//SetActorRotation(FRotator(CurRot.Pitch, Yaw, CurRot.Roll));

	SceneCaptureComponent->CaptureScene();

	
}

bool AMinimapCaptureActor::IsEnableMapRotation() const
{
	if (MinimapCollectionInstance.IsValid())
	{
		float OutValue = 0.f;
		if (MinimapCollectionInstance->GetScalarParameterValue(FName("EnableRotation"), OutValue))
		{
			return (OutValue == 1.0f);
		}
	}
	return false;
}
