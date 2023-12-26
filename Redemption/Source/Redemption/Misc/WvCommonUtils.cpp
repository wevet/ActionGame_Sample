// Copyright 2022 wevet works All Rights Reserved.


#include "Misc/WvCommonUtils.h"
#include "Character/BaseCharacter.h"
#include "Component/CombatComponent.h"
#include "Component/InventoryComponent.h"
#include "GameExtension.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvCommonUtils)

FLSComponentAndTransform UWvCommonUtils::ComponentWorldToLocal(const FLSComponentAndTransform WorldSpaceComponent)
{
	FLSComponentAndTransform LocalSpaceComponent;
	LocalSpaceComponent.Component = WorldSpaceComponent.Component;
	// DisplayName GetWorldTransform
	LocalSpaceComponent.Transform = WorldSpaceComponent.Transform * UKismetMathLibrary::InvertTransform(WorldSpaceComponent.Component->K2_GetComponentToWorld());
	return LocalSpaceComponent;
}

FLSComponentAndTransform UWvCommonUtils::ComponentLocalToWorld(const FLSComponentAndTransform LocalSpaceComponent)
{
	FLSComponentAndTransform WorldSpaceComponent;
	WorldSpaceComponent.Component = LocalSpaceComponent.Component;
	WorldSpaceComponent.Transform = LocalSpaceComponent.Transform * WorldSpaceComponent.Component->K2_GetComponentToWorld();
	return WorldSpaceComponent;
}

FTransform UWvCommonUtils::TransformMinus(const FTransform A, const FTransform B)
{
	FTransform Out = FTransform::Identity;
	Out.SetLocation(A.GetLocation() - B.GetLocation());
	Out.SetScale3D(A.GetScale3D() - B.GetScale3D());

	const float Roll = (A.GetRotation().Rotator().Roll - B.GetRotation().Rotator().Roll);
	const float Pitch = (A.GetRotation().Rotator().Pitch - B.GetRotation().Rotator().Pitch);
	const float Yaw = (A.GetRotation().Rotator().Yaw - B.GetRotation().Rotator().Yaw);
	Out.SetRotation(FQuat(FRotator(Pitch, Yaw, Roll)));
	return Out;
}

FTransform UWvCommonUtils::TransformPlus(const FTransform A, const FTransform B)
{
	FTransform Out = FTransform::Identity;
	Out.SetLocation(A.GetLocation() + B.GetLocation());
	Out.SetScale3D(A.GetScale3D() + B.GetScale3D());

	const float Roll = (A.GetRotation().Rotator().Roll + B.GetRotation().Rotator().Roll);
	const float Pitch = (A.GetRotation().Rotator().Pitch + B.GetRotation().Rotator().Pitch);
	const float Yaw = (A.GetRotation().Rotator().Yaw + B.GetRotation().Rotator().Yaw);
	Out.SetRotation(FQuat(FRotator(Pitch, Yaw, Roll)));
	return Out;
}

bool UWvCommonUtils::IsHost(const AController* Controller)
{
	if (!IsValid(Controller))
	{
		UE_LOG(LogTemp, Error, TEXT("UWvCommonUtils::IsHost->Controller is nullptr"));
		return false;
	}
	return (Controller->HasAuthority() && Controller->IsLocalController() && !IsBot(Controller));
}

bool UWvCommonUtils::IsBot(const AController* Controller)
{
	if (!IsValid(Controller))
	{
		UE_LOG(LogTemp, Error, TEXT("UWvCommonUtils::IsBot -> Controller is nullptr"));
		return false;
	}

	const UClass* Class = Controller->GetClass();
	if (!IsValid(Class))
	{
		UE_LOG(LogTemp, Error, TEXT("UWvCommonUtils::IsBot -> Class is nullptr"));
		return false;
	}
	//Is a BOT
	if (!Class->IsChildOf(AAIController::StaticClass()))
	{
		return false;
	}
	return true;
}

bool UWvCommonUtils::IsBotPawn(AActor* Actor)
{
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		return IsBot(Pawn->GetController());
	}
	return false;
}

void UWvCommonUtils::CircleSpawnPoints(const int32 InSpawnCount, const float InRadius, const FVector InRelativeLocation, TArray<FVector>& OutPointArray)
{
	const float AngleDiff = 360.f / (float)InSpawnCount;
	for (int i = 0; i < InSpawnCount; ++i)
	{
		FVector Position = InRelativeLocation;
		float Ang = FMath::DegreesToRadians(90 - AngleDiff * i);
		Position.X += InRadius * FMath::Cos(Ang);
		Position.Y += InRadius * FMath::Sin(Ang);
		OutPointArray.Add(Position);
	}
}

bool UWvCommonUtils::IsInViewport(AActor* Actor)
{
	if (!IsBotPawn(Actor))
	{
		return true;
	}

	auto PC = UGameplayStatics::GetPlayerController(Actor->GetWorld(), 0);
	if (PC)
	{
		FVector2D ScreenLocation;
		PC->ProjectWorldLocationToScreen(Actor->GetActorLocation(), ScreenLocation);
		FVector2D ViewportSize;
		Actor->GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);
		return ScreenLocation.X > 0 && ScreenLocation.Y > 0 && ScreenLocation.X < ViewportSize.X&& ScreenLocation.Y < ViewportSize.Y;
	}
	return false;
}

float UWvCommonUtils::PlayerPawnToDistance(AActor* Actor)
{
	if (!IsBotPawn(Actor))
	{
		return 0.f;
	}

	auto Pawn = Game::ControllerExtension::GetPlayerPawn(Actor->GetWorld(), 0);
	if (Pawn)
	{
		return (Pawn->GetActorLocation() - Actor->GetActorLocation()).Size();
	}
	return 0.f;
}

bool UWvCommonUtils::IsInEditor()
{
	return GIsEditor;
}

void UWvCommonUtils::CreateDynamicMaterialInstance(UPrimitiveComponent* PrimitiveComponent, TArray<UMaterialInstanceDynamic*>& OutMaterialArray)
{
	const int32 MaterialNum = PrimitiveComponent->GetNumMaterials();
	for (int Index = 0; Index < MaterialNum; ++Index)
	{
		if (UMaterialInstanceDynamic* Instance = PrimitiveComponent->CreateDynamicMaterialInstance(Index, PrimitiveComponent->GetMaterial(Index)))
		{
			OutMaterialArray.Add(Instance);
		}
	}
}

FHitReactInfoRow* UWvCommonUtils::FindHitReactInfoRow(ABaseCharacter* Character)
{
	IWvAbilitySystemAvatarInterface* Avatar = Cast<IWvAbilitySystemAvatarInterface>(Character);
	if (!Avatar)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s : Avatar is null.]"), *FString(__FUNCTION__));
		return nullptr;
	}

	const FWvAbilitySystemAvatarData& AbilitySystemData = Avatar->GetAbilitySystemData();
	UWvHitReactDataAsset* HitReactDA = AbilitySystemData.HitReactData;
	UCombatComponent* CombatComponent = Character->GetCombatComponent();
	const UInventoryComponent* InventoryComponent = Character->GetInventoryComponent();

	if (!IsValid(CombatComponent) || !IsValid(HitReactDA) || !IsValid(InventoryComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("[%s : CombatComponent or HitReactDA or InventoryComponent is null.]"), *FString(__FUNCTION__));
		return nullptr;
	}

	const FGameplayTag HitReactFeatureTag = CombatComponent->GetHitReactFeature();
	const bool bIsFixed = CombatComponent->GetIsFixedHitReactFeature();
	const FGameplayTag WeaknesshitReactFeatureTag = CombatComponent->GetWeaknessHitReactFeature();
	CombatComponent->SetWeaknessHitReactFeature(FGameplayTag::EmptyTag);
	const FName WeaponName = InventoryComponent->GetEquipWeaponName();
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);
	FHitReactInfoRow* HitReactInfo = nullptr;
	if (WeaknesshitReactFeatureTag.IsValid())
	{
		HitReactInfo = const_cast<FHitReactInfoRow*>(HitReactDA->GetHitReactInfoRow_Special(ASC, WeaknesshitReactFeatureTag));
	}

	if (!HitReactInfo && bIsFixed)
	{
		HitReactInfo = const_cast<FHitReactInfoRow*>(HitReactDA->GetHitReactInfoRow_Special(ASC, HitReactFeatureTag));
	}

	if (!HitReactInfo && !WeaponName.IsNone())
	{
		HitReactInfo = const_cast<FHitReactInfoRow*>(HitReactDA->GetHitReactInfoRow_Weapon(WeaponName, ASC, HitReactFeatureTag));
	}

	if (!HitReactInfo)
	{
		HitReactInfo = const_cast<FHitReactInfoRow*>(HitReactDA->GetHitReactInfoRow_Normal(ASC, HitReactFeatureTag));
	}
	return HitReactInfo;
}

AActor* UWvCommonUtils::CloneActor(const AActor* InputActor)
{
	if (IsValid(InputActor))
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Template = const_cast<AActor*>(InputActor);
	AActor* SpawnedActor = InputActor->GetWorld()->SpawnActor<AActor>(InputActor->GetClass(), Params);
	return SpawnedActor;
}

float UWvCommonUtils::GetMeanValue(const TArray<float> Values)
{
	const int32 Size = Values.Num();
	float Sum = 0;
	for (int32 Index = 0; Index < Size; ++Index)
	{
		Sum += Values[Index];
	}
	return Sum / Size;
}

const bool UWvCommonUtils::IsNetworked(const AActor* Owner)
{
	if (!IsValid(Owner))
	{
		return false;
	}
	return UKismetSystemLibrary::IsStandalone(Owner->GetWorld());
}

float UWvCommonUtils::GetAngleBetweenVector(FVector Vec1, FVector Vec2)
{
	Vec1.Normalize();
	Vec2.Normalize();
	return FMath::RadiansToDegrees(FMath::Acos(Vec1 | Vec2));
}

float UWvCommonUtils::GetAngleBetween3DVector(FVector Vec1, FVector Vec2)
{
	Vec1.Normalize();
	Vec2.Normalize();
	float Angle = FMath::RadiansToDegrees(FMath::Acos(Vec1 | Vec2));
	if (FVector::DotProduct(FVector::CrossProduct(Vec1, Vec2), FVector::UpVector) < 0)
	{
		Angle = -Angle;
	}
	return Angle;
}

float UWvCommonUtils::GetAngleBetween3DVector(FVector Vec1, FVector Vec2, FVector RefUpVector)
{
	Vec1.Normalize();
	Vec2.Normalize();
	float Angle = UKismetMathLibrary::DegAcos(FVector::DotProduct(Vec1, Vec2));
	if (FVector::DotProduct(FVector::CrossProduct(Vec1, Vec2), RefUpVector) < 0)
	{
		Angle = -Angle;
	}
	return Angle;
}

FTransform UWvCommonUtils::TransformSubStract(const FTransform& TransformA, const FTransform& TransformB)
{
	FVector Location = TransformA.GetLocation() - TransformB.GetLocation();
	FVector Scale = TransformA.GetScale3D() - TransformB.GetScale3D();
	FQuat Rotation = TransformA.GetRotation() - TransformA.GetRotation();
	return FTransform(Rotation, Location, Scale);
}

FTransform UWvCommonUtils::TransformAdd(const FTransform& TransformA, const FTransform& TransformB)
{
	FVector Location = TransformA.GetTranslation() + TransformB.GetTranslation();
	FVector Scale = TransformA.GetScale3D() + TransformB.GetScale3D();
	FQuat Rotation = TransformA.GetRotation() + TransformB.GetRotation();
	return FTransform(Rotation, Location, Scale);
}

UFXSystemComponent* UWvCommonUtils::SpawnParticleAtLocation(const UObject* WorldContextObject, UFXSystemAsset* Particle, FVector Location, FRotator Rotation, FVector Scale)
{
	if (Particle)
	{
		if (Particle->IsA(UNiagaraSystem::StaticClass()))
		{
			return UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContextObject, Cast<UNiagaraSystem>(Particle), Location, Rotation, Scale);
		}
	}
	return nullptr;
}

UFXSystemComponent* UWvCommonUtils::SpawnParticleAttached(UFXSystemAsset* Particle, USceneComponent* Component, FName BoneName, FVector Location, FRotator Rotation, FVector Scale, EAttachLocation::Type LocationType)
{
	if (Particle)
	{
		if (Particle->IsA(UNiagaraSystem::StaticClass()))
		{
			return UNiagaraFunctionLibrary::SpawnSystemAttached(Cast<UNiagaraSystem>(Particle), Component, BoneName, Location, Rotation, Scale, LocationType, true, ENCPoolMethod::None);
		}
	}
	return nullptr;
}

bool UWvCommonUtils::GetBoneTransForm(const USkeletalMeshComponent* MeshComp, const FName BoneName, FTransform& OutBoneTransform)
{
	const int32 BoneIndex = MeshComp->GetBoneIndex(BoneName);
	if (BoneIndex != INDEX_NONE)
	{
		OutBoneTransform = MeshComp->GetBoneTransform(BoneIndex);
		return true;
	}
	else
	{
		const USkeletalMeshSocket* Socket = MeshComp->GetSocketByName(BoneName);
		if (Socket)
		{
			OutBoneTransform = MeshComp->GetSocketTransform(BoneName);
			return true;
		}
	}
	return false;
}

const FString UWvCommonUtils::NormalizeFileName(const char* String)
{
	return NormalizeFileName(FString(UTF8_TO_TCHAR(String)));
}

const FString UWvCommonUtils::NormalizeFileName(const FString& String)
{
	FString Ret = String;
	FText ErrorText;

	if (!FName::IsValidXName(*Ret, INVALID_OBJECTNAME_CHARACTERS INVALID_LONGPACKAGE_CHARACTERS, &ErrorText))
	{
		FString InString = INVALID_OBJECTNAME_CHARACTERS;
		InString += INVALID_LONGPACKAGE_CHARACTERS;
		const TArray<TCHAR> CharArray = InString.GetCharArray();
		for (int32 Index = 0; Index < CharArray.Num(); ++Index)
		{
			FString Template;
			Template.AppendChar(CharArray[Index]);
			Ret = Ret.Replace(Template.GetCharArray().GetData(), TEXT("_"));
		}
	}
	return Ret;
}

bool UWvCommonUtils::Probability(const float InPercent)
{
	const float ProbabilityRate = FMath::RandRange(0.f, 100.0f);
	if (InPercent == 100.0f && ProbabilityRate == InPercent)
	{
		return true;
	}
	else if (ProbabilityRate < InPercent)
	{
		return true;
	}
	return false;
}

const FName UWvCommonUtils::GetSurfaceName(TEnumAsByte<EPhysicalSurface> SurfaceType)
{
	if (SurfaceType == SurfaceType_Default)
	{
		return TEXT("Default");
	}

	const FPhysicalSurfaceName* FoundSurface = UPhysicsSettings::Get()->PhysicalSurfaces.FindByPredicate([&](const FPhysicalSurfaceName& SurfaceName)
	{
		return SurfaceName.Type == SurfaceType;
	});

	if (FoundSurface)
	{
		return FoundSurface->Name;
	}

	return TEXT("Default");
}

AActor* UWvCommonUtils::FindNearestDistanceTarget(AActor* Owner, TArray<AActor*> Actors, const float ClosestTargetDistance)
{
	// From the hit actors, check distance and return the nearest
	if (Actors.Num() <= 0)
	{
		return nullptr;
	}

	float ClosestDistance = ClosestTargetDistance;
	AActor* Target = nullptr;
	for (int32 Index = 0; Index < Actors.Num(); ++Index)
	{
		const float Distance2D = (Owner->GetActorLocation() - Actors[Index]->GetActorLocation()).Size2D();
		UE_LOG(LogTemp, Log, TEXT("ClosestDistance => %.3f, Distance2D => %.3f"), ClosestDistance, Distance2D);

		if (Distance2D < ClosestDistance)
		{
			ClosestDistance = Distance2D;
			Target = Actors[Index];
		}
	}
	return Target;
}

