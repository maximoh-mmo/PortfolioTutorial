import re

umap_path = "E:/Unreal Projects/PortfolioTutorial/Project/Content/OnsetCharacterCreation/Maps/NativeOnsetAppearancePreview.umap"
with open(umap_path, "rb") as f:
    data = f.read()

strings = re.findall(rb'/Game/[a-zA-Z0-9_/]+', data)
unique_strings = sorted(list(set(s.decode('latin1') for s in strings)))

print("--- Packages referenced in NativeOnsetAppearancePreview.umap ---")
for s in unique_strings:
    if "Cloth" in s or "Mesh" in s or "Outfit" in s or "Item" in s:
        print(" ", s)
