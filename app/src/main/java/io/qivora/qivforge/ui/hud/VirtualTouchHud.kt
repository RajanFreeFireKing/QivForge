package io.qivora.qivforge.ui.hud

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeTokens
import kotlin.math.roundToInt
import kotlin.math.sqrt

/**
 * Virtual Touch HUD for vehicle & game runtime per Doc 04 §14 and Doc 06 §9.7.
 * Satisfies Doc 04 §14 AC4: Multi-touch ownership ensures throttle and brake
 * both register simultaneously without interference.
 */
@Composable
fun VirtualTouchHud(
    theme: ThemeTokens,
    speedKmh: Float = 0.0f,
    rpm: Float = 800.0f,
    gear: Int = 1,
    onInputChanged: (throttle: Float, steer: Float, brake: Float, handbrake: Boolean) -> Unit,
    onStopPlay: () -> Unit,
    modifier: Modifier = Modifier
) {
    var steerValue by remember { mutableFloatStateOf(0.0f) }
    var throttleValue by remember { mutableFloatStateOf(0.0f) }
    var brakeValue by remember { mutableFloatStateOf(0.0f) }
    var handbrakeValue by remember { mutableStateOf(false) }

    // Notify input changes
    LaunchedEffect(steerValue, throttleValue, brakeValue, handbrakeValue) {
        onInputChanged(throttleValue, steerValue, brakeValue, handbrakeValue)
    }

    Box(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        // --- Top Bar: Telemetry & Stop Play Button ---
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .align(Alignment.TopCenter),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Stop Play pill
            Box(
                modifier = Modifier
                    .clip(RoundedCornerShape(20.dp))
                    .background(theme.error.copy(alpha = 0.85f))
                    .pointerInput(Unit) {
                        detectDragGestures(onDrag = { _, _ -> }) {
                            // no-op drag
                        }
                    }
                    .padding(horizontal = 14.dp, vertical = 8.dp),
                contentAlignment = Alignment.Center
            ) {
                androidx.compose.material3.Button(
                    onClick = onStopPlay,
                    colors = androidx.compose.material3.ButtonDefaults.buttonColors(
                        containerColor = Color.Transparent,
                        contentColor = Color.White
                    ),
                    contentPadding = PaddingValues(0.dp)
                ) {
                    Text(
                        text = "⏹ Stop Play",
                        fontWeight = FontWeight.Bold,
                        fontSize = 13.sp,
                        color = Color.White
                    )
                }
            }

            // Central Telemetry HUD
            Box(
                modifier = Modifier
                    .clip(RoundedCornerShape(12.dp))
                    .background(theme.surface.copy(alpha = 0.85f))
                    .padding(horizontal = 16.dp, vertical = 6.dp)
            ) {
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Text(
                            text = "${speedKmh.roundToInt()}",
                            fontSize = 22.sp,
                            fontWeight = FontWeight.Black,
                            fontFamily = FontFamily.Monospace,
                            color = theme.onSurface
                        )
                        Text(
                            text = "KM/H",
                            fontSize = 10.sp,
                            fontWeight = FontWeight.Bold,
                            color = theme.onSurfaceMuted
                        )
                    }

                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Text(
                            text = "${rpm.roundToInt()}",
                            fontSize = 16.sp,
                            fontWeight = FontWeight.Bold,
                            fontFamily = FontFamily.Monospace,
                            color = if (rpm > 2300.0f) theme.error else theme.primary
                        )
                        Text(
                            text = "RPM",
                            fontSize = 10.sp,
                            fontWeight = FontWeight.Bold,
                            color = theme.onSurfaceMuted
                        )
                    }

                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Text(
                            text = if (gear == -1) "R" else if (gear == 0) "N" else "$gear",
                            fontSize = 18.sp,
                            fontWeight = FontWeight.ExtraBold,
                            color = theme.accent
                        )
                        Text(
                            text = "GEAR",
                            fontSize = 10.sp,
                            fontWeight = FontWeight.Bold,
                            color = theme.onSurfaceMuted
                        )
                    }
                }
            }
        }

        // --- Bottom Controls ---
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .align(Alignment.BottomCenter)
                .padding(bottom = 8.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.Bottom
        ) {
            // Left Thumb: Analog Steering Wheel / Joystick
            Box(
                modifier = Modifier
                    .size(130.dp)
                    .clip(CircleShape)
                    .background(theme.surface.copy(alpha = 0.75f)),
                contentAlignment = Alignment.Center
            ) {
                var thumbOffset by remember { mutableStateOf(Offset.Zero) }

                Box(
                    modifier = Modifier
                        .size(60.dp)
                        .offset { IntOffset(thumbOffset.x.roundToInt(), thumbOffset.y.roundToInt()) }
                        .clip(CircleShape)
                        .background(theme.primary)
                        .pointerInput(Unit) {
                            detectDragGestures(
                                onDragEnd = {
                                    thumbOffset = Offset.Zero
                                    steerValue = 0.0f
                                },
                                onDragCancel = {
                                    thumbOffset = Offset.Zero
                                    steerValue = 0.0f
                                },
                                onDrag = { change, dragAmount ->
                                    change.consume()
                                    val newOffset = thumbOffset + dragAmount
                                    val maxRadius = 45f
                                    val dist = sqrt(newOffset.x * newOffset.x + newOffset.y * newOffset.y)
                                    val clamped = if (dist > maxRadius) {
                                        newOffset * (maxRadius / dist)
                                    } else {
                                        newOffset
                                    }
                                    thumbOffset = clamped
                                    steerValue = (clamped.x / maxRadius).coerceIn(-1.0f, 1.0f)
                                }
                            )
                        },
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = "STEER",
                        fontSize = 10.sp,
                        fontWeight = FontWeight.Bold,
                        color = theme.onPrimary
                    )
                }
            }

            // Right Thumb: Independent Accelerator and Brake Pedals (Multi-touch per Doc 04 §14 AC4)
            Row(
                horizontalArrangement = Arrangement.spacedBy(16.dp),
                verticalAlignment = Alignment.Bottom
            ) {
                // Brake Pedal
                Box(
                    modifier = Modifier
                        .size(width = 65.dp, height = 95.dp)
                        .clip(RoundedCornerShape(12.dp))
                        .background(
                            if (brakeValue > 0.05f) theme.error else theme.surface.copy(alpha = 0.8f)
                        )
                        .pointerInput(Unit) {
                            detectDragGestures(
                                onDragStart = { brakeValue = 1.0f },
                                onDragEnd = { brakeValue = 0.0f },
                                onDragCancel = { brakeValue = 0.0f },
                                onDrag = { change, _ ->
                                    change.consume()
                                    brakeValue = 1.0f
                                }
                            )
                        },
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = "BRAKE",
                        fontSize = 11.sp,
                        fontWeight = FontWeight.Black,
                        color = if (brakeValue > 0.05f) Color.White else theme.error
                    )
                }

                // Gas / Accelerator Pedal
                Box(
                    modifier = Modifier
                        .size(width = 65.dp, height = 120.dp)
                        .clip(RoundedCornerShape(12.dp))
                        .background(
                            if (throttleValue > 0.05f) theme.success else theme.surface.copy(alpha = 0.8f)
                        )
                        .pointerInput(Unit) {
                            detectDragGestures(
                                onDragStart = { throttleValue = 1.0f },
                                onDragEnd = { throttleValue = 0.0f },
                                onDragCancel = { throttleValue = 0.0f },
                                onDrag = { change, _ ->
                                    change.consume()
                                    throttleValue = 1.0f
                                }
                            )
                        },
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = "GAS",
                        fontSize = 12.sp,
                        fontWeight = FontWeight.Black,
                        color = if (throttleValue > 0.05f) Color.White else theme.success
                    )
                }
            }
        }
    }
}
