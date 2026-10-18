import os

onset_cc_dir = "E:/Unreal Projects/PortfolioTutorial/Project/Content/IS_CharacterCreator"

all_files = []
for root, dirs, files in os.walk(onset_cc_dir):
    for f in files:
        rel = os.path.relpath(os.path.join(root, f), onset_cc_dir).replace("\\", "/")
        all_files.append(rel)

print(f"Total files in Onset IS_CharacterCreator: {len(all_files)}")
print("\nFiles in Outfit/:")
for f in sorted(all_files):
    if f.startswith("Outfit/"):
        print(" ", f)

print("\nFiles in Item/:")
for f in sorted(all_files):
    if f.startswith("Item/"):
        print(" ", f)
