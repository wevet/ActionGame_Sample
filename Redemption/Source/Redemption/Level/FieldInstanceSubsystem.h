// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FieldInstanceSubsystem.generated.h"

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

private:
	static UFieldInstanceSubsystem* Instance;

};
