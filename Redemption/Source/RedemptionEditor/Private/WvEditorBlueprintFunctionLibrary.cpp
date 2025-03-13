// Copyright 2022 wevet works All Rights Reserved.


#include "WvEditorBlueprintFunctionLibrary.h"
#include "EditorReimportHandler.h"
#include "LevelEditorViewport.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryHelpers.h"

#include "Animation/Skeleton.h"
#include "Animation/BlendProfile.h"
#include "Engine/SkeletalMeshSocket.h"
//#include "Logging/LogMacros.h
#include "AssetRegistry/AssetRegistryModule.h"


void UWvEditorBlueprintFunctionLibrary::ReImportAnimation(const TArray<FName> PackagePaths)
{

	TArray<UAnimSequenceBase*> OutAnimations;
	UWvEditorBlueprintFunctionLibrary::LoadAssetsByAnimation(PackagePaths, OutAnimations);

	for (UAnimSequenceBase* Anim : OutAnimations)
	{
		UWvEditorBlueprintFunctionLibrary::Reimport(Anim);
	}
}

bool UWvEditorBlueprintFunctionLibrary::Reimport(UObject* InAsset)
{
	if (!InAsset)
	{
		return false;
	}
	return FReimportManager::Instance()->Reimport(InAsset, true);
}

void UWvEditorBlueprintFunctionLibrary::LoadAssetsByAnimation(const TArray<FName> PackagePaths, TArray<UAnimSequenceBase*>& OutAnimations)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry"));

	for (FName PackagePath : PackagePaths)
	{
		TArray<FAssetData> OutAssetList;
		AssetRegistryModule.Get().GetAssetsByPath(PackagePath, OutAssetList, true, true);

		for (FAssetData Asset : OutAssetList)
		{
			if (!Asset.GetAsset())
			{
				continue;
			}

			UAnimSequenceBase* AnimInstance = Cast<UAnimSequenceBase>(Asset.GetAsset());
			if (!AnimInstance)
			{
				continue;
			}
			UE_LOG(LogTemp, Log, TEXT("AnimAssetName => %s"), *AnimInstance->GetName());
			OutAnimations.Add(AnimInstance);
		}
	}

	OutAnimations.RemoveAll([](UAnimSequenceBase* AnimSequence)
		{
			return AnimSequence == nullptr;
		});
}



void UWvEditorBlueprintFunctionLibrary::CopyBlendProfiles(USkeleton* SourceSkeleton, TArray<USkeleton*> TargetSkeletons)
{
	if (!SourceSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("nullptr source skeleton"));
		return;
	}

	for (USkeleton* TargetSkeleton : TargetSkeletons)
	{
		if (!IsValid(TargetSkeleton))
		{
			continue;
		}

		// TargetSkeleton を編集可能な状態にする
		TargetSkeleton->Modify();
		TargetSkeleton->MarkPackageDirty();

		TMap<FName, TArray<const UBlendProfile*>> UniqueBlendProfiles;

		for (const TObjectPtr<UBlendProfile>& BlendProfile : SourceSkeleton->BlendProfiles)
		{
			UniqueBlendProfiles.FindOrAdd(BlendProfile->GetFName()).Add(BlendProfile.Get());
		}

		for (const TPair<FName, TArray<const UBlendProfile*>>& BlendProfilesPair : UniqueBlendProfiles)
		{
			const FName BlendProfileName = BlendProfilesPair.Key;
			const TArray<const UBlendProfile*>& BlendProfiles = BlendProfilesPair.Value;

			// 既存の BlendProfile を削除（あれば）
			if (UBlendProfile* ExistingProfile = TargetSkeleton->GetBlendProfile(BlendProfileName))
			{
				UE_LOG(LogTemp, Warning, TEXT("Existing Blend Profile %s found. Deleting..."), *BlendProfileName.ToString());
				TargetSkeleton->BlendProfiles.Remove(ExistingProfile);
			}

			UBlendProfile* MergedBlendProfile = TargetSkeleton->CreateNewBlendProfile(BlendProfilesPair.Key);

			UE_LOG(LogTemp, Log, TEXT("Copying Blend Profile: %s"), *BlendProfileName.ToString());

			const FReferenceSkeleton& TargetRefSkeleton = TargetSkeleton->GetReferenceSkeleton();

			for (int32 ProfileIndex = 0; ProfileIndex < BlendProfiles.Num(); ++ProfileIndex)
			{
				const UBlendProfile* Profile = BlendProfiles[ProfileIndex];
				MergedBlendProfile->Mode = ProfileIndex == 0 ? Profile->Mode : MergedBlendProfile->Mode;

				// Mismatch in terms of blend profile type
				ensure(MergedBlendProfile->Mode == Profile->Mode);

				for (const FBlendProfileBoneEntry& Entry : Profile->ProfileEntries)
				{
					const FName BoneName = Entry.BoneReference.BoneName;
					float SourceBlendScale = Entry.BlendScale;

					// Bone が TargetSkeleton に存在しない場合はスキップ
					if (TargetRefSkeleton.FindBoneIndex(BoneName) == INDEX_NONE)
					{
						UE_LOG(LogTemp, Warning, TEXT("Skipping Bone: %s, not found in TargetSkeleton"), *BoneName.ToString());
						continue;
					}

					// Overlapping bone entries
					ensure(!MergedBlendProfile->ProfileEntries.ContainsByPredicate([Entry](const FBlendProfileBoneEntry& InEntry)
						{
							return InEntry.BoneReference.BoneName == Entry.BoneReference.BoneName;
						}));

					MergedBlendProfile->SetBoneBlendScale(Entry.BoneReference.BoneName, Entry.BlendScale, false, true);

					const float TargetBlendScale = MergedBlendProfile->GetBoneBlendScale(BoneName);
					UE_LOG(LogTemp, Log, TEXT("Bone: %s, Source Blend Scale: %f, Target Blend Scale: %f"), *BoneName.ToString(), SourceBlendScale, TargetBlendScale);
				}
			}
		}

		// Skeleton を保存
		TargetSkeleton->MarkPackageDirty();
		TargetSkeleton->Modify();
	}

	UE_LOG(LogTemp, Log, TEXT("Blend Profile Copy Completed"));
}



void UWvEditorBlueprintFunctionLibrary::CopySkeletalSockets(USkeleton* SourceSkeleton, TArray<USkeleton*> TargetSkeletons)
{
	if (!SourceSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("nullptr source skeleton"));
		return;
	}


	for (USkeleton* TargetSkeleton : TargetSkeletons)
	{

		// TargetSkeleton を編集可能な状態にする
		TargetSkeleton->Modify();
		TargetSkeleton->MarkPackageDirty();

		// 既存のソケットをハッシュ化してマップを作成
		TSet<uint32> ExistingSocketHashes;
		for (const TObjectPtr<USkeletalMeshSocket>& ExistingSocket : TargetSkeleton->Sockets)
		{
			const uint32 Hash = HashCombine(GetTypeHash(ExistingSocket->SocketName), GetTypeHash(ExistingSocket->BoneName));
			ExistingSocketHashes.Add(Hash);
		}

		// コピー対象のソケットをハッシュ化してマップを作成
		TMap<uint32, TObjectPtr<USkeletalMeshSocket>> HashToSockets;
		for (const TObjectPtr<USkeletalMeshSocket>& Socket : SourceSkeleton->Sockets)
		{
			const uint32 Hash = HashCombine(GetTypeHash(Socket->SocketName), GetTypeHash(Socket->BoneName));
			HashToSockets.Add(Hash, Socket);
		}

		TArray<TObjectPtr<USkeletalMeshSocket>> Sockets;
		HashToSockets.GenerateValueArray(Sockets);

		int32 CopiedSockets = 0;
		int32 SkippedSockets = 0;


		for (const TObjectPtr<USkeletalMeshSocket>& MergeSocket : Sockets)
		{
			const uint32 Hash = HashCombine(GetTypeHash(MergeSocket->SocketName), GetTypeHash(MergeSocket->BoneName));

			// 既存のソケットがある場合はスキップ
			if (ExistingSocketHashes.Contains(Hash))
			{
				UE_LOG(LogTemp, Warning, TEXT("Skipping existing socket: %s (Bone: %s)"), *MergeSocket->SocketName.ToString(), *MergeSocket->BoneName.ToString());
				SkippedSockets++;

				continue;
			}

			USkeletalMeshSocket* NewSocket = NewObject<USkeletalMeshSocket>(TargetSkeleton);
			if (NewSocket != nullptr)
			{
				TargetSkeleton->Sockets.Add(NewSocket);

				// Copy over all socket information
				NewSocket->SocketName = MergeSocket->SocketName;
				NewSocket->BoneName = MergeSocket->BoneName;
				NewSocket->RelativeLocation = MergeSocket->RelativeLocation;
				NewSocket->RelativeRotation = MergeSocket->RelativeRotation;
				NewSocket->RelativeScale = MergeSocket->RelativeScale;
				NewSocket->bForceAlwaysAnimated = MergeSocket->bForceAlwaysAnimated;

				UE_LOG(LogTemp, Log, TEXT("Copied socket: %s (Bone: %s)"), *NewSocket->SocketName.ToString(), *NewSocket->BoneName.ToString());
				CopiedSockets++;
			}
		}

		// Skeleton を保存
		TargetSkeleton->MarkPackageDirty();
		TargetSkeleton->Modify();

		UE_LOG(LogTemp, Log, TEXT("Socket Copy Completed for TargetSkeleton: %s | Copied: %d | Skipped: %d"), *TargetSkeleton->GetName(), CopiedSockets, SkippedSockets);
	}

	UE_LOG(LogTemp, Log, TEXT("Socket Copy Completed"));
}


void UWvEditorBlueprintFunctionLibrary::CopySkeletalSlots(USkeleton* SourceSkeleton, TArray<USkeleton*> TargetSkeletons)
{
	if (!SourceSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("nullptr source skeleton"));
		return;
	}

	for (USkeleton* TargetSkeleton : TargetSkeletons)
	{
		if (!TargetSkeleton)
		{
			UE_LOG(LogTemp, Error, TEXT("nullptr target skeleton"));
			continue;
		}

		// TargetSkeleton を編集可能な状態にする
		TargetSkeleton->Modify();
		TargetSkeleton->MarkPackageDirty();

		// 既存のスロットを取得
		TMap<FName, TSet<FName>> ExistingSlots;
		for (const FAnimSlotGroup& SlotGroup : TargetSkeleton->GetSlotGroups())
		{
			ExistingSlots.FindOrAdd(SlotGroup.GroupName).Append(SlotGroup.SlotNames);
		}

		// ソーススケルトンのスロットを取得
		TMap<FName, TSet<FName>> GroupToSlotNames;
		const TArray<FAnimSlotGroup>& SlotGroups = SourceSkeleton->GetSlotGroups();
		for (const FAnimSlotGroup& AnimSlotGroup : SlotGroups)
		{
			GroupToSlotNames.FindOrAdd(AnimSlotGroup.GroupName).Append(AnimSlotGroup.SlotNames);
		}

		int32 CopiedSlots = 0;
		int32 SkippedSlots = 0;

		// スロットをコピー
		for (const TPair<FName, TSet<FName>>& SlotGroupNamePair : GroupToSlotNames)
		{
			const FName GroupName = SlotGroupNamePair.Key;
			const TSet<FName>& SlotNames = SlotGroupNamePair.Value;

			// グループを追加
			TargetSkeleton->AddSlotGroupName(GroupName);

			for (const FName& SlotName : SlotNames)
			{
				// 既存のスロットならスキップ
				if (ExistingSlots.Contains(GroupName) && ExistingSlots[GroupName].Contains(SlotName))
				{
					UE_LOG(LogTemp, Warning, TEXT("Skipping existing slot: %s (Group: %s)"), *SlotName.ToString(), *GroupName.ToString());
					SkippedSlots++;
					continue;
				}

				// 新規スロットを追加
				TargetSkeleton->SetSlotGroupName(SlotName, GroupName);
				UE_LOG(LogTemp, Log, TEXT("Copied slot: %s (Group: %s)"), *SlotName.ToString(), *GroupName.ToString());
				CopiedSlots++;
			}
		}

		// Skeleton を保存
		TargetSkeleton->MarkPackageDirty();
		TargetSkeleton->Modify();

		UE_LOG(LogTemp, Log, TEXT("Slot Copy Completed for TargetSkeleton: %s | Copied: %d | Skipped: %d"), *TargetSkeleton->GetName(), CopiedSlots, SkippedSlots);
	}

	UE_LOG(LogTemp, Log, TEXT("Slots Copy Completed"));
}



