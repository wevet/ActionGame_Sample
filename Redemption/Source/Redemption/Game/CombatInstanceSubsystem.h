// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Game/WvGameInstance.h"
#include "Engine/DataTable.h"
#include "CombatInstanceSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FHitReactEnvironmentRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class UNiagaraSystem* HitEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	class USoundBase* HitSound;
};

/**
 * 
 */
UCLASS()
class REDEMPTION_API UCombatInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UCombatInstanceSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static UCombatInstanceSubsystem* Get();


	// asset load
	void BeginHitImpactAssetsLoad();

	void OnHitEnvironment(const AActor* Attacker, const FHitResult& HitResult);

private:
	static UCombatInstanceSubsystem* Instance;


	void OnHitImpactAssetsLoadCompleted();

	FSoftObjectPath LoadSoftObjectPathes;
	TSharedPtr<FStreamableHandle> StreamableHandle;

	TSoftObjectPtr<UDataTable> HitEffectDTRawPtr;
	UPROPERTY()
	TObjectPtr<class UDataTable> HitEffectDTInstance;
};
