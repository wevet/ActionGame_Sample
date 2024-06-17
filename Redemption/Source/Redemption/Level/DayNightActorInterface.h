// Copyright 2022 wevet works All Rights Reserved.

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DayNightActorInterface.generated.h"


UINTERFACE(BlueprintType)
class UDayNightActorInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class REDEMPTION_API IDayNightActorInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DayNightActor")
	void StartNight();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DayNightActor")
	void EndNight();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DayNightActor")
	void StartDay();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DayNightActor")
	void EndDay();
};

