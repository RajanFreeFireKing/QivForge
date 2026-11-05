package io.qivora.qivforge.ui.viewport

import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import io.qivora.qivforge.engine.EngineStats
import io.qivora.qivforge.engine.NativeBridge
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive

/**
 * 3D Viewport hosting the native Filament surface and Stats Badge overlay per Doc 02 §6 / Doc 04 §6
 */
@Composable
fun EngineViewport(
    modifier: Modifier = Modifier,
    showStatsBadge: Boolean = true
) {
    var stats by remember { mutableStateOf(EngineStats(fps = 60.0f, frameMs = 16.6f, drawCalls = 1, triangles = 12)) }

    // High frequency frame tick loop
    LaunchedEffect(Unit) {
        var lastTime = System.nanoTime()
        val eventBuffer = IntArray(5)

        while (isActive) {
            val now = System.nanoTime()
            val dt = ((now - lastTime) / 1_000_000_000.0f).coerceAtLeast(0.001f)
            lastTime = now

            if (NativeBridge.isNativeLoaded()) {
                NativeBridge.tick(dt)
                if (NativeBridge.pollNextEvent(eventBuffer)) {
                    // Process ring buffer stats event
                    val eventType = eventBuffer[0]
                    if (eventType == 1) { // STATS_UPDATED
                        val fps = eventBuffer[1].toFloat()
                        val frameMs = eventBuffer[2] / 10.0f
                        val dc = eventBuffer[3]
                        val tri = eventBuffer[4]
                        stats = EngineStats(fps, frameMs, dc, tri)
                    }
                } else {
                    stats = NativeBridge.getStats()
                }
            } else {
                // Host fallback simulation
                stats = EngineStats(60.0f, 16.6f, 1, 12)
            }

            delay(16) // ~60 Hz frame target
        }
    }

    Box(
        modifier = modifier
            .fillMaxSize()
            .background(Color.Black)
    ) {
        AndroidView(
            factory = { context ->
                SurfaceView(context).apply {
                    holder.addCallback(object : SurfaceHolder.Callback {
                        override fun surfaceCreated(holder: SurfaceHolder) {
                            NativeBridge.onSurfaceCreated(holder.surface)
                        }

                        override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                            NativeBridge.onSurfaceChanged(width, height)
                        }

                        override fun surfaceDestroyed(holder: SurfaceHolder) {
                            NativeBridge.onSurfaceDestroyed()
                        }
                    })
                }
            },
            modifier = Modifier.fillMaxSize()
        )

        if (showStatsBadge) {
            StatsBadge(
                stats = stats,
                modifier = Modifier
                    .align(Alignment.TopStart)
                    .padding(12.dp)
            )
        }
    }
}
