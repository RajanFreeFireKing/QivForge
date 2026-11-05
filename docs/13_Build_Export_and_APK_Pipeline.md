# QivForge Doc 13 — Build, Export and APK Pipeline

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01 to 12, 14 · **Audience:** AI coding agents

---

## 1. Two separate build problems

1. **Building QivForge itself** (the editor app): done by the development team with Gradle and CMake (Doc 02 section 14, Doc 15).
2. **Exporting a user's game as an installable app** from inside QivForge on the phone: the subject of this document.

The old blueprints said "1-click Android APK packaging pipeline (Android NDK + Gradle)" and "APK Cloud Builder". A phone cannot practically run Gradle plus the NDK, and the cloud builder had no defined server. This document replaces that with a design that works.

## 2. Locked decision: Player plus Data export

| Decision (LOCKED) | Detail |
|---|---|
| Default export method | **Player APK template + game data pack**, assembled and signed **on the device**, fully offline |
| Why | No NDK, no Gradle, no server, no account. The engine library is already compiled inside the template |
| Optional method (v1.0, tier 1, not blocking) | **Cloud build** for Play-ready **AAB** files and for the optional AOT script backend (section 9) |
| Not allowed | Compiling C++ on the phone; downloading and loading native code at runtime (Google Play policy and security) |

## 3. Player template

The QivForge release build includes a second app artifact inside its assets: `player_template.apk`, an **unsigned** Android app that contains:

- `libqivengine.so` (release build, arm64-v8a) with the QVM, renderer, physics, audio, and no editor code.
- A minimal Kotlin `GameActivity` that creates the surface, handles lifecycle and input, and starts the engine.
- A placeholder `AndroidManifest.xml` with markers for the package name, label, version, and icon.
- An empty `assets/game/` folder where the data pack goes.
- No permissions except `INTERNET` (needed only when the game uses cloud streaming; removed by the exporter when the project uses none).

The template version equals the engine version. The exporter refuses to export a project made with a newer engine than the template.

## 4. Game data pack (`.qpk`)

The exporter bakes the project into one or more pack files placed in `assets/game/`.

### 4.1 Pack format

```
Header:  magic "QPK1", format version, index offset, index size, flags
Data:    file blobs (each independently compressed with LZ4 or Zstd, aligned to 16 bytes)
Index:   sorted table of entries: path hash (64-bit), path, offset, compressed size,
         raw size, codec, SHA-256 (optional in release), asset type, flags
Footer:  checksum of the index
```

- Large assets stay in separate pack files (`game_0.qpk`, `game_1.qpk`, ...) with a maximum of 1.5 GB each; the APK limit is 4 GB (zip limit), but Google Play delivery limits are lower (see section 10).
- The engine memory-maps packs; lookups use the hash table so opening a file costs microseconds.
- Streaming packs can be marked **remote**: they are not included in the APK and are fetched from the cloud at runtime (section 8).

### 4.2 What the baker does

| Step | Action |
|---|---|
| 1. Validate | Run all validators: scene references, missing assets, script compile, patch compile, HUD layout, license checks (Doc 14) |
| 2. Collect | Walk the scene graph and collect only used assets (dead assets are skipped) |
| 3. Textures | Convert to KTX2 with ASTC (4x4 for UI and normal maps, 6x6 or 8x8 for diffuse by quality preset) and ETC2 fallback pack chosen by device capability at runtime |
| 4. Meshes | Meshopt compression, quantized attributes, LOD generation if missing (v1.0) |
| 5. Audio | OGG Vorbis at the configured bitrate |
| 6. Scenes | Convert JSON to a binary scene format for fast loading |
| 7. Scripts and patches | Compile to `.qvc` bytecode (release mode: no debug line tables unless Debug variant) |
| 8. Materials | Include prebuilt `filamat` files only for used materials |
| 9. Terrain | Chunk files packed by region |
| 10. Navmesh | Include baked navmesh |
| 11. Pack | Write packs, index, checksum |
| 12. Report | Size per category, top 20 largest assets, warnings |

A **Size report** screen shows the result before building so the user can reduce size.

## 5. Export settings screen (Doc 04 Settings, Build and Export)

| Field | Rules |
|---|---|
| App name | 1 to 30 characters; shown under the icon |
| Package name | Reverse domain format `com.example.mygame`; regex `^[a-z][a-z0-9_]*(\.[a-z][a-z0-9_]*)+$`; default generated from a random suffix; warns that it cannot change after publishing |
| Version name and code | Name free text; code integer, auto increments each export |
| Icon | 512 by 512 PNG; the exporter generates adaptive icon layers (foreground and background) and legacy sizes |
| Splash | Background color and optional logo; uses the Android 12 splash API |
| Orientation | Landscape, portrait, or auto |
| Target quality | Low, Medium, High, Auto (Doc 05) |
| Min Android | Fixed at API 29 for v1.0 |
| Cloud streaming | On or off; if on, shows the rules in section 8 |
| Build variant | Debug (console enabled, debug symbols, signed with debug key) or Release |
| Signing | Keystore choice (section 6) |
| Output | APK (default) or AAB (cloud build only) |
| Permissions | Read-only list computed by the exporter |
| Credits and licenses | Auto-generated screen (section 7) |
| Age rating note | Text field reminding the creator about store rating requirements (Doc 14) |

## 6. Signing

- The exporter uses the **Apache-2.0 `apksig` library** (the same signing code used by Android tools) to produce **APK Signature Scheme v2 and v3** signatures. v1 (JAR signing) is added for devices below Android 7 only if min SDK required it (not needed at API 29).
- **Keystore:** QivForge can **Create keystore** (RSA 3072 or EC P-256, validity 25 years) and stores it in app-private storage, with its password protected by the Vault (Doc 14). It can **Import keystore** (PKCS12 or JKS) via SAF and **Export keystore** with a strong warning.
- A prominent notice: **if the keystore is lost, updates of the same app cannot be installed over the old version and a store listing cannot be updated.** The wizard requires the user to export a backup before the first Release export.
- Debug builds use a separate debug keystore generated on first run.
- The exporter never uploads a keystore anywhere, including the cloud build option (cloud builds produce an **unsigned** AAB or APK that the user signs on the device, or use Play App Signing).

## 7. APK assembly steps on the device

1. Copy `player_template.apk` to a temp folder.
2. **Edit the binary manifest** with a small binary XML writer library (included in the app, based on a permissive-licensed AXML editor) to set package, label, version, orientation, permissions, and optionally `android:debuggable` for Debug builds.
3. **Replace resources:** icon layers, splash color, strings. Resources are compiled ahead of time in the template as placeholders with fixed ids; the exporter replaces the binary values (no aapt2 on the device).
4. Insert packs into `assets/game/` as **stored** (not compressed) zip entries aligned to 4096 bytes so they can be memory-mapped directly (also satisfies the 16 KB alignment rules for native libraries; see section 10).
5. **Zipalign** with the built-in implementation (4 bytes for general entries, 16384 bytes for native libraries).
6. Sign with `apksig` (v2 and v3).
7. **Verify:** run `apksig` verification on the output, check that the manifest parses with the Android `PackageParser`-equivalent API (`PackageManager.getPackageArchiveInfo`), check that all pack checksums match.
8. Save to the SAF location chosen by the user and offer **Install** (using `ACTION_VIEW` with a content URI; Android requires the user to allow installs from this source, the app explains this).
9. Offer **Share** and **Open folder**.

Credits screen: the template includes a **Licenses** activity that lists engine third-party licenses and the creator's asset attributions (generated from `.meta.json` license fields). It is reachable from the game's pause menu and cannot be removed by the exporter (Doc 14).

## 8. Cloud streaming in exported games

| Case | Rule |
|---|---|
| Project in Local mode | All data in packs; no network needed; the `INTERNET` permission is not added |
| Project in Cloud mode, assets bundled | Same as Local; the repo is only used by the editor |
| Streaming from a **public** Hugging Face repo | Allowed. The pack index lists remote assets with URLs and SHA-256; the game downloads on demand into its cache, verifying hashes. `INTERNET` permission added |
| Streaming from a **private** repo | **Blocked.** A private repo needs a token and a token must never be embedded in an APK (anyone can extract it). The exporter shows: "Make the repo public, bundle assets, or use your own server" |
| Creator-provided HTTPS server | Allowed with a base URL in settings; same hash verification |

Exported games stream with the same streamer as the editor (Doc 11 section 4) including disk cache limits (default 1 GB, set by the creator), prefetch, and offline placeholders. A first-run **Download size** screen shows how many MB will be downloaded and asks for confirmation on mobile data.

Public repo limits and terms are Hugging Face's; the exporter shows a warning that a popular game could exceed rate limits and recommends bundling essential assets (the first level) in the APK.

## 9. Optional: cloud build and AOT backend (tier 1 option, not required for MVP)

Use cases: Play Store **AAB** output, optional ahead-of-time compilation of QivScript to native code for speed, and building with an updated template without updating the editor app.

| Item | Specification |
|---|---|
| Trigger | The user taps **Build in cloud** and confirms. No automatic builds (R4) |
| Backends (user chooses in Settings, adapters) | (a) The user's own GitHub repository with a workflow dispatched once per build through the API using the user's fine-grained token, (b) a Hugging Face Docker Space the user owns, (c) a self-hosted URL. No shared QivForge server is assumed |
| Input | A `.qivpack` of the project (without secrets) uploaded to the backend as a temporary artifact |
| Process | The backend job runs the `tools/` host baker, `qvsc` AOT compiler (clang for Android arm64), builds the player template with the game as a module, and outputs **unsigned** AAB and APK |
| Output | Downloaded by the app, then signed locally (section 6) |
| AOT equivalence | AOT output must pass the VM equivalence test suite (Doc 08 section 20) and fall back to bytecode for any script that fails to compile natively |
| Limits | Build jobs time out at 20 minutes; the job uploads logs to the app |
| Abuse prevention | One build at a time per project; the app rate limits to 1 build per 5 minutes; no cron or background triggers |
| Provider rules | The user is responsible for the provider's terms and quotas; GitHub workflow runs are user-triggered and cause no spam commits |

This replaces "Cloud APK Builder" in the old blueprint, and no part of it is required for the app to function.

## 10. Store and platform requirements (verify at implementation time)

The platform rules below change often; the implementation task must re-check them in current Android and Google Play documentation before release, and record the checked date in `DECISIONS.md`.

- Target API level: use the latest stable `targetSdk` that Google Play requires for new apps at export time (the template is rebuilt with each engine release).
- 64-bit only (arm64-v8a); an x86_64 emulator build is available only for Debug.
- **16 KB page size** support for native libraries (required for apps targeting recent Android versions on Google Play): the template's `.so` is linked with 16 KB alignment and verified by a build check.
- AAB is required by Google Play for new apps; APK is fine for sideloading and other stores.
- Play asset delivery is available via the cloud build option (large packs as install-time or on-demand asset packs).
- Data safety form and privacy policy: the exporter generates a **draft** list of data the game collects (none by default; INTERNET plus stream requests when streaming; AI dialogue if used) to help the creator fill the store forms (Doc 14). It is not legal advice.
- Content rating questionnaire: the creator completes it in the store console.

## 11. Debug versus Release

| Aspect | Debug | Release |
|---|---|---|
| Console and profiler | Included | Removed |
| Script debug info | Included | Stripped |
| Signing | Debug keystore | Creator keystore |
| Asset validation at load | Hash checks on | Index checksum only |
| Logging | Verbose | Warnings and errors only, no file paths |
| `android:debuggable` | true | false |
| Size | Larger | Smaller |

## 12. Testing exported games

- **Install test** on the same device: install, launch, play 2 minutes, quit.
- **Clean device test:** copy to a second device and install without QivForge present.
- **Update test:** change version code, export, install over the previous build with the same keystore, and verify data persistence of `SaveGame`.
- **Corruption test:** truncate a pack file; the game must show an error screen and not crash.
- **Size gates:** minimal template export under 90 MB; the Tractor template under 250 MB.
- **Startup time:** under 3 seconds to first frame for the Tractor template on the reference device.

## 13. Error handling

| Code | Condition | Behavior |
|---|---|---|
| QF-BUILD-001 | Validation failure (missing asset, script error) | List with links to the items; no build |
| QF-BUILD-002 | License check failure | List assets with blocked licenses (Doc 14) |
| QF-BUILD-003 | Not enough free space | Show required MB |
| QF-BUILD-004 | Signing failure (bad keystore or password) | Message with the field to correct |
| QF-BUILD-005 | Output verification failed | Delete the output, show details |
| QF-BUILD-006 | Template version mismatch | Offer to update the editor |
| QF-BUILD-007 | Private repo streaming blocked | Explanation (section 8) |
| QF-BUILD-008 | Embedded secret detected in project files | Block build, list files; user removes secrets |
| QF-BUILD-009 | Cloud build failed or timed out | Show logs; local export remains available |

The build runs in a foreground service with a progress notification so it survives the user leaving the app; progress and cancellation are always available, and a cancelled build leaves no partial output.

## 14. Security rules for export

- The exporter scans all project files for strings that look like API keys, Hugging Face tokens (`hf_...`), private keys, and passwords, and blocks the build if found (QF-BUILD-008).
- The exported game contains no QivForge account data, no tokens, and no editor telemetry.
- Packs are not encrypted in v1.0 (obfuscation is not security); an optional **obfuscate packs** switch scrambles file names and index, with a note that this does not prevent extraction.
- Native library and manifest are not modifiable by the creator beyond the settings screen.

## 15. Acceptance criteria

1. From a fresh install with no account, the user exports the Tractor template to an APK in under 90 seconds on the reference device, and installs it.
2. The exported APK runs at the same frame rate as Play mode in the editor (within 5%).
3. The APK installs on a second phone that has no QivForge, with a custom name, package, and icon.
4. The signature verifies with the Android `apksigner verify` tool on a computer.
5. Updating with a higher version code over the same package succeeds only with the same keystore, and fails with a different one (as designed).
6. A project containing a `hf_` token string fails the export with QF-BUILD-008.
7. A project in private Cloud mode with streaming enabled is blocked with QF-BUILD-007.
8. Native libraries in the APK pass the 16 KB alignment check script.
9. Deleting a pack file at random bytes shows a safe error screen and no crash or ANR.
