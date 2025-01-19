// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueManager.h"
#include "GameplayCueSet.h"
#include "GameplayCueNotify_Actor.h"
#include "WvGameplayCueManager.generated.h"


UCLASS()
class WVABILITYSYSTEM_API UWvGameplayCueSet : public UGameplayCueSet
{
	GENERATED_BODY()

protected:
	virtual bool HandleGameplayCueNotify_Internal(AActor* TargetActor, int32 DataIdx, EGameplayCueEvent::Type EventType, FGameplayCueParameters& Parameters) override;
};


UCLASS()
class WVABILITYSYSTEM_API UWvGameplayCueManager : public UGameplayCueManager
{
	GENERATED_BODY()


public:
	static UWvGameplayCueManager* Get();

	virtual void ActInitializeRuntimeObjectLibrary();
	
	virtual AGameplayCueNotify_Actor* GetInstancedCueActor(AActor* TargetActor, UClass* CueClass, const FGameplayCueParameters& Parameters) override;
	
	virtual void NotifyGameplayCueActorFinished(AGameplayCueNotify_Actor* Actor) override;

	virtual void OnCreated() override;
	virtual bool ShouldAsyncLoadObjectLibrariesAtStart() const override { return false; }

	AGameplayCueNotify_Actor* FindExistingCueOnActorContainer(const AActor& TargetActor, const TSubclassOf<AGameplayCueNotify_Actor>& CueClass, const FGameplayCueParameters& Parameters) const;

public:
	// @TODO
	// TMap<FGCNotifyActorKey, TSet<AGameplayCueNotify_Actor*> > NotifyMapActorList;

	// wip
	TSet<AGameplayCueNotify_Actor*> NotifyMapActorHash;
};
