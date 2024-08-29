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
//#include "WvAIController.h"
#include "Animation/WvAnimInstance.h"
//#include "Ability/WvInheritanceAttributeSet.h"
#include "Game/WvGameInstance.h"
#include "GameExtension.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameplayTagContainer.h"
#include "Math/Rotator.h"
#include "Math/UnrealMathSSE.h"
#include "Math/Vector.h"
#include "UObject/UObjectBaseUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"


#include "WvAIController.h"
#include "Level/FieldInstanceSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Engine/SkinnedAssetCommon.h"


#define K_MATERIAL_DYNAMIC_INST_PREFIX TEXT("_DynInst")


void ABaseCharacter::RecalcurateBounds(UMeshComponent* IgnoreMesh)
{
	auto Components = Game::ComponentExtension::GetComponentsArray<UMeshComponent>(this);

	for (UMeshComponent* SkelMesh : Components)
	{
		if (SkelMesh != IgnoreMesh)
		{
			SkelMesh->bUseAttachParentBound = true;
		}
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
			}
			AccessoryObjectRoot->SetVisibility(IsValid(Mesh));
		}
		StaticMeshHandle.Reset();
	});
}

UMaterialInstanceDynamic* ABaseCharacter::GetSkeletalMeshDynamicMaterialInstance(USkeletalMeshComponent* SkeletalMeshComponent, const FName SlotName) const
{
	const int32 L_ElementIndex = SkeletalMeshComponent->GetMaterialIndex(SlotName);

	if (!IsValid(SkeletalMeshComponent->GetSkeletalMeshAsset()))
	{
		return nullptr;
	}

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

UMaterialInstanceDynamic* ABaseCharacter::GetGroomDynamicMaterialInstance(UPrimitiveComponent* MeshComponent, const FName SlotName) const
{
	if (!IsValid(HairMaterialsInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("not async load yet hair material DA => [%s]"), *FString(__FUNCTION__));
		return nullptr;
	}

	const int32 L_ElementIndex = MeshComponent->GetMaterialIndex(SlotName);

	auto Str = SlotName.ToString();
	Str.Append(K_MATERIAL_DYNAMIC_INST_PREFIX);

	switch (L_ElementIndex)
	{
		case 0:
			return MeshComponent->CreateDynamicMaterialInstance(L_ElementIndex, HairMaterialsInstance->HairMat, FName(Str));
			break;
		case 1:
			return MeshComponent->CreateDynamicMaterialInstance(L_ElementIndex, HairMaterialsInstance->FacialMat, FName(Str));
			break;
		case 2:
			return MeshComponent->CreateDynamicMaterialInstance(L_ElementIndex, HairMaterialsInstance->HelmetMat, FName(Str));
			break;
	}

	UE_LOG(LogTemp, Warning, TEXT("Not applicable. slot index => %d, function = >[% s]"), L_ElementIndex, *FString(__FUNCTION__));
	return nullptr;
}

UTexture2D* ABaseCharacter::GetDefaultGroomTexture() const
{
	return HairMaterialsInstance ? HairMaterialsInstance->DefaultGroomTexture : nullptr;
}

UTexture2D* ABaseCharacter::GetDefaultMaskTexture() const
{
	return HairMaterialsInstance ? HairMaterialsInstance->DefaultMaskTexture : nullptr;
}

void ABaseCharacter::SetUpdateAnimationEditor(USkeletalMeshComponent* Face)
{
#if WITH_EDITOR
	if (IsValid(Face))
	{
		Face->SetUpdateAnimationInEditor(true);
	}
#endif
}

void ABaseCharacter::HairStrandsLODSetUp(USkeletalMeshComponent* Face)
{
	if (!IsValid(Face))
	{
		return;
	}

	// If strands are disabled then make fused eyelashes visible on lower LODs 
	static const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HairStrands.Strands"));
	const int32 ConsoleValue = CVar->GetInt();
	if (!(ConsoleValue > 0))
	{
		// Get eyelashes material on higher LODs
		UMaterialInstanceDynamic* MID_Ten = Cast<UMaterialInstanceDynamic>(Face->GetMaterial(10));
		// Get eyelashes material on lower LODs
		UMaterialInstanceDynamic* MID_Six = Cast<UMaterialInstanceDynamic>(Face->GetMaterial(6));

		if (MID_Ten && MID_Six)
		{
			const FName Opacity = FName(TEXT("Opacity"));
			const FName OpacityRT = FName(TEXT("OpacityRT"));
			MID_Six->SetScalarParameterValue(Opacity, MID_Ten->K2_GetScalarParameterValue(Opacity));
			MID_Six->SetScalarParameterValue(OpacityRT, MID_Ten->K2_GetScalarParameterValue(OpacityRT));
		}
	}
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
		// draw gender
		BasePos.Z += Step;
		const FString DebugStr = *FString::Format(TEXT("Gender => {0}"), { *GETENUMSTRING("/Script/Redemption.EGenderType", GetGenderType()) });
		DrawDebugString(GetWorld(), BasePos, DebugStr, nullptr, FColor::Blue, 0.f, false, 1.2f);
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
}

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

#pragma endregion


