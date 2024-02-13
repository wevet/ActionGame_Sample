// Copyright 2020 wevet works All Rights Reserved.

#include "WvGameplayCueManager.h"
#include "WvAbilitySystem.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "GameplayCueNotify_Actor.h"
#include "GameplayCueNotify_Static.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Launch/Resources/Version.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvGameplayCueManager)

int32 ActGameplayCueActorRecycle = 1;
static FAutoConsoleVariableRef CVarGameplayCueActorRecycle(TEXT("AbilitySystem.ActGameplayCueActorRecycle"), ActGameplayCueActorRecycle, TEXT("Allow recycling of GameplayCue Actors"), ECVF_Default);

int32 ActGameplayCueActorDebug = 0;
static FAutoConsoleVariableRef CVarGameplayCueActorRecycleDebug(TEXT("AbilitySystem.ActGameplayCueActorDebug"), ActGameplayCueActorDebug, TEXT("Prints logs for Act GC actor debugging"), ECVF_Default);


bool UWvGameplayCueSet::HandleGameplayCueNotify_Internal(AActor* TargetActor, int32 DataIdx, EGameplayCueEvent::Type EventType, FGameplayCueParameters& Parameters)
{
	bool bReturnVal = false;

	UWvGameplayCueManager* CueManager = Cast<UWvGameplayCueManager>(UAbilitySystemGlobals::Get().GetGameplayCueManager());
	if (!ensure(CueManager))
	{
		return false;
	}

	if (DataIdx != INDEX_NONE)
	{
		check(GameplayCueData.IsValidIndex(DataIdx));
		FGameplayCueNotifyData& CueData = GameplayCueData[DataIdx];
		Parameters.MatchedTagName = CueData.GameplayCueTag;

		// If object is not loaded yet
		if (CueData.LoadedGameplayCueClass == nullptr)
		{
			// See if the object is loaded but just not hooked up here
			CueData.LoadedGameplayCueClass = FindObject<UClass>(nullptr, *CueData.GameplayCueNotifyObj.ToString());
			if (CueData.LoadedGameplayCueClass == nullptr)
			{
				if (!CueManager->HandleMissingGameplayCue(this, CueData, TargetActor, EventType, Parameters))
				{
					return false;
				}
			}
		}

		check(CueData.LoadedGameplayCueClass);

		// Handle the Notify if we found something
		if (UGameplayCueNotify_Static* NonInstancedCue = Cast<UGameplayCueNotify_Static>(CueData.LoadedGameplayCueClass->ClassDefaultObject))
		{
			if (NonInstancedCue->HandlesEvent(EventType))
			{
				NonInstancedCue->HandleGameplayCue(TargetActor, EventType, Parameters);
				bReturnVal = true;
				if (!NonInstancedCue->IsOverride)
				{
					HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
				}
			}
			else
			{
				//Didn't even handle it, so IsOverride should not apply.
				HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
			}
		}
		else if (AGameplayCueNotify_Actor* InstancedCue = Cast<AGameplayCueNotify_Actor>(CueData.LoadedGameplayCueClass->ClassDefaultObject))
		{
			if (InstancedCue->HandlesEvent(EventType))
			{
				if (EventType == EGameplayCueEvent::WhileActive)
				{
					//Get our instance. We should probably have a flag or something to determine if we want to reuse or stack instances. That would mean changing our map to have a list of active instances.
					AGameplayCueNotify_Actor* SpawnedInstancedCue = CueManager->GetInstancedCueActor(TargetActor, CueData.LoadedGameplayCueClass, Parameters);
					if (ensure(SpawnedInstancedCue))
					{
						SpawnedInstancedCue->HandleGameplayCue(TargetActor, EventType, Parameters);
						bReturnVal = true;
						if (!SpawnedInstancedCue->IsOverride)
						{
							HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
						}
					}
				}
				else if (EventType == EGameplayCueEvent::Executed)
				{
					AGameplayCueNotify_Actor* CDO = Cast<AGameplayCueNotify_Actor>(CueData.LoadedGameplayCueClass->ClassDefaultObject);
					FGCNotifyActorKey NotifyKey(TargetActor, CueData.LoadedGameplayCueClass, CDO->bUniqueInstancePerInstigator ? Parameters.GetInstigator() : nullptr, CDO->bUniqueInstancePerSourceObject ? Parameters.GetSourceObject() : nullptr);
					if (const TSet<AGameplayCueNotify_Actor*>* CueList = CueManager->NotifyMapActorList.Find(NotifyKey))
					{
						for (auto It = CueList->CreateConstIterator(); It; ++It)
						{
							AGameplayCueNotify_Actor* Cue = *It;
							Cue->HandleGameplayCue(TargetActor, EventType, Parameters);
							if (!Cue->IsOverride)
							{
								HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
							}
						}
					}
				}
				else if (EventType == EGameplayCueEvent::Removed)
				{
					AGameplayCueNotify_Actor* CDO = Cast<AGameplayCueNotify_Actor>(CueData.LoadedGameplayCueClass->ClassDefaultObject);
					FGCNotifyActorKey NotifyKey(TargetActor, CueData.LoadedGameplayCueClass, CDO->bUniqueInstancePerInstigator ? Parameters.GetInstigator() : nullptr, CDO->bUniqueInstancePerSourceObject ? Parameters.GetSourceObject() : nullptr);
					TSet<AGameplayCueNotify_Actor*> CueList;
					if (CueManager->NotifyMapActorList.RemoveAndCopyValue(NotifyKey, CueList))
					{
						for (auto It = CueList.CreateConstIterator(); It; ++It)
						{
							AGameplayCueNotify_Actor* Cue = *It;
							Cue->bAutoDestroyOnRemove = true;
							Cue->HandleGameplayCue(TargetActor, EventType, Parameters);
							if (!Cue->IsOverride)
							{
								HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
							}
						}
						bReturnVal = true;
					}
				}
			}
			else
			{
				//Didn't even handle it, so IsOverride should not apply.
				HandleGameplayCueNotify_Internal(TargetActor, CueData.ParentDataIdx, EventType, Parameters);
			}
		}
	}

	return bReturnVal;
}

UWvGameplayCueManager* UWvGameplayCueManager::Get()
{
	return Cast<UWvGameplayCueManager>(UAbilitySystemGlobals::Get().GetGameplayCueManager());
}

void UWvGameplayCueManager::OnCreated()
{
#if  (ENGINE_MAJOR_VERSION < 5 || ENGINE_MINOR_VERSION >= 3)
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &UWvGameplayCueManager::OnPostWorldCleanup);
	FWorldDelegates::OnPreWorldFinishDestroy.AddUObject(this, &UWvGameplayCueManager::OnPostWorldCleanup, true, true);
#else
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &UWvGameplayCueManager::OnWorldCleanup);
	FWorldDelegates::OnPreWorldFinishDestroy.AddUObject(this, &UWvGameplayCueManager::OnWorldCleanup, true, true);
#endif

	FNetworkReplayDelegates::OnPreScrub.AddUObject(this, &UWvGameplayCueManager::OnPreReplayScrub);

	if (GIsRunning)
	{
		// Engine init already completed
		OnEngineInitComplete();
	}
	else
	{
		FCoreDelegates::OnFEngineLoopInitComplete.AddUObject(this, &UWvGameplayCueManager::OnEngineInitComplete);
	}
}

void UWvGameplayCueManager::ActInitializeRuntimeObjectLibrary()
{
	RuntimeGameplayCueObjectLibrary.Paths = GetAlwaysLoadedGameplayCuePaths();
	if (RuntimeGameplayCueObjectLibrary.CueSet == nullptr)
	{
		RuntimeGameplayCueObjectLibrary.CueSet = NewObject<UWvGameplayCueSet>(this, TEXT("GlobalGameplayCueSet"));
	}

	RuntimeGameplayCueObjectLibrary.CueSet->Empty();
	RuntimeGameplayCueObjectLibrary.bHasBeenInitialized = true;
	RuntimeGameplayCueObjectLibrary.bShouldSyncScan = ShouldSyncScanRuntimeObjectLibraries();
	RuntimeGameplayCueObjectLibrary.bShouldSyncLoad = ShouldSyncLoadRuntimeObjectLibraries();
	RuntimeGameplayCueObjectLibrary.bShouldAsyncLoad = ShouldAsyncLoadRuntimeObjectLibraries();
	InitObjectLibrary(RuntimeGameplayCueObjectLibrary);

	for (FString Path : RuntimeGameplayCueObjectLibrary.Paths)
	{
		UE_LOG(LogWvAbility, Log, TEXT("Path => %s"), *Path);
	}
	UE_LOG(LogWvAbility, Error, TEXT("%s"), *FString(__FUNCTION__));

}


AGameplayCueNotify_Actor* UWvGameplayCueManager::GetInstancedCueActor(AActor* TargetActor, UClass* CueClass, const FGameplayCueParameters& Parameters)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_WvGameplayCueManager_GetInstancedCueActor);

	// First, see if this actor already have a GameplayCueNotifyActor already going for this CueClass
	AGameplayCueNotify_Actor* CDO = Cast<AGameplayCueNotify_Actor>(CueClass->ClassDefaultObject);
	FGCNotifyActorKey NotifyKey(TargetActor, CueClass, CDO->bUniqueInstancePerInstigator ? Parameters.GetInstigator() : nullptr, CDO->bUniqueInstancePerSourceObject ? Parameters.GetSourceObject() : nullptr);

	AGameplayCueNotify_Actor* SpawnedCue = nullptr;
	FVector  CueLocation = Parameters.Location;
	FRotator CurRotation = FRotationMatrix::MakeFromX(Parameters.Normal).Rotator();
	UWorld* World = GetWorld();

	// We don't have an instance for this, and we need one, so make one
	if (ensure(TargetActor) && ensure(CueClass) && ensure(World))
	{
		AActor* NewOwnerActor = TargetActor;
		bool UseActorRecycling = (ActGameplayCueActorRecycle > 0);

#if WITH_EDITOR	
		// Animtion preview hack. If we are trying to play the GC on a CDO, then don't use actor recycling and don't set the owner (to the CDO, which would cause problems)
		if (TargetActor && TargetActor->HasAnyFlags(RF_ClassDefaultObject))
		{
			NewOwnerActor = nullptr;
			UseActorRecycling = false;
		}
#endif
		// Look to reuse an existing one that is stored on the CDO:
		if (UseActorRecycling)
		{
			FPreallocationInfo& Info = GetPreallocationInfo(World);
			FGameplayCueNotifyActorArray* PreallocatedList = Info.PreallocatedInstances.Find(CueClass);
			if (PreallocatedList && PreallocatedList->Actors.Num() > 0)
			{
				SpawnedCue = nullptr;
				while (true)
				{
					SpawnedCue = PreallocatedList->Actors.Pop(false);

					// Temp: tracking down possible memory corruption
					// null is maybe ok. But invalid low level is bad and we want to crash hard to find out who/why.
					if (SpawnedCue && !SpawnedCue->IsValidLowLevelFast())
					{
						checkf(false, TEXT("UGameplayCueManager::GetInstancedCueActor found an invalid SpawnedCue for class %s"), *GetNameSafe(CueClass));
					}

					// Normal check: if cue was destroyed or is pending kill, then don't use it.
					if (SpawnedCue && !IsValid(SpawnedCue))
					{
						break;
					}

					// outside of replays, this should not happen. GC Notifies should not be actually destroyed.
					checkf(World->GetDemoNetDriver(), TEXT("Spawned Cue is pending kill or null: %s."), *GetNameSafe(SpawnedCue));

					if (PreallocatedList->Actors.Num() <= 0)
					{
						// Ran out of preallocated instances... break and create a new one.
						break;
					}
				}

				if (SpawnedCue)
				{
					SpawnedCue->bInRecycleQueue = false;
					SpawnedCue->SetOwner(NewOwnerActor);
					if (!SpawnedCue->bAutoAttachToOwner)
					{
						SpawnedCue->SetActorLocationAndRotation(CueLocation, CurRotation);
					}
					SpawnedCue->ReuseAfterRecycle();
				}

				UE_CLOG((ActGameplayCueActorDebug > 0), LogAbilitySystem, Display, TEXT("GetInstancedCueActor Popping Recycled %s (Target: %s). Using GC Actor: %s"), *GetNameSafe(CueClass), *GetNameSafe(TargetActor), *GetNameSafe(SpawnedCue));
			}
		}

		// If we can't reuse, then spawn a new one
		if (SpawnedCue == nullptr)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = NewOwnerActor;
			if (ActGameplayCueActorDebug)
			{
				ABILITY_LOG(Warning, TEXT("Spawning GameplaycueActor: %s"), *CueClass->GetName());
			}

			SpawnParams.OverrideLevel = World->PersistentLevel;
			SpawnedCue = World->SpawnActor<AGameplayCueNotify_Actor>(CueClass, CueLocation, CurRotation, SpawnParams);
		}

		// Associate this GameplayCueNotifyActor with this target actor/key
		if (ensure(SpawnedCue))
		{
			SpawnedCue->NotifyKey = NotifyKey;
			TSet<AGameplayCueNotify_Actor*>& CueList = NotifyMapActorList.FindOrAdd(NotifyKey);
			CueList.Add(SpawnedCue);
		}
	}

	UE_CLOG((ActGameplayCueActorDebug > 0), LogAbilitySystem, Display, TEXT("GetInstancedCueActor  Returning %s (Target: %s). Using GC Actor: %s"), *GetNameSafe(CueClass), *GetNameSafe(TargetActor), *GetNameSafe(SpawnedCue));
	return SpawnedCue;
}

void UWvGameplayCueManager::NotifyGameplayCueActorFinished(AGameplayCueNotify_Actor* Actor)
{
	bool UseActorRecycling = (ActGameplayCueActorRecycle > 0);

#if WITH_EDITOR	
	// Don't recycle in preview worlds
	if (Actor->GetWorld()->IsPreviewWorld())
	{
		UseActorRecycling = false;
	}
#endif

	if (TSet<AGameplayCueNotify_Actor*>* CueList = NotifyMapActorList.Find(Actor->NotifyKey))
	{
		// Only remove if this is the current actor in the map!
		// This could happen if a GC notify actor has a delayed removal and another GC event happens before the delayed removal happens (the old GC actor could replace the latest one in the map)
		CueList->Remove(Actor);
		if (CueList->Num() == 0)
		{
			NotifyMapActorList.Remove(Actor->NotifyKey);
		}
	}

	if (UseActorRecycling)
	{
		if (Actor->bInRecycleQueue)
		{
			// We are already in the recycle queue. This can happen normally
			// (For example the GC is removed and the owner is destroyed in the same frame)
			return;
		}

		AGameplayCueNotify_Actor* CDO = Actor->GetClass()->GetDefaultObject<AGameplayCueNotify_Actor>();
		if (CDO && Actor->Recycle())
		{
			if (!IsValid(Actor))
			{
				ensureMsgf(GetWorld()->GetDemoNetDriver(), TEXT("GameplayCueNotify %s is pending kill in ::NotifyGameplayCueActorFinished (and not in network demo)"), *GetNameSafe(Actor));
				return;
			}
			Actor->bInRecycleQueue = true;
			UE_CLOG((ActGameplayCueActorDebug > 0), LogAbilitySystem, Display, TEXT("NotifyGameplayCueActorFinished %s"), *GetNameSafe(Actor));
			FPreallocationInfo& Info = GetPreallocationInfo(Actor->GetWorld());
			FGameplayCueNotifyActorArray& PreAllocatedList = Info.PreallocatedInstances.FindOrAdd(Actor->GetClass());

			// Put the actor back in the list
			if (ensureMsgf(PreAllocatedList.Actors.Contains(Actor) == false, TEXT("GC Actor PreallocationList already contains Actor %s"), *GetNameSafe(Actor)))
			{
				PreAllocatedList.Actors.Push(Actor);
			}
			return;
		}
	}

	// We didn't recycle, so just destroy
	Actor->Destroy();
}


AGameplayCueNotify_Actor* UWvGameplayCueManager::FindExistingCueOnActorContainer(const AActor& TargetActor, const TSubclassOf<AGameplayCueNotify_Actor>& CueClass, const FGameplayCueParameters& Parameters) const
{
	return FindExistingCueOnActor(TargetActor, CueClass, Parameters);
}

