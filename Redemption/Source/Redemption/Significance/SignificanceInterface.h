
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
//#include "SignificanceConfig.h"
#include "SignificanceInterface.generated.h"

UINTERFACE(MinimalAPI)
class USignificanceInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class REDEMPTION_API ISignificanceInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="ISignificanceInterface")
	void OnSignificanceLevelChanged(int32 SignificanceLevel);

	//UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="ISignificanceInterface")
	//void OnSignificanceFruitTypeChanged(ESignificanceFruitType SignificanceFruitType);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="ISignificanceInterface")
	void GetSignificanceBounds(FVector& Origin, FVector& BoxExtent, float& SphereRadius);
};

