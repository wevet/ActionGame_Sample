// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "MassAgentComponent.h"
#include "MassCharacter.generated.h"

/**
 * 
 */
UCLASS()
class REDEMPTION_API AMassCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
public:
	AMassCharacter(const FObjectInitializer& ObjectInitializer);


	virtual void Freeze() override;
	virtual void UnFreeze() override;
	virtual void OnReceiveKillTarget(AActor* Actor, const float Damage) override;


	virtual void RequestAsyncLoad() override;

	UFUNCTION(BlueprintCallable, Category = Components)
	class UMassAgentComponent* GetMassAgentComponent() const;

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UMassAgentComponent> MassAgentComponent;

};

