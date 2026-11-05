# QivForge Doc 03 — Complete Feature Catalogue

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01 and 02 · **Audience:** AI coding agents

This is the master list of everything a modern game engine has, mapped to QivForge. Each row has a **tier**: **M** = MVP (v0.1), **1** = v1.0, **2** = v2.0. A row marked M or 1 is mandatory for that tier. The detailed behavior of each group lives in the document named in the group heading.

---

## A. Project and workflow (Docs 04, 11)

| Feature | Tier | Notes |
|---|---|---|
| Project Hub with project cards | M | Name, thumbnail, last edited, mode badge (Local or Cloud) |
| New Project from template | M | Five templates from Doc 01 |
| Local and Cloud project modes | M local / 1 cloud | Both work without any account |
| One-tap migration Local to Cloud and back | 1 | Progress screen and rollback |
| Autosave journal and crash recovery | M | Doc 02 section 11 |
| Undo and redo (200 steps) | M | Command pattern |
| Project import and export as .qivpack zip | 1 | Through Storage Access Framework |
| Duplicate and rename project | M | |
| Project settings screen | M | Engine version, target FPS, orientation, package name |
| Version history (cloud commits list and restore) | 1 | |
| Multi-scene projects and scene switching | 1 | |
| Build variants: Debug and Release | 1 | |

## B. Scene and editor (Doc 04)

| Feature | Tier | Notes |
|---|---|---|
| Scene hierarchy tree with hide, lock, rename, drag to reparent | M | |
| 3D viewport with orbit, pan, zoom, focus on selection | M | One finger orbit, two finger pan and pinch zoom |
| Transform gizmos: move, rotate, scale, local and world | M | Snap toggles for grid, angle, scale |
| Multi-select and group | 1 | |
| Inspector built from property tables | M | Doc 02 section 7 |
| Prefabs with overrides | 1 | |
| Copy, paste, duplicate, delete | M | |
| Search bar for hierarchy and assets | M | |
| Viewport shading modes: lit, unlit, wireframe, collision, normals | 1 | |
| Debug overlays: FPS, frame ms, draw calls, triangles, RAM, VRAM | M | |
| Layers and tags | 1 | |
| Editor camera bookmarks | 2 | |
| Split view and dockable panels (tablet) | 2 | |

## C. Rendering (Doc 05)

| Feature | Tier | Notes |
|---|---|---|
| PBR metallic-roughness materials | M | |
| Directional, point, and spot lights | M | |
| Dynamic shadows with cascades | 1 | M ships one cascade |
| Image-based lighting and skybox | M | Procedural sky plus HDR import |
| Material editor with texture slots | 1 | Base color, normal, metal-rough, occlusion, emissive |
| Transparent and cutout materials | 1 | |
| Post-process: tone mapping, bloom, SSAO, FXAA/MSAA, color grading, vignette | 1 | Each toggle-able |
| Fog (distance and height) | 1 | |
| Reflection probes | 2 | |
| LOD groups and distance culling | 1 | |
| Frustum culling and occlusion culling | M frustum / 2 occlusion | |
| GPU instancing for vegetation and props | 1 | |
| Decals (bullet holes, tire marks) | 1 | |
| Water surface shader | 2 | |
| Quality governor and graphics presets: Low, Medium, High | 1 | Doc 01 section 6 |
| Screenshot and video capture | 1 | |

## D. Physics and vehicles (Doc 06)

| Feature | Tier | Notes |
|---|---|---|
| Rigid bodies: static, dynamic, kinematic | M | |
| Colliders: box, sphere, capsule, convex, mesh, heightfield | M | |
| Fixed 60 Hz step with interpolation | M | |
| Raycast, shape cast, overlap queries | M | |
| Triggers and collision events | M | |
| Joints: hinge, fixed, spring, slider | 1 | |
| Physics materials: friction, bounce | M | |
| Four-wheel vehicle with suspension, motor, brake, steering | M | Tractor and car presets |
| Tire friction curves and terrain surface types (grass, mud, asphalt) | 1 | |
| Tracked vehicles, trailers, and hitch joints | 2 | Tractor with trailer |
| Character controller: walk, run, jump, slopes, stairs | M | |
| Ragdoll | 1 | |
| Destructible props with fracture | 2 | |
| Cloth and soft bodies | 2 | |
| Physics debug draw | M | |

## E. Animation and characters (Doc 07)

| Feature | Tier | Notes |
|---|---|---|
| Skeletal animation from GLB | M | |
| Animation clips with play, loop, speed | M | |
| Blend trees (1D and 2D) | 1 | Walk and run by speed |
| State machine with transition conditions and crossfade | 1 | AnimGraph |
| Layers and avatar masks (upper body shoot while legs run) | 1 | |
| Animation events (footstep, muzzle flash) | 1 | |
| Inverse kinematics: foot IK and look-at | 2 | |
| Root motion | 1 | |
| Animation retargeting between skeletons | 2 | Humanoid map |
| Timeline and cutscene editor | 2 | |
| Facial blend shapes | 2 | |

## F. Scripting and logic (Docs 08, 09)

| Feature | Tier | Notes |
|---|---|---|
| QivScript lexer, parser, compiler, VM | M | |
| Hot reload during Play | 1 | |
| @export and @range inspector fields | M | |
| Engine base classes: Actor, VehiclePawn, CharacterPawn, WeaponComponent, AIController, UIWidget | M | |
| Code editor with syntax highlighting, autocomplete, error bar | M | Doc 04 and Doc 08 |
| Code editor 3-dot menu (six actions) | M | R2 |
| Script debugger: breakpoints, step, watch | 2 | |
| Visual Patch logic graphs | 1 | |
| Visual AnimGraph | 1 | |
| Convert Script to Patch and Patch to Script | 1 | |
| Event bus and custom events | M | |
| Coroutines and async cloud loading | 1 | |
| Save game API (local key-value store) | 1 | |
| Standard library: math, strings, arrays, maps, timers | M | |

## G. Input and mobile controls (Docs 04, 10)

| Feature | Tier | Notes |
|---|---|---|
| Touch HUD controls: joystick, buttons, steering wheel, pedals, sliders | M | |
| Multi-touch with correct pointer ownership | M | |
| Gesture recognizers: tap, long press, swipe, pinch | 1 | |
| Gyroscope and accelerometer steering | 1 | |
| Gamepad and keyboard support (Bluetooth) | 1 | |
| Input action map editable in project settings | 1 | |
| Haptic feedback hooks | 1 | |

## H. World building (Doc 10)

| Feature | Tier | Notes |
|---|---|---|
| Terrain: heightmap sculpt (raise, lower, smooth, flatten) | 1 | |
| Terrain texture painting with 4 to 8 layers | 1 | |
| Vegetation and prop scatter with density brush | 1 | |
| Road and path spline tool | 2 | |
| Chunked world streaming | 1 | Doc 02 section 10 |
| Navmesh bake for AI | 1 | |
| Day and night cycle, weather | 2 | |
| Procedural terrain generator | 2 | |

## I. VFX and audio (Doc 10)

| Feature | Tier | Notes |
|---|---|---|
| Particle system: emitters, shapes, curves, sprite sheets | 1 | Smoke, fire, muzzle flash, sparks |
| Trails and ribbons | 2 | |
| 3D spatial audio with distance falloff and doppler | M | |
| Audio mixer with buses and volume sliders | 1 | |
| Music playlist and crossfade | 1 | |
| Engine sound pitch driven by RPM | M | Tractor and car |
| Audio import: WAV, MP3, OGG | M | |

## J. In-game UI (Doc 10)

| Feature | Tier | Notes |
|---|---|---|
| 2D HUD Designer with drag and drop | 1 | |
| Widgets: button, label, image, progress bar, joystick, minimap, speedometer, list | 1 | |
| Anchors, safe area, and multiple screen sizes | 1 | |
| UI animations and transitions | 2 | |
| Localization tables (multiple languages) | 2 | |
| Font import | 1 | |

## K. Gameplay systems (Docs 06, 07, 08)

| Feature | Tier | Notes |
|---|---|---|
| Weapon component: raycast, ammo, reload, recoil, fire rate | 1 | |
| Health and damage system with events | M | |
| Enemy AI: patrol, chase, attack with navmesh | 1 | |
| Behavior tree editor | 2 | |
| Inventory and pickup components | 2 | |
| Checkpoints, lap timer, and respawn | 1 | |
| Camera rigs: follow, orbit, first person, shoulder | M | |
| Camera shake and smooth transitions | 1 | |

## L. Assets and marketplace (Docs 02, 11)

| Feature | Tier | Notes |
|---|---|---|
| Asset browser with thumbnails, search, and tags | M | |
| Import GLB, PNG, JPG, KTX2, WAV, MP3, OGG | M | |
| Automatic texture and mesh compression | 1 | |
| Asset license field and attribution export | 1 | Doc 14 |
| Hugging Face asset browser with one-tap stream | 1 | |
| Asset packs: install, update, remove | 2 | |
| Built-in starter library: primitives, tractor, car, character, tree, rock, building kit | 1 | Only CC0 or original assets |
| Publish and share a project or asset | 2 | |

## M. AI assistant (Doc 12)

| Feature | Tier | Notes |
|---|---|---|
| Chat panel with project context | 1 | |
| Providers: Gemini, Groq, OpenRouter, Hugging Face, local GGUF | 1 | |
| Antigravity provider | 2 | Experimental flag, off by default |
| Tool calls: spawn asset, write script, create patch, edit property | 1 | |
| Every AI edit shown as a diff with Accept or Reject | 1 | No silent changes |
| Fix this error button in the code editor | 1 | |
| Generate texture or sound through Hugging Face models | 2 | |

## N. Build, publish, performance (Docs 13, 14)

| Feature | Tier | Notes |
|---|---|---|
| APK and AAB export with signing | 1 | |
| Cloud APK builder | 1 | User-triggered, Doc 13 |
| App icon, splash, name, version editor | 1 | |
| Console and logs | M | |
| Profiler: CPU, GPU, memory timeline | 1 | |
| Memory leak and asset budget report | 1 | |
| Crash reporting opt-in | 2 | |
| Device capability detection and automatic quality choice | 1 | |

## O. Settings, theme, accessibility (Doc 04)

| Feature | Tier | Notes |
|---|---|---|
| Dynamic theme engine (R1) with live color picker, hex and RGB | M | Presets: Dark, Light, AMOLED, Forest, Ocean |
| Theme export and import as JSON | 1 | |
| UI scale and font size | M | |
| Language packs for the editor | 2 | |
| Reduced motion and high contrast | 1 | |
| Vault screen for tokens | M | Doc 14 |

## P. Multiplayer and live services (v2)

| Feature | Tier | Notes |
|---|---|---|
| Client-server networking with state replication | 2 | |
| Lobby and matchmaking | 2 | |
| Leaderboards and cloud save | 2 | |
| Voice chat | 2 | |

## Q. Feature count and rules

- The MVP contains the rows marked M only; the v1.0 contains M and 1; v2.0 contains everything.
- Agents must add each feature to `FEATURES.md` in the repo as a checklist and tick it only when its acceptance criterion passes.
- New features may be proposed by the owner or the agent, but they enter the catalogue only with a tier, an owning document, and an acceptance criterion.

## R. Added features (not in Blueprint v1 or v2)

Autosave journal and crash recovery, undo and redo, prefabs, layers and tags, multi-scene projects, version history, LOD and culling, GPU instancing, decals, post-processing stack, quality governor, device capability detection, gamepad and gyroscope input, haptics, localization, save game API, camera rigs, checkpoints, behavior trees, navmesh, ragdoll, tire and surface physics, trailer hitch joints, audio mixer, profiler, asset license tracking, AI edit diff review, and the build variants Debug and Release.
