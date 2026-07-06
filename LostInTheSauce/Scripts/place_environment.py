"""
Lost in the Sauce — place the Town environment mesh into MarketMap.

The town is ONE pre-built mesh (from import_scenes.py): centered on world
origin, grounded at z=0, complex-as-simple collision so navmesh + capsules +
clicks all work against the real geometry. Also removes leftover graybox
decor (MV_ prefix) and the graybox Floor, and sizes NavBounds to the town.

Idempotent: re-places the TownEnv actor each run. Safe to rerun.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<this file>
"""

import unreal

MAP_PATH = "/Game/LostInTheSauce/Maps/MarketMap"
TOWN_MESH = "/Game/LostInTheSauce/Scenes/modular_lowpoly_medieval_environment/MediEval_Scene"

eal = unreal.EditorAssetLibrary
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

level_editor.load_level(MAP_PATH)


def find_by_label(label):
    for a in actor_sub.get_all_level_actors():
        if a.get_actor_label() == label:
            return a
    return None


# --- Clear old graybox decor + placeholder floor -------------------------------
removed = 0
for actor in list(actor_sub.get_all_level_actors()):
    label = actor.get_actor_label()
    if label.startswith("MV_") or label in ("Floor", "TownEnv", "NavFloor"):
        actor_sub.destroy_actor(actor)
        removed += 1
unreal.log(f"[env] removed {removed} old decor/floor/prev-town actors")

CUBE = eal.load_asset("/Engine/BasicShapes/Cube")
MAT_DIR = "/Game/LostInTheSauce/Materials"

# Guaranteed walkable ground: a flat sandy plane the navmesh reliably builds
# on (the town's own combined-mesh collision does not feed navigation). The
# town sits on top as environment; NPCs walk this surface at town-ground z=0.
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
if not eal.does_asset_exist(f"{MAT_DIR}/MI_Ground"):
    mi = asset_tools.create_asset("MI_Ground", MAT_DIR, unreal.MaterialInstanceConstant,
                                  unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, eal.load_asset(f"{MAT_DIR}/M_NPCColor"))
    mel.set_material_instance_vector_parameter_value(
        mi, "Color", unreal.LinearColor(0.52, 0.45, 0.34, 1.0))
    eal.save_asset(f"{MAT_DIR}/MI_Ground")

navfloor = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -25))
navfloor.set_actor_label("NavFloor")
nfc = navfloor.static_mesh_component
nfc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
nfc.set_static_mesh(CUBE)
nfc.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
navfloor.set_actor_scale3d(unreal.Vector(64, 64, 0.5))  # 64x64m, top at z=0
nfc.set_material(0, eal.load_asset(f"{MAT_DIR}/MI_Ground"))

# --- Place the town ------------------------------------------------------------
# Use spawn_class + set_static_mesh (the pattern that works for cubes);
# spawn_actor_from_object crashed the editor on this large mesh.
mesh = eal.load_asset(TOWN_MESH)
town = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0),
                                        unreal.Rotator(0, 0, 0))
town.set_actor_label("TownEnv")
comp = town.static_mesh_component
comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
comp.set_static_mesh(mesh)

# Measure the ACTUAL placed world bounds (the mesh pivot is not at its
# geometry center, so trusting local bounds put the town off-origin). Then
# shift so the footprint centers on origin and the base sits at z=0.
origin, extent = town.get_actor_bounds(False)
# The combined mesh's bbox center sits between the main village and some
# stray outlying pieces, so bbox-centering leaves the village off to +Y.
# VILLAGE_OFFSET nudges the village over the play area (tune from aerial).
VILLAGE_OFFSET = unreal.Vector(-300, -2200, 0)
town.set_actor_location(
    unreal.Vector(-origin.x, -origin.y, extent.z - origin.z) + VILLAGE_OFFSET, False, False)
comp.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
# StaticMeshActor blocks by default; the mesh's complex-as-simple collision
# (set at import) serves navmesh raycasts, capsules and the click-trace.

unreal.log(f"[env] TownEnv footprint {round(extent.x*2/100,1)}x{round(extent.y*2/100,1)}m")

# --- Size NavBounds to the flat play area (the NavFloor) -----------------------
nav = find_by_label("NavBounds")
if nav:
    # NavMeshBoundsVolume base box is 200 units; cover the 64x64m floor + air.
    nav.set_actor_location(unreal.Vector(0, 0, 200), False, False)
    nav.set_actor_scale3d(unreal.Vector(34, 34, 16))

# --- Rebuild nav + save --------------------------------------------------------
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
level_editor.save_current_level()
unreal.log("[env] Done. Town placed and nav rebuilt.")
