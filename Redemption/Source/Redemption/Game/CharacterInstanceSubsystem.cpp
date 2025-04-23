// Copyright 2022 wevet works All Rights Reserved.


#include "Game/CharacterInstanceSubsystem.h"
#include "Character/WvAIController.h"
#include "Component/WvSkeletalMeshComponent.h"
#include "Mission/MinimapMarkerComponent.h"

#include "Misc/WvCommonUtils.h"
#include "Redemption.h"
#include "GameExtension.h"

#include "Engine/World.h"
#include "EngineUtils.h"

// plugin
#include "IAnimationBudgetAllocator.h"
#include "SignificanceManager.h"

using namespace Game;

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
	//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));
}

void UCharacterInstanceSubsystem::Deinitialize()
{
	//UE_LOG(LogTemp, Log, TEXT("%s"), *FString(__FUNCTION__));

	Characters.Reset(0);
}

void UCharacterInstanceSubsystem::WorldCharacterIterator(TArray<class ABaseCharacter*>& OutCharacterArray)
{
	ArrayExtension::WorldActorIterator<ABaseCharacter>(GetWorld(), OutCharacterArray);

	OutCharacterArray.RemoveAll([](ABaseCharacter* Character)
	{
		return !IsValid(Character);
	});
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
	if (IsValid(NewCharacter))
	{
		if (!Characters.Contains(NewCharacter))
		{
			Characters.Add(NewCharacter);
		}
	}


}

void UCharacterInstanceSubsystem::RemoveAICharacter(ABaseCharacter* InCharacter)
{
	if (IsValid(InCharacter))
	{
		if (Characters.Contains(InCharacter))
		{
			Characters.Remove(InCharacter);
		}
	}

}

TArray<UWvSkeletalMeshComponent*> UCharacterInstanceSubsystem::GetSkelMeshComponents(const ABaseCharacter* InCharacter) const
{
	auto Components = ComponentExtension::GetComponentsArray<UWvSkeletalMeshComponent>(InCharacter);
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


TArray<ABaseCharacter*> UCharacterInstanceSubsystem::GetPOIActors() const
{

	TArray<ABaseCharacter*> Filtered;
	ArrayExtension::FilterArray(Characters, Filtered, [](const ABaseCharacter* Char)
	{
		if (const auto* Marker = Char->GetMinimapMarkerComponent())
		{
			return Marker->IsVisibleMakerTag();
		}
		return false;
	});

	return Filtered;
}



