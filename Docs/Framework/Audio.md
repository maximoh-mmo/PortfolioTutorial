# MHAudio Architecture and Integration

`MHAudio` provides a provider-neutral audio abstraction layer for gameplay systems, UI, abilities, and animations without hardcoding engine-specific asset types (`USoundBase*`) or vendor middleware APIs (Audiokinetic Wwise, Firelight FMOD).

---

## 1. Core Philosophy

Gameplay code, animation montages, and editor tools must never directly bind to concrete audio middleware. All sound events, dynamic music transitions, and real-time parameters are issued against abstract contracts:

```
+-------------------------------------------------------------+
|                        Gameplay Code                        |
|   (Abilities, Enemies, Doors, AnimNotifies, Settings UI)    |
+-------------------------------------------------------------+
                              |
                              | Speaks to UMHAudioSubsystem
                              v
+-------------------------------------------------------------+
|                           MHAudio                           |
|  - IMHAudioProvider (Interface)                             |
|  - UMHAudioCatalog (DataAsset Mapping)                      |
|  - UMHAudioSubsystem (GameInstance Subsystem)               |
|  - UMHAnimNotify_AudioEvent (Montage Animation Notify)      |
+-------------------------------------------------------------+
                              |
        +---------------------+---------------------+
        |                                           |
        v                                           v
+-------------------------+             +-------------------------+
| FMHUnrealAudioProvider  |             | FMHWwiseProvider (Drop-in|
| (Native UE / MetaSound) |             |  / FMHFMODProvider)     |
+-------------------------+             +-------------------------+
```

---

## 2. Core Concepts

### Events
Fire-and-forget 2D or 3D spatialized audio triggers, identified by `FGameplayTag` (e.g. `Audio.Event.Combat.FireballCast`, `Audio.Event.Door.Open`).
* `PostEvent(EventTag, Location, Rotation)`
* `PostEventAttached(EventTag, AttachToComponent, SocketName)`

### States & Dynamic Music
Discrete states governing musical and environmental layers:
* `SetState(FName("CombatState"), FName("Combat"))` $\rightarrow$ Triggers combat music stems.
* `SetState(FName("CombatState"), FName("Boss"))` $\rightarrow$ Triggers boss music layer.
* `SetState(FName("CombatState"), FName("Exploration"))` $\rightarrow$ Fades back to ambient theme.

### RTPCs (Real-Time Parameter Controls)
Continuous floating-point parameters (0.0 to 1.0 or unbounded) driving dynamic audio DSP:
* `PlayerHealthPercent`: Ducks music volume and applies muffled low-pass filtering when player health drops below 25%.

### Buses
Normalized submix channels (0.0 to 1.0) for game settings menus:
* `EMHAudioBus::Master`
* `EMHAudioBus::Music`
* `EMHAudioBus::SFX`
* `EMHAudioBus::Ambience`
* `EMHAudioBus::Voice`
* `EMHAudioBus::UI`

---

## 3. Catalog Configuration (`UMHAudioCatalog`)

`UMHAudioCatalog` is an authored `UDataAsset` in Content (e.g. `DA_DefaultAudioCatalog`) mapping tags to native assets:
* `Events`: Maps `FGameplayTag` to `USoundBase*`, bus channel, volume multiplier, pitch multiplier, and attenuation assets.
* `MusicTracks`: Maps `FGameplayTag` to background music assets with loop flags and default crossfade durations.
* `Bus Overrides`: Optional sound class overrides for native Unreal Engine bus attenuation.

---

## 4. Swapping Providers (Wwise / FMOD)

To add Wwise or FMOD down the line:
1. Implement `IMHAudioProvider` in a dedicated bridge module (e.g. `FMHWwiseAudioProvider`).
2. Map `PostEvent` to `AkGameplayStatics::PostEvent`.
3. Map `SetState` to `AkGameplayStatics::SetState`.
4. Map `SetRTPC` to `AkGameplayStatics::SetRTPCValue`.
5. Switch the active provider instantiation in `UMHAudioSubsystem::Initialize()`.
6. **Zero changes are required in gameplay, abilities, or UI blueprints.**
