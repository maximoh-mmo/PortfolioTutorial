import unreal

clips = [
    '/Game/IS_CharacterCreator/Character/Human/Kid/Animation/Retarget/A_ACC_Hmn_HC_Wait_01',
    '/Game/IS_CharacterCreator/Character/Human/Kid/Animation/Retarget/A_ACC_Hmn_HC_Grd_Run_F',
    '/Game/IS_CharacterCreator/Character/Human/Kid/Animation/Retarget/A_ACC_Hmn_HC_Jump_Air',
]

for c in clips:
    obj = unreal.load_asset(c)
    if obj:
        unreal.log(f'SUCCESS: Loaded {c} -> {obj.get_class().get_name()} (Skeleton={obj.get_editor_property("skeleton").get_name() if obj.get_editor_property("skeleton") else "NONE"})')
    else:
        unreal.log_error(f'FAILED: Could not load {c}')
