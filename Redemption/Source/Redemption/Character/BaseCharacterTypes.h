// Copyright 2022 wevet works All Rights Reserved.

#include "Locomotion/LocomotionSystemTypes.h"

// builtin
#include "Animation/AnimInstance.h"
#include "BaseCharacterTypes.generated.h"


USTRUCT(BlueprintType)
struct FOverlayAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELSOverlayState OverlayState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimInstanceClass;
};


UCLASS(BlueprintType)
class REDEMPTION_API UOverlayAnimInstanceDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FOverlayAnimInstance> OverlayAnimInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> UnArmedAnimInstanceClass;

	TSubclassOf<UAnimInstance> FindAnimInstance(const ELSOverlayState InOverlayState) const;
};


UCLASS(BlueprintType)
class REDEMPTION_API UHairMaterialsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* HairMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* FacialMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* HelmetMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* DefaultGroomTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* DefaultMaskTexture;
};

