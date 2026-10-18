import unreal
import os
import json

print("=== INSPECTING ONSET APPEARANCE CATALOG & PRESETS ===")

catalog_path = "/Game/OnsetCharacterCreation/Catalog/DA_OnsetAppearanceCatalog"
catalog = unreal.load_asset(catalog_path)
if not catalog:
    print(f"ERROR: Could not load catalog at {catalog_path}")
else:
    print(f"SUCCESS: Loaded catalog {catalog_path}")

presets_to_check = [
    ("DA_EpicFemale", "/Game/OnsetCharacterCreation/Presets/DA_EpicFemale"),
    ("DA_EpicMale", "/Game/OnsetCharacterCreation/Presets/DA_EpicMale"),
    ("DA_HumanKidFemaleNPC", "/Game/OnsetCharacterCreation/Presets/DA_HumanKidFemaleNPC"),
    ("DA_HumanKidMaleNPC", "/Game/OnsetCharacterCreation/Presets/DA_HumanKidMaleNPC"),
]

missing_assets = []
all_referenced_assets = set()

def get_obj_path(val):
    if not val:
        return ""
    if hasattr(val, "to_soft_object_path"):
        soft = val.to_soft_object_path()
        return str(getattr(soft, "asset_path_name", soft))
    if hasattr(val, "get_path_name"):
        return val.get_path_name()
    return str(val)

def check_soft_path(path_str, context):
    if not path_str or path_str == "None":
        return
    clean_path = path_str.split(".")[0]
    all_referenced_assets.add(clean_path)
    exists = unreal.EditorAssetLibrary.does_asset_exist(clean_path)
    if not exists:
        missing_assets.append((context, clean_path, path_str))
        print(f"MISSING: [{context}] -> {path_str}")
    else:
        loaded = unreal.load_asset(clean_path)
        if not loaded:
            missing_assets.append((context + " (failed load)", clean_path, path_str))
            print(f"FAILED_LOAD: [{context}] -> {path_str}")
        else:
            print(f"OK: [{context}] -> {clean_path} ({loaded.get_class().get_name()})")

if catalog:
    blank_tex = get_obj_path(catalog.get_editor_property("blank_body_hide_texture"))
    print(f"\n--- Blank Hide Texture: {blank_tex} ---")
    check_soft_path(blank_tex, "Catalog.BlankBodyHideTexture")

    bodies = catalog.get_editor_property("bodies")
    print(f"\n--- Bodies ({len(bodies)}) ---")
    for body_id, body_def in bodies.items():
        mesh_path = get_obj_path(body_def.get_editor_property("mesh"))
        print(f"Body: {body_id}, Mesh: {mesh_path}")
        check_soft_path(mesh_path, f"Body.{body_id}")

    heads = catalog.get_editor_property("heads")
    print(f"\n--- Heads ({len(heads)}) ---")
    for head_id, head_def in heads.items():
        mesh_path = get_obj_path(head_def.get_editor_property("mesh"))
        print(f"Head: {head_id}, Mesh: {mesh_path}")
        check_soft_path(mesh_path, f"Head.{head_id}")

    parts = catalog.get_editor_property("parts")
    print(f"\n--- Parts ({len(parts)}) ---")
    for part_id, part_def in parts.items():
        mesh_path = get_obj_path(part_def.get_editor_property("mesh"))
        hide_tex = get_obj_path(part_def.get_editor_property("body_hide_texture"))
        slot = part_def.get_editor_property("slot")
        print(f"Part: {part_id}, Slot: {slot}, Mesh: {mesh_path}, HideTex: {hide_tex}")
        check_soft_path(mesh_path, f"Part.{part_id}.Mesh")
        if hide_tex and hide_tex != "None":
            check_soft_path(hide_tex, f"Part.{part_id}.HideTexture")

print("\n--- Presets ---")
for name, ppath in presets_to_check:
    preset = unreal.load_asset(ppath)
    if not preset:
        print(f"ERROR: Could not load preset {ppath}")
        continue
    recipe = preset.get_editor_property("recipe")
    body_id = recipe.get_editor_property("body_id")
    head_id = recipe.get_editor_property("head_id")
    part_list = recipe.get_editor_property("parts")
    print(f"\nPreset {name}: Use={preset.get_editor_property('use')}, Body={body_id}, Head={head_id}")
    for p in part_list:
        pid = p.get_editor_property("part_id")
        print(f"  Preset Part: {pid}")
        if catalog and pid not in parts:
            print(f"  ERROR: Part {pid} in preset {name} is NOT in catalog!")

print(f"\nTotal missing assets: {len(missing_assets)}")
for ctx, clean, raw in missing_assets:
    print(f"  Missing: [{ctx}] {clean}")

source_content_dir = "E:/Unreal Projects/Character Creator/Content"
target_content_dir = "E:/Unreal Projects/PortfolioTutorial/Project/Content"

print(f"\n--- Searching Source Project ({source_content_dir}) ---")
source_files = {}
if os.path.exists(source_content_dir):
    for root, dirs, files in os.walk(source_content_dir):
        for f in files:
            if f.endswith(".uasset"):
                base_name = f[:-7]
                rel_path = os.path.relpath(os.path.join(root, f), source_content_dir).replace("\\", "/")
                source_files[base_name] = rel_path
                source_files[f] = rel_path
    print(f"Indexed {len(source_files)} source assets.")
else:
    print(f"WARNING: Source content dir not found: {source_content_dir}")

target_files = {}
for root, dirs, files in os.walk(target_content_dir):
    for f in files:
        if f.endswith(".uasset"):
            base_name = f[:-7]
            rel_path = os.path.relpath(os.path.join(root, f), target_content_dir).replace("\\", "/")
            target_files[base_name] = rel_path
            target_files[f] = rel_path

print("\n--- Missing Asset Resolution ---")
resolution_report = []
for ctx, clean, raw in missing_assets:
    asset_name = clean.split("/")[-1]
    in_source = source_files.get(asset_name)
    in_target = target_files.get(asset_name)
    info = {
        "context": ctx,
        "clean_path": clean,
        "asset_name": asset_name,
        "found_in_target": in_target,
        "found_in_source": in_source,
    }
    resolution_report.append(info)
    print(f"Asset: {asset_name}")
    print(f"  Referenced as: {clean}")
    print(f"  In Target: {in_target}")
    print(f"  In Source: {in_source}")

out_report_path = "E:/Unreal Projects/PortfolioTutorial/Project/Saved/AppearanceAssetResolution.json"
with open(out_report_path, "w") as fp:
    json.dump(resolution_report, fp, indent=2)
print(f"\nWrote resolution report to {out_report_path}")
