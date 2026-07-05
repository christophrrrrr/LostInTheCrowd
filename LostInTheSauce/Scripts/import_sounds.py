"""
Lost in the Sauce — import the curated Kenney sounds.

Only the clips the game actually uses get imported, renamed to their game
role so the C++ paths stay stable if we swap the underlying clip later.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>
"""

import json
import os
import traceback

import unreal

SOUNDS_ROOT = r"C:\Users\HP\Desktop\unreal_engine\LostInTheSauce\assets\sounds"
DEST = "/Game/LostInTheSauce/Sounds"
REPORT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sound_report.json")

# (source file, game-role asset name)
SOUNDS = [
    (r"kenney_interface-sounds\Audio\click_002.ogg", "S_UIClick"),
    (r"kenney_interface-sounds\Audio\error_008.ogg", "S_WrongGuess"),
    (r"kenney_interface-sounds\Audio\confirmation_002.ogg", "S_Found"),
    (r"kenney_rpg-audio\Audio\handleCoins.ogg", "S_Reward"),
    (r"kenney_rpg-audio\Audio\cloth1.ogg", "S_Transition"),
    (r"kenney_rpg-audio\Audio\doorOpen_1.ogg", "S_GameStart"),
]

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"imported": [], "errors": []}

if eal.does_directory_exist(DEST):
    eal.delete_directory(DEST)
eal.make_directory(DEST)

for rel_path, role_name in SOUNDS:
    source = os.path.join(SOUNDS_ROOT, rel_path)
    if not os.path.exists(source):
        report["errors"].append(f"missing {source}")
        continue
    try:
        task = unreal.AssetImportTask()
        task.filename = source
        task.destination_path = DEST
        task.automated = True
        task.save = True
        task.replace_existing = True
        asset_tools.import_asset_tasks([task])
        imported = [str(p).split(".")[0] for p in task.get_editor_property("imported_object_paths")]
        if imported:
            target = f"{DEST}/{role_name}"
            if imported[0] != target:
                eal.rename_asset(imported[0], target)
            eal.save_asset(target)
            report["imported"].append(role_name)
        else:
            report["errors"].append(f"{role_name}: nothing imported (format unsupported?)")
    except Exception:
        report["errors"].append(f"{role_name}: failed\n" + traceback.format_exc())

with open(REPORT_PATH, "w") as f:
    json.dump(report, f, indent=2)
