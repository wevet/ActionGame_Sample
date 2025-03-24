// Copyright 2022 wevet works All Rights Reserved.


#include "Significance/SignificanceComponent.h"
#include "Significance/SignificanceInterface.h"
#include "Game/WvProjectSetting.h"

#include "IAnimationBudgetAllocator.h"
#include "SignificanceManager.h"

USignificanceComponent::USignificanceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USignificanceComponent::BeginPlay()
{

	Super::BeginPlay();

	UObject* Object = GetOwner();
	if (Object->Implements<USignificanceInterface>())
	{
		if (UWorld* World = GetWorld())
		{
			if (USignificanceManager* SignificanceManager = USignificanceManager::Get(World))
			{
				SignificanceManager->RegisterObject(Object, Tag, &USignificanceComponent::K2_SignificanceFunction, USignificanceManager::EPostSignificanceType::None, nullptr);
			}
		}
	}

	const UWvProjectSetting* ProjectSettings = GetDefault<UWvProjectSetting>();

	// read Significance setting
	FSignificanceConfigType2 Level0Config;
	Level0Config.TickInterval = 0.f;
	Level0Config.MinLOD = 0;
	SignificanceConfigArray.Add(Level0Config);

	FSignificanceConfigType2 Level1Config;
	Level1Config.TickInterval = 1.0f / ProjectSettings->SignificanceConfig_Level1.TickFPS;
	Level1Config.MinLOD = ProjectSettings->SignificanceConfig_Level1.MinLOD;
	SignificanceConfigArray.Add(Level1Config);

	FSignificanceConfigType2 Level2Config;
	Level2Config.TickInterval = 1.0f / ProjectSettings->SignificanceConfig_Level2.TickFPS;
	Level2Config.MinLOD = ProjectSettings->SignificanceConfig_Level2.MinLOD;
	SignificanceConfigArray.Add(Level2Config);

	FSignificanceConfigType2 Level3Config;
	Level3Config.TickInterval = 1.0f / ProjectSettings->SignificanceConfig_Level3.TickFPS;
	Level3Config.MinLOD = ProjectSettings->SignificanceConfig_Level3.MinLOD;
	SignificanceConfigArray.Add(Level3Config);

	FSignificanceConfigType2 Level4Config;
	Level4Config.TickInterval = 1.0f / ProjectSettings->SignificanceConfig_Level4.TickFPS;
	Level4Config.MinLOD = ProjectSettings->SignificanceConfig_Level4.MinLOD;
	SignificanceConfigArray.Add(Level4Config);

	FSignificanceConfigType2 OutOfRangeConfig;
	OutOfRangeConfig.TickInterval = 1.0f / ProjectSettings->SignificanceConfig_OutOfRange.TickFPS;
	OutOfRangeConfig.MinLOD = ProjectSettings->SignificanceConfig_OutOfRange.MinLOD;
	SignificanceConfigArray.Add(OutOfRangeConfig);

	Significance_CharacterBaseBattleMinLevel = ProjectSettings->Significance_CharacterBaseBattleMinLevel;
	Significance_CharacterBaseAIUnlockLevel = ProjectSettings->Significance_CharacterBaseAIUnlockLevel;
	Significance_CharacterBaseMeshVisibleLevel = ProjectSettings->Significance_CharacterBaseMeshVisibleLevel;
	Significance_EnvironmentCreatureBaseBattleMinLevel = ProjectSettings->Significance_EnvironmentCreatureBaseBattleMinLevel;


	Super::SetComponentTickEnabled(false);
}

float USignificanceComponent::K2_SignificanceFunction(USignificanceManager::FManagedObjectInfo* ManagedObjectInfo, const FTransform& Viewpoint)
{
	AActor* Actor = Cast<AActor>(ManagedObjectInfo->GetObject());
	FVector Origin;
	FVector BoxExtent;
	float SphereRadius;
	ISignificanceInterface::Execute_GetSignificanceBounds(Actor, Origin, BoxExtent, SphereRadius);

	const FVector ViewpointToActorSpear = Origin - Viewpoint.GetTranslation();
	float ViewpointToActorDistance = ViewpointToActorSpear.Size();
	if (ViewpointToActorDistance < 1.0f)
	{
		ViewpointToActorDistance = 1.0f;
	}

	const float Depth = FVector::DotProduct(ViewpointToActorSpear, Viewpoint.GetRotation().Vector());

	// 負の深度は、キャラクタがカメラの後ろにいることを示します。 距離が半径より大きい場合は、キャラクタがモデルの中にいないことを示します。 
	// 両方が満たされている場合、キャラクタは目で見ることができません。
	const bool IsEyeCanNotSee = Depth < 0.0f && ViewpointToActorDistance > SphereRadius;

	// キャラクターが占める画面領域が大きければ大きいほど、その重要性は高まります。 画面領域を推定しているだけで、正確に計算しているわけではありません。
	// キャラクターを円柱と仮定すると、一定の距離に配置することで面積が得られ、これが推定される画面領域となる。
	// 実際のスクリーン面積を、FMath::Max(BoxExtent.X, BoxExtent.Y) * BoxExtent.Z / ViewpointToActorDistance / ViewpointToActorDistance * 4.0f として計算します。
	// 各値に 4.0f を乗じるが、これはサイズの比較には影響しないので、この操作は必要ない。 計算を次のように単純化します。
	const float SignificanceValue = FMath::Max(BoxExtent.X, BoxExtent.Y) * BoxExtent.Z / ViewpointToActorDistance / ViewpointToActorDistance;

	if (IsEyeCanNotSee)
	{
		// 文字が目に見えない場合は (-∞, 0) を返す。

		// 保護ゼロ
		if (SignificanceValue == 0.0f)
		{
			return -340282346638528859811704183484516925440.0;
		}
		// ここでの1/X演算は、重要度の順序が正しいことを確認するためのものである。
		return -1.0f / SignificanceValue;
	}
	else
	{
		// 文字が目に見える場合は (0, +∞) を返す。
		return SignificanceValue;
	}


}

void USignificanceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USignificanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UObject* Object = GetOwner();
	if (Object->Implements<USignificanceInterface>())
	{
		if (UWorld* World = GetWorld())
		{
			if (USignificanceManager* SignificanceManager = USignificanceManager::Get(World))
			{
				SignificanceManager->UnregisterObject(Object);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

