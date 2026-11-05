# QivForge Doc 01 — Vision, Scope & Locked Decisions

**Series:** QivForge Engineering Spec (15 documents) · **Parent brand:** Qivora · **Platform:** Android (local + cloud) · **Audience:** AI coding agents and human engineers

---

## 0. How an AI agent must use this document set

1. Read Doc 01 fully first. Every decision marked **LOCKED** is final. Do not re-debate it and do not substitute another technology without written approval from the owner.
2. Read the other documents in the order of section 9. Each later document assumes the earlier ones.
3. Build only what the current milestone asks for (Doc 15). Never start a later milestone early.
4. Every feature has an **acceptance criterion**. A feature is not done until its criterion passes on a real mid-range Android phone, not only on an emulator.
5. When two documents disagree, the lower document number wins, except Doc 14 (Security), which always wins.
6. If something is unspecified, choose the simplest option that satisfies the rules in section 3, write the choice into `DECISIONS.md` in the repo, and continue.

## 1. Product vision

QivForge is a mobile-first 3D game engine and editor that runs on an Android phone. A creator can build a 3D game (vehicle simulator, third-person shooter, survival game), test it instantly on the same phone, store the project locally or on Hugging Face, and export an installable APK.

| Source engine | Idea borrowed |
|---|---|
| Unreal | Node-based visual logic (Patches), animation graph, PBR viewport, pawn classes |
| Unity | Component inspector, prefabs, asset store feel |
| Godot | Small scene tree, small footprint, custom script language |

## 2. Problems found in Blueprint v1 and v2, and how they are fixed

| # | Problem in the old PDFs | Fix (now LOCKED) |
|---|---|---|
| 1 | v1 says Three.js/WebGL and v2 says C++ Vulkan, so the two contradict | One native C++ engine, one renderer stack (section 4) |
| 2 | v2 says compile QivScript with an on-device LLVM/Clang toolchain | Not allowed. QivScript compiles to bytecode that a VM inside the engine runs. Optional ahead-of-time native compile happens in the cloud build, never on device (Doc 08) |
| 3 | v2 says Rapier3D is C++ physics, but Rapier is written in Rust | Jolt Physics (C++, MIT license) |
| 4 | Path `/storage/emulated/0/QivForge/` is blocked by Android scoped storage | App-specific storage plus Storage Access Framework for export and import (Doc 11) |
| 5 | Hugging Face described as unlimited free 100 GB+ | Treated as unverified. The cloud layer must read the quota from the API and show it, and must work fully offline (Doc 11) |
| 6 | Hugging Face Commit API described as one simple POST | Real flow: commit endpoint with NDJSON operations, and Git LFS upload for large files (Doc 11) |
| 7 | Antigravity account login assumed to exist | No public third-party login is known. It is an optional experimental provider behind a feature flag, off by default (Doc 12) |
| 8 | EncryptedSharedPreferences is deprecated, and v2 mixes it with Keystore | Android Keystore wrapped keys with Tink and DataStore (Doc 14) |
| 9 | Goal of Free Fire/PUBG-level graphics in 8 milestones | Staged targets with measurable budgets (section 6). Quality is a budget, not a slogan |
| 10 | No testing, error handling, licensing, performance budgets, or build server plan | Added in Docs 13, 14, 15 |
| 11 | v1 Rule 4 says "never run bots on GitHub" but the APK builder implies CI | Cloud APK build is user-triggered and uses a documented backend (Doc 13) |

## 3. Non-negotiable rules (refined)

- **R1 Dynamic theme, no static background images.** All colors come from a runtime theme (primary, surface, panel, accent, text, error). Changing a color updates every screen immediately and persists. A theme can be exported and imported as JSON.
- **R2 The 3-dot menu exists only in the Code Editor header (top right).** It does not appear on the Home screen. Other screens use their own contextual controls, never a global 3-dot menu.
- **R3 Dual storage.** Every project is either Local or Cloud (Hugging Face). Both modes must work with no account. Switching a project from Local to Cloud is a guided migration with a progress screen and a rollback.
- **R4 Cloud operations are explicit.** No background bots, no polling loops, no automatic commits. A cloud write happens only when the user taps Save, Sync, or Publish.
- **R5 Instant on-device play.** Tapping Play shows the first rendered game frame within **300 ms** on a reference mid-range device when the scene is already loaded, and within 2 s for a cold scene of up to 200 MB. Play runs physics, animation, audio, and touch controls.
- **R6 Secrets never leave secure storage.** Tokens are never logged, never put in project files, and never sent to any host except the one they belong to.
- **R7 Crash safety.** An autosave journal protects unsaved work. After a crash the user can recover the last 30 seconds of edits.
- **R8 Accessibility.** Minimum touch target 48 dp, scalable UI text, color contrast of at least 4.5:1 for text in every theme preset.

## 4. Locked technology decisions

| Area | Decision |
|---|---|
| App shell | Kotlin, Jetpack Compose, single-activity, Material 3 as base with the QivForge dynamic theme on top |
| Engine core | C++20, Android NDK r26 or newer, built with CMake |
| Native bridge | JNI for commands and events, and a shared-memory ring buffer for high-frequency data |
| Rendering | Vulkan 1.1 primary, OpenGL ES 3.1 fallback. Use **Google Filament** (Apache-2.0) for PBR, shadows, IBL, and glTF loading instead of writing a renderer from zero |
| Physics | Jolt Physics, fixed 60 Hz step |
| Audio | Oboe for low-latency output and miniaudio for decode and 3D mixing |
| Model format | glTF 2.0 / GLB with Meshopt or Draco compression, KTX2 with Basis Universal for textures |
| Scene format | JSON for editing, with a binary cache for fast loading |
| Scripting | QivScript compiled to bytecode and run by an engine VM (Doc 08) |
| Visual logic | Patches compile to the same bytecode as QivScript so there is one runtime (Doc 09) |
| Cloud | Hugging Face Hub (Doc 11) |
| AI | Provider interface with Gemini, Groq, OpenRouter, Hugging Face Inference, local GGUF through llama.cpp, and experimental Antigravity (Doc 12) |
| Minimum device | Android 10 (API 29), 4 GB RAM, Vulkan 1.1 or GLES 3.1 |
| Reference device | Snapdragon 7-series class phone with 6 GB RAM |

## 5. Product tiers

**MVP (v0.1)** proves the whole loop on one screen: create a project, place objects, drive a vehicle, press Play, save locally.

**v1.0** is the full spec in Docs 02 to 14, with cloud, AI, patches, terrain, HUD designer, and APK export.

**v2.0** adds multiplayer, cinematics timeline, advanced terrain, and an asset marketplace.

## 6. Performance and size budgets

| Metric | MVP target | v1.0 target |
|---|---|---|
| Editor viewport frame rate | 30 fps stable | 60 fps on reference device |
| Play mode frame rate | 30 fps | 60 fps for a scene with 150k visible triangles |
| Physics step time | under 4 ms | under 3 ms at 100 active bodies |
| Cold app start | under 3 s | under 2.5 s |
| Peak RAM in Play mode | under 1.5 GB | under 1.8 GB |
| Streaming chunk load | 1 chunk in under 150 ms from local | 1 chunk in under 400 ms from network on 20 Mbps |
| Engine library size (arm64) | under 40 MB | under 60 MB |
| Thermal | no throttling in 10 min | graceful quality drop before throttling |

The engine must include an automatic **quality governor** that lowers shadow resolution, render scale, and particle counts when frame time or temperature exceeds the budget (Doc 05).

## 7. Starter templates

1. **Blank 3D World:** flat terrain, sun, skybox, one camera.
2. **Open-World Tractor and Vehicle Simulator:** terrain, 4-wheel tractor, follow camera, touch steering wheel and pedals.
3. **TPS Survival Arena:** character with joystick and fire button, enemy AI, weapon raycast, health HUD.
4. **Racing Track:** road spline, checkpoints, lap timer.
5. **Empty 2D HUD Playground** for testing the HUD designer.

Every template must open, play, and save with zero errors. These are the first automated tests (Doc 15).

## 8. Non-goals for v1.0

- Desktop or iOS editor builds. The architecture must not block them, but they are not delivered.
- Real-time ray tracing.
- Console export.
- Training or fine-tuning AI models inside the app.

## 9. Document map

| Doc | Title |
|---|---|
| 01 | Vision, Scope & Locked Decisions (this document) |
| 02 | System Architecture |
| 03 | Complete Feature Catalogue |
| 04 | UI/UX and Design System, Screen by Screen |
| 05 | Rendering Engine |
| 06 | Physics and Vehicles |
| 07 | Animation and Characters |
| 08 | QivScript Language and VM |
| 09 | Visual Patch System |
| 10 | Editor Tools: Terrain, HUD, VFX, Audio |
| 11 | Storage and Hugging Face Cloud Protocol |
| 12 | AI Agent Integration |
| 13 | Build, Export and APK Pipeline |
| 14 | Security, Permissions, Legal and Performance Governance |
| 15 | Roadmap, Testing and Agent Prompts |

## 10. Glossary

- **Actor:** any object in a scene. An actor owns components.
- **Component:** data and behavior attached to an actor, such as RigidBody or a script.
- **Pawn:** an actor that a player or AI controls (VehiclePawn, CharacterPawn).
- **Patch:** a visual node graph. Logic Patch and AnimGraph Patch are the two kinds.
- **Chunk:** a square region of the world streamed in and out of memory.
- **QVM:** the QivScript virtual machine.
- **Project root:** the app-private folder that holds one project.

## 11. Definition of done for the whole product

All five starter templates build and play for 10 minutes without a crash on the reference device; every R1 to R8 rule passes its test; Docs 02 to 14 have no unimplemented item marked MUST; the APK export installs and runs on a second device that does not have QivForge installed.

## 12. Open risks

| Risk | Mitigation |
|---|---|
| Scope is very large for an AI-agent build | Strict milestones, MVP first, one subsystem per agent task |
| Hugging Face limits or terms change | Cloud layer is an adapter, so S3-compatible storage can replace it |
| Phones overheat in long Play sessions | Quality governor and thermal API listener |
| AI agent drifts from the spec | Per-milestone checklists and the repo-level `DECISIONS.md` |
| Asset license violations | License field required on every imported asset (Doc 14) |
