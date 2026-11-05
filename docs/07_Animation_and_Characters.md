# QivForge Doc 07 — Animation and Characters

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01, 02, 03, 05, 06 · **Audience:** AI coding agents

---

## 1. Scope

This document defines skeletal animation, the animation state machine (AnimGraph), blending, layers, events, IK, ragdoll hand-off, retargeting, and the character and weapon animation workflow. The Visual AnimGraph editor UI is in Doc 09; this document defines the runtime it edits.

## 2. Data model

| Object | Description |
|---|---|
| **Skeleton** | Bone hierarchy with bind poses and inverse bind matrices, taken from the GLB skin. Max 128 bones (Doc 05 section 9) |
| **AnimationClip** | Per-bone translation, rotation, scale tracks plus optional blend shape tracks and events. Stored as compressed curves (keyframe reduction with 0.001 error tolerance) |
| **Avatar** | A map from skeleton bones to humanoid roles (Hips, Spine, Chest, Neck, Head, L/R Shoulder, UpperArm, LowerArm, Hand, UpperLeg, LowerLeg, Foot, Toes). Required for retargeting, masks, IK, and ragdoll |
| **AvatarMask** | A set of bones included in a layer, with presets Full Body, Upper Body, Lower Body, Arms, Head |
| **AnimGraph** | Parameters, layers, and state machines that produce a pose each frame |
| **Animator (component)** | Holds the AnimGraph instance, parameter values, and the playback state |

## 3. Import rules

- Source: GLB/glTF 2.0 animations. FBX is not supported in v1.0 (convert to GLB beforehand).
- Clip settings (per clip, in the `.meta.json`): `loop`, `loopPose`, `speed`, `startFrame`, `endFrame`, `rootMotion` (None, XZ, Full), `additive` (bool and reference pose), `mirror` (bool).
- Multiple takes in one GLB become separate clips named by their animation name.
- On import the tool reports: bone count, clip count, duration, and warnings (too many bones, scale animation present, non-uniform scale).
- Sampling rate: clips are resampled to 30 Hz at bake; playback interpolates with linear for translation and scale, spherical (nlerp) for rotation.

## 4. Runtime pipeline per frame

1. Update parameters (from scripts, patches, physics, input).
2. Evaluate each layer's state machine: check transitions, advance state time, update blend weights.
3. Sample clips and blend trees into per-layer poses.
4. Combine layers (override or additive) using masks and weights.
5. Apply root motion (extract and send to the character controller or transform).
6. Apply IK (foot placement, look-at, aim) if enabled.
7. Apply physics overrides (ragdoll blend, hit reactions).
8. Compute final bone matrices and send to the renderer for GPU skinning.

Animation runs on Engine main for scenes with up to 8 animated characters in view; additional ones are evaluated on job workers. Characters out of view or beyond `animationCullDistance` (default 60 m) update at a lower rate (every 2nd or 4th frame) or are paused, controlled by a LOD setting.

## 5. Parameters

| Type | Description | Script API |
|---|---|---|
| Float | Continuous value such as `MoveSpeed` | `setFloat(name, v)`, `getFloat(name)` |
| Int | Discrete value such as `WeaponType` | `setInt`, `getInt` |
| Bool | Flag such as `IsAiming` | `setBool`, `getBool` |
| Trigger | One-shot flag consumed by a transition, such as `ShootRecoil` | `trigger(name)`, `resetTrigger(name)` |

Parameter changes are applied at the start of the animation update. Unknown parameter names log `QF-ANIM-001` once and are ignored.

## 6. State machine

### 6.1 States

| State type | Behavior |
|---|---|
| **Clip state** | Plays one clip, with speed multiplier and optional speed parameter |
| **Blend tree state** | Contains a blend tree (section 7) |
| **Sub-state machine** | Group of states with entry and exit |
| **Any state** | Special node whose transitions can fire from any current state (hit reaction, death) |
| **Entry** and **Exit** | Start and end markers |

### 6.2 Transitions

| Field | Meaning |
|---|---|
| `from`, `to` | States |
| `conditions` | List of rules, all must be true. Rule: parameter, operator (`>`, `<`, `>=`, `<=`, `==`, `!=`), value; or Bool true/false; or Trigger |
| `hasExitTime` | If true, only fires after the source state reaches `exitTime` (0 to 1 of its duration) |
| `duration` | Crossfade time in seconds (default 0.15, range 0 to 1) |
| `offset` | Start time offset in the target state |
| `interruptionSource` | None, Source, Destination, SourceThenDestination — which transitions may interrupt a running crossfade |
| `priority` | Lower number is checked first |

Crossfade is a smooth blend (smoothstep curve) of the pose from the source to the destination over `duration`. When interrupted, the engine blends from the current blended pose to the new target to avoid popping.

### 6.3 Default humanoid graph (ships in the TPS template)

States: `Idle`, `Locomotion` (blend tree), `Jump` (start, loop, land), `Crouch Locomotion`, `Aim Locomotion`, `Reload`, `Hit`, `Death`.

| Transition | Conditions |
|---|---|
| Idle to Locomotion | `MoveSpeed > 0.1` |
| Locomotion to Idle | `MoveSpeed < 0.05` |
| Any to Jump | trigger `Jump` |
| Jump to Locomotion or Idle | `IsGrounded == true` with exit time 0.9 |
| Any to Hit | trigger `Hit` (shortened duration 0.05) |
| Any to Death | trigger `Die` (no interruption) |
| Locomotion to Aim Locomotion | `IsAiming == true` |

The Blueprint v1 and v2 condition `speed > 5.0` is expressed as `MoveSpeed > 5.0` with the speed in meters per second when a parameter is bound to real velocity, or `MoveSpeed > 0.8` when it is the normalized joystick length. Each project must document which one it uses in the `MoveSpeed` parameter description.

## 7. Blend trees

| Type | Inputs | Use |
|---|---|---|
| 1D | one float | Idle to Walk to Run by speed |
| 2D Simple Directional | two floats | Strafe: forward speed and sideways speed |
| 2D Freeform Cartesian | two floats | Free layout of clips on a grid |
| Direct | weights per clip | Facial or additive layers |
| Additive | base plus additive clip | Lean, breathing |

Rules: clip speeds are synchronized with **foot-phase normalized time** so walk and run steps stay aligned (each clip has a flag `syncGroup` and a normalized loop phase). Thresholds are sorted ascending and must be unique; the editor enforces this.

## 8. Layers and masks

- Layer 0 is the **Base layer** (full body).
- Additional layers have: `mask`, `weight` (0 to 1, scriptable), `blending` (Override or Additive), `syncWithLayer` (optional), `ikPass` (bool).
- Typical TPS setup: Base layer = locomotion (legs and body); Layer 1 = upper-body aim and shoot with an Upper Body mask, Override; Layer 2 = additive hit flinch.
- Script API: `setLayerWeight(layerIndex, weight)`, `getLayerWeight`.

## 9. Animation events

- Events are markers on a clip with a time (normalized or seconds), a name, and optional float, int, string payload.
- At runtime, when a state plays past an event, the engine fires `onAnimationEvent(name, payload)` to the actor's scripts and patches.
- Standard events used by templates: `Footstep_L`, `Footstep_R` (play surface-dependent footstep sound), `Fire` (spawn muzzle flash and raycast), `ReloadDone`, `JumpLand`, `ShowWeapon`, `HideWeapon`, `DeathFinished`.
- Events inside a blend tree fire only from clips with weight above 0.5 to avoid doubled footsteps.

## 10. Root motion

| Mode | Behavior |
|---|---|
| None | Clip motion is in place; the controller moves the character |
| XZ | Horizontal root movement drives the character; vertical comes from physics |
| Full | Root drives position and rotation (used for vault, climb, melee takedown) |

With root motion on, the animator outputs a delta each frame to the `CharacterController.move()` call so collisions still apply.

## 11. Inverse kinematics (tier 1 for foot IK, tier 2 for the rest)

| IK | Behavior | Tier |
|---|---|---|
| Foot IK | Raycast down from each foot; adjust foot height and rotate to ground normal; lower pelvis if needed (max 0.35 m) | 1 |
| Look-at IK | Head and spine rotate toward a target with weights and angle limits | 1 |
| Aim IK | Upper body and weapon-holding arm align to the aim direction (camera ray) | 1 |
| Hand IK | Left hand snaps to weapon foregrip or ladder rung | 2 |
| Full body | Grab and carry | 2 |

IK is evaluated after layers. It is skipped for characters beyond 20 m or when the quality governor is at Low.

## 12. Ragdoll and physical reaction

- On `Die` (or explosion), the Animator hands control to the ragdoll (Doc 06 section 12): copy current animated pose and velocities into the ragdoll bodies, disable animation, enable physics.
- **Hit reactions:** small hits play the additive `Hit` layer; large impacts (impulse above a threshold) trigger a short partial ragdoll with a blend back in 0.8 seconds (tier 2).
- **Get up:** after ragdoll is at rest, play a get-up clip matching face-up or face-down (tier 2).

## 13. Retargeting (tier 2)

- Both skeletons need an Avatar. Retargeting maps rotations by humanoid role using the T-pose difference, and scales translation by the hip height ratio.
- Retargeted clips are baked and cached so runtime cost is zero.
- The editor shows a side-by-side preview before accepting.

## 14. Facial animation and blend shapes (tier 2)

Meshes may have up to 8 active morph targets (Doc 05 section 9). A `FaceController` component maps phoneme and emotion names to targets. Lip sync from audio is out of scope for v1.0.

## 15. Characters

### 15.1 CharacterPawn base class

| Member | Description |
|---|---|
| `moveCharacter(Vec3 delta)` | Moves with collisions (calls the controller) |
| `setLookDirection(Vec3)` | Rotates the character smoothly |
| `getAnimator()` | Access to parameters |
| `enterVehicle(v)`, `exitVehicle()` | See Doc 06 |
| `health` | Health component reference |
| Events | `onSpawn`, `onDamaged`, `onDied`, `onLanded` |

### 15.2 Standard character setup (editor wizard **Make Character**)

1. Pick a rigged GLB.
2. The tool auto-detects the humanoid Avatar and shows a bone map for correction.
3. Adds components: `SkinnedMesh`, `Animator` with the default humanoid graph, `CharacterController` capsule fitted to the mesh, `Health`, an optional `TPSPlayer` or `AIController` script, and hitboxes.
4. Assigns clips from the built-in animation pack by name matching (Idle, Walk, Run, Jump, Crouch, Aim, Shoot, Reload, Hit, Death); missing clips are listed for the user to assign.

### 15.3 Built-in animation pack

Original or CC0 clips only (Doc 14): idle, walk, run, sprint, strafe 4 directions, jump start, jump loop, land, crouch idle, crouch walk, aim idle, aim walk, shoot (rifle), reload, hit front, hit back, death 2 variants, drive sitting (tractor), enter and exit vehicle.

### 15.4 Weapon animation workflow

- A weapon is an actor attached to a hand bone socket (`RightHandSocket`) with offsets saved per weapon.
- Fire sequence in the standard TPS script: `trigger("ShootRecoil")`, spawn muzzle VFX from socket `Muzzle`, perform raycast (Doc 06 section 11), play fire sound, apply camera recoil (Doc 05 camera shake).
- Reload: `trigger("Reload")`; ammo restored when event `ReloadDone` fires, not when the key is pressed.

### 15.5 Vehicle driving animation

Driver character in a vehicle seat plays a sitting pose; steering wheel bone and hands follow a `steering` parameter (-1 to 1) through a 1D blend tree; head look-at follows camera.

## 16. AI character animation

`AIController` writes the same parameters as the player (`MoveSpeed`, `IsAiming`, triggers), so enemies reuse the same graph. Navmesh agent velocity maps to `MoveSpeed`; facing uses `setLookDirection`.

## 17. Performance rules

- Budget: under 1.2 ms total animation time for 8 visible characters on the reference device.
- Clips are stored compressed in memory, decoded per sample; maximum 64 MB of animation data resident (LRU eviction beyond that).
- Do not allocate during animation update.
- Characters beyond the cull distance with `alwaysAnimate` false do not evaluate.

## 18. Error handling

| Code | Condition | Behavior |
|---|---|---|
| QF-ANIM-001 | Unknown parameter | Ignore and warn once |
| QF-ANIM-002 | Clip missing in state | Play bind pose for the state, show warning icon in the graph |
| QF-ANIM-003 | Bone count above limit | Import warning; GPU skinning disabled for extra bones |
| QF-ANIM-004 | Transition loop with zero duration and no condition | Editor refuses to save the graph |
| QF-ANIM-005 | Avatar incomplete | Humanoid-only features disabled with a notice |

## 19. File format: `.qanim` (AnimGraph asset)

```json
{
  "formatVersion": 1,
  "parameters": [ {"name":"MoveSpeed","type":"float","default":0.0} ],
  "layers": [ {
    "name":"Base","weight":1.0,"blending":"override","mask":null,
    "states":[ {"id":"idle","type":"clip","clip":"uuid","speed":1.0} ],
    "transitions":[ {"from":"idle","to":"loco","conditions":[{"param":"MoveSpeed","op":">","value":0.1}],
                     "hasExitTime":false,"duration":0.15} ],
    "entry":"idle"
  } ]
}
```

The Visual AnimGraph editor reads and writes this same format; the `.qpatch` file for an AnimGraph patch references the `.qanim` asset (Doc 09).

## 20. Acceptance criteria

1. The default humanoid graph blends from idle to run without foot sliding at speeds from 0 to 6.5 m/s in a recorded test.
2. A crossfade interrupted mid-way never pops (max bone rotation delta between frames below 15 degrees).
3. The upper-body shoot layer plays while legs keep running, verified with the mask.
4. Footstep events fire exactly once per step across a blend of walk and run.
5. Death hands control to the ragdoll within one frame and the ragdoll settles within 5 seconds.
6. 8 characters animate at under 1.2 ms total in the reference scene.
7. Foot IK keeps feet on a 30 degree slope without penetration above 1 cm.
8. Changing a parameter from a script, a patch, and the Inspector produces identical results.
