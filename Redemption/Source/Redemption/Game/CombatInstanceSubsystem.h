// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WvFoleyAssetTypes.h"
#include "CombatInstanceSubsystem.generated.h"


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
	void LoadAllEnvironmentVFXAssets();

	void OnHitEnvironment(const AActor* Attacker, const FHitResult& HitResult);

	const FVFXBaseAsset& GetVFXAssets(const FName SurfaceName, const FGameplayTag InTag, bool& bOutFound) const;

private:
	static UCombatInstanceSubsystem* Instance;

	void OnEnvironmentVFXDataAssetsLoaded();

	UPROPERTY()
	TObjectPtr<class UEnvironmentVFXDataAsset> EnvironmentVFXDataAsset;
};
