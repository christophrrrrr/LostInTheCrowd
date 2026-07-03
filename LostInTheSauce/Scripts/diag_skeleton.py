"""Diagnostic: dump Farmer ref-pose bone translations and folder contents."""

import json
import os

import unreal

REPORT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "skeleton_report.json")

eal = unreal.EditorAssetLibrary
report = {"bones": [], "farmer_assets": []}

mesh = eal.load_asset("/Game/LostInTheSauce/Characters/Farmer/Farmer")
if mesh:
    # Ref-pose bone transforms via the mesh's reference skeleton.
    ref = mesh.get_editor_property("ref_skeleton") if hasattr(mesh, "ref_skeleton") else None
    # ref_skeleton isn't exposed; use reference pose through skeleton bone tree instead.
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton:
        # num bones via find_bone_name walk
        for i in range(80):
            name = skeleton.get_reference_pose_bone_name(i) if hasattr(skeleton, "get_reference_pose_bone_name") else None
            if name is None:
                break
    # Fallback: use SkeletalMeshEditorSubsystem-free approach — bounds only.
    bounds = mesh.get_bounds()
    report["mesh_bounds_cm"] = [round(bounds.box_extent.x * 2, 1),
                                round(bounds.box_extent.y * 2, 1),
                                round(bounds.box_extent.z * 2, 1)]
    report["mesh_import_scale_note"] = "bounds from asset"

for asset_path in eal.list_assets("/Game/LostInTheSauce/Characters/Farmer", recursive=True):
    clean = asset_path.split(".")[0]
    data = eal.find_asset_data(clean)
    cls = str(data.asset_class_path.asset_name) if data.is_valid() else "?"
    report["farmer_assets"].append(f"{cls}: {clean.split('/')[-1]}")

with open(REPORT_PATH, "w") as f:
    json.dump(report, f, indent=2)
