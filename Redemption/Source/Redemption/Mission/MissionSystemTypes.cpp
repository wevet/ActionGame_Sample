// Copyright 2022 wevet works All Rights Reserved.

#include "MissionSystemTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MissionSystemTypes)


FSendMissionData USendMissionDataAsset::Find(const int32 InSendMissionIndex, bool& bIsValid)
{
	auto FindSendMissionData = SendMissionDatas.FindByPredicate([&](FSendMissionData Item)
	{
		return (Item.SendMissionIndex== InSendMissionIndex);
	});

	if (FindSendMissionData)
	{
		bIsValid = true;
		return *FindSendMissionData;
	}

	FSendMissionData Temp;
	return Temp;

}

