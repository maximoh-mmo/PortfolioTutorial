import unreal

assets_to_test = [
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/F_001/SK_ACC_EpicHmn_CT_Cmn_Top',
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/M_001/SK_ACC_Epic_HMN_Adult_M_Top_001',
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/M_001/SK_ACC_Epic_HMN_Adult_M_Top_002',
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Pant/F_001/SK_ACC_EpicHmn_CT_Cmn_Pant',
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Pant/M_001/SK_ACC_Epic_HMN_Adult_M_Pant_001',
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Foot/F_001/SK_ACC_EpicHmn_CT_Cmn_Foot',
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Foot/M_001/SK_ACC_Epic_HMN_Adult_M_Foot_001',
    '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACCHCF_Foot_001',
    '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACCHCF_Outfit_001',
    '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACC_HC_Outfit_001',
    '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/T_ACC_HC_Foot_1_Mask',
    '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/T_ACC_HC_Outfit_1_Mask',
    '/Game/IS_CharacterCreator/Item/Cloth/Hmn/Kid/T_ACC_HC_Outfit_2_Mask',
    '/Game/IS_CharacterCreator/Item/Cloth/OldHmn/Adult/HeadTopA/001/SK_ACC_Access_HeadTopA_001',
    '/Game/IS_CharacterCreator/Item/Cloth/OldHmn/Adult/Tail/001/SK_ACC_Access_Tail_001',
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/SK_ACC_EpicHmn_Adult_Chistmas_Hat_F',
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/SK_ACC_EpicHmn_Christam_Outfit',
    '/Game/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/T_ACC_EpicHmn_Christmas_F_Mask',
]

failed = 0
for a in assets_to_test:
    obj = unreal.load_asset(a)
    if not obj:
        print(f'FAILED to load {a}')
        failed += 1
    else:
        print(f'LOADED {a} -> {obj.get_class().get_name()}')

print(f'Summary: {len(assets_to_test) - failed}/{len(assets_to_test)} loaded successfully.')
