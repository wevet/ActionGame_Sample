// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WvAbilitySystemTypes.h"
#include "WvAbilitySystemAvatarInterface.generated.h"

class UWvAbilitySystemComponentBase;
class UBehaviorTree;

UINTERFACE(MinimalAPI)
class UWvAbilitySystemAvatarInterface : public UInterface
{
	GENERATED_BODY()
};


class WVABILITYSYSTEM_API IWvAbilitySystemAvatarInterface
{
	GENERATED_BODY()

public:
	virtual const FWvAbilitySystemAvatarData& GetAbilitySystemData() = 0;
	virtual void InitAbilitySystemComponentByData(class UWvAbilitySystemComponentBase* ASC);

	virtual UBehaviorTree* GetBehaviorTree() const;
	virtual UWvHitReactDataAsset* GetHitReactDataAsset() const;

	virtual FName GetAvatarName() const = 0;

	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveOnInitAttribute();
};
