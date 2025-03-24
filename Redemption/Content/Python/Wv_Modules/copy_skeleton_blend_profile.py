# -*- coding: utf-8 -*-

import unreal


def copy_blend_profiles(source_skeleton_path, target_skeleton_path):
    """Blend Profile をコピーする関数"""

    # Skeleton アセットをロード
    source_skeleton = unreal.load_asset(source_skeleton_path)
    target_skeleton = unreal.load_asset(target_skeleton_path)

    # アセットが見つからない場合
    if not source_skeleton or not target_skeleton:
        unreal.log_error("エラー: Skeleton が正しく設定されていません。")
        return

    reference_skeleton = source_skeleton.get_reference_skeleton()
    bone_count = reference_skeleton.get_num_bones()

    blend_profile_names = source_skeleton.get_blend_profile_names()

    for profile_name in blend_profile_names:
        unreal.log(f"Copying Blend Profile: {profile_name}")

        # ソースの Blend Profile を取得
        source_profile = source_skeleton.get_blend_profile_by_name(profile_name)
        if not source_profile:
            unreal.log_warning(f"Warning: Blend Profile '{profile_name}' not found in source skeleton.")
            continue

        # ターゲットの Blend Profile を作成または取得
        target_profile = target_skeleton.find_blend_profile(profile_name)
        if not target_profile:
            target_profile = target_skeleton.create_blend_profile(profile_name)

        # 各ボーンのブレンド値をコピー
        for bone_index in range(bone_count):
            bone_name = reference_skeleton.get_bone_name(bone_index)

            # ターゲットスケルトンにこのボーンがあるかチェック
            if target_skeleton.get_reference_skeleton().find_bone_index(bone_name) == -1:
                unreal.log_warning(f"Skipping {bone_name} (not found in target skeleton)")
                continue

            # Time, Weight の取得と適用
            blend_value = source_profile.get_blend_value_for_bone(bone_name)
            blend_time = source_profile.get_blend_time_for_bone(bone_name)
            target_profile.set_blend_value_for_bone(bone_name, blend_value)
            target_profile.set_blend_time_for_bone(bone_name, blend_time)

    unreal.log("Blend Profile コピー完了！")


if __name__ == "__main__":
    #copy_blend_profiles("/Game/Characters/Hero/Hero_Skeleton", "/Game/Characters/Enemy/Enemy_Skeleton")
    pass
