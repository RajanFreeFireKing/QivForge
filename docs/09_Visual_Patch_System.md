# QivForge Doc 09 — Visual Patch System

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01 to 08 · **Audience:** AI coding agents

---

## 1. Purpose and locked decisions

Patches are touch-friendly node graphs for gameplay logic and animation. They are equal citizens with QivScript.

| Decision (LOCKED) | Detail |
|---|---|
| One runtime | A Logic Patch compiles to the same QVM bytecode as QivScript (Doc 08). No second interpreter |
| Two kinds | **Logic Patch** (events and actions) and **AnimGraph Patch** (visual editor for the `.qanim` state machine from Doc 07) |
| File format | `.qpatch` JSON, `formatVersion` integer, stable node and pin ids (UUIDs) |
| Mixing | An actor can have scripts and patches together. A patch can call script functions; a script can call patch **custom events** |
| Touch first | Everything is reachable with fingers; wiring needs no precision tools |

## 2. Graph model

- **Graph:** list of nodes, list of wires, list of variables, list of comment boxes, list of sub-graphs.
- **Node:** id, type id, position (x, y), size (auto), constants (values typed in the node), and pins.
- **Pin:** id, name, direction (in or out), kind (Exec or Data), data type, optional default value.
- **Wire:** from output pin to input pin. Exec wires carry control flow; data wires carry values.
- **Variables:** typed values scoped to the graph. `@export`-style flag makes a variable appear in the Inspector.
- **Sub-graph (function):** a reusable graph with inputs and outputs, callable as a node.
- **Comment box:** a titled colored rectangle that groups nodes visually (no runtime effect).

### Pin data types and colors

| Type | Pin color token |
|---|---|
| Exec | white |
| bool | red |
| int | cyan |
| float | green |
| string | magenta |
| Vec2, Vec3 | yellow |
| Quat | purple |
| Color | rainbow dot |
| Actor, Component | blue |
| Asset | orange |
| Any (wildcard, resolves on connect) | grey |

### Connection rules

- An output can feed many inputs; an input accepts only one wire.
- An Exec output may connect to only one Exec input (use a Sequence node to fan out).
- Implicit conversions shown by a small badge on the wire: int to float, float to int (truncate), bool to int, any to string. Other mismatches are rejected with a red flash and a message.
- Cycles in exec flow are allowed only through Loop nodes; data cycles are rejected.

## 3. Execution model

- **Event nodes** are entry points. When an event fires, execution follows Exec wires node by node until the chain ends.
- **Data nodes** (pure) are evaluated on demand when an Exec node reads their output; results are cached within one event execution.
- **Latent nodes** (Delay, Wait for Event, Stream Asset, Move To) suspend that chain without blocking the frame and resume later (compiled to coroutine bytecode).
- Execution order for multiple chains from the same event: the order listed in the node's **Sequence** node; for multiple event nodes of the same type in one graph, ordered by node creation.
- Infinite loop protection: same instruction budget as scripts (Doc 08 section 13.1).

## 4. Node catalog — Logic Patch

Category colors follow Doc 04 (Event red, Physics and Combat blue, Animation purple, Cloud and AI green). Flow, Math, Variables, and Utility use neutral tones.

### 4.1 Event nodes (red header)

| Node | Outputs | Fires |
|---|---|---|
| OnGameStart | Exec | Once at Play start (after Awake) |
| OnFrameTick | Exec, DeltaTime | Every frame |
| OnPhysicsStep | Exec, FixedDt | Every physics step |
| OnTouchButtonDown / Up / Held | Exec, ButtonName filter | HUD button events |
| OnJoystickMoved | Exec, Vec2 | Joystick changes |
| OnTriggerEnter / Exit | Exec, Other Actor | Trigger overlaps |
| OnCollision | Exec, Other, Point, Normal, Impulse | Collisions |
| OnDamageReceived | Exec, Amount, Source Actor | Health events |
| OnDeath | Exec, Killer | Health reaches 0 |
| OnAnimationEvent | Exec, Name, Payload | Doc 07 |
| OnTimer | Exec | Timer elapsed |
| OnCustomEvent(name) | Exec, Payload | Raised by Send Custom Event |
| OnDestroy | Exec | Actor destroyed |

### 4.2 Flow control

Branch (if), Sequence, Switch (int, string, enum), For Loop, For Each, While Loop, Do Once, Do N Times, Gate (open or close), FlipFlop, Delay (latent), Retriggerable Delay, Wait For Event (latent), Break, Return, Set Timer, Clear Timer.

### 4.3 Math and logic (pure)

Add, Subtract, Multiply, Divide, Modulo, Min, Max, Clamp, Lerp, Abs, Sign, Sin, Cos, Pow, Sqrt, Random Float, Random Int, Compare (equal, not equal, greater, less), AND, OR, NOT, Select, Map Range, Vector Make and Break, Vec3 Dot, Cross, Length, Normalize, Distance, Rotate Vector by Quat, Quat From Euler, Euler From Quat, Color Make and Lerp.

### 4.4 Variables and data

Get Variable, Set Variable, Get Exported Property, Set Property (Inspector path), List (Add, Remove, Get, Count, Contains, For Each), Map (Get, Set, Has), Make Literal nodes for each type.

### 4.5 Actor and scene

Get Self, Find Actor By Name, Find Actors With Tag, Spawn Prefab, Destroy Actor, Set Active, Set Parent, Get and Set Position, Rotation, Scale, Get Forward, Attach To Socket, Has Tag, Send Message, Send Custom Event, Load Scene.

### 4.6 Physics and vehicle (blue header)

| Node | Behavior |
|---|---|
| SetWheelTorque | Wheel index or all, torque |
| SetSteeringAngle | Wheel index or axle, angle |
| SetBrakeTorque | Wheel or all |
| AddImpulseForce | Actor, Vec3, mode (force, impulse, acceleration) |
| SetVelocity, GetVelocity | Rigid body |
| SetGravityScale | Actor or world |
| RaycastLine | Origin, direction, distance, layer mask. Outputs: Hit exec and Miss exec, point, normal, actor, surface type |
| SphereCast, OverlapSphere | Queries (Doc 06) |
| ApplyDamage | Target actor, amount, damage type |
| SpawnProjectile | Prefab, muzzle transform, speed |
| SetVehicleInput | Throttle, brake, steer, handbrake |
| GetVehicleState | Speed, RPM, gear, grounded |
| MoveCharacter | Direction vector, speed |
| Jump | Character |

### 4.7 Animation (purple header)

SetAnimFloat, SetAnimBool, SetAnimInt, TriggerAnim, SetLayerWeight, PlayClip (one-shot override), CrossfadeToState, GetCurrentState, GetNormalizedTime, SetLookAtTarget, EnableFootIK.

### 4.8 Audio, VFX, camera, UI

PlaySound (2D or 3D), StopSound, SetSoundPitch, SetBusVolume, PlayMusic, SpawnParticles, SetParticleRate, BurstParticles, SpawnImpact, ShakeCamera, SetCameraFollow, SetFov, ShowWidget, HideWidget, SetWidgetText, SetProgressBar, SetWidgetColor.

### 4.9 Cloud and AI (green header)

| Node | Behavior |
|---|---|
| StreamHFAsset | Latent. Asset path in the project repo, output model asset, Success and Failed execs; follows the storage mode (Local reads from disk) |
| SaveSceneCheckpoint | Saves a gameplay checkpoint to `SaveGame` (local), not a cloud commit |
| SpawnPrefab | Instantiates a prefab by asset |
| GenerateAIDialogue | Latent. Prompt, character persona, outputs text. Uses the user's selected AI provider only in the editor and in games that opt in; see Doc 12 and Doc 14 |
| IsOnline | Boolean |
| DownloadAsset | Latent download with progress output |

Cloud and AI nodes show a small cloud badge, and games that use them show a one-time consent dialog on first run in the exported app (Doc 14).

## 5. Node catalog — AnimGraph Patch

The AnimGraph editor draws the `.qanim` state machine (Doc 07) as nodes.

| Node | Header | Description |
|---|---|---|
| State: Clip | purple | Clip asset, speed, loop |
| State: Blend Tree | purple | Opens a nested 1D or 2D blend editor |
| State: Idle, Walk/Run Blend, Jump, Aim and Shoot, Die | purple | Presets that create a state with the standard clips |
| Entry | purple | Start state |
| Any State | purple | Source for global transitions |
| Transition (wire with a badge) | purple | Click the wire to edit conditions, exit time, duration |
| CrossfadeTransition | purple | Same as a transition with explicit duration field exposed on the node |
| Parameter nodes | grey | Float, Int, Bool, Trigger definitions |
| Layer node | purple | Mask, weight, blending |
| Condition editor | popover | Rows of: parameter, operator, value |

The Blueprint example "Idle to Walk to Run to Shoot with speed > 5.0" is built by dragging `Idle`, `Locomotion`, and `Aim and Shoot` states, drawing wires, and setting `MoveSpeed > 0.1` on the first transition. All data is saved in the `.qanim` file; the `.qpatch` stores node positions only.

## 6. Custom nodes (v1.0)

A script function marked `@node("Category/Name")` appears in the palette as a node, with pins generated from the function signature. Return values become data outputs; `async fn` becomes a latent node. This lets users and the AI extend the library without engine changes.

## 7. Editor behavior details (extends Doc 04 section 9)

| Feature | Detail |
|---|---|
| Palette search | Fuzzy match on name, category, and keywords; context-sensitive filtering when dragging from a pin (only compatible nodes) |
| Add node by dropping a wire on empty canvas | Opens a filtered search; selecting auto-connects |
| Move nodes | Drag; multi-select drag; snap to 16 px grid when snap is on |
| Delete | Select and tap Delete; wires removed with their nodes; Undo restores |
| Reroute node | A small dot to organize long wires |
| Collapse node | Hides unconnected pins |
| Comment boxes | Move child nodes together |
| Sub-graph creation | Select nodes, tap **Make Function**; the editor creates input and output pins from crossing wires |
| Find references | Highlights all uses of a variable or custom event |
| Minimap | Tap to jump |
| Copy and paste | Between patches and between projects using a clipboard JSON |
| Undo and redo | Per-graph command history of 200 steps |
| Validation | Live; errors listed in the compile chip with tap to focus the node |
| Debug in Play | Active wires animate; clicking a data pin shows the last value; breakpoint on a node pauses the game (v2) |
| Run Test | Executes the graph in an isolated mini scene with a stub actor |

## 8. Compilation of a Logic Patch

```
.qpatch JSON
  -> Graph validation (types, required pins, orphans, cycles)
  -> Topological ordering of pure nodes per exec chain
  -> IR generation (same IR as QivScript)
  -> Optimizer
  -> QVM bytecode with debug info (node id for each instruction)
```

- Each event node becomes a lifecycle method or an event handler; latent chains become coroutines.
- Node ids map to bytecode line markers so runtime errors point to the node and highlight it in red.
- Compile time target: under 50 ms for a graph of 200 nodes.
- Output cached in `.cache/` by content hash like scripts.

## 9. Variables, exports, and communication

- Exported graph variables appear in the Inspector under the PatchGraph component with types and optional ranges set in the variable panel.
- **Custom events:** `Send Custom Event(target, name, payload)` and `OnCustomEvent(name)`. A script receives them with `@event("name") fn handler(payload)`.
- **Calling a script from a patch:** the **Call Script Function** node lists public methods of scripts on the selected actor.
- **Reading patch variables from scripts:** `getPatch().getVariable<f32>("health")`.

## 10. File format `.qpatch`

```json
{
  "formatVersion": 1,
  "kind": "logic",
  "variables": [ {"name":"ammo","type":"int","default":30,"exported":true} ],
  "nodes": [
    {"id":"n1","type":"event.OnTouchButtonDown","pos":[0,0],"constants":{"button":"FireButton"}},
    {"id":"n2","type":"physics.RaycastLine","pos":[260,0],"constants":{"distance":300.0}},
    {"id":"n3","type":"combat.ApplyDamage","pos":[560,-40],"constants":{"amount":35.0}}
  ],
  "wires": [
    {"from":"n1.exec","to":"n2.exec"},
    {"from":"n2.hit","to":"n3.exec"},
    {"from":"n2.actor","to":"n3.target"}
  ],
  "comments": [ {"title":"Fire","rect":[-20,-60,700,200],"colorToken":"nodeEvent"} ],
  "subgraphs": []
}
```

Rules: unknown node types load as **Missing Node** placeholders (red) so no data is lost; ids never change on save; arrays are sorted by id for clean diffs in Hugging Face commits.

## 11. Templates (included graphs)

| Template | Contents |
|---|---|
| Fire Weapon | Button down, ammo check, raycast, damage, impact VFX, sound, recoil |
| Pickup | Trigger enter on Player, add item, play sound, destroy |
| Checkpoint | Trigger enter, save checkpoint, show message |
| Lap Timer | Start line trigger, timer variable, HUD text |
| Enemy Patrol and Chase | Timer, random patrol point, chase when player in range |
| Vehicle Input | Joystick and pedals to vehicle input |
| Day-Night Toggle | Timer rotating the sun (v2) |
| Door Open | Trigger plus animation trigger |

## 12. Conversion mapping (used by Doc 08 section 17)

| QivScript | Patch node |
|---|---|
| `onTick(dt)` | OnFrameTick |
| `onPhysicsStep(dt)` | OnPhysicsStep |
| `if / else` | Branch |
| `for (i in 0..n)` | For Loop |
| `a + b`, `a * b` | Add, Multiply |
| `PhysicsWorld.raycast(...)` | RaycastLine |
| `animator.setFloat("x", v)` | SetAnimFloat |
| `actor.sendMessage(n, v)` | Send Message |
| `await Time.wait(s)` | Delay |
| exported field | Exported variable |
| Anything else | Call Script Function node (original code preserved in a sidecar script) |

Round trip is stable: Script to Patch to Script keeps behavior; formatting may differ.

## 13. AI integration

The AI tool `engine.createVisualPatch(actorName, nodesJson)` accepts the `.qpatch` JSON schema in section 10. The engine validates it through the same validator (section 8), rejects invalid graphs with a list of errors that the AI sees, and shows the user a visual preview with **Accept or Reject** (Doc 12). The schema, node catalog, and pin types are published as a machine-readable `patch_catalog.json` generated from the binding files so the AI never invents node names.

## 14. Performance rules

- A graph executes like script bytecode; budget per graph per frame: 0.1 ms for 50 active nodes.
- Pure node results are cached per event execution to avoid repeated work.
- Maximum 2,000 nodes per graph; the editor warns at 500 and suggests sub-graphs.
- The editor renders only visible nodes (culling) and simplifies wires beyond 300 on screen.

## 15. Errors and diagnostics

| Code | Condition | Behavior |
|---|---|---|
| QP0001 | Required input pin not connected and no default | Node outlined red; graph does not compile |
| QP0002 | Type mismatch on connection | Wire rejected |
| QP0003 | Data cycle | Wire rejected |
| QP0004 | Missing node type (loaded from file) | Red placeholder node; graph compiles with that branch disabled |
| QP0005 | Duplicate event node where only one is allowed | Warning |
| QP-RUN-001 | Runtime error in a node | Node flashes red in Play, chain stops, other chains continue |

## 16. Test plan (feeds Doc 15)

- Validator: 100 invalid graphs with expected error codes.
- Compiler: golden bytecode for the 8 templates.
- Round trip: 50 scripts to patches to scripts compared by running a deterministic scenario.
- UI: automated gesture tests for dragging nodes, wiring, deleting, undo, and box-select at 25% and 300% zoom.
- Performance: a 500-node graph scrolls at 60 fps on the reference device.
- Fuzz: random JSON must never crash the loader.

## 17. Acceptance criteria

1. A user can build Fire Weapon from scratch in under 3 minutes using only touch: Button Down to Raycast to Apply Damage.
2. The Fire Weapon template and an equivalent script produce identical hit results in the same test scene.
3. A graph with a missing node type opens, shows a red placeholder, and saves without losing that node.
4. Wire dragging works with two fingers (one drags a wire while the other pans) without losing the wire.
5. A 500-node graph pans and zooms at 60 fps.
6. The AI can create a valid patch from a text request and every node name it uses exists in `patch_catalog.json`.
7. Runtime errors highlight the exact node in red in the editor.
8. Script to Patch conversion of the TPS reference script converts at least 80% of nodes without Script Call fallbacks and plays identically.
