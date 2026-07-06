"""
Lost in the Sauce — import the character portrait PNGs.

assets/portraits/P_<Type>.png -> /Game/LostInTheSauce/Portraits/P_<Type>
(The PNGs are cropped from real in-game close-up shots by the automated
portrait factory; re-run the factory + this script to refresh them.)
"""

import os

import unreal

SRC = r"C:\Users\HP\Desktop\unreal_engine\LostInTheSauce\assets\portraits"
DEST = "/Game/LostInTheSauce/Portraits"

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

for filename in sorted(os.listdir(SRC)):
    if not filename.startswith("P_") or not filename.lower().endswith(".png"):
        continue
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, filename)
    task.destination_path = DEST
    task.automated = True
    task.save = True
    task.replace_existing = True
    asset_tools.import_asset_tasks([task])
    unreal.log(f"[portraits] imported {filename}")
