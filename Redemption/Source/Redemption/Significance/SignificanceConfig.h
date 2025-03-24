// Author: Tan Junhui (Balous~)

#pragma once

#include "CoreMinimal.h"
#include "SignificanceConfig.generated.h"

/**
 * Configに使用される重要度設定
 */
USTRUCT(BlueprintType)
struct FSignificanceConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float TickFPS = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 MinLOD = 1;
};

/**
 * Significance Structure
 */
USTRUCT(BlueprintType)
struct FSignificanceConfigType2
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float TickInterval = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 MinLOD = 0;
};

UENUM(BlueprintType)
enum class ESignificanceFruitType : uint8
{
	Melon UMETA(DisplayName = "Melon"),
	Peanut UMETA(DisplayName = "Peanut"),
	Sesame UMETA(DisplayName = "Sesame"),
};

