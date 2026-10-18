import unreal

catalog_path = '/Game/OnsetCharacterCreation/Catalog/DA_OnsetAppearanceCatalog'
catalog = unreal.load_asset(catalog_path)
if not catalog:
    raise RuntimeError('Could not load catalog: ' + catalog_path)

parts = catalog.get_editor_property('parts')

updates = {
    'epic_hmn_adult_F_1001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/F_001/SK_ACC_EpicHmn_CT_Cmn_Top',
    },
    'epic_hmn_adult_M_1001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/M_001/SK_ACC_Epic_HMN_Adult_M_Top_001',
    },
    'epic_hmn_adult_M_1002': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/M_001/SK_ACC_Epic_HMN_Adult_M_Top_002',
    },
    'epic_hmn_adult_F_2001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Pant/F_001/SK_ACC_EpicHmn_CT_Cmn_Pant',
    },
    'epic_hmn_adult_M_2001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Pant/M_001/SK_ACC_Epic_HMN_Adult_M_Pant_001',
    },
    'epic_hmn_adult_F_3001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Foot/F_001/SK_ACC_EpicHmn_CT_Cmn_Foot',
    },
    'epic_hmn_adult_M_3001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Foot/M_001/SK_ACC_Epic_HMN_Adult_M_Foot_001',
    },
    'cmn_4001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/OldHmn/Adult/HeadTopA/001/SK_ACC_Access_HeadTopA_001',
    },
    'cmn_4002': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/SK_ACC_EpicHmn_Adult_Chistmas_Hat_F',
    },
    'epic_hmn_kid_cmn_3001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACCHCF_Foot_001',
        'body_hide_texture': '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/T_ACC_HC_Foot_1_Mask',
    },
    'cmn_9001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/OldHmn/Adult/Tail/001/SK_ACC_Access_Tail_001',
    },
    'epic_hmn_adult_F_1003': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/SK_ACC_EpicHmn_Christam_Outfit',
        'body_hide_texture': '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/T_ACC_EpicHmn_Christmas_F_Mask',
    },
    'epic_hmn_kid_f_1002': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACCHCF_Outfit_001',
        'body_hide_texture': '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/T_ACC_HC_Outfit_2_Mask',
    },
    'epic_hmn_kid_cmn_1001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACC_HC_Outfit_001',
        'body_hide_texture': '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/T_ACC_HC_Outfit_1_Mask',
    },
    'SK_ACC_Access_HeadTopA_001': {
        'mesh': '/Game/IS_CharacterCreator/Item/Cloth/OldHmn/Adult/HeadTopA/001/SK_ACC_Access_HeadTopA_001',
    },
}

for pid_name, part_updates in updates.items():
    pid = unreal.Name(pid_name)
    if pid not in parts:
        print('WARNING: Part not found in catalog:', pid_name)
        continue
    pdef = parts[pid]
    for prop, path_str in part_updates.items():
        obj = unreal.load_asset(path_str)
        if not obj:
            print('ERROR: Could not load', path_str, 'for part', pid_name)
            continue
        pdef.set_editor_property(prop, obj)
        print('Updated', pid_name, prop, '->', path_str)
    parts[pid] = pdef

catalog.set_editor_property('parts', parts)
saved = unreal.EditorAssetLibrary.save_loaded_asset(catalog)
print('Catalog saved:', saved)
