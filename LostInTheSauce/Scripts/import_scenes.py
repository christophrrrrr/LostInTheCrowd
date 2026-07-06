"""
Lost in the Sauce — import the two finished village scenes (single FBX each).

Imports into /Game/LostInTheSauce/Scenes/<name> with textures/materials and
complex-as-simple collision (needed for navmesh + clicks), then writes
scene_report.json with mesh bounds so we can scale/place them correctly.
"""

import json
import os
import traceback

import unreal

PROPS_ROOT = r"C:\Users\HP\Desktop\unreal_engine\LostInTheSauce\assets\props"
DEST_ROOT = "/Game/LostInTheSauce/Scenes"
REPORT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "scene_report.json")

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {"scenes": {}, "errors": []}

for folder in sorted(os.listdir(PROPS_ROOT)):
    folder_path = os.path.join(PROPS_ROOT, folder)
    if not os.path.isdir(folder_path):
        continue
    fbx_files = [f for f in os.listdir(folder_path) if f.lower().endswith(".fbx")]
    if not fbx_files:
        # textures may sit next to the fbx one level deeper
        for sub in os.listdir(folder_path):
            sub_path = os.path.join(folder_path, sub)
            if os.path.isdir(sub_path):
                fbx_files = [os.path.join(sub, f) for f in os.listdir(sub_path)
                             if f.lower().endswith(".fbx")]
                if fbx_files:
                    break
    if not fbx_files:
        report["errors"].append(f"{folder}: no fbx found")
        continue

    clean_name = folder.replace("-", "_").replace(" ", "_")
    dest = f"{DEST_ROOT}/{clean_name}"
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
        task.filename = os.path.join(folder_path, fbx_files[0])
        task.destination_path = dest
        task.automated = True
        task.save = True
        task.replace_existing = True
        task.options = ui
        asset_tools.import_asset_tasks([task])

        meshes = []
        for asset_path in eal.list_assets(dest, recursive=True):
            clean = asset_path.split(".")[0]
            data = eal.find_asset_data(clean)
            if data.is_valid() and str(data.asset_class_path.asset_name) == "StaticMesh":
                mesh = eal.load_asset(clean)
                body_setup = mesh.get_editor_property("body_setup")
                if body_setup:
                    body_setup.set_editor_property(
                        "collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
                eal.save_asset(clean)
                box = mesh.get_bounding_box()
                size = box.max - box.min
                meshes.append({
                    "path": clean,
                    "size": [round(size.x, 1), round(size.y, 1), round(size.z, 1)],
                    "min_z": round(box.min.z, 1),
                    "center": [round((box.max.x + box.min.x) / 2, 1),
                               round((box.max.y + box.min.y) / 2, 1)],
                })
        report["scenes"][clean_name] = meshes
    except Exception:
        report["errors"].append(f"{folder}\n" + traceback.format_exc())

with open(REPORT_PATH, "w") as f:
    json.dump(report, f, indent=2)
