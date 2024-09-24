// Copyright 2022 wevet works All Rights Reserved.


#include "Game/CharacterInstanceSubsystem.h"
#include "Character/WvAIController.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Redemption.h"
#include "GameExtension.h"

#include "Engine/World.h"
#include "EngineUtils.h"

// plugin
#include "IAnimationBudgetAllocator.h"
#include "SignificanceManager.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(CharacterInstanceSubsystem)

float GSignificanceDistance = 15000.0f;

float MySignificanceFunction(USignificanceManager::FManagedObjectInfo* Obj, const FTransform& InTransform)
{
	// Note that the function is called in parallel processing by ParallelFor from SignificanceManager.
	if (ABaseCharacter* Actor = Cast<ABaseCharacter>(Obj->GetObject()))
	{
		// Significance calculation based on distance between player and self
		float Significance = 0.0f;
		const FVector Distance = InTransform.GetLocation() - Actor->GetActorLocation();
		if (Distance.Size() < GSignificanceDistance)
		{
			Significance = 1.f - Distance.Size() / GSignificanceDistance;
		}
		return Significance;
	}
	return 0.f;
}

void MyPostSignificanceFunction(USignificanceManager::FManagedObjectInfo* Obj, float OldSignificance, float Significance, bool bUnregistered)
{
	if (ABaseCharacter* Actor = Cast<ABaseCharacter>(Obj->GetObject()))
	{
		// @TODO tempolaly function
		if (Significance > 0.f)
		{
			Actor->SetActorTickEnabled(true);
		}
		else
		{
			Actor->SetActorTickEnabled(false);
		}
	}
}

UCharacterInstanceSubsystem* UCharacterInstanceSubsystem::Instance = nullptr;

UCharacterInstanceSubsystem::UCharacterInstanceSubsystem()
{
	UCharacterInstanceSubsystem::Instance = this;
}

UCharacterInstanceSubsystem* UCharacterInstanceSubsystem::Get()
{
	return Instance;
}

void UCharacterInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void UCharacterInstanceSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

	if (USignificanceManager* SignificanceManager = USignificanceManager::Get(GetWorld()))
	{
		SignificanceManager->UnregisterAll(K_SIGNIGICANCE_ACTOR);
	}
	Characters.Reset(0);
}

void UCharacterInstanceSubsystem::WorldCharacterIterator(TArray<class ABaseCharacter*>& OutCharacterArray)
{
	auto World = GetWorld();
	for (TActorIterator<ABaseCharacter> It(World); It; ++It)
	{
		ABaseCharacter* Character = Cast<ABaseCharacter>(*It);
		if (Character == nullptr)
		{
			continue;
		}
		OutCharacterArray.AddUnique(Character);
	}
}

void UCharacterInstanceSubsystem::FreezeAlCharacters(bool bFindWorldActorIterator/* = false*/)
{
	if (bFindWorldActorIterator)
	{
		UpdateCharacterInWorld();
	}

	for (ABaseCharacter* Character : Characters)
	{
		if (IsValid(Character))
		{
			Character->Freeze();
		}
	}
}

void UCharacterInstanceSubsystem::UnFreezeAlCharacters(bool bFindWorldActorIterator/* = false*/)
{
	if (bFindWorldActorIterator)
	{
		UpdateCharacterInWorld();
	}

	for (ABaseCharacter* Character : Characters)
	{
		if (IsValid(Character))
		{
			Character->UnFreeze();
		}
	}
}

void UCharacterInstanceSubsystem::DoForceKill(bool bFindWorldActorIterator/* = false*/)
{
	if (bFindWorldActorIterator)
	{
		UpdateCharacterInWorld();
	}

	for (ABaseCharacter* Character : Characters)
	{
		if (IsValid(Character) && !Character->IsDead())
		{
			Character->DoForceKill();
		}
	}
}

void UCharacterInstanceSubsystem::DoForceKillIgnorePlayer(bool bFindWorldActorIterator/* = false*/)
{
	if (bFindWorldActorIterator)
	{
		UpdateCharacterInWorld();
	}

	Characters.RemoveAll([](ABaseCharacter* Character)
	{
		return !Character->IsBotCharacter();
	});

	for (ABaseCharacter* Character : Characters)
	{
		if (IsValid(Character))
		{
			Character->DoForceKill();
		}
	}
}

void UCharacterInstanceSubsystem::AssignAICharacter(ABaseCharacter* NewCharacter)
{
	if (!Characters.Contains(NewCharacter))
	{
		Characters.Add(NewCharacter);


		auto Components = GetSkelMeshComponents(NewCharacter);
		for (UWvSkeletalMeshComponent* ActComp : Components)
		{
			IAnimationBudgetAllocator::Get(GetWorld())->RegisterComponent(ActComp);
		}

		if (USignificanceManager* SignificanceManager = USignificanceManager::Get(GetWorld()))
		{
			//const auto Tag = NewCharacter->GetAvatarTag().GetTagName();
			SignificanceManager->RegisterObject(NewCharacter, K_SIGNIGICANCE_ACTOR, MySignificanceFunction, USignificanceManager::EPostSignificanceType::Sequential, MyPostSignificanceFunction);
		}
	}
}

void UCharacterInstanceSubsystem::RemoveAICharacter(ABaseCharacter* InCharacter)
{
	if (Characters.Contains(InCharacter))
	{
		Characters.Remove(InCharacter);

		auto Components = GetSkelMeshComponents(InCharacter);
		for (UWvSkeletalMeshComponent* ActComp : Components)
		{
			IAnimationBudgetAllocator::Get(GetWorld())->UnregisterComponent(ActComp);
		}

		if (USignificanceManager* SignificanceManager = USignificanceManager::Get(GetWorld()))
		{
			SignificanceManager->UnregisterObject(InCharacter);
		}
	}
}

TArray<UWvSkeletalMeshComponent*> UCharacterInstanceSubsystem::GetSkelMeshComponents(const ABaseCharacter* InCharacter) const
{
	auto Components = Game::ComponentExtension::GetComponentsArray<UWvSkeletalMeshComponent>(InCharacter);
	Components.RemoveAll([](UWvSkeletalMeshComponent* SkelMesh)
	{
		return SkelMesh == nullptr;
	});

	return Components;
}

bool UCharacterInstanceSubsystem::IsInEnemyAgent(const ABaseCharacter* Other) const
{
	if (AWvAIController* AICtrl = Cast<AWvAIController>(Other->GetController()))
	{
		return AICtrl->IsInEnemyAgent(*Other);
	}
	return false;
}

bool UCharacterInstanceSubsystem::IsInFriendAgent(const ABaseCharacter* Other) const
{
	if (AWvAIController* AICtrl = Cast<AWvAIController>(Other->GetController()))
	{
		return AICtrl->IsInFriendAgent(*Other);
	}
	return false;
}

bool UCharacterInstanceSubsystem::IsInNeutralAgent(const ABaseCharacter* Other) const
{
	if (AWvAIController* AICtrl = Cast<AWvAIController>(Other->GetController()))
	{
		return AICtrl->IsInNeutralAgent(*Other);
	}
	return true;
}

bool UCharacterInstanceSubsystem::IsInBattleAny() const
{
	for (const ABaseCharacter* Character : Characters)
	{
		if (Character->IsInBattled())
		{
			return true;
		}
	}
	return false;
}

TArray<ABaseCharacter*> UCharacterInstanceSubsystem::GetLeaderAgent() const
{
	TArray<ABaseCharacter*> Result = Characters;
	Result.RemoveAll([](ABaseCharacter* Character) 
	{
		return !Character->IsLeader();
	});

	return Result;
}

void UCharacterInstanceSubsystem::GeneratorSpawnedFinish()
{
}

void UCharacterInstanceSubsystem::UpdateCharacterInWorld()
{
	TArray<ABaseCharacter*> Array;
	WorldCharacterIterator(Array);
	Characters += Array;

	Characters.RemoveAll([](ABaseCharacter* Character)
	{
		return IsValid(Character) == false;
	});
}

void UCharacterInstanceSubsystem::StartCinematicCharacter(ABaseCharacter* InCharacter)
{
	RemoveAICharacter(InCharacter);

	if (IsValid(InCharacter))
	{
		InCharacter->DoStartCinematic();
	}
}

void UCharacterInstanceSubsystem::StopCinematicCharacter(ABaseCharacter* InCharacter)
{
	AssignAICharacter(InCharacter);

	if (IsValid(InCharacter))
	{
		InCharacter->DoStopCinematic();
	}
}


