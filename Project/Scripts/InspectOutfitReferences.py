import os
import re

def extract_game_strings(uasset_path):
    if not os.path.exists(uasset_path):
        return ["FILE NOT FOUND"]
    with open(uasset_path, "rb") as f:
        data = f.read()
    strings = re.findall(rb'/Game/[a-zA-Z0-9_/]+', data)
    return sorted(list(set(s.decode('latin1') for s in strings)))

target_outfit1 = "E:/Unreal Projects/PortfolioTutorial/Project/Content/IS_CharacterCreator/Outfit/Cloth/EpicHmn/Kid/SK_ACCHCF_Outfit_001.uasset"
target_outfit2 = "E:/Unreal Projects/PortfolioTutorial/Project/Content/IS_CharacterCreator/Item/Cloth/EpicHmn/Kid/SK_ACCHCF_Outfit_001.uasset"
source_outfit = "E:/Unreal Projects/Character Creator/Content/IS_CharacterCreator/Item/Cloth/Hmn/Kid/SK_ACCHCF_Outfit_001.uasset"

print("--- Onset target Outfit/ SK_ACCHCF_Outfit_001 ---")
for s in extract_game_strings(target_outfit1):
    print(" ", s)

print("\n--- Onset target Item/ SK_ACCHCF_Outfit_001 ---")
for s in extract_game_strings(target_outfit2):
    print(" ", s)

print("\n--- Source project SK_ACCHCF_Outfit_001 ---")
for s in extract_game_strings(source_outfit):
    print(" ", s)
