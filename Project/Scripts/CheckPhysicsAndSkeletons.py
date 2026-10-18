import os

def find_file(root_dir, filename):
    for root, dirs, files in os.walk(root_dir):
        if filename in files:
            return os.path.relpath(os.path.join(root, filename), root_dir).replace("\\", "/")
    return None

onset_content = "E:/Unreal Projects/PortfolioTutorial/Project/Content"
cc_content = "E:/Unreal Projects/Character Creator/Content"

for name in [
    "PHYS_ACC_EpicHmn_Character_Adult.uasset",
    "SKEL_ACC_EpicHmn_Adult.uasset",
    "PHYS_ACC_HCF_Body.uasset",
    "SKEL_ACC_HCF_Body.uasset",
    "SKEL_ACC_HC_Outfit_Body.uasset",
    "PHYS_ACC_HCF_Outfit_001.uasset",
    "SK_ACC_Epic_HMN_Adult_M_Top_001.uasset",
    "SK_ACC_EpicHmn_CT_Cmn_Top.uasset",
]:
    print(f"{name}:")
    print("  Onset: ", find_file(onset_content, name))
    print("  CC:    ", find_file(cc_content, name))
