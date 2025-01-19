// Copyright 2022 wevet works All Rights Reserved.


#include "Character/EnemyCharacter.h"
#include "GameExtension.h"
#include "Component/InventoryComponent.h"
#include "Component/WvSkeletalMeshComponent.h"


AEnemyCharacter::AEnemyCharacter(const FObjectInitializer& ObjectInitializer)
{

	// dont async load components
	bIsAllowAsyncLoadComponentAssets = true;
	bIsDiedRemoveInventory = true;

	UWvSkeletalMeshComponent* WvMeshComp = CastChecked<UWvSkeletalMeshComponent>(GetMesh());
	check(WvMeshComp);

	// sets Damage
	//WvMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Block);


}

void AEnemyCharacter::MoveBlockedBy(const FHitResult& Impact)
{
}

#pragma region IWvAbilityTargetInterface
void AEnemyCharacter::OnReceiveKillTarget(AActor* Actor, const float Damage)
{
	Super::OnReceiveKillTarget(Actor, Damage);
}

void AEnemyCharacter::Freeze()
{
	Super::Freeze();
}

void AEnemyCharacter::UnFreeze()
{
	Super::UnFreeze();
}

void AEnemyCharacter::OnReceiveHitReact(FGameplayEffectContextHandle Context, const bool IsInDead, const float Damage)
{
	Super::OnReceiveHitReact(Context, IsInDead, Damage);
}
#pragma endregion

void AEnemyCharacter::SkillAction()
{
	if (bCanSkillUse)
	{
		Super::SkillAction();
	}
}

void AEnemyCharacter::RequestComponentsAsyncLoad()
{
	/*
	* InventoryComponent
	*/

	auto Inventory = GetInventoryComponent();
	if (IsValid(Inventory))
	{
		Inventory->RequestAsyncLoad();
	}

}

