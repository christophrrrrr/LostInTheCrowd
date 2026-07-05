"""Diagnostic: what materials exist, and what do the market actors really use?"""

import json
import os

import unreal

REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "mat_report.json")

eal = unreal.EditorAssetLibrary
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

level_editor.load_level("/Game/LostInTheSauce/Maps/MarketMap")

report = {"materials": [], "actors": {}}
for p in eal.list_assets("/Game/LostInTheSauce/Materials", recursive=True):
    report["materials"].append(p.split(".")[0].split("/")[-1])

for actor in actor_sub.get_all_level_actors():
    label = actor.get_actor_label()
    if label in ("MV_Stall0_Canopy", "MV_House0_Roof", "MV_Tile_0_0", "MV_Stall0_Counter", "Floor"):
        comp = actor.static_mesh_component
        mats = [str(comp.get_material(s).get_name()) if comp.get_material(s) else "None"
                for s in range(comp.get_num_materials())]
        report["actors"][label] = mats

with open(REPORT, "w") as f:
    json.dump(report, f, indent=2)
