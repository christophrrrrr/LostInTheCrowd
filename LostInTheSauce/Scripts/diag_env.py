"""Diagnostic: what's actually in MarketMap, and does the town mesh exist?"""

import json
import os

import unreal

REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "env_diag.json")
TOWN_MESH = "/Game/LostInTheSauce/Scenes/modular_lowpoly_medieval_environment/MediEval_Scene"

eal = unreal.EditorAssetLibrary
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

level_editor.load_level("/Game/LostInTheSauce/Maps/MarketMap")

out = {"town_mesh_exists": eal.does_asset_exist(TOWN_MESH), "actors": []}
if out["town_mesh_exists"]:
    m = eal.load_asset(TOWN_MESH)
    box = m.get_bounding_box()
    out["town_bounds"] = [round((box.max.x - box.min.x), 1),
                          round((box.max.y - box.min.y), 1),
                          round((box.max.z - box.min.z), 1)]

for a in actor_sub.get_all_level_actors():
    label = a.get_actor_label()
    if label in ("TownEnv", "Floor", "NavBounds", "MarketPlayerStart") or label.startswith("MV_"):
        loc = a.get_actor_location()
        entry = {"label": label, "class": a.get_class().get_name(),
                 "loc": [round(loc.x, 1), round(loc.y, 1), round(loc.z, 1)]}
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        sm = smc.get_editor_property("static_mesh") if smc else None
        if sm:
            entry["mesh"] = sm.get_name()
        out["actors"].append(entry)

with open(REPORT, "w") as f:
    json.dump(out, f, indent=2)
