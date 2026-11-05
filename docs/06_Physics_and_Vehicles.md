# QivForge Doc 06 — Physics and Vehicles

**Series:** QivForge Engineering Spec · **Depends on:** Docs 01, 02, 03, 05 · **Audience:** AI coding agents

---

## 1. Technology decision (LOCKED)

- Physics engine: **Jolt Physics** (MIT license, C++, multithreaded, built for games, has a vehicle module, character controller, ragdoll, and heightfield shapes).
- Rapier3D from the old blueprints is **not used** (it is a Rust library, which conflicts with the C++ core).
- Wrapped by the `physics/` module. No other module includes Jolt headers.

## 2. Simulation loop

| Item | Value |
|---|---|
| Fixed step | 1/60 s |
| Max sub-steps per frame | 4 (if the frame is longer than 4 steps, extra time is dropped and `QF-PHYS-001` is logged once per second) |
| Collision steps per update | 1 (2 for vehicles on High) |
| Interpolation | Rendered transforms are interpolated between the last two physics states using the accumulator remainder, so motion is smooth at any display refresh rate |
| Threads | Jolt job system uses the engine job workers, never more than cores minus 2 |
| Determinism | Not guaranteed across devices in v1.0; same device and same input gives near identical results. Networked games in v2 use server authority |

Order inside a physics step: scripts `onPhysicsStep(fixedDt)` run first (apply forces and torques), then Jolt steps, then results are copied to actor transforms, then collision and trigger events are queued for the next script tick.

## 3. Layers and filtering

Object layers (16 available, 8 named by default):

| Layer | Collides with |
|---|---|
| Static | Dynamic, Vehicle, Character, Debris |
| Dynamic | all |
| Vehicle | Static, Dynamic, Vehicle, Character |
| Character | Static, Dynamic, Vehicle, Trigger |
| Projectile | Static, Dynamic, Vehicle, Character |
| Debris | Static only |
| Trigger | Character, Vehicle, Projectile (overlap only) |
| Ragdoll | Static, Dynamic |

A 16 by 16 collision matrix is editable in Project Settings. The default matrix above ships with every template.

## 4. Rigid bodies

| Property | Type | Default | Notes |
|---|---|---|---|
| bodyType | enum | Dynamic | Static, Dynamic, Kinematic |
| mass | f32 kg | 1 | Range 0.01 to 100000 |
| linearDamping | f32 | 0.05 | |
| angularDamping | f32 | 0.05 | |
| gravityScale | f32 | 1 | |
| centerOfMass | Vec3 | auto | Override for vehicles |
| continuousDetection | enum | Off | Off, Linear Cast (use for bullets and fast bodies) |
| allowSleep | bool | true | Sleeping bodies cost almost nothing |
| lockAxes | flags | none | Lock position or rotation per axis |

Rules: a Static body never moves. A Kinematic body is moved by script or animation and pushes dynamic bodies. Changing a Static body's transform at runtime is allowed but logs a performance warning.

## 5. Colliders

| Shape | Use | Notes |
|---|---|---|
| Box, Sphere, Capsule, Cylinder | Primitives | Cheapest; prefer them |
| Convex hull | Props | Auto-generated at import, max 64 vertices after simplification |
| Compound | Complex dynamic objects | Multiple primitives under one body |
| Triangle mesh | Static level geometry only | Cannot be dynamic |
| Heightfield | Terrain | Built from the terrain heightmap per chunk; shares data with the renderer |

Each collider has: `isTrigger`, `physicsMaterial`, `layer`, `offset`, and `size`. **Physics material:** friction (0 to 2), restitution (0 to 1), combine mode (average, min, max, multiply). Presets: Default, Rubber, Metal, Ice, Mud, Grass, Asphalt, Wood.

### Auto collision generation

When importing a GLB the importer can generate colliders: **None**, **Box**, **Convex**, **Mesh (static)**. Generation runs in a job worker and shows progress; results are cached by content hash.

## 6. Queries

All queries are available to scripts and Patches (Doc 08, 09).

| Query | Signature | Notes |
|---|---|---|
| Raycast | `PhysicsWorld.raycast(ray, maxDistance, layerMask) -> RayHit` | First hit. RayHit has `hasHit`, `point`, `normal`, `distance`, `actor`, `surfaceType` |
| RaycastAll | returns array sorted by distance | Capped at 32 results |
| Shape cast | `sphereCast`, `boxCast`, `capsuleCast` | For wide-ray aiming and ground checks |
| Overlap | `overlapSphere(center, radius, layerMask)`, `overlapBox` | Returns actors |
| Closest point | `closestPoint(actor, point)` | |

Query budget: at most 256 raycasts per frame; excess queries are deferred and a warning is logged. Raycasts against the heightfield use Jolt's heightfield acceleration.

## 7. Events

| Event | Delivered to | Payload |
|---|---|---|
| `onCollisionEnter / Stay / Exit` | both actors' scripts | other actor, contact point, normal, relative speed, impulse |
| `onTriggerEnter / Stay / Exit` | trigger and entering actor | other actor |
| `onSleep / onWake` | the body | none |

Events are collected during the step, sorted by actor id for stable order, and dispatched during the next `onTick`. A script can mark an event handler `@fast` to run during the physics step (use only for vehicles and projectiles).

## 8. Joints

| Joint | Parameters | Tier |
|---|---|---|
| Fixed | break force and torque | 1 |
| Hinge | axis, limits, motor (target velocity, max torque), friction | 1 |
| Slider | axis, limits, motor | 1 |
| Spring (distance) | rest length, frequency, damping | 1 |
| Ball (cone) | swing and twist limits | 1 |
| Hitch (trailer) | Hinge on vertical axis plus pitch freedom, break force | 2 |

Joints can break when force exceeds `breakForce`, raising `onJointBreak`.

## 9. Vehicle system (core feature)

### 9.1 Components

- **VehicleController:** owns chassis rigid body and the drivetrain.
- **WheelCollider** (one per wheel): suspension, tire, brake, steering.
- **Drivetrain:** engine torque curve, gearbox, differential, clutch (simple).
- **VehiclePawn (script base class):** exposes `setThrottle`, `setBrake`, `setSteer`, `setHandbrake`, `setGear`, and reads `speedKmh`, `rpm`, `gear`, `isGrounded`.

### 9.2 WheelCollider parameters

| Parameter | Unit | Tractor preset | Car preset |
|---|---|---|---|
| radius | m | 0.75 rear, 0.55 front | 0.34 |
| width | m | 0.45 rear, 0.25 front | 0.22 |
| suspensionTravel | m | 0.25 | 0.20 |
| suspensionFrequency | Hz | 1.2 | 1.6 |
| suspensionDamping | ratio | 0.7 | 0.8 |
| maxSteerAngle | deg | 38 (front) | 32 |
| maxBrakeTorque | Nm | 5000 | 2500 |
| motorized | bool | rear wheels | rear or all (setting) |
| forwardFriction | curve | see 9.4 | see 9.4 |
| sidewaysFriction | curve | see 9.4 | see 9.4 |

Vehicle mass presets: tractor 2100 kg with center of mass lowered 0.65 m below the geometric center to prevent rollovers on slopes; car 1400 kg lowered 0.3 m.

### 9.3 Drivetrain model

- **Engine:** torque curve defined as a table of RPM vs Nm (default tractor: idle 800 RPM, peak 3200 Nm at 1500 RPM, redline 2600 RPM; default car: idle 900, peak 320 Nm at 4500, redline 7000). Throttle scales the curve.
- **Gearbox:** list of gear ratios plus reverse and final drive. Automatic mode shifts up at 90% of redline and down at 40%. Manual mode accepts `shiftUp`, `shiftDown`.
- **Differential:** open, limited-slip (parameter), or locked (tractor option for mud).
- **Wheel torque:** engine torque times gear ratio times final drive, split by differential.
- **Brakes:** brake torque per wheel; handbrake locks rear wheels.
- **Anti-roll bar** (optional): stiffness parameter per axle to reduce body roll.
- **Downforce** (cars): force proportional to speed squared.

### 9.4 Tire friction and surfaces

Friction uses a slip curve: friction coefficient rises to a peak at the optimal slip, then falls to a sliding value. Each surface type scales the curves.

| Surface type | Forward multiplier | Sideways multiplier | Rolling resistance | Dust or spray effect |
|---|---|---|---|---|
| Asphalt | 1.0 | 1.0 | low | tire smoke when sliding |
| Dirt | 0.75 | 0.65 | medium | dust |
| Grass | 0.65 | 0.55 | medium | grass bits |
| Mud | 0.45 | 0.4 | high | mud spray, wheels sink visually by up to 8 cm |
| Sand | 0.55 | 0.45 | high | sand cloud |
| Ice | 0.15 | 0.1 | very low | none |

Surface type comes from the terrain splat layer under the wheel or from the collider's physics material tag. The wheel reads it each physics step.

### 9.5 Vehicle outputs for audio and VFX

The vehicle publishes `rpm`, `speedKmh`, `slipRatio` per wheel, `isSkidding`, `suspensionCompression` per wheel. Scripts and Patches map these to engine sound pitch (Doc 10), dust particles, and tire marks (decal).

### 9.6 Tractor-specific features

- **Implements and trailers:** a `HitchPoint` component on the tractor and a `HitchSocket` on a trailer create a hinge joint (v2).
- **PTO and attachments:** `AttachmentPoint` for plow or loader, driven by a hinge joint with motor (v2).
- **Heavy traction:** rear wheels have higher friction and torque; front axle is steerable and can be driven (4WD toggle).
- **Roll and slope safety:** if the roll angle exceeds 70 degrees for more than 2 seconds, raise `onVehicleOverturned` so game logic can offer a reset.

### 9.7 Input mapping

`TouchHUD.getAxis("Vertical")` maps to throttle and brake (positive gas, negative brake or reverse depending on speed), `TouchHUD.getAxis("Horizontal")` maps to steering. Steering is smoothed with a rate limit of 120 degrees per second and speed-sensitive reduction (steer angle shrinks as speed grows above 30 km/h). Tilt (gyroscope) steering is optional and scaled by a sensitivity slider.

## 10. Character controller

| Feature | Value |
|---|---|
| Shape | Capsule, default height 1.8 m, radius 0.35 m |
| Walk and run speed | 3.0 and 6.5 m/s (script exposes `moveSpeed`) |
| Jump | Impulse so peak height is 1.2 m by default; coyote time 0.1 s; jump buffer 0.1 s |
| Slope limit | 50 degrees; slide down slopes above that |
| Step height | 0.35 m |
| Ground check | Shape cast downward 0.1 m below the capsule |
| Gravity | -20 m/s squared for a snappier feel (project setting) |
| Push dynamic bodies | Applies a capped force |
| Crouch | Capsule height 1.1 m |
| Moving platforms | Inherits platform velocity |
| Vehicle entry | `enterVehicle(vehicle)` disables the controller and attaches to a seat socket |

## 11. Weapons and projectiles

- **Hitscan weapon** (default for TPS): `WeaponComponent` with fields `damage`, `fireRate` (shots per second), `magazineSize`, `reloadTime`, `spreadDegrees`, `recoilPattern`, `range`, `penetration`. On fire: build a ray from the camera center plus spread, call `raycast`, spawn impact VFX and decal based on `surfaceType`, apply damage via the Health system.
- **Projectile weapon:** spawns a body on the Projectile layer with continuous detection; supports gravity drop and explosion radius.
- **Damage system:** `HealthComponent` with `maxHealth`, `armor`, events `onDamaged`, `onDied`; damage multipliers by hit zone (head 2.0, body 1.0, limb 0.75) when hitboxes are on a ragdoll skeleton.
- **Bullet hole decals:** limited to 64 decals alive; oldest removed first.

## 12. Ragdoll

- Built from a skeleton map: pelvis, spine, head, upper and lower limbs, each a capsule with hinge or cone limits.
- Triggered on death: the animator is disabled, bone transforms feed the ragdoll, and the ragdoll uses the Ragdoll layer.
- Settle: after 5 seconds of low velocity, the ragdoll sleeps; after 20 seconds it can be removed (setting).
- Presets generated automatically for humanoid skeletons from the avatar map (Doc 07).

## 13. Debug and tools

- **Physics debug draw:** colliders wireframe, contact points, joint axes, center of mass, wheel suspension rays, raycast lines (last 1 second). Toggle in the viewport shading modes and the Live Debug Drawer.
- **Stats:** active bodies, sleeping bodies, contacts, step time, query count. Shown in the Profiler tab.
- **Vehicle tuning panel (Inspector):** live sliders for suspension, torque curve editing as a draggable graph, friction curve graph, and a **Test drive** button that enters Play with the vehicle selected.

## 14. Scripting API summary

```
RigidBody: addForce(Vec3), addImpulse(Vec3), addTorque(Vec3), setVelocity(Vec3),
           getLinearVelocity(), getAngularVelocity(), setMass(f32), setCenterOfMass(Vec3), wake(), sleep()
WheelCollider: setSteerAngle(f32), applyMotorTorque(f32), setBrakeTorque(f32),
               getRpm(), isGrounded(), getSurfaceType(), getSlip()
VehicleController: setThrottle, setBrake, setSteer, setHandbrake, setGear, speedKmh, rpm
CharacterController: move(Vec3), jump(), setCrouch(bool), isGrounded(), setVelocity(Vec3)
PhysicsWorld: raycast, raycastAll, sphereCast, overlapSphere, setGravity(Vec3)
```

## 15. Performance rules

- Budget: under 3 ms physics step at 100 active bodies and 1 vehicle on the reference device.
- Bodies sleep after 0.5 s of rest; the world supports at most 1,000 active bodies; beyond that a warning shows.
- Mesh colliders only for static geometry; large terrain uses heightfield chunks created and destroyed with chunk streaming (Doc 02 section 10).
- Colliders for distant chunks are removed when their chunk unloads.

## 16. Error handling

- Invalid mass or zero-size collider: clamp to the minimum and log `QF-PHYS-003`.
- NaN in a transform or velocity: reset the body to its last valid state and log `QF-PHYS-004` with the actor name (never crash).
- Vehicle with fewer than 2 wheels: the Inspector shows an error and the controller stays disabled.

## 17. Acceptance criteria

1. A tractor on a 25 degree grass slope does not roll over at normal driving and climbs a 20 degree slope in first gear with full throttle.
2. The tractor reaches a top speed within 10% of the value implied by the torque curve and gear ratios.
3. Driving from asphalt to mud visibly reduces acceleration and changes the dust effect within 0.2 s.
4. A character walks up a 0.3 m step and slides down a 60 degree slope.
5. Raycast against a 300 m distance in a scene with 1,000 colliders takes under 0.2 ms per call on average.
6. A stack of 100 boxes comes to rest and sleeps within 5 seconds with no jitter.
7. Fast bullets (300 m/s projectile) never tunnel through a 0.1 m wall when continuous detection is on.
8. Physics step stays under 3 ms for the reference scene for 10 minutes.
