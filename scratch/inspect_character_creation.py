import unreal

output_path = r"E:\Unreal Projects\PortfolioTutorial\scratch\wbp_char_creation_dump.txt"

with open(output_path, "w", encoding="utf-8") as out:
    def log(msg):
        out.write(msg + "\n")
        print(msg)

    bp_path = "/Game/UI/Screens/WBP_CharacterCreation"
    bp = unreal.load_asset(bp_path)
    log(f"Loaded BP: {bp}")

    tree = unreal.load_object(None, f"{bp_path}.WBP_CharacterCreation:WidgetTree")
    log(f"WidgetTree: {tree}")

    if tree:
        all_widgets = []
        try:
            all_widgets = tree.get_editor_property("AllWidgets")
        except Exception as e:
            log(f"AllWidgets error: {e}")

        log(f"Total widgets: {len(all_widgets) if all_widgets else 0}")
        for w in (all_widgets or []):
            name = w.get_name()
            cls_name = w.get_class().get_name()
            parent = w.get_parent()
            parent_name = parent.get_name() if parent else "None"
            vis = str(w.get_visibility())
            is_focus = getattr(w, "b_is_focusable", None)
            slot_cls = w.slot.get_class().get_name() if hasattr(w, "slot") and w.slot else "None"
            log(f"WIDGET: {name} ({cls_name}) Parent={parent_name} Slot={slot_cls} Vis={vis} Focusable={is_focus}")

    # Inspect Blueprint graph nodes
    if bp:
        log("\n--- Blueprint Graphs ---")
        try:
            graphs = bp.get_editor_property("UberGraphPages")
            for g in (graphs or []):
                log(f"UberGraphPage: {g.get_name()}")
                nodes = g.get_editor_property("Nodes")
                for n in (nodes or []):
                    title = n.get_node_title(unreal.EdGraphNodeTitleType.FULL_TITLE)
                    cls_name = n.get_class().get_name()
                    log(f"  NODE: {n.get_name()} ({cls_name}) -> {title}")
        except Exception as e:
            log(f"UberGraphPages error: {e}")

        try:
            func_graphs = bp.get_editor_property("FunctionGraphs")
            for g in (func_graphs or []):
                log(f"FunctionGraph: {g.get_name()}")
                nodes = g.get_editor_property("Nodes")
                for n in (nodes or []):
                    title = n.get_node_title(unreal.EdGraphNodeTitleType.FULL_TITLE)
                    cls_name = n.get_class().get_name()
                    log(f"  NODE: {n.get_name()} ({cls_name}) -> {title}")
        except Exception as e:
            log(f"FunctionGraphs error: {e}")

log("Done dump!")
