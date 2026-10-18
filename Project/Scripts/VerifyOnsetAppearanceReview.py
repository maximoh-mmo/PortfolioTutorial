import unreal

path = '/Game/OnsetCharacterCreation/Maps/NativeOnsetAppearancePreview'
if not unreal.EditorLevelLibrary.load_level(path):
    raise RuntimeError('Could not reload ' + path)

expected = {
    'Epic Female': 'epic.adult.female',
    'Epic Male': 'epic.adult.male',
    'Human Kid Female NPC': 'human.kid.authored',
    'Human Kid Male NPC': 'human.kid.authored',
}
found = {}
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    label = actor.get_actor_label()
    if label not in expected:
        continue
    component = actor.get_component_by_class(unreal.OnsetCharacterAppearanceComponent)
    if not component:
        raise RuntimeError(label + ' has no appearance component')
    json = component.get_editor_property('appearance_json')
    if not json:
        raise RuntimeError(label + ' has no serialized recipe')
    recipe = component.get_editor_property('applied_recipe')
    body = component.get_body_mesh()
    head = component.get_head_mesh()
    selected_parts = len(recipe.get_editor_property('parts'))
    mesh_components = len(actor.get_components_by_class(unreal.SkeletalMeshComponent))
    print('ONSET_REVIEW_RELOADED', label, recipe.get_editor_property('body_id'), bool(body), bool(head), selected_parts, mesh_components, len(json))
    if recipe.get_editor_property('body_id') != expected[label] or not body or not head or mesh_components < selected_parts + 2:
        raise RuntimeError(label + ' failed reconstruction')
    found[label] = True
if set(found) != set(expected):
    raise RuntimeError('Missing review actors: ' + str(set(expected) - set(found)))
print('ONSET_REVIEW_VERIFIED', len(found))
