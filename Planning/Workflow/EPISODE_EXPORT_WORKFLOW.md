# 📦 **Episode Export Workflow**

## **Purpose**
Create a clean, self-contained episode snapshot from the private development repo and stage it for the public repo.

See the [Branching Strategy](BRANCHING_STRATEGY.md) for branch conventions and the [Episode List](../Outlines/Episode_List.md) for episode contents.

---

## **Prerequisites**

- All features for this episode are complete and merged to `dev`
- The episode script is drafted (see [Episode Script Template](../Scripts/EPISODE_SCRIPT_TEMPLATE.md))
- The episode README template is ready (see [Episode README Template](../Templates/EPISODE_README_TEMPLATE.md))

---

## **Step-by-Step**

### **1. Create the Episode Branch**
```powershell
git checkout dev
git checkout -b episode/XX
```

### **2. Strip Advanced Features**
Remove systems and code not yet introduced in this episode:

- Stripped content depends on the episode — earlier episodes remove more
- Common items to remove:
  - C++ classes for systems not yet covered
  - Config settings for unreleased features (e.g., Steam, multiplayer)
  - Unused assets, maps, blueprints
  - Debug tooling not yet introduced
  - Anything that would spoil a future episode

Refer to the [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md) to identify which systems belong at this point in the series.

### **3. Verify the Snapshot**
- Open the project — no missing references or broken blueprints
- Compile in the target Unreal Engine version
- Play through the episode's feature set — nothing more, nothing less

### **4. Export to Series Directory**
```powershell
# Copy the cleaned project to the episode export folder
Copy-Item -Path "Project/" -Destination "Series/EpisodeXX/" -Recurse -Force
```

### **5. Add Episode README**
Place the episode README (from the [Episode README Template](../Templates/EPISODE_README_TEMPLATE.md)) at:
```
Series/EpisodeXX/README.md
```

### **6. Final Checks**
- [ ] Episode branch contains only the intended changes
- [ ] No references to future systems or episodes
- [ ] Episode README is accurate
- [ ] Project compiles and runs in PIE
- [ ] `.gitkeep` files are present in empty directories

### **7. Push**
```powershell
git push origin episode/XX
```

---

## **Future Automation**

A dedicated export tool will live at `Tools/EpisodeExporter/` once implemented. The tool will automate stripping, copying, and README generation.

---

## **Related Documents**
- [Branching Strategy](BRANCHING_STRATEGY.md) — branch naming and merge flow
- [Episode Script Template](../Scripts/EPISODE_SCRIPT_TEMPLATE.md) — episode content outline
- [Episode README Template](../Templates/EPISODE_README_TEMPLATE.md) — public-facing episode summary
- [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md) — system dependency map
