// Copyright 2022 wevet works All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "Modules/ModuleManager.h"

// using
// *GETENUMSTRING("/Script/ProjectName.EnumName")
#define GETENUMSTRING(etype, evalue)\
	 ((FindObject<UEnum>(nullptr, TEXT(etype), true) != nullptr) ? FindObject<UEnum>(nullptr, TEXT(etype), true)->GetNameStringByIndex((int32)evalue) : FString("Invalid - UENUM() macro?"))

#define WEVET_COMMENT(Comment)

// Input
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_Input_Disable);

// PlayerAction
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Player_Melee);

// Is AI allowed to attack Player?
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_AI_NotAllowed_Attack);

// QTE Command Enable
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Action_QTE);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Action_QTE_Pressed);

// Combo frag tag
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionMelee_ComboRequire);

// Attack frag tag
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionMelee_Forbid);

// look at target
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionLookAt);

// Melee Action 
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionMelee);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionMelee_Hold);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionMelee_Combo1);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionMelee_Combo2);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionMelee_Combo3);

// Knife Action
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionKnife);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionKnife_Hold);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionKnife_Combo1);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionKnife_Combo2);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionKnife_Combo3);

// Combat Chain
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionCombatChain);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionCombatChain_Trigger);

// Rotation Mode Change
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionStrafeChange);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionAimChange);

// JumpAction
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionJump);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionJump_Forbid);

// Dash
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionDash);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionDash_Forbid);

// Walk
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionWalk);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionWalk_Forbid);

// Crouch
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionCrouch);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionCrouch_Forbid);

// Drive
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionDrive);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_ActionDrive_Forbid);

// if reload active tag added character state tag
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Action_GunReload);

// TargetLockOn
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_TargetLock);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_TargetLocking);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_TargetLock_Forbid);

// cinematic
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_Action_Cinematic);


// Climbing
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Locomotion_ClimbingLedgeEnd);

// Mantling
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Locomotion_Mantling);

// Vault Hurdle Mantle
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Locomotion_Traversal);

// Forbid
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Locomotion_ForbidClimbing);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Locomotion_ForbidMantling);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Locomotion_ForbidMovement);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Locomotion_ForbidJump);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Locomotion_ForbidRagdoll);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Locomotion_ForbidTraversal);


// vehicle
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Vehicle_Drive); // drive action start
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Vehicle_UnDrive); // drive actoin end
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Vehicle_State_Drive); // current is driving

// smart object difinition
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Character_State_SmartObject_Using);

// async load data asset tag
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_Asset_HitReaction);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_Asset_FinisherSender);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_Asset_FinisherReceiver);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_Asset_CloseCombat);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_Asset_CharacterVFX);


// minimap
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_Minimap_Player);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_Minimap_KeyCharacter);
REDEMPTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_Minimap_EventCheckPoint);

/*
* AIPerception Sight CancelEvent Interval
*/
#define SIGHT_AGE 20.0f

/*
* AIPerception Hear CancelEvent Interval
*/
#define HEAR_AGE 30.0f

/*
* AIPerception Follow CancelEvent Interval
*/
#define FOLLOW_AGE -1.0

/*
* AIPerception Friend CancelEvent Interval
*/
#define FRIEND_AGE 30.0


#define NEARLEST_TARGET_SYNC_POINT FName(TEXT("NearlestTarget"))
#define AI_NEARLEST_TARGET_SYNC_POINT FName(TEXT("AI_NearlestTarget"))
#define FINISHER_TARGET_SYNC_POINT FName(TEXT("FinisherTarget"))

/*
* project custom collision preset
*/
#define K_CHARACTER_COLLISION_PRESET FName(TEXT("BaseCharacter"))

/*
* actor and component add tagname
*/
#define K_QTE_COMPONENT_TAG FName(TEXT("QTE"))

/*
* save game
*/
#define K_PLAYER_SLOT_NAME TEXT("PlayerID")

#define FPS_60 1.0/60

#define K_LOCK_ON_WIDGET_TAG FName("TargetSystem.LockOnWidget")

#define QTE_SYSTEM_RECEIVE 1

#define K_MOVING_THRESHOLD 3.0f

#define K_WALKABLE_FLOOR_ANGLE 50.0f

// character dead reason
#define K_REASON_DEAD TEXT("Character Dead")
#define K_REASON_FREEZE TEXT("Character Freeze")
#define K_REASON_UNFREEZE TEXT("Character UnFreeze")
#define K_REASON_SYSTEM_HIDDEN TEXT("System Hidden")

#define K_FOLLOW_NUM 5

#define K_WALK_SPEED 200.0f
#define K_RUNNING_SPEED 400.0f
#define K_SPRINTING_SPEED 700.0f
#define K_CROUCHING_SPEED 250.0f

#define K_SIGNIGICANCE_ACTOR FName(TEXT("Significance"))



class FRedemptionModule : public IModuleInterface
{
public:
	static FRedemptionModule* GameModuleInstance;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

protected:
};
