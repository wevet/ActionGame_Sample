// Copyright 2022 wevet works All Rights Reserved.


#include "EQHidingPointGenerator.h"
#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"
#include "Component/CombatComponent.h"

#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "CollisionQueryParams.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"


UEQHidingPointGenerator::UEQHidingPointGenerator()
{
	// ��������A�C�e���̌^�� Point�i�ʒu�j�Ƃ���
	ItemType = UEnvQueryItemType_Point::StaticClass();

	TraceRange = 1000.f;
	StartZOffset = -10.f;
	OffsetFromWall = 50.f;
	TraceChannel = ECC_Visibility;
}

FText UEQHidingPointGenerator::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("HidingPoint: around Context"));
}

FText UEQHidingPointGenerator::GetDescriptionDetails() const
{
	return FText::Format(NSLOCTEXT("EQS", "HidingPointsDetails", "TraceRange={1}"), FText::AsNumber(TraceRange));
}


void UEQHidingPointGenerator::UpdateHidingPoints(const TArray<FVector>& ContextLocations, TArray<FVector>& OutPositions) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ABaseCharacter* OwnerActor = Cast<ABaseCharacter>(GetQuerier());
	if (!OwnerActor)
	{
		return;
	}

	OutPositions.Reset(0);

	float LocalTraceRange = TraceRange;
	UCombatComponent* CombatComp = OwnerActor->FindComponentByClass<UCombatComponent>();
	if (CombatComp)
	{
		// @NOTE
		// ���ׂ������Ȃ�̂Ŏg��Ȃ����������H
		//LocalTraceRange = FMath::Max((CombatComp->GetWeaponAttackInfo().TraceRange / 2.0f), TraceRange);
	}

	// ContextLocations �̊e�ʒu�ɂ��� 1 ��� SphereTraceMulti �����s���A�����q�b�g���擾 ---
	for (const FVector& BaseLoc : ContextLocations)
	{
		// Trace �J�n�ʒu������������
		const FVector RayStartBase = BaseLoc + FVector(0.f, 0.f, StartZOffset);

		// SphereTrace�iSweepMultiByChannel�j�\�\ ��ʒu�𒆐S�ɁA���a TraceRange �̋����g�����ăq�b�g���擾
		TArray<FHitResult> HitResults;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(OwnerActor);

		const bool bDidHitAny = World->SweepMultiByChannel(HitResults, RayStartBase, RayStartBase,
			FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(LocalTraceRange), Params);

		if (bDidHitAny)
		{
			// --- 3) HitResults ���̂��ׂĂ̌��ʂ��A���ꂼ��B����Ƃ��Ēǉ� ---
			for (const FHitResult& Hit : HitResults)
			{
				AActor* HitActor = Hit.GetActor();
				if (!HitActor)
				{
					continue;
				}

				FVector ImpactNormal = Hit.ImpactNormal;
				FVector ImpactPoint = Hit.ImpactPoint;
				FVector ChosenLoc;

				// 3-1) �q�b�g���� Actor �� ACharacter�i�J�v�Z���R���|�[�l���g�����L�����j�̏ꍇ
				if (ACharacter* Char = Cast<ACharacter>(HitActor))
				{
					// �J�v�Z���̃X�P�[���㔼�a���擾
					if (UCapsuleComponent* CapsuleComp = Char->GetCapsuleComponent())
					{
						const float CapsuleRadius = CapsuleComp->GetScaledCapsuleRadius();
						ChosenLoc = ImpactPoint + ImpactNormal * CapsuleRadius;
					}
					else
					{
						ChosenLoc = ImpactPoint + ImpactNormal * OffsetFromWall;
					}
				}
				else if (APawn* Pawn = Cast<APawn>(HitActor))
				{
					// 3-2) �q�b�g���� Actor �� APawn�i��蕨�Ȃǁj �̏ꍇ
					// �R���|�[�l���g�̃o�E���f�B���O�{�b�N�X�̃G�N�X�e���g�i�����T�C�Y�j���擾
					const FVector BoxExtent = Pawn->GetComponentsBoundingBox().GetExtent();
					// �����T�C�Y�x�N�g���̒������g���ĉ����߂��ʂ��v�Z
					const float ExtentSize = BoxExtent.Size();
					ChosenLoc = ImpactPoint + ImpactNormal * ExtentSize;
				}
				else
				{
					// 3-3) ����ȊO�̃A�N�^�[�i�ǂ⏰�ȂǐÓI�W�I���g���j�̏ꍇ
					ChosenLoc = ImpactPoint + ImpactNormal * OffsetFromWall;
				}

				// 4) NavMesh �ɍڂ������ėL�����ɃX�i�b�v�i�K�v�ɉ����āj
				FNavLocation NavLoc;
				if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
				{
					if (NavSys->ProjectPointToNavigation(ChosenLoc, NavLoc))
					{
						ChosenLoc = NavLoc.Location;
					}
				}

				// 5) EQS �ɉB����ʒu��ǉ�
				OutPositions.Add(ChosenLoc);

				if (bIsShowTrace)
				{
					//DrawDebugSphere(World, ImpactPoint, TraceRadius, 8, FColor::Red, false, TraceTime);
					DrawDebugPoint(World, ChosenLoc, 6.f, FColor::Green, false, TraceTime);
				}
			}
		}
		else
		{
			// --- 3-4) �q�b�g����؂Ȃ������ꍇ�A�ŉ��_������ǉ� ---
			// �����ł́u��ʒu�� X ���������ɐL�΂����ŉ��_�v�����ɂ����
			const FVector RayEndPoint = RayStartBase + FVector(1.f, 0.f, 0.f) * TraceRange;
			OutPositions.Add(RayEndPoint);

			if (bIsShowTrace)
			{
				//DrawDebugSphere(World, RayStartBase, TraceRange, 12, FColor::Blue, false, TraceTime);
				DrawDebugPoint(World, RayEndPoint, 6.f, FColor::Yellow, false, TraceTime);

			}
		}
	}

}

