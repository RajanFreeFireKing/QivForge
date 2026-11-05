# QivForge Third-Party Licenses (THIRD_PARTY.md)

This document tracks all external open-source libraries incorporated into QivForge per Doc 14 Section 8.
All libraries must conform to permissive open-source licenses (MIT, Apache-2.0, BSD, or Public Domain).
Copyleft licenses (GPL/AGPL) are strictly disallowed in client builds.

---

## 1. Core Engine Subsystems

| Library | License | Usage | Link |
|:---|:---|:---|:---|
| **Google Filament** | Apache-2.0 | PBR 3D rendering engine, glTF loader, shader compiler | [Filament Repo](https://github.com/google/filament) |
| **Jolt Physics** | MIT | 3D Rigid body & vehicle physics engine | [Jolt Repo](https://github.com/jrouwe/JoltPhysics) |
| **Google Oboe** | Apache-2.0 | Low-latency Android native audio output | [Oboe Repo](https://github.com/google/oboe) |
| **miniaudio** | MIT-0 / Public Domain | Audio decoding and 3D spatial mixing | [miniaudio Repo](https://github.com/mackron/miniaudio) |

---

## 2. Platform & App Shell

| Library | License | Usage | Link |
|:---|:---|:---|:---|
| **Android Jetpack Compose** | Apache-2.0 | Declarative mobile UI framework | [Android Jetpack](https://developer.android.com/jetpack/compose) |
| **Google Tink** | Apache-2.0 | Cryptographic key wrapping for Vault | [Google Tink](https://github.com/tink-crypto/tink-java) |
| **Kotlin Coroutines** | Apache-2.0 | Asynchronous IO and bridge event handling | [Kotlin Coroutines](https://github.com/Kotlin/kotlinx.coroutines) |
| **Ktor Client** | Apache-2.0 | HTTP client for Hugging Face & AI providers | [Ktor](https://github.com/ktorio/ktor) |

---

## 3. Optional & Tooling Modules

| Library | License | Usage | Link |
|:---|:---|:---|:---|
| **llama.cpp** | MIT | Optional on-device local GGUF AI inference (M10) | [llama.cpp](https://github.com/ggerganov/llama.cpp) |
| **Draco / Meshopt** | Apache-2.0 / MIT | 3D mesh compression and decompression | [Meshoptimizer](https://github.com/zeux/meshoptimizer) |
| **KTX-Software** | Apache-2.0 | Basis Universal texture transcoding | [KTX-Software](https://github.com/KhronosGroup/KTX-Software) |
