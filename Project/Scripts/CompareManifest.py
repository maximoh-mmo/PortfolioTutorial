import os
import json

manifest_path = "E:/Unreal Projects/PortfolioTutorial/Project/Docs/AppearanceMigrationDependencies.json"
source_content_dir = "E:/Unreal Projects/Character Creator/Content"
target_content_dir = "E:/Unreal Projects/PortfolioTutorial/Project/Content"

with open(manifest_path, "r") as f:
    manifest = json.load(f)

packages = manifest.get("packages", [])
print(f"Total packages in manifest: {len(packages)}")

missing_in_source = []
missing_in_target = []
present_in_both = []

for pkg in packages:
    # Package is e.g. /Game/IS_CharacterCreator/Character/...
    # Relative path from Content is e.g. IS_CharacterCreator/Character/...
    assert pkg.startswith("/Game/")
    rel_path = pkg[len("/Game/"):]
    src_uasset = os.path.join(source_content_dir, rel_path + ".uasset")
    tgt_uasset = os.path.join(target_content_dir, rel_path + ".uasset")

    src_exists = os.path.exists(src_uasset)
    tgt_exists = os.path.exists(tgt_uasset)

    if not src_exists:
        missing_in_source.append(pkg)
    if not tgt_exists:
        missing_in_target.append((pkg, src_exists))
    if src_exists and tgt_exists:
        present_in_both.append(pkg)

print(f"Present in both: {len(present_in_both)}")
print(f"Missing in target: {len(missing_in_target)}")
print(f"Missing in source: {len(missing_in_source)}")

print("\n--- Missing in Source ---")
for p in missing_in_source:
    print(" ", p)

print("\n--- Missing in Target ---")
for p, src_ex in missing_in_target:
    print(f"  {p} (exists in source: {src_ex})")
