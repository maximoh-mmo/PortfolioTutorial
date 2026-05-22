# 📘 **PLAYER SYSTEM DOCUMENT**  
**File:** `/Docs/Player/PlayerSystem.md`

---

# **Player System**

## **Purpose**
Provide top‑down ARPG controls and a UI‑driven PvP toggle that affects targeting and damage rules.

---

## **Responsibilities**
- Handle input  
- Click‑to‑move  
- Click‑to‑target  
- Ability activation  
- PvP toggle UI → PlayerState  
- Autoplay handoff  

---

## **PvP Toggle Flow**

### UI → PlayerController
Player clicks PvP toggle:

```
OnPvPToggleChanged(bool bEnabled)
```

### PlayerController → Server
```
Server_SetPvPEnabled(bEnabled)
```

### Server → PlayerState
```
bIsPvPEnabled = bEnabled;
```

### Replication → All Clients
`OnRep_PvPEnabled()` updates UI + targeting rules.

---

## **Targeting Integration**
- If PvP disabled → ignore player actors  
- If PvP enabled → include player actors  
- If `CurrentTarget` becomes invalid due to PvP toggle → auto‑select nearest NPC  

---

## **GAS Integration**
PlayerController routes ability input → ASC  
ASC checks PvP rules before applying damage.

---

## **UI Integration**
HUD displays:

- PvP ON/OFF  
- Color‑coded indicator  
- Optional tooltip  

---

## **Edge Cases**
- Player toggles PvP mid‑combat  
- Player AI must respect PvP rules  
- Player targeting a player when PvP is turned OFF  
