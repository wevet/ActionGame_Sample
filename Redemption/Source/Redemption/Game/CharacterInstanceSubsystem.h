// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Character/BaseCharacter.h"
#include "CharacterInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API UCharacterInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	

public:
	UCharacterInstanceSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static UCharacterInstanceSubsystem* Get();

	UFUNCTION(BlueprintCallable, Category = CharacterInstanceSubsystem)
	void FreezeAlCharacters(bool bFindWorldActorIterator = false);

	UFUNCTION(BlueprintCallable, Category = CharacterInstanceSubsystem)
	void UnFreezeAlCharacters(bool bFindWorldActorIterator = false);

	UFUNCTION(BlueprintCallable, Category = CharacterInstanceSubsystem)
	void DoForceKill(bool bFindWorldActorIterator = false);

	void AssignAICharacter(ABaseCharacter* NewCharacter);
	void RemoveAICharacter(ABaseCharacter* InCharacter);
	void GeneratorSpawnedFinish();

	bool IsInEnemyAgent(const ABaseCharacter* Other) const;
	bool IsInFriendAgent(const ABaseCharacter* Other) const;
	bool IsInNeutralAgent(const ABaseCharacter* Other) const;

	TArray<ABaseCharacter*> GetLeaderAgent() const;

	void SendDayNightPhaseCharacter(const uint8 InDayNightPhase, bool bFindWorldActorIterator = false);

private:
	UPROPERTY()
	TArray<ABaseCharacter*> Characters;

	static UCharacterInstanceSubsystem* Instance;

	void UpdateCharacterInWorld();
	void WorldCharacterIterator(TArray<class ABaseCharacter*>& OutCharacterArray);

};
