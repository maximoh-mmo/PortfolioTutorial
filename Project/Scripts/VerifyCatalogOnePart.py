import unreal

catalog_path = '/Game/OnsetCharacterCreation/Catalog/DA_OnsetAppearanceCatalog'
catalog = unreal.load_asset(catalog_path)
parts = catalog.get_editor_property('parts')
pid = unreal.Name('epic_hmn_adult_F_1001')
pdef = parts[pid]
mesh = pdef.get_editor_property('mesh')
print('VERIFY epic_hmn_adult_F_1001 mesh:', mesh)
if mesh:
    print('SUCCESS: mesh path is', mesh.get_path_name())
else:
    print('FAILED: mesh is None')
