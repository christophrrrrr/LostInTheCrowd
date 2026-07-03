"""
Lost in the Sauce — import the GLB characters as skeletal meshes.

Each character keeps its OWN skeleton and its OWN animation set (forcing a
shared skeleton explodes the skinning — learned the hard way). Knight.glb is
excluded: it uses an incompatible 31-bone rig from a different pack and has
no animations.

Wipes and recreates /Game/LostInTheSauce/Characters for a clean slate, and
writes import_report.json next to this script.

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

CHARACTERS = [
    ("Farmer", "Farmer.glb"),
    ("King", "King.glb"),
    ("Witch", "Witch.glb"),
    ("Adventurer", "Hooded Adventurer.glb"),
]

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"imported": {}, "errors": []}

# Clean slate so renames/leftovers from earlier import attempts can't linger.
if eal.does_directory_exist(DEST):
    eal.delete_directory(DEST)

for name, filename in CHARACTERS:
    try:
        task = unreal.AssetImportTask()
        task.filename = os.path.join(ASSETS_DIR, filename)
        task.destination_path = f"{DEST}/{name}"
        task.automated = True
        task.save = True
        task.replace_existing = True

        # The code-constructed generic pipeline combines the modular body
        # parts into ONE skeletal mesh per character (the default task
        # pipeline splits them into Body/Feet/Head/Legs). Each character
        # keeps its own skeleton — never force a foreign one.
        pipeline = unreal.InterchangeGenericAssetsPipeline()
        pipeline.get_editor_property("animation_pipeline").set_editor_property(
            "import_animations", True)
        # The code-constructed pipeline skips materials unless told otherwise.
        try:
            mat_pipeline = pipeline.get_editor_property("material_pipeline")
            mat_pipeline.set_editor_property("import_materials", True)
            mat_pipeline.get_editor_property("texture_pipeline").set_editor_property(
                "import_textures", True)
        except Exception:
            report["errors"].append(f"{name}: material pipeline options\n" + traceback.format_exc())
        override = unreal.InterchangePipelineStackOverride()
        override.add_pipeline(pipeline)
        task.options = override

        asset_tools.import_asset_tasks([task])
        report["imported"][name] = [str(p) for p in task.get_editor_property("imported_object_paths")]
    except Exception:
        report["errors"].append(f"{name}: import failed\n" + traceback.format_exc())

# The combining pipeline imports the materials but forgets to assign them to
# the mesh slots. Slot names match the material asset names, so re-link them.
def assign_materials(name):
    folder = f"{DEST}/{name}"
    materials = {}
    meshes = []
    for asset_path in eal.list_assets(folder, recursive=True):
        clean = asset_path.split(".")[0]
        data = eal.find_asset_data(clean)
        cls = str(data.asset_class_path.asset_name) if data.is_valid() else "?"
        if cls in ("Material", "MaterialInstanceConstant"):
            materials[clean.split("/")[-1]] = eal.load_asset(clean)
        elif cls == "SkeletalMesh":
            meshes.append(clean)
    for mesh_path in meshes:
        mesh = eal.load_asset(mesh_path)
        slots = mesh.get_editor_property("materials")
        new_slots = []
        changed = False
        for slot in slots:
            slot_name = str(slot.get_editor_property("material_slot_name"))
            if slot_name in materials:
                slot.set_editor_property("material_interface", materials[slot_name])
                changed = True
            new_slots.append(slot)
        if changed:
            mesh.set_editor_property("materials", new_slots)
            unreal.EditorAssetLibrary.save_asset(mesh_path)


for name, _ in CHARACTERS:
    try:
        assign_materials(name)
    except Exception:
        report["errors"].append(f"{name}: material assignment failed\n" + traceback.format_exc())

# Inventory: skeletal meshes and anim sequences per character folder.
inventory = {}
for name, _ in CHARACTERS:
    folder = f"{DEST}/{name}"
    entries = []
    if eal.does_directory_exist(folder):
        for asset_path in eal.list_assets(folder, recursive=True):
            clean = asset_path.split(".")[0]
            data = eal.find_asset_data(clean)
            cls = str(data.asset_class_path.asset_name) if data.is_valid() else "?"
            if cls in ("SkeletalMesh", "AnimSequence", "Skeleton"):
                entries.append({"path": clean, "class": cls})
    inventory[name] = entries
report["inventory"] = inventory

eal.save_directory(DEST, recursive=True)

with open(REPORT_PATH, "w") as f:
    json.dump(report, f, indent=2)
