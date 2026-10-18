import os
import json

source_content_dir = "E:/Unreal Projects/Character Creator/Content"
target_content_dir = "E:/Unreal Projects/PortfolioTutorial/Project/Content"

assets_to_find = [
    # Missing parts used by presets:
    "SK_ACC_EpicHmn_CT_Cmn_Top",
    "SK_ACC_Epic_HMN_Adult_M_Top_001",
    "SK_ACC_Epic_HMN_Adult_M_Top_002",
    "SK_ACC_EpicHmn_CT_Cmn_Pant",
    "SK_ACC_EpicHmn_CT_Cmn_Foot",
    "SK_ACC_Epic_HMN_Adult_M_Foot_001",
    "SK_ACC_Access_HeadTopA_001",
    "SK_ACC_EpicHmn_Adult_Chistmas_Hat_F",
    "SK_ACCHCF_Foot_001",
    "SK_ACC_Access_Tail_001",
    "SK_ACC_EpicHmn_Christam_Outfit",
    "T_ACC_EpicHmn_Christmas_F_Mask",
    "SK_ACC_HC_Outfit_001",
    # And other parts from catalog:
    "SK_ACC_Epic_HMN_Adult_M_Pant_001",
    "SK_ACCHCF_Outfit_001",
    "SK_ACCHC_HairCommon",
    "SK_ACCHKF_Hair_02",
    "SK_ACCHKM_Hair_01",
    "SK_ACC_OldHmn_Adult_HF_001",
    "SK_ACC_OldHmn_Adult_HF_002",
    "SK_ACC_OldHmn_Adult_HF_003",
    "SK_ACC_OldHmn_Adult_HB_001",
    "SK_ACC_OldHmn_Adult_HB_002",
    "SK_ACC_OldHmn_Adult_HB_003",
    "SK_ACC_OldHmn_HS_001",
    "SK_ACC_OldHmn_HS_002",
    "SK_ACC_OldHmn_HS_003",
    "SK_ACC_EpicHmn_Adult_Male",
    "SK_ACC_EpicHmn_Adult_Female",
    "SK_ACCHCF_Body",
    "SK_ACC_EpicHmn_Adult_M_Head_001",
    "SK_ACC_EpicHmn_Adult_M_Head_002",
    "SK_ACC_EpicHmn_Adult_F_Head_001",
    "SK_ACC_EpicHmn_Adult_F_Head_002",
    "SK_ACC_EpicHmn_Adult_F_Head_003",
    "SK_ACC_HCM_Head_001",
    "SK_ACC_HCF_Head_001",
]

print("Scanning source content...")
source_matches = {}
for root, dirs, files in os.walk(source_content_dir):
    for f in files:
        base, ext = os.path.splitext(f)
        if base in assets_to_find:
            rel = os.path.relpath(os.path.join(root, f), source_content_dir).replace("\\", "/")
            if base not in source_matches:
                source_matches[base] = []
            source_matches[base].append(rel)

print("Scanning target content...")
target_matches = {}
for root, dirs, files in os.walk(target_content_dir):
    for f in files:
        base, ext = os.path.splitext(f)
        if base in assets_to_find:
            rel = os.path.relpath(os.path.join(root, f), target_content_dir).replace("\\", "/")
            if base not in target_matches:
                target_matches[base] = []
            target_matches[base].append(rel)

results = []
for asset in assets_to_find:
    src = source_matches.get(asset, [])
    tgt = target_matches.get(asset, [])
    info = {
        "asset": asset,
        "in_source": src,
        "in_target": tgt
    }
    results.append(info)
    print(f"\nAsset: {asset}")
    print(f"  Source: {src}")
    print(f"  Target: {tgt}")

with open("E:/Unreal Projects/PortfolioTutorial/Project/Saved/AssetLocationMatches.json", "w") as fp:
    json.dump(results, fp, indent=2)

print("\nDone. Saved to Saved/AssetLocationMatches.json")
