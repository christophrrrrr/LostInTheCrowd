"""
Lost in the Sauce — import music, crowd ambience, and footstep sounds.

  S_Music     <- assets/sounds/tavernmusic.mp3   (looping)
  S_Crowd     <- assets/sounds/crowdnoise.mp3    (looping)
  S_Footstep  <- kenney rpg footstep OGG         (one-shot)

Writes audio_report.json with what actually imported (MP3 support in UE is
not guaranteed — the report tells us whether the music/crowd came through).
"""

import json
import os

import unreal

SOUNDS = r"C:\Users\HP\Desktop\unreal_engine\LostInTheSauce\assets\sounds"
DEST = "/Game/LostInTheSauce/Sounds"
REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "audio_report.json")

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"imported": [], "missing_source": [], "import_failed": []}

# (source path, asset name, looping)
CLIPS = [
    (os.path.join(SOUNDS, "tavernmusic.mp3"), "S_Music", True),
    (os.path.join(SOUNDS, "crowdnoise.mp3"), "S_Crowd", True),
    (os.path.join(SOUNDS, r"kenney_rpg-audio\Audio\footstep00.ogg"), "S_Footstep", False),
]

for source, name, looping in CLIPS:
    if not os.path.exists(source):
        report["missing_source"].append(source)
        continue
    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = DEST
    task.automated = True
    task.save = True
    task.replace_existing = True
    asset_tools.import_asset_tasks([task])

    imported = [str(p).split(".")[0] for p in task.get_editor_property("imported_object_paths")]
    if not imported:
        report["import_failed"].append(os.path.basename(source))
        continue

    clean = imported[0]
    target = f"{DEST}/{name}"
    if clean != target:
        eal.rename_asset(clean, target)
    wave = eal.load_asset(target)
    if wave and looping:
        try:
            wave.set_editor_property("looping", True)
        except Exception:
            pass
    eal.save_asset(target)
    report["imported"].append(name)

with open(REPORT, "w") as f:
    json.dump(report, f, indent=2)
