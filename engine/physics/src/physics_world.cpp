#include "qivforge/physics/physics_world.h"
#include <algorithm>
#include <cmath>

namespace qivforge::physics {

PhysicsWorld::PhysicsWorld() noexcept {
    // Default 8-layer collision matrix per Doc 06 §3
    m_layerCollisionMasks[Layers::STATIC]     = Layers::DYNAMIC | Layers::VEHICLE | Layers::CHARACTER | Layers::DEBRIS;
    m_layerCollisionMasks[Layers::DYNAMIC]    = Layers::ALL & ~Layers::TRIGGER; // Collides with all except trigger overlaps
    m_layerCollisionMasks[Layers::VEHICLE]    = Layers::STATIC | Layers::DYNAMIC | Layers::VEHICLE | Layers::CHARACTER | Layers::TRIGGER;
    m_layerCollisionMasks[Layers::CHARACTER]  = Layers::STATIC | Layers::DYNAMIC | Layers::VEHICLE | Layers::TRIGGER;
    m_layerCollisionMasks[Layers::PROJECTILE] = Layers::STATIC | Layers::DYNAMIC | Layers::VEHICLE | Layers::CHARACTER | Layers::TRIGGER;
    m_layerCollisionMasks[Layers::DEBRIS]     = Layers::STATIC;
    m_layerCollisionMasks[Layers::TRIGGER]    = Layers::CHARACTER | Layers::VEHICLE | Layers::PROJECTILE;
    m_layerCollisionMasks[Layers::RAGDOLL]    = Layers::STATIC | Layers::DYNAMIC;
}

RigidBody* PhysicsWorld::CreateBody(uint32_t actorId, BodyType type) {
    auto body = std::make_unique<RigidBody>(actorId, type);
    RigidBody* ptr = body.get();
    m_bodies[actorId] = std::move(body);
    return ptr;
}

bool PhysicsWorld::DestroyBody(uint32_t actorId) {
    return m_bodies.erase(actorId) > 0;
}

RigidBody* PhysicsWorld::GetBody(uint32_t actorId) noexcept {
    auto it = m_bodies.find(actorId);
    return it != m_bodies.end() ? it->second.get() : nullptr;
}

const RigidBody* PhysicsWorld::GetBody(uint32_t actorId) const noexcept {
    auto it = m_bodies.find(actorId);
    return it != m_bodies.end() ? it->second.get() : nullptr;
}

void PhysicsWorld::Clear() noexcept {
    m_bodies.clear();
    m_collisionEvents.clear();
    m_triggerEvents.clear();
    m_accumulator = 0.0f;
    m_interpolationAlpha = 0.0f;
}

size_t PhysicsWorld::GetActiveBodyCount() const noexcept {
    size_t count = 0;
    for (const auto& [id, body] : m_bodies) {
        if (body->GetBodyType() == BodyType::DYNAMIC && !body->IsSleeping()) {
            count++;
        }
    }
    return count;
}

size_t PhysicsWorld::GetSleepingBodyCount() const noexcept {
    size_t count = 0;
    for (const auto& [id, body] : m_bodies) {
        if (body->GetBodyType() == BodyType::DYNAMIC && body->IsSleeping()) {
            count++;
        }
    }
    return count;
}

bool PhysicsWorld::CanLayersCollide(uint16_t layerA, uint16_t layerB) const noexcept {
    auto it = m_layerCollisionMasks.find(layerA);
    if (it != m_layerCollisionMasks.end()) {
        return (it->second & layerB) != 0;
    }
    return true;
}

void PhysicsWorld::SetCollisionMask(uint16_t layer, uint16_t mask) noexcept {
    m_layerCollisionMasks[layer] = mask;
}

void PhysicsWorld::Update(float deltaTime) noexcept {
    m_raycastsThisFrame = 0;
    m_accumulator += deltaTime;

    // Max 4 sub-steps per frame per Doc 06 §2 (drop extra time to prevent spiral of death)
    float maxAccumulator = 4.0f * m_fixedDt;
    if (m_accumulator > maxAccumulator) {
        m_accumulator = maxAccumulator;
    }

    while (m_accumulator >= m_fixedDt) {
        StepFixed(m_fixedDt);
        m_accumulator -= m_fixedDt;
    }

    m_interpolationAlpha = m_accumulator / m_fixedDt;
}

void PhysicsWorld::StepFixed(float fixedDt) noexcept {
    ClearEvents();

    // 1. Integrate dynamic bodies
    for (auto& [id, body] : m_bodies) {
        if (body->GetBodyType() == BodyType::DYNAMIC && !body->IsSleeping()) {
            body->Integrate(fixedDt, m_gravity);
        }
    }

    // 2. Resolve collisions (ground and body-vs-body) with 4 iterations for stack stability
    for (int iter = 0; iter < 4; ++iter) {
        ResolveCollisions(fixedDt);
    }

    // 3. Update sleep states after collision resolution
    for (auto& [id, body] : m_bodies) {
        if (body->GetBodyType() == BodyType::DYNAMIC && !body->IsSleeping()) {
            body->CheckSleep(fixedDt);
        }
    }

    // 4. Stable event ordering by actor id per Doc 06 §7
    std::sort(m_collisionEvents.begin(), m_collisionEvents.end(), [](const CollisionEvent& a, const CollisionEvent& b) {
        if (a.actorA != b.actorA) return a.actorA < b.actorA;
        return a.actorB < b.actorB;
    });

    std::sort(m_triggerEvents.begin(), m_triggerEvents.end(), [](const TriggerEvent& a, const TriggerEvent& b) {
        if (a.triggerActor != b.triggerActor) return a.triggerActor < b.triggerActor;
        return a.otherActor < b.otherActor;
    });
}

void PhysicsWorld::ResolveCollisions(float dt) noexcept {
    // A. Ground collisions
    if (m_hasGroundPlane) {
        for (auto& [id, body] : m_bodies) {
            if (body->GetBodyType() == BodyType::DYNAMIC) {
                ResolveGroundCollisions(*body);
            }
        }
    }

    // B. Body-vs-Body collisions
    std::vector<RigidBody*> bodyList;
    bodyList.reserve(m_bodies.size());
    for (auto& [id, body] : m_bodies) {
        bodyList.push_back(body.get());
    }

    size_t count = bodyList.size();
    for (size_t i = 0; i < count; ++i) {
        RigidBody* a = bodyList[i];
        for (size_t j = i + 1; j < count; ++j) {
            RigidBody* b = bodyList[j];

            // Skip if both static or both sleeping
            if ((a->GetBodyType() == BodyType::STATIC && b->GetBodyType() == BodyType::STATIC) ||
                (a->IsSleeping() && b->IsSleeping())) {
                continue;
            }

            // Check layer collision filter
            if (!CanLayersCollide(a->GetLayer(), b->GetLayer())) {
                continue;
            }

            ResolveBodyVsBody(*a, *b);
        }
    }
}

void PhysicsWorld::ResolveGroundCollisions(RigidBody& body) noexcept {
    Vec3 pos = body.GetPosition();
    Vec3 size = body.GetColliderSize();
    float bottomY = pos.y - size.y;

    if (bottomY < m_groundHeight) {
        float penetration = m_groundHeight - bottomY;
        pos.y += penetration;
        body.SetPosition(pos);

        Vec3 vel = body.GetLinearVelocity();
        if (vel.y < 0.0f) {
            float restitution = body.GetMaterial().restitution;
            vel.y = -vel.y * restitution;
            if (std::abs(vel.y) < 0.15f) {
                vel.y = 0.0f; // Settle micro-bounces
            }

            // Friction along X/Z
            float friction = body.GetMaterial().friction;
            float frictionFactor = std::clamp(1.0f - friction * 12.0f * m_fixedDt, 0.0f, 1.0f);
            vel.x *= frictionFactor;
            vel.z *= frictionFactor;

            body.SetLinearVelocity(vel, false);

            // Damping angular velocity when in ground contact
            Vec3 angVel = body.GetAngularVelocity();
            angVel.x *= frictionFactor;
            angVel.z *= frictionFactor;
            body.SetAngularVelocity(angVel, false);
        }
    }
}

void PhysicsWorld::ResolveBodyVsBody(RigidBody& a, RigidBody& b) noexcept {
    Vec3 posA = a.GetPosition();
    Vec3 sizeA = a.GetColliderSize();
    Vec3 posB = b.GetPosition();
    Vec3 sizeB = b.GetColliderSize();

    // Fast Axis-Aligned Bounding Box (AABB) overlap test
    float dx = posA.x - posB.x;
    float dy = posA.y - posB.y;
    float dz = posA.z - posB.z;

    float overlapX = (sizeA.x + sizeB.x) - std::abs(dx);
    if (overlapX <= 0.0f) return;

    float overlapY = (sizeA.y + sizeB.y) - std::abs(dy);
    if (overlapY <= 0.0f) return;

    float overlapZ = (sizeA.z + sizeB.z) - std::abs(dz);
    if (overlapZ <= 0.0f) return;

    // Triggers only report overlap, no physical separation
    if (a.IsTrigger() || b.IsTrigger()) {
        TriggerEvent ev;
        ev.triggerActor = a.IsTrigger() ? a.GetActorId() : b.GetActorId();
        ev.otherActor = a.IsTrigger() ? b.GetActorId() : a.GetActorId();
        ev.entered = true;
        m_triggerEvents.push_back(ev);
        return;
    }

    // Minimum penetration vector
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float minOverlap = overlapY;

    if (overlapX < minOverlap) {
        minOverlap = overlapX;
        normal = Vec3{dx > 0 ? 1.0f : -1.0f, 0.0f, 0.0f};
    }
    if (overlapZ < minOverlap) {
        minOverlap = overlapZ;
        normal = Vec3{0.0f, 0.0f, dz > 0 ? 1.0f : -1.0f};
    }
    if (minOverlap == overlapY) {
        normal = Vec3{0.0f, dy > 0 ? 1.0f : -1.0f, 0.0f};
    }

    // Positional separation based on inverse mass
    float invMassA = a.GetInverseMass();
    float invMassB = b.GetInverseMass();
    float totalInvMass = invMassA + invMassB;
    if (totalInvMass <= 0.0f) return;

    Vec3 separation = normal * (minOverlap / totalInvMass);
    if (a.GetBodyType() == BodyType::DYNAMIC) {
        a.SetPosition(posA + separation * invMassA);
    }
    if (b.GetBodyType() == BodyType::DYNAMIC) {
        b.SetPosition(posB - separation * invMassB);
    }

    // Collision Impulse
    Vec3 relVel = a.GetLinearVelocity() - b.GetLinearVelocity();
    float velAlongNormal = relVel.Dot(normal);

    // Only apply impulse if objects are moving towards each other
    if (velAlongNormal < 0.0f) {
        float restitution = 0.5f * (a.GetMaterial().restitution + b.GetMaterial().restitution);
        float j = -(1.0f + restitution) * velAlongNormal / totalInvMass;
        Vec3 impulse = normal * j;

        if (a.GetBodyType() == BodyType::DYNAMIC) {
            a.AddImpulse(impulse);
        }
        if (b.GetBodyType() == BodyType::DYNAMIC) {
            b.AddImpulse(-impulse);
        }

        if (std::abs(velAlongNormal) < 0.25f) {
            Vec3 va = a.GetLinearVelocity();
            Vec3 vb = b.GetLinearVelocity();
            va.y = 0.0f;
            vb.y = 0.0f;
            a.SetLinearVelocity(va, false);
            b.SetLinearVelocity(vb, false);
        } else if (std::abs(j) > 1.0f) {
            a.Wake();
            b.Wake();
        }

        // Record collision event
        CollisionEvent ev;
        ev.actorA = a.GetActorId();
        ev.actorB = b.GetActorId();
        ev.contactPoint = (posA + posB) * 0.5f;
        ev.normal = normal;
        ev.relativeSpeed = std::abs(velAlongNormal);
        ev.impulse = j;
        m_collisionEvents.push_back(ev);
    }
}

RayHit PhysicsWorld::Raycast(const Vec3& origin, const Vec3& direction, float maxDistance, uint16_t layerMask) noexcept {
    m_raycastsThisFrame++;
    RayHit closestHit;
    closestHit.hasHit = false;
    closestHit.distance = maxDistance;

    Vec3 dir = direction.Normalized();

    // Check ground plane first (Y = m_groundHeight, normal = (0, 1, 0))
    if (m_hasGroundPlane && (layerMask & Layers::STATIC)) {
        if (std::abs(dir.y) > 1e-6f) {
            float t = (m_groundHeight - origin.y) / dir.y;
            if (t > 0.0f && t < closestHit.distance) {
                closestHit.hasHit = true;
                closestHit.distance = t;
                closestHit.point = origin + dir * t;
                closestHit.normal = Vec3{0.0f, 1.0f, 0.0f};
                closestHit.actorId = 0; // Ground actor
                closestHit.surfaceType = SurfaceType::GRASS;
            }
        }
    }

    // Check bodies against ray
    for (const auto& [id, body] : m_bodies) {
        if (!(body->GetLayer() & layerMask)) continue;

        Vec3 bMin = body->GetPosition() - body->GetColliderSize();
        Vec3 bMax = body->GetPosition() + body->GetColliderSize();

        // Ray vs AABB slab method
        float tmin = 0.0f;
        float tmax = closestHit.distance;

        bool hit = true;
        for (int i = 0; i < 3; ++i) {
            float o = (i == 0) ? origin.x : (i == 1 ? origin.y : origin.z);
            float d = (i == 0) ? dir.x : (i == 1 ? dir.y : dir.z);
            float mn = (i == 0) ? bMin.x : (i == 1 ? bMin.y : bMin.z);
            float mx = (i == 0) ? bMax.x : (i == 1 ? bMax.y : bMax.z);

            if (std::abs(d) < 1e-6f) {
                if (o < mn || o > mx) {
                    hit = false;
                    break;
                }
            } else {
                float invD = 1.0f / d;
                float t1 = (mn - o) * invD;
                float t2 = (mx - o) * invD;
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                if (tmin > tmax) {
                    hit = false;
                    break;
                }
            }
        }

        if (hit && tmin > 0.0f && tmin < closestHit.distance) {
            closestHit.hasHit = true;
            closestHit.distance = tmin;
            closestHit.point = origin + dir * tmin;
            closestHit.actorId = id;

            // Compute hit normal
            Vec3 hitPos = closestHit.point - body->GetPosition();
            Vec3 half = body->GetColliderSize();
            float eps = 0.001f;
            if (std::abs(hitPos.x - half.x) < eps) closestHit.normal = Vec3{1.0f, 0.0f, 0.0f};
            else if (std::abs(hitPos.x + half.x) < eps) closestHit.normal = Vec3{-1.0f, 0.0f, 0.0f};
            else if (std::abs(hitPos.y - half.y) < eps) closestHit.normal = Vec3{0.0f, 1.0f, 0.0f};
            else if (std::abs(hitPos.y + half.y) < eps) closestHit.normal = Vec3{0.0f, -1.0f, 0.0f};
            else if (std::abs(hitPos.z - half.z) < eps) closestHit.normal = Vec3{0.0f, 0.0f, 1.0f};
            else closestHit.normal = Vec3{0.0f, 0.0f, -1.0f};

            closestHit.surfaceType = SurfaceType::ASPHALT;
        }
    }

    return closestHit;
}

std::vector<RayHit> PhysicsWorld::RaycastAll(const Vec3& origin, const Vec3& direction, float maxDistance, uint16_t layerMask) noexcept {
    m_raycastsThisFrame++;
    std::vector<RayHit> hits;
    hits.reserve(16);

    Vec3 dir = direction.Normalized();

    for (const auto& [id, body] : m_bodies) {
        if (!(body->GetLayer() & layerMask)) continue;

        Vec3 bMin = body->GetPosition() - body->GetColliderSize();
        Vec3 bMax = body->GetPosition() + body->GetColliderSize();

        float tmin = 0.0f;
        float tmax = maxDistance;
        bool hit = true;

        for (int i = 0; i < 3; ++i) {
            float o = (i == 0) ? origin.x : (i == 1 ? origin.y : origin.z);
            float d = (i == 0) ? dir.x : (i == 1 ? dir.y : dir.z);
            float mn = (i == 0) ? bMin.x : (i == 1 ? bMin.y : bMin.z);
            float mx = (i == 0) ? bMax.x : (i == 1 ? bMax.y : bMax.z);

            if (std::abs(d) < 1e-6f) {
                if (o < mn || o > mx) { hit = false; break; }
            } else {
                float invD = 1.0f / d;
                float t1 = (mn - o) * invD;
                float t2 = (mx - o) * invD;
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                if (tmin > tmax) { hit = false; break; }
            }
        }

        if (hit && tmin > 0.0f && tmin < maxDistance) {
            RayHit rh;
            rh.hasHit = true;
            rh.distance = tmin;
            rh.point = origin + dir * tmin;
            rh.actorId = id;
            rh.normal = Vec3{0.0f, 1.0f, 0.0f};
            hits.push_back(rh);
            if (hits.size() >= 32) break; // Capped at 32 results per Doc 06 §6
        }
    }

    // Sort by distance
    std::sort(hits.begin(), hits.end(), [](const RayHit& a, const RayHit& b) {
        return a.distance < b.distance;
    });

    return hits;
}

RayHit PhysicsWorld::SphereCast(const Vec3& origin, float radius, const Vec3& direction, float maxDistance, uint16_t layerMask) noexcept {
    // Sphere cast modeled as raycast with expanded collider extents
    m_raycastsThisFrame++;
    RayHit hit = Raycast(origin, direction, maxDistance, layerMask);
    if (hit.hasHit) {
        hit.distance = std::max(0.0f, hit.distance - radius);
        hit.point = origin + direction.Normalized() * hit.distance;
    }
    return hit;
}

std::vector<uint32_t> PhysicsWorld::OverlapSphere(const Vec3& center, float radius, uint16_t layerMask) noexcept {
    std::vector<uint32_t> result;
    float rSq = radius * radius;

    for (const auto& [id, body] : m_bodies) {
        if (!(body->GetLayer() & layerMask)) continue;

        Vec3 pos = body->GetPosition();
        if ((pos - center).LengthSquared() <= rSq) {
            result.push_back(id);
        }
    }
    return result;
}

std::vector<uint32_t> PhysicsWorld::OverlapBox(const Vec3& center, const Vec3& halfExtents, uint16_t layerMask) noexcept {
    std::vector<uint32_t> result;

    for (const auto& [id, body] : m_bodies) {
        if (!(body->GetLayer() & layerMask)) continue;

        Vec3 pos = body->GetPosition();
        Vec3 diff = pos - center;
        if (std::abs(diff.x) <= halfExtents.x &&
            std::abs(diff.y) <= halfExtents.y &&
            std::abs(diff.z) <= halfExtents.z) {
            result.push_back(id);
        }
    }
    return result;
}

} // namespace qivforge::physics
