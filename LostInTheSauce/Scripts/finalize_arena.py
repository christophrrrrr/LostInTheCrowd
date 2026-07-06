"""
Lost in the Sauce — finalize the arena around the (hand-placed) town.

Does three things WITHOUT moving TownEnv (the user positioned it by hand):
  1. Enables collision + navigation-affect on the town, so the navmesh
     carves streets and characters can't walk through buildings.
  2. Rings the play platform with a tall stone rampart (clean edge cutoff
     that also hides the stray outlying mesh pieces beyond it).
  3. Rebuilds navigation and saves.

Idempotent: rebuilds the ArenaWall_* actors each run; never touches TownEnv
location/rotation/scale.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>
"""

import unreal

MAP_PATH = "/Game/LostInTheSauce/Maps/MarketMap"
MAT_DIR = "/Game/LostInTheSauce/Materials"
CUBE = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

level_editor.load_level(MAP_PATH)


def find_by_label(label):
    for a in actor_sub.get_all_level_actors():
        if a.get_actor_label() == label:
            return a
    return None


# --- 1) Town collision (do NOT move the actor) ---------------------------------
town = find_by_label("TownEnv")
if town:
    comp = town.static_mesh_component
    # BlockAll profile sets every channel to Block without the response enum.
    comp.set_collision_profile_name("BlockAll")
    comp.set_editor_property("can_ever_affect_navigation", True)
    # Complex-as-simple was set at import; confirm the mesh asset keeps it so
    # per-building geometry blocks capsules and carves the navmesh.
    mesh = comp.get_editor_property("static_mesh")
    if mesh:
        body = mesh.get_editor_property("body_setup")
        if body:
            body.set_editor_property(
                "collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            eal.save_asset(mesh.get_path_name().split(".")[0])
    unreal.log("[arena] TownEnv collision + navigation enabled (not moved)")
else:
    unreal.log("[arena] WARNING: no TownEnv found")

# --- Stone material ------------------------------------------------------------
if not eal.does_asset_exist(f"{MAT_DIR}/MI_Stone"):
    mi = asset_tools.create_asset("MI_Stone", MAT_DIR, unreal.MaterialInstanceConstant,
                                  unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, eal.load_asset(f"{MAT_DIR}/M_NPCColor"))
    mel.set_material_instance_vector_parameter_value(
        mi, "Color", unreal.LinearColor(0.52, 0.50, 0.46, 1.0))
    eal.save_asset(f"{MAT_DIR}/MI_Stone")
STONE = eal.load_asset(f"{MAT_DIR}/MI_Stone")

# --- 2) Stone rampart ring around the platform ---------------------------------
for actor in list(actor_sub.get_all_level_actors()):
    if actor.get_actor_label().startswith("ArenaWall_"):
        actor_sub.destroy_actor(actor)

# Platform is the 64x64m NavFloor centered on origin; walls sit just outside.
HALF = 3200.0        # platform half-extent (cm)
WALL_H = 1000.0      # 10m castle wall — tall enough to hide strays at all
                     # orbit angles (mid-pitch sightlines clear a 7.5m wall)
WALL_T = 60.0        # thickness (cm)
run = (HALF * 2 + WALL_T * 2) / 100.0  # length in meters -> cube scale


def wall(label, loc, scale):
    a = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc))
    a.set_actor_label(label)
    c = a.static_mesh_component
    c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    c.set_static_mesh(CUBE)
    c.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    a.set_actor_scale3d(unreal.Vector(*scale))
    c.set_material(0, STONE)
    return a


z = WALL_H / 2.0
wall("ArenaWall_N", (0,  HALF + WALL_T / 2, z), (run, WALL_T / 100.0, WALL_H / 100.0))
wall("ArenaWall_S", (0, -HALF - WALL_T / 2, z), (run, WALL_T / 100.0, WALL_H / 100.0))
wall("ArenaWall_E", ( HALF + WALL_T / 2, 0, z), (WALL_T / 100.0, run, WALL_H / 100.0))
wall("ArenaWall_W", (-HALF - WALL_T / 2, 0, z), (WALL_T / 100.0, run, WALL_H / 100.0))
unreal.log("[arena] rampart ring placed")

# --- 3) Rebuild nav + save -----------------------------------------------------
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
level_editor.save_current_level()
unreal.log("[arena] Done.")
