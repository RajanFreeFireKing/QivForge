# QivForge Doc 15 — Roadmap, Testing and Agent Prompts

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01 to 14 · **Audience:** AI coding agents and the product owner

---

## 1. Why this roadmap replaced the old 8 milestones

The Blueprint had 8 milestones, each one a huge subsystem (a full renderer, a full physics engine, a compiler, all in one step). Agents drift or stall on tasks that big. This roadmap has **13 milestones (M0 to M12)** with small, testable deliverables, and an explicit **MVP cut** at M6 so there is a working product early.

Order rule: never start a milestone before the previous one passes its **exit tests** on a real device.

## 2. Milestone overview

| ID | Name | Delivers | Main docs | Tier |
|---|---|---|---|---|
| M0 | Repo, CI, toolchain | Empty app builds, CI green, DECISIONS.md, FEATURES.md | 02 | MVP |
| M1 | App shell, theme engine, Project Hub | R1 theme, Hub screen, new project modal (Local), Settings skeleton | 04 | MVP |
| M2 | Engine bring-up | libqivengine.so, Filament cube, JNI bridge, surface lifecycle | 02, 05 | MVP |
| M3 | Editor workspace | Viewport orbit, hierarchy, inspector, gizmos, undo and redo, local save and journal | 02, 03, 04 | MVP |
| M4 | Physics, vehicle and Play | Jolt, colliders, 4-wheel vehicle, touch HUD basics, Play and Stop | 06, 10 (HUD basics) | MVP |
| M5 | QivScript | Lexer to VM, @export, Code Editor with the 3-dot menu, hot reload | 08, 04 | MVP |
| M6 | Templates and MVP polish | Five templates run, assets import, audio basics, console, MVP acceptance | 01, 03, 10 | **MVP release** |
| M7 | Cloud | Vault, HF client, Sync, streaming, migration, version history | 11, 14 | v1.0 |
| M8 | Animation and Patches | Skeletal animation, AnimGraph, Visual Patches | 07, 09 | v1.0 |
| M9 | World and polish tools | Terrain, vegetation, navmesh, HUD Designer, particles, audio mixer, post-processing, shadows | 05, 10 | v1.0 |
| M10 | AI assistant | Providers, tools, diff review, local GGUF | 12 | v1.0 |
| M11 | Export | Player template, baker, signing, size report, optional cloud build | 13 | v1.0 |
| M12 | Hardening and release | Fuzzing, perf gates, accessibility, licenses, store checks | 14, 15 | v1.0 release |

v2.0 items (multiplayer, timeline, retargeting, water, advanced terrain) are listed in Doc 03 with tier 2 and are not scheduled here.

## 3. Milestone details

For each milestone: **Scope**, **Tasks**, **Exit tests**. Effort numbers are rough planning estimates for one skilled developer with an AI coding agent; adjust after measuring M1 and M2.

### M0 — Repo, CI, toolchain (about 1 week)

- Tasks: create the repo layout (Doc 02 section 3); Gradle and CMake wiring with NDK r26 or newer; ktlint, clang-tidy, unit test runners; GitHub Actions or equivalent CI building debug APK on each push; `DECISIONS.md`, `FEATURES.md` (from Doc 03), `docs/` with these 15 files; `THIRD_PARTY.md` skeleton; error code registry file.
- Exit: CI builds and runs a trivial unit test on host and an instrumented test on an emulator.

### M1 — App shell, theme, Project Hub (about 2 weeks)

- Tasks: Compose navigation; theme token model, presets, color picker with hex and RGB, contrast guard, theme export and import (Doc 04 section 2); Project Hub UI with local projects list, new project modal, project actions; project folder creation and `project.qivproj` writer; Settings skeleton with Theme and Storage sections.
- Exit: Doc 04 acceptance criteria 1, 2 (Hub part), 3, 7 pass; a project can be created and listed with no engine.

### M2 — Engine bring-up (about 3 weeks)

- Tasks: CMake for `libqivengine.so` with Filament; Vulkan with GLES fallback; surface create and destroy; JNI bridge with command channel and event ring; Engine main loop; stats badge data; a lit PBR cube with a camera; logging and error codes.
- Exit: Doc 02 acceptance 1, 3; rotate and background 50 times with no leak; 60 fps cube on the reference device.

### M3 — Editor workspace (about 4 weeks)

- Tasks: Scene model, actors, components, transforms, property tables; Hierarchy; Inspector built from property tables; viewport gestures and gizmos; selection outline; undo and redo command system; scene JSON save and load; autosave journal and crash recovery; primitives and basic lights.
- Exit: Doc 02 acceptance 2, 4, 5; Doc 04 acceptance 4 (multi-touch groundwork), 5; undo of 100 steps works; killing the app recovers work.

### M4 — Physics, vehicle, Play (about 4 weeks)

- Tasks: Jolt integration; colliders and bodies from components; fixed step with interpolation; raycasts; trigger and collision events; WheelCollider vehicle with the tractor preset; character controller; HUD runtime basics (joystick, buttons, steering wheel, pedals) with multi-touch ownership; Play, Pause, Stop with scene snapshot; physics debug draw.
- Exit: Doc 06 acceptance 1 to 4 and 6; Play start within 300 ms for a loaded scene; Stop restores the scene exactly.

### M5 — QivScript (about 6 weeks)

- Tasks: lexer, parser, semantic analysis, IR, bytecode, VM (register-based), native API binding generator, standard library, engine base classes; `@export` pipeline into the Inspector; lifecycle dispatch; instruction budget; coroutines; Code Editor with highlighting, autocomplete, diagnostics bar, quick symbol row, and the **3-dot menu** with the six items (Compile and Hot-Reload, Auto-Format, Generate Patch placeholder that reports "available after M8", Ask AI placeholder that reports "available after M10", Snippets, Find and Replace); hot reload.
- Exit: Doc 08 acceptance 1 to 7; the two reference scripts run in templates; the 3-dot menu is verified to exist only in the Code Editor.

### M6 — Templates and MVP release (about 3 weeks)

- Tasks: Blank, Tractor, TPS (without enemy AI), Racing, HUD Playground templates; asset import for GLB, PNG, JPG, audio with the baker basics; audio engine basics (3D emitter, engine pitch by RPM); console tab; profiler basics; MVP acceptance run.
- Exit: Doc 01 section 11 criteria for the MVP tier (all five templates open, play 10 minutes, save); budgets for MVP column in Doc 01 section 6; MVP release candidate.

### M7 — Cloud (about 5 weeks)

- Tasks: Vault (Doc 14 section 2); `HfClient`; auth by token; repo creation; pre-upload, LFS, commit; Sync algorithm with three-way merge and semantic merge; conflict UI; version history; streamer with disk cache and Range; prefetch; migration wizard; quota display; error codes; mock HF server for tests.
- Exit: Doc 11 acceptance 1 to 9; Doc 14 acceptance 1, 2.

### M8 — Animation and Patches (about 6 weeks)

- Tasks: skeleton import and GPU skinning; clips; state machine; blend trees; layers and masks; events; foot IK; ragdoll hand-off; `.qanim` format; Patch graph model, validator, compiler to QVM; Patch Editor UI; node catalog; script to patch and patch to script conversion; templates.
- Exit: Doc 07 acceptance 1 to 8; Doc 09 acceptance 1 to 8; the 3-dot item **Generate Visual Patch** now works.

### M9 — World and polish tools (about 8 weeks)

- Tasks: terrain chunks, brushes, splat painting, LOD and skirts, vegetation instancing, navmesh; HUD Designer and presets; particle system and effect library; audio mixer, buses, music; shadows with cascades, bloom, SSAO, color grading, fog; LOD groups, decals; quality governor and presets.
- Exit: Doc 10 acceptance for Parts A to D; Doc 05 acceptance 1, 2, 3, 6, 7; v1.0 column budgets in Doc 01 section 6 for the Tractor and TPS templates.

### M10 — AI assistant (about 4 weeks)

- Tasks: provider adapters; credentials UI; context builder; tool router with validation, dry run, EditSet, review UI; undo grouping; local GGUF module; consent cards; AI log; AI task suite and mock provider.
- Exit: Doc 12 acceptance 1 to 9; the 3-dot item **Ask AI** now works; prompt injection tests pass.

### M11 — Export (about 4 weeks)

- Tasks: Player template build; baker; pack format and reader in the engine; manifest and resource patching; zipalign; apksig signing; verification; size report; license and secret gates; install flow; optional cloud build adapter.
- Exit: Doc 13 acceptance 1 to 9.

### M12 — Hardening and release (about 4 weeks)

- Tasks: fuzz targets; perf gates and baselines; accessibility tests; licenses screen; privacy policy integration; store checks; documentation and tutorials; beta program.
- Exit: Doc 14 section 15 checklist complete; Doc 01 section 11 definition of done.

Total rough estimate: 50 to 60 weeks for one developer; realistically shorter with a team. This is the honest size of the project; the Blueprint's 8 sequential milestones understated it.

## 4. Testing strategy

### 4.1 Test pyramid

| Level | Examples | Runs |
|---|---|---|
| Unit (host) | Math, serializer, QivScript compiler, patch validator, merge, NDJSON, pack format | Every push |
| Component (host with mocks) | Scene operations, command system, streaming logic, HF client against the mock server | Every push |
| Instrumented (emulator) | JNI bridge, Compose UI tests, storage, Vault | Every push or nightly |
| Device (physical reference phone) | Rendering, physics, audio, performance, thermal, install of exports | Nightly and before each milestone sign-off |
| Golden image tests | Render fixed scenes on the device and compare with stored images (threshold 2% mean error) | Nightly |
| Soak and stress | Memory growth, streaming, Play and Stop cycles | Weekly |
| Fuzz | Parsers and importers | Weekly and before release |
| AI suite | Tool pipeline with a mock provider and real providers manually | Weekly |

### 4.2 Device matrix

| Class | Example hardware (use equivalents) | Purpose |
|---|---|---|
| Low | 4 GB RAM, entry GPU, Android 10 | Minimum spec behavior |
| Mid (reference) | 6 GB RAM, Snapdragon 7-series class, Android 13 | Budgets |
| High | 8 GB or more, flagship GPU, Android 14 or newer | High preset and headroom |
| Tablet | 600 dp or wider | Layout |
| Foldable or odd aspect ratio | 20:9 and 4:3 | HUD and safe area |

At least one device from each class must pass the Exit tests of M6, M9, and M12.

### 4.3 Definition of done for any task

1. Code builds with zero warnings in CI.
2. Unit tests added for logic; instrumented or device tests added for UI and rendering behavior.
3. The related acceptance criteria in the owning document pass.
4. Error cases and error codes implemented as specified.
5. No new TODO without an issue id.
6. Documentation updated where behavior changed, including `DECISIONS.md` if a choice was made.
7. Performance budgets not exceeded (CI perf gate).
8. Security rules from Doc 14 respected (no secrets, no broad permissions).
9. `FEATURES.md` checkbox ticked only when the acceptance criterion passes on a device.

### 4.4 Bug triage

| Severity | Definition | Response |
|---|---|---|
| S0 | Data loss, crash on launch, token exposure | Stop work, fix before anything else |
| S1 | Crash in common flow, project cannot play or save | Fix within the milestone |
| S2 | Wrong behavior with workaround | Schedule |
| S3 | Cosmetic | Backlog |

### 4.5 Release checklist (v1.0)

All items in Doc 14 section 15, plus: five templates export and run on all four device classes; documentation published; a known-issues list; rollback plan; signed release build with the offline keystore.

## 5. Agent working rules (anti-drift)

1. **Read before you build:** read Doc 01, the owning document of the milestone, and Doc 02 before writing code.
2. **One milestone at a time, one task at a time.** Each task produces a small pull request (at most about 600 changed lines excluding generated files) with tests.
3. **Ask or decide:** if a spec is silent, choose the simplest compliant option, record it in `DECISIONS.md` with date and reason, and continue. If a spec is contradictory, apply the precedence rules (Doc 01 section 0) and record it.
4. **Never silently change a LOCKED decision.** Propose the change in `DECISIONS.md` as PROPOSED and stop for owner approval.
5. **Never invent APIs.** For external services (Hugging Face, AI providers, Android), read current documentation and note the doc URL and date in code comments or `DECISIONS.md`.
6. **Do not remove tests to make a build pass.**
7. **Report honestly.** A task is done only when tests pass on a device. State what was not tested.
8. **No secrets** in code, tests, logs, or commits.
9. **Keep the layering:** no Filament or Jolt headers outside `render/` and `physics/`; no Kotlin calls from engine code.
10. **Performance first:** no allocations in per-frame loops after warm-up.

### 5.1 `DECISIONS.md` entry format

```
## D-0007 — <short title>
Date: YYYY-MM-DD
Milestone: M4
Status: ACCEPTED | PROPOSED | REJECTED
Context: <what was unspecified or contradictory>
Decision: <what was chosen>
Alternatives: <what was rejected and why>
Impact: <files and docs affected>
```

### 5.2 `FEATURES.md` format

```
# Feature checklist (generated from Doc 03)
[ ] A-001 Project Hub with project cards (M) — Doc 04 §5 — acceptance: Doc 04 AC3
[ ] A-002 New Project from template (M) — Doc 04 §5
...
```

An agent ticks `[x]` only with a link to the passing test or device run.

### 5.3 Traceability from the original five rules

| Original rule (Blueprint v1/v2) | Where it lives now |
|---|---|
| Rule 1 Dynamic theme, no static backgrounds | Doc 01 R1, Doc 04 section 2 |
| Rule 2 3-dot menu only in Code Editor | Doc 01 R2, Doc 04 section 8.2, Doc 15 M5 exit |
| Rule 3 Dual storage and secure vault | Doc 01 R3, Docs 11 and 14 |
| Rule 4 Free cloud, no bots | Doc 01 R4, Doc 11 (explicit Sync), Doc 13 section 9 |
| Rule 5 Instant Play | Doc 01 R5, Doc 06 section 2, Doc 15 M4 exit |

## 6. Prompt library for the AI coding agent

Use these prompts with Antigravity, a terminal agent, or any coding agent. Replace the bracketed values.

### 6.1 Master prompt (first message of a session)

```
You are the lead engineer for QivForge (by Qivora), a mobile-first 3D game engine for Android.
The complete specification is the folder docs/ with 15 documents (01 to 15). Read in this order:
Doc 01 (Vision, Scope and LOCKED decisions), Doc 02 (Architecture), then the documents that
the current milestone names in Doc 15.

Rules:
- Doc 01 LOCKED decisions are final. Doc 14 (Security) overrides everything.
- Work on one milestone and one task at a time; small changes with tests.
- Never invent APIs; check current official documentation for Android, Filament, Jolt,
  Hugging Face and AI providers, and record the URL and date in DECISIONS.md.
- A task is done only when its tests pass and the acceptance criteria in the owning document are met.
- Record every choice that the spec leaves open in DECISIONS.md.
- Never put secrets in code, logs or commits.

Start now with milestone [M0]. First reply with: (1) a short summary of what you will build,
(2) the list of files you will create, (3) the tests you will write. Wait for my "go".
```

### 6.2 Milestone start prompt

```
Milestone [Mx — name]. Read these documents fully: [list from Doc 15 section 2].
Produce a task breakdown of at most 12 tasks, each with: goal, files, tests, acceptance criteria
copied from the docs. Do not start coding. I will approve the order.
```

### 6.3 Task execution prompt

```
Execute task [T-id: title] of milestone [Mx].
Constraints: follow Doc 02 layering and coding standards; add unit tests; keep the change under
600 lines excluding generated code; update FEATURES.md and DECISIONS.md if needed.
When finished, report: what changed, tests added, how you verified on a device or emulator,
what you did not verify, and any spec ambiguity you resolved (with the DECISIONS.md id).
```

### 6.4 Code review prompt

```
Review the diff [paste or path]. Check against: Doc 02 layering and thread rules, Doc 14 security
rules (secrets, permissions, input validation), performance rules (no per-frame allocations,
budgets), error codes and error handling, tests and acceptance criteria of the owning document.
List findings by severity S0 to S3 with file and line, and a concrete fix for each.
Do not rewrite unrelated code.
```

### 6.5 Bug fix prompt

```
Bug: [description]. Reproduction: [steps]. Device: [model, Android version]. Logs: [redacted].
First write a failing test or a reproducible script. Then find the root cause (explain it in
three sentences). Then fix with the smallest change. Confirm the failing test passes and that
related tests still pass. Do not add workarounds that hide the problem.
```

### 6.6 Spec question prompt (agent asks the owner)

```
I need a decision. Context: [what the spec says or lacks]. Options: A) ... B) ... C) ...
My recommendation: [one option] because [reason]. Impact on schedule, risk and docs: [short].
```

### 6.7 Performance investigation prompt

```
Scene [name] misses the budget [metric: value vs target]. Capture a profile with the in-app
profiler and, if available, Perfetto or Android GPU Inspector. Identify the top three costs,
propose fixes ordered by gain per effort, implement the first, and show before and after numbers
on the reference device.
```

### 6.8 Documentation update prompt

```
Update docs/ to match the implemented behavior of [feature]. Only change the owning document
and any document that references the changed behavior. Add a DECISIONS.md entry. Do not change
LOCKED decisions without marking them PROPOSED.
```

## 7. Risk register

| ID | Risk | Likelihood | Impact | Mitigation | Owner |
|---|---|---|---|---|---|
| R-01 | Scope too large | High | High | MVP at M6, strict milestones, cut tier 2 features | Product |
| R-02 | Agent drift or contradictory output | High | Medium | Doc 15 section 5 rules, reviews, DECISIONS.md | Lead |
| R-03 | Performance below budget on low devices | Medium | High | Governor, presets, early device tests, perf gates | Engine |
| R-04 | Filament or Jolt integration problems | Medium | Medium | Spike tasks in M2 and M4; adapter layers | Engine |
| R-05 | Hugging Face limits or policy changes | Medium | Medium | Adapter, quota display, S3 option in v2 | Cloud |
| R-06 | Play Store policy changes | Medium | Medium | Re-verify at M11 and M12; APK sideload path remains | Release |
| R-07 | Thermal throttling in long sessions | High | Medium | Governor, thermal tests | Engine |
| R-08 | Token leak or insecure storage | Low | Critical | Vault, scans, fuzz, reviews | Security |
| R-09 | Asset license violations | Medium | High | License gates, library with clean assets | Product |
| R-10 | QivScript complexity delays everything | Medium | High | Start with a minimal language slice in M5 (classes, fields, lifecycle, math, engine calls); add advanced features after exit tests | Language |
| R-11 | On-device APK assembly fragile | Medium | Medium | Verify with apksigner in CI, template tests, fall back to cloud build | Export |
| R-12 | Local AI too slow on mid phones | High | Low | Optional module, clear fit and speed checks | AI |

## 8. MVP definition (restated)

The MVP (end of M6) can: create a Local project from five templates; edit scenes with hierarchy, inspector, gizmos, undo and redo; write and hot-reload QivScript with the Code Editor and its 3-dot menu; drive the tractor with touch controls and play the TPS template movement; import GLB, textures, and audio; save and recover projects; change the theme dynamically; and show console and basic profiler. It does **not** include cloud, patches, animation graphs beyond basic clip playback, terrain sculpting, AI, or export.

## 9. Ownership of tasks that are not code (owner checklist)

| Task | When |
|---|---|
| Choose and verify app and brand names (QivForge, Qivora) | Before M12 |
| Acquire or create original assets for templates and the starter library; document licenses | By M6 for MVP set, M9 for the full set |
| Lawyer review of privacy policy and terms | Before M12 |
| Create Google Play developer account and store listing material | M11 to M12 |
| Decide on a hosted backend for optional cloud builds | Before M11 |
| Recruit beta testers across the device matrix | M10 onward |
| Decide the monetization model (not covered here; ads and tracking are excluded by Doc 14 unless revised) | Before M12 |

## 10. Master prompt from the old blueprint, upgraded

The old "Ready-to-Copy Master Prompt" asked the agent to start Milestone 1 immediately with no safeguards. Use section 6.1 instead. It forces the agent to read the documents, plan, list files and tests, and wait for approval.

## 11. Final acceptance (whole project)

1. All 13 milestones have their exit tests passing on the device matrix.
2. Every row in `FEATURES.md` marked M or 1 is ticked with evidence.
3. Doc 01 section 11 definition of done is met.
4. Doc 14 section 15 release checklist is complete.
5. A person who has never seen QivForge installs it, creates a Tractor project, drives, edits a script, exports an APK, and installs it on a second phone using only the in-app guidance.
