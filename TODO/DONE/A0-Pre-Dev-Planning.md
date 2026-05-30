# 🧱 **PHASE 1 — Finalize High‑Level Direction**
### *Goal: Lock the vision, scope, and structure.*

### ✔ 1. Finalize the Series Overview  
- Confirm final demo description  
- Confirm technologies used  
- Confirm target audience  
- Confirm learning outcomes  

### ✔ 2. Finalize the Scope Overview  
- What’s in scope  
- What’s out of scope  
- Final deliverables  
- Success criteria  

### ✔ 3. Finalize the Episode List  
- Titles  
- Goals  
- Dependencies  
- Phase grouping  

**Done when:**  
You can explain the entire series in 2 minutes without hesitation.

---

# 🧱 **PHASE 2 — Architecture Planning**
### *Goal: Define the technical shape of the entire project.*

### ✔ 4. Complete the Architecture Overview  
- High‑level system diagram  
- Data flow between systems  
- Server vs client responsibilities  
- Steam integration flow  
- Ability system flow  
- AI behaviour flow  

### ✔ 5. Create System Documentation Templates  
(Already done — now use them.)

### ✔ 6. Write System Documents for Each Major System  
Recommended order:

1. Player System  
2. Targeting System  
3. Spawner System  
4. Pooling System  
5. NPC AI System  
6. StateTree System  
7. GAS System  
8. Player AI System  
9. Multiplayer System  
10. Steam Integration  
11. UI System  
12. Final Demo Loop  

Each system doc should include:

- Purpose  
- Responsibilities  
- Key classes  
- Key functions  
- Data flow  
- Replication rules  
- Edge cases  
- Testing checklist  

**Done when:**  
You can describe how every system works and how they interact without guessing.

---

# 🧱 **PHASE 3 — Tutorial Production Planning**
### *Goal: Prepare the entire teaching pipeline before coding.*

### ✔ 7. Create Episode Script Templates  
(Already done.)

### ✔ 8. Create Episode README Templates (for public repo)  
- Summary  
- Steps  
- Code snippets  
- Diagrams  
- Testing instructions  

### ✔ 9. Create Episode Folder Structure in Private Repo  
```
/Series/Episode01
/Series/Episode02
...
```

### ✔ 10. Draft Episode Scripts for the First 5 Episodes  
This ensures the early foundation is rock‑solid.

Episodes to script first:

1. Project Setup  
2. Top‑Down Camera  
3. Click‑to‑Move  
4. Click‑to‑Target  
5. Spawner  

**Done when:**  
You can record Episodes 1–5 tomorrow without improvising.

---

# 🧱 **PHASE 4 — Repo & Workflow Setup**
### *Goal: Prepare the private + public repo workflow.*

### ✔ 11. Create Private Repo Structure  
(You already have the structure — now create the folders.)

### ✔ 12. Create Public Repo Structure  
- `/Episode01`  
- `/Episode02`  
- …  
- `/FinalDemo` (optional)  

### ✔ 13. Define Branching Strategy  
- `main`  
- `dev`  
- `feature/*`  
- `episode/*`  
- `steam/*`  
- `server/*`  

### ✔ 14. Create Episode Export Workflow  
Document the steps:

1. Build feature in private repo  
2. Create `episode/*` branch  
3. Strip advanced features  
4. Export snapshot to `/Series/EpisodeXX`  
5. Push to public repo  

### ✔ 15. Create a “Public Release Checklist”  
- Clean code  
- Remove unused assets  
- Update README  
- Add diagrams  
- Tag release  

**Done when:**  
You can export an episode snapshot in under 10 minutes.

---

# 🧱 **PHASE 5 — Visual Documentation**
### *Goal: Create diagrams that will be used in episodes and internal docs.*

### ✔ 16. Create High‑Level Diagrams  
- Architecture diagram  
- Gameplay loop diagram  
- AI behaviour diagram  
- Ability system flow  
- Multiplayer authority diagram  
- Steam auth flow  

### ✔ 17. Create Episode‑Specific Diagrams (for early episodes)  
- Click‑to‑move flow  
- Targeting flow  
- Spawner lifecycle  
- Pooling lifecycle  

**Done when:**  
You have at least 6–8 diagrams ready for early episodes.

---

# 🧱 **PHASE 6 — Pre‑Production Review**
### *Goal: Validate the plan before writing code.*

### ✔ 18. Review All Documents for Consistency  
- Episode list matches architecture  
- Scope matches final demo  
- System docs match episode order  
- No contradictions  

### ✔ 19. Identify Risks  
Examples:  
- GAS complexity  
- Multiplayer replication pitfalls  
- Steam integration edge cases  
- StateTree learning curve  

### ✔ 20. Create a Risk Mitigation Plan  
- What to prototype early  
- What to simplify if needed  
- What to delay or cut  

### ✔ 21. Create a Production Timeline  
- Estimate time per episode  
- Estimate time for private demo  
- Estimate time for editing/recording  

**Done when:**  
You feel confident, not overwhelmed.

---

# 🧱 **PHASE 7 — Start Development**
### *Goal: Begin building the private demo.*

Only start coding when:

- The architecture is locked  
- The episode list is locked  
- The first 5 scripts are written  
- The repo workflow is ready  
- The diagrams are prepared  
- The scope is stable  

This ensures:

- No rewrites  
- No broken episodes  
- No wasted recording time  
- A clean, professional series  

---