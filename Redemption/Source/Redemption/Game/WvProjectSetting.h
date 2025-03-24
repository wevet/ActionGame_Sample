// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Significance/SignificanceConfig.h"
#include "WvProjectSetting.generated.h"

/**
 * 
 */
UCLASS(config = Engine, defaultconfig)
class REDEMPTION_API UWvProjectSetting : public UObject
{
	GENERATED_BODY()
	
public:
	UWvProjectSetting(const FObjectInitializer& ObjectInitializer);
	
	
	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "Significance Update Interval"), Category = "SignificanceManager")
	float SignificanceUpdateInterval = 0.35f;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "SignificanceLevel0 "), Category = "SignificanceManager")
	int32 SignificanceLevel0_BucketNumber = 6;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "SignificanceLevel1 "), Category = "SignificanceManager")
	int32 SignificanceLevel1_BucketNumber = 8;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "SignificanceLevel2 "), Category = "SignificanceManager")
	int32 SignificanceLevel2_BucketNumber = 10;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "SignificanceLevel3 "), Category = "SignificanceManager")
	int32 SignificanceLevel3_BucketNumber = 12;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "Significance far"), Category = "SignificanceManager")
	float SignificanceOutOfRange = 10000.0f;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "Significance Melon"), Category = "SignificanceManager")
	float SignificanceMelonValue = 0.005f;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "Significance Peanut"), Category = "SignificanceManager")
	float SignificancePeanutValue = 0.0002f;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "Significance Sesame"), Category = "SignificanceManager")
	float SignificanceSesameValue = 0.000075f;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "SignificanceLevel1"), Category = "SignificanceManager")
	FSignificanceConfig SignificanceConfig_Level1;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "SignificanceLevel2"), Category = "SignificanceManager")
	FSignificanceConfig SignificanceConfig_Level2;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "SignificanceLevel3"), Category = "SignificanceManager")
	FSignificanceConfig SignificanceConfig_Level3;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "SignificanceLevel4"), Category = "SignificanceManager")
	FSignificanceConfig SignificanceConfig_Level4;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "Significance far"), Category = "SignificanceManager")
	FSignificanceConfig SignificanceConfig_OutOfRange;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "CharacterBaseのバトル最低重要度評価"), Category = "SignificanceManager")
	int32 Significance_CharacterBaseBattleMinLevel = 3;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "CharacterBaseのレベルを含む、AIイネーブルメント重要度のレベル"), Category = "SignificanceManager")
	int32 Significance_CharacterBaseAIUnlockLevel = 3;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "CharacterBaseのMeshは、重要度を表示"), Category = "SignificanceManager")
	int32 Significance_CharacterBaseMeshVisibleLevel = 5;

	UPROPERTY(EditDefaultsOnly, config, meta = (ToolTip = "EnvironmentCreatureのバトル最低重要度評価"), Category = "SignificanceManager")
	int32 Significance_EnvironmentCreatureBaseBattleMinLevel = 4;
};
