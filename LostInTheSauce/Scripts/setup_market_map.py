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

# --- Lighting ---------------------------------------------------------------
if "Sun" not in labels:
    sun = spawn(unreal.DirectionalLight, "Sun", (0, 0, 2000), (0.0, -50.0, 35.0))
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property(
        "atmosphere_sun_light", True)

if "Sky" not in labels:
    spawn(unreal.SkyAtmosphere, "Sky", (0, 0, 0))

if "SkyLight" not in labels:
    skylight = spawn(unreal.SkyLight, "SkyLight", (0, 0, 500))
    skylight.get_component_by_class(unreal.SkyLightComponent).set_editor_property(
        "real_time_capture", True)

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

# --- Rebuild navigation and save ---------------------------------------------
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
level_editor.save_current_level()
log("Done! MarketMap saved. Press Play to test.")
