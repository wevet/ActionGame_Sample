// Copyright 2022 wevet works All Rights Reserved.

#include "Redemption.h"
#include "Modules/ModuleManager.h"


// avatar
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Default, "Character.Default");

// character
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Player, "Character.Player");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Enemy, "Character.Enemy");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Neutral, "Character.Neutral");

// MeleeAction
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee, "Character.Action.Melee");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_ComboRequire, "Character.Action.Melee.ComboRequire");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Combo1, "Character.Action.Melee.Combo1");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Combo2, "Character.Action.Melee.Combo2");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Combo3, "Character.Action.Melee.Combo3");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Forbid, "Character.Action.Melee.Forbid");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateMelee, "Character.State.Melee");

// IsDead
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateDead, "Character.State.Dead");


// Jump
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionJump, "Character.Action.Jump");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionJump_Forbid, "Character.Action.Jump.Forbid");
// Dash
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDash, "Character.Action.Dash");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDash_Forbid, "Character.Action.Dash.Forbid");
// Crouch
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionCrouch, "Character.Action.Crouch");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionCrouch_Forbid, "Character.Action.Crouch.Forbid");

// climbing 
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingJump, "Locomotion.Climbing.Jump");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingMovement, "Locomotion.Climbing.Movement");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingStop, "Locomotion.Climbing.Stop");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingLedgeEnd, "Locomotion.Climbing.LedgeEnd");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_Mantling, "Locomotion.Mantling");

// forbid 
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidClimbing, "Locomotion.Forbid.Climbing");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidMantling, "Locomotion.Forbid.Mantling");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidMovement, "Locomotion.Forbid.Movement");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidJump, "Locomotion.Forbid.Jump");


IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, Redemption, "Redemption" );

