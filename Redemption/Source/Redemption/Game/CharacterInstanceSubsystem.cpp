// Copyright 2022 wevet works All Rights Reserved.


#include "Game/CharacterInstanceSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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
		TArray<ABaseCharacter*> Array;
		WorldCharacterIterator(Array);
		Characters += Array;

		Characters.RemoveAll([](ABaseCharacter* Character)
		{
			return Character == nullptr;
		});
	}

	for (ABaseCharacter* Character : Characters)
	{
		if (Character)
		{
			Character->Freeze();
		}
	}
}

void UCharacterInstanceSubsystem::UnFreezeAlCharacters(bool bFindWorldActorIterator/* = false*/)
{
	if (bFindWorldActorIterator)
	{
		TArray<ABaseCharacter*> Array;
		WorldCharacterIterator(Array);
		Characters += Array;

		Characters.RemoveAll([](ABaseCharacter* Character)
		{
			return Character == nullptr;
		});
	}

	for (ABaseCharacter* Character : Characters)
	{
		if (Character)
		{
			Character->UnFreeze();
		}
	}
}

void UCharacterInstanceSubsystem::AssignAICharacter(ABaseCharacter* NewCharacter)
{
	if (!Characters.Contains(NewCharacter))
	{
		Characters.Add(NewCharacter);
	}
}

void UCharacterInstanceSubsystem::RemoveAICharacter(ABaseCharacter* InCharacter)
{
	if (Characters.Contains(InCharacter))
	{
		Characters.Remove(InCharacter);
	}
}

