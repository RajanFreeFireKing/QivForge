# QivForge Doc 10 — Editor Tools: Terrain, HUD Designer, VFX and Audio

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01 to 09 · **Audience:** AI coding agents

---

## 1. Scope

These are the five Bottom Dock tools from Doc 04 section 6.6: Asset Market (assets side only; cloud streaming is Doc 11), Terrain Sculptor, 2D HUD Designer, VFX and Audio, and Console and Profiler. Each section gives data formats, tool behavior, runtime rules, limits, and acceptance criteria.

---

## PART A — TERRAIN SCULPTOR (tier 1)

### A1. Terrain data model

| Item | Specification |
|---|---|
| Terrain actor | One `Terrain` component per world. Contains a grid of chunks |
| Chunk size | 64 m by 64 m (matches streaming chunks, Doc 02 section 10) |
| Heightmap resolution per chunk | 129 by 129 samples (about 0.5 m spacing), 16-bit height, stored as a `.r16` file |
| Height range | Configurable per terrain, default -50 m to +450 m |
| Splat maps | 2 RGBA8 textures per chunk, 128 by 128 (up to 8 layers) |
| Layers | Up to 8 per terrain. Each layer: albedo, normal, roughness-occlusion (packed), UV tiling, surface type (asphalt, dirt, grass, mud, sand, ice), footstep sound |
| Holes mask | 1-bit mask per chunk for caves and tunnels (v2) |
| Vegetation | Per-chunk instance lists for trees, grass, rocks (section A5) |
| World size | Up to 8 km by 8 km (125 by 125 chunks) in v1.0; only edited or non-flat chunks are stored |
| Files | `terrain/terrain.json` (settings and layers), `terrain/h_<x>_<z>.r16`, `terrain/s_<x>_<z>.png`, `terrain/veg_<x>_<z>.bin` |

Only chunks that differ from the default flat height are written to disk or to Hugging Face. Chunk files are independent so cloud commits touch only edited chunks.

### A2. Brushes and tools

| Tool | Behavior | Parameters |
|---|---|---|
| Raise / Lower | Adds or subtracts height under the brush | size (1 to 200 m), strength (0 to 1), falloff curve (smooth, linear, sharp), invert toggle |
| Smooth | Averages neighboring heights | size, strength |
| Flatten | Moves heights toward a target height | target height (tap to sample), size, strength |
| Set Height | Sets exact height value | height, size |
| Noise | Adds Perlin or ridged noise | scale, amplitude, seed |
| Erosion (simple thermal) | Moves material downhill for N iterations | iterations (1 to 50), strength |
| Ramp / Road cut | Draw a line; terrain is flattened along it with sloped sides | width, side slope |
| Stamp | Applies a saved heightmap stamp (mountain, crater, hill) | size, rotation, strength |
| Paint Layer | Paints splat weights for the selected layer | size, strength, falloff, opacity |
| Fill Layer | Floods the selected layer over the whole terrain or by slope range | min slope, max slope, min and max height |
| Scatter | Places vegetation and props | density, scale range, rotation, align to normal, slope limits, exclusion by layer |
| Erase Scatter | Removes instances under the brush | size |

### A3. Interaction rules

- One finger paints while the camera is locked; two fingers move the camera (orbit, pan, zoom) without painting. A toggle **Camera lock** is shown in the tool header.
- Brush preview: a circle on the terrain projected along the normal, colored by tool.
- Strokes are applied on a job worker in 16 ms slices; results appear in the viewport within one frame of finger movement (the brush edits the CPU heightmap and uploads only the dirty rectangle to the GPU).
- Every stroke is one **undo command** storing only the changed rectangle (before and after) so history is cheap (maximum 64 MB of terrain undo data, oldest dropped first).
- Terrain collision (Jolt heightfield) updates after the stroke ends, not during it, to avoid hitches.
- Symmetry options: none, mirror X, mirror Z, radial (v1.0).
- Import and export: heightmap as 16-bit PNG or RAW; splat maps as PNG; the importer resamples to the chunk grid.
- **Procedural generator (v2):** noise-based world with sliders for mountains, rivers, and biomes.

### A4. Terrain materials and LOD

- Rendering follows Doc 05 section 10 (3 LODs with skirts, splat blending, triplanar on steep slopes).
- Layer limits per chunk: only layers with nonzero weight in a chunk are sampled (the shader uses 4 layers per pass; chunks with more than 4 layers use two passes).
- Layer thumbnails are shown as swatches in a layer list; tap to select, long press to edit tiling.

### A5. Vegetation and scatter

| Item | Specification |
|---|---|
| Scatter item | Model (with LOD levels), billboard imposter for far LOD, scale range, color variance, slope range, height range, collision (none, trunk capsule), wind sway amount |
| Storage | Per-chunk compact binary: position (3 x 16-bit relative to chunk), rotation (8 bit yaw), scale (8 bit), item id |
| Instances per chunk | Up to 10,000 (Doc 05 section 8) |
| Rendering | GPU instancing, distance culled by item (grass 60 m, trees 250 m, rocks 200 m), density multiplier from the quality preset |
| Wind | Vertex shader sway driven by a global wind vector parameter |
| Collision | Only trees and rocks have colliders (capsule or convex); grass never has colliders. Colliders exist only in chunks within physics radius (80 m) |

### A6. Roads (tier 2) and navmesh (tier 1)

- **Road tool (v2):** spline with width, banking, surface type; generates mesh and carves terrain. Roads set surface type for vehicle physics.
- **Navmesh bake (v1.0):** per chunk using Recast-style voxelization on a job worker; parameters: agent radius, height, step height, max slope. Bake runs in the background with a progress chip, results stored in `navmesh/nav_<x>_<z>.bin`. Used by enemy AI (Doc 06 and 07).

### A7. Acceptance criteria (Part A)

1. Raising terrain with a 50 m brush at 60 fps in the viewport on the reference device with a 4 by 4 chunk area.
2. A 10-stroke edit and undo returns the heightmap byte-identical to the original.
3. Saving a project with 3 edited chunks out of 100 writes only those 3 height files.
4. A tractor driving from a grass layer onto a mud layer shows the surface change in physics (Doc 06 section 9.4).
5. Scattering 50,000 trees over 8 chunks stays above 45 fps on High.
6. Importing a 4096 by 4096 16-bit heightmap completes in under 20 seconds.

---

## PART B — 2D HUD DESIGNER (tier 1)

### B1. Concept

A HUD Layout is the on-screen UI of a game, built from widgets, saved as `ui/<name>.hud.json`. A game can have several layouts (Gameplay, Pause menu, Garage, Game over), shown or hidden by scripts or patches.

### B2. Canvas model

- Reference resolution: 1920 by 1080 (landscape), with an alternative 1080 by 1920 for portrait projects.
- Each widget has an **anchor** (9 points plus stretch), **pivot**, **offset**, **size**, **rotation**, **scale**, **opacity**, **visible**, and **z-order**.
- **Safe area:** the layout respects display cutouts and gesture bars. A guide overlay shows the safe area, and widgets anchored to the edge are automatically inset.
- Scaling rule: the layout scales to the screen using **match width or height** at 0.5 by default (project setting), so controls keep sensible physical size on tablets. Minimum touch size 48 dp is enforced with a warning.
- A **Device preview** dropdown shows common aspect ratios (16:9, 18:9, 20:9, 4:3, tablet).

### B3. Widget library

| Widget | Properties | Typical use |
|---|---|---|
| Panel | color, image, 9-slice border, corner radius | Backgrounds |
| Label | text, font, size, color, outline, alignment, localization key | Score, messages |
| Image | texture, tint, preserve aspect | Icons, crosshair |
| Button | normal, pressed, disabled images or colors, label, action name | Fire, Aim, Jump, Reload |
| Toggle | on/off images, action | Settings |
| Slider | min, max, value binding | Volume |
| Progress bar | fill color, background, direction, value binding, segment count | Health, ammo, fuel |
| Joystick (virtual) | outer radius, thumb radius, dead zone, mode (fixed or floating), axis name | Move, look |
| Steering wheel | image, max angle, return speed, axis name | Vehicle |
| Pedal | image, pressed image, axis name and direction | Gas, brake |
| Speedometer | style (needle or digital), max speed, unit (km/h or mph), value binding | Vehicle |
| RPM gauge | needle, redline zone | Vehicle |
| Minimap | orthographic camera, size, rotate with player, icon list, zoom | World map |
| Compass | heading binding | Navigation |
| Crosshair | style, spread binding | Shooting |
| List view | item template, scroll | Inventory (v2) |
| Timer / Lap display | format | Racing |
| Damage indicator | direction arc | TPS |
| Notification toast | text, duration, queue | Messages |
| Container (H or V layout, Grid) | spacing, padding, alignment | Auto layout |

### B4. Control binding

- Every control has an **action name** (for example `FireButton`, `AimButton`, `LeftMoveStick`, `BrakePedal`, `Vertical`, `Horizontal`) that scripts read through `TouchHUD` (Doc 08 section 10).
- Axes can be bound to multiple sources: touch widgets, gyro, gamepad, keyboard; the Project Settings **Input Map** lists bindings.
- **Multi-touch:** each widget claims a pointer id on touch down and keeps it until touch up, even if the finger leaves the widget (needed for joysticks and steering wheels). Pointers are never shared between widgets. Up to 10 simultaneous pointers.
- **Floating joystick:** appears where the thumb touches inside its zone and hides on release.
- Haptic feedback per control (off, light, medium).

### B5. Data binding

Value bindings use a path expression that scripts and engine objects expose: `vehicle.speedKmh`, `player.health / player.maxHealth`, `weapon.ammo`, `game.timer`. Bindings update only when the value changes (dirty check). Text format strings: `"{value:0} km/h"`.

### B6. Editor behavior

- Layout of the tool: left widget palette, center canvas with device frame, right property list, top bar with Undo, Redo, Preview, Align, Distribute, Snap, Device.
- Gestures: drag to move, handles to resize and rotate, two finger pinch to zoom canvas, long press to multi-select.
- Alignment tools: align left, center, right, top, middle, bottom; distribute horizontally and vertically; match size; snap to guides and to a 8 px grid.
- **Play preview:** runs the layout live with fake or real data and shows touch response without leaving the tool.
- **Presets:** Racing HUD, TPS HUD, Tractor HUD, Minimal (each is a complete layout with proper bindings).
- Fonts: bundled fonts plus import of TTF or OTF (license field required).
- Images: from assets; recommended 9-slice and atlas packing at bake time (all HUD images packed into 2048 by 2048 atlases).

### B7. Runtime

- HUD drawn in a separate Filament view with an orthographic camera and no post-processing (Doc 05 section 4).
- Batching: one draw call per atlas page in the typical case; label glyphs use a signed distance field font atlas.
- Update cost budget: under 0.3 ms per frame for a HUD of 60 widgets.
- Events: `onHudButton(name, state)` for scripts, and the OnTouchButtonDown Patch node (Doc 09).
- Layout switching API: `UI.show("Gameplay")`, `UI.hide`, `UI.setText(widget, text)`, `UI.setValue(widget, v)`.

### B8. Acceptance criteria (Part B)

1. Steering wheel and gas pedal pressed at the same time both report values for a 60-second test with sliding fingers.
2. A layout designed at 16:9 shows no clipped widgets at 20:9 and on a 4:3 tablet.
3. A HUD with 60 widgets costs under 0.3 ms per frame.
4. Changing `player.health` updates the bar the same frame with no allocations.
5. The Racing HUD preset works in the Vehicle template without any script edits.
6. All buttons in presets meet the 48 dp minimum touch size.

---

## PART C — VFX AND AUDIO (tier 1 unless marked)

### C1. Particle system

| Group | Parameters |
|---|---|
| Emission | rate over time, bursts (time, count, probability), duration, looping, prewarm, max particles (hard cap 2,000 per emitter, 8,000 per scene on High) |
| Shape | point, sphere, hemisphere, cone (angle, radius), box, circle, mesh surface (v2); emit from surface or volume |
| Start values | lifetime, speed, size, rotation, color (each as constant, random between two, or curve) |
| Over lifetime | size, color gradient, speed multiplier, rotation speed, opacity, gravity multiplier |
| Forces | gravity scale, wind, drag, noise turbulence (strength, frequency) |
| Collision | none, world plane, or depth buffer collision (v2); dies or bounces |
| Rendering | billboard (camera facing), stretched billboard (velocity), horizontal, mesh; blend mode (alpha, additive, multiply); soft particles using depth fade |
| Texture | single texture or sprite sheet (columns, rows, frame rate or over lifetime, random start frame) |
| Sub-emitters (v2) | on birth, on death |
| Trails (v2) | ribbon trails |
| Lights (v2) | light per N particles |
| Simulation space | local or world |
| Sorting | by distance or oldest first |

Implementation rules:

- Simulation on the CPU for up to 2,000 particles (SIMD, in job workers); GPU simulation is not required in v1.0.
- Particles use one instanced quad draw per emitter; vertex data uploaded each frame via a ring buffer.
- Effects are saved as `.qfx` assets referenced by name (`VFXManager.spawn("MuzzleFlash", pos, rot)`).
- **Pooling:** effects are pooled by name; spawning an effect never allocates after warm-up.
- The quality governor scales emission rate and max particles by the preset (Doc 05 section 11).

### C2. Built-in effect library (original assets only)

Muzzle flash, bullet spark impact, dirt impact, metal impact, blood-free generic hit puff, explosion (fire, smoke, shockwave), tractor exhaust smoke, tire dust (per surface), mud spray, water splash, campfire, smoke column, rain, dust motes, sparks, magic glow.

### C3. VFX editor tab

- Live preview viewport at the top with play, pause, restart, loop, and a **bounds** display.
- Property groups collapsible in the list below, curves edited with draggable points in a small curve editor, gradients edited with stops and a color picker.
- **Presets** to start from, **Duplicate**, **Save as new effect**, **Test in scene** (places at viewport center).
- Cost meter showing current particle count and estimated cost in ms.

### C4. Audio system

| Item | Specification |
|---|---|
| Output | Oboe (AAudio on Android 8.1 and newer, OpenSL ES fallback), 48 kHz, low-latency mode when available |
| Decode | miniaudio for WAV, MP3, OGG; sounds under 3 seconds are fully decoded to memory, longer ones stream |
| Voices | Up to 32 simultaneous voices; lowest priority and farthest are stopped first |
| 3D audio | Distance models: inverse, linear, logarithmic; min distance, max distance, rolloff; panning by listener orientation (stereo pan plus simple HRTF-free attenuation); Doppler with scale factor; optional occlusion low-pass when a raycast to the listener is blocked (v2) |
| Listener | Attached to the active camera, or to a target actor |
| Emitters | `AudioEmitter` component: clip, volume, pitch, loop, spatial blend (0 to 1), play on awake, priority, bus |
| Buses | Master, Music, SFX, Voice, Ambient, UI; each with volume, mute, and a send to reverb |
| Effects per bus | Low-pass, high-pass, reverb (small room, hall, outdoor), compressor (v2) |
| Music | Playlist with crossfade (duration), loop, shuffle; `Audio.playMusic(asset, fade)` |
| Engine sound | Looping sample whose pitch follows RPM (`pitch = 0.6 + rpm / redline * 1.6`) and volume follows throttle; blend of 3 samples (idle, mid, high) crossfaded by RPM in v1.0 |
| Footsteps | Event driven from animation events with surface type lookup (Doc 07 section 9) |
| Doppler | Optional for vehicles |
| Ducking | Music bus lowers when Voice bus plays (parameter amount and time) |
| Format policy | Import converts to OGG Vorbis at 96 kbps for music and 64 kbps for SFX; mono for 3D sounds |

### C5. Audio editor tab

- Mixer view with vertical sliders per bus, mute and solo buttons, level meters.
- Clip browser with waveform preview, play button, trim handles, loop points, normalize, and a license field.
- Emitter visualization in the viewport: spheres for min and max distance when an emitter is selected.

### C6. Acceptance criteria (Part C)

1. Muzzle flash, impact, and exhaust effects each run at under 0.15 ms CPU on the reference device.
2. 2,000 particles in one emitter hold 60 fps on High with soft particles on.
3. Spawning the same effect 1,000 times causes no memory growth after the first 20 spawns (pooled).
4. Audio output latency is under 80 ms on devices that support low-latency mode.
5. The tractor engine pitch follows RPM with no clicks or pops during acceleration and gear changes.
6. With 40 sounds triggered at once the audio thread never underruns and the least important voices stop first.

---

## PART D — CONSOLE AND PROFILER (tier M for console, 1 for profiler)

### D1. Console

- Log lines with timestamp, level icon, source (engine, script file and line, patch node), and message.
- Filters: level toggles, text search, source filter, **Errors only** quick toggle.
- Tap a script line to open the Code Editor at that line; tap a patch line to focus the node.
- Commands: a command field supports engine console commands: `fps on`, `quality High`, `timescale 0.5`, `physics.debug on`, `tp x y z`, `spawn <prefab>`, `list actors`, `memory`, `clear`. Commands are available only in Debug builds of an exported game and always in the editor.
- Copy a line or share the whole log (tokens removed).

### D2. Profiler

| Graph | Content |
|---|---|
| Frame time | Stacked bars: scripts, physics, animation, render submit, GPU, audio, other. Marks frames above budget in red |
| CPU timeline | Per-thread zones for the last 300 frames, tap a frame to inspect |
| Script profiler | Time per script and per lifecycle method, call counts, top 10 |
| GPU | Draw calls, triangles, texture memory, render target memory, estimated GPU ms (via timestamp queries where supported) |
| Memory | Native heap, Java heap, GPU, asset cache, streaming chunks resident |
| Streaming | Chunks loaded and unloaded, bytes downloaded, cache hit rate |
| Physics | Active bodies, contacts, step time |
| Thermal and battery | Thermal status and battery temperature when available |

- Recording: Start and Stop, with captures saved to the project `.profile/` folder (not committed to the cloud by default).
- **Budget report:** a button that checks the current scene against Doc 01 section 6 and Doc 05 section 14 budgets and lists violations with suggested fixes (reduce textures, add LODs, merge meshes).

### D3. Acceptance criteria (Part D)

1. Console shows 10,000 lines per second of logging without dropping frames (ring buffer and batched UI updates).
2. The profiler overhead is under 3% frame time when open and zero when closed.
3. The budget report flags a scene with a 4096 by 4096 uncompressed texture and gives the memory cost.

---

## PART E — ASSET MARKET, ASSET SIDE (tier M and 1)

Cloud browsing and streaming are in Doc 11; this part defines the local asset browser.

- Grid and list views with thumbnails generated at import (256 by 256) and cached.
- Filters by type (model, texture, material, audio, animation, prefab, script, patch), tags, and license; search by name.
- Asset detail sheet: preview, size, triangle count or duration, license, author, source, used-by list (scenes and prefabs that reference it), **Replace**, **Reimport**, **Delete** (warns if used).
- **Unused assets** report to free space.
- **Add to scene** places at the viewport center on the ground; drag from the market into the viewport places at the drop point (raycast onto terrain or colliders).
- Built-in starter library (original or CC0 content only, each with a license file): primitives, tractor, car, character, tree set, rock set, building kit, props, sounds, animations.
- Asset import dialog lets the user choose compression quality and collision generation, and shows a size estimate before importing.

### Acceptance criteria (Part E)

1. Importing a 50 MB GLB completes in the background with a progress chip and the UI stays responsive.
2. The used-by list is correct after renaming and moving assets (references use UUIDs).
3. Deleting an asset used by a scene shows a warning listing every reference.
