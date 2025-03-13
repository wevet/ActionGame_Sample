// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "Kismet/GameplayStatics.h"


namespace Game
{
	class REDEMPTION_API ControllerExtension
	{
	public:
		// usage
		// ControllerExtension::GetPlayer(this)
		static FORCEINLINE APlayerController* GetPlayer(const UObject* WorldContextObject, int32 PlayerIndex = 0)
		{
			return UGameplayStatics::GetPlayerController(WorldContextObject, PlayerIndex);
		}

		// usage
		// ControllerExtension::GetCameraManager(this);
		static FORCEINLINE APlayerCameraManager* GetCameraManager(const UObject* WorldContextObject, int32 PlayerIndex = 0)
		{
			return UGameplayStatics::GetPlayerCameraManager(WorldContextObject, PlayerIndex);
		}

		// usage
		// ControllerExtension::GetPlayerPawn(this)
		static FORCEINLINE APawn* GetPlayerPawn(const UObject* WorldContextObject, int32 PlayerIndex = 0)
		{
			return UGameplayStatics::GetPlayerController(WorldContextObject, PlayerIndex)->GetPawn();
		}
	};

	class REDEMPTION_API AssetExtension
	{
	public:
		template<typename T>
		static FORCEINLINE T* GetAssetFromAssetLibrary(const TArray<T*> AssetLibrary, const FString& AssetID)
		{
			for (const UObject* Asset : AssetLibrary)
			{
				T* AssetPtr = Cast<T>(Asset);
				if (!AssetPtr)
				{
					continue;
				}

				if (AssetPtr == AssetID)
				{
					return AssetPtr;
				}
			}
			return nullptr;
		}
	};

	class REDEMPTION_API ArrayExtension
	{
	public:
		template<typename T>
		static FORCEINLINE bool NullOrEmpty(const TArray<T*> Array)
		{
			return (Array.Num() <= 0);
		}

		template<typename T>
		static FORCEINLINE bool NullOrEmpty(const TArray<T> Array)
		{
			return (Array.Num() <= 0);
		}

		template<typename T>
		static FORCEINLINE bool NullOrEmpty(const TArray<TWeakObjectPtr<T>> Array)
		{
			return (Array.Num() <= 0);
		}

		template<typename T>
		static FORCEINLINE bool NullOrEmpty(const TArray<TObjectPtr<T>> Array)
		{
			return (Array.Num() <= 0);
		}
	};

	class REDEMPTION_API ComponentExtension
	{
	public :
		// usage
		// auto Components = ComponentExtension::GetComponentsArray<USkeletalMeshComponent>(this);
		template<typename T>
		static FORCEINLINE TArray<T*> GetComponentsArray(const AActor* Owner)
		{
			TArray<T*> FindComponents;
			TArray<UActorComponent*> Components;
			Owner->GetComponents(T::StaticClass(), Components, true);
			for (UActorComponent* Component : Components)
			{
				if (T* CustomComp = Cast<T>(Component))
				{
					FindComponents.Add(CustomComp);
					FindComponents.Shrink();
				}
			}
			return FindComponents;
		}

		// usage
		// auto TargetComponent = ComponentExtension::GetComponentFirst<USkeletalMeshComponent>(this);
		template<typename T>
		static FORCEINLINE T* GetComponentFirst(const AActor* Owner)
		{
			TArray<UActorComponent*> Components;
			Owner->GetComponents(T::StaticClass(), Components, true);
			if (Components.Num() <= 0)
			{
				return nullptr;
			}
			if (T* CustomComp = Cast<T>(Components[0]))
			{
				return CustomComp;
			}
			return nullptr;
		}

		// usage
		// auto TargetComponent = ComponentExtension::GetComponentLast<USkeletalMeshComponent>(this);
		template<typename T>
		static FORCEINLINE T* GetComponentLast(const AActor* Owner)
		{
			TArray<UActorComponent*> Components;
			Owner->GetComponents(T::StaticClass(), Components, true);
			if (Components.Num() <= 0)
			{
				return nullptr;
			}
			const int32 LastIndex = (Components.Num() - 1);
			if (T* CustomComp = Cast<T>(Components[LastIndex]))
			{
				return CustomComp;
			}
			return nullptr;
		}
	};
}

