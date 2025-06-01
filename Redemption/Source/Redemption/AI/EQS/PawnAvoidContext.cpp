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
	// 1) �N�G�����s���ɑI�����ꂽ�u���̌��`�F�[���v���擾
	TArray<FVector> RawLocations;
	//QueryInstance.PrepareContext(RawLocations);

	UWorld* World = QueryInstance.World;
	if (!World)
	{
		return;
	}

	// 2) �e RawLocation �ɑ΂��� Pawn �̉��ɂ��邩�`�F�b�N
	for (const FVector& Loc : RawLocations)
	{
		// Trace �̊J�n/�I���n�_���v�Z
		const FVector Start = Loc + FVector(0, 0, TraceUpHeight);
		const FVector End = Loc - FVector(0, 0, TraceDownLength);

		FHitResult HitResult;
		FCollisionQueryParams Params;
		// �N�G���Ώێ��g�iQuerier�j�𖳎�
		Params.AddIgnoredActor(Cast<AActor>(QueryInstance.Owner));

		const bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params);

		if (bHit && HitResult.GetActor() && HitResult.GetActor()->IsA<APawn>())
		{
			// Hit ���� Actor �� Pawn�i��蕨�܂ށj�������ꍇ
			// �n�ʂł͂Ȃ� Pawn �̃T�[�t�F�X�̐^��ɂ���̂ŁA���������߂����ʒu���v�Z
			const FVector NewLoc = HitResult.ImpactPoint + HitResult.ImpactNormal * PushBackDistance;

			// �I�v�V�����FNavMesh ��ɖ߂����߂� Z ��n�ʃ��x���ɃX�i�b�v�������ꍇ�́A
			// NavMeshQuery �Ȃǂ� NewLoc �� Z ��␳���邱�Ƃ��ł��܂��B

			//ContextData.RegisterLocation(NewLoc);
		}
		else
		{
			// Hit �Ȃ��A�܂��� Pawn �łȂ��ꍇ�͂��̂܂܌��ʒu�Ƃ��ēo�^
			//ContextData.RegisterLocation(Loc);
		}
	}
}


