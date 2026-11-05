# QivForge Doc 11 — Storage and Hugging Face Cloud Protocol

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01, 02, 03, 14 · **Audience:** AI coding agents

---

## 1. Principles

1. **Two modes, one code path.** The engine reads and writes assets through an `AssetStore` interface. `LocalStore` and `HfStore` both implement it. Game code and the editor never know which one is active.
2. **Cloud is explicit (R4).** No background sync, no polling, no automatic commits. Cloud writes happen only on Save, Sync, Publish, or Migrate.
3. **Offline always works.** A Cloud project keeps a full local working copy of every file you have opened or edited. Without network you can edit, play, and save locally; the next explicit Sync pushes the changes.
4. **Never trust the quota claims.** The old blueprint said "100% free, 100 GB+". Hugging Face limits and terms can change. The app reads what the server reports, shows it, and handles errors gracefully. The cloud layer is an adapter so another backend can replace it.
5. **Secrets stay in the Vault (R6).** The Hugging Face token is never written to project files, logs, commits, or URLs.

## 2. Local storage

### 2.1 Locations (fixes the scoped storage problem)

| Purpose | Location | Notes |
|---|---|---|
| Projects | App-specific external files dir (`getExternalFilesDir("Projects")`), falling back to internal `filesDir/Projects` | No permission needed. Removed on uninstall, so export reminders are shown |
| Cache (baked data, streamed chunks) | `cacheDir/qivforge/` | May be cleared by the system; always rebuildable |
| Exports, imports, screenshots | **Storage Access Framework** (`ACTION_CREATE_DOCUMENT`, `ACTION_OPEN_DOCUMENT`, `ACTION_OPEN_DOCUMENT_TREE`) | The user picks the destination; the app keeps a persisted URI permission |
| Secrets | Android Keystore backed storage (Doc 14) | Never in the project folder |

The path `/storage/emulated/0/QivForge/Projects/` from Blueprint v1 and v2 is **not used**. A user who wants projects in shared storage chooses a folder once via the SAF tree picker; the app then works through `DocumentFile` URIs and copies data to the private working directory for engine access (the engine needs real file paths for memory mapping).

### 2.2 Project format

Defined in Doc 02 section 11. Additional rules:

- Writes are **atomic**: write to `file.tmp`, flush, rename.
- A `project.lock` file with the process id prevents two instances from editing the same project.
- Free space check before large operations; the app requires at least 200 MB free to start Play and warns below 500 MB.
- `.journal/` holds the autosave journal (R7).

### 2.3 Export and import

- **Export** (`.qivpack`): a zip containing the project folder without `.cache/`, `.journal/`, `.profile/`, with a `manifest.json` listing every file with size and SHA-256 and an asset license report. Large asset exports show a size estimate first.
- **Import:** validates the manifest, checks hashes, rejects zip path traversal (`..`, absolute paths) and total size above the free space, migrates older `formatVersion` files, and keeps a backup.

## 3. Hugging Face cloud mode

### 3.1 Repository model

- One repository per game project, of type **dataset** by default (a model repo is allowed as an option). Name pattern: `qf-<project-slug>`; the user can choose the name. Default visibility is **private**.
- Repo layout equals the project folder (Doc 02 section 11), so a Cloud project and a Local project have identical structure.
- Additional files: `README.md` (dataset card, auto generated with project name, engine version, license summary) and `.gitattributes` managed by the app for LFS patterns (`*.glb`, `*.ktx2`, `*.ogg`, `*.mp3`, `*.png`, `*.jpg`, `*.r16`, `*.bin`, `*.wav`).
- **Branches:** `main` is the working branch. Optional: `release` for published builds (v1.0).
- **Repo identification:** `hf://<username>/<repo>` is a display form only; the app stores `repoType`, `repoId`, and `revision` in `project.qivproj`.

### 3.2 Authentication

| Method | Behavior |
|---|---|
| Token paste (default) | User creates a **fine-grained token** on Hugging Face with write access limited to the repos QivForge uses (guide text shown in the app), pastes it; the app validates with `GET /api/whoami-v2` (header `Authorization: Bearer <token>`), shows username and avatar, and stores the token in the Vault |
| OAuth sign-in (v1.0, optional) | Authorization Code flow with PKCE through a Custom Tab, only if the owner registers an OAuth app; the access token is stored in the Vault with its refresh token |
| Sign out | Deletes the token locally. The UI reminds the user that the token itself must be revoked on the Hugging Face website to fully invalidate it |

Rules: tokens are sent only to `huggingface.co` hosts over HTTPS, never to CDN or redirect hosts (the HTTP client strips `Authorization` on cross-host redirects); tokens are never logged; error messages never include them; screen capture is blocked on the token entry screen (`FLAG_SECURE`).

### 3.3 API calls used

All endpoints are on `https://huggingface.co`. The app must verify each against the current Hugging Face documentation when implementing, and wrap them in the `HfClient` class so a change affects one file.

| Purpose | Call |
|---|---|
| Who am I | `GET /api/whoami-v2` |
| Create repo | `POST /api/repos/create` with JSON `{"type":"dataset","name":"qf-my-game","private":true}` (add `organization` for orgs) |
| List files | `GET /api/datasets/{repo_id}/tree/{revision}?recursive=true` (paginated; follow the `Link` header) |
| Read a file or a byte range | `GET /datasets/{repo_id}/resolve/{revision}/{path}` with optional `Range: bytes=a-b` header |
| Pre-upload check | `POST /api/datasets/{repo_id}/preupload/{revision}` with JSON listing `{path, size, sample}` for each file; the response says for each file whether to upload it as `regular` or `lfs` |
| LFS upload | Git LFS batch API for the repo, then PUT to the returned URLs, then verify; (multipart for very large files) |
| Commit | `POST /api/datasets/{repo_id}/commit/{revision}` with `Content-Type: application/x-ndjson` |
| Commit history | `GET /api/datasets/{repo_id}/commits/{revision}` |
| Delete repo | Not exposed in the app UI; the user must do it on the website (safety) |

The old blueprint described saving as "a single clean HTTP POST". The real flow has up to four steps: pre-upload check, LFS uploads for large files, then the commit call with operations. Small text files (`scene.json`, `.qvs`, `.qpatch`) are sent inline in the commit as base64 content; large binaries go through LFS first and are referenced in the commit by their SHA-256 and size.

### 3.4 Commit body format (NDJSON, one JSON object per line)

```
{"key":"header","value":{"summary":"Edit tractor scene","description":"3 files changed","parentCommit":"<sha-of-last-synced-commit>"}}
{"key":"file","value":{"path":"scenes/main_map.scene.json","encoding":"base64","content":"<base64>"}}
{"key":"file","value":{"path":"scripts/TractorController.qvs","encoding":"base64","content":"<base64>"}}
{"key":"lfsFile","value":{"path":"assets/models/tractor_heavy.glb","algo":"sha256","oid":"<sha256>","size":18234567}}
{"key":"deletedFile","value":{"path":"scenes/old.scene.json"}}
```

- `parentCommit` is the commit the local copy was last synced with; the server rejects the commit if the branch moved (conflict, section 3.7).
- One commit contains at most 100 file operations or 50 MB inline data; larger syncs are split into several commits in dependency order (assets first, then scenes), reported in the progress UI as "Commit 2 of 4".

### 3.5 Save versus Sync behavior

| Action | Local mode | Cloud mode |
|---|---|---|
| **Save** (top bar) | Writes files atomically | Writes files locally and marks them **pending** (the badge shows "Changes pending"); does not touch the network |
| **Sync** (long press on Save, or the Cloud button) | Not shown | Pushes pending files as a commit with the user's message (default "QivForge sync <date>"), then pulls remote changes |
| **Publish** | Export `.qivpack` | Optional: create a `release` branch commit and generate the APK build (Doc 13) |

This resolves a conflict in the old blueprint, which said pressing Save sends a commit. Under R4, Save never hides a network operation. A setting **Sync on Save** (off by default) exists for users who want the old one-tap behavior; it still only runs on explicit Save and never in the background.

### 3.6 Sync algorithm

1. Fetch the remote tree and the head commit id.
2. Compute a three-way comparison per file using hashes: **base** (last synced), **local**, **remote**.
3. Classify: unchanged, local only changed, remote only changed, both changed.
4. Remote only: download (queue Range-resumable downloads) and replace the local copy.
5. Local only: upload (pre-upload check, LFS, commit).
6. Both changed: conflict (section 3.7).
7. After success, store the new head commit as the base for the next sync and clear pending flags.

Resumability: every upload and download is chunked and resumable; progress is persisted so killing the app mid-sync loses nothing.

### 3.7 Conflict handling

| File type | Rule |
|---|---|
| Binary assets | The user chooses **Keep mine**, **Keep remote**, or **Keep both** (the remote copy is renamed with a suffix); no merging |
| `.qvs` scripts | Text three-way merge; if the merge conflicts, the Code Editor opens with conflict markers and a **Resolve** bar |
| `.scene.json`, `.qpatch`, `.hud.json`, `.prefab.json` | **Semantic merge** by stable ids: actors and nodes are merged individually (arrays are keyed by id), property conflicts on the same actor property are shown in a conflict list; JSON files are written with sorted keys and stable ordering so diffs stay small |
| `project.qivproj` | Field-level merge; theme and settings from the local side win unless the user chooses otherwise |

A conflict screen lists each file with a diff view; the app never overwrites a side without the user's choice.

### 3.8 Version history

- History screen lists commits with message, author, date, and file count.
- **Restore a version:** creates a new commit that restores the selected files (never rewrites history).
- **Compare:** shows changed files and a visual diff for scenes (added, removed, moved actors).

### 3.9 Quota and limits

- The Settings Cloud screen shows the **used storage** and **limit** only if the API reports them for the account; otherwise it displays "Not reported" and uses a locally computed repo size.
- Before upload, the app warns if the estimated upload exceeds what the account reports as free.
- Large files: no single file above 5 GB (LFS multipart), recommended asset size under 200 MB.
- Rate limits (HTTP 429) are handled with exponential backoff, a visible progress message, and the `Retry-After` header; the app never retries in a tight loop.
- If Hugging Face terms or limits change, only `HfClient` and the Settings text change.

## 4. Streaming assets from the cloud

### 4.1 Chunk manifests

The terrain, models, and textures are organized so that a chunk is a set of files. Each chunk has `world/chunks/c_<x>_<z>/manifest.json` listing its assets with path, size, SHA-256, and LOD level. The project has `world/index.json` with the grid size and a list of chunks with their bounding boxes and total bytes.

### 4.2 Streamer behavior (extends Doc 02 section 10)

1. Predict needed chunks from camera position and velocity (look-ahead 2 seconds).
2. Priority queue: nearest first, highest LOD first for the closest ring; low LOD of the farther ring.
3. Check the **disk cache** (cacheDir, content-addressed by SHA-256). On a hit, memory-map and load. On a miss, download.
4. Download with HTTP/2, Range requests for large GLB files (only the needed LOD mesh ranges when the GLB stores LODs as separate buffers), up to 4 concurrent requests, each with a 15-second timeout and 3 retries with backoff.
5. Verify SHA-256 after download; a mismatch discards the file and retries once, then raises `QF-CLOUD-004`.
6. Decode on job workers, create GPU resources on Engine main within a per-frame time budget of 2 ms (spread across frames to avoid hitches).
7. Unload rules from Doc 02: distant chunks are freed in the same frame GPU memory is needed; their disk cache entries remain until evicted.

### 4.3 Disk cache

- Location `cacheDir/qivforge/stream/`, content addressed.
- Size limit: default 2 GB, user adjustable from 200 MB to 20 GB, never more than 50% of free space.
- Eviction: least recently used first; files pinned for the current scene are not evicted.
- A **Prefetch scene** button downloads every asset the current scene references, so the user can play offline later; the dialog shows total size and time estimate.

### 4.4 Behavior without network

- Cached chunks load normally.
- A missing chunk shows a low-poly placeholder and a toast **Offline: some assets are not downloaded**; gameplay continues; the streamer retries when connectivity returns (using the connectivity callback, not polling).
- In Local mode `StreamHFAsset` and `AssetManager.streamModel` read from disk with identical APIs.

### 4.5 Public asset library

The Hugging Face tab of the Asset Market browses user-chosen public dataset repos (a curated list of `qivora/*` plus user-added repos). Each library repo contains an `index.json` with assets, thumbnails, license, author, and file paths. The app reads `index.json` and thumbnails only; assets download on demand. Repos without a license field are shown with a warning and can't be added to exported builds (Doc 14).

## 5. Migration between modes

### 5.1 Local to Cloud (one-tap)

1. Preconditions: token valid, network available, repo name valid and free; estimated size shown with a warning if above the account's reported free space.
2. Steps shown in a progress wizard: create repo, write `README.md` and `.gitattributes`, upload in batches (pre-upload check, LFS, commits), verify (list tree and compare hashes), switch the project to Cloud mode in `project.qivproj`.
3. Failure at any step keeps the project Local; the partial remote repo is left untouched and the user can **Resume** (the wizard remembers uploaded hashes).
4. The local files stay as the working copy. A **Free local space** option removes cached copies of large assets after a verified upload (they will stream on demand).

### 5.2 Cloud to Local

Downloads every file (the wizard shows total size), verifies hashes, switches the mode to Local, and keeps the remote repo as a backup (it does not delete it).

### 5.3 Duplicate to another repo

Fork-like copy into a new repo (for sharing a template). Creates a new repo and uploads the current head.

## 6. Error handling

| Code | Condition | User message and behavior |
|---|---|---|
| QF-CLOUD-001 | Token invalid or expired | Banner with **Reconnect**; local work continues |
| QF-CLOUD-002 | Network unavailable | Operation queued as pending; no retry loops |
| QF-CLOUD-003 | HTTP 429 or 503 | Backoff with countdown; **Cancel** button |
| QF-CLOUD-004 | Hash mismatch | Discard and retry once, then error with file name |
| QF-CLOUD-005 | Quota exceeded or repo too large (server rejection) | Explain with the server message; suggest cleaning unused assets or switching asset hosting |
| QF-CLOUD-006 | Commit rejected because the branch moved | Start sync flow (pull and merge) |
| QF-CLOUD-007 | Permission denied (token lacks write on the repo) | Explain which permission is needed |
| QF-CLOUD-008 | Repo not found or renamed | Offer to relink |
| QF-STORE-001 | Disk full | Show space needed; suggest clearing cache |
| QF-STORE-002 | Corrupt project file | Load the last journal or backup copy; report |

Every failure leaves local data intact. No operation ends in a half-written state (atomic writes and resumable transfers).

## 7. Security and privacy

- Repos are **private by default**. Making a repo public requires a confirmation dialog listing the consequences and a license choice.
- The app never uploads anything outside the project folder. `.cache`, `.journal`, `.profile` are excluded by default.
- The commit author name and email are never read from the device; commits use the token's account.
- Scripts and patches downloaded from other repos follow the untrusted rules in Doc 08 section 19.
- No analytics or telemetry go to Hugging Face from the app apart from the API calls the user triggers.

## 8. Alternatives behind the adapter (v2)

The `AssetStore` interface lets an S3-compatible bucket, Cloudflare R2, or a self-hosted server replace Hugging Face. A project stores `backend: "hf" | "s3" | "http"` in `project.qivproj`. Only `hf` is implemented in v1.0.

## 9. Test plan (feeds Doc 15)

| Area | Tests |
|---|---|
| Unit | NDJSON builder, SHA-256 and chunk hashing, three-way merge, semantic JSON merge, path validation |
| Mock server | A local fake Hugging Face server implementing the listed endpoints; tests for success, 429, 5xx, resets, partial uploads, branch moved |
| Integration (manual, real account, guarded by a test token) | Create repo, commit small files, commit LFS file, range download, restore version |
| Resume | Kill the app during upload and download; resume completes with correct hashes |
| Offline | Edit offline, reconnect, Sync, verify the remote |
| Conflict | Two devices edit the same scene; merge keeps both actors |
| Security | Token never appears in logs, crash reports, or exported packs; redirects strip auth |
| Performance | Stream 20 chunks of 5 MB at 20 Mbps with the player moving at 15 m/s without stalls over 200 ms |

## 10. Acceptance criteria

1. A project can be created, edited, and played with no account (Local).
2. With a valid token, creating a Cloud project creates a private repo and the first commit contains the template files.
3. Pressing Save in Cloud mode never uses the network (verified by a network monitor), and Sync pushes only changed files.
4. A 1 GB project uploads with LFS, survives killing the app at 50%, and resumes to completion with matching hashes.
5. Two devices editing different actors in the same scene merge cleanly; the same property edited on both shows a conflict list.
6. Streaming a 30 GB project keeps local usage under the cache limit and RAM within the budget in Doc 01 section 6.
7. Disabling Wi-Fi mid-play never crashes; missing chunks show placeholders; reconnecting resumes streaming without a user action.
8. The token cannot be found in any file in the app's data directories except the Vault storage, and is absent from `logcat` in a full test session.
9. Local to Cloud migration failing at 70% leaves the project Local and valid, and Resume completes it.
