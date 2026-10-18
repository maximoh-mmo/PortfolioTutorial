import os
import re

def extract_game_strings(uasset_path):
    with open(uasset_path, "rb") as f:
        data = f.read()
    strings = re.findall(rb'/Game/[a-zA-Z0-9_/]+', data)
    return sorted(list(set(s.decode('latin1') for s in strings)))

target_pant = "E:/Unreal Projects/PortfolioTutorial/Project/Content/IS_CharacterCreator/Outfit/Cloth/Human/Adult/Pant/M_001/SK_ACC_Epic_HMN_Adult_M_Pant_001.uasset"
source_pant = "E:/Unreal Projects/Character Creator/Content/IS_CharacterCreator/Item/Cloth/EpicHmn/Adult/Pant/M_001/SK_ACC_Epic_HMN_Adult_M_Pant_001.uasset"

print("--- Onset target pant references ---")
for s in extract_game_strings(target_pant):
    print(" ", s)

print("\n--- Source project pant references ---")
for s in extract_game_strings(source_pant):
    print(" ", s)
