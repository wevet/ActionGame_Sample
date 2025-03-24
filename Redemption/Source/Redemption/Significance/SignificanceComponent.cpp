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

	// ���̐[�x�́A�L�����N�^���J�����̌��ɂ��邱�Ƃ������܂��B ���������a���傫���ꍇ�́A�L�����N�^�����f���̒��ɂ��Ȃ����Ƃ������܂��B 
	// ��������������Ă���ꍇ�A�L�����N�^�͖ڂŌ��邱�Ƃ��ł��܂���B
	const bool IsEyeCanNotSee = Depth < 0.0f && ViewpointToActorDistance > SphereRadius;

	// �L�����N�^�[����߂��ʗ̈悪�傫����Α傫���قǁA���̏d�v���͍��܂�܂��B ��ʗ̈�𐄒肵�Ă��邾���ŁA���m�Ɍv�Z���Ă���킯�ł͂���܂���B
	// �L�����N�^�[���~���Ɖ��肷��ƁA���̋����ɔz�u���邱�ƂŖʐς������A���ꂪ���肳����ʗ̈�ƂȂ�B
	// ���ۂ̃X�N���[���ʐς��AFMath::Max(BoxExtent.X, BoxExtent.Y) * BoxExtent.Z / ViewpointToActorDistance / ViewpointToActorDistance * 4.0f �Ƃ��Čv�Z���܂��B
	// �e�l�� 4.0f ���悶�邪�A����̓T�C�Y�̔�r�ɂ͉e�����Ȃ��̂ŁA���̑���͕K�v�Ȃ��B �v�Z�����̂悤�ɒP�������܂��B
	const float SignificanceValue = FMath::Max(BoxExtent.X, BoxExtent.Y) * BoxExtent.Z / ViewpointToActorDistance / ViewpointToActorDistance;

	if (IsEyeCanNotSee)
	{
		// �������ڂɌ����Ȃ��ꍇ�� (-��, 0) ��Ԃ��B

		// �ی�[��
		if (SignificanceValue == 0.0f)
		{
			return -340282346638528859811704183484516925440.0;
		}
		// �����ł�1/X���Z�́A�d�v�x�̏��������������Ƃ��m�F���邽�߂̂��̂ł���B
		return -1.0f / SignificanceValue;
	}
	else
	{
		// �������ڂɌ�����ꍇ�� (0, +��) ��Ԃ��B
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

