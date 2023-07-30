// Copyright 2022 wevet works All Rights Reserved.


#include "WvFootstepAnimNotify.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"


void UWvFootstepAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	TraceFoot(MeshComp, Animation);
}

FName UWvFootstepAnimNotify::GetSurfaceName(TEnumAsByte<EPhysicalSurface> SurfaceType) const
{
	if (SurfaceType == SurfaceType_Default)
	{
		return TEXT("Default");
	}
	else if (const FPhysicalSurfaceName* FoundSurface = UPhysicsSettings::Get()->PhysicalSurfaces.FindByPredicate(
		[&](const FPhysicalSurfaceName& SurfaceName)
	{
		return SurfaceName.Type == SurfaceType;
	}))
	{
		return FoundSurface->Name;
	}
	return TEXT("Default");
}

void UWvFootstepAnimNotify::TraceFoot(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		// 足元下方向への距離はアセット依存なのでプロパティ化
		const FVector SocketLocation = MeshComp->GetSocketLocation(SocketName);
		// 地面へのめり込みを考慮して開始地点は少し浮かす
		const FVector TraceBegin = SocketLocation + FVector::UpVector * TraceBeginDistance;
		const FVector TraceEnd = SocketLocation + FVector::DownVector * TraceEndDistance;

		FCollisionQueryParams TraceParams(NAME_None, false, OwnerActor);
		TraceParams.bReturnPhysicalMaterial = true;

		UWorld* World = OwnerActor->GetWorld();
		if (World->IsGameWorld())
		{
			FTraceDelegate TraceFootDelegate;
			TraceFootDelegate.BindUObject(this, &UWvFootstepAnimNotify::TraceFootDone, MeshComp, Animation);
			World->AsyncLineTraceByChannel(
				EAsyncTraceType::Single, 
				TraceBegin, 
				TraceEnd,
				ECC_Visibility, 
				TraceParams,
				FCollisionResponseParams::DefaultResponseParam,
				&TraceFootDelegate);

			//DrawDebugLine(World, TraceBegin, TraceEnd, FColor::Red, false, 1.f);
		}
#if WITH_EDITOR
		else
		{
			FHitResult HitResult;
			if (World->LineTraceSingleByChannel(HitResult, TraceBegin, TraceEnd, ECC_Visibility, TraceParams))
			{
				FVector HitLocation = HitResult.Location;
				EPhysicalSurface HitSurfaceType = SurfaceTypeInEditor;
				TriggerEffect(OwnerActor, Animation, HitLocation, HitSurfaceType);
			}
			else
			{
				DrawDebugLine(World, TraceBegin, TraceEnd, FColor::Red, false, 1.f);
			}
		}
#endif
	}
}

void UWvFootstepAnimNotify::TraceFootDone(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum, USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (TraceDatum.OutHits.Num() == 0)
	{
		return;
	}

	if (AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		const FHitResult& HitResult = TraceDatum.OutHits[0];
		FVector HitLocation = HitResult.Location;
		EPhysicalSurface HitSurfaceType = HitResult.PhysMaterial.IsValid() ? HitResult.PhysMaterial->SurfaceType.GetValue() : EPhysicalSurface::SurfaceType_Default;
		TriggerEffect(OwnerActor, Animation, HitLocation, HitSurfaceType);
	}
}

void UWvFootstepAnimNotify::TriggerEffect(AActor* Owner, UAnimSequenceBase* Animation, const FVector Location, const TEnumAsByte<EPhysicalSurface> SurfaceType)
{
	FString SurfaceName = GetSurfaceName(SurfaceType).ToString();

	const ELSGait GaitMode = GetGaitMode(Owner);
	FString GaitModeName = TEXT(".Running");
	if (GaitMode == ELSGait::Walking)
	{
		GaitModeName = TEXT(".Walking");
	}

	SurfaceName.Append(GaitModeName);

	if (FootStepDT)
	{
		auto RowData = FootStepDT->FindRow<FFootStepTableRow>(FName(SurfaceName), "");
		if (RowData)
		{
			UWorld* World = Owner->GetWorld();
			if (RowData->NiagaraSystems)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, RowData->NiagaraSystems, Location);
			}

			if (RowData->FootStepSound)
			{
				UGameplayStatics::PlaySoundAtLocation(World, RowData->FootStepSound, Location, Volume, 1.0f, 0.0f, nullptr, nullptr);
			}
		}
	}

	//UE_LOG(LogTemp, Log, TEXT("SurfaceName => %s"), *SurfaceName);
}


const ELSGait UWvFootstepAnimNotify::GetGaitMode(AActor* Owner)
{
	if (IsValid(Owner))
	{
		auto LocomotionComponent = Owner->FindComponentByClass<ULocomotionComponent>();
		if (LocomotionComponent)
		{
			return LocomotionComponent->GetLSGaitMode_Implementation();
		}
	}
	return ELSGait::Running;
}

