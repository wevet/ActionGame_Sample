// Copyright 2020 wevet works All Rights Reserved.

#include "WvAbilityDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WvAbilityDataAsset)

UWvAbilityDataAsset::UWvAbilityDataAsset()
{
	//AbilityTypeTag = (FGameplayTagContainer)FGameplayTag::RequestGameplayTag(TAG_Common_Attack_Ability.GetTag().GetTagName());
	AbilityTypeTag.AddTag(TAG_Common_Attack_Ability);
}


#if WITH_EDITORONLY_DATA
void UWvAbilityDataAsset::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property && PropertyChangedEvent.PropertyChain.GetActiveMemberNode())
	{
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		const FName MemberPropertyName = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName();
		if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWvAbilityDataAsset, AbilityClass))
		{
		}
	}
}

void UWvAbilityDataAsset::PostLoad()
{
	Super::PostLoad();
}
#endif



