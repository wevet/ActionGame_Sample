// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAbilityBase.h"

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "LegacyCameraShake.h"
#include "NativeGameplayTags.h"
#include "WvAbilitySystemTypes.generated.h"


// Avatar
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Default);

// Character
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Player);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Enemy);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Neutral);

WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_Attack_Ability);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_Passive_Ability);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_EffectContext_Damage_Value);

// State
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateMelee);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateHitReact);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateDead);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateDead_Action);

// Alive
// ability playing added owner tag
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateAlive); 
// ability trigger tag
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateAlive_Action); 
// player input trigger tag
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateAlive_Trigger);

// Damage
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_DamageBlock);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_DamageKill);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_DamageReaction);

// HitReact
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_HitReact_Default_Character);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_HitReact_Default_Trigger);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_HitReact_Default_Streangth);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_HitReact_Default_Weakness);

// ShakeBone
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ShakeBone_Default_Character);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ShakeBone_Default_Trigger);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ShakeBone_Default_Streangth);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ShakeBone_Default_Weakness);

// FWvHitReact set config tag
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Config_HitReactFeature_Hit);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Config_HitReactFeature_Dead);

// HoldUp 
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_HoldUp);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_HoldUp_Sender);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_HoldUp_Receiver);

// KnockOut
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut_Sender);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut_Receiver);

// Finisher
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher_Sender);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher_Receiver);

// GameplayCue
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Attack);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Damage);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Weakness);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Bullet);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Scar);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Environment_BulletHit);

// Throw HitReaction
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_PassiveAbilityTrigger_HitReact);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_PassiveAbilityTrigger_KillReact);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_PassiveAbilityTrigger_KillTarget);


class USkeletalMeshComponent;
class UPrimitiveComponent;


#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


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

// character common skill list
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
	//Assist
	Auxiliary,
	Attack,
	Shield,
	Recover,
	// weapon enchantment
	WeaponEnchantment,
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
enum class EAIActionState : uint8
{
	None UMETA(DisplayName = "None"),
	Patrol UMETA(DisplayName = "Patrol"),
	Search UMETA(DisplayName = "Search"),
	Combat UMETA(DisplayName = "Combat"),
	Follow UMETA(DisplayName = "Follow"),
	Friendly UMETA(DisplayName = "Friendly"),
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


USTRUCT(BlueprintType)
struct FWvHitReact
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FGameplayTag FeatureTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FGameplayTag FixFeatureTag;

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
	Bomb,
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


#pragma region HitReaction
USTRUCT(BlueprintType)
struct FHitReactRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly)
	class UAnimMontage* Montage = nullptr;
};

UENUM(BlueprintType)
enum class EHitReactDirection : uint8
{
	Forward,
	Back,
	Left,
	Right,
};

UENUM(BlueprintType)
enum class EHitVerticalDirection : uint8
{
	Top,
	Bottom,
	Middle,
};

USTRUCT(BlueprintType)
struct FHitReactConditionInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitReactDirection HitDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UAnimMontage* Montage = nullptr;
};

USTRUCT(BlueprintType)
struct FHitReactVerticalConditionInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitVerticalDirection VerticalDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FHitReactConditionInfo> Montages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UAnimMontage* NormalMontage = nullptr;
};

UENUM(BlueprintType)
enum class EDynamicHitDirection : uint8
{
	NONE,
	HitDirection,
	FaceToAttacker,
};

USTRUCT(BlueprintType)
struct FHitReactInfoRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FHitReactVerticalConditionInfo> VerticalConditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FHitReactConditionInfo> CrouchingMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDynamicHitDirection DynamicHitDirection { EDynamicHitDirection::FaceToAttacker };
};

UCLASS()
class WVABILITYSYSTEM_API UWvHitReactDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UDataTable* NormalHitReactTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UDataTable* SpecialHitReactTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, class UDataTable*> WeaponHitReactTables;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag BoneShakeTriggerTag;

public:
	const FHitReactInfoRow* GetHitReactInfoRow_Normal(const UAbilitySystemComponent* ASC, const FGameplayTag& Tag);
	const FHitReactInfoRow* GetHitReactInfoRow_Weapon(const FName WeaponName, const UAbilitySystemComponent* ASC, const FGameplayTag& Tag);
	const FHitReactInfoRow* GetHitReactInfoRow_Special(const UAbilitySystemComponent* ASC, const FGameplayTag& Tag);

protected:
	const FHitReactInfoRow* GetHitReactInfoRow(UDataTable* Table, const UAbilitySystemComponent* ASC, const FGameplayTag& AbilityMontageFilter);

};

USTRUCT(BlueprintType)
struct FNearestShakableBone
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Bone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight;
};

USTRUCT(BlueprintType)
struct FTransmitShakableBoneInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, float> OtherBoneTransmitShakeStrength;
};

USTRUCT(BlueprintType)
struct FHitReactBoneShake
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ShakeStrength = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ShakeDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* DampingCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransmitShakableBoneInfo Transmits;
};

USTRUCT(BlueprintType)
struct FHitReactBoneShakeStrengthConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Strength{ 1.f };

	// bone jitter data
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FHitReactBoneShake> BoneShakeData;
};

USTRUCT(BlueprintType)
struct FSkeletalMeshShakeData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, FHitReactBoneShakeStrengthConfig> StrengthBoneShakeData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> LockBoneNams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FNearestShakableBone> NearestShakableBoneData;
};

UCLASS(BlueprintType)
class WVABILITYSYSTEM_API UHitReactBoneShakeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	//Trigger tag - hit jitter data for skeletal mesh bodies
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, FSkeletalMeshShakeData> SkeletalShakeData;
};

UCLASS(MinimalAPI, Blueprintable)
class UBoneShakeExecuteData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FName BoneName;

	UPROPERTY(BlueprintReadOnly)
	FVector ShakeOffsetLocation { FVector::ZeroVector };

	FVector SourceLocation;
	float Strength = 0.f;
	float TotalTime = 0.f;
	float CurTime = 0.f;
	float Direction = 0.f;

	UPROPERTY()
	UCurveFloat* DampingCurve = nullptr;
};

UCLASS(MinimalAPI)
class USkeletalMeshBoneShakeExecuteData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<UBoneShakeExecuteData*> BoneShakeDatas;
	FVector HitDirection;
};

UCLASS(Blueprintable)
class UAAU_HitReactBoneShakeDATool : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ResetHitReactBoneShakeDA(class UHitReactBoneShakeDataAsset* DA, class USkeletalMesh* SkeletalMesh, FGameplayTag TriggetTag, FGameplayTag StrengthTag, float BaseStrength, float ShakeBoneStrength, float ShakeDuration, class UCurveFloat* DampingCurve, float NearestBoneDistance, float ShakeBoneDistance);

	UFUNCTION(BlueprintCallable)
	void ResetSkeletalBoneData(class UHitReactBoneShakeDataAsset* DA, class USkeletalMesh* SkeletalMesh, FGameplayTag TriggetTag, FGameplayTag StrengthTag, float NearestBoneDistance, float ShakeBoneDistance);

private:
	void SkeletalMeshCalculateAllNearestShakeBone(USkeletalMesh* SkeletalMesh, TArray<FName> ShakeBoneNames, TMap<FName, FTransform> boneName2PosDict, float BoneDistance, FSkeletalMeshShakeData& SkeletalMeshShakeData);
	void SkeletalMeshCalculateShakeBoneTransmitStrength(USkeletalMesh* SkeletalMesh, TArray<FName> ShakeBoneNames, TMap<FName, FTransform> boneName2PosDict, float BoneDistance, FHitReactBoneShakeStrengthConfig& HitReactBoneShakeStrengthConfig);
	void SaveDA(class UHitReactBoneShakeDataAsset* DA);
};
#pragma endregion


#pragma region BotSetting
USTRUCT(BlueprintType)
struct FBotConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	float FootStepMaxVolume = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	FVector2D HearingRange {0.f, 500.0f};
};

USTRUCT(BlueprintType)
struct FAIActionStateData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAIActionState AIActionState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag StateTag;
};

// set character classes
UCLASS(BlueprintType)
class WVABILITYSYSTEM_API UAIActionStateDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FAIActionStateData> AIActionStateData;

	FGameplayTag FindActionStateTag(const EAIActionState InAIActionState) const;
};
#pragma endregion


#pragma region Finiher
UENUM(BlueprintType)
enum class EFinisherDirectionType : uint8
{
	Always UMETA(DisplayName = "Always"),
	Forward UMETA(DisplayName = "Forward"),
	Backward UMETA(DisplayName = "Backward"),
};

USTRUCT(BlueprintType)
struct FFinisherAnimation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = Sender)
	bool bIsSender = false;

	UPROPERTY(EditDefaultsOnly, Category = Receiver)
	bool bIsReceiver = false;

	UPROPERTY(EditDefaultsOnly, Category = Sender, meta = (EditCondition = "bIsSender"))
	EFinisherDirectionType FinisherDirectionType = EFinisherDirectionType::Backward;

	UPROPERTY(EditDefaultsOnly, Category = Sender, meta = (EditCondition = "bIsSender"))
	float PushDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = Receiver, meta = (EditCondition = "bIsReceiver"))
	bool IsTurnAround = true;

	UPROPERTY(EditDefaultsOnly)
	class UAnimMontage* Montage;
};

USTRUCT(BlueprintType)
struct FFinisherAnimationContainer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TArray<FFinisherAnimation> Montages;
};

UCLASS(BlueprintType)
class WVABILITYSYSTEM_API UFinisherDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, FFinisherAnimationContainer> FinisherAnimationMap;

	FFinisherAnimationContainer FindContainer(const FGameplayTag Tag) const;
};

USTRUCT(BlueprintType)
struct FFinisherConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	float NearlestDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly)
	float AngleThreshold = 20.0f;
};
#pragma endregion


#pragma region EnvironmentSetting
USTRUCT(BlueprintType)
struct FEnvironmentConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	float HitSoundVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float Roudness = 500.0f;
};
#pragma endregion


#pragma region VehicleTraceConfig
USTRUCT(BlueprintType)
struct FVehicleTraceConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	float ClosestTargetDistance = 400.0f;

	UPROPERTY(EditDefaultsOnly)
	float NearestDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly)
	FVector2D ViewRange{ 45.0f, 110.0f };
};
#pragma endregion


USTRUCT(BlueprintType)
struct FBotLeaderConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	float FormationRadius = 200.0f;

	UPROPERTY(EditDefaultsOnly)
	int32 FollowStackCount = 5;
};
