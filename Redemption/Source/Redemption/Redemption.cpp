// Copyright Epic Games, Inc. All Rights Reserved.

#include "Redemption.h"
#include "Modules/ModuleManager.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_MovementStopped, "Gameplay.MovementStopped");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_CombatReady, "Character.Combat.Ready");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Footstep_Left, "Character.Locomotion.Footstep.Left");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Footstep_Right, "Character.Locomotion.Footstep.Right");

UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingJump, "Locomotion.Climbing.Jump");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingMovement, "Locomotion.Climbing.Movement");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingStop, "Locomotion.Climbing.Stop");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingLedgeEnd, "Locomotion.Climbing.LedgeEnd");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingForbid, "Locomotion.Climbing.Forbid");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_Vaulting, "Locomotion.Vaulting");


IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, Redemption, "Redemption" );
 