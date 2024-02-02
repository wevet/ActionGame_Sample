// Copyright 2022 wevet works All Rights Reserved.

#include "Redemption.h"
#include "Modules/ModuleManager.h"

// Input
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Input_Disable, "Game.Input.Disable");

// PlayerAction
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Player_Melee, "Character.Player.Melee");

// MeleeAction
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee, "Character.Action.Melee");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_ComboRequire, "Character.Action.Melee.ComboRequire");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Combo1, "Character.Action.Melee.Combo1");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Combo2, "Character.Action.Melee.Combo2");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Combo3, "Character.Action.Melee.Combo3");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Forbid, "Character.Action.Melee.Forbid");

// MeleeActionHold
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Hold, "Character.Action.Melee.Hold");

// JumpAction
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionJump, "Character.Action.Jump");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionJump_Forbid, "Character.Action.Jump.Forbid");

// Dash
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDash, "Character.Action.Dash");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDash_Forbid, "Character.Action.Dash.Forbid");

// Crouch
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionCrouch, "Character.Action.Crouch");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionCrouch_Forbid, "Character.Action.Crouch.Forbid");

// Drive
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDrive, "Character.Action.Drive");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDrive_Forbid, "Character.Action.Drive.Forbid");

// if reload active tag added character state tag
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Action_GunReload, "Character.Action.GunReload");

UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionStrafeChange, "Character.Action.StrafeChange");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionAimChange, "Character.Action.AimChange");

// TargetLock
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_TargetLock, "Character.Action.TargetLock");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_TargetLocking, "Character.Action.TargetLocking");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_TargetLock_Forbid, "Character.Action.TargetLock.Forbid");

// Climbing 
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingJump, "Locomotion.Climbing.Jump");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingLedgeEnd, "Locomotion.Climbing.LedgeEnd");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingAbort, "Locomotion.Climbing.Abort");

// Mantling
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_Mantling, "Locomotion.Mantling");

// Forbid
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidClimbing, "Locomotion.Forbid.Climbing");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidMantling, "Locomotion.Forbid.Mantling");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidMovement, "Locomotion.Forbid.Movement");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidJump, "Locomotion.Forbid.Jump");

// weapon type
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Melee_Default, "Weapon.Melee.Default");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Melee_Knife, "Weapon.Melee.Knife");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Melee_Pistol, "Weapon.Melee.Pistol");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Melee_Pistol_AmmoEmpty, "Weapon.Melee.Pistol.Ammo.Empty");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Melee_Rifle, "Weapon.Melee.Rifle");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Melee_Rifle_AmmoEmpty, "Weapon.Melee.Rifle.Ammo.Empty");

// bullet weapon state
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Gun_Reload, "Weapon.Gun.Reload");
UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Gun_Fire, "Weapon.Gun.Fire");


// AI 
// waypoint visited
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Waypoint_Visited, "AI.Waypoint.Visited");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Waypoint_UnVisited, "AI.Waypoint.UnVisited");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Search, "AI.State.Search");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Combat, "AI.State.Combat");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Patrol, "AI.State.Patrol");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Follow, "AI.State.Follow");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Friend, "AI.State.Friend");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Character_Leader, "AI.Character.Leader");

// vehicle
UE_DEFINE_GAMEPLAY_TAG(TAG_Vehicle_Drive, "Vehicle.Drive");
UE_DEFINE_GAMEPLAY_TAG(TAG_Vehicle_UnDrive, "Vehicle.UnDrive");
UE_DEFINE_GAMEPLAY_TAG(TAG_Vehicle_State_Drive, "Vehicle.State.Drive");


IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, Redemption, "Redemption" );

