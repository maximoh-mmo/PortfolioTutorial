import unreal

catalog_path = '/Game/OnsetCharacterCreation/Catalog/DA_OnsetAppearanceCatalog'
catalog = unreal.load_asset(catalog_path)
print('Loaded catalog:', catalog)

parts = catalog.get_editor_property('parts')
print('Parts count in python:', len(parts))

# Let's inspect part epic_hmn_adult_F_1001
pid = unreal.Name('epic_hmn_adult_F_1001')
pdef = parts[pid]
mesh_before = pdef.get_editor_property('mesh')
print('Before:', pid, 'mesh =', mesh_before)

target_asset = '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/F_001/SK_ACC_EpicHmn_CT_Cmn_Top'
obj = unreal.load_asset(target_asset)
print('Loaded target asset:', obj)

pdef.set_editor_property('mesh', obj)
mesh_after_set = pdef.get_editor_property('mesh')
print('After pdef set:', pid, 'mesh =', mesh_after_set)

parts[pid] = pdef
catalog.set_editor_property('parts', parts)

# Read back from catalog
parts_readback = catalog.get_editor_property('parts')
mesh_readback = parts_readback[pid].get_editor_property('mesh')
print('Read back from catalog:', pid, 'mesh =', mesh_readback)

# Save with only_if_dirty=False
saved = unreal.EditorAssetLibrary.save_loaded_asset(catalog, False)
print('save_loaded_asset(catalog, False):', saved)

# Also test package save
pkg = catalog.get_outermost()
print('Package:', pkg.get_name(), 'is_dirty:', pkg.is_dirty())
