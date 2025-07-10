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
	// 生成するアイテムの型は Point（位置）とする
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
		// 負荷が高くなるので使わない方がいい？
		//LocalTraceRange = FMath::Max((CombatComp->GetWeaponAttackInfo().TraceRange / 2.0f), TraceRange);
	}

	// ContextLocations の各位置について 1 回の SphereTraceMulti を実行し、複数ヒットを取得 ---
	for (const FVector& BaseLoc : ContextLocations)
	{
		// Trace 開始位置を少し下げる
		const FVector RayStartBase = BaseLoc + FVector(0.f, 0.f, StartZOffset);

		// SphereTrace（SweepMultiByChannel）―― 基準位置を中心に、半径 TraceRange の球を拡張してヒットを取得
		TArray<FHitResult> HitResults;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(OwnerActor);

		const bool bDidHitAny = World->SweepMultiByChannel(HitResults, RayStartBase, RayStartBase,
			FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(LocalTraceRange), Params);

		if (bDidHitAny)
		{
			// --- 3) HitResults 内のすべての結果を、それぞれ隠れ候補として追加 ---
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

				// 3-1) ヒットした Actor が ACharacter（カプセルコンポーネントを持つキャラ）の場合
				if (ACharacter* Char = Cast<ACharacter>(HitActor))
				{
					// カプセルのスケール後半径を取得
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
					// 3-2) ヒットした Actor が APawn（乗り物など） の場合
					// コンポーネントのバウンディングボックスのエクステント（半分サイズ）を取得
					const FVector BoxExtent = Pawn->GetComponentsBoundingBox().GetExtent();
					// 半分サイズベクトルの長さを使って押し戻し量を計算
					const float ExtentSize = BoxExtent.Size();
					ChosenLoc = ImpactPoint + ImpactNormal * ExtentSize;
				}
				else
				{
					// 3-3) それ以外のアクター（壁や床など静的ジオメトリ）の場合
					ChosenLoc = ImpactPoint + ImpactNormal * OffsetFromWall;
				}

				// 4) NavMesh に載せ直して有効床にスナップ（必要に応じて）
				FNavLocation NavLoc;
				if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
				{
					if (NavSys->ProjectPointToNavigation(ChosenLoc, NavLoc))
					{
						ChosenLoc = NavLoc.Location;
					}
				}

				// 5) EQS に隠れ候補位置を追加
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
			// --- 3-4) ヒットが一切なかった場合、最遠点だけを追加 ---
			// ここでは「基準位置の X 軸正方向に伸ばした最遠点」を候補にする例
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

