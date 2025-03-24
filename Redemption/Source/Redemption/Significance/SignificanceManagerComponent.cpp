// Copyright 2022 wevet works All Rights Reserved.


#include "Significance/SignificanceManagerComponent.h"
#include "Game/WvProjectSetting.h"
#include "Character/WvPlayerCameraManager.h"
#include "Significance/SignificanceInterface.h"

#include "IAnimationBudgetAllocator.h"
#include "SignificanceManager.h"


USignificanceManagerComponent::USignificanceManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USignificanceManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	const UWvProjectSetting* ProjectSettings = GetDefault<UWvProjectSetting>();
	SignificanceUpdateInterval = ProjectSettings->SignificanceUpdateInterval;
	SignificanceUpdateTimer = SignificanceUpdateInterval;
	SignificanceLevel0EndIndex = ProjectSettings->SignificanceLevel0_BucketNumber;
	SignificanceLevel1EndIndex = SignificanceLevel0EndIndex + ProjectSettings->SignificanceLevel1_BucketNumber;
	SignificanceLevel2EndIndex = SignificanceLevel1EndIndex + ProjectSettings->SignificanceLevel2_BucketNumber;
	SignificanceLevel3EndIndex = SignificanceLevel2EndIndex + ProjectSettings->SignificanceLevel3_BucketNumber;
	SignificanceOutOfRange = ProjectSettings->SignificanceOutOfRange;
	SignificanceMelonValue = ProjectSettings->SignificanceMelonValue;
	SignificancePeanutValue = ProjectSettings->SignificancePeanutValue;
	SignificanceSesameValue = ProjectSettings->SignificanceSesameValue;

	CameraManager = Cast<AWvPlayerCameraManager>(GetOwner());
	if (IsValid(CameraManager))
	{
		CameraManager->OnPostUpdateCamera.AddUniqueDynamic(this, &USignificanceManagerComponent::UpdateSignificance);
	}

	Super::SetComponentTickEnabled(false);
}

void USignificanceManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CameraManager && IsValid(CameraManager))
	{
		CameraManager->OnPostUpdateCamera.RemoveDynamic(this, &USignificanceManagerComponent::UpdateSignificance);
	}

	Super::EndPlay(EndPlayReason);
}


void USignificanceManagerComponent::UpdateSignificance(const float DeltaTime)
{
	if (!IsValid(CameraManager))
	{
		return;
	}

	SignificanceUpdateTimer += DeltaTime;
	if (SignificanceUpdateTimer < SignificanceUpdateInterval)
	{
		return;
	}

	SignificanceUpdateTimer = 0.0f;

	if (UWorld* World = GetWorld())
	{
		if (USignificanceManager* SignificanceManager = USignificanceManager::Get(World))
		{
			// Use the camera's global transform to update the Significance
			FTransform Viewpoint = FTransform(CameraManager->GetCameraRotation(), CameraManager->GetCameraLocation());
			TArray<FTransform> TransformArray;
			TransformArray.Add(Viewpoint);
			SignificanceManager->Update(TArrayView<FTransform>(TransformArray));

			// basket count
			int32 BucketIndex = 0;

			TArray<USignificanceManager::FManagedObjectInfo*> ManagedObjectsArray;
			SignificanceManager->GetManagedObjects(ManagedObjectsArray, true);
			for (int32 Index = 0; Index < ManagedObjectsArray.Num(); Index++)
			{
				USignificanceManager::FManagedObjectInfo* ManagedObjectInfo = ManagedObjectsArray[Index];
				AActor* Actor = Cast<AActor>(ManagedObjectInfo->GetObject());
				int32 SignificanceLevel = 0;
				float SignificanceValue = ManagedObjectInfo->GetSignificance();
				if (SignificanceValue <= 0.0f)
				{
					// A negative significance indicates that the character is invisible to the eye and that Level4 is used directly
					SignificanceLevel = 4;
				}
				else
				{
					if (BucketIndex < SignificanceLevel0EndIndex)
					{
						SignificanceLevel = 0;
					}
					else if (BucketIndex < SignificanceLevel1EndIndex)
					{
						SignificanceLevel = 1;
					}
					else if (BucketIndex < SignificanceLevel2EndIndex)
					{
						SignificanceLevel = 2;
					}
					else if (BucketIndex < SignificanceLevel3EndIndex)
					{
						SignificanceLevel = 3;
					}
					else
					{
						SignificanceLevel = 4;
					}
				}

				// Disengagement range determination at Level 2 and above
				if (SignificanceLevel >= 2)
				{
					FVector Origin;
					FVector BoxExtent;
					float SphereRadius;
					ISignificanceInterface::Execute_GetSignificanceBounds(Actor, Origin, BoxExtent, SphereRadius);

					FVector ViewpointToActorSpear = Origin - Viewpoint.GetTranslation();
					float ViewpointToActorDistance = ViewpointToActorSpear.Size();
					if (ViewpointToActorDistance > SignificanceOutOfRange)
					{
						SignificanceLevel = 5;
					}
				}

				// Increase the basket count when Level 4 or lower
				if (SignificanceLevel < 4)
				{
					BucketIndex++;
				}
				ISignificanceInterface::Execute_OnSignificanceLevelChanged(Actor, SignificanceLevel);

#if 0
				// Significance Fruit type changes
				const float RawSignificanceValue = SignificanceValue < 0.0f ? -1.0f / SignificanceValue : SignificanceValue;
				ESignificanceFruitType SignificanceFruitType = ESignificanceFruitType::Melon;
				if (SignificanceLevel > 2)
				{
					if (RawSignificanceValue <= SignificanceSesameValue)
					{
						SignificanceFruitType = ESignificanceFruitType::Sesame;
					}
					else if (RawSignificanceValue <= SignificancePeanutValue)
					{
						SignificanceFruitType = ESignificanceFruitType::Peanut;
					}
				}
				ISignificanceInterface::Execute_OnSignificanceFruitTypeChanged(Actor, SignificanceFruitType);
#endif

#if 0
				if (CVarDebugDrawSignificanceLevel.GetValueOnAnyThread() != 0)
				{
					const float DebugValue = CVarDebugDrawSignificanceLevel.GetValueOnAnyThread();
					UActorComponent* ActorComp = Actor->GetComponentByClass(USignificanceComponent::StaticClass());
					USignificanceComponent* SignComp = Cast<USignificanceComponent>(ActorComp);
					const FColor DrawColors[6] = { FColor::Green, FColor::Blue, FColor::Purple, FColor::Yellow, FColor::Red, FColor::Black };
					const FColor DrawColor = DrawColors[SignComp->SignificanceLevel];
					DrawDebugBox(Actor->GetWorld(), Actor->GetActorLocation(), FVector(100.f, 100.f, 100.f), FQuat::Identity, DrawColor, false, 0.3f, 0, DebugValue);
				}

				if (CVarDebugDrawSignificanceType.GetValueOnAnyThread() != 0)
				{
					UActorComponent* ActorComp = Actor->GetComponentByClass(USignificanceComponent::StaticClass());
					USignificanceComponent* SignComp = Cast<USignificanceComponent>(ActorComp);
					const FColor DrawColors[5] = { FColor::Green, FColor::Blue, FColor::Purple, FColor::Yellow, FColor::Red };
					const FColor DrawColor = SignComp->SignificanceFruitType == ESignificanceFruitType::Melon ? DrawColors[0] : SignComp->SignificanceFruitType == ESignificanceFruitType::Peanut ? DrawColors[1] : DrawColors[2];
					DrawDebugSphere(Actor->GetWorld(), Actor->GetActorLocation(), 100.f, 12, DrawColor, false, 0.3f, 0, 1.f);
				}
#endif

			}
		}
	}
}


void USignificanceManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

