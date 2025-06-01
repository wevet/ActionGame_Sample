// Copyright 2022 wevet works All Rights Reserved.


#include "AI/EQS/PawnAvoidContext.h"
#include "Character/WvAIController.h"
#include "Character/BaseCharacter.h"
#include "Locomotion/LocomotionComponent.h"


#include "EnvironmentQuery/EnvQueryTypes.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "AITypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "EnvironmentQuery/EnvQueryManager.h"



void UPawnAvoidContext::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	// 1) クエリ実行時に選択された「元の候補チェーン」を取得
	TArray<FVector> RawLocations;
	//QueryInstance.PrepareContext(RawLocations);

	UWorld* World = QueryInstance.World;
	if (!World)
	{
		return;
	}

	// 2) 各 RawLocation に対して Pawn の下にいるかチェック
	for (const FVector& Loc : RawLocations)
	{
		// Trace の開始/終了地点を計算
		const FVector Start = Loc + FVector(0, 0, TraceUpHeight);
		const FVector End = Loc - FVector(0, 0, TraceDownLength);

		FHitResult HitResult;
		FCollisionQueryParams Params;
		// クエリ対象自身（Querier）を無視
		Params.AddIgnoredActor(Cast<AActor>(QueryInstance.Owner));

		const bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

		if (bHit && HitResult.GetActor() && HitResult.GetActor()->IsA<APawn>())
		{
			// Hit した Actor が Pawn（乗り物含む）だった場合
			// 地面ではなく Pawn のサーフェスの真上にいるので、少し押し戻した位置を計算
			const FVector NewLoc = HitResult.ImpactPoint + HitResult.ImpactNormal * PushBackDistance;

			// オプション：NavMesh 上に戻すために Z を地面レベルにスナップしたい場合は、
			// NavMeshQuery などで NewLoc の Z を補正することもできます。

			//ContextData.RegisterLocation(NewLoc);
		}
		else
		{
			// Hit なし、または Pawn でない場合はそのまま候補位置として登録
			//ContextData.RegisterLocation(Loc);
		}
	}
}


