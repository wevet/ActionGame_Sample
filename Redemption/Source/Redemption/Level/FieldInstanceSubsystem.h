// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DayNightBaseActor.h"
#include "WvFoleyAssetTypes.h"
#include "FieldInstanceSubsystem.generated.h"


class ASkyActor;
class UFoleyEventDataAsset;


UENUM(BlueprintType)
enum class EDayNightPhase : uint8
{
	Day   UMETA(DisplayName = "Day"),
	Night UMETA(DisplayName = "Night"),
};

/**
 * 
 */
UCLASS()
class REDEMPTION_API UFieldInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UFieldInstanceSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	static UFieldInstanceSubsystem* Get();

	UFUNCTION(BlueprintCallable, Category = Level)
	void SetHour(const int32 InHour);

	UFUNCTION(BlueprintCallable, Category = Level)
	int32 GetHour() const;

	UFUNCTION(BlueprintCallable, Category = Level)
	void AddDayNightActor(AActor* InActor);
	void StartNight();
	void StartDay();
	EDayNightPhase GetDayNightPhase() const;

	bool IsInNight() const;
	bool IsInDay() const;

	UFUNCTION(BlueprintCallable, Category = Level)
	void SetSkyActor(ASkyActor* NewSkyActor);

	UFUNCTION(BlueprintCallable, Category = Level)
	void AddPOIActor(AActor* NewActor);

	UFUNCTION(BlueprintCallable, Category = Level)
	void RemovePOIActor(AActor* NewActor);

	UFUNCTION(BlueprintCallable, Category = Level)
	TArray<AActor*> GetPOIActors() const;


	//UFUNCTION(BlueprintCallable, Category = "Foley")
	void LoadAllFootstepAssets();

	void OnFoleyEventDataAssetsLoaded();


	const FFoleyBaseAsset& GetFoleyBaseAsset(const FGameplayTag SurfaceTag, TEnumAsByte<EPhysicalSurface> SurfaceTypeInEditor, bool& bOutFound) const;

	UFUNCTION(BlueprintCallable, Category = "Foley")
	FFoleyBaseAsset GetFoleyBaseAssetCopy(FGameplayTag SurfaceTag, TEnumAsByte<EPhysicalSurface> SurfaceTypeInEditor, bool& bOutFound) const;

private:
	static UFieldInstanceSubsystem* Instance;
	EDayNightPhase DayNightPhase = EDayNightPhase::Day;

	UPROPERTY()
	TArray<AActor*> POIActors;

	UPROPERTY()
	TArray<ADayNightBaseActor*> DayNightActors;

	UPROPERTY()
	TObjectPtr<class ASkyActor> SkyActor;

	UPROPERTY()
	TObjectPtr<class UFoleyEventDataAsset> FoleyEventDataAsset;
};

