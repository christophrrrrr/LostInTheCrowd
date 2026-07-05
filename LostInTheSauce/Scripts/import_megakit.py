"""
Lost in the Sauce — import the Medieval Village MegaKit (FBX + textures).

Imports every FBX as a static mesh with materials/textures into
/Game/LostInTheSauce/MegaKit for hand-placing in the editor. Writes
megakit_report.json next to this script.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>
"""

import json
import os
import traceback

import unreal

KIT_ROOT = r"C:\Users\HP\Desktop\unreal_engine\LostInTheSauce\assets\props"
DEST = "/Game/LostInTheSauce/MegaKit"
REPORT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "megakit_report.json")

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"imported": 0, "failed": [], "errors": []}

# Find the FBX folder (pack name contains brackets; walk instead of glob).
fbx_files = []
for root, _dirs, files in os.walk(KIT_ROOT):
    if "MegaKit" not in root:
        continue
    for f in files:
        if f.lower().endswith(".fbx"):
            fbx_files.append(os.path.join(root, f))
fbx_files.sort()

if not fbx_files:
    report["errors"].append("no FBX files found under " + KIT_ROOT)

for source in fbx_files:
    base = os.path.splitext(os.path.basename(source))[0].replace(" ", "_")
    try:
        ui = unreal.FbxImportUI()
        ui.set_editor_property("import_mesh", True)
        ui.set_editor_property("import_as_skeletal", False)
        ui.set_editor_property("import_animations", False)
        ui.set_editor_property("import_materials", True)
        ui.set_editor_property("import_textures", True)
        ui.set_editor_property("automated_import_should_detect_type", False)
        ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        ui.static_mesh_import_data.set_editor_property("combine_meshes", True)

        task = unreal.AssetImportTask()
        task.filename = source
        task.destination_path = DEST
        task.automated = True
        task.save = False
        task.replace_existing = True
        task.options = ui
        asset_tools.import_asset_tasks([task])

        if eal.does_asset_exist(f"{DEST}/{base}"):
            report["imported"] += 1
        else:
            report["failed"].append(base)
    except Exception:
        report["errors"].append(f"{base}\n" + traceback.format_exc())

eal.save_directory(DEST, recursive=True)

with open(REPORT_PATH, "w") as f:
    json.dump(report, f, indent=2)
