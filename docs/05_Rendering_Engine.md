# QivForge Doc 05 — Rendering Engine

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01, 02, 03, 04 · **Audience:** AI coding agents

---

## 1. Goals and honest targets

The renderer must give a PBR look on mid-range phones at stable frame rates, and degrade gracefully on weaker ones. "Free Fire level" is treated as a measurable target: the v1.0 budget is 60 fps with 150k visible triangles, one directional shadow cascade set, and a post-process stack on the reference device (Doc 01 section 6). Visual quality beyond that comes from art and content, not from engine code.

## 2. Technology decision (LOCKED)

- **Google Filament** (Apache-2.0) is the rendering core: PBR materials, clustered forward lighting, shadows, image-based lighting, post-processing, skinning, morph targets, and glTF loading through `gltfio`.
- Backends: **Vulkan 1.1** primary, **OpenGL ES 3.1** fallback, chosen at start by capability detection. The engine never uses features that exist on only one backend.
- QivForge wraps Filament in a thin `render/` module so that the rest of the engine never includes Filament headers. This allows replacing the renderer later.

## 3. Render module structure

| Class | Responsibility |
|---|---|
| `RenderDevice` | Creates Filament `Engine`, `SwapChain`, `Renderer`; handles surface create and destroy |
| `RenderScene` | Holds renderables, lights, probes; mirrors the engine Scene |
| `RenderView` | One camera plus one set of post-process options; the editor has one, Play has one, mini previews have more |
| `MaterialLibrary` | Compiled `.filamat` materials and instances; caches by key |
| `TextureManager` | Loads KTX2 textures, tracks GPU memory, applies mip bias for quality presets |
| `MeshManager` | Vertex and index buffers, skinning and morph data, LOD sets |
| `DebugDraw` | Lines, boxes, spheres, text for gizmos, colliders, navmesh, grid |
| `QualityGovernor` | Watches frame time and temperature, changes settings (section 11) |
| `CaptureService` | Screenshot and video frame readback |

## 4. Frame pipeline

1. Engine main computes transforms and animation (Doc 02 section 5).
2. `RenderScene` syncs changed transforms and material parameters only (dirty flags).
3. Visibility: frustum culling (Filament), distance culling, LOD selection (section 8).
4. Shadow passes for shadow-casting lights.
5. Main color pass (clustered forward).
6. Post-process chain (section 7).
7. Debug draw and editor overlays (gizmos, outlines) if in the editor.
8. UI (HUD) drawn on top by the `ui2d` module in a separate Filament view with no post-processing.
9. Present.

The render thread is Filament's own; Engine main only enqueues work and never waits on the GPU except during `Stop` and surface destruction.

## 5. Materials

### 5.1 Supported material types

| Type | Description | Tier |
|---|---|---|
| Lit PBR (metallic-roughness) | Default; matches glTF | M |
| Unlit | Flat color or texture, for HUD in world, sky cards | M |
| Lit cutout | Alpha test for leaves and fences | 1 |
| Lit transparent | Blended glass, with sorted draw order | 1 |
| Terrain splat | Up to 8 layers blended by a weight map | 1 |
| Skybox | Procedural gradient or cubemap | M |
| Decal | Projected texture on surfaces | 1 |
| Water | Animated normals, depth tint, foam | 2 |

### 5.2 Material parameters (Inspector fields)

Base color (RGBA and texture), metallic, roughness, normal map and strength, occlusion map and strength, emissive color, emissive intensity, alpha mode (opaque, mask, blend), alpha cutoff, double sided, UV tiling and offset, and clear coat (v2).

### 5.3 Rules

- Materials are Filament `filamat` files built **offline** by a host tool (`tools/matc`). No runtime shader compilation on device (it causes hitches).
- Material instances are shared between objects when parameters match.
- Maximum 4 texture samplers per standard material to protect low-end GPUs; the terrain material uses packed textures.

## 6. Lighting and shadows

| Item | Specification |
|---|---|
| Directional light | One sun per scene with intensity (lux), color, angle, shadow cascades (1 in MVP, up to 3 in v1.0), cascade split lambda, shadow distance |
| Point and spot lights | Up to 8 shadow-casting spot lights are not supported on mobile; shadows for point and spot are off by default and allowed for at most 2 lights |
| Clustered lighting | Up to 32 dynamic lights visible per frame; extras are culled by intensity and distance |
| Shadow map size | Low 1024, Medium 2048, High 2048 with soft filtering (PCF or VSM) |
| Image-based lighting | Procedural sky generates a small cubemap and SH coefficients at load; HDR import is converted to KTX2 cubemap on import |
| Ambient | Spherical harmonics from IBL |
| Exposure | Auto exposure off by default; manual EV slider; optional simple auto exposure in v1.0 |
| Baked lighting | Not in v1.0 (documented as v2 consideration: lightmaps for static objects) |

## 7. Post-processing stack

Each effect is a toggle with parameters, stored in the `PostProcess` component or project defaults. The governor can disable effects in order.

| Order | Effect | Notes | Tier |
|---|---|---|---|
| 1 | Anti-aliasing | MSAA 4x on Vulkan High, FXAA on others, TAA is not used on mobile | M (FXAA) |
| 2 | SSAO | Half resolution, mobile quality | 1 |
| 3 | Bloom | Threshold, intensity, dirt off | 1 |
| 4 | Tone mapping | ACES-like default, Filmic and Linear options | M |
| 5 | Color grading | Exposure, contrast, saturation, temperature, tint, 3D LUT slot | 1 |
| 6 | Vignette | Intensity and smoothness | 1 |
| 7 | Fog | Distance fog and height fog, color from sky | 1 |
| 8 | Depth of field | Off on mobile unless High preset on flagship devices | 2 |
| 9 | Motion blur | Camera only, off by default | 2 |

## 8. Culling, LOD, instancing

- **Frustum culling:** per-renderable bounding spheres and boxes. Handled by Filament.
- **Distance culling:** each renderable has `maxDrawDistance`. Default is 200 m for props, 400 m for large buildings, unlimited for terrain.
- **LOD groups:** an actor can define 3 LOD meshes and switch distances. Selection uses screen-space height with hysteresis of 10% to avoid popping. GLB files may carry LOD through the `MSFT_lod` extension or a naming convention `_LOD0`, `_LOD1`, `_LOD2`.
- **Instancing:** vegetation and repeated props use Filament instancing. A scatter component can hold up to 10,000 instances per chunk, culled in chunks of 32 instances.
- **Occlusion culling:** v2 (software occlusion with a low-resolution depth buffer for large cities).
- **Static batching:** merge static meshes that share a material in a chunk at bake time to cut draw calls. Target maximum 300 draw calls per frame on the reference device.

## 9. Skinning and morph targets

- Skinning is done on the GPU with up to 4 influences per vertex and up to 128 bones per mesh on High, 64 bones on Low (the importer warns when exceeded).
- Bone matrices are uploaded once per frame per skinned mesh.
- Morph targets: up to 8 active targets per mesh (v2 for facial animation).

## 10. Terrain rendering

- Heightfield mesh generated per chunk with 3 LODs (65x65, 33x33, 17x17 vertices) and **skirts** to hide cracks between LODs.
- Normals from the heightmap; splat weights from RGBA textures (2 maps give 8 layers).
- Triplanar mapping only on steep slopes (above 45 degrees) to avoid stretching; flat areas use cheap UV mapping.
- Heightfield data is shared with Jolt's heightfield collider (Doc 06).

## 11. Quality governor and presets

### 11.1 Presets

| Setting | Low | Medium | High |
|---|---|---|---|
| Render scale | 0.7 | 0.85 | 1.0 |
| Shadow map | 1024, 1 cascade | 2048, 2 cascades | 2048, 3 cascades, soft |
| Shadow distance | 40 m | 80 m | 120 m |
| Anti-aliasing | FXAA | FXAA | MSAA 4x (Vulkan) |
| SSAO | off | off | on (half res) |
| Bloom | off | on (low) | on |
| Max dynamic lights | 8 | 16 | 32 |
| Texture mip bias | +1 | 0 | 0 |
| Particle budget | 1,000 | 3,000 | 8,000 |
| Draw distance multiplier | 0.6 | 0.8 | 1.0 |
| Vegetation density | 40% | 70% | 100% |

### 11.2 Governor algorithm

- Measure a 60-frame rolling average of frame time and read the thermal status (Android `PowerManager` thermal API).
- If average frame time is above target + 15% for 2 seconds, or thermal status is **moderate** or higher: step down one level in this order: render scale (in 0.05 steps down to 0.6), particles, vegetation density, shadow resolution, shadow cascades, SSAO, bloom, MSAA to FXAA.
- If frame time stays below target - 25% for 10 seconds and thermal is none, step back up one level, never faster than every 10 seconds.
- A visible toast appears only when the governor changes presets by a full level, and the user can lock the preset (turns the governor off).
- **Auto** preset chooses the starting preset from device capability detection: GPU model, RAM, Vulkan support, and a 2-second benchmark scene run on first launch.

## 12. Editor-specific rendering

- **Selection outline:** drawn with a stencil-based pass in `accent` color, 2 px.
- **Grid:** infinite grid shader fading with distance; size and color in Editor settings.
- **Gizmos:** drawn in a separate overlay view with depth test off; constant screen size.
- **Shading modes:** lit, unlit, wireframe (line rendering of the mesh), collision (Jolt debug draw), normals (visualized), overdraw (v1.0 debug).
- **Mini previews:** the Asset Market uses a second `RenderView` with a tiny render target (256x256) and a fixed light rig to show model previews and material balls.

## 13. Capture

- Screenshot: read back the final frame, save PNG through the Storage Access Framework.
- Project thumbnail: 512x288 JPEG at 80%, generated on Save from the editor camera.
- Video capture (v1.0): MediaCodec hardware encoder fed from the swap chain surface at 30 fps; limited to 5 minutes.

## 14. Memory and resource rules

- GPU texture memory budget: Low 256 MB, Medium 384 MB, High 512 MB. The `TextureManager` evicts least-recently-used textures and lowers mip level before failing.
- Mesh memory budget: 128 MB.
- Every GPU resource is owned by a handle type with RAII release; destroying a chunk must release all its GPU resources in the same frame (Doc 02 section 10).
- The stats badge reads from these counters.

## 15. Fallback and error behavior

- If Vulkan initialization fails, retry with OpenGL ES 3.1 and log once.
- If both fail, show a blocking screen "This device cannot run QivForge" with the reason and the diagnostics share button.
- Surface lost (backgrounding): pause rendering, keep resources; recreate swap chain on resume.
- A shader or material failing to load renders the magenta error material and logs `QF-RENDER-002`.

## 16. Scripting API surface for rendering (read by Doc 08)

```
Camera.setFov(deg), Camera.setOrthographic(size), Camera.shake(amplitude, duration)
Light.setIntensity(lux), Light.setColor(Color), Light.setEnabled(bool)
Material.setColor(name, Color), Material.setFloat(name, f32), Material.setTexture(name, asset)
RenderSettings.setQuality("Low"|"Medium"|"High"|"Auto")
RenderSettings.setPostProcess(effect, enabled)
```

## 17. Acceptance criteria

1. A scene with 150k visible triangles, 1 sun with 2 cascades, bloom, FXAA, and 8 lights holds at 60 fps for 10 minutes on the reference device on High.
2. Forcing the device into thermal throttling drops the governor to a lower preset within 5 seconds with no visible hitch beyond one frame of 33 ms.
3. Destroying a chunk reduces reported GPU memory by the chunk's size in the same frame.
4. All material types render identically on Vulkan and GLES backends within a small tolerance (golden image test with a threshold of 2% mean error).
5. Rotating the device or leaving and returning to the app never leaks the swap chain (test with 50 cycles).
6. No runtime shader compilation hitch above 5 ms in a 5-minute playthrough.
7. Terrain LOD transitions show no cracks in a flythrough capture.
