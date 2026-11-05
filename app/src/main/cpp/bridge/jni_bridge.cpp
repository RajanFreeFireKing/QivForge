#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include "qivforge/core/engine.h"
#include "qivforge/scene/scene.h"
#include "qivforge/physics/physics_world.h"
#include "qivforge/physics/vehicle_controller.h"

#include "qivforge/script/lexer.h"
#include "qivforge/script/parser.h"
#include "qivforge/script/compiler.h"
#include "qivforge/script/script_instance.h"
#include <unordered_map>

#define LOG_TAG "QivForgeBridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace qivforge::core;

static std::unordered_map<uint32_t, std::shared_ptr<qivforge::script::ScriptInstance>> s_scriptInstances;
static qivforge::script::Qvm s_qvm;

extern "C" {

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeInit(
    JNIEnv* env,
    jobject /* thiz */,
    jstring appName,
    jint targetFps,
    jboolean useVulkan) {
    
    const char* nativeAppName = env->GetStringUTFChars(appName, nullptr);
    EngineConfig config;
    config.appName = nativeAppName ? nativeAppName : "QivForge";
    config.targetFps = targetFps;
    config.useVulkan = useVulkan;
    
    if (nativeAppName) {
        env->ReleaseStringUTFChars(appName, nativeAppName);
    }

    LOGI("NativeBridge: Initializing engine (Vulkan: %d, Target FPS: %d)", useVulkan, targetFps);
    ErrorCode res = Engine::Instance().Initialize(config);
    return static_cast<jint>(res);
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeShutdown(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    LOGI("NativeBridge: Shutting down engine");
    ErrorCode res = Engine::Instance().Shutdown();
    return static_cast<jint>(res);
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeOnSurfaceCreated(
    JNIEnv* env,
    jobject /* thiz */,
    jobject surface) {
    if (!surface) {
        return static_cast<jint>(ErrorCode::ERR_CORE_INVALID_ARGUMENT);
    }
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("NativeBridge: Failed to get ANativeWindow from surface");
        return static_cast<jint>(ErrorCode::ERR_RENDER_SURFACE_LOST);
    }
    LOGI("NativeBridge: Surface created");
    return static_cast<jint>(Engine::Instance().OnSurfaceCreated(window));
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeOnSurfaceChanged(
    JNIEnv* /* env */,
    jobject /* thiz */,
    jint width,
    jint height) {
    LOGI("NativeBridge: Surface changed to %dx%d", width, height);
    return static_cast<jint>(Engine::Instance().OnSurfaceChanged(width, height));
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeOnSurfaceDestroyed(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    LOGI("NativeBridge: Surface destroyed");
    return static_cast<jint>(Engine::Instance().OnSurfaceDestroyed());
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeTick(
    JNIEnv* /* env */,
    jobject /* thiz */,
    jfloat deltaTime) {
    return static_cast<jint>(Engine::Instance().Tick(deltaTime));
}

JNIEXPORT jfloat JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeGetFps(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    return Engine::Instance().GetStats().fps;
}

JNIEXPORT jfloat JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeGetFrameMs(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    return Engine::Instance().GetStats().frameMs;
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeGetDrawCalls(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    return static_cast<jint>(Engine::Instance().GetStats().drawCalls);
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeGetTriangles(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    return static_cast<jint>(Engine::Instance().GetStats().triangleCount);
}

JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativePollNextEvent(
    JNIEnv* env,
    jobject /* thiz */,
    jintArray outData) {
    
    EngineEvent ev;
    if (!Engine::Instance().GetEventRingBuffer().Pop(ev)) {
        return JNI_FALSE;
    }

    jint buffer[5];
    buffer[0] = static_cast<jint>(ev.type);
    if (ev.type == EventType::STATS_UPDATED) {
        buffer[1] = static_cast<jint>(ev.payload.stats.fps);
        buffer[2] = static_cast<jint>(ev.payload.stats.frameMs * 10.0f);
        buffer[3] = static_cast<jint>(ev.payload.stats.drawCalls);
        buffer[4] = static_cast<jint>(ev.payload.stats.triangleCount);
    } else if (ev.type == EventType::HIERARCHY_CHANGED) {
        buffer[1] = static_cast<jint>(ev.payload.actor.actorId);
        buffer[2] = buffer[3] = buffer[4] = 0;
    } else if (ev.type == EventType::PLAY_STATE_CHANGED) {
        buffer[1] = static_cast<jint>(ev.payload.rawData[0]);
        buffer[2] = buffer[3] = buffer[4] = 0;
    } else {
        buffer[1] = buffer[2] = buffer[3] = buffer[4] = 0;
    }

    env->SetIntArrayRegion(outData, 0, 5, buffer);
    return JNI_TRUE;
}

// Scene & Undo JNI endpoints (Doc 02 §16 AC2 / Doc 15 M3)
JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeCreateActor(
    JNIEnv* env,
    jobject /* thiz */,
    jstring name,
    jint parentId) {
    
    const char* nativeName = env->GetStringUTFChars(name, nullptr);
    std::string actorName = nativeName ? nativeName : "NewActor";
    if (nativeName) {
        env->ReleaseStringUTFChars(name, nativeName);
    }

    uint32_t actorId = Engine::Instance().CreateActor(actorName, static_cast<uint32_t>(parentId));
    return static_cast<jint>(actorId);
}

JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeDestroyActor(
    JNIEnv* /* env */,
    jobject /* thiz */,
    jint actorId) {
    return Engine::Instance().DestroyActor(static_cast<uint32_t>(actorId)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeUndo(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    return Engine::Instance().Undo() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeRedo(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    return Engine::Instance().Redo() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeGetActorCount(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    return static_cast<jint>(Engine::Instance().GetScene().GetActorCount());
}

// Instant Play Mode & Physics endpoints (Doc 06 / Doc 15 M4)
JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeStartPlay(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    LOGI("NativeBridge: StartPlay invoked");
    return static_cast<jint>(Engine::Instance().StartPlay());
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativePausePlay(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    LOGI("NativeBridge: PausePlay invoked");
    return static_cast<jint>(Engine::Instance().PausePlay());
}

JNIEXPORT jint JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeStopPlay(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    LOGI("NativeBridge: StopPlay invoked");
    return static_cast<jint>(Engine::Instance().StopPlay());
}

JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeIsPlaying(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    return Engine::Instance().IsPlaying() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeIsPaused(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    return Engine::Instance().IsPaused() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeSetVehicleInput(
    JNIEnv* /* env */,
    jobject /* thiz */,
    jfloat throttle,
    jfloat steer,
    jfloat brake,
    jboolean handbrake) {
    Engine::Instance().SetVehicleInput(throttle, steer, brake, handbrake == JNI_TRUE);
}

JNIEXPORT jfloat JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeGetVehicleSpeedKmh(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    if (auto* vehicle = Engine::Instance().GetActiveVehicle()) {
        return vehicle->GetSpeedKmh();
    }
    return 0.0f;
}

JNIEXPORT jfloat JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeGetVehicleRpm(
    JNIEnv* /* env */,
    jobject /* thiz */) {
    if (auto* vehicle = Engine::Instance().GetActiveVehicle()) {
        return vehicle->GetRpm();
    }
    return 0.0f;
}

JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeRaycast(
    JNIEnv* env,
    jobject /* thiz */,
    jfloat ox, jfloat oy, jfloat oz,
    jfloat dx, jfloat dy, jfloat dz,
    jfloat maxDist,
    jfloatArray outHitData) {
    
    physics::RayHit hit = Engine::Instance().GetPhysicsWorld().Raycast(
        physics::Vec3{ox, oy, oz},
        physics::Vec3{dx, dy, dz},
        maxDist
    );

    if (!hit.hasHit) return JNI_FALSE;

    jfloat buffer[8];
    buffer[0] = 1.0f;
    buffer[1] = hit.point.x;
    buffer[2] = hit.point.y;
    buffer[3] = hit.point.z;
    buffer[4] = hit.normal.x;
    buffer[5] = hit.normal.y;
    buffer[6] = hit.normal.z;
    buffer[7] = hit.distance;

    env->SetFloatArrayRegion(outHitData, 0, 8, buffer);
    return JNI_TRUE;
}

// Scripting JNI Methods (Doc 08 & Milestone M5)
JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeCompileScript(
    JNIEnv* env,
    jobject /* thiz */,
    jstring scriptSource,
    jobjectArray outDiagnostics) {
    
    if (!scriptSource) return JNI_FALSE;
    const char* srcChars = env->GetStringUTFChars(scriptSource, nullptr);
    std::string source = srcChars ? srcChars : "";
    if (srcChars) env->ReleaseStringUTFChars(scriptSource, srcChars);

    qivforge::script::Lexer lexer(source);
    auto tokens = lexer.Tokenize();
    qivforge::script::Parser parser(tokens);
    auto ast = parser.Parse();

    if (parser.HasErrors()) {
        LOGE("NativeBridge: Script syntax errors encountered");
        return JNI_FALSE;
    }

    qivforge::script::Compiler compiler(*ast);
    auto module = compiler.Compile();

    if (compiler.HasErrors()) {
        LOGE("NativeBridge: Script semantic/compiler errors encountered");
        return JNI_FALSE;
    }

    LOGI("NativeBridge: Script compiled successfully (Class: %s, Functions: %zu, Fields: %zu)",
         module->className.c_str(), module->functions.size(), module->exportedFields.size());
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeAttachScript(
    JNIEnv* env,
    jobject /* thiz */,
    jint actorId,
    jstring scriptSource) {
    
    if (!scriptSource) return JNI_FALSE;
    const char* srcChars = env->GetStringUTFChars(scriptSource, nullptr);
    std::string source = srcChars ? srcChars : "";
    if (srcChars) env->ReleaseStringUTFChars(scriptSource, srcChars);

    qivforge::script::Lexer lexer(source);
    auto tokens = lexer.Tokenize();
    qivforge::script::Parser parser(tokens);
    auto ast = parser.Parse();
    if (parser.HasErrors()) return JNI_FALSE;

    qivforge::script::Compiler compiler(*ast);
    auto module = std::shared_ptr<qivforge::script::BytecodeModule>(compiler.Compile().release());
    if (compiler.HasErrors() || !module) return JNI_FALSE;

    auto instance = std::make_shared<qivforge::script::ScriptInstance>(static_cast<uint32_t>(actorId), module);
    s_scriptInstances[static_cast<uint32_t>(actorId)] = instance;
    s_qvm.RegisterValidHandle(static_cast<uint32_t>(actorId), 1);

    instance->OnAwake(s_qvm);
    instance->OnStart(s_qvm);
    LOGI("NativeBridge: Script attached to actor %d", actorId);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeHotReloadScript(
    JNIEnv* env,
    jobject /* thiz */,
    jint actorId,
    jstring newSource) {
    
    auto it = s_scriptInstances.find(static_cast<uint32_t>(actorId));
    if (it == s_scriptInstances.end()) return JNI_FALSE;

    if (!newSource) return JNI_FALSE;
    const char* srcChars = env->GetStringUTFChars(newSource, nullptr);
    std::string source = srcChars ? srcChars : "";
    if (srcChars) env->ReleaseStringUTFChars(newSource, srcChars);

    qivforge::script::Lexer lexer(source);
    auto tokens = lexer.Tokenize();
    qivforge::script::Parser parser(tokens);
    auto ast = parser.Parse();
    if (parser.HasErrors()) return JNI_FALSE;

    qivforge::script::Compiler compiler(*ast);
    auto newModule = std::shared_ptr<qivforge::script::BytecodeModule>(compiler.Compile().release());
    if (compiler.HasErrors() || !newModule) return JNI_FALSE;

    bool success = it->second->HotReload(newModule);
    LOGI("NativeBridge: Script hot reloaded for actor %d (Success: %d)", actorId, success);
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jdouble JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeGetScriptFieldValue(
    JNIEnv* env,
    jobject /* thiz */,
    jint actorId,
    jstring fieldName) {
    
    auto it = s_scriptInstances.find(static_cast<uint32_t>(actorId));
    if (it == s_scriptInstances.end() || !fieldName) return 0.0;

    const char* nameChars = env->GetStringUTFChars(fieldName, nullptr);
    std::string name = nameChars ? nameChars : "";
    if (nameChars) env->ReleaseStringUTFChars(fieldName, nameChars);

    return it->second->GetFieldValue(name);
}

JNIEXPORT void JNICALL
Java_io_qivora_qivforge_engine_NativeBridge_nativeSetScriptFieldValue(
    JNIEnv* env,
    jobject /* thiz */,
    jint actorId,
    jstring fieldName,
    jdouble value) {
    
    auto it = s_scriptInstances.find(static_cast<uint32_t>(actorId));
    if (it == s_scriptInstances.end() || !fieldName) return;

    const char* nameChars = env->GetStringUTFChars(fieldName, nullptr);
    std::string name = nameChars ? nameChars : "";
    if (nameChars) env->ReleaseStringUTFChars(fieldName, nameChars);

    it->second->SetFieldValue(name, value);
}

} // extern "C"
