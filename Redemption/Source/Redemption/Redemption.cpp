// Copyright 2022 wevet works All Rights Reserved.

#include "Redemption.h"
#include "Modules/ModuleManager.h"


// Attack
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_StateMelee, "Character.State.Melee");

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
 