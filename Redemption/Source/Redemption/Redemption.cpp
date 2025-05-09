// Copyright 2022 wevet works All Rights Reserved.

#include "Redemption.h"
#include "Modules/ModuleManager.h"
#include "Game/WvProjectSetting.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#endif // WITH_EDITOR



// Input
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Input_Disable, "Game.Input.Disable");

// PlayerAction
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Player_Melee, "Character.Player.Melee");

// Is AI allowed to attack Player?
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_AI_NotAllowed_Attack, "Character.AI.NotAllowed.Attack");

// QTE Command Enable
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Action_QTE, "Character.Action.QTE");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Action_QTE_Pressed, "Character.Action.QTE.Pressed");

// Combo frag tag
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_ComboRequire, "Character.Action.Melee.ComboRequire");

// Attack frag tag
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Forbid, "Character.Action.Melee.Forbid");

// look at target
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionLookAt, "Character.Action.LookAt");

// Melee Action 
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee, "Character.Action.Melee");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Hold, "Character.Action.Melee.Hold");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Combo1, "Character.Action.Melee.Combo1");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Combo2, "Character.Action.Melee.Combo2");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionMelee_Combo3, "Character.Action.Melee.Combo3");

// Knife Action
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionKnife, "Character.Action.Knife");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionKnife_Hold, "Character.Action.Knife.Hold");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionKnife_Combo1, "Character.Action.Knife.Combo1");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionKnife_Combo2, "Character.Action.Knife.Combo2");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionKnife_Combo3, "Character.Action.Knife.Combo3");

// Combat Chain
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionCombatChain, "Character.Action.CombatChain");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionCombatChain_Trigger, "Character.Action.CombatChain.Trigger");

// Rotation Mode Change
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionStrafeChange, "Character.Action.StrafeChange");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionAimChange, "Character.Action.AimChange");

// JumpAction
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionJump, "Character.Action.Jump");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionJump_Forbid, "Character.Action.Jump.Forbid");

// Dash
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDash, "Character.Action.Dash");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDash_Forbid, "Character.Action.Dash.Forbid");

// Walk
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionWalk, "Character.Action.Walk");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionWalk_Forbid, "Character.Action.Walk.Forbid");

// Crouch
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionCrouch, "Character.Action.Crouch");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionCrouch_Forbid, "Character.Action.Crouch.Forbid");

// Drive
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDrive, "Character.Action.Drive");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_ActionDrive_Forbid, "Character.Action.Drive.Forbid");

// if reload active tag added character state tag
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Action_GunReload, "Character.Action.GunReload");

// TargetLock
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_TargetLock, "Character.Action.TargetLock");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_TargetLocking, "Character.Action.TargetLocking");
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_TargetLock_Forbid, "Character.Action.TargetLock.Forbid");

// cinematic
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_Action_Cinematic, "Character.Action.Cinematic");

// Climbing 
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ClimbingLedgeEnd, "Locomotion.Climbing.LedgeEnd");

// Mantling
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_Mantling, "Locomotion.Mantling");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_Vaulting, "Locomotion.Vaulting");

UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_Traversal, "Locomotion.Traversal");

// Forbid
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidClimbing, "Locomotion.Forbid.Climbing");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidMantling, "Locomotion.Forbid.Mantling");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidMovement, "Locomotion.Forbid.Movement");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidJump, "Locomotion.Forbid.Jump");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidRagdoll, "Locomotion.Forbid.Ragdoll");
UE_DEFINE_GAMEPLAY_TAG(TAG_Locomotion_ForbidVaulting, "Locomotion.Forbid.Vaulting");


// vehicle
UE_DEFINE_GAMEPLAY_TAG(TAG_Vehicle_Drive, "Vehicle.Drive");
UE_DEFINE_GAMEPLAY_TAG(TAG_Vehicle_UnDrive, "Vehicle.UnDrive");
UE_DEFINE_GAMEPLAY_TAG(TAG_Vehicle_State_Drive, "Vehicle.State.Drive");

// smart object difinition
UE_DEFINE_GAMEPLAY_TAG(TAG_Character_State_SmartObject_Using, "Character.State.SmartObject.Using");

// async load data asset tag
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Asset_AnimationBlueprint, "Game.Asset.AnimationBlueprint");
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Asset_HitReaction, "Game.Asset.HitReaction");
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Asset_FinisherSender, "Game.Asset.FinisherSender");
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Asset_FinisherReceiver, "Game.Asset.FinisherReceiver");
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Asset_CloseCombat, "Game.Asset.CloseCombat");
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Asset_CharacterVFX, "Game.Asset.CharacterVFX");

// minimap
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Minimap_Player, "Game.MiniMap.Player");
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Minimap_KeyCharacter, "Game.MiniMap.KeyCharacter");
UE_DEFINE_GAMEPLAY_TAG(TAG_Game_Minimap_EventCheckPoint, "Game.MiniMap.EventCheckPoint");


FRedemptionModule* FRedemptionModule::GameModuleInstance = nullptr;

#define LOCTEXT_NAMESPACE ""

void FRedemptionModule::StartupModule()
{
	GameModuleInstance = this;


#if WITH_EDITOR
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		SettingsModule->RegisterSettings(
			"Project",
			"Game",
			"WvSetting",
			LOCTEXT("MySettingName", "Wv Settings"),
			LOCTEXT("MySettingDescription", "Wv project settings."),
			GetMutableDefault<UWvProjectSetting>()
		);
	}
#endif // WITH_EDITOR
}

void FRedemptionModule::ShutdownModule()
{
	GameModuleInstance = nullptr;

#if WITH_EDITOR
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings(
			"Project",
			"Game",
			"WvSetting"
		);
	}
#endif // WITH_EDITOR
}


IMPLEMENT_PRIMARY_GAME_MODULE(FRedemptionModule, Redemption, "Redemption");

#undef LOCTEXT_NAMESPACE

