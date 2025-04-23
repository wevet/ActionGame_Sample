// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NativeGameplayTags.h"
#include "MinimapMarkerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class REDEMPTION_API UMinimapMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UMinimapMarkerComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FGameplayTag MiniMapMakerTag{FGameplayTag::EmptyTag};


	void SetVisibleMakerTag(const bool NewIsAllowVisibleMakerTag);
	bool IsVisibleMakerTag() const;
	
private:
	bool bIsAllowVisibleMakerTag{true};
};
