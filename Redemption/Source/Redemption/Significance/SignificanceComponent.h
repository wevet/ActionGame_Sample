// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SignificanceManager.h"
#include "SignificanceConfig.h"
#include "SignificanceComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class REDEMPTION_API USignificanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USignificanceComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:
	static float K2_SignificanceFunction(USignificanceManager::FManagedObjectInfo* ManagedObjectInfo, const FTransform& Viewpoint);


public:
	UPROPERTY(EditDefaultsOnly)
	FName Tag = FName("Default");

	/** 重要度：キャラクターの演技の質を調整するもので、レベルが低いほど質が高くなる。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int32 SignificanceLevel = -1;

	/** Significance 画面空間でキャラクターがどれだけ大きく見えるかを示すFruit */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	ESignificanceFruitType SignificanceFruitType = ESignificanceFruitType::Melon;

	/** パフォーマンス・レベルの異なる構成 */
	UPROPERTY(BlueprintReadOnly)
	TArray<FSignificanceConfigType2> SignificanceConfigArray;

public:
	int32 Significance_CharacterBaseBattleMinLevel = 0;
	int32 Significance_CharacterBaseAIUnlockLevel = 0;
	int32 Significance_CharacterBaseMeshVisibleLevel = 0;
	int32 Significance_EnvironmentCreatureBaseBattleMinLevel = 0;
};
