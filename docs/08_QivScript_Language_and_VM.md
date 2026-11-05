# QivForge Doc 08 — QivScript Language and VM

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01 to 07 · **Audience:** AI coding agents

---

## 1. Goals and locked decisions

QivScript (`.qvs`) is a statically typed, class-based, compiled-to-bytecode language designed for phone keyboards and for game logic.

| Decision (LOCKED) | Detail |
|---|---|
| Execution | Source compiles to **QVM bytecode**. The QVM is a register-based virtual machine written in C++ inside the engine |
| No on-device native compiler | The old plan of LLVM/Clang on the phone is rejected (Doc 01). An optional **AOT native backend** exists only in the cloud build for release exports (Doc 13) and must produce identical behavior |
| One runtime | Visual Patches compile to the same bytecode (Doc 09) |
| Performance target | The QVM executes at least 50 million simple instructions per second on the reference device; a typical vehicle script (about 60 lines in `onPhysicsStep`) costs under 0.05 ms |
| Safety | Scripts run in a sandbox: no file, network, or OS access except through engine APIs |
| Hot reload | Supported during Play (section 12) |

## 2. Lexical structure

- Encoding: UTF-8. Line endings `\n` or `\r\n`.
- Comments: `// line` and `/* block */` (block comments nest).
- Identifiers: letters, digits, underscore, not starting with a digit. Case sensitive.
- Statements end with `;`. Blocks use `{ }`.

### Keywords

`module import pub private protected class interface enum struct extends implements override fn var let const if else while for in do switch case default break continue return async await yield spawn new self super true false null is as static event emit try catch throw`

### Literals

| Type | Examples |
|---|---|
| Integer | `42`, `0xFF`, `1_000_000` |
| Float | `3.14`, `1e-3`, `2.0f` (suffix optional) |
| Bool | `true`, `false` |
| String | `"hello"`, with escapes `\n \t \" \\ \u{1F600}`, and interpolation `"speed {speedKmh} km/h"` |
| Null | `null` (only for nullable types) |
| Array | `[1, 2, 3]` |
| Map | `{"a": 1, "b": 2}` |

## 3. Types

### 3.1 Primitive types

`bool`, `i8 i16 i32 i64`, `u8 u16 u32 u64`, `f32`, `f64`, `string`, `void`. Default integer is `i32`; default float literal is `f32`.

### 3.2 Engine value types (structs, copied by value)

`Vec2`, `Vec3`, `Vec4`, `Quat`, `Color`, `Transform`, `Ray`, `RayHit`, `Rect`, `Bounds`, `Mat4`.

### 3.3 Engine reference types (classes, handles)

`Actor`, `Component`, `RigidBody`, `WheelCollider`, `SkeletalAnimator`, `AudioEmitter`, `ParticleSystem`, `Camera`, `Light`, `Material`, `UIWidget`, `Asset`, `ModelAsset`, `AudioAsset`, `TextureAsset`.

### 3.4 Composite types

| Type | Syntax | Notes |
|---|---|---|
| Fixed array | `Array<T, N>` | Size known at compile time |
| Dynamic list | `List<T>` | Methods: `add`, `removeAt`, `remove`, `insert`, `clear`, `count`, `contains`, `indexOf`, `sort`, `first`, `last` |
| Map | `Map<K, V>` | Keys are string or integer types |
| Set | `Set<T>` | |
| Nullable | `T?` | Must be checked before use; `x ?? default`, `x?.member` |
| Function type | `fn(f32, i32) -> bool` | For callbacks |
| Tuple | `(f32, f32)` | Returned from functions, destructured with `var (a, b) = f();` |

### 3.5 Type rules

- Static typing with **local type inference**: `var x = 5;` infers `i32`. Public members must declare types.
- Numeric conversions: widening is implicit (`i32` to `i64`, `i32` to `f32`), narrowing requires `as` (`var n = f as i32;`).
- No implicit conversion between bool and numbers.
- Reference types may be `null` only when declared `T?`.
- Null access on a non-nullable value cannot occur by construction; on a nullable it is a compile error unless checked.

## 4. Declarations

### 4.1 Module and import

```
module Game.Vehicles;
import QivForge.Core.*;
import QivForge.Physics.*;
```

One module per file. The module path need not match the folder, but the editor suggests it. Imports are resolved at compile time; circular imports are errors.

### 4.2 Classes

```
@component("Heavy Tractor Physics")
pub class TractorController extends VehiclePawn {
    @export @range(800.0, 6000.0)
    pub var engineTorque: f32 = 3200.0;

    private var rb: RigidBody;

    override async fn onAwake() -> void { ... }
    override fn onPhysicsStep(fixedDt: f32) -> void { ... }
}
```

- Single inheritance from an engine base class or another script class. Interfaces allow multiple `implements`.
- Visibility: `pub` (visible to other scripts and the Inspector if also `@export`), `private` (class only), `protected` (class and subclasses). Default is `private`.
- `static` members belong to the class; they are not serialized in scenes.
- `const` fields are compile-time constants.
- `struct` types are user-defined value types with fields and methods (no inheritance).
- `enum` types: `enum Gear { Neutral, Drive, Reverse }`, shown as dropdowns in the Inspector when exported.

### 4.3 Functions

```
fn name(a: f32, b: i32 = 2) -> f32 { return a * b; }
```

Default parameters are allowed. Named arguments at call sites: `raycast(ray, maxDistance = 300.0)`. Lambdas: `(x) => x * 2`. Overloading is not supported (keeps compile errors clear); use defaults.

### 4.4 Variables

`var` (mutable), `let` (immutable binding), `const` (compile-time). Fields declared `pub` and `@export` are serialized in the scene.

## 5. Decorators (attributes)

| Decorator | Target | Effect |
|---|---|---|
| `@component("Display name")` | class | Registers the class so it appears in Add Component |
| `@export` | field | Shows in Inspector and serializes in the scene |
| `@range(min, max)` | numeric field | Inspector slider; also clamps on set |
| `@step(value)` | numeric field | Inspector step size |
| `@header("Text")` | field | Group heading in Inspector |
| `@tooltip("Text")` | field | Hover help |
| `@readonly` | field | Shown but not editable |
| `@hidden` | field | Serialized but not shown |
| `@require(ComponentType)` | class | Editor adds the required component automatically |
| `@executeInEditor` | class | `onTick` runs in the editor (use sparingly) |
| `@fast` | method | Event handler runs during the physics step (Doc 06 section 7) |
| `@deprecated("msg")` | any | Compile warning on use |

Exported field types allowed: bool, integers, floats, string, Vec2, Vec3, Color, enum, Actor, Component types, Asset types, `List<T>` of those, and `Array<T,N>`. Unsupported exported types are a compile error with a clear message.

## 6. Statements and expressions

- Control flow: `if / else`, `while`, `do while`, `for (var i: i32 = 0; i < n; i++)`, `for (item in list)`, `switch` (no fall-through; each case ends with implicit break), `break`, `continue`, `return`.
- Operators (highest to lowest precedence): member `.` `?.`, call and index, unary `! - ~ ++ --`, `as`, `* / %`, `+ -`, `<< >>`, `< <= > >= is`, `== !=`, `&`, `^`, `|`, `&&`, `||`, `??`, ternary `?:`, assignment `= += -= *= /= %=`.
- Vector math operators are defined: `Vec3 + Vec3`, `Vec3 * f32`, `Quat * Vec3` (rotate), `Quat * Quat`.
- Division by zero for integers throws `DivideByZero` (see errors); for floats yields infinity or NaN.
- Integer overflow wraps.
- String concatenation with `+` and interpolation with `{expr}`.

## 7. Engine lifecycle methods

| Method | When | Notes |
|---|---|---|
| `onAwake()` | Once when the actor is created in Play, before any `onStart`. May be `async` | Cloud and asset loading here |
| `onStart()` | Once before the first update, after all `onAwake` finished | Safe to find other actors |
| `onTick(dt: f32)` | Every frame | Camera, animation parameters, effects, input reading |
| `onLateTick(dt: f32)` | After all `onTick` | Camera follow |
| `onPhysicsStep(fixedDt: f32)` | Fixed 60 Hz before the physics step | Forces, motor torque |
| `onCollisionEnter/Stay/Exit(c: Collision)` | After physics | Doc 06 |
| `onTriggerEnter/Stay/Exit(other: Actor)` | After physics | |
| `onAnimationEvent(name: string, payload: EventPayload)` | When an animation event fires | Doc 07 |
| `onEnable()`, `onDisable()` | Active state changes | |
| `onDestroy()` | Before the actor is removed | Release resources |
| `onValidate()` | Editor only, when an exported field changes | |

The order within a frame is defined in Doc 02 section 5. Exceptions inside a lifecycle method are caught per script (section 14).

## 8. Coroutines and async

- `async fn` returns when its work is complete; `await` suspends the script until a task finishes without blocking the frame.
- Awaitable engine tasks: `AssetManager.streamModel(path)`, `AssetManager.streamTexture`, `Time.wait(seconds)`, `Time.nextFrame()`, `Cloud.downloadAsset(...)`, `Scene.load(name)`.
- `spawn expr;` starts a coroutine without awaiting it: `spawn flashMuzzle();`.
- `yield` is only allowed in `async fn` that return `Iterator` for step-by-step sequences.
- When an actor is destroyed, its coroutines are cancelled automatically.
- Cancelled awaits throw a `Cancelled` signal that is never shown as an error.

## 9. Events and messaging

```
event onHealthChanged(current: f32, max: f32);      // declare in a class
emit onHealthChanged(health, maxHealth);            // raise
other.onHealthChanged += handler;                   // subscribe
other.onHealthChanged -= handler;                   // unsubscribe
actor.sendMessage("applyDamage", 35.0);             // dynamic message by name
EventBus.publish("LapCompleted", payload);          // global
```

`sendMessage` calls a public method with the given name if it exists with a compatible signature; otherwise nothing happens and a debug log is written (not an error).

## 10. Standard library

| Namespace | Contents |
|---|---|
| `Math` | `abs, sign, min, max, clamp, lerp, inverseLerp, smoothstep, sin, cos, tan, asin, acos, atan, atan2, sqrt, pow, exp, log, floor, ceil, round, fract, pi, deg2rad, rad2deg, moveTowards, repeat, pingPong` |
| `Random` | `range(min, max)`, `int(min, max)`, `unit()`, `insideUnitSphere()`, `seed(n)`, `chance(p)` |
| `Time` | `deltaTime`, `fixedDeltaTime`, `time`, `timeScale`, `frameCount`, `wait(s)`, `after(s, fn)`, `every(s, fn)` |
| `String` | `length, substring, indexOf, contains, startsWith, endsWith, replace, split, join, trim, toUpper, toLower, format` |
| `Debug` | `log, warn, error, drawLine, drawSphere, drawBox, drawText, assert` |
| `Input` / `TouchHUD` | `getAxis, getJoystick, isPressed, justPressed, justReleased, getTouch, gyro, accelerometer, vibrate(ms)` |
| `Scene` | `find(name)`, `findWithTag(tag)`, `findAll(tag)`, `instantiate(prefab, pos, rot)`, `destroy(actor, delay)`, `load(name)` |
| `SaveGame` | `setInt/Float/String/Bool(key, v)`, `get...`, `has`, `delete`, `save()`, `load()` |
| `Cloud` | `downloadAsset`, `isOnline`, `projectMode` |
| `Audio` | `play(asset, pos, volume)`, `setBusVolume(bus, v)`, `playMusic(asset, fade)` |
| `VFXManager` | `spawn(effect, pos, rot)`, `spawnImpact(name, point, normal)` |
| `CameraManager` | `follow(target, offset, smooth)`, `screenCenterRay()`, `shake(a, d)`, `setActive(camera)` |
| `PhysicsWorld` | Doc 06 section 14 |

All standard library calls are implemented natively; they validate arguments and never crash.

## 11. Compilation pipeline

```
.qvs source
  -> Lexer (tokens with line and column)
  -> Parser (AST, recovers after errors to report up to 50 diagnostics)
  -> Semantic analysis (name resolution, types, nullability, decorator checks, async rules)
  -> Intermediate Representation (typed, SSA-like)
  -> Optimizer (constant folding, dead code removal, inlining of tiny functions, loop invariant hoisting)
  -> QVM bytecode (.qvc cached by content hash)
  -> Link with engine API table (binding by stable ids, not by names, at load time)
```

- Compile happens on the device in a job worker. A 500-line script compiles in under 100 ms on the reference device.
- Output `.qvc` files are cached in `.cache/`; they are never part of the project source and can be deleted safely.
- Diagnostics have: severity, code (`QS0001` style), message, file, line, column, length, and an optional **fix hint** (for example "Did you mean `getComponent<RigidBody>()`?"). They drive the error bar, squiggles, and the AI Fix button.
- The compiler also produces the **Inspector metadata** (exported fields, ranges, tooltips) so the editor can show them without running the script.

## 12. Hot reload

- Triggered from the 3-dot menu item **Compile & Hot-Reload Script** or Ctrl+B.
- Steps: compile the changed file; if there are errors, stop (the running version continues). If success: pause the script's lifecycle for the current frame, create new class metadata, **migrate instances**: copy over values for fields with the same name and compatible type, apply new defaults for new fields, drop removed fields, keep coroutines only if their function signature did not change (else cancel them with a toast), call `onValidate`, resume.
- If the class shape changes in an incompatible way (base class changed, field type changed), the affected instances are re-created and a warning is shown.
- Hot reload during Play is limited to files under 500 lines of change; larger changes recommend Stop and Play.

## 13. QVM design (for engine implementers)

### 13.1 Machine model

- **Register-based**, 256 virtual registers per call frame, 32-bit instruction words with an 8-bit opcode and three 8-bit operands, or one 16-bit and one 8-bit operand. Constants live in a per-function constant pool.
- **Value representation:** 64-bit tagged values: i32, i64, f32, f64, bool, vector types are stored in special wide registers (Vec3 uses three consecutive f32 registers), references are 32-bit handle ids plus a generation counter to detect stale references.
- **Memory management:** reference-counted script objects with cycle detection run at frame end budgeted to 0.2 ms; engine objects are accessed through generational handles so a destroyed Actor can never be dereferenced (access returns a clear `DestroyedObject` error).
- **Call frames:** up to 64 deep; a deeper call raises `StackOverflow`.
- **Instruction budget:** each script call has a budget of 2 million instructions per frame. Exceeding it aborts that call and shows `QS-RUN-002 Script took too long` with the line number. This prevents infinite loops from freezing the game.

### 13.2 Opcode groups

| Group | Examples |
|---|---|
| Move and load | `MOV, LOADK, LOADNULL, LOADTRUE, LOADFALSE` |
| Integer math | `ADDI, SUBI, MULI, DIVI, MODI, NEGI, SHL, SHR, AND, OR, XOR, NOT` |
| Float math | `ADDF, SUBF, MULF, DIVF, NEGF, FMA, MINF, MAXF, ABSF` |
| Vector math (SIMD friendly) | `V3ADD, V3SUB, V3MULS, V3DOT, V3CROSS, V3LEN, V3NORM, QMUL, QROTV` |
| Compare and jump | `EQ, NE, LT, LE, GT, GE, JMP, JMPT, JMPF` |
| Calls | `CALL, CALLNATIVE, CALLVIRT, RET, TAILCALL` |
| Objects | `NEWOBJ, GETFIELD, SETFIELD, GETSTATIC, SETSTATIC, CAST, ISTYPE` |
| Collections | `NEWLIST, LISTGET, LISTSET, LISTADD, NEWMAP, MAPGET, MAPSET, LEN` |
| Async | `AWAIT, SPAWN, YIELD, RESUME` |
| Events | `EMIT, SUBSCRIBE, UNSUBSCRIBE` |
| Debug | `LINE (debug builds only), BREAK` |

### 13.3 Native bindings

- Engine APIs are exposed through a generated table `NativeApi` with stable numeric ids. A **binding generator** reads a declaration file (`engine/script/api/*.qvapi`) and produces: the QivScript type signatures for the compiler and autocomplete, the C++ glue code, and the documentation shown in the editor.
- Native calls validate argument types again at runtime in debug builds only.

### 13.4 Threading

Scripts run only on the Engine main thread (and the physics callback for `@fast` handlers). Scripts cannot create threads. Async tasks complete on job workers but resume on Engine main.

## 14. Errors and diagnostics

### 14.1 Compile-time codes (examples)

| Code | Meaning |
|---|---|
| QS0001 | Unexpected token |
| QS0010 | Unknown identifier |
| QS0011 | Type mismatch (expected X, found Y) |
| QS0012 | Nullable value used without check |
| QS0020 | Missing return value |
| QS0030 | `@export` on unsupported type |
| QS0031 | `@range` on non-numeric field |
| QS0040 | `await` outside async function |
| QS0050 | Circular import |
| QS0060 | Override of a method that does not exist in the base class |

### 14.2 Runtime codes

| Code | Meaning | Behavior |
|---|---|---|
| QS-RUN-001 | Unhandled exception in lifecycle method | That script instance is disabled; others continue; red toast with link to line |
| QS-RUN-002 | Instruction budget exceeded | Call aborted; script stays enabled; warning |
| QS-RUN-003 | Use of destroyed object | Call returns default value; logged once per script per second |
| QS-RUN-004 | Stack overflow | Script disabled |
| QS-RUN-005 | Divide by zero (integer) | Exception |

`try { ... } catch (e) { ... }` and `throw` exist for expected failures such as asset loading. Exceptions never cross to the engine; the engine treats an uncaught exception in a lifecycle method as QS-RUN-001.

## 15. Reference script 1 — Tractor (corrected)

The Blueprint example never assigned the `wheels` array. This version wires it through exported references and a find-by-name fallback.

```
module Game.Vehicles;

import QivForge.Core.*;
import QivForge.Physics.*;
import QivForge.Cloud.*;
import QivForge.Input.*;
import QivForge.Audio.*;

@component("Heavy Tractor Physics & Cloud Stream Controller")
@require(RigidBody)
pub class TractorController extends VehiclePawn {

    @header("Engine")
    @export @range(800.0, 6000.0)
    pub var engineTorque: f32 = 3200.0;

    @header("Steering")
    @export @range(15.0, 50.0)
    pub var maxSteerAngle: f32 = 38.0;

    @header("Body")
    @export @range(500.0, 4000.0)
    pub var tractorMassKg: f32 = 2100.0;

    @header("Wheels (front left, front right, rear left, rear right)")
    @export pub var wheels: Array<WheelCollider, 4>;

    @header("Effects")
    @export pub var engineSound: AudioEmitter;
    @export pub var exhaustSmoke: ParticleSystem;

    private var rb: RigidBody;

    override async fn onAwake() -> void {
        self.rb = self.getComponent<RigidBody>();
        self.rb.setMass(self.tractorMassKg);
        self.rb.setCenterOfMass(Vec3(0.0, -0.65, 0.0));

        try {
            var modelAsset = await AssetManager.streamModel("assets/models/tractor_heavy.glb");
            self.attachModel(modelAsset);
        } catch (e) {
            Debug.warn("Tractor model failed to load: {e.message}");
        }

        CameraManager.follow(self.transform, Vec3(0.0, 3.2, -6.5), 8.0);
    }

    override fn onPhysicsStep(fixedDt: f32) -> void {
        var gas: f32 = TouchHUD.getAxis("Vertical");
        var steerInput: f32 = TouchHUD.getAxis("Horizontal");
        var steerAngle: f32 = steerInput * self.maxSteerAngle;
        var motorForce: f32 = gas * self.engineTorque;

        self.wheels[0].setSteerAngle(steerAngle);
        self.wheels[1].setSteerAngle(steerAngle);
        self.wheels[2].applyMotorTorque(motorForce);
        self.wheels[3].applyMotorTorque(motorForce);

        var brake: f32 = TouchHUD.isPressed("BrakePedal") ? 5000.0 : 0.0;
        for (var i: i32 = 0; i < 4; i++) {
            self.wheels[i].setBrakeTorque(brake);
        }
    }

    override fn onTick(dt: f32) -> void {
        var speedKmh: f32 = self.rb.getLinearVelocity().length() * 3.6;
        self.engineSound.setPitch(0.9 + speedKmh / 55.0);
        self.exhaustSmoke.setRate(10.0 + speedKmh * 2.0);
        WorldStreamer.updateActiveChunks(self.transform.position, radiusMeters = 250.0);
    }
}
```

## 16. Reference script 2 — TPS player

```
module Game.Characters;

import QivForge.Core.*;
import QivForge.Animation.*;
import QivForge.Physics.*;
import QivForge.Input.*;

@component("TPS Shooter & Survival Character")
pub class TPSPlayer extends CharacterPawn {

    @export @range(2.0, 15.0)  pub var moveSpeed: f32 = 6.5;
    @export @range(10.0, 200.0) pub var weaponDamage: f32 = 35.0;
    @export @range(0.05, 1.0)  pub var fireInterval: f32 = 0.12;
    @export pub var muzzleVFX: ParticleSystem;

    private var animGraph: SkeletalAnimator;
    private var lastFire: f32 = -10.0;

    override fn onStart() -> void {
        self.animGraph = self.getComponent<SkeletalAnimator>();
    }

    override fn onTick(dt: f32) -> void {
        var joy: Vec2 = TouchHUD.getJoystick("LeftMoveStick");
        var aiming: bool = TouchHUD.isPressed("AimButton");

        self.moveCharacter(Vec3(joy.x, 0.0, joy.y) * self.moveSpeed * dt);
        self.animGraph.setFloat("MoveSpeed", joy.length());
        self.animGraph.setBool("IsAiming", aiming);

        if (TouchHUD.isPressed("FireButton") && Time.time - self.lastFire >= self.fireInterval) {
            self.lastFire = Time.time;
            self.fireWeapon();
        }
    }

    private fn fireWeapon() -> void {
        self.animGraph.trigger("ShootRecoil");
        self.muzzleVFX.burst(15);

        var ray: Ray = CameraManager.screenCenterRay();
        var hit: RayHit = PhysicsWorld.raycast(ray, maxDistance = 300.0);

        if (hit.hasHit) {
            VFXManager.spawnImpact("BulletSpark", hit.point, hit.normal);
            if (hit.actor.hasTag("Enemy")) {
                hit.actor.sendMessage("applyDamage", self.weaponDamage);
            }
        }
    }
}
```

Changes from the Blueprint: `onStart` obtains the animator; fire rate is limited; `TouchHUD.isPressed` supports automatic fire while the button is held.

## 17. Conversion between script and Patches

- **Script to Patch (menu item 3):** supported constructs are lifecycle methods, `if/else`, `for` over ranges, arithmetic, API calls with simple arguments, exported fields (become variables), events. Unsupported constructs (classes with inheritance beyond the base, generics, `try/catch`, lambdas, coroutines with `yield`) become a **Script Call** node that references the original function, so behavior is preserved.
- **Patch to Script:** always possible; output is formatted and commented with the graph node ids.
- The mapping tables live in Doc 09 section 12.

## 18. Editor support (consumed by Doc 04)

- Syntax highlighting categories map to theme tokens `codeKeyword`, `codeType`, `codeString`, `codeNumber`, `codeComment`, `codeError`.
- Autocomplete sources: keywords, decorators, module imports, class members using the current expression type, engine API from the binding files, project classes.
- Auto-format rules: 4 spaces, K&R braces, one statement per line, space around binary operators, trailing commas allowed.
- Snippets (menu item 5): Vehicle controller, TPS player, Weapon, AI enemy, Pickup, Checkpoint, Health component. Each is a complete valid script that compiles with zero warnings.
- A **command-line compiler** `qvsc` (host tool) compiles scripts for tests and for the cloud AOT backend.

## 19. Security rules

- Scripts cannot access the file system, network, reflection, or other apps. Cloud calls go through `Cloud.*` and obey the user's explicit consent flags.
- Scripts loaded from downloaded assets or marketplace packs show a **Review scripts** dialog before first run and are marked untrusted until the user approves them.
- The instruction budget and memory budget (16 MB per script instance group) protect against runaway scripts.

## 20. Test plan (feeds Doc 15)

| Area | Tests |
|---|---|
| Lexer | All tokens, unicode strings, nested comments, error recovery |
| Parser | Grammar coverage with golden AST files; 200 negative cases with expected diagnostics |
| Semantics | Type inference, nullability, decorator validation |
| Codegen | Golden bytecode for 50 reference snippets |
| VM | Opcode unit tests, instruction budget, stack overflow, handle invalidation |
| Hot reload | Field migration cases, coroutine cancellation |
| Performance | Fibonacci, vector math loops, 1,000 scripts ticking per frame under 4 ms |
| Fuzz | Random token streams must never crash the compiler |
| Conformance | The two reference scripts compile with zero errors and run in templates |

## 21. Acceptance criteria

1. Both reference scripts compile with zero errors and zero warnings and run in the templates at full frame rate.
2. A syntax error shows a squiggle and a message in under 200 ms and the compiler reports the correct line and column.
3. `@export` and `@range` fields appear in the Inspector as sliders without any UI code, and changed values persist after save and reload.
4. An infinite loop in `onTick` is stopped within one frame by the instruction budget and the game keeps running.
5. Hot reload preserves a vehicle's exported values and does not reset its velocity.
6. Destroying an actor referenced by a script never crashes; it produces QS-RUN-003.
7. 1,000 simple scripts execute per frame in under 4 ms on the reference device.
8. Script to Patch and back yields behavior-identical results for the supported constructs in a recorded test.
