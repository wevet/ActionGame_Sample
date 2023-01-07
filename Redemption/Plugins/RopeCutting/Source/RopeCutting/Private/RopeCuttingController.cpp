// Copyright Epic Games, Inc. All Rights Reserved.

#include "RopeCuttingController.h"

URopeCuttingController::URopeCuttingController()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FName URopeCuttingController::GetCutComponentName_RC(UPrimitiveComponent* HitCollisionComponent)
{
	FName RopeCompID = FName("None");

	if (HitCollisionComponent->ComponentTags[0] != FName("None"))
	{
		RopeCompID = HitCollisionComponent->ComponentTags[0];
	}

	return RopeCompID;

}

