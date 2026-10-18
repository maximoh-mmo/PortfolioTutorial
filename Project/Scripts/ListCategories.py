import os

onset_cc_dir = "E:/Unreal Projects/PortfolioTutorial/Project/Content/IS_CharacterCreator"

all_files = []
for root, dirs, files in os.walk(onset_cc_dir):
    for f in files:
        rel = os.path.relpath(os.path.join(root, f), onset_cc_dir).replace("\\", "/")
        all_files.append(rel)

categories = {}
for f in all_files:
    top = f.split("/")[0]
    categories[top] = categories.get(top, 0) + 1

print("Categories in Content/IS_CharacterCreator:")
for cat, count in categories.items():
    print(f"  {cat}: {count} files")
