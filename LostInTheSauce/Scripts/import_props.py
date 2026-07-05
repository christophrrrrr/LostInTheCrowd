"""
Lost in the Sauce — import the Modular Village OBJ props as static meshes.

Imports every .obj into /Game/LostInTheSauce/Props, adds simple box
collision (OBJ imports ship without any), and writes prop_report.json with
per-mesh bounds so the market layout script can place things accurately.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>
"""

import json
import os
import traceback

import unreal

PROPS_DIR = r"C:\Users\HP\Desktop\unreal_engine\LostInTheSauce\assets\props\Modular Village"
DEST = "/Game/LostInTheSauce/Props"
REPORT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "prop_report.json")

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"errors": [], "retried_ok": [], "still_missing": [], "meshes": {}}

if eal.does_directory_exist(DEST):
    eal.delete_directory(DEST)

obj_files = sorted(f for f in os.listdir(PROPS_DIR) if f.lower().endswith(".obj"))


def make_task(filename):
    task = unreal.AssetImportTask()
    task.filename = os.path.join(PROPS_DIR, filename)
    task.destination_path = DEST
    task.automated = True
    task.save = False  # save the whole directory once at the end
    task.replace_existing = True
    return task


try:
    asset_tools.import_asset_tasks([make_task(f) for f in obj_files])
except Exception:
    report["errors"].append("batch import failed\n" + traceback.format_exc())

# ~57 files carry generic internal names (object1, object2, ...) so batch
# imports overwrite each other. Purge those survivors, then re-import each
# affected file alone into a scratch folder and rename to the filename.
for asset_path in list(eal.list_assets(DEST, recursive=True)):
    clean = asset_path.split(".")[0]
    if clean.split("/")[-1].startswith("object"):
        eal.delete_asset(clean)

SCRATCH = f"{DEST}/_Scratch"
for filename in obj_files:
    base = os.path.splitext(filename)[0].replace(" ", "_")
    if eal.does_asset_exist(f"{DEST}/{base}"):
        continue
    try:
        if eal.does_directory_exist(SCRATCH):
            eal.delete_directory(SCRATCH)
        task = make_task(filename)
        task.destination_path = SCRATCH
        asset_tools.import_asset_tasks([task])
        renamed = False
        for scratch_path in eal.list_assets(SCRATCH, recursive=True):
            clean = scratch_path.split(".")[0]
            data = eal.find_asset_data(clean)
            if data.is_valid() and str(data.asset_class_path.asset_name) == "StaticMesh":
                eal.rename_asset(clean, f"{DEST}/{base}")
                renamed = True
                break
        if renamed:
            report["retried_ok"].append(base)
        else:
            report["still_missing"].append(base)
    except Exception:
        report["still_missing"].append(base)
if eal.does_directory_exist(SCRATCH):
    eal.delete_directory(SCRATCH)

# Collision (complex-as-simple: fine for static props, and the navmesh
# carves around them) + bounds report.
for asset_path in eal.list_assets(DEST, recursive=True):
    clean = asset_path.split(".")[0]
    data = eal.find_asset_data(clean)
    if not data.is_valid() or str(data.asset_class_path.asset_name) != "StaticMesh":
        continue
    mesh = eal.load_asset(clean)
    try:
        body_setup = mesh.get_editor_property("body_setup")
        if body_setup:
            body_setup.set_editor_property(
                "collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    except Exception:
        report["errors"].append(f"{clean}: collision failed\n" + traceback.format_exc())
    bounds = mesh.get_bounding_box()
    size = bounds.max - bounds.min
    report["meshes"][clean.split("/")[-1]] = {
        "size": [round(size.x, 2), round(size.y, 2), round(size.z, 2)],
        "min_z": round(bounds.min.z, 2),
    }

eal.save_directory(DEST, recursive=True)

with open(REPORT_PATH, "w") as f:
    json.dump(report, f, indent=2)
