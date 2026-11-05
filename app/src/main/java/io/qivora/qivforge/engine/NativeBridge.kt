package io.qivora.qivforge.engine

import android.view.Surface
import io.qivora.qivforge.core.ErrorCode

data class EngineStats(
    val fps: Float = 0.0f,
    val frameMs: Float = 0.0f,
    val drawCalls: Int = 0,
    val triangles: Int = 0
)

data class RayHitResult(
    val hasHit: Boolean = false,
    val hitPointX: Float = 0.0f,
    val hitPointY: Float = 0.0f,
    val hitPointZ: Float = 0.0f,
    val normalX: Float = 0.0f,
    val normalY: Float = 1.0f,
    val normalZ: Float = 0.0f,
    val distance: Float = 0.0f
)

object NativeBridge {
    private var isLoaded = false
    private var hostActorCounter = 1
    private var hostIsPlaying = false
    private var hostIsPaused = false

    init {
        try {
            System.loadLibrary("qivengine")
            isLoaded = true
        } catch (e: UnsatisfiedLinkError) {
            // For unit testing environments on host JVM without libqivengine.so
            isLoaded = false
        }
    }

    fun isNativeLoaded(): Boolean = isLoaded

    fun init(appName: String = "QivForge", targetFps: Int = 60, useVulkan: Boolean = true): ErrorCode {
        if (!isLoaded) return ErrorCode.ERR_CORE_NOT_INITIALIZED
        val code = nativeInit(appName, targetFps, useVulkan)
        return ErrorCode.fromCode(code)
    }

    fun shutdown(): ErrorCode {
        if (!isLoaded) return ErrorCode.ERR_CORE_NOT_INITIALIZED
        val code = nativeShutdown()
        return ErrorCode.fromCode(code)
    }

    fun onSurfaceCreated(surface: Surface): ErrorCode {
        if (!isLoaded) return ErrorCode.ERR_CORE_NOT_INITIALIZED
        val code = nativeOnSurfaceCreated(surface)
        return ErrorCode.fromCode(code)
    }

    fun onSurfaceChanged(width: Int, height: Int): ErrorCode {
        if (!isLoaded) return ErrorCode.ERR_CORE_NOT_INITIALIZED
        val code = nativeOnSurfaceChanged(width, height)
        return ErrorCode.fromCode(code)
    }

    fun onSurfaceDestroyed(): ErrorCode {
        if (!isLoaded) return ErrorCode.ERR_CORE_NOT_INITIALIZED
        val code = nativeOnSurfaceDestroyed()
        return ErrorCode.fromCode(code)
    }

    fun tick(deltaTime: Float): ErrorCode {
        if (!isLoaded) return ErrorCode.ERR_CORE_NOT_INITIALIZED
        val code = nativeTick(deltaTime)
        return ErrorCode.fromCode(code)
    }

    fun getFps(): Float = if (isLoaded) nativeGetFps() else 60.0f
    fun getFrameMs(): Float = if (isLoaded) nativeGetFrameMs() else 16.6f
    fun getDrawCalls(): Int = if (isLoaded) nativeGetDrawCalls() else 1
    fun getTriangles(): Int = if (isLoaded) nativeGetTriangles() else 12

    fun getStats(): EngineStats {
        return EngineStats(
            fps = getFps(),
            frameMs = getFrameMs(),
            drawCalls = getDrawCalls(),
            triangles = getTriangles()
        )
    }

    fun pollNextEvent(outArray: IntArray): Boolean {
        if (!isLoaded || outArray.size < 5) return false
        return nativePollNextEvent(outArray)
    }

    // Scene & Undo Methods (Doc 02 §16 AC2 / Doc 15 M3)
    fun createActor(name: String, parentId: Int = 0): Int {
        return if (isLoaded) {
            nativeCreateActor(name, parentId)
        } else {
            hostActorCounter++
        }
    }

    fun destroyActor(actorId: Int): Boolean {
        return if (isLoaded) {
            nativeDestroyActor(actorId)
        } else {
            true
        }
    }

    fun undo(): Boolean {
        return if (isLoaded) nativeUndo() else true
    }

    fun redo(): Boolean {
        return if (isLoaded) nativeRedo() else true
    }

    fun getActorCount(): Int {
        return if (isLoaded) nativeGetActorCount() else hostActorCounter
    }

    // Instant Play Mode & Physics (Rule R5 & Doc 15 M4)
    fun startPlay(): ErrorCode {
        return if (isLoaded) {
            ErrorCode.fromCode(nativeStartPlay())
        } else {
            hostIsPlaying = true
            hostIsPaused = false
            ErrorCode.OK
        }
    }

    fun pausePlay(): ErrorCode {
        return if (isLoaded) {
            ErrorCode.fromCode(nativePausePlay())
        } else {
            hostIsPaused = true
            ErrorCode.OK
        }
    }

    fun stopPlay(): ErrorCode {
        return if (isLoaded) {
            ErrorCode.fromCode(nativeStopPlay())
        } else {
            hostIsPlaying = false
            hostIsPaused = false
            ErrorCode.OK
        }
    }

    fun isPlaying(): Boolean = if (isLoaded) nativeIsPlaying() else hostIsPlaying
    fun isPaused(): Boolean = if (isLoaded) nativeIsPaused() else hostIsPaused

    fun setVehicleInput(throttle: Float, steer: Float, brake: Float, handbrake: Boolean = false) {
        if (isLoaded) {
            nativeSetVehicleInput(throttle, steer, brake, handbrake)
        }
    }

    fun getVehicleSpeedKmh(): Float = if (isLoaded) nativeGetVehicleSpeedKmh() else 0.0f
    fun getVehicleRpm(): Float = if (isLoaded) nativeGetVehicleRpm() else 800.0f

    fun raycast(
        originX: Float, originY: Float, originZ: Float,
        dirX: Float, dirY: Float, dirZ: Float,
        maxDist: Float = 1000.0f
    ): RayHitResult {
        if (!isLoaded) return RayHitResult()
        val outData = FloatArray(8)
        val hit = nativeRaycast(originX, originY, originZ, dirX, dirY, dirZ, maxDist, outData)
        return if (hit) {
            RayHitResult(
                hasHit = true,
                hitPointX = outData[1],
                hitPointY = outData[2],
                hitPointZ = outData[3],
                normalX = outData[4],
                normalY = outData[5],
                normalZ = outData[6],
                distance = outData[7]
            )
        } else {
            RayHitResult()
        }
    }

    // Scripting Bridge Methods (Doc 08 & Milestone M5)
    fun compileScript(source: String): Boolean {
        if (!isLoaded) return true // Host preview fallback
        return nativeCompileScript(source, null)
    }

    fun attachScript(actorId: Int, source: String): Boolean {
        if (!isLoaded) return true
        return nativeAttachScript(actorId, source)
    }

    fun hotReloadScript(actorId: Int, newSource: String): Boolean {
        if (!isLoaded) return true
        return nativeHotReloadScript(actorId, newSource)
    }

    fun getScriptFieldValue(actorId: Int, fieldName: String): Double {
        if (!isLoaded) return 0.0
        return nativeGetScriptFieldValue(actorId, fieldName)
    }

    fun setScriptFieldValue(actorId: Int, fieldName: String, value: Double) {
        if (isLoaded) {
            nativeSetScriptFieldValue(actorId, fieldName, value)
        }
    }

    // External native signatures
    private external fun nativeInit(appName: String, targetFps: Int, useVulkan: Boolean): Int
    private external fun nativeShutdown(): Int
    private external fun nativeOnSurfaceCreated(surface: Surface): Int
    private external fun nativeOnSurfaceChanged(width: Int, height: Int): Int
    private external fun nativeOnSurfaceDestroyed(): Int
    private external fun nativeTick(deltaTime: Float): Int
    private external fun nativeGetFps(): Float
    private external fun nativeGetFrameMs(): Float
    private external fun nativeGetDrawCalls(): Int
    private external fun nativeGetTriangles(): Int
    private external fun nativePollNextEvent(outData: IntArray): Boolean

    private external fun nativeCreateActor(name: String, parentId: Int): Int
    private external fun nativeDestroyActor(actorId: Int): Boolean
    private external fun nativeUndo(): Boolean
    private external fun nativeRedo(): Boolean
    private external fun nativeGetActorCount(): Int

    private external fun nativeStartPlay(): Int
    private external fun nativePausePlay(): Int
    private external fun nativeStopPlay(): Int
    private external fun nativeIsPlaying(): Boolean
    private external fun nativeIsPaused(): Boolean
    private external fun nativeSetVehicleInput(throttle: Float, steer: Float, brake: Float, handbrake: Boolean)
    private external fun nativeGetVehicleSpeedKmh(): Float
    private external fun nativeGetVehicleRpm(): Float
    private external fun nativeRaycast(
        ox: Float, oy: Float, oz: Float,
        dx: Float, dy: Float, dz: Float,
        maxDist: Float,
        outHitData: FloatArray
    ): Boolean

    private external fun nativeCompileScript(scriptSource: String, outDiagnostics: Array<String>?): Boolean
    private external fun nativeAttachScript(actorId: Int, scriptSource: String): Boolean
    private external fun nativeHotReloadScript(actorId: Int, newSource: String): Boolean
    private external fun nativeGetScriptFieldValue(actorId: Int, fieldName: String): Double
    private external fun nativeSetScriptFieldValue(actorId: Int, fieldName: String, value: Double)
}
