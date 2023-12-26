// Copyright 2022 wevet works All Rights Reserved.


#include "WvAbilityType.h"


bool UWvCueConfigDataAssest::GetAttackCueConfigRow(const FGameplayTag& AvatarTag, const FGameplayTag& CueTag, FWvAttackCueConfigRow& CueConfigRow)
{
	if (AttackCueConfigTable == nullptr)
	{
		return false;
	}

	bool bIsSuccess = false;
	TArray<FWvAttackCueConfigRow*> AllRows;
	AttackCueConfigTable->GetAllRows<FWvAttackCueConfigRow>(TEXT("FWvAttackCueConfigRow"), AllRows);

	for (const FWvAttackCueConfigRow* ConfigRow : AllRows)
	{
		if (ConfigRow->AvatarTag == AvatarTag)
		{
			if (ConfigRow->CueTag == CueTag)
			{
				bIsSuccess = true;
				CueConfigRow = *ConfigRow;
				break;
			}
		}
	}
	return bIsSuccess;
}

bool UWvCueConfigDataAssest::GetDamageCueConfigRow(const FGameplayTag& AvatarTag, const FGameplayTag& CueTag, FWvDamageCueConfigRow& CueConfigRow)
{
	if (DamageCueConfigTable == nullptr)
	{
		return false;
	}

	//UE_LOG(LogTemp, Log, TEXT("AvatarTag => %s, CueTag => %s"), *AvatarTag.GetTagName().ToString(), *CueTag.GetTagName().ToString());

	bool bIsSuccess = false;
	TArray<FWvDamageCueConfigRow*> AllRows;
	DamageCueConfigTable->GetAllRows<FWvDamageCueConfigRow>(TEXT("FWvDamageCueConfigRow"), AllRows);

	for (const FWvDamageCueConfigRow* ConfigRow : AllRows)
	{
		if (ConfigRow->AvatarTag == AvatarTag)
		{
			if (ConfigRow->CueTag == CueTag)
			{
				bIsSuccess = true;
				CueConfigRow = *ConfigRow;
				break;
			}
		}
	}

	return bIsSuccess;
}

