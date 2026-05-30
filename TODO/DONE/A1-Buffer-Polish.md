# 🗓 THU 4 JUN — A1 Buffer / Overflow

## ~~If Behind~~ — Moot (A1 all 38/38)
- [x] ~~Catch up any incomplete A1 tasks from previous days~~  
- [x] ~~Priority: A1.4 (Targeting Component) — most edge cases~~  
- [x] ~~Re-run all A1 tests to ensure nothing regressed~~

## If On Schedule — Polish
- [x] ~~Verify every box in A1 sections of `TODO/Private_Demo_Checklist.md`~~ — all [x]
- [x] ~~PvP toggle mid-combat — rapid toggling while targeting~~ — no exploit, disregarded
- [x] ~~Click/tap enemy then quickly click/tap ground — clean transition~~ — code guarantees clean transition: SetTarget on enemy, ClearTarget+MoveTo on ground
- [x] ~~Gamepad R-Stick cursor at viewport edges — clamp correctly~~ — verified in 05-29
- [x] ~~Switch input methods mid-pathfinding (joystick interrupts move-to)~~ — verified in 05-29

## Remaining
- [x] Add future module dependencies to `Onset.Build.cs`: `"GameplayAbilities"`, `"GameplayTags"`, `"GameplayTasks"`
- [x] Test mobile viewport (Editor → Preview Device → Tablet/Phone)
- [x] Verify virtual joystick + tap-to-move both work on touch device
- [x] Verify gamepad input via Editor → Preferences → Gamepad Simulation
- [x] Clean up temporary test actors from the level
- [x] Commit progress to `dev` branch

## Deferred
- [~] Camera collision in tight corridors, under overhangs — **Deferred.** No level geometry to test with; revisit when level design begins.

## ✅ Done
- [x] ~~Update A1 progress tracking in `TODO/Private_Demo_Checklist.md`~~ — already at 38/38 (100%)
