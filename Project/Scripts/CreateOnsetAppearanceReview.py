"""Create a disposable, inspectable Onset appearance review level."""
import unreal

MAP = '/Game/OnsetCharacterCreation/Maps/NativeOnsetAppearancePreview'
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    raise RuntimeError('Refusing to overwrite ' + MAP)

unreal.EditorAssetLibrary.make_directory('/Game/OnsetCharacterCreation/Maps')
if not unreal.EditorLevelLibrary.new_level(MAP):
    raise RuntimeError('Could not create ' + MAP)

player_class = unreal.EditorAssetLibrary.load_blueprint_class('/Game/Characters/Player/PlayerCharacter')
if not player_class:
    raise RuntimeError('PlayerCharacter Blueprint missing')

presets = [
    ('Epic Female', '/Game/OnsetCharacterCreation/Presets/DA_EpicFemale', player_class, (0, 0, 90)),
    ('Epic Male', '/Game/OnsetCharacterCreation/Presets/DA_EpicMale', player_class, (0, 250, 90)),
    ('Human Kid Female NPC', '/Game/OnsetCharacterCreation/Presets/DA_HumanKidFemaleNPC', unreal.OnsetBaseCharacter, (0, 500, 55)),
    ('Human Kid Male NPC', '/Game/OnsetCharacterCreation/Presets/DA_HumanKidMaleNPC', unreal.OnsetBaseCharacter, (0, 750, 55)),
]

for label, path, actor_class, xyz in presets:
    preset = unreal.load_asset(path)
    if not preset:
        raise RuntimeError('Missing ' + path)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, unreal.Vector(*xyz))
    if not actor:
        raise RuntimeError('Could not spawn ' + label)
    actor.set_actor_label(label)
    component = actor.get_component_by_class(unreal.OnsetCharacterAppearanceComponent)
    if not component:
        raise RuntimeError('No character appearance component on ' + label)
    if actor_class == unreal.OnsetBaseCharacter:
        actor.get_components_by_class(unreal.CapsuleComponent)[0].set_capsule_size(25, 55)
        component.set_editor_property('authored_preset', preset)
    result = component.set_appearance_recipe(preset.get_editor_property('recipe'))
    print('ONSET_REVIEW_APPLIED', label, result)
    if result is False or (isinstance(result, tuple) and result[0] is False):
        raise RuntimeError('Recipe failed for ' + label)

floor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 375, -1))
floor.set_actor_label('Review Floor')
floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'))
floor.set_actor_scale3d(unreal.Vector(10, 14, 1))

light = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(-400, 375, 600))
light.set_actor_label('Review Light')
light.set_actor_rotation(unreal.Rotator(-40, -20, 0), False)

if not unreal.EditorLevelLibrary.save_current_level():
    raise RuntimeError('Could not save ' + MAP)
print('ONSET_REVIEW_SAVED', MAP)
