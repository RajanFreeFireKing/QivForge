package io.qivora.qivforge.ui.vfx

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeTokens

enum class VfxAudioTab {
    VFX_PARTICLES,
    AUDIO_MIXER
}

data class BusState(
    val name: String,
    var volume: Float = 0.8f,
    var muted: Boolean = false,
    var solo: Boolean = false
)

@Composable
fun VfxAudioEditorScreen(
    theme: ThemeTokens,
    onClose: () -> Unit,
    modifier: Modifier = Modifier
) {
    var selectedTab by remember { mutableStateOf(VfxAudioTab.VFX_PARTICLES) }

    // VFX State
    var selectedVfxPreset by remember { mutableStateOf("MuzzleFlash") }
    var emissionRate by remember { mutableFloatStateOf(40.0f) }
    var maxParticles by remember { mutableFloatStateOf(1000.0f) }
    var particleLifetime by remember { mutableFloatStateOf(0.5f) }
    var activeParticleCount by remember { mutableIntStateOf(180) }

    // Audio Mixer State (6 buses per Doc 10 §C4)
    var buses by remember {
        mutableStateOf(
            listOf(
                BusState("Master", 1.0f),
                BusState("Music", 0.75f),
                BusState("SFX", 0.9f),
                BusState("Voice", 1.0f),
                BusState("Ambient", 0.6f),
                BusState("UI", 0.8f)
            )
        )
    }
    var isDuckingActive by remember { mutableStateOf(false) }

    Surface(
        modifier = modifier
            .fillMaxWidth()
            .heightIn(max = 380.dp),
        color = theme.surfaceColor,
        shape = RoundedCornerShape(topStart = 16.dp, topEnd = 16.dp),
        tonalElevation = 8.dp
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp)
        ) {
            // Header with tab selector
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                TabRow(
                    selectedTabIndex = selectedTab.ordinal,
                    modifier = Modifier.width(320.dp),
                    containerColor = theme.surfaceColor,
                    contentColor = theme.primaryColor
                ) {
                    Tab(
                        selected = selectedTab == VfxAudioTab.VFX_PARTICLES,
                        onClick = { selectedTab = VfxAudioTab.VFX_PARTICLES },
                        text = { Text("Particle VFX", fontSize = 13.sp) }
                    )
                    Tab(
                        selected = selectedTab == VfxAudioTab.AUDIO_MIXER,
                        onClick = { selectedTab = VfxAudioTab.AUDIO_MIXER },
                        text = { Text("Audio Mixer", fontSize = 13.sp) }
                    )
                }

                TextButton(onClick = onClose) {
                    Text("✕", fontSize = 16.sp, color = theme.textColor)
                }
            }

            Spacer(modifier = Modifier.height(12.dp))

            if (selectedTab == VfxAudioTab.VFX_PARTICLES) {
                // --- VFX Tab ---
                Row(
                    modifier = Modifier
                        .fillMaxSize()
                        .weight(1.0f)
                ) {
                    // Left: Presets & Controls
                    Column(
                        modifier = Modifier
                            .weight(1.0f)
                            .verticalScroll(rememberScrollState()),
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Text("Effect Preset:", fontSize = 12.sp, color = theme.textColor)
                        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            listOf("MuzzleFlash", "BulletSparks", "DirtImpact", "Exhaust", "Campfire").forEach { preset ->
                                FilterChip(
                                    selected = selectedVfxPreset == preset,
                                    onClick = { selectedVfxPreset = preset },
                                    label = { Text(preset, fontSize = 10.sp) }
                                )
                            }
                        }

                        // Sliders
                        Text("Rate: ${emissionRate.toInt()} /s", fontSize = 11.sp, color = theme.textColor)
                        Slider(
                            value = emissionRate,
                            onValueChange = { emissionRate = it },
                            valueRange = 0f..200f
                        )

                        Text("Max Particles: ${maxParticles.toInt()}", fontSize = 11.sp, color = theme.textColor)
                        Slider(
                            value = maxParticles,
                            onValueChange = { maxParticles = it },
                            valueRange = 50f..2000f // Hard cap 2,000 per emitter (Doc 10 §C1)
                        )

                        Button(
                            onClick = { activeParticleCount = (activeParticleCount + 40).coerceAtMost(2000) },
                            contentPadding = PaddingValues(horizontal = 12.dp, vertical = 6.dp)
                        ) {
                            Text("Burst Test (Space/Tap)", fontSize = 11.sp)
                        }
                    }

                    Spacer(modifier = Modifier.width(16.dp))

                    // Right: Cost Meter & Stats (Doc 10 §C3)
                    Surface(
                        modifier = Modifier
                            .width(160.dp)
                            .fillMaxHeight(),
                        color = theme.backgroundColor,
                        shape = RoundedCornerShape(8.dp)
                    ) {
                        Column(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(12.dp),
                            verticalArrangement = Arrangement.Center,
                            horizontalAlignment = Alignment.CenterHorizontally
                        ) {
                            Text("VFX Cost Meter", fontSize = 12.sp, fontWeight = FontWeight.Bold, color = theme.textColor)
                            Spacer(modifier = Modifier.height(8.dp))
                            Text("$activeParticleCount", fontSize = 28.sp, fontWeight = FontWeight.ExtraBold, color = theme.primaryColor)
                            Text("Active Particles", fontSize = 10.sp, color = theme.secondaryTextColor)
                            Spacer(modifier = Modifier.height(10.dp))
                            val cpuMs = (activeParticleCount * 0.00006f).coerceAtLeast(0.01f)
                            Text(String.format("%.2f ms CPU", cpuMs), fontSize = 13.sp, color = if (cpuMs < 0.15f) Color(0xFF4CAF50) else Color(0xFFFF9800))
                            Text("Budget < 0.15 ms", fontSize = 9.sp, color = Color.Gray)
                        }
                    }
                }
            } else {
                // --- Audio Mixer Tab (Doc 10 §C4 & §C5) ---
                Column(modifier = Modifier.fillMaxSize()) {
                    if (isDuckingActive) {
                        Surface(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(bottom = 8.dp),
                            color = Color(0xFF2E7D32).copy(alpha = 0.2f),
                            shape = RoundedCornerShape(4.dp)
                        ) {
                            Text(
                                "⚡ Voice Ducking Active: Music gain reduced to 40%",
                                modifier = Modifier.padding(6.dp),
                                fontSize = 11.sp,
                                color = Color(0xFF81C784)
                            )
                        }
                    }

                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .weight(1.0f),
                        horizontalArrangement = Arrangement.SpaceEvenly
                    ) {
                        buses.forEachIndexed { idx, bus ->
                            Column(
                                horizontalAlignment = Alignment.CenterHorizontally,
                                modifier = Modifier.width(52.dp)
                            ) {
                                Text(bus.name, fontSize = 11.sp, fontWeight = FontWeight.Bold, color = theme.textColor)
                                Text("${(bus.volume * 100).toInt()}%", fontSize = 10.sp, color = theme.secondaryTextColor)

                                Slider(
                                    value = bus.volume,
                                    onValueChange = { newVol ->
                                        buses = buses.toMutableList().also { it[idx] = it[idx].copy(volume = newVol) }
                                    },
                                    valueRange = 0f..1f,
                                    modifier = Modifier
                                        .weight(1.0f)
                                        .padding(vertical = 4.dp)
                                )

                                Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                                    Box(
                                        modifier = Modifier
                                            .size(22.dp)
                                            .clip(RoundedCornerShape(4.dp))
                                            .background(if (bus.muted) Color.Red else Color.DarkGray)
                                            .clickable {
                                                buses = buses.toMutableList().also { it[idx] = it[idx].copy(muted = !it[idx].muted) }
                                            },
                                        contentAlignment = Alignment.Center
                                    ) {
                                        Text("M", fontSize = 9.sp, fontWeight = FontWeight.Bold, color = Color.White)
                                    }

                                    Box(
                                        modifier = Modifier
                                            .size(22.dp)
                                            .clip(RoundedCornerShape(4.dp))
                                            .background(if (bus.solo) Color(0xFFFFB300) else Color.DarkGray)
                                            .clickable {
                                                buses = buses.toMutableList().also { it[idx] = it[idx].copy(solo = !it[idx].solo) }
                                            },
                                        contentAlignment = Alignment.Center
                                    ) {
                                        Text("S", fontSize = 9.sp, fontWeight = FontWeight.Bold, color = Color.White)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
