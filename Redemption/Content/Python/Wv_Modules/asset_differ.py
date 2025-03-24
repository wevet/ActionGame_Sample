# -*- coding: utf-8 -*-

import unreal


def get_assets_in_directory(directory):
    """
    指定したディレクトリ配下のアセットを取得
    """
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = asset_registry.get_assets_by_path(directory, recursive=True)
    asset_names = {asset.asset_name for asset in assets}
    return asset_names


# 比較するディレクトリ（UEのパス）
directory_a = "/Game/Characters/MetaHuman/MM/Walk"
directory_b = "/Game/Characters/UEFN_Mannequin/Animations/Walk"

# アセット一覧を取得
assets_a = get_assets_in_directory(directory_a)
assets_b = get_assets_in_directory(directory_b)

# 片方にしかないアセットを抽出
only_in_a = assets_a - assets_b  # Aにしかない
only_in_b = assets_b - assets_a  # Bにしかない

# 警告ログを出力
for asset in only_in_a:
    unreal.log_warning(f"Asset {asset} exists in {directory_a} but not in {directory_b}")

for asset in only_in_b:
    unreal.log_warning(f"Asset {asset} exists in {directory_b} but not in {directory_a}")

unreal.log("Asset comparison completed.")


