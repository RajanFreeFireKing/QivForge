# QivForge Feature Checklist (FEATURES.md)

> Generated from Doc 03 — Complete Feature Catalogue. Ticks [x] only when exit tests pass on real device.


### A. Project and workflow (Docs 04, 11)

- [x] **[A-001]** Project Hub with project cards [M] — Name, thumbnail, last edited, mode badge (Local or Cloud) (Verified in M1)
- [x] **[A-002]** New Project from template [M] — Five templates from Doc 01 (Verified in M1)
- [x] **[A-003]** Local and Cloud project modes [M local / 1 cloud] — Both work without any account (Verified in M1 & M7)
- [x] **[A-004]** One-tap migration Local to Cloud and back [1] — Progress screen and rollback (Verified in M7)
- [x] **[A-005]** Autosave journal and crash recovery [M] — Doc 02 section 11 (Verified in M3)
- [x] **[A-006]** Undo and redo (200 steps) [M] — Command pattern (Verified in M3)
- [ ] **[A-007]** Project import and export as .qivpack zip [1] — Through Storage Access Framework
- [x] **[A-008]** Duplicate and rename project [M] (Verified in M1)
- [ ] **[A-009]** Project settings screen [M] — Engine version, target FPS, orientation, package name
- [x] **[A-010]** Version history (cloud commits list and restore) [1] (Verified in M7)
- [ ] **[A-011]** Multi-scene projects and scene switching [1]
- [ ] **[A-012]** Build variants: Debug and Release [1]

### B. Scene and editor (Doc 04)

- [x] **[B-001]** Scene hierarchy tree with hide, lock, rename, drag to reparent [M] (Verified in M3)
- [x] **[B-002]** 3D viewport with orbit, pan, zoom, focus on selection [M] — One finger orbit, two finger pan and pinch zoom (Verified in M3)
- [x] **[B-003]** Transform gizmos: move, rotate, scale, local and world [M] — Snap toggles for grid, angle, scale (Verified in M3)
- [ ] **[B-004]** Multi-select and group [1]
- [x] **[B-005]** Inspector built from property tables [M] — Doc 02 section 7 (Verified in M3)
- [ ] **[B-006]** Prefabs with overrides [1]
- [x] **[B-007]** Copy, paste, duplicate, delete [M] (Verified in M3)
- [x] **[B-008]** Search bar for hierarchy and assets [M] (Verified in M3)
- [ ] **[B-009]** Viewport shading modes: lit, unlit, wireframe, collision, normals [1]
- [x] **[B-010]** Debug overlays: FPS, frame ms, draw calls, triangles, RAM, VRAM [M] (Verified in M2)
- [ ] **[B-011]** Layers and tags [1]
- [ ] **[B-012]** Editor camera bookmarks [2]
- [ ] **[B-013]** Split view and dockable panels (tablet) [2]

### C. Rendering (Doc 05)

- [x] **[C-001]** PBR metallic-roughness materials [M] (Verified in M2)
- [x] **[C-002]** Directional, point, and spot lights [M] (Verified in M2)
- [x] **[C-003]** Dynamic shadows with cascades [1] — M ships one cascade (Verified in M9)
- [ ] **[C-004]** Image-based lighting and skybox [M] — Procedural sky plus HDR import
- [ ] **[C-005]** Material editor with texture slots [1] — Base color, normal, metal-rough, occlusion, emissive
- [ ] **[C-006]** Transparent and cutout materials [1]
- [x] **[C-007]** Post-process: tone mapping, bloom, SSAO, FXAA/MSAA, color grading, vignette [1] — Each toggle-able (Verified in M9)
- [x] **[C-008]** Fog (distance and height) [1] (Verified in M9)
- [ ] **[C-009]** Reflection probes [2]
- [x] **[C-010]** LOD groups and distance culling [1] (Verified in M9)
- [ ] **[C-011]** Frustum culling and occlusion culling [M frustum / 2 occlusion]
- [x] **[C-012]** GPU instancing for vegetation and props [1] (Verified in M9)
- [ ] **[C-013]** Decals (bullet holes, tire marks) [1]
- [ ] **[C-014]** Water surface shader [2]
- [x] **[C-015]** Quality governor and graphics presets: Low, Medium, High [1] — Doc 01 section 6 (Verified in M6)
- [ ] **[C-016]** Screenshot and video capture [1]

### D. Physics and vehicles (Doc 06)

- [x] **[D-001]** Rigid bodies: static, dynamic, kinematic [M]
- [x] **[D-002]** Colliders: box, sphere, capsule, convex, mesh, heightfield [M]
- [x] **[D-003]** Fixed 60 Hz step with interpolation [M]
- [x] **[D-004]** Raycast, shape cast, overlap queries [M]
- [x] **[D-005]** Triggers and collision events [M]
- [ ] **[D-006]** Joints: hinge, fixed, spring, slider [1]
- [x] **[D-007]** Physics materials: friction, bounce [M]
- [x] **[D-008]** Four-wheel vehicle with suspension, motor, brake, steering [M] — Tractor and car presets
- [x] **[D-009]** Tire friction curves and terrain surface types (grass, mud, asphalt) [1]
- [ ] **[D-010]** Tracked vehicles, trailers, and hitch joints [2] — Tractor with trailer
- [x] **[D-011]** Character controller: walk, run, jump, slopes, stairs [M]
- [ ] **[D-012]** Ragdoll [1]
- [ ] **[D-013]** Destructible props with fracture [2]
- [ ] **[D-014]** Cloth and soft bodies [2]
- [x] **[D-015]** Physics debug draw [M]

### E. Animation and characters (Doc 07)

- [x] **[E-001]** Skeletal animation from GLB [M] (Verified in M8)
- [x] **[E-002]** Animation clips with play, loop, speed [M] (Verified in M8)
- [x] **[E-003]** Blend trees (1D and 2D) [1] — Walk and run by speed (Verified in M8)
- [x] **[E-004]** State machine with transition conditions and crossfade [1] — AnimGraph (Verified in M8)
- [x] **[E-005]** Layers and avatar masks (upper body shoot while legs run) [1] (Verified in M8)
- [x] **[E-006]** Animation events (footstep, muzzle flash) [1] (Verified in M8)
- [x] **[E-007]** Inverse kinematics: foot IK and look-at [2] — Tier 1 Foot IK (Verified in M8)
- [ ] **[E-008]** Root motion [1]
- [ ] **[E-009]** Animation retargeting between skeletons [2] — Humanoid map
- [ ] **[E-010]** Timeline and cutscene editor [2]
- [ ] **[E-011]** Facial blend shapes [2]

### F. Scripting and logic (Docs 08, 09)

- [x] **[F-001]** QivScript lexer, parser, compiler, VM [M] (Verified in M5)
- [x] **[F-002]** Hot reload during Play [1] (Verified in M5)
- [x] **[F-003]** @export and @range inspector fields [M] (Verified in M5)
- [x] **[F-004]** Engine base classes: Actor, VehiclePawn, CharacterPawn, WeaponComponent, AIController, UIWidget [M] (Verified in M5)
- [x] **[F-005]** Code editor with syntax highlighting, autocomplete, error bar [M] — Doc 04 and Doc 08 (Verified in M5)
- [x] **[F-006]** Code editor 3-dot menu (six actions) [M] — R2 (Verified in M5)
- [ ] **[F-007]** Script debugger: breakpoints, step, watch [2]
- [x] **[F-008]** Visual Patch logic graphs [1] (Verified in M8)
- [x] **[F-009]** Visual AnimGraph [1] (Verified in M8)
- [x] **[F-010]** Convert Script to Patch and Patch to Script [1] (Verified in M8)
- [ ] **[F-011]** Event bus and custom events [M]
- [ ] **[F-012]** Coroutines and async cloud loading [1]
- [ ] **[F-013]** Save game API (local key-value store) [1]
- [x] **[F-014]** Standard library: math, strings, arrays, maps, timers [M] (Verified in M5)

### G. Input and mobile controls (Docs 04, 10)

- [x] **[G-001]** Touch HUD controls: joystick, buttons, steering wheel, pedals, sliders [M] (Verified in M4)
- [x] **[G-002]** Multi-touch with correct pointer ownership [M] (Verified in M4)
- [ ] **[G-003]** Gesture recognizers: tap, long press, swipe, pinch [1]
- [ ] **[G-004]** Gyroscope and accelerometer steering [1]
- [ ] **[G-005]** Gamepad and keyboard support (Bluetooth) [1]
- [ ] **[G-006]** Input action map editable in project settings [1]
- [ ] **[G-007]** Haptic feedback hooks [1]

### H. World building (Doc 10)

- [x] **[H-001]** Terrain: heightmap sculpt (raise, lower, smooth, flatten) [1] (Verified in M9)
- [x] **[H-002]** Terrain texture painting with 4 to 8 layers [1] (Verified in M9)
- [x] **[H-003]** Vegetation and prop scatter with density brush [1] (Verified in M9)
- [ ] **[H-004]** Road and path spline tool [2]
- [ ] **[H-005]** Chunked world streaming [1] — Doc 02 section 10
- [x] **[H-006]** Navmesh bake for AI [1] (Verified in M9)
- [ ] **[H-007]** Day and night cycle, weather [2]
- [ ] **[H-008]** Procedural terrain generator [2]

### I. VFX and audio (Doc 10)

- [x] **[I-001]** Particle system: emitters, shapes, curves, sprite sheets [1] — Smoke, fire, muzzle flash, sparks (Verified in M9)
- [ ] **[I-002]** Trails and ribbons [2]
- [x] **[I-003]** 3D spatial audio with distance falloff and doppler [M] (Verified in M6)
- [x] **[I-004]** Audio mixer with buses and volume sliders [1] (Verified in M9)
- [x] **[I-005]** Music playlist and crossfade [1] (Verified in M9)
- [x] **[I-006]** Engine sound pitch driven by RPM [M] — Tractor and car (Verified in M6)
- [x] **[I-007]** Audio import: WAV, MP3, OGG [M] — Embedded WAV parser (Verified in M6)

### J. In-game UI (Doc 10)

- [x] **[J-001]** 2D HUD Designer with drag and drop [1] (Verified in M9)
- [x] **[J-002]** Widgets: button, label, image, progress bar, joystick, minimap, speedometer, list [1] (Verified in M9)
- [x] **[J-003]** Anchors, safe area, and multiple screen sizes [1] (Verified in M9)
- [ ] **[J-004]** UI animations and transitions [2]
- [ ] **[J-005]** Localization tables (multiple languages) [2]
- [ ] **[J-006]** Font import [1]

### K. Gameplay systems (Docs 06, 07, 08)

- [ ] **[K-001]** Weapon component: raycast, ammo, reload, recoil, fire rate [1]
- [ ] **[K-002]** Health and damage system with events [M]
- [ ] **[K-003]** Enemy AI: patrol, chase, attack with navmesh [1]
- [ ] **[K-004]** Behavior tree editor [2]
- [ ] **[K-005]** Inventory and pickup components [2]
- [ ] **[K-006]** Checkpoints, lap timer, and respawn [1]
- [ ] **[K-007]** Camera rigs: follow, orbit, first person, shoulder [M]
- [ ] **[K-008]** Camera shake and smooth transitions [1]

### L. Assets and marketplace (Docs 02, 11)

- [ ] **[L-001]** Asset browser with thumbnails, search, and tags [M]
- [x] **[L-002]** Import GLB, PNG, JPG, KTX2, WAV, MP3, OGG [M] — Binary GLB & .qmesh baker (Verified in M6)
- [ ] **[L-003]** Automatic texture and mesh compression [1]
- [x] **[L-004]** Asset license field and attribution export [1] — Doc 14 (Verified in M12)
- [x] **[L-005]** Hugging Face asset browser with one-tap stream [1] (Verified in M7)
- [ ] **[L-006]** Asset packs: install, update, remove [2]
- [ ] **[L-007]** Built-in starter library: primitives, tractor, car, character, tree, rock, building kit [1] — Only CC0 or original assets
- [ ] **[L-008]** Publish and share a project or asset [2]

### M. AI assistant (Doc 12)

- [x] **[M-001]** Chat panel with project context [1] (Verified in M10)
- [x] **[M-002]** Providers: Gemini, Groq, OpenRouter, Hugging Face, local GGUF [1] (Verified in M10)
- [x] **[M-003]** Antigravity provider [2] — Experimental flag, off by default (Verified in M10)
- [x] **[M-004]** Tool calls: spawn asset, write script, create patch, edit property [1] (Verified in M10)
- [x] **[M-005]** Every AI edit shown as a diff with Accept or Reject [1] — No silent changes (Verified in M10)
- [x] **[M-006]** Fix this error button in the code editor [1] (Verified in M10)
- [ ] **[M-007]** Generate texture or sound through Hugging Face models [2]

### N. Build, publish, performance (Docs 13, 14)

- [x] **[N-001]** APK and AAB export with signing [1] (Verified in M11)
- [x] **[N-002]** Cloud APK builder [1] — User-triggered, Doc 13 (Verified in M11)
- [x] **[N-003]** App icon, splash, name, version editor [1] (Verified in M11)
- [x] **[N-004]** Console and logs [M] (Verified in M5/M6)
- [x] **[N-005]** Profiler: CPU, GPU, memory timeline [1] (Verified in M6/M9)
- [x] **[N-006]** Memory leak and asset budget report [1] (Verified in M11)
- [x] **[N-007]** Crash reporting opt-in [2] — Doc 14 §14 (Verified in M12)
- [x] **[N-008]** Device capability detection and automatic quality choice [1] (Verified in M6)

### O. Settings, theme, accessibility (Doc 04)

- [x] **[O-001]** Dynamic theme engine (R1) with live color picker, hex and RGB [M] — Presets: Dark, Light, AMOLED, Forest, Ocean (Verified in M1)
- [x] **[O-002]** Theme export and import as JSON [1] (Verified in M12)
- [ ] **[O-003]** UI scale and font size [M]
- [ ] **[O-004]** Language packs for the editor [2]
- [x] **[O-005]** Reduced motion and high contrast [1] (Verified in M12)
- [x] **[O-006]** Vault screen for tokens [M] — Doc 14 (Verified in M7)

### P. Multiplayer and live services (v2)

- [ ] **[P-001]** Client-server networking with state replication [2]
- [ ] **[P-002]** Lobby and matchmaking [2]
- [ ] **[P-003]** Leaderboards and cloud save [2]
- [ ] **[P-004]** Voice chat [2]

### Q. Feature count and rules


### R. Added features (not in Blueprint v1 or v2)

