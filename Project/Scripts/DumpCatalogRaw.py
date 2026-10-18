import unreal
import json

catalog = unreal.load_asset("/Game/OnsetCharacterCreation/Catalog/DA_OnsetAppearanceCatalog")
parts = catalog.get_editor_property("parts")

data = {}
for pid, pdef in parts.items():
    mesh_soft = pdef.get_editor_property("mesh")
    hide_soft = pdef.get_editor_property("body_hide_texture")
    
    mesh_path = mesh_soft.to_soft_object_path().export_text() if hasattr(mesh_soft, "to_soft_object_path") else str(mesh_soft)
    hide_path = hide_soft.to_soft_object_path().export_text() if hasattr(hide_soft, "to_soft_object_path") else str(hide_soft)
    
    data[str(pid)] = {
        "slot": str(pdef.get_editor_property("slot")),
        "mesh": mesh_path,
        "hide_texture": hide_path,
        "attach_to_head": bool(pdef.get_editor_property("attach_to_head")),
        "share_body_pose": bool(pdef.get_editor_property("share_body_pose")),
        "hair": bool(pdef.get_editor_property("hair")),
        "hide_hair": bool(pdef.get_editor_property("hide_hair")),
        "socket": str(pdef.get_editor_property("socket")),
        "compatible_bodies": [str(b) for b in pdef.get_editor_property("compatible_bodies")],
    }

with open("E:/Unreal Projects/PortfolioTutorial/Project/Saved/CatalogPartsDump.json", "w") as f:
    json.dump(data, f, indent=2)

print("Dumped", len(data), "parts to Saved/CatalogPartsDump.json")
