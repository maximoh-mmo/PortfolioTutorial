import os

source_content_dir = "E:/Unreal Projects/Character Creator/Content"

cloth_items = [
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Common/MI_ACCEpicHmn_CT_001",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Common/MI_ACCEpicHmn_CT_002",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Common/T_ACC_HMN_CT_002_ColorMap",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Common/T_ACC_HMN_CT_002_ILM",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Foot/Common/SKEL_ACC_EpicHmn_CT_Cmn_Foot",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Foot/F_001/PHYS_ACC_EpicHmn_CT_Cmn_Foot",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Foot/F_001/SK_ACC_EpicHmn_CT_Cmn_Foot",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Foot/M_001/SK_ACC_Epic_HMN_Adult_M_Foot_001",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Pant/Common/SKEL_ACC_EpicHmn_CT_Cmn_Pant",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Pant/F_001/PHYS_ACC_EpicHmn_CT_Cmn_Pant",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Pant/F_001/SK_ACC_EpicHmn_CT_Cmn_Pant",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Pant/M_001/SK_ACC_Epic_HMN_Adult_M_Pant_001",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/MI_ACC_EpicHmn_Chistmas_CT",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/PHYS_ACC_EpicHmn_Adult_Chistmas_Hat_F",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/SKEL_ACC_EpicHmn_Adult_Chistmas_Hat_F",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/SK_ACC_EpicHmn_Adult_Chistmas_Hat_F",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/SK_ACC_EpicHmn_Christam_Outfit",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/T_ACC_EpicHmn_Christmas_F_Mask",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/T_ACC_HMN_Christmas_Color",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Set/Christmas/T_ACC_HMN_Christmas_ColorMap",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/Common/SKEL_ACC_EpicHmn_CT_Cmn_Top",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/F_001/PHYS_ACC_EpicHmn_CT_Cmn_Top",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/F_001/SK_ACC_EpicHmn_CT_Cmn_Top",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/M_001/SK_ACC_Epic_HMN_Adult_M_Top_001",
    "IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Top/M_001/SK_ACC_Epic_HMN_Adult_M_Top_002",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/MI_ACC_Hmn_HK_CT_002",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/PHYS_ACC_HCF_Foot_001",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/PHYS_ACC_HCF_Outfit_001",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/PHYS_ACC_HC_Outfit_001",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/SKEL_ACC_HCF_Foot_001",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/SKEL_ACC_HC_Outfit_Body",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACCHCF_Foot_001",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACCHCF_Outfit_001",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACC_HC_Outfit_001",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/T_ACC_HC_Foot_1_Mask",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/T_ACC_HC_Outfit_1_Mask",
    "IS_CharacterCreator/Item/Cloth/Hmn/Kid/T_ACC_HC_Outfit_2_Mask",
    "IS_CharacterCreator/Item/Cloth/OldHmn/Adult/HeadTopA/001/MI_ACC_Access_HeadTopA_001",
    "IS_CharacterCreator/Item/Cloth/OldHmn/Adult/HeadTopA/001/PHYS_ACC_Access_HeadTopA_001",
    "IS_CharacterCreator/Item/Cloth/OldHmn/Adult/HeadTopA/001/SKEL_ACC_OldHmn_Access_HeadTopA_001",
    "IS_CharacterCreator/Item/Cloth/OldHmn/Adult/HeadTopA/001/SK_ACC_Access_HeadTopA_001",
    "IS_CharacterCreator/Item/Cloth/OldHmn/Adult/Tail/001/MI_Access_Tail_001",
    "IS_CharacterCreator/Item/Cloth/OldHmn/Adult/Tail/001/PHYS_ACC_Access_Tail_001",
    "IS_CharacterCreator/Item/Cloth/OldHmn/Adult/Tail/001/SKEL_ACC_OldHmn_Access_Tail_001",
    "IS_CharacterCreator/Item/Cloth/OldHmn/Adult/Tail/001/SK_ACC_Access_Tail_001",
]

sidecars = []
for item in cloth_items:
    full_no_ext = os.path.join(source_content_dir, item)
    folder = os.path.dirname(full_no_ext)
    base = os.path.basename(full_no_ext)
    if os.path.exists(folder):
        for f in os.listdir(folder):
            if f.startswith(base + "."):
                ext = f[len(base):]
                sidecars.append((item, ext))

print(f"Total files found for {len(cloth_items)} items: {len(sidecars)}")
extensions = set(ext for item, ext in sidecars)
print("Extensions found:", extensions)
