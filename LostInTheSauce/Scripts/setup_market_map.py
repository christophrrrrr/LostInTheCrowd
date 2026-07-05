"""
Lost in the Sauce — market map setup (Modular Village edition).

Creates/refreshes /Game/LostInTheSauce/Maps/MarketMap: floor, prop-built
market (stalls from posts+canopies, stone perimeter with arch gates, well,
barrels/hay clutter), lighting, exposure, gameplay materials, navmesh
bounds and PlayerStart. Prop placement measures each mesh at runtime, so
pack-scale quirks can't break the layout.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>

Idempotent: market props are wiped (label prefix MV_/legacy labels) and
rebuilt every run; lights and settings are found-and-updated.
"""

import math
import random

import unreal

MAP_DIR = "/Game/LostInTheSauce/Maps"
MAP_PATH = f"{MAP_DIR}/MarketMap"
PROPS = "/Game/LostInTheSauce/Props"
PROP_SCALE = 100.0  # pack is authored in meters; UE wants centimeters

eal = unreal.EditorAssetLibrary
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

CUBE = eal.load_asset("/Engine/BasicShapes/Cube")
rng = random.Random(7)


def log(msg):
    unreal.log(f"[LostInTheSauce map] {msg}")


# --- Open or create the map ---------------------------------------------------
if not eal.does_directory_exist(MAP_DIR):
    eal.make_directory(MAP_DIR)

if eal.does_asset_exist(MAP_PATH):
    level_editor.load_level(MAP_PATH)
    log("MarketMap reopened")
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


def set_static_mesh(actor, mesh):
    comp = actor.static_mesh_component
    comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    comp.set_static_mesh(mesh)
    comp.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    return comp


def spawn_cube(label, loc, scale, yaw=0.0):
    actor = spawn(unreal.StaticMeshActor, label, loc, (0.0, 0.0, yaw))
    set_static_mesh(actor, CUBE)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


_prop_cache = {}


def prop_mesh(name):
    if name not in _prop_cache:
        path = f"{PROPS}/{name}"
        _prop_cache[name] = eal.load_asset(path) if eal.does_asset_exist(path) else None
        if _prop_cache[name] is None:
            log(f"MISSING prop: {name}")
    return _prop_cache[name]


def prop_bounds(name):
    mesh = prop_mesh(name)
    if not mesh:
        return None
    box = mesh.get_bounding_box()
    return box  # in pack units (meters-ish)


def place(name, label, x, y, yaw=0.0, scale=PROP_SCALE, scale_vec=None, z=None, max_dim=None):
    """Place a prop grounded on the floor (z=0) unless z is given.
    max_dim caps the largest world dimension (some retried imports carry
    surprise author scales)."""
    mesh = prop_mesh(name)
    if not mesh:
        return None
    box = mesh.get_bounding_box()
    sv = list(scale_vec) if scale_vec else [scale, scale, scale]
    if max_dim:
        size = box.max - box.min
        largest = max(size.x * sv[0], size.y * sv[1], size.z * sv[2])
        if largest > max_dim:
            shrink = max_dim / largest
            sv = [c * shrink for c in sv]
    if z is None:
        z = -box.min.z * sv[2]
    actor = spawn(unreal.StaticMeshActor, f"MV_{label}", (x, y, z), (0.0, 0.0, yaw))
    set_static_mesh(actor, mesh)
    actor.set_actor_scale3d(unreal.Vector(*sv))
    return actor


# --- Wipe previous market content ----------------------------------------------
LEGACY_PREFIXES = ("MV_", "Stall_", "Crate_", "Wall_")
for actor in list(actor_sub.get_all_level_actors()):
    label = actor.get_actor_label()
    if any(label.startswith(p) for p in LEGACY_PREFIXES):
        actor_sub.destroy_actor(actor)

# --- Lighting (find-or-create, then always refresh properties) ------------------
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
fog_comp.set_editor_property("fog_density", 0.006)
fog_comp.set_editor_property("start_distance", 2000.0)

skylight = find_by_label("SkyLight") or spawn(unreal.SkyLight, "SkyLight", (0, 0, 500))
skylight_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
skylight_comp.set_editor_property("real_time_capture", True)

# Exposure clamp + no motion blur (scanning game).
ppv = find_by_label("ExposureVolume") or spawn(unreal.PostProcessVolume, "ExposureVolume", (0, 0, 0))
ppv.set_editor_property("unbound", True)
pp_settings = ppv.get_editor_property("settings")
pp_settings.set_editor_property("override_auto_exposure_min_brightness", True)
pp_settings.set_editor_property("override_auto_exposure_max_brightness", True)
pp_settings.set_editor_property("auto_exposure_min_brightness", 11.8)
pp_settings.set_editor_property("auto_exposure_max_brightness", 12.8)
pp_settings.set_editor_property("override_motion_blur_amount", True)
pp_settings.set_editor_property("motion_blur_amount", 0.0)
ppv.set_editor_property("settings", pp_settings)

# --- Gameplay materials (idempotent) --------------------------------------------
MAT_DIR = "/Game/LostInTheSauce/Materials"
if not eal.does_directory_exist(MAT_DIR):
    eal.make_directory(MAT_DIR)

# Recreate the shared tint material from scratch every run: mutating and
# recompiling an EXISTING material in a commandlet corrupts its shader
# state and greys out everything derived from it (learned the hard way).
# Two-sided is set BEFORE the first compile (flipped OBJ normals otherwise
# render black from behind).
for stale in [f"{MAT_DIR}/M_NPCColor", f"{MAT_DIR}/MI_Ground", f"{MAT_DIR}/MI_Stone",
              f"{MAT_DIR}/MI_Wood", f"{MAT_DIR}/MI_Cloth", f"{MAT_DIR}/MI_Straw"]:
    if eal.does_asset_exist(stale):
        eal.delete_asset(stale)

npc_mat = asset_tools.create_asset("M_NPCColor", MAT_DIR, unreal.Material,
                                   unreal.MaterialFactoryNew())
npc_mat.set_editor_property("two_sided", True)
# Without this usage flag, -game builds silently swap in the default gray
# material on all skeletal meshes (editor auto-adds it; packaged/game won't).
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
log("Recreated M_NPCColor (two-sided)")

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

# Prop palette: the OBJ materials don't survive import, so props get flat
# stylized tints from our parameterized material (matches the crowd look).
PALETTE = {
    "MI_Ground": (0.42, 0.36, 0.28),
    "MI_Stone":  (0.55, 0.52, 0.47),
    "MI_Wood":   (0.38, 0.26, 0.16),
    "MI_Cloth":  (0.62, 0.28, 0.20),
    "MI_Straw":  (0.72, 0.62, 0.32),
}
for mi_name, rgb in PALETTE.items():
    if not eal.does_asset_exist(f"{MAT_DIR}/{mi_name}"):
        mi = asset_tools.create_asset(mi_name, MAT_DIR, unreal.MaterialInstanceConstant,
                                      unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, eal.load_asset(f"{MAT_DIR}/M_NPCColor"))
        mel.set_material_instance_vector_parameter_value(
            mi, "Color", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        log(f"Created {mi_name}")


def tint(actor, mi_name):
    if actor:
        mi = eal.load_asset(f"{MAT_DIR}/{mi_name}")
        comp = actor.static_mesh_component
        for slot in range(comp.get_num_materials()):
            comp.set_material(slot, mi)
    return actor

# --- Ground: 46m x 46m square ----------------------------------------------------
floor = find_by_label("Floor") or spawn_cube("Floor", (0, 0, -25), (46, 46, 0.5))
floor.static_mesh_component.set_material(0, eal.load_asset(f"{MAT_DIR}/MI_Ground"))

# --- Perimeter: stone wall segments with arch gates -------------------------------
WALL_SCALE = 200.0
wall_box = prop_bounds("Stone_Wall_1")
if wall_box:
    # One wall type only: mixed variants have different widths and leave gaps.
    seg_w = (wall_box.max.y - wall_box.min.y) * WALL_SCALE  # wall runs along its Y
    half = 2300.0
    n_segs = int((half * 2) / seg_w) + 1
    idx = 0
    for side in range(4):
        for s in range(n_segs):
            pos_along = -half + seg_w * (s + 0.5)
            if abs(pos_along) < seg_w:  # gate opening mid-side
                continue
            # Piece width runs along its LOCAL Y: rows along world X need
            # yaw=90, rows along world Y need yaw=0.
            if side == 0:
                x, y, yaw = pos_along, half, 90
            elif side == 1:
                x, y, yaw = pos_along, -half, 90
            elif side == 2:
                x, y, yaw = half, pos_along, 0
            else:
                x, y, yaw = -half, pos_along, 0
            tint(place("Stone_Wall_1", f"WallSeg_{idx}", x, y, yaw=yaw, scale=WALL_SCALE), "MI_Stone")
            idx += 1
    for gi, (gx, gy, gyaw) in enumerate([(0, 2300, 90), (0, -2300, 90), (2300, 0, 0), (-2300, 0, 0)]):
        tint(place("Stone_Arch", f"Gate_{gi}", gx, gy, yaw=gyaw, scale=175.0), "MI_Stone")

# --- Market stalls: posts + canopy roof, barrels and hay beside -------------------
post_box = prop_bounds("Wood_Post_Large")
canopy_box = prop_bounds("Canopy_Top")
if post_box and canopy_box:
    # Force stall height: stretch posts to reach it no matter how the post
    # mesh is authored (it imported far shorter than expected).
    STALL_H = 230.0
    post_nat_h = max((post_box.max.z - post_box.min.z), 0.01)
    post_scale_z = STALL_H / (post_nat_h * PROP_SCALE) * PROP_SCALE
    can_sx = (canopy_box.max.x - canopy_box.min.x)
    can_sy = (canopy_box.max.y - canopy_box.min.y)
    STALL_W, STALL_D = 300.0, 220.0
    for i in range(8):
        ang = math.radians(i * 45.0 + 22.5)
        cx, cy = math.cos(ang) * 1250.0, math.sin(ang) * 1250.0
        yaw = i * 45.0 + 22.5
        rot = math.radians(yaw)
        for pi, (lx, ly) in enumerate([(-STALL_W / 2, -STALL_D / 2), (STALL_W / 2, -STALL_D / 2),
                                       (-STALL_W / 2, STALL_D / 2), (STALL_W / 2, STALL_D / 2)]):
            wx = cx + lx * math.cos(rot) - ly * math.sin(rot)
            wy = cy + lx * math.sin(rot) + ly * math.cos(rot)
            tint(place("Wood_Post_Large", f"Stall{i}_Post{pi}", wx, wy,
                       scale_vec=(PROP_SCALE, PROP_SCALE, post_scale_z)), "MI_Wood")
        canopy_scale = ((STALL_W + 60) / can_sx, (STALL_D + 60) / can_sy, PROP_SCALE)
        tint(place("Canopy_Top", f"Stall{i}_Canopy", cx, cy, yaw=yaw,
                   scale_vec=canopy_scale, z=STALL_H), "MI_Cloth")
        # market goods beside each stall
        goods = rng.choice(["Prop_Barrel_1", "Prop_Barrel_2", "Prop_Hay_1"])
        gx = cx + math.cos(ang) * 240.0
        gy = cy + math.sin(ang) * 240.0
        tint(place(goods, f"Stall{i}_Goods", gx, gy, yaw=rng.uniform(0, 360), max_dim=110.0),
             "MI_Straw" if "Hay" in goods else "MI_Wood")

# --- Centerpiece well + scattered clutter -----------------------------------------
tint(place("Prop_Well_Cobblestone", "Well", 0, 0, yaw=15.0, scale=140.0), "MI_Stone")

CLUTTER = {
    "Prop_Barrel_1": "MI_Wood", "Prop_Barrel_2": "MI_Wood",
    "Prop_Hay_1": "MI_Straw", "Prop_Crate_1": "MI_Wood",
    "Prop_Cart_1_Hay": "MI_Straw", "Prop_Lamp_Street": "MI_Wood",
}
clutter_names = sorted(CLUTTER.keys())
for ci in range(18):
    name = rng.choice(clutter_names)
    ang = rng.uniform(0, math.tau)
    r = rng.uniform(500, 2050)
    if 1050 < r < 1450:  # keep the stall ring breathable
        r += 450
    cap = 300.0 if "Cart" in name or "Lamp" in name else 130.0
    tint(place(name, f"Clutter_{ci}", math.cos(ang) * r, math.sin(ang) * r,
               yaw=rng.uniform(0, 360), max_dim=cap), CLUTTER[name])

# --- Stale navmesh actor cleanup (Dynamic runtime generation handles it) ----------
for actor in list(actor_sub.get_all_level_actors()):
    if isinstance(actor, unreal.RecastNavMesh):
        log(f"Deleting stale navmesh actor {actor.get_actor_label()}")
        actor_sub.destroy_actor(actor)

# --- Navigation bounds + player start ----------------------------------------------
if not find_by_label("NavBounds"):
    nav = spawn(unreal.NavMeshBoundsVolume, "NavBounds", (0, 0, 200))
    nav.set_actor_scale3d(unreal.Vector(24, 24, 5))

if not find_by_label("MarketPlayerStart"):
    spawn(unreal.PlayerStart, "MarketPlayerStart", (0, 0, 300))

# --- Save ---------------------------------------------------------------------------
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
level_editor.save_current_level()
log("Done! MarketMap saved.")
