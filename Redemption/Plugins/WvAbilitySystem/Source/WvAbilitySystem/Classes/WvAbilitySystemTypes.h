// Copyright 2020 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WvAbilityBase.h"

#include "AttributeSet.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "LegacyCameraShake.h"
#include "NativeGameplayTags.h"
#include "Abilities/GameplayAbilityRepAnimMontage.h"
#include "WvAbilitySystemTypes.generated.h"


class UAbilitySystemComponent;
class USkeletalMeshComponent;
class UPrimitiveComponent;


// Avatar
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Default);

// Character
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Player);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Enemy);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Neutral);

WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_Attack_Ability);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_Passive_Ability);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_Friendly_Ability);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_Attack_Coefficient);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_AttackWeakness_Coefficient);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_EffectContext_Health_Value);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_EffectContext_Damage_Value);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_EffectContext_Skill_Value);

// State
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateMelee);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateHitReact); // DA_Repel State
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateDead); // Add DA_Death After 
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateDead_Action);

// Status gameplay effect
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Status_Buff_Damage);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Status_Buff_Skill);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Status_Buff_Recover);

// any character Action to be taken by hitting an object
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateBlock);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateBlock_Action);

// Skill
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateSkill); //using skill playing
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateSkill_Trigger); // trigger tag
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateSkill_Enable); // enabling added tag
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateSkill_Ready); // 


// melee counter action
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateCounter);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_StateCounter_Action);

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
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_HoldUp_Playing);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_HoldUp_Ignore);

// KnockOut
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut_Sender);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut_Receiver);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut_Playing);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_KnockOut_Ignore);

// Finisher
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher_Sender);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher_Receiver);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher_Playing);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Finisher_Ignore);

// weapon type
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Melee_Default);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Melee_Knife);

WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Melee_Pistol);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Melee_Pistol_AmmoEmpty);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Melee_Rifle);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Melee_Rifle_AmmoEmpty);

// bullet weapon state
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Gun_Reload);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Gun_Fire);

// equip unequip action
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Action_Equip);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Action_UnEquip);

// GameplayCue
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Attack);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Damage);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Weakness);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Bullet);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Scar);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GameplayCue_HitImpact_Environment_BulletHit);

// Throw HitReaction
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_PassiveAbilityTrigger_HitReact); // DA_Repel
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_PassiveAbilityTrigger_KillReact); // DA_Death
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Common_PassiveAbilityTrigger_KillTarget);

// AI 
// waypoint visited
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Charactre_AI_Waypoint_Visited);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Charactre_AI_Waypoint_UnVisited);

// AI ActionState ABaseCharacter::SetAIActionState using
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_State_Search);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_State_Combat);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_State_Patrol);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_State_Follow);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_State_Friend);

WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_Leader);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_Ignore);

// using friend ai action abp used
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_Friend_Action);
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_Friend_Action_Playing);

// fooley asset tag FoleyEvent
WVABILITYSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Plugin_Game_Asset_FoleyEvent);




#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


UENUM(BlueprintType)
enum class EGenderType : uint8
{
	Male   UMETA(DisplayName = "Male"),
	Female UMETA(DisplayName = "Female"),
};

UENUM(BlueprintType)
enum class EBodyShapeType : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Over   UMETA(DisplayName = "Over"),
	Under  UMETA(DisplayName = "Under"),
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
	None = 0,
	Buff,
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
	None = 0,
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

	FWvApplyEffect() : EffectClass(nullptr)
	{
	}
};

USTRUCT(BlueprintType)
struct FWvAbilityTagConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer AbilityTags { FGameplayTagContainer::EmptyContainer };

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer ActivationOwnedTags{ FGameplayTagContainer::EmptyContainer };

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer ActivationRequiredTags{ FGameplayTagContainer::EmptyContainer };

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer ActivationBlockedTags{ FGameplayTagContainer::EmptyContainer };

	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer CancelAbilitiesWithTag{ FGameplayTagContainer::EmptyContainer };
};

USTRUCT(BlueprintType)
struct FWvAbilityConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FWvAbilityTagConfig TagConfig;

	UPROPERTY(EditDefaultsOnly)
	class UWvAbilityEffectDataAsset* EffectDataAsset{nullptr};

	UPROPERTY(EditDefaultsOnly)
	FGameplayAttribute CostAttribute;

	UPROPERTY(EditDefaultsOnly)
	int32 CostAttributeMagnitude{0};

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> CustomCooldownGE{nullptr};

	UPROPERTY(EditDefaultsOnly)
	float InitialCD{0.f};

	UPROPERTY(EditDefaultsOnly)
	float CD{ 0.f };
};

// character common skill list
USTRUCT(BlueprintType)
struct FWvAbilityRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DamageMotion = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UWvAbilityDataAsset* AbilityData{ nullptr };
};



USTRUCT(BlueprintType)
struct FMagicAbilityRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DamageMotion = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicAttackType AttackType{ EMagicAttackType::Magic};

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicType MagicType{ EMagicType::None};

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicSubType MagicSubType{ EMagicSubType::None};

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicElementType Type{ EMagicElementType::None};

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicUseTargetPloy TargetPloy{EMagicUseTargetPloy::HostileCamp };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicUseRangeType RangeType{EMagicUseRangeType::RangeOne };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicUseRangeSize RangeSize{ EMagicUseRangeSize::SmallRange};

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	EMagicUseCondition UseCondition{ EMagicUseCondition::Any};

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	bool UseWhenDangling{false};

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	float SingDuration{0.f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> EffectIdx;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MagicCostAttribute = TEXT("MP");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MagicCostValue{ 0.f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCanSwitchTarget{ false };

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UMagicAbilityDataAsset* AbilityData{nullptr};
};

USTRUCT(BlueprintType)
struct FWvAbilitySystemAvatarData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	TSet<TSubclassOf<UAttributeSet>> AttributeSetClassList;

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	TSubclassOf<UAttributeSet> StatusAttributeSetClass{nullptr};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	UDataTable* GenericAbilityTable{ nullptr };

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	FGameplayTagContainer ExcludeGenericAbilityTags{FGameplayTagContainer::EmptyContainer};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	UDataTable* CustomAbilityTable{ nullptr };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	UDataTable* AiStrategyTable{ nullptr };

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	TArray<TSubclassOf<class UGameplayEffect>> StartupEffects;

	UPROPERTY(EditAnywhere, Category = "AbilitySystem")
	UDataTable* DebugAbilityTable{ nullptr };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AbilitySystem")
	UWvHitReactDataAsset* HitReactData{ nullptr };
};

USTRUCT(BlueprintType)
struct FHitOscillatorData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VOscillator)
	float OscillatorDuration{0.f};

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
	UApplyEffectExData* ExData{nullptr};

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
	FGameplayTag ParamTag{FGameplayTag::EmptyTag};

	UPROPERTY(EditDefaultsOnly)
	float ParamDefaultMagnitude{0.f};
};

USTRUCT(BlueprintType)
struct FWvGameplayEffectParam : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass{nullptr};

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
struct FCharacterInfo
{
	GENERATED_BODY()

public:
	UPROPERTY()
	EBodyShapeType BodyShapeType;

	UPROPERTY()
	EGenderType GenderType;

	UPROPERTY()
	FName Name;

	FCharacterInfo()
	{
		BodyShapeType = EBodyShapeType::Normal;
		GenderType = EGenderType::Male;
		Name = NAME_None;
	}

};

USTRUCT(BlueprintType)
struct FWvAbilityLocation
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool IsBaseAvatar{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "IsBaseAvatar"))
	TArray<FName> SocketNames;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FVector Offset{FVector::ZeroVector};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FRotator Rotator{FRotator::ZeroRotator};

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
	FVector SourceCollisionCenter{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRotator CollisionVelocity{FRotator::ZeroRotator};
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
	EWvBattleDamageAttackSourceType SourceType{ EWvBattleDamageAttackSourceType::None};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UWvAbilityBase* SourceAbility{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WeaponID{0};

};

USTRUCT(BlueprintType)
struct FWvBattleDamageAttackTargetInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class AActor* Target{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitResult HitResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> WeaknessNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ComboAttackCoefficient{ 1 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsOverrideGEDamageValue = false;

	UPROPERTY()
	FName MaxDamageWeaknessName;

	void SetMaxDamageWeaknessName(const FName NewMaxDamageWeaknessName);
	FName GetMaxDamageWeaknessName() { return MaxDamageWeaknessName; }

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
	EHitReactDirection HitDirection{ EHitReactDirection::Forward};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UAnimMontage* Montage = nullptr;
};

USTRUCT(BlueprintType)
struct FHitReactVerticalConditionInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHitVerticalDirection VerticalDirection{ EHitVerticalDirection::Middle};

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
	class UDataTable* NormalHitReactTable{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UDataTable* SpecialHitReactTable{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, class UDataTable*> WeaponHitReactTables;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag BoneShakeTriggerTag{ FGameplayTag::EmptyTag };

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
	FName Bone{NAME_None};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight{0.f};
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
	float ShakeDuration{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* DampingCurve{nullptr};

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
	FName BoneName{NAME_None};

	UPROPERTY(BlueprintReadOnly)
	FVector ShakeOffsetLocation { FVector::ZeroVector };

	FVector SourceLocation{ FVector::ZeroVector };
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
	FVector HitDirection{FVector::ZeroVector};
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
	float PlayerDistThreshold = 300.0f;

	UPROPERTY(EditDefaultsOnly)
	FVector2D HearingRange {0.f, 500.0f};
};

USTRUCT(BlueprintType)
struct FAIActionStateData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAIActionState AIActionState{EAIActionState::None};

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
	class UAnimMontage* Montage{nullptr};
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


#pragma region CloseCombat
USTRUCT(BlueprintType)
struct FCloseCombatAnimation
{
	GENERATED_BODY()

public:
	FCloseCombatAnimation()
	{
		DefaultComboMontage = nullptr;
	}

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* DefaultComboMontage;

	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, UAnimMontage*> TagToComboMontageMap;

	UAnimMontage* GetComboMatchMontage(const FGameplayTag ComboTag) const;

	int32 GetTotalAnimationCount() const;
};


USTRUCT(BlueprintType)
struct FCloseCombatAnimationContainer
{
	GENERATED_BODY()

public:
	FCloseCombatAnimationContainer()
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCloseCombatAnimation> ComboAnimations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsRandomPlayRate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bIsRandomPlayRate"))
	FVector2D PlayRateRange{ 0.75f, 1.0f};
};


UCLASS(BlueprintType)
class WVABILITYSYSTEM_API UCloseCombatAnimationDataAsset : public UDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EBodyShapeType, FCloseCombatAnimationContainer> ComboAnimationContainer;

	FCloseCombatAnimationContainer FindContainer(const EBodyShapeType BodyShape) const;

public:
	FCloseCombatAnimation GetChooseCombatAnimation(const EBodyShapeType BodyShape, const int32 Index) const;

	int32 GetCombatAnimationIndex(const EBodyShapeType BodyShape) const;
	int32 CloseCombatMaxComboCount(const EBodyShapeType BodyShape, const int32 Index) const;

	UAnimMontage* GetAnimMontage(const EBodyShapeType BodyShape, const int32 Index, const FGameplayTag Tag) const;

	float CalcurateBodyShapePlayRate(const EBodyShapeType BodyShape) const;
};
#pragma endregion


#pragma region CharacterVFX
UCLASS(BlueprintType)
class WVABILITYSYSTEM_API UCharacterVFXDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* OverlayMaterial{nullptr};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* FaceOverlayMaterial{ nullptr };
};
#pragma endregion


USTRUCT(BlueprintType)
struct FWvWeaknessDataTableRow : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName AttachBoneName{NAME_None};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsAutoActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool ContinueActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsOpenUse = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHP = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AttackCoefficient = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsOpenPerceive{ true };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsShowHitNiagara{ true };

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag TriggerHitReactFeature{ FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageAccumulation{0.f};
};

USTRUCT(BlueprintType)
struct FWvWeaknessInfo
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WeaknessName{NAME_None};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FWvWeaknessDataTableRow ConfigInfo;
};


USTRUCT(BlueprintType)
struct WVABILITYSYSTEM_API FSkillAnimMontage
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag CharacterTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBodyShapeType BodyShapeType = EBodyShapeType::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* AnimMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PlayRate = 1.0f;
};




