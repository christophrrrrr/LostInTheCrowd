"""
Lost in the Sauce — market map SYSTEMS setup.

Manages only the technical actors and assets the game needs: lighting,
exposure, fog, gameplay materials, base floor, navmesh bounds, player start.

IT NEVER TOUCHES DECOR. The village itself (houses, stalls, props from the
MegaKit) is hand-placed in the editor and must survive every rerun of this
script. Do not add anything here that deletes or spawns decoration.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>
"""

import json
import os
import sys
import traceback

import unreal

MAP_DIR = "/Game/LostInTheSauce/Maps"
MAP_PATH = f"{MAP_DIR}/MarketMap"
MAT_DIR = "/Game/LostInTheSauce/Materials"

eal = unreal.EditorAssetLibrary
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

CUBE = eal.load_asset("/Engine/BasicShapes/Cube")

# Commandlets report "success" even when this script dies mid-way — keep our
# own progress file so failures are loud and located.
_REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "setup_report.json")
_progress = {"checkpoints": [], "status": "STARTED"}


def checkpoint(name):
    _progress["checkpoints"].append(name)
    with open(_REPORT, "w") as f:
        json.dump(_progress, f, indent=2)


def _hook(etype, value, tb):
    _progress["status"] = "FAILED"
    _progress["traceback"] = "".join(traceback.format_exception(etype, value, tb))
    with open(_REPORT, "w") as f:
        json.dump(_progress, f, indent=2)
    sys.__excepthook__(etype, value, tb)


sys.excepthook = _hook
checkpoint("start")


def log(msg):
    unreal.log(f"[LostInTheSauce map] {msg}")


# --- Open or create the map ---------------------------------------------------
if not eal.does_directory_exist(MAP_DIR):
    eal.make_directory(MAP_DIR)

if eal.does_asset_exist(MAP_PATH):
    level_editor.load_level(MAP_PATH)
else:
    level_editor.new_level(MAP_PATH)
    log("Created new MarketMap")


def find_by_label(label):
    for a in actor_sub.get_all_level_actors():
        if a.get_actor_label() == label:
            return a
    return None


def spawn(cls, label, loc, rot=(0.0, 0.0, 0.0)):
    """rot is (roll, pitch, yaw) — unreal.Rotator argument order."""
    actor = actor_sub.spawn_actor_from_class(
        cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    actor.set_actor_label(label)
    return actor


# --- Lighting (find-or-create, then refresh properties) -------------------------
sun = find_by_label("Sun") or spawn(unreal.DirectionalLight, "Sun", (0, 0, 2000))
sun.set_actor_rotation(unreal.Rotator(0.0, -50.0, 35.0), False)
sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_comp.set_editor_property("atmosphere_sun_light", True)
sun_comp.set_editor_property("intensity", 50000.0)
# NOTE: unreal.Color positional args are B,G,R,A — use named args.
sun_comp.set_editor_property("light_color", unreal.Color(r=255, g=240, b=214, a=255))

if not find_by_label("Sky"):
    spawn(unreal.SkyAtmosphere, "Sky", (0, 0, 0))

fog = find_by_label("Fog") or spawn(unreal.ExponentialHeightFog, "Fog", (0, 0, 0))
fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
fog_comp.set_editor_property("fog_density", 0.003)
fog_comp.set_editor_property("start_distance", 3000.0)

skylight = find_by_label("SkyLight") or spawn(unreal.SkyLight, "SkyLight", (0, 0, 500))
skylight_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
skylight_comp.set_editor_property("real_time_capture", True)
skylight_comp.set_editor_property("intensity", 2.0)

ppv = find_by_label("ExposureVolume") or spawn(unreal.PostProcessVolume, "ExposureVolume", (0, 0, 0))
ppv.set_editor_property("unbound", True)
pp_settings = ppv.get_editor_property("settings")
pp_settings.set_editor_property("override_auto_exposure_min_brightness", True)
pp_settings.set_editor_property("override_auto_exposure_max_brightness", True)
pp_settings.set_editor_property("auto_exposure_min_brightness", 11.8)
pp_settings.set_editor_property("auto_exposure_max_brightness", 12.8)
pp_settings.set_editor_property("override_motion_blur_amount", True)
pp_settings.set_editor_property("motion_blur_amount", 0.0)
pp_settings.set_editor_property("override_color_saturation", True)
pp_settings.set_editor_property("color_saturation", unreal.Vector4(1.18, 1.18, 1.18, 1.0))
pp_settings.set_editor_property("override_color_contrast", True)
pp_settings.set_editor_property("color_contrast", unreal.Vector4(1.04, 1.04, 1.04, 1.0))
ppv.set_editor_property("settings", pp_settings)
checkpoint("lighting")

# --- Gameplay materials (create-if-missing; NEVER delete or recompile) ----------
if not eal.does_directory_exist(MAT_DIR):
    eal.make_directory(MAT_DIR)

if eal.does_asset_exist(f"{MAT_DIR}/M_NPCColor"):
    npc_mat = eal.load_asset(f"{MAT_DIR}/M_NPCColor")
else:
    npc_mat = asset_tools.create_asset("M_NPCColor", MAT_DIR, unreal.Material,
                                       unreal.MaterialFactoryNew())
    npc_mat.set_editor_property("two_sided", True)
    # Without this usage flag, -game builds silently swap in the default gray
    # material on all skeletal meshes (editor auto-adds it; game won't).
    npc_mat.set_editor_property("used_with_skeletal_mesh", True)
    color = mel.create_material_expression(npc_mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(npc_mat, unreal.MaterialExpressionConstant, -400, 300)
    rough.set_editor_property("r", 0.85)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(npc_mat)
    eal.save_asset(f"{MAT_DIR}/M_NPCColor")
    log("Created M_NPCColor (two-sided, skeletal usage)")

if not eal.does_asset_exist(f"{MAT_DIR}/M_Highlight"):
    hl_mat = asset_tools.create_asset("M_Highlight", MAT_DIR, unreal.Material,
                                      unreal.MaterialFactoryNew())
    hl_mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    hl_mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    fresnel = mel.create_material_expression(hl_mat, unreal.MaterialExpressionFresnel, -600, 0)
    fresnel.set_editor_property("exponent", 3.0)
    tint_p = mel.create_material_expression(hl_mat, unreal.MaterialExpressionVectorParameter, -600, 250)
    tint_p.set_editor_property("parameter_name", "Color")
    tint_p.set_editor_property("default_value", unreal.LinearColor(1.0, 0.85, 0.2, 1.0))
    mul = mel.create_material_expression(hl_mat, unreal.MaterialExpressionMultiply, -350, 100)
    mel.connect_material_expressions(fresnel, "", mul, "A")
    mel.connect_material_expressions(tint_p, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(hl_mat)
    eal.save_asset(f"{MAT_DIR}/M_Highlight")
    log("Created M_Highlight")
checkpoint("materials")

# --- Base floor + navigation + player start (find-or-create only) ----------------
# The Town environment mesh (place_environment.py) is the real ground once
# present; only fall back to a graybox floor when there is no town.
if not find_by_label("Floor") and not find_by_label("TownEnv"):
    floor = spawn(unreal.StaticMeshActor, "Floor", (0, 0, -25))
    comp = floor.static_mesh_component
    comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    comp.set_static_mesh(CUBE)
    comp.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    floor.set_actor_scale3d(unreal.Vector(46, 46, 0.5))

for actor in list(actor_sub.get_all_level_actors()):
    if isinstance(actor, unreal.RecastNavMesh):
        log(f"Deleting stale navmesh actor {actor.get_actor_label()}")
        actor_sub.destroy_actor(actor)

if not find_by_label("NavBounds"):
    nav = spawn(unreal.NavMeshBoundsVolume, "NavBounds", (0, 0, 200))
    nav.set_actor_scale3d(unreal.Vector(24, 24, 5))

if not find_by_label("MarketPlayerStart"):
    spawn(unreal.PlayerStart, "MarketPlayerStart", (0, 0, 300))

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
level_editor.save_current_level()
_progress["status"] = "OK"
checkpoint("saved")
log("Done! Systems refreshed; hand-placed decor untouched.")
