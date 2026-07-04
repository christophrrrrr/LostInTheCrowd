"""
Lost in the Sauce — import the FBX characters as skeletal meshes.

Uses the classic FBX import path (mature, handles armature scale properly —
unlike the Interchange glTF path that mangled the GLB versions). Each
character keeps its own skeleton and animation set.

Wipes and recreates /Game/LostInTheSauce/Characters, writes
import_report.json next to this script.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>
"""

import json
import os
import traceback

import unreal

ASSETS_DIR = r"C:\Users\HP\Desktop\unreal_engine\LostInTheSauce\assets"
DEST = "/Game/LostInTheSauce/Characters"
REPORT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "import_report.json")

# Adventurer goes first: its FBX carries the full animation set, and the
# anim-less meshes (Farmer/King/Witch) import onto its skeleton so they can
# play the same clips. The FBX importer validates rig compatibility, so a
# mismatch fails loudly instead of exploding silently like glTF did.
CHARACTERS = [
    ("Adventurer", "Adventurer.fbx", False),
    ("Farmer", "Farmer.fbx", True),
    ("King", "King.fbx", True),
    ("Witch", "Witch.fbx", True),
    ("Medieval", "Medieval.fbx", True),
]

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"imported": {}, "errors": []}

# Clean slate: drop every earlier import attempt.
if eal.does_directory_exist(DEST):
    eal.delete_directory(DEST)

donor_skeleton = None
for name, filename, use_donor_skeleton in CHARACTERS:
    source = os.path.join(ASSETS_DIR, filename)
    if not os.path.exists(source):
        report["errors"].append(f"{name}: missing file {source}")
        continue
    try:
        ui = unreal.FbxImportUI()
        ui.set_editor_property("import_mesh", True)
        ui.set_editor_property("import_as_skeletal", True)
        ui.set_editor_property("import_animations", True)
        ui.set_editor_property("import_materials", True)
        ui.set_editor_property("import_textures", True)
        ui.set_editor_property("automated_import_should_detect_type", False)
        ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        # The pack's animation tracks are authored 90 degrees rolled relative
        # to the axis-converted mesh; counter-rotate them at import so the
        # animated pose matches the upright reference pose.
        # unreal.Rotator argument order is (roll, pitch, yaw).
        ui.anim_sequence_import_data.set_editor_property(
            "import_rotation", unreal.Rotator(-90.0, 0.0, 0.0))
        if use_donor_skeleton and donor_skeleton:
            ui.set_editor_property("skeleton", donor_skeleton)

        task = unreal.AssetImportTask()
        task.filename = source
        task.destination_path = f"{DEST}/{name}"
        task.automated = True
        task.save = True
        task.replace_existing = True
        task.options = ui

        asset_tools.import_asset_tasks([task])
        imported = [str(p) for p in task.get_editor_property("imported_object_paths")]
        report["imported"][name] = imported
        if donor_skeleton is None:
            for path in imported:
                clean = path.split(".")[0]
                data = eal.find_asset_data(clean)
                if data.is_valid() and str(data.asset_class_path.asset_name) == "Skeleton":
                    donor_skeleton = eal.load_asset(clean)
                    break
    except Exception:
        report["errors"].append(f"{name}: import failed\n" + traceback.format_exc())

# --- Knight: Mixamo-rigged, separate anim files --------------------------------
# The Mixamo rig is self-consistent; import base mesh first, then each clip
# onto its skeleton, renamed to match the game's _Suffix lookup convention.
KNIGHT_DIR = f"{DEST}/Knight"
KNIGHT_ANIMS = [
    ("KnightIdle.fbx", "Knight_Idle_Neutral"),
    ("KnightWalking.fbx", "Knight_Walk"),
    ("KnightWaving.fbx", "Knight_Wave"),
    ("KinghtHitReaction.fbx", "Knight_HitRecieve"),  # (sic) user's filename
]

try:
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("import_materials", True)
    ui.set_editor_property("import_textures", True)
    ui.set_editor_property("automated_import_should_detect_type", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)

    task = unreal.AssetImportTask()
    task.filename = os.path.join(ASSETS_DIR, "Knight.fbx")
    task.destination_path = KNIGHT_DIR
    task.automated = True
    task.save = True
    task.replace_existing = True
    task.options = ui
    asset_tools.import_asset_tasks([task])
    knight_paths = [str(p) for p in task.get_editor_property("imported_object_paths")]
    report["imported"]["Knight"] = knight_paths

    knight_skeleton = None
    for path in knight_paths:
        clean = path.split(".")[0]
        data = eal.find_asset_data(clean)
        if data.is_valid() and str(data.asset_class_path.asset_name) == "Skeleton":
            knight_skeleton = eal.load_asset(clean)
            break

    if knight_skeleton:
        for filename, anim_name in KNIGHT_ANIMS:
            # Import each clip PAIRED with its mesh (the only pipeline whose
            # orientation handling is proven correct — anim-only imports
            # silently ignore rotation fixups). The duplicate geometry gets
            # deleted right after; only the AnimSequence survives.
            scratch_dir = f"{KNIGHT_DIR}/_AnimScratch"
            anim_ui = unreal.FbxImportUI()
            anim_ui.set_editor_property("import_mesh", True)
            anim_ui.set_editor_property("import_as_skeletal", True)
            anim_ui.set_editor_property("import_animations", True)
            anim_ui.set_editor_property("import_materials", False)
            anim_ui.set_editor_property("import_textures", False)
            anim_ui.set_editor_property("automated_import_should_detect_type", False)
            anim_ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
            anim_ui.set_editor_property("skeleton", knight_skeleton)

            anim_task = unreal.AssetImportTask()
            anim_task.filename = os.path.join(ASSETS_DIR, filename)
            anim_task.destination_path = scratch_dir
            anim_task.automated = True
            anim_task.save = True
            anim_task.replace_existing = True
            anim_task.options = anim_ui
            asset_tools.import_asset_tasks([anim_task])

            for path in [str(p) for p in anim_task.get_editor_property("imported_object_paths")]:
                clean = path.split(".")[0]
                data = eal.find_asset_data(clean)
                if data.is_valid() and str(data.asset_class_path.asset_name) == "AnimSequence":
                    eal.rename_asset(clean, f"{KNIGHT_DIR}/{anim_name}")
                    break

        # Drop the scratch meshes; the clips have been moved out already.
        if eal.does_directory_exist(f"{KNIGHT_DIR}/_AnimScratch"):
            eal.delete_directory(f"{KNIGHT_DIR}/_AnimScratch")
    else:
        report["errors"].append("Knight: no skeleton found after base import")
except Exception:
    report["errors"].append("Knight: import failed\n" + traceback.format_exc())

CHARACTERS.append(("Knight", "Knight.fbx", False))

# Inventory: what actually exists per character folder.
inventory = {}
for name, _, _ in CHARACTERS:
    folder = f"{DEST}/{name}"
    entries = []
    if eal.does_directory_exist(folder):
        for asset_path in eal.list_assets(folder, recursive=True):
            clean = asset_path.split(".")[0]
            data = eal.find_asset_data(clean)
            cls = str(data.asset_class_path.asset_name) if data.is_valid() else "?"
            entry = {"path": clean, "class": cls}
            if cls == "SkeletalMesh":
                mesh = eal.load_asset(clean)
                bounds = mesh.get_bounds()
                entry["size_xyz_cm"] = [round(bounds.box_extent.x * 2, 1),
                                        round(bounds.box_extent.y * 2, 1),
                                        round(bounds.box_extent.z * 2, 1)]
            entries.append(entry)
    inventory[name] = entries
report["inventory"] = inventory

eal.save_directory(DEST, recursive=True)

with open(REPORT_PATH, "w") as f:
    json.dump(report, f, indent=2)
