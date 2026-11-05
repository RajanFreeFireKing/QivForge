# QivForge Engine 🚀

> **Mobile-First 3D Game Engine & Editor for Android**  
> Developed by **Qivora** | Production Release **v1.0.0**

[![QivForge CI](https://github.com/RajanFreeFireKing/QivForge/actions/workflows/build-apk.yml/badge.svg)](https://github.com/RajanFreeFireKing/QivForge/actions/workflows/build-apk.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](legal/THIRD_PARTY.md)
[![Platform](https://img.shields.io/badge/Platform-Android%2029%2B%20%7C%20arm64--v8a-brightgreen.svg)]()
[![Vulkan](https://img.shields.io/badge/Vulkan-1.1%2B%20%2F%201.3-red.svg)]()

---

## 📱 About QivForge

**QivForge** is a complete, native 3D game engine and visual editor engineered exclusively for Android devices. Create, sculpt, script, simulate, and export production-ready 3D Android games directly from your smartphone or tablet—with zero desktop or PC required.

### 🌟 Key Features

- **High-Performance Vulkan Renderer**: Powered by Google Filament with PBR metallic-roughness materials, dynamic 3-cascade CSM shadows, PSSM, height & distance atmospheric fog, and ACES tonemapping.
- **60 Hz Deterministic Physics**: Integrated with Jolt Physics, featuring raycasts, triggers, 4-wheel vehicles (Tractor and Sports Car presets), tire friction curves, and character controllers.
- **QivScript Language & VM**: Lightweight register-based bytecode virtual machine with deterministic gas limits, hot-reload, generational handles, and seamless reflection.
- **Visual Patch & AnimGraph System**: Node-based logic graph and skeletal animation state machine supporting 1D/2D blend trees, avatar masks, and foot IK.
- **World & Level Building**: 64m x 64m terrain chunks with 16-bit heightfield sculpting, 8-layer splat texture painting, skirted LOD meshes, vegetation density scattering, and automatic navmesh baking.
- **In-Game 2D HUD Designer**: Visual UI builder with 16 responsive widgets, 9-point anchors, multi-touch pointer ownership, and 48 dp minimum touch bounds.
- **VFX & Spatial Audio**: CPU particle emitters (2,000 particles/emitter, 8,000/scene) with zero heap allocations, and 6-bus spatial audio mixer with vehicle RPM engine sound synthesis.
- **Offline & Private AI Assistant**: Multi-provider assistant (Gemini, Groq, OpenRouter, HF, Local GGUF) with prompt injection isolation, credential redaction, and EditSet diff confirmation.
- **On-Device APK & QPK Export**: 12-step project baking pipeline with dead-asset elimination, 16 KB page-size alignment for Android 15+, 16-byte aligned QPK pack generation, and APK Signature Schemes v2/v3.

---

## 📥 Download APK

Pre-built release APKs are automatically generated on every commit and release via GitHub Actions:

- **[Download Latest Release APK](https://github.com/RajanFreeFireKing/QivForge/releases/latest)**
- **[All Releases & Nightly Builds](https://github.com/RajanFreeFireKing/QivForge/releases)**
- **[GitHub Actions Artifacts](https://github.com/RajanFreeFireKing/QivForge/actions)**

---

## 🏗️ Project Architecture

```
QivForge/
├── app/                  # Android application module (Jetpack Compose UI)
│   ├── src/main/java/    # Kotlin Compose screens (Editor, Viewport, HUD, AI, Export, Licenses)
│   └── src/main/cpp/     # JNI bindings and native bridge
├── engine/               # Native C++20 Core Game Engine
│   ├── core/             # ECS, Fuzz targets, Error codes, Math
│   ├── render/           # Vulkan / Filament PBR, CSM, Post-process
│   ├── physics/          # Jolt Physics 60 Hz simulation & vehicles
│   ├── script/           # QivScript lexer, parser, compiler, bytecode QVM
│   ├── patch/            # Visual Patch node graphs
│   ├── anim/             # Skeletal animation, blend trees, state machines
│   ├── terrain/          # Heightmap sculpting, LOD skirts, vegetation, navmesh
│   ├── ui2d/             # 2D HUD designer and responsive layout
│   ├── vfx/              # Particle emitters and pool management
│   ├── audio/            # 6-bus audio mixer and spatial voices
│   ├── ai/               # AI context builder, tool router, edit sets
│   └── export/           # QPK pack builder/reader, APK assembler, signer
├── tests/                # Verification and unit test suites
│   ├── perf/             # Performance baselines & budgets (baselines.json)
│   └── unit/             # Hardening, fuzzing, rules R1-R8 unit tests
├── legal/                # Third-party notices and licenses (THIRD_PARTY.md)
├── docs/                 # Complete 15-document engineering specifications
├── DECISIONS.md          # Architecture Decision Records (D-0001 to D-0013)
├── FEATURES.md           # Specification Feature Checklist
└── SECURITY.md           # Security policy and vulnerability response SLA
```

---

## 🛠️ Building from Source

### On Android (Termux)
```bash
# Build engine core and execute full verification suite
cd qivforge
clang++ -std=c++20 -O2 \
  -Iengine/core/include -Iengine/export/include -Iengine/render/include \
  -Iengine/anim/include -Iengine/script/include \
  engine/core/src/fuzz_target.cpp engine/export/src/qpk_builder.cpp \
  engine/export/src/qpk_reader.cpp engine/render/src/gltf_importer.cpp \
  engine/anim/src/skeleton.cpp engine/anim/src/anim_graph.cpp \
  engine/anim/src/animation_clip.cpp engine/anim/src/blend_tree.cpp \
  engine/anim/src/avatar_mask.cpp engine/anim/src/foot_ik.cpp \
  engine/script/src/qvm.cpp tests/unit/hardening/test_fuzz_targets.cpp \
  -o build/test_fuzz_targets && ./build/test_fuzz_targets
```

### Android APK Build (Gradle)
```bash
./gradlew assembleRelease
```

The resulting signed APK will be generated at:
`app/build/outputs/apk/release/app-release.apk`

---

## ⚖️ License & Compliance

- QivForge is licensed under the [MIT License](LICENSE).
- All third-party libraries and licenses are documented in [`legal/THIRD_PARTY.md`](legal/THIRD_PARTY.md).
- Security policy is detailed in [`SECURITY.md`](SECURITY.md).
