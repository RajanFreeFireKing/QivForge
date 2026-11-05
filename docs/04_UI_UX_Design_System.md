# QivForge Doc 04 — UI/UX and Design System, Screen by Screen

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01, 02, 03 · **Audience:** AI coding agents and designers

---

## 1. Design principles

1. **Phone first, tablet better.** Every screen must be usable on a 360 dp wide phone in portrait and landscape. Tablets (600 dp and wider) get extra docked panels, not different features.
2. **Viewport is king.** In the editor the 3D viewport gets at least 55% of the screen area. Panels slide over it or dock beside it and can be hidden with one tap.
3. **One thumb.** Primary actions sit in the lower half of the screen in portrait. Destructive actions need a confirm step.
4. **No clutter.** Home has no 3-dot menu (R2). The only 3-dot menu in the app is in the Code Editor header.
5. **Everything themeable.** No hard-coded colors, no static background images (R1). Only theme tokens.
6. **Never lose work.** Every screen survives rotation, backgrounding, and process death (state is restored from the journal).

## 2. Dynamic theme engine (R1)

### 2.1 Tokens

| Token | Use |
|---|---|
| `primary` | Main buttons, selected tab, active gizmo |
| `onPrimary` | Text and icons on primary |
| `secondary` | Secondary actions, chips |
| `accent` | Highlights, selection outline in viewport, sliders |
| `surface` | App background and sheets |
| `panel` | Docked panels, cards, inspector |
| `panelAlt` | Alternating rows, input fields |
| `onSurface` | Main text |
| `onSurfaceMuted` | Hints, secondary text |
| `outline` | Dividers and borders |
| `success`, `warning`, `error` | Status colors and console lines |
| `nodeEvent`, `nodePhysics`, `nodeAnim`, `nodeCloud` | Patch node header colors (red, blue, purple, green defaults) |
| `codeKeyword`, `codeType`, `codeString`, `codeNumber`, `codeComment`, `codeError` | Syntax colors |

### 2.2 Behavior

- A Theme is a JSON object of tokens plus `name`, `mode` (dark or light), and `version`.
- Presets: **Dark** (default), **Light**, **AMOLED** (pure black surface), **Forest**, **Ocean**.
- Theme screen shows a live preview card. A color row opens a picker with a hue/saturation square, brightness slider, hex field (`#RRGGBB`), and RGB fields. Changes apply live to the whole app with no restart.
- **Contrast guard:** when the user picks a color pair with contrast below 4.5:1 (text on surface or onPrimary on primary), show a warning chip and offer **Fix automatically**, which adjusts text color.
- Themes are stored in DataStore. Export writes `theme_<name>.json`; import validates every token and ignores unknown keys.
- The viewport sky and grid colors are separate editor settings and do not change with the UI theme.

## 3. Typography, spacing, shape, motion

| Item | Value |
|---|---|
| Font | System default sans for UI, bundled monospace (JetBrains Mono, OFL license) for the code editor |
| Sizes | Title 20 sp, section 16 sp, body 14 sp, caption 12 sp, code 14 sp (user adjustable 10 to 24) |
| Spacing scale | 4, 8, 12, 16, 24, 32 dp |
| Corner radius | 8 dp cards, 12 dp sheets, 20 dp buttons |
| Touch target | minimum 48 x 48 dp |
| Motion | 150 to 250 ms, ease-out. Disabled when Reduced Motion is on |
| Icons | Material Symbols outlined, 24 dp |

## 4. Navigation map

```
Project Hub (Home)
  ├─ New Project modal ─────────────► Editor Workspace
  ├─ Project card tap ──────────────► Editor Workspace
  ├─ Settings gear ─────────────────► Settings Hub
  └─ Theme button ──────────────────► Theme screen

Editor Workspace
  ├─ Play button ───────────────────► Live Play (in place, fullscreen option)
  ├─ Script file tap ───────────────► Code Editor
  ├─ Patch file tap ────────────────► Visual Patch Editor
  ├─ Bottom Dock tabs ──────────────► Asset Market / Terrain / HUD / VFX & Audio / Console & Profiler
  └─ AI toggle ─────────────────────► AI side sheet

Settings Hub
  ├─ Cloud (Hugging Face) ──────────► Auth card, quota, repo list
  ├─ AI Providers ──────────────────► Provider list, Vault
  ├─ Theme ─────────────────────────► Theme screen
  ├─ Build & Export ────────────────► Export wizard
  └─ About, licenses, diagnostics
```

Android Back button: closes the top sheet or modal first, then goes up one level. Back on the Editor root asks **Save and exit?** only when there are unsaved changes.

## 5. Screen 1 — Project Hub (Home)

**Header:** app name "QivForge" left, Theme color button and Settings gear right. **No 3-dot menu.**

**Body (portrait):**
- Segmented filter: **All · Local · Cloud**.
- Search field.
- Project cards in a 1-column list on phones, 2 to 3 columns on tablets. Each card: thumbnail (auto-captured at last save), name, mode badge (Local with phone icon, Cloud with cloud icon), last edited, size, and cloud sync status (Synced, Changes pending, Offline).
- Long press a card: bottom sheet with Rename, Duplicate, Export .qivpack, Migrate to Cloud (or Back to Local), Delete.
- Floating action button **+ New Project**.

**New Project modal (full-screen sheet on phones):**
1. Name (required, 3 to 40 chars, unique).
2. Template chooser: five template cards (Doc 01 section 7), each with preview image generated from the template's scene.
3. **Storage mode toggle:** Local Offline or Hugging Face Cloud. Cloud requires a connected token; if none, the toggle shows **Connect Hugging Face** which opens Settings, and returns here with the form preserved.
4. Orientation (Landscape default for games), target quality preset.
5. **Create** button. Shows a progress bar while the template is copied and, for Cloud, the repository is created.

**Empty state:** illustration drawn with theme colors, text "No projects yet", and the + button highlighted.

**Errors:** name taken, no storage space (show required MB), token invalid (link to Settings), network failure for cloud (offer Create as Local instead).

## 6. Screen 2 — Editor Workspace

### 6.1 Layout (landscape phone, default)

```
+------------------------------------------------------------------------+
| TOP BAR                                                                |
+--------+---------------------------------------------+-----------------+
| LEFT   |                                             | RIGHT           |
| Hier-  |            3D VIEWPORT                      | Inspector       |
| archy  |   (gizmo, grid, selection outline)          | (tabs: Actor,   |
| (slide |                                             |  Material,      |
|  over) |                                             |  Script)        |
+--------+---------------------------------------------+-----------------+
| BOTTOM DOCK TAB BAR + collapsible content (Market, Terrain, HUD, ...)  |
+------------------------------------------------------------------------+
```

- **Portrait phone:** viewport on top (55%), a segmented switcher below it chooses Hierarchy, Inspector, or Dock content. Only one of the three is visible at a time.
- **Tablet:** Left and right panels stay docked; dock is a resizable bottom panel.
- Panels can be collapsed by swiping their edge; the viewport grows to fill.

### 6.2 Top bar (left to right)

| Control | Behavior |
|---|---|
| Back arrow | Return to Hub, asks to save if dirty |
| Project name + dirty dot | Tap to rename |
| **Save / Cloud Sync** | Local: writes to disk. Cloud: Commit to Hugging Face (explicit, shows diff size and progress). Long press shows **Commit message** field |
| Undo, Redo | Disabled state when stack is empty |
| Gizmo group | Select, Move, Rotate, Scale (radio buttons) |
| Space toggle | Local / World |
| Snap toggle | Opens a popover with grid, angle, scale steps |
| **Play / Pause / Stop** | Play starts Live Play; Pause freezes; Stop restores scene |
| Stats badge | FPS and RAM, tap to expand to frame ms, draw calls, triangles, VRAM |
| AI toggle | Opens AI side sheet (Doc 12) |

### 6.3 Viewport interaction

| Gesture | Action |
|---|---|
| One finger drag on empty space | Orbit |
| Two finger drag | Pan |
| Pinch | Zoom |
| Tap object | Select (outline in `accent`) |
| Tap empty | Deselect |
| Long press object | Context menu: Duplicate, Delete, Focus, Frame, Hide, Lock |
| Drag gizmo handle | Transform with snap if enabled |
| Double tap object | Focus camera on it |
| Three finger swipe left or right | Undo or redo |

Viewport overlay buttons (bottom right): Shading mode, Camera bookmarks (v2), Reset view, Fullscreen. Top-left shows a small axis cube that snaps the view when tapped.

### 6.4 Scene Hierarchy

- Tree of actors, indent per level, expand arrows, icon by component type.
- Per row: visibility eye and lock icons. Drag the handle to reparent or reorder. Drop onto a row makes a child; drop between rows reorders.
- Search field at top, filters live.
- Header buttons: **+ Add** (primitives, light, camera, empty, audio, particle, terrain, vehicle, character, UI canvas), Group, Collapse all.

### 6.5 Inspector

- Built from property tables (Doc 02 section 7). Sections collapse. Each property has a reset-to-default icon.
- Numeric fields: tap to type, or drag the label left and right to scrub. Sliders appear when `@range` exists. Vec3 shows three fields with XYZ colored labels.
- Header shows actor name field, active checkbox, tag dropdown, layer dropdown.
- **Add Component** button lists all component types with search.
- Script component lists all `@export` variables automatically (Doc 08).
- Material tab: texture slots with thumbnail tap to pick, color pickers, sliders for metallic, roughness, emissive intensity, alpha mode.

### 6.6 Bottom Dock (five tabs)

| Tab | Content |
|---|---|
| Asset Market | Sub-tabs: **Project**, **Built-in**, **Hugging Face**. Grid with thumbnails, search, tag chips, license badge. Tap to preview in a mini viewport. **Add to scene** places at viewport center. HF items show size and **Stream** or **Download** buttons |
| Terrain Sculptor | Brush tools and sliders (Doc 10) |
| 2D HUD Designer | Canvas with widget palette (Doc 10) |
| VFX & Audio | Particle editor and audio mixer (Doc 10) |
| Console & Profiler | Log list with level filter and search, script `print`, CPU/GPU/memory graphs |

## 7. Screen 3 — Live Play Mode

- Tapping **Play** snapshots the scene (Doc 02 section 9) and switches the viewport to the game camera within the R5 time budget.
- **Docked mode:** play inside the viewport rectangle, editor panels remain. **Fullscreen mode:** hides all editor UI, shows only the game and touch HUD, plus a small **pause chip** at the top corner that opens an overlay with Resume, Stop, Fullscreen toggle, Restart.
- **Touch HUD overlay** is drawn from the project's HUD layout (Doc 10). Controls support multi-touch: steering wheel and pedals at once, joystick and fire at once.
- **Live debug drawer:** swipe from the right edge. Tabs: Stats, Physics debug toggle (draws colliders), Console, Watch (variables), Time scale slider (0.1x to 2x).
- Rotation of the phone during play is locked to the project's orientation.
- On any script error, a red toast shows file and line; tapping it opens the Code Editor at that line (play continues).

## 8. Screen 4 — QivScript Code Editor

### 8.1 Layout

- **Header (top):** back arrow, file name with dirty dot, **Run / Compile** icon button, and the **3-dot menu at the top-right** (R2).
- **File tabs bar:** horizontally scrollable tabs for open `.qvs` files; long press for Close, Close others.
- **Editor body:** line numbers gutter, code area with syntax highlighting, bracket matching, current line highlight, indent guides, minimap (tablet only), pinch to zoom text.
- **Error diagnostic bar (bottom of editor):** shows count of errors and warnings; tap expands a list; tap an item jumps to the line. Inline squiggles under error ranges.
- **Quick symbol row above the keyboard:** `Tab`, `{ } ( ) [ ] ; : . , " = + - * / < > !`, undo, redo, and a move cursor joystick pad. This row is essential for phone typing.
- **Autocomplete popup:** appears after 2 typed characters or on `.`; lists keywords, types, engine API members, project script classes, and `@` decorators. Each item shows a short doc string. Tab or tap accepts.

### 8.2 The 3-dot menu (exactly these six items, in this order)

| # | Item | Behavior |
|---|---|---|
| 1 | **Compile & Hot-Reload Script** | Compiles current file to bytecode (Doc 08). On success while Playing, swaps the running script and keeps its state for unchanged `@export` fields. On errors, shows diagnostics and does not swap |
| 2 | **Auto-Format Code** | Re-indents, normalizes spacing and braces, preserves comments. Undoable |
| 3 | **Generate Visual Patch from Script** | Converts supported constructs to a `.qpatch` (Doc 09). Unsupported constructs become **Script Call** nodes. Opens a result sheet listing what converted |
| 4 | **Ask AI to Write/Fix Script** | Opens AI sheet pre-filled with the file and current errors. Result arrives as a diff to Accept or Reject (Doc 12) |
| 5 | **Insert Game Template Snippet** | Submenu: Vehicle controller, TPS player, Weapon, AI enemy, Pickup, Checkpoint, Health component. Inserts at cursor |
| 6 | **Find & Replace** | Bottom bar with find field, replace field, case, whole word, regex toggles, next, previous, replace, replace all |

No other screen may contain a 3-dot overflow menu.

### 8.3 Behaviors

- Autosave on pause and every 10 seconds to the journal.
- Opening a script that has compile errors shows the diagnostics immediately.
- Read-only mode for built-in template scripts with a **Duplicate to edit** button.
- Hardware keyboard shortcuts: Ctrl+S save, Ctrl+Z / Ctrl+Y, Ctrl+F, Ctrl+B compile, Ctrl+/ comment.

## 9. Screen 5 — Visual Patch Editor

- **Canvas:** infinite grid, pan with two fingers, pinch zoom (25% to 300%), minimap in the corner.
- **Mode switch (top segmented control):** **Logic** and **AnimGraph**.
- **Node palette drawer** (left edge swipe or + button): search, categories collapsible, drag a node onto the canvas or tap to place at center.
- **Nodes:** colored header by category (Event red, Physics blue, Animation purple, Cloud green; Flow control and Math have their own colors), title, input pins on the left, output pins on the right. Execution pins are white arrows; data pins are colored circles by type.
- **Wires:** drag from a pin to another pin. Bezier curve. Compatible pins glow, incompatible ones dim. Releasing on empty canvas opens a filtered node search. Tap a wire to delete with a small x.
- **Selection:** tap node; drag a box with long press on empty space; multi-select then group into a **Comment box** or **Sub-graph**.
- **Node inspector strip** at the bottom shows editable constants for the selected node.
- **Compile status chip** at top: green check, or red count; tapping lists errors such as unconnected required pins or type mismatch.
- **Debug wires in Play:** active execution wires animate with a moving dash, and data pins show their current values on tap.
- Actions: Undo/Redo, Auto-arrange, Convert to Script (opens Code Editor with generated code), Run Test (runs only this graph in a sandbox scene).

## 10. Screen 6 — Bottom Dock modules (details)

Detailed tool behavior is in Doc 10. This screen specifies only shared UI: each module has a header strip with its name, a **maximize** button (takes the full screen while keeping the Play button visible), and a **collapse** button.

## 11. Screen 7 — Settings, Cloud and AI Hub

Sections as a list on phones, as two panes on tablets.

| Section | Content |
|---|---|
| **Hugging Face** | Auth card: **Sign in with token** (paste a Write token, validated with the whoami endpoint, shows username and avatar) and **Open Hugging Face login** if OAuth is available. Quota bar (used and total, read from the API; if unavailable shows "Not reported"). Default repo visibility (Private default). **Sign out** removes the token from the Vault |
| **AI Providers** | List of providers with enable switch and key field (masked, show for 3 seconds on tap). Providers: Gemini, Groq, OpenRouter, Hugging Face Inference, Local GGUF model (pick file, shows size and speed test), Antigravity (hidden unless the Experimental switch is on). **Test connection** button per provider |
| **Theme** | Dynamic color picker (section 2) |
| **Editor** | Font size, auto-save interval, snap defaults, gizmo size, viewport grid and sky colors, haptics |
| **Graphics** | Preset Low, Medium, High, Auto; render scale slider; shadows, post-process toggles |
| **Build & Export** | Package name, version code and name, icon, splash, signing keystore (create or import), build target (APK or AAB), **Build** button |
| **Storage** | Space used per project, clear cache, move projects (SAF folder) |
| **Accessibility** | UI scale, reduced motion, high contrast |
| **About** | Version, open-source licenses list, **Share diagnostics** |

## 12. Cross-cutting components

- **Toasts and snackbars:** non-blocking, 4 seconds, with Undo where reversible.
- **Dialogs:** only for destructive or irreversible actions.
- **Progress:** determinate bars for known work (downloads, commits), indeterminate for unknown. Always with a Cancel button for work longer than 3 seconds.
- **Empty and error states:** each list view has an empty-state message; each network view has Retry.
- **Permissions:** requested only at point of use with a rationale sheet first (notifications for build completion, optional microphone for AI voice later).

## 13. Accessibility checklist

- Every icon button has a content description.
- Focus order follows reading order; hardware keyboard Tab works across panels.
- Text scales to 200% without clipping in the Inspector and Code Editor.
- Color is never the only signal (errors also have an icon and text).
- Reduced Motion removes parallax, node wire animation, and panel slide animations.

## 14. Acceptance criteria

1. Changing any theme token updates all seven screens instantly and survives app restart.
2. There is no 3-dot menu on the Hub, Editor Workspace, Patch Editor, or Settings; there is exactly one in the Code Editor header, with the six items listed.
3. On a 360 x 640 dp device in both orientations no control is clipped or unreachable.
4. Multi-touch test: steering wheel and gas pedal pressed simultaneously both register.
5. Rotating the device in the editor keeps selection, panel states, and camera.
6. A TalkBack run can create a project, add a cube, and press Play.
7. All screens pass the 4.5:1 contrast check in all five preset themes.
