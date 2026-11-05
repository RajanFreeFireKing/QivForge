# QivForge Doc 14 — Security, Permissions, Legal and Performance Governance

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01 to 13 · **Audience:** AI coding agents and the product owner

**Priority rule:** This document wins over every other document when they disagree (Doc 01 section 0). Legal items here are engineering guidance, **not legal advice**. The owner should have a qualified lawyer review the privacy policy, terms, and license choices before public release.

---

## 1. Threat model

| Asset | Threats | Main controls |
|---|---|---|
| Hugging Face token and AI API keys | Theft by malware, screenshots, logs, backups, exported APK, malicious scripts | Vault, no logging, backup exclusion, export scanner, FLAG_SECURE |
| User projects | Corruption, loss, unwanted upload | Atomic writes, journal, explicit cloud actions |
| The device | Malicious imported project (zip slip, huge files, hostile scripts), malicious downloaded assets | Validation, sandboxed VM, size limits, untrusted-content dialogs |
| The user's cloud account | Over-privileged token, accidental public repo | Fine-grained token guidance, private default |
| Players of exported games | Games that collect data or run hidden code | No telemetry by default, no runtime native code download, licenses screen |
| The product | Third-party license violation, trademark problems | License tracker, export gate |

## 2. Secrets and the Vault

### 2.1 Decision (LOCKED, replaces Blueprint v2 "EncryptedSharedPreferences")

`EncryptedSharedPreferences` from the Jetpack Security library is deprecated. QivForge uses:

- An **AES-256-GCM key generated inside Android Keystore** (hardware backed or StrongBox when available, `setUserAuthenticationRequired(false)` by default; an optional **Require device unlock** setting turns on user authentication validity of 30 seconds).
- **Google Tink** `AeadKeyTemplates` with a Keystore-managed master key to encrypt each secret blob.
- Encrypted blobs stored in **Jetpack DataStore** (file in `filesDir/vault/`). The associated data for each blob is its key name, so blobs cannot be swapped.
- If Keystore operations fail (rare vendor bugs), the app shows an error and refuses to store the secret; it never falls back to plain text.

### 2.2 Vault API

```kotlin
interface Vault {
    suspend fun put(name: VaultKey, secret: CharArray)
    suspend fun get(name: VaultKey): CharArray?     // caller must zero the array after use
    suspend fun remove(name: VaultKey)
    suspend fun clearAll()
}
enum class VaultKey { HF_TOKEN, HF_REFRESH, GEMINI_KEY, GROQ_KEY, OPENROUTER_KEY, HF_INFERENCE_KEY, KEYSTORE_PASSWORD_<id> }
```

### 2.3 Rules

1. Secrets are never stored in project files, scene JSON, `.qivproj`, logs, crash reports, analytics, intents, notifications, clipboard (except when the user taps **Copy** on their own entry, and the clipboard is cleared after 60 seconds on supported versions), or URLs.
2. Secret text fields use the password input type, disable autocomplete and suggestions, and use `FLAG_SECURE` so screenshots and recents thumbnails are blank on the Vault and token screens.
3. HTTP clients redact `Authorization` headers in all logging interceptors.
4. Android backup: `android:allowBackup="false"` for the Vault files and `dataExtractionRules` exclude `vault/` and `shared_prefs` (cloud backup and device transfer).
5. **Sign out** and **Remove key** delete the stored value and overwrite memory buffers.
6. Keys are sent only to their own provider's host (Doc 12 section 4); the HTTP client has an allow-list per provider.
7. Secret scanning runs on project save and on export (Doc 13 section 14) using patterns for Hugging Face tokens (`hf_` prefix), Google API keys, generic high-entropy strings in assignments, and PEM private keys.

## 3. Network security

- `usesCleartextTraffic="false"` and a network security config that allows only HTTPS.
- TLS 1.2 or higher using the system trust store. **No certificate pinning** to third-party providers (their certificates rotate); pinning only for a QivForge-owned endpoint if one is ever introduced.
- Timeouts: connect 10 s, read 30 s, write 60 s; downloads have separate long timeouts with resume.
- Redirects follow only HTTPS; `Authorization` is removed on cross-host redirects.
- Request size caps and response size caps for JSON endpoints (10 MB) prevent memory exhaustion.
- No WebView is used for sign-in unless OAuth is implemented, and then only Custom Tabs.

## 4. Permissions

### 4.1 Editor app

| Permission | Why | When requested |
|---|---|---|
| `INTERNET` | Cloud, AI, library browsing | Install-time (normal permission) |
| `ACCESS_NETWORK_STATE` | Detect offline for placeholders and queues | Install-time |
| `POST_NOTIFICATIONS` (Android 13+) | Build and sync completion | Only when the user first starts a long build or sync, after a rationale |
| `FOREGROUND_SERVICE` plus the proper type (`dataSync`) | Long uploads and builds | When used |
| `VIBRATE` | Haptics | Install-time |
| `REQUEST_INSTALL_PACKAGES` | Offer to install an exported APK | Only if the install feature ships; otherwise leave the user to open the file |

**Not requested:** `READ_EXTERNAL_STORAGE`, `WRITE_EXTERNAL_STORAGE`, `MANAGE_EXTERNAL_STORAGE` (broad storage access that Google Play restricts), location, contacts, microphone, camera, phone state, advertising id. File access uses app-specific storage and the Storage Access Framework (Doc 11).

### 4.2 Exported games

Computed per project (Doc 13): `INTERNET` only if streaming or AI dialogue is used, `VIBRATE` if haptics are used, nothing else by default. Any additional permission requires an explicit entry in the export settings with a reason text, and shows in the data safety draft.

## 5. Input validation and hostile files

| Surface | Rules |
|---|---|
| Zip import (`.qivpack`) | Reject entries with `..` or absolute paths, symbolic links, entries above 2 GB, total uncompressed size above 4 times the archive size or above free space, more than 100,000 entries |
| JSON | Maximum size 50 MB, maximum nesting depth 64, duplicate keys rejected, unknown fields preserved but not interpreted |
| GLB and glTF | Validate with the glTF validator rules on import, limit triangles (2 million per mesh), textures (8192 by 8192), bones (128), reject external URI references |
| Images and audio | Decoded by maintained libraries in a job worker with size and dimension caps; decode failures never crash |
| Scripts and patches | Compiled in the sandbox; a compile cannot execute code; execution budgets from Doc 08 |
| Downloads | Verified by SHA-256 against manifests (Doc 11); file names sanitized; extension allow-list |
| Deep links (if added) | Only to open a project import dialog, never to execute anything automatically |
| Clipboard JSON (patch copy and paste) | Validated like any patch file |

**Fuzzing:** the JSON loaders, GLB importer, pack reader, QivScript lexer and parser, patch loader, and zip importer have fuzz targets (libFuzzer with sanitizers) run in CI for at least 10 minutes each per release, and any crash blocks release.

## 6. Script and plugin sandbox

- QivScript runs in the QVM with no file, network, reflection, or native access except audited engine APIs (Doc 08 section 19).
- Third-party content (marketplace packs, library assets, imported projects) with scripts or patches is flagged **Untrusted**. First run shows a dialog: the list of scripts, their API usage summary (cloud, file save, AI, network), and **Run / Review / Remove**.
- Engine APIs with side effects beyond the game (`Cloud.*`, `SaveGame`, AI nodes) are gated by per-project permission switches in Project Settings, all **off** by default for imported projects.
- No dynamic native code loading anywhere in the editor or in exported games (Google Play policy and security).

## 7. Privacy

### 7.1 Principles

- **No analytics, no ads, no tracking** in v1.0. No advertising identifier is read.
- **No account with QivForge or Qivora** is required.
- Data goes only where the user directs it: Hugging Face (their repos), AI providers (their keys), asset libraries they add.
- **Crash reporting is opt-in** (tier 2): an off-by-default switch; the report preview shows exactly what would be sent; tokens and paths are redacted.

### 7.2 Data inventory (for the privacy policy and store forms)

| Data | Where it lives | Leaves the device when | Purpose |
|---|---|---|---|
| Projects | Device storage | User syncs to their HF repo or shares an export | Core function |
| HF token and AI keys | Vault | Sent to their own provider on requests | Authentication |
| AI prompts and context | Memory and `.journal` AI log | User sends an AI request to a cloud provider | AI assistant |
| Diagnostics zip | Created on demand | Only if the user shares it | Support |
| Streaming requests | Network | Exported games streaming from a repo or server | Asset delivery (the host sees IP address and requested URLs) |

### 7.3 Consent flows

- First use of Cloud: card explaining that files go to the user's Hugging Face account and that repos are private by default.
- First use of each AI provider: consent card (Doc 12 section 10).
- Exported games with streaming or AI dialogue: first-run consent screen (text template provided; the creator must review it).
- A **Privacy** screen in Settings lists these, shows what is enabled, and has **Delete all local secrets**.

### 7.4 Policy documents to produce (owner task, templates provided in `legal/`)

Privacy Policy, Terms of Use for the editor, and a game-creator guide describing creators' responsibilities (their games' privacy policy, age rating, content). The templates are drafts that need legal review.

## 8. Children and content

- QivForge is a general creation tool. The default listing is **not designed for children under 13** and the policy says so; this avoids child-directed data obligations (COPPA, GDPR-K, Google Play Families policy). If the owner wants a children's audience, a separate legal review is required before any change.
- Exported games are the creator's responsibility for content rating; the exporter shows a reminder and links to the store rating process.
- The AI assistant and AI dialogue feature use provider safety settings where available, and the in-game dialogue output is length and character filtered; creators are responsible for content moderation in their games.
- The built-in asset library contains no violent gore or sexual content; weapons are generic and fictional.

## 9. Licensing and intellectual property

### 9.1 Third-party software (verify each license text and version at integration; keep the table in `legal/THIRD_PARTY.md`)

| Component | Expected license | Obligation summary |
|---|---|---|
| Filament | Apache-2.0 | Include license and NOTICE; state changes |
| Jolt Physics | MIT | Include copyright and license text |
| Oboe | Apache-2.0 | Include license and NOTICE |
| miniaudio | Public domain or MIT-0 (dual) | No obligation; attribution appreciated |
| Draco | Apache-2.0 | Include license |
| meshoptimizer | MIT | Include license |
| Basis Universal transcoder | Apache-2.0 | Include license |
| nlohmann/json or simdjson | MIT or Apache-2.0 | Include license |
| llama.cpp | MIT | Include license; model licenses are separate (section 9.3) |
| Google Tink | Apache-2.0 | Include license |
| apksig | Apache-2.0 | Include license |
| JetBrains Mono | SIL OFL 1.1 | Ship license; do not sell font alone |
| Jetpack and Material components | Apache-2.0 | Include license |
| Kotlin and kotlinx libraries | Apache-2.0 | Include license |

The build generates an **Open-source licenses** screen from this table automatically, and the Player template includes the subset it contains. The release checklist fails if a dependency lacks an entry.

**Rule:** no GPL or AGPL code is linked into the editor or Player. LGPL only by dynamic linking with replacement allowed, and only after review. Rapier, if ever added (Rust, Apache-2.0 and MIT), would need a separate decision.

### 9.2 Asset licenses

Every asset has `license` metadata (Doc 02 section 10): SPDX-like id, author, source URL, attribution text.

| License class | Export behavior |
|---|---|
| Original (created by the user) and CC0 | Allowed |
| CC-BY | Allowed; attribution auto-added to the credits screen |
| CC-BY-SA | Allowed with a warning about share-alike; credits entry; the creator must understand the effect on their game's assets |
| CC-BY-NC, CC-BY-ND, NC or ND variants | **Blocked** in Release export unless the user confirms a personal or non-commercial build with a visible warning |
| Royalty-free commercial licenses with proof (user-entered) | Allowed after confirmation; the license text must be attached |
| **Unknown or empty** | Blocked until the user chooses a license or confirms the asset is their own work |
| AI-generated | License follows the generating model's terms (stored in metadata); unknown means blocked until confirmed |

The importer never guesses a license. The built-in library ships only CC0 or original content with license files.

### 9.3 AI model licenses

- Local GGUF models have their own licenses (some restrict commercial use or output use). The app shows the license field read from the GGUF metadata when present and asks the user to confirm they comply. The app never bundles a model by default.
- Output from AI providers is subject to each provider's terms; the app explains this on the consent card but gives no legal conclusions.

### 9.4 Trademarks and likeness

- The product must not use names, logos, characters, or maps of existing games (for example Free Fire or PUBG) in its UI, templates, or marketing. The old blueprint mentions them only as quality targets; the new spec uses measurable budgets instead (Doc 01 section 6).
- Templates use generic fictional content.
- "Unreal", "Unity", and "Godot" appear only as nominative references in documentation, not in the UI.
- The name **QivForge** and **Qivora** should be checked for conflicts in the owner's markets before public launch (owner task).

### 9.5 Creator rights

The Terms state that creators own their projects and games, QivForge claims no rights over them, and templates and built-in assets licensed as above may be used in commercial games without attribution when CC0, or with credits screen attribution when CC-BY.

## 10. Hugging Face and provider terms

- The owner and creators must follow Hugging Face's terms of service and content policy when storing assets; the app shows a short reminder at repo creation and links to the policy.
- Using Hugging Face as a delivery network for exported games may be subject to rate limits or acceptable-use rules; the app shows a warning (Doc 13 section 8) and provides the adapter for other storage (Doc 11 section 8).
- The old blueprint's claims about "unlimited" and "100% free" storage must not appear in the app, store listing, or marketing text.

## 11. Secure development lifecycle

| Practice | Rule |
|---|---|
| Code review | Every change reviewed by a human or by a second AI pass using the review prompt in Doc 15; security-sensitive folders (`vault/`, `cloud/`, `ai/`, `export/`) require human review |
| Static analysis | Kotlin: ktlint and Android Lint with security checks; C++: clang-tidy, AddressSanitizer, UBSan, ThreadSanitizer in CI test runs |
| Dependency control | Pinned versions, lock file, Dependabot-style update checks monthly, license scan on each update |
| Secrets in repo | git-secrets or equivalent pre-commit and CI scan; no keys in source |
| Release signing | Release keystore stored offline; CI signing uses a protected secret store |
| Vulnerability response | A `SECURITY.md` with a contact; fix timeline targets (critical within 7 days) |
| Build reproducibility | Same commit and toolchain produce a byte-identical engine library where practical |

## 12. Performance governance (budgets are law)

Budgets from Doc 01 section 6 and Doc 05 section 14 are enforced automatically.

| Gate | Metric | Failure action |
|---|---|---|
| Nightly perf run on the reference device (or a device farm) | Play frame time p95 for each template, physics step, startup time, RAM peak | Fail the build if a metric regresses more than 10% from the baseline or exceeds the budget |
| Size gate | Engine library size, minimal export size | Fail above the budget |
| Memory leak test | 30 minute soak with chunk streaming, scene switching, and 50 Play and Stop cycles; RAM growth under 50 MB | Fail |
| Thermal test | 10 minute Play session; the governor must keep frame time within 20% of target | Report |
| Battery | Drain per hour reported for each template (informational) | Report |
| Budget report tool (Doc 10 section D2) | Runs in CI against all templates | Fail on violations |

Baselines are stored in `tests/perf/baselines.json`; updating a baseline requires a written reason in `DECISIONS.md`.

## 13. Accessibility and inclusion (R8 enforcement)

Automated checks (contrast, touch size, content descriptions) run on all screens in all theme presets with Compose accessibility tests and the Accessibility Test Framework; failures block release.

## 14. Incident and data handling

- No user data is held by the owner, so the main incident type is a security bug in the app. The `SECURITY.md` process applies.
- If a vulnerability could expose Vault contents, the release notes instruct users to revoke their tokens at the providers.
- **Delete all local data** in Settings clears projects (with confirmation), caches, vault, and settings.

## 15. Release checklist (security, legal, governance)

1. Third-party license table complete; licenses screen generated.
2. Privacy policy and terms published (owner, lawyer reviewed).
3. Data safety form matches the data inventory (section 7.2).
4. No broad storage permission in the manifest.
5. `allowBackup` and extraction rules verified; Vault excluded.
6. Secret scan clean on the repo, the APK, and the exported sample games.
7. Fuzz targets ran with no crashes.
8. Perf, size, memory, and accessibility gates green.
9. Built-in assets all have license files.
10. No references to third-party game names or marks in UI or templates.
11. Play requirements re-verified and recorded (Doc 13 section 10).

## 16. Acceptance criteria

1. A scan of app data, logs, `logcat`, crash reports, and exports after a full session with tokens entered finds no token or key in plain text.
2. The Vault screen and token entry screen show a blank thumbnail in recents and cannot be screenshotted.
3. Hostile import files (zip slip, giant JSON, bad GLB, deep nesting) are rejected with clear messages and no crash.
4. An imported project with scripts shows the untrusted dialog before any script runs, and Cloud, AI, and SaveGame APIs are disabled until allowed.
5. An asset with unknown license blocks Release export with the listed assets.
6. The manifest has no storage, location, contacts, microphone, or camera permissions.
7. The licenses screen lists every shipped third-party component with its license text.
8. The nightly perf gate fails when a deliberate regression of 15% is introduced.
9. A reviewer can find the privacy-relevant data flows in section 7.2 and verify each with a network trace.
