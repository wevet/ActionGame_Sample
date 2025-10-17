// Copyright 2022 wevet works All Rights Reserved.


#include "BaseCharacter.h"
#include "Redemption.h"
#include "Misc/WvCommonUtils.h"
#include "Component/WvCharacterMovementComponent.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Locomotion/LocomotionComponent.h"
#include "Component/CombatComponent.h"
#include "Component/StatusComponent.h"
#include "Game/WvGameInstance.h"
#include "GameExtension.h"


#include "Components/CapsuleComponent.h"
#include "GroomComponent.h"
#include "UObject/UObjectBaseUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkinnedAssetCommon.h"


#define K_MATERIAL_DYNAMIC_INST_PREFIX TEXT("_DynInst")


using namespace CharacterDebug;


void ABaseCharacter::RecalcurateBounds()
{
	for (UMeshComponent* SkelMesh : GetBodyMeshComponents())
	{
		SkelMesh->bUseAttachParentBound = true;
	}
}

void ABaseCharacter::AsyncSetSkelMesh(USkeletalMeshComponent* SkeletalMeshComponent, TSoftObjectPtr<USkeletalMesh> SkelMesh)
{
	if (SkelMesh.IsNull())
	{
		return;
	}

	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();
	const FSoftObjectPath ObjectPath = SkelMesh.ToSoftObjectPath();
	SkelMeshHandle = StreamableManager.RequestAsyncLoad(ObjectPath, [this, SkeletalMeshComponent]
	{
		UObject* LoadedObj = SkelMeshHandle.Get()->GetLoadedAsset();
		if (LoadedObj)
		{
			USkeletalMesh* Mesh = Cast<USkeletalMesh>(LoadedObj);
			if (Mesh)
			{
				SkeletalMeshComponent->SetSkinnedAssetAndUpdate(Mesh, true);
			}
		}
		SkelMeshHandle.Reset();
	});
}

void ABaseCharacter::AsyncSetAccessoryMesh(TSoftObjectPtr<UStaticMesh> StaticMesh, const FName SocketName)
{
	if (StaticMesh.IsNull())
	{
		return;
	}

	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();
	const FSoftObjectPath ObjectPath = StaticMesh.ToSoftObjectPath();
	StaticMeshHandle = StreamableManager.RequestAsyncLoad(ObjectPath, [this, SocketName]
	{
		UObject* LoadedObj = StaticMeshHandle.Get()->GetLoadedAsset();
		if (LoadedObj)
		{
			UStaticMesh* Mesh = Cast<UStaticMesh>(LoadedObj);
			AccessoryObjectRoot->SetStaticMesh(Mesh);

			if (Mesh)
			{
				AccessoryObjectRoot->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, SocketName);
				AccessoryObjectRoot->bUseAttachParentBound = true;
			}
			AccessoryObjectRoot->SetVisibility(IsValid(Mesh));
		}
		StaticMeshHandle.Reset();
	});
}

UMaterialInstanceDynamic* ABaseCharacter::GetSkeletalMeshDynamicMaterialInstance(USkeletalMeshComponent* SkeletalMeshComponent, const FName SlotName) const
{
	if (!IsValid(SkeletalMeshComponent->GetSkeletalMeshAsset()))
	{
		return nullptr;
	}

	const int32 L_ElementIndex = SkeletalMeshComponent->GetMaterialIndex(SlotName);
	TArray<FSkeletalMaterial> Materials = SkeletalMeshComponent->GetSkeletalMeshAsset()->GetMaterials();
	if (Materials.IsValidIndex(L_ElementIndex))
	{
		FSkeletalMaterial& MaterialInfo = Materials[L_ElementIndex];

		if (MaterialInfo.MaterialInterface)
		{
			auto Str = MaterialInfo.MaterialSlotName.ToString();
			Str.Append(K_MATERIAL_DYNAMIC_INST_PREFIX);
			return SkeletalMeshComponent->CreateDynamicMaterialInstance(L_ElementIndex, MaterialInfo.MaterialInterface, FName(Str));
		}
	}

	return nullptr;
}

UMaterialInstanceDynamic* ABaseCharacter::GetStaticMeshDynamicMaterialInstance(UStaticMeshComponent* StaticMeshComponent, const FName SlotName) const
{
	const int32 L_ElementIndex = StaticMeshComponent->GetMaterialIndex(SlotName);
	UMaterialInterface* MaterialInterface = StaticMeshComponent->GetMaterial(L_ElementIndex);

	if (MaterialInterface)
	{
		auto Str = SlotName.ToString();
		Str.Append(K_MATERIAL_DYNAMIC_INST_PREFIX);
		return StaticMeshComponent->CreateDynamicMaterialInstance(L_ElementIndex, MaterialInterface, FName(Str));
	}

	return nullptr;
}

UMaterialInstanceDynamic* ABaseCharacter::GetGroomDynamicMaterialInstanceWithMaterials(UPrimitiveComponent* MeshComponent, const FName SlotName,
	UMaterialInterface* HairMat,
	UMaterialInterface* FacialMat,
	UMaterialInterface* HelmetMat) const
{
	const int32 L_ElementIndex = MeshComponent->GetMaterialIndex(SlotName);
	const FString MIDSlotName = SlotName.ToString().Append(K_MATERIAL_DYNAMIC_INST_PREFIX);

	switch (L_ElementIndex)
	{
	case 0:
		return HairMat ? MeshComponent->CreateDynamicMaterialInstance(L_ElementIndex, HairMat, FName(MIDSlotName)) : nullptr;
		break;
	case 1:
		return FacialMat ? MeshComponent->CreateDynamicMaterialInstance(L_ElementIndex, FacialMat, FName(MIDSlotName)) : nullptr;
		break;
	case 2:
		return HelmetMat ? MeshComponent->CreateDynamicMaterialInstance(L_ElementIndex, HelmetMat, FName(MIDSlotName)) : nullptr;
		break;
	}
	return nullptr;
}


void ABaseCharacter::SetUpdateAnimationEditor()
{
#if WITH_EDITOR
	Face->SetUpdateAnimationInEditor(true);
#endif
}

void ABaseCharacter::HairStrandsLODSetUp()
{
	// If strands are disabled then make fused eyelashes visible on lower LODs 
	static const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HairStrands.Strands"));
	const int32 ConsoleValue = CVar->GetInt();
	if (!(ConsoleValue > 0))
	{
		UMaterialInstanceDynamic* MID_Ten = nullptr;
		UMaterialInstanceDynamic* MID_Six = nullptr;

		// Get eyelashes material on higher LODs
		if (Face->GetMaterials().IsValidIndex(10))
		{
			MID_Ten = Cast<UMaterialInstanceDynamic>(Face->GetMaterial(10));
			if (!IsValid(MID_Ten))
			{
				MID_Ten = Face->CreateDynamicMaterialInstance(10);
			}
		}

		// Get eyelashes material on lower LODs
		if (Face->GetMaterials().IsValidIndex(6))
		{
			MID_Six = Cast<UMaterialInstanceDynamic>(Face->GetMaterial(6));
			if (!IsValid(MID_Six))
			{
				MID_Six = Face->CreateDynamicMaterialInstance(6);
			}
		}

		if (MID_Ten && MID_Six)
		{
			const FName Opacity = FName(TEXT("Opacity"));
			const FName OpacityRT = FName(TEXT("OpacityRT"));
			MID_Six->SetScalarParameterValue(Opacity, MID_Ten->K2_GetScalarParameterValue(Opacity));
			MID_Six->SetScalarParameterValue(OpacityRT, MID_Ten->K2_GetScalarParameterValue(OpacityRT));
		}
	}
}

void ABaseCharacter::OnMobDeath()
{
	if (IsBotCharacter())
	{
		Face->SetLeaderPoseComponent(GetMesh(), true, true);
	}

	RecalcurateBounds();

	// force lod accessory
	{
		AccessoryObjectRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AccessoryObjectRoot->bUseAttachParentBound = true;

		auto ACC_Mesh = GetAccessoryMesh();
		if (ACC_Mesh)
		{
			const int32 NumLods = ACC_Mesh->GetNumLODs();
			AccessoryObjectRoot->SetForcedLodModel(NumLods);
		}
	}

	// disable groom collision
	SetGroomSimulation(false);

#if false
	{
		// In case of ragdoll, the lod of the root mesh is not changed.
		// Because it will be jerky.
		auto Components = Game::ComponentExtension::GetComponentsArray<USkeletalMeshComponent>(this);
		Components.RemoveAll([this](USkeletalMeshComponent* SkelMesh)
		{
			return SkelMesh == nullptr || SkelMesh == GetMesh();
		});

		for (USkeletalMeshComponent* Component : Components)
		{
			const int32 MaxLod = Component->GetNumLODs();
			Component->SetForcedLOD(MaxLod);
		}

		const int32 MaxLod = GetMesh()->GetNumLODs();
		const int32 MinLod = (GetMesh()->GetNumLODs() - 1) - MaxLod;
		GetMesh()->SetForcedLOD(FMath::Min(0, MinLod));
	}

	// force lod hair
	{
		auto Components = Game::ComponentExtension::GetComponentsArray<UGroomComponent>(this);
		Components.RemoveAll([](UGroomComponent* Groom)
		{
			return Groom == nullptr;
		});

		for (UGroomComponent* Component : Components)
		{
			const int32 NumLods = Component->GetNumLODs();
			Component->SetForcedLOD(NumLods);
			Component->ReleaseHairSimulation();
			Component->SetEnableSimulation(false);

			if (Component->GroomCache)
			{

			}
		}
	}
#endif


}

void ABaseCharacter::UpdateAccessory(const FAccessoryData& InAccessoryData)
{
	Accessory = InAccessoryData;
}

UStaticMesh* ABaseCharacter::GetAccessoryMesh() const
{
	return AccessoryObjectRoot->GetStaticMesh();
}

FAccessoryData ABaseCharacter::GetAccessoryData() const
{
	return Accessory;
}

void ABaseCharacter::SetGenderType(const EGenderType InGenderType)
{
	StatusComponent->SetGenderType(InGenderType);
}

EGenderType ABaseCharacter::GetGenderType() const
{
	return StatusComponent->GetGenderType();
}

void ABaseCharacter::SetBodyShapeType(const EBodyShapeType InBodyShapeType)
{
	if (IsValid(StatusComponent))
	{
		StatusComponent->SetBodyShapeType(InBodyShapeType);
	}
}

EBodyShapeType ABaseCharacter::GetBodyShapeType() const
{
	if (IsValid(StatusComponent))
	{
		return StatusComponent->GetBodyShapeType();
	}
	return EBodyShapeType::Normal;
}

FCharacterInfo ABaseCharacter::GetCharacterInfo() const
{
	return StatusComponent->GetCharacterInfo();
}

/// <summary>
/// ëÃån
/// </summary>
void ABaseCharacter::GenerateRandomBodyShapeType()
{
	const uint8 Index = (uint8)FMath::RandRange(0, 2);

	SetBodyShapeType((EBodyShapeType)Index);
}

/// <summary>
/// ê´ï 
/// </summary>
void ABaseCharacter::GenerateRandomGenderType()
{
	const uint8 Index = (uint8)FMath::RandRange(0, 1);

	SetGenderType((EGenderType)Index);
}


TArray<USkeletalMeshComponent*> ABaseCharacter::GetBodyMeshComponents() const
{
	TArray<USkeletalMeshComponent*>Result;
	Result.Add(GetMesh());
	Result.Add(Face);

	Result.Add(Body);
	Result.Add(Top);
	Result.Add(Bottom);
	Result.Add(Feet);
	Result.Add(Other1);
	return Result;
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

FVector ABaseCharacter::GetCharacterHeadLocation() const
{
	auto Position = GetActorLocation();
	const float Height = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Position.Z += Height;
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

void ABaseCharacter::SetOverlayMaterials(const bool IsEnable, TArray<USkeletalMeshComponent*> IgnoreMeshes)
{
	if (!IsValid(CharacterVFXDA))
	{
		return;
	}

	auto MID = CharacterVFXDA->OverlayMaterial;

	if (IsValid(MID))
	{
		TArray<USkeletalMeshComponent*> Components = GetBodyMeshComponents();

		Components.RemoveAll([&](USkeletalMeshComponent* SkelMesh)
		{
			return (SkelMesh == nullptr || (IgnoreMeshes.Contains(SkelMesh)));
		});

		for (USkeletalMeshComponent* SkelMesh : Components)
		{
			SkelMesh->SetOverlayMaterial(IsEnable ? MID : nullptr);
		}
	}


	auto F_MID = CharacterVFXDA->FaceOverlayMaterial;
	if (IsValid(F_MID))
	{
		Face->SetOverlayMaterial(IsEnable ? F_MID : nullptr);
	}
}

void ABaseCharacter::SetGroomSimulation(const bool IsEnable)
{
	auto Components = Game::ComponentExtension::GetComponentsArray<UGroomComponent>(this);
	Components.RemoveAll([](UGroomComponent* Groom)
	{
		return Groom == nullptr;
	});

	for (UGroomComponent* Component : Components)
	{
		Component->SetCollisionEnabled(IsEnable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

		const FName ProfileName = IsEnable ? FName(TEXT("PhysicsActor")) : FName(TEXT("Ragdoll"));
		Component->SetCollisionProfileName(ProfileName);

		if (Component->GroomCache)
		{
			//
		}

	}
}


void ABaseCharacter::UpdateMontageMatching(const float InPosition)
{
	//UE_LOG(LogTemp, Log, TEXT("Ladder Position => %.3f : [%s]"), InPosition, *FString(__FUNCTION__));
}

void ABaseCharacter::FinishMontageMatching()
{
	auto CMC = GetWvCharacterMovementComponent();
	if (CMC)
	{
		CMC->MantleEnd();
	}
}


void ABaseCharacter::EnableMasterPose(USkeletalMeshComponent* SkeletalMeshComponent)
{
	const auto Asset = SkeletalMeshComponent->GetSkeletalMeshAsset();

	if (IsValid(Asset))
	{
		if (Asset->GetPostProcessAnimBlueprint() == nullptr && SkeletalMeshComponent->AnimClass == nullptr)
		{
			SkeletalMeshComponent->SetLeaderPoseComponent(GetMesh());
		}
	}
}


void ABaseCharacter::SetMasterPoseBody()
{
	EnableMasterPose(Body);
	EnableMasterPose(Top);
	EnableMasterPose(Bottom);
	EnableMasterPose(Feet);

	EnableMasterPose(Other1);
}

void ABaseCharacter::SetCrowdMasterPoseBody()
{
	SetMasterPoseBody();
	SetUpdateAnimationEditor();
	RecalcurateBounds();
}

void ABaseCharacter::LoadAndSetMeshes(const bool bIsBlockLoadAssets, TSoftObjectPtr<USkeletalMesh> BaseMesh, TSoftObjectPtr<USkeletalMesh> BodyMesh, TSoftObjectPtr<USkeletalMesh> FaceMesh, TSoftObjectPtr<USkeletalMesh> TopMesh, TSoftObjectPtr<USkeletalMesh> BottomMesh, TSoftObjectPtr<USkeletalMesh> FeetMesh)
{

	if (bIsBlockLoadAssets)
	{
		UWvCommonUtils::SetSkeletalMeshLoadAssetBlocking(GetMesh(), BaseMesh);
		UWvCommonUtils::SetSkeletalMeshLoadAssetBlocking(Body, BodyMesh);
		UWvCommonUtils::SetSkeletalMeshLoadAssetBlocking(Face, FaceMesh);
		UWvCommonUtils::SetSkeletalMeshLoadAssetBlocking(Top, TopMesh);
		UWvCommonUtils::SetSkeletalMeshLoadAssetBlocking(Bottom, BottomMesh);
		UWvCommonUtils::SetSkeletalMeshLoadAssetBlocking(Feet, FeetMesh);
		return;
	}

	FStreamableManager& StreamableManager = UWvGameInstance::GetStreamableManager();

	TArray<TSoftObjectPtr<USkeletalMesh>> SoftSKMeshes({ BaseMesh, BodyMesh, FaceMesh, TopMesh, BottomMesh, FeetMesh});

	InitialBodyMeshesPath.Reset();
	InitialBodyMeshesPath.Reserve(SoftSKMeshes.Num());

	for (auto SoftMesh : SoftSKMeshes)
	{
		InitialBodyMeshesPath.Add(SoftMesh.ToSoftObjectPath());
	}

	StreamableManager.RequestAsyncLoad(InitialBodyMeshesPath, [this] 
	{
		this->OnLoadAndSetMeshes_Callback();
	});

}


void ABaseCharacter::OnLoadAndSetMeshes_Callback()
{
	// 0 Base
	// 1 Body
	// 2 Face
	// 3 Top
	// 4 Bottom
	// 5 Feet

	if (!IsInGameThread())
	{
		UE_LOG(LogTemp, Warning, TEXT("not game thread: [%s]"), *FString(__FUNCTION__));
	}

	if (InitialBodyMeshesPath.IsValidIndex(0))
	{
		auto LoadedObj = TSoftObjectPtr<USkeletalMesh>(InitialBodyMeshesPath[0]).Get();
		if (LoadedObj)
		{
			GetMesh()->SetSkinnedAssetAndUpdate(LoadedObj, true);
		}
	}

	if (InitialBodyMeshesPath.IsValidIndex(1))
	{
		auto LoadedObj = TSoftObjectPtr<USkeletalMesh>(InitialBodyMeshesPath[1]).Get();
		if (LoadedObj)
		{
			Body->SetSkinnedAssetAndUpdate(LoadedObj, true);
		}
	}

	if (InitialBodyMeshesPath.IsValidIndex(2))
	{
		auto LoadedObj = TSoftObjectPtr<USkeletalMesh>(InitialBodyMeshesPath[2]).Get();
		if (LoadedObj)
		{
			Face->SetSkinnedAssetAndUpdate(LoadedObj, true);
		}
	}

	if (InitialBodyMeshesPath.IsValidIndex(3))
	{
		auto LoadedObj = TSoftObjectPtr<USkeletalMesh>(InitialBodyMeshesPath[3]).Get();
		if (LoadedObj)
		{
			Top->SetSkinnedAssetAndUpdate(LoadedObj, true);
		}
	}

	if (InitialBodyMeshesPath.IsValidIndex(4))
	{
		auto LoadedObj = TSoftObjectPtr<USkeletalMesh>(InitialBodyMeshesPath[4]).Get();
		if (LoadedObj)
		{
			Bottom->SetSkinnedAssetAndUpdate(LoadedObj, true);
		}
	}

	if (InitialBodyMeshesPath.IsValidIndex(5))
	{
		auto LoadedObj = TSoftObjectPtr<USkeletalMesh>(InitialBodyMeshesPath[5]).Get();
		if (LoadedObj)
		{
			Feet->SetSkinnedAssetAndUpdate(LoadedObj, true);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("All meshes have been loaded and set. => [%s]"), *FString(__FUNCTION__));
	this->AsyncMeshesLoadCompleteDelegate.Broadcast();
	this->OnAsyncMeshesLoadCompleteDelegate();
}


#pragma region Debug
void ABaseCharacter::DrawDebug()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (CVarDebugCharacterStatus.GetValueOnGameThread() > 0)
	{
		DisplayDrawDebug_Internal();
	}

	LocomotionComponent->DrawLocomotionDebug();
#endif

}

void ABaseCharacter::DisplayDrawDebug_Internal()
{
	if (!IsValid(GetCapsuleComponent()))
	{
		return;
	}

	if (!IsBotCharacter() || IsDead() || !UWvCommonUtils::IsInViewport(this))
	{
		return;
	}

	constexpr float Step = 40.0f;
	FVector BasePos = GetActorLocation();
	const float CapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	BasePos.Z += CapsuleHalfHeight;

	{
		// draw skeleton name
		if (GetMesh()->GetSkeletalMeshAsset())
		{
			const USkeleton* SK = GetMesh()->GetSkeletalMeshAsset()->GetSkeleton();
			const FString DebugStr = FString::Format(TEXT("SK => {0}"), { GetNameSafe(SK) });
			DrawDebugString(GetWorld(), BasePos, DebugStr, nullptr, FColor::Blue, 0.f, false, 1.2f);
		}
	}

	{
		// draw teamid
		BasePos.Z += Step;
		const int32 TeamID = GenericTeamIdToInteger(GetGenericTeamId());
		const FString DebugStr = *FString::Format(TEXT("TeamID => {0}"), { FString::FromInt(TeamID) });
		DrawDebugString(GetWorld(), BasePos, DebugStr, nullptr, FColor::Blue, 0.f, false, 1.2f);
	}

	{
		// draw character tag
		BasePos.Z += Step;
		const FString TagName = CharacterTag.GetTagName().ToString();
		const FString DebugStr = *FString::Format(TEXT("TagName => {0}"), { *TagName });
		DrawDebugString(GetWorld(), BasePos, DebugStr, nullptr, FColor::Blue, 0.f, false, 1.2f);
	}

	{
		// draw gender
		BasePos.Z += Step;
		const FString DebugStr = *FString::Format(TEXT("Gender => {0}"), { *GETENUMSTRING("/Script/WvAbilitySystem.EGenderType", GetGenderType()) });
		DrawDebugString(GetWorld(), BasePos, DebugStr, nullptr, FColor::Red, 0.f, false, 1.2f);
	}

	{
		// draw body shape
		BasePos.Z += Step;
		const FString DebugStr = *FString::Format(TEXT("BodyShape => {0}"), { *GETENUMSTRING("/Script/WvAbilitySystem.EBodyShapeType", GetBodyShapeType()) });
		DrawDebugString(GetWorld(), BasePos, DebugStr, nullptr, FColor::Red, 0.f, false, 1.2f);
	}

	{
		// draw ai action state
		FColor Color = FColor::Red;
		switch (AIActionState)
		{
		case EAIActionState::Friendly:
			Color = FColor::Green;
			break;
		case EAIActionState::Patrol:
			Color = FColor::Blue;
			break;
		case EAIActionState::Search:
			Color = FColor::Yellow;
			break;
		}

		BasePos.Z += Step;
		const FString DebugStr = *FString::Format(TEXT("AIActionState => {0}"), { *GETENUMSTRING("/Script/WvAbilitySystem.EAIActionState", AIActionState) });
		UKismetSystemLibrary::DrawDebugString(GetWorld(), BasePos, DebugStr, nullptr, Color, 0.f);
	}
}

#pragma endregion


