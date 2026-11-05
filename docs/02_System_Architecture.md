# QivForge Doc 02 — System Architecture

**Series:** QivForge Engineering Spec · **Depends on:** Doc 01 (all LOCKED decisions apply) · **Audience:** AI coding agents

---

## 1. Purpose

This document defines the layers, modules, threads, data flow, file layout, and build system of QivForge. It is the blueprint every other document plugs into. If an agent is unsure where code belongs, the answer is in this document.

## 2. Layer diagram

```
+--------------------------------------------------------------+
|  LAYER 5  Kotlin App Shell (Jetpack Compose)                 |
|  Project Hub, Editor panels, Code Editor, Settings, Theme    |
+--------------------------------------------------------------+
|  LAYER 4  Bridge (JNI + shared-memory ring buffer)           |
|  Commands down, events and stats up                          |
+--------------------------------------------------------------+
|  LAYER 3  Engine Services (C++20)                            |
|  Scene, ECS, Assets, Streaming, Scripting VM, Patches, Audio |
+--------------------------------------------------------------+
|  LAYER 2  Subsystems (C++20)                                 |
|  Renderer (Filament), Physics (Jolt), Animation, Input       |
+--------------------------------------------------------------+
|  LAYER 1  Platform (NDK)                                     |
|  Vulkan/GLES, Oboe, Android Surface, Sensors, File access    |
+--------------------------------------------------------------+
```

**Rule of dependency:** a layer may call only the layer directly below it or its own peers. Layer 2 never calls Layer 3. Layer 3 never calls Kotlin directly; it emits events through the bridge.

## 3. Repository layout

```
qivforge/
  app/                    Kotlin shell (Gradle module)
    src/main/java/.../ui/        Compose screens
    src/main/java/.../theme/     Dynamic theme engine
    src/main/java/.../cloud/     Hugging Face client
    src/main/java/.../ai/        AI provider clients
    src/main/java/.../vault/     Keystore secret storage
    src/main/cpp/bridge/         JNI glue
  engine/                 C++ engine (CMake library libqivengine.so)
    core/        math, memory, logging, job system, event bus
    scene/       Actor, Component, Scene, Prefab, serializer
    render/      Filament wrapper, camera, lights, debug draw
    physics/     Jolt wrapper, vehicles, character controller
    anim/        skeleton, clips, blend tree, state machine
    audio/       Oboe output, 3D mixer, emitters
    input/       touch, gestures, virtual HUD controls
    assets/      loaders, cache, streaming, LOD
    script/      QivScript lexer, parser, compiler, QVM
    patch/       Patch graph model and compiler
    terrain/     heightmap, splat, vegetation scatter
    vfx/         particle system
    ui2d/        in-game HUD widgets
    ai/          navmesh, behavior states for enemies
  tools/                  Host tools (asset baker, qvs CLI compiler)
  tests/                  Unit, integration, golden-image tests
  templates/              Starter project templates
  docs/                   These 15 documents
  DECISIONS.md            Agent decision log
```

## 4. Threads

| Thread | Owner | Job | Rule |
|---|---|---|---|
| UI thread | Android | Compose drawing, touch to shell | Never blocks on engine or network |
| Engine main | Engine | Frame loop: input, script tick, scene update, build render commands | Target frame time 16.6 ms |
| Physics | Engine | Fixed 60 Hz step with an accumulator | Results are double-buffered and read by Engine main |
| Render | Filament | Executes GPU commands | Owned by Filament engine thread |
| Audio | Oboe | Callback fills buffers | No allocation and no locks in the callback |
| Job workers | Engine | Decode textures, parse GLB, build collision, bake navmesh | Count is cores minus 2, minimum 2 |
| IO and network | Kotlin coroutines | Hugging Face and AI HTTP | Results enter the engine as events |

## 5. Scene model

QivForge uses an **entity-component** model with a scene tree for transforms.

- **Actor:** integer id plus a name, tag set, parent id, and a flags field (hidden, locked, static).
- **Transform:** local position, rotation (quaternion), scale. World matrix is cached and marked dirty when a parent changes.
- **Components:** stored in dense arrays per type for cache speed. Built-in types: `MeshRenderer`, `SkinnedMesh`, `Light`, `Camera`, `RigidBody`, `Collider`, `WheelCollider`, `VehicleController`, `CharacterController`, `Animator`, `AudioEmitter`, `ParticleEmitter`, `Terrain`, `UIWidget`, `Script`, `PatchGraph`.
- **Prefab:** a saved actor subtree with overrides. Instances store only changed properties.

### Update order each frame

1. Collect input and touch HUD state.
2. Run `onTick(dt)` for every script and patch.
3. Step physics zero or more times with `onPhysicsStep(fixedDt)`.
4. Sync physics results to transforms.
5. Update animation, then skinning.
6. Update streaming (decide which chunks to load or free).
7. Update audio listener and emitters.
8. Build and submit render commands.
9. Emit stats events (fps, frame ms, RAM) at 4 Hz.

## 6. Bridge protocol

There are two channels.

**Command channel (Kotlin to engine).** JNI calls carrying a small binary or JSON command, for example `CreateActor`, `SetProperty(actorId, path, value)`, `Play`, `Pause`, `Stop`, `LoadScene(path)`, `SaveScene`. Each command returns a status code. Commands are queued and applied at the start of the next frame on Engine main.

**Event channel (engine to Kotlin).** A lock-free single-producer ring buffer in shared memory. Events: `SelectionChanged`, `HierarchyChanged`, `StatsUpdated`, `ScriptError(file, line, message)`, `AssetLoaded`, `StreamProgress`, `PlayStateChanged`. The Kotlin side polls the ring on a 16 ms timer and posts to Compose state.

**Surface handoff.** The Compose viewport hosts a `SurfaceView`. When the surface is created, Kotlin passes the `Surface` to the engine, which creates the Filament swap chain. On surface destroy, the engine pauses rendering and releases the swap chain.

## 7. Property system (powers the Inspector and @export)

Every component declares its properties in a table: name, type, default, range, tooltip, and group. The Inspector builds itself from this table, so no per-component UI code is written. QivScript `@export` variables register into the same table when a script is compiled. Supported types: bool, int, float, string, Vec2, Vec3, Color, enum, asset reference, actor reference.

## 8. Undo and redo

All editor changes go through **commands** that implement `do()` and `undo()`. History keeps the last 200 commands. Slider drags merge into one command. Saving does not clear history. Play mode does not record into history; stopping Play restores the scene to the exact state it had before Play.

## 9. Play mode isolation

Pressing Play snapshots the scene into memory, starts a runtime copy, and runs it. Pressing Stop discards the runtime copy and restores the editor scene. Pause freezes physics, animation, and scripts but keeps rendering. Any `ScriptError` stops the script that failed, shows the diagnostic, and keeps the rest of the game running.

## 10. Asset pipeline

1. **Import:** user picks a file through the Storage Access Framework. Supported: GLB/glTF, PNG, JPG, KTX2, WAV, MP3, OGG.
2. **Bake (on device or host tool):** textures to KTX2 (ASTC or ETC2), meshes to Meshopt compressed GLB, audio to OGG.
3. **Metadata:** each asset gets a sidecar `.meta.json` with id (UUID), type, source license, author, and import settings.
4. **Cache:** baked output goes to the project `.cache/` folder keyed by content hash.
5. **Reference:** scenes reference assets by UUID, never by file path, so renaming files never breaks scenes.

### Streaming

The world is split into square chunks (default 64 m). Each chunk has a manifest listing its assets and three LOD levels. The streamer keeps a ring of chunks around the camera: load radius and unload radius differ by one chunk to avoid thrashing. Loads run on job workers. Frees happen on Engine main and must release GPU buffers the same frame. Memory budget is a hard cap; when it is hit the streamer drops the farthest chunks first.

## 11. Project folder format

```
MyGame/
  project.qivproj        engine version, mode (local or cloud), theme, input map
  scenes/*.scene.json    actors, transforms, component data, asset UUIDs
  scripts/*.qvs          QivScript sources
  patches/*.qpatch       visual graphs
  prefabs/*.prefab.json
  assets/models/ textures/ audio/ animations/
  ui/*.hud.json          HUD layouts
  .cache/                baked data, safe to delete
  .journal/              autosave and crash recovery
```

Every JSON file has a `formatVersion` integer. A migration function exists for each version bump; opening an older project upgrades it and keeps a backup copy.

## 12. Error handling policy

- No exception crosses the JNI boundary. The bridge catches everything and returns a status code.
- Asset load failure shows a pink placeholder mesh and logs a warning; it never crashes.
- Out of memory during streaming triggers an emergency unload of distant chunks and a toast.
- Every error has a stable code (for example `QF-ASSET-004`) listed in `docs/errors.md`.

## 13. Logging and diagnostics

Log levels: error, warn, info, debug. The Console tab shows logs and script prints with filtering. A ring buffer keeps the last 5,000 lines. A **Share diagnostics** action zips logs, device info, and the last crash, with all tokens removed.

## 14. Build system

- Gradle builds the app. CMake builds `libqivengine.so` for `arm64-v8a` (required) and `x86_64` (emulator only).
- C++ standard: 20. Flags: `-O2 -fno-exceptions -fno-rtti` for engine code except where a dependency requires them.
- Third-party libraries are pinned by exact version and fetched by a lock file. Required: Filament, Jolt Physics, Oboe, miniaudio, Draco decoder, meshoptimizer, Basis Universal transcoder, nlohmann/json or simdjson, and llama.cpp (optional module, Doc 12).
- CI runs unit tests on host and instrumented tests on an emulator. Device tests run on a physical phone before each milestone is accepted.

## 15. Coding standards

- Names: `PascalCase` types, `camelCase` functions, `snake_case` files.
- No raw owning pointers; use `unique_ptr` or arena handles.
- No allocation inside per-frame loops after warm-up; use pools.
- Every public engine function has a one-line comment on thread and ownership rules.
- Each subsystem ships with a test target and a small demo scene.

## 16. Acceptance criteria for this document

1. A blank project builds, installs, and shows a lit cube at 60 fps.
2. A Kotlin button sends `CreateActor` and the actor appears in the Hierarchy through an event.
3. Rotating the phone or backgrounding the app does not crash or leak the swap chain.
4. Play then Stop returns the scene to exactly its earlier state.
5. Killing the app during editing and reopening it offers journal recovery.
