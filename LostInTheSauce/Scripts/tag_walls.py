"""
Lost in the Sauce — tag the (hand-moved) rampart walls for the camera.

The orbit camera clamps its pan bounds to actors tagged "CameraBound". The
user repositioned the ArenaWall_* actors by hand, so this tags them in place
WITHOUT moving them. Run once after moving walls.
"""

import unreal

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

level_editor.load_level("/Game/LostInTheSauce/Maps/MarketMap")

tagged = 0
for actor in actor_sub.get_all_level_actors():
    if actor.get_actor_label().startswith("ArenaWall_"):
        actor.set_editor_property("tags", ["CameraBound"])
        tagged += 1

level_editor.save_current_level()
unreal.log(f"[tag_walls] tagged {tagged} rampart actors as CameraBound")
