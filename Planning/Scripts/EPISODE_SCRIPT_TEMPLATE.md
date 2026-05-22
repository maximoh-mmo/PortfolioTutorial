# 🎬 **EPISODE SCRIPT TEMPLATE**  
*(Save as `/Planning/Scripts/EpisodeXX_Title.md`)*

---

# **Episode X — Title**

## **Episode Goal**
*A single sentence describing what the viewer will achieve by the end of the episode.*

Example:  
“Implement click‑to‑move using raycasts and MoveToLocation.”

---

# **Context & Dependencies**
- What episodes must come before this one  
- What systems must already exist  
- What the viewer should already understand  

Example:  
- Requires Episode 2 (Top‑Down Camera)  
- Player character and controller already created  

---

# **High‑Level Summary**
*A short paragraph describing the purpose of the episode and why it matters.*

Example:  
“In this episode we implement point‑and‑click movement, which forms the foundation of our top‑down ARPG control scheme.”

---

# **Key Concepts Introduced**
List the new ideas the viewer will learn.

Example:
- Mouse raycasting  
- Hit results  
- MoveToLocation  
- Click feedback  

---

# **Technical Breakdown**
*A clear, step‑by‑step explanation of the system you’re implementing.*

Use sections like:

### **1. Create/Modify C++ Classes**
- Class name  
- Header changes  
- CPP changes  
- Why these changes matter  

### **2. Blueprint Setup (if any)**
- Components  
- Variables  
- Event graph logic  

### **3. Editor Setup**
- Input mappings  
- Project settings  
- Components in the scene  

### **4. Testing Steps**
- How to verify the system works  
- Debug commands  
- Expected behaviour  

---

# **Code Snippets**
Include only the code relevant to the episode.

Use fenced blocks:

```cpp
// Example snippet
void AMyPlayerController::HandleClickMove()
{
    // ...
}
```

---

# **Diagrams (Optional)**
If helpful, include:

- Flowcharts  
- State diagrams  
- Data flow  
- Behaviour logic  

Example:

```
Click → Raycast → Hit Location → MoveToLocation
```

---

# **Common Pitfalls**
List mistakes viewers often make.

Example:
- Forgetting to enable “Show Mouse Cursor”  
- Using the wrong collision channel  
- Not enabling “Use Controller Rotation”  

---

# **Episode Checklist**
A quick list to confirm everything is done.

Example:
- [ ] Raycast from mouse cursor  
- [ ] MoveToLocation implemented  
- [ ] Click feedback added  
- [ ] Tested in PIE  

---

# **Public Repo Notes**
Instructions for preparing the episode snapshot.

Example:
- Remove advanced features not yet introduced  
- Clean up unused assets  
- Commit only Episode 3 changes  
- Update `/Series/Episode03/README.md`  

---

# **Recording Script (Optional)**
A short, conversational outline for the video.

Example:

**Intro:**  
“Welcome back! Today we’re adding click‑to‑move, the core of our top‑down control scheme.”

**Body:**  
- Explain raycasting  
- Show code  
- Test in editor  

**Outro:**  
“In the next episode, we’ll add click‑to‑target so we can attack enemies.”

---

# **Next Episode Preview**
A single sentence.

Example:  
“Next time, we’ll implement click‑to‑target selection.”

---