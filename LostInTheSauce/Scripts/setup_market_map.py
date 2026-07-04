"""
Lost in the Sauce — one-time market map setup.

Creates /Game/LostInTheSauce/Maps/MarketMap: graybox floor, market stalls,
crates, perimeter walls, lighting, a NavMeshBoundsVolume for NPC wandering,
and a PlayerStart. Everything else (crowd, camera, rules) is C++.

How to run (inside the Unreal editor):
  Tools -> Execute Python Script... -> pick this file

Safe to re-run: it reopens the map if it already exists and only adds
actors that are missing (checked by label).
"""

import math
import unreal

MAP_DIR = "/Game/LostInTheSauce/Maps"
MAP_PATH = f"{MAP_DIR}/MarketMap"

eal = unreal.EditorAssetLibrary
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

CUBE = eal.load_asset("/Engine/BasicShapes/Cube")


def log(msg):
    unreal.log(f"[LostInTheSauce map] {msg}")


def existing_labels():
    return {a.get_actor_label() for a in actor_sub.get_all_level_actors()}


def spawn(cls, label, loc, rot=(0.0, 0.0, 0.0)):
    """rot is (roll, pitch, yaw) — unreal.Rotator argument order."""
    actor = actor_sub.spawn_actor_from_class(
        cls, unreal.Vector(*loc), unreal.Rotator(*rot))
    actor.set_actor_label(label)
    return actor


def spawn_cube(label, loc, scale, yaw=0.0):
    actor = spawn(unreal.StaticMeshActor, label, loc, (0.0, 0.0, yaw))
    comp = actor.static_mesh_component
    # Editing a Static-mobility mesh needs a brief hop through Movable.
    comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    comp.set_static_mesh(CUBE)
    comp.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


# --- Open or create the map ------------------------------------------------
if not eal.does_directory_exist(MAP_DIR):
    eal.make_directory(MAP_DIR)

if eal.does_asset_exist(MAP_PATH):
    level_editor.load_level(MAP_PATH)
    log("MarketMap already exists - reopened it")
else:
    level_editor.new_level(MAP_PATH)
    log("Created new MarketMap")

labels = existing_labels()

# --- Lighting (find-or-create, then always refresh properties) --------------
def find_by_label(label):
    for a in actor_sub.get_all_level_actors():
        if a.get_actor_label() == label:
            return a
    return None


sun = find_by_label("Sun") or spawn(unreal.DirectionalLight, "Sun", (0, 0, 2000))
sun.set_actor_rotation(unreal.Rotator(0.0, -50.0, 35.0), False)
sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_comp.set_editor_property("atmosphere_sun_light", True)
# Physical daylight (lux) so the EV100 exposure clamp below lands correctly.
sun_comp.set_editor_property("intensity", 50000.0)
# Slight golden-hour warmth. NOTE: unreal.Color positional args are B,G,R,A —
# use named args or you get a blue sun.
sun_comp.set_editor_property("light_color", unreal.Color(r=255, g=240, b=214, a=255))

if not find_by_label("Sky"):
    spawn(unreal.SkyAtmosphere, "Sky", (0, 0, 0))

fog = find_by_label("Fog") or spawn(unreal.ExponentialHeightFog, "Fog", (0, 0, 0))
fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
fog_comp.set_editor_property("fog_density", 0.006)
fog_comp.set_editor_property("start_distance", 2000.0)

skylight = find_by_label("SkyLight") or spawn(unreal.SkyLight, "SkyLight", (0, 0, 500))
skylight_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
# Real-time capture back ON: the one-off capture ran before the atmosphere
# was ready and baked a near-black ambient, flattening the whole scene.
# Cost on this scene is negligible.
skylight_comp.set_editor_property("real_time_capture", True)

# Clamp auto-exposure to daylight values so the white graybox doesn't blow out.
ppv = find_by_label("ExposureVolume") or spawn(unreal.PostProcessVolume, "ExposureVolume", (0, 0, 0))
ppv.set_editor_property("unbound", True)
pp_settings = ppv.get_editor_property("settings")
pp_settings.set_editor_property("override_auto_exposure_min_brightness", True)
pp_settings.set_editor_property("override_auto_exposure_max_brightness", True)
pp_settings.set_editor_property("auto_exposure_min_brightness", 11.8)
pp_settings.set_editor_property("auto_exposure_max_brightness", 12.8)
# Motion blur turns a scanning game into soup; off.
pp_settings.set_editor_property("override_motion_blur_amount", True)
pp_settings.set_editor_property("motion_blur_amount", 0.0)
ppv.set_editor_property("settings", pp_settings)

# A stale RecastNavMesh actor saved in the map overrides the Dynamic runtime
# generation configured in DefaultEngine.ini. Delete it; the game auto-creates
# a fresh one with correct (Dynamic) class defaults at startup.
for actor in list(actor_sub.get_all_level_actors()):
    if isinstance(actor, unreal.RecastNavMesh):
        log(f"Deleting stale navmesh actor {actor.get_actor_label()}")
        actor_sub.destroy_actor(actor)

# --- Ground: 46m x 46m square ------------------------------------------------
if "Floor" not in labels:
    spawn_cube("Floor", (0, 0, -25), (46, 46, 0.5))

# --- Perimeter walls so the market feels contained ---------------------------
if "Wall_N" not in labels:
    spawn_cube("Wall_N", (0, 2300, 150), (46, 0.3, 3))
    spawn_cube("Wall_S", (0, -2300, 150), (46, 0.3, 3))
    spawn_cube("Wall_E", (2300, 0, 150), (0.3, 46, 3))
    spawn_cube("Wall_W", (-2300, 0, 150), (0.3, 46, 3))

# --- Market stalls: a ring of 8 around an open center ------------------------
if "Stall_0" not in labels:
    for i in range(8):
        angle = math.radians(i * 45.0)
        x = math.cos(angle) * 1250.0
        y = math.sin(angle) * 1250.0
        spawn_cube(f"Stall_{i}", (x, y, 110), (3.2, 2.2, 2.2), yaw=i * 45.0)

# --- Crates and barrels for visual clutter and occlusion ---------------------
CRATES = [
    (620, 380), (-540, 460), (300, -700), (-820, -260),
    (1650, 900), (-1500, 1100), (900, 1650), (-1100, -1500),
    (1800, -400), (-400, 1800), (1400, -1400), (-1750, 300),
]
if "Crate_0" not in labels:
    for i, (x, y) in enumerate(CRATES):
        size = 0.55 + (i % 3) * 0.15
        spawn_cube(f"Crate_{i}", (x, y, size * 50), (size, size, size), yaw=(i * 37) % 90)

# --- Navigation bounds for the wander AI -------------------------------------
if "NavBounds" not in labels:
    nav = spawn(unreal.NavMeshBoundsVolume, "NavBounds", (0, 0, 200))
    nav.set_actor_scale3d(unreal.Vector(24, 24, 5))
    log("Spawned NavMeshBoundsVolume")

# --- Player start (orbit camera spawns here, at market center) ---------------
if "MarketPlayerStart" not in labels:
    spawn(unreal.PlayerStart, "MarketPlayerStart", (0, 0, 300))

# --- Gameplay materials (idempotent) ------------------------------------------
MAT_DIR = "/Game/LostInTheSauce/Materials"
if not eal.does_directory_exist(MAT_DIR):
    eal.make_directory(MAT_DIR)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

# M_NPCColor: one flat-color parameterized material that C++ instances per
# NPC material slot — full control over crowd colors for the difficulty tint.
if not eal.does_asset_exist(f"{MAT_DIR}/M_NPCColor"):
    npc_mat = asset_tools.create_asset("M_NPCColor", MAT_DIR, unreal.Material,
                                       unreal.MaterialFactoryNew())
    color = mel.create_material_expression(npc_mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(npc_mat, unreal.MaterialExpressionConstant, -400, 300)
    rough.set_editor_property("r", 0.85)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(npc_mat)
    log("Created M_NPCColor")

# M_Highlight: additive fresnel rim used as OverlayMaterial on the hovered NPC.
if not eal.does_asset_exist(f"{MAT_DIR}/M_Highlight"):
    hl_mat = asset_tools.create_asset("M_Highlight", MAT_DIR, unreal.Material,
                                      unreal.MaterialFactoryNew())
    hl_mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    hl_mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    fresnel = mel.create_material_expression(hl_mat, unreal.MaterialExpressionFresnel, -600, 0)
    fresnel.set_editor_property("exponent", 3.0)
    tint = mel.create_material_expression(hl_mat, unreal.MaterialExpressionVectorParameter, -600, 250)
    tint.set_editor_property("parameter_name", "Color")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 0.85, 0.2, 1.0))
    mul = mel.create_material_expression(hl_mat, unreal.MaterialExpressionMultiply, -350, 100)
    mel.connect_material_expressions(fresnel, "", mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(hl_mat)
    log("Created M_Highlight")

# --- Rebuild navigation and save ---------------------------------------------
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
level_editor.save_current_level()
log("Done! MarketMap saved. Press Play to test.")
