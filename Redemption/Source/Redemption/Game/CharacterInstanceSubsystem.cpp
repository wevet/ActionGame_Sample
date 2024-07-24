// Copyright 2022 wevet works All Rights Reserved.


#include "Game/CharacterInstanceSubsystem.h"
#include "Character/WvAIController.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Misc/WvCommonUtils.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// plugin
#include "IAnimationBudgetAllocator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CharacterInstanceSubsystem)

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

		IAnimationBudgetAllocator::Get(GetWorld())->RegisterComponent(NewCharacter->GetWvSkeletalMeshComponent());
	}
}

void UCharacterInstanceSubsystem::RemoveAICharacter(ABaseCharacter* InCharacter)
{
	if (Characters.Contains(InCharacter))
	{
		Characters.Remove(InCharacter);

		IAnimationBudgetAllocator::Get(GetWorld())->UnregisterComponent(InCharacter->GetWvSkeletalMeshComponent());
	}
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

