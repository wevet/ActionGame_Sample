// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MinimapCaptureActor.generated.h"

class APawn;

UCLASS()
class REDEMPTION_API AMinimapCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	AMinimapCaptureActor();
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

public:
	void Initializer(APawn* NewPlayerCharacter);

	void UpdateCapture(const float DeltaTime);

	bool IsEnableMapRotation() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USceneComponent> SceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USceneCaptureComponent2D> SceneCaptureComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Minimap")
	TObjectPtr<UMaterialParameterCollection> MinimapCollection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Minimap")
	FName PlayerPositionParamName = FName("CharacterLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Minimap")
	FName PlayerRotationParamName = FName("CharacterYaw");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Minimap")
	float OrthoWidth{ 4000.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Minimap")
	float HeightOffset{ 5000.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Minimap")
	float CaptureInterval{ 0.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Minimap")
	float InterpSpeed{4.0f};

private:
	void UpdateCapture_Internal(const float DeltaTime);


	UPROPERTY()
	TWeakObjectPtr<UMaterialParameterCollectionInstance> MinimapCollectionInstance;

	UPROPERTY()
	TWeakObjectPtr<APawn> PlayerCharacter;

	float CaptureElapsed{ 0.f };
	float CurrentYaw{0.f};
};
