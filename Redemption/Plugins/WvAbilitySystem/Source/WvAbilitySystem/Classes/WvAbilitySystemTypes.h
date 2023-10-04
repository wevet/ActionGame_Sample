// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "Components/PrimitiveComponent.h"
#include "LegacyCameraShake.h"
#include "WvAbilityBase.h"
#include "WvAbilitySystemTypes.generated.h"


class USkeletalMeshComponent;

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


UENUM(BlueprintType)
enum class ECharacterRelation : uint8
{
	Friend UMETA(DisplayName = "Friend"),
	Enemy UMETA(DisplayName = "Enemy"),
};

USTRUCT(BlueprintType)
struct FWvOverlapResult
{
	GENERATED_BODY()

	bool IsValid() const;

	UPROPERTY()
	TWeakObjectPtr<class AActor> Actor;

	UPROPERTY()
	TWeakObjectPtr<class UPrimitiveComponent> Component;

	UPROPERTY()
	int32 ItemIndex = -1;

	FORCEINLINE AActor* GetActor() const
	{
		return Actor.Get();
	}

	FORCEINLINE UPrimitiveComponent* GetComponent() const
	{
		return Component.Get();
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

// All members of FHitResult are PODs.
template<> struct TIsPODType<FWvOverlapResult> { enum { Value = true }; };

template<>
struct TStructOpsTypeTraits<FWvOverlapResult> : public TStructOpsTypeTraitsBase2<FWvOverlapResult>
{
	enum
	{
		WithNetSerializer = true,
	};
};

USTRUCT(BlueprintType)
struct FWvApplyEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, float> MagnitudeSet;
};

USTRUCT(BlueprintType)
struct FWvAbilityTagConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer AbilityTags;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer ActivationOwnedTags;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer ActivationRequiredTags;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer ActivationBlockedTags;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer CancelAbilitiesWithTag;
};

USTRUCT(BlueprintType)
struct FWvAbilityConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FWvAbilityTagConfig TagConfig;

	UPROPERTY(EditDefaultsOnly)
	class UWvAbilityEffectDataAsset* EffectDataAsset;

	UPROPERTY(EditDefaultsOnly)
	FGameplayAttribute CostAttribute;

	UPROPERTY(EditDefaultsOnly)
	int32 CostAttributeMagnitude;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> CustomCooldownGE;

	UPROPERTY(EditDefaultsOnly)
	float InitialCD;

	UPROPERTY(EditDefaultsOnly)
	float CD;
};

//主人公の汎用スキルリスト
USTRUCT(BlueprintType)
struct FWvAbilityRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DamageMotion = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UWvAbilityDataAsset* AbilityData;
};

UENUM(BlueprintType)
enum class EMagicAttackType : uint8
{
	Magic = 0,
	Other
};

UENUM(BlueprintType)
enum class EMagicType : uint8
{
	Buff = 0,
	DeBuff,
	//補助
	Auxiliary,
	Attack,
	Shield,
	Recover,
	//武器エンチャント
	WeaponEnchantment,
	//状態異常
	AbnormalStatus
};

UENUM(BlueprintType)
enum class EMagicSubType : uint8
{
	None = 0,
	Ninjutsu,
	TopMagic,
	MiddleMagic,
	LowMagic,
	Throw,
	Trap,
	SummonMagic
};

UENUM(BlueprintType)
enum class EMagicElementType : uint8
{
	Nothing = 0,
	Drak,
	Wind,
	Bright,
	Fire,
	Wood,
	Water,
	Earth,
	Moon,
};

UENUM(BlueprintType)
enum class EMagicUseTargetPloy : uint8
{
	HostileCamp,
	FriendlyCamp,
	OnlySelf
};

UENUM(BlueprintType)
enum class EMagicUseRangeType : uint8
{
	RangeOne,
	RangeTwo,
	RangeThree,
	RangeFour,
	RangeFive,
	RangeSix
};

UENUM(BlueprintType)
enum class EMagicUseRangeSize : uint8
{
	LargeRange,
	MiddleRange,
	SmallRange
};

UENUM(BlueprintType)
enum class EMagicUseCondition : uint8
{
	OnlyInBattle = 0,
	Any
};

UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	None UMETA(DisplayName = "None"),
	Left UMETA(DisplayName = "Right to left"),
	Right UMETA(DisplayName = "Left to right"),
	Up UMETA(DisplayName = "Bottom to top"),
	Down UMETA(DisplayName = "Top to bottom"),
	LeftDown_RightUp UMETA(DisplayName = "Lower left to upper right"),
	LeftUp_RightDown UMETA(DisplayName = "Upper left to lower right"),
	RightDown_LeftUp UMETA(DisplayName = "Lower right to upper left"),
	RightUp_LeftDown UMETA(DisplayName = "Upper right to lower left"),
	Back_Front UMETA(DisplayName = "Back to front"),
};

USTRUCT(BlueprintType)
struct FMagicAbilityRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DamageMotion = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicAttackType AttackType;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicType MagicType;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicSubType MagicSubType;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicElementType Type;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicUseTargetPloy TargetPloy;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicUseRangeType RangeType;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicUseRangeSize RangeSize;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicUseCondition UseCondition;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	bool UseWhenDangling;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	float SingDuration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> EffectIdx;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MagicCostAttribute = TEXT("MP");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MagicCostValue = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCanSwitchTarget = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UMagicAbilityDataAsset* AbilityData;
};

USTRUCT(BlueprintType)
struct FWvAbilitySystemAvatarData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	TSet<TSubclassOf<UAttributeSet>> AttributeSetClassList;

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	TSubclassOf<UAttributeSet> StatusAttributeSetClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	UDataTable* GenericAbilityTable;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	UDataTable* MagicAbilityTable;

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	FGameplayTagContainer ExcludeGenericAbilityTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	UDataTable* CustomAbilityTable;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	UDataTable* AiStrategyTable;

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	TArray<TSubclassOf<class UGameplayEffect>> StartupEffects;

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	UDataTable* DebugAbilityTable;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	UWvHitReactDataAsset* HitReactData;
};

USTRUCT(BlueprintType)
struct FHitOscillatorData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VOscillator)
	float OscillatorDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VOscillator)
	bool bShakeBody = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VOscillator)
	struct FFOscillator X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VOscillator)
	struct FFOscillator Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VOscillator)
	struct FFOscillator Z;
};

USTRUCT(BlueprintType)
struct FOnceApplyEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FWvApplyEffect> TargetApplyEffect;

	UPROPERTY(EditDefaultsOnly)
	bool bActiveSelfOnce = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FWvApplyEffect> SelfApplyEffect;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadWrite)
	TArray<UWvHitFeedback*> TargetReact;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadWrite)
	TArray<UWvHitFeedback*> Decomposition;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadWrite)
	TArray<UWvHitFeedback*> SelfFeedback;

	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadWrite)
	UApplyEffectExData* ExData;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly)
	FString Comment;
#endif


};

USTRUCT(BlueprintType)
struct FWvGameplayEffectParamSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag ParamTag;

	UPROPERTY(EditDefaultsOnly)
	float ParamDefaultMagnitude;
};

USTRUCT(BlueprintType)
struct FWvGameplayEffectParam : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditDefaultsOnly)
	TArray<FWvGameplayEffectParamSet> ParamSet;
};

USTRUCT()
struct FAbilityMeshMontage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class USkeletalMeshComponent* Mesh;

	UPROPERTY()
	class UAnimMontage* Montage;

	FAbilityMeshMontage() : Mesh(nullptr), Montage(nullptr)
	{}

	FAbilityMeshMontage(class USkeletalMeshComponent* InMesh, class UAnimMontage* InMontage)
		: Mesh(InMesh), Montage(InMontage)
	{}
};

USTRUCT()
struct FGameplayAbilityLocalAnimMontageForMesh
{
	GENERATED_BODY()

public:
	UPROPERTY()
	USkeletalMeshComponent* Mesh;

	UPROPERTY()
	FGameplayAbilityLocalAnimMontage LocalMontageInfo;

	FGameplayAbilityLocalAnimMontageForMesh() : Mesh(nullptr), LocalMontageInfo()
	{}

	FGameplayAbilityLocalAnimMontageForMesh(USkeletalMeshComponent* InMesh) : Mesh(InMesh), LocalMontageInfo()
	{}

	FGameplayAbilityLocalAnimMontageForMesh(USkeletalMeshComponent* InMesh, FGameplayAbilityLocalAnimMontage& InLocalMontageInfo) : Mesh(InMesh), LocalMontageInfo(InLocalMontageInfo)
	{}
};

USTRUCT()
struct FGameplayAbilityRepAnimMontageForMesh
{
	GENERATED_BODY();

public:
	UPROPERTY()
	USkeletalMeshComponent* Mesh;

	UPROPERTY()
	FGameplayAbilityRepAnimMontage RepMontageInfo;

	FGameplayAbilityRepAnimMontageForMesh() : Mesh(nullptr), RepMontageInfo()
	{
	}

	FGameplayAbilityRepAnimMontageForMesh(USkeletalMeshComponent* InMesh) : Mesh(InMesh), RepMontageInfo()
	{
	}
};

USTRUCT(BlueprintType)
struct FWvAbilityLocation
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool IsBaseAvatar;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "IsBaseAvatar"))
	TArray<FName> SocketNames;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FVector Offset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FRotator Rotator;

	FTransform GetTransform(FName SocketName, AActor* Avatar = nullptr) const;
};

USTRUCT(BlueprintType)
struct FGameplayTagRefTableRowBase : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsGlobal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ScopeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UObject> ScopeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer TagContainer;

public:

	FGameplayTagRefTableRowBase() :IsGlobal(false)
	{
		ScopeName = TEXT("");
	}
};

USTRUCT(BlueprintType)
struct FWvAbilityData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector SourceCollisionCenter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator CollisionVelocity;
};


UCLASS(BlueprintType)
class WVABILITYSYSTEM_API UWvHitReactDataAsset : public UDataAsset
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FWvHitReact
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FGameplayTag FeatureTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool IsFixed = false;
};

USTRUCT(BlueprintType)
struct FHitBoneShakeInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag WeaknessBoneShakeStrengthTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DefaultBoneShakeStrengthTag;
};

USTRUCT(BlueprintType)
struct FWvAbilityEffectCueConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag AttackCueTag;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag BeHitCueTag;

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag CommitTargetDataExecuteCueTag;
};


UCLASS(BlueprintType)
class WVABILITYSYSTEM_API UApplyEffectExData : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	virtual void PostLoaded(FOnceApplyEffect& ApplyEffect) {}
#endif

	UFUNCTION(BlueprintCallable)
	bool GetExactFeatureTag(FGameplayTag FeatureGroup, FGameplayTag& ExactTag);

	UFUNCTION(BlueprintCallable)
	bool GetExactFeatureTags(FGameplayTag FeatureGroup, FGameplayTagContainer& ExactTags);

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer FeatureTags;

	UPROPERTY(EditDefaultsOnly)
	FWvHitReact TargetHitReact;

	UPROPERTY(EditDefaultsOnly)
	EHitDirection HitDirection;

	UPROPERTY(EditDefaultsOnly)
	FWvAbilityEffectCueConfig CueConfig;

	UPROPERTY(EditDefaultsOnly)
	FHitBoneShakeInfo BoneShakeConfig;
};

UCLASS(DefaultToInstanced, BlueprintType, abstract)
class WVABILITYSYSTEM_API UWvHitFeedback : public UObject
{
	GENERATED_BODY()

public:

	virtual void DoFeedback(FGameplayEffectContextHandle EffectContextHandle, AActor* Target);
};

UCLASS(BlueprintType)
class WVABILITYSYSTEM_API UWvAbilityEffectDataAsset : public UDataAsset
{
	GENERATED_BODY()

protected:
#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
	void UpdateEffectGroup(struct FPropertyChangedChainEvent& PropertyChangedEvent);
	virtual void PostLoad() override;
	void CreateExData(FOnceApplyEffect& Effect);
#endif

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FName, float> FloatSetter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FName, FName> NameSetter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FName, TSubclassOf<UObject>> ClassPropertySetter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FName, UObject*> ObjectSetter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FName, bool> BoolSetter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FName, FGameplayTag> TagSetter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TMap<FName, FVector> VectorSetter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FOnceApplyEffect> AbilityEffectGroup;
};


UENUM(BlueprintType)
enum class EWvBattleDamageAttackSourceType : uint8
{
	None,
	BasicMelee,
	Bullet,
};

USTRUCT(BlueprintType)
struct FWvBattleDamageAttackSourceInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWvBattleDamageAttackSourceType SourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UWvAbilityBase* SourceAbility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WeaponID;

};

USTRUCT(BlueprintType)
struct FWvBattleDamageAttackTargetInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class AActor* Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitResult HitResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> WeaknessNames;

	UPROPERTY(BlueprintReadOnly)
	FName MaxDamageWeaknessName;
};



