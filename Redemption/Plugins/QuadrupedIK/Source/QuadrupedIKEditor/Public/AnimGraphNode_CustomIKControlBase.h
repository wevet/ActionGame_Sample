// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BonePose.h"
#include "AnimGraphNode_Base.h"
#include "AnimGraphNode_CustomIKControlBase.generated.h"

class FCompilerResultsLog;
class USkeletalMeshComponent;
struct FAnimNode_CustomIKControlBase;
struct HActor;

/**
 * 
 */
UCLASS(Abstract)
class QUADRUPEDIKEDITOR_API UAnimGraphNode_CustomIKControlBase : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual FString GetNodeCategory() const override;
	virtual void CreateOutputPins() override;
	virtual void ValidateAnimNodePostCompile(FCompilerResultsLog& MessageLog, UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex) override;
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	bool IsPinShown(const FName PinName) const;

public:
	void SetDefaultValue(const FName InDefaultValueName, const FVector& InValue);
	void GetDefaultValue(const FName UpdateDefaultValueName, FVector& OutVec);

	void GetDefaultValue(const FName PropName, FRotator& OutValue)
	{
		FVector Value;
		GetDefaultValue(PropName, Value);
		OutValue.Pitch = Value.X;
		OutValue.Yaw = Value.Y;
		OutValue.Roll = Value.Z;
	}

	template<class ValueType>
	ValueType GetNodeValue(const FName PropName, const ValueType& CompileNodeValue)
	{
		if (IsPinShown(PropName))
		{
			ValueType Val;
			GetDefaultValue(PropName, Val);
			return Val;
		}
		return CompileNodeValue;
	}

	void SetDefaultValue(const FName PropName, const FRotator& InValue)
	{
		FVector VecValue(InValue.Pitch, InValue.Yaw, InValue.Roll);
		SetDefaultValue(PropName, VecValue);
	}

	template<class ValueType>
	void SetNodeValue(const FName PropName, ValueType& CompileNodeValue, const ValueType& InValue)
	{
		if (IsPinShown(PropName))
		{
			SetDefaultValue(PropName, InValue);
		}
		CompileNodeValue = InValue;
	}

protected:
	virtual FText GetControllerDescription() const;
	virtual const FAnimNode_CustomIKControlBase* GetNode() const PURE_VIRTUAL(UAnimGraphNode_CustomIKControlBase::GetNode, return nullptr;);
};


