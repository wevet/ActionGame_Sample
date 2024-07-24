// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DayNightBaseActor.h"
#include "FieldInstanceSubsystem.generated.h"

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

private:
	static UFieldInstanceSubsystem* Instance;

	UPROPERTY()
	TArray<ADayNightBaseActor*> DayNightActors;

	EDayNightPhase DayNightPhase = EDayNightPhase::Day;

};

