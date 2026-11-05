package io.qivora.qivforge.ui.terrain

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
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

enum class SculptMode(val label: String) {
    RAISE("Raise"),
    LOWER("Lower"),
    SMOOTH("Smooth"),
    FLATTEN("Flatten"),
    SET_HEIGHT("Set Height"),
    PAINT_LAYER("Paint Layer"),
    SCATTER("Scatter")
}

enum class FalloffOption(val label: String) {
    SMOOTH("Smooth"),
    LINEAR("Linear"),
    SHARP("Sharp")
}

@Composable
fun TerrainToolsScreen(
    theme: ThemeTokens,
    onBrushSettingsChanged: (mode: SculptMode, size: Float, strength: Float, falloff: FalloffOption) -> Unit,
    onUndoStroke: () -> Unit,
    onRedoStroke: () -> Unit,
    onClose: () -> Unit,
    modifier: Modifier = Modifier
) {
    var selectedMode by remember { mutableStateOf(SculptMode.RAISE) }
    var selectedFalloff by remember { mutableStateOf(FalloffOption.SMOOTH) }
    var brushSize by remember { mutableFloatStateOf(25.0f) }
    var brushStrength by remember { mutableFloatStateOf(0.5f) }
    var selectedLayerIndex by remember { mutableIntStateOf(0) }
    var cameraLock by remember { mutableStateOf(true) }

    val layerColors = listOf(
        "Grass" to Color(0xFF4CAF50),
        "Mud" to Color(0xFF795548),
        "Dirt" to Color(0xFF8D6E63),
        "Asphalt" to Color(0xFF424242),
        "Sand" to Color(0xFFFFD54F),
        "Ice" to Color(0xFF80DEEA)
    )

    LaunchedEffect(selectedMode, brushSize, brushStrength, selectedFalloff) {
        onBrushSettingsChanged(selectedMode, brushSize, brushStrength, selectedFalloff)
    }

    Surface(
        modifier = modifier
            .fillMaxWidth()
            .heightIn(max = 340.dp),
        color = theme.surfaceColor,
        shape = RoundedCornerShape(topStart = 16.dp, topEnd = 16.dp),
        tonalElevation = 8.dp
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp)
                .verticalScroll(rememberScrollState())
        ) {
            // Header Bar
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "Terrain Sculptor",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Bold,
                    color = theme.textColor
                )

                Row(verticalAlignment = Alignment.CenterVertically) {
                    FilterChip(
                        selected = cameraLock,
                        onClick = { cameraLock = !cameraLock },
                        label = { Text(if (cameraLock) "Camera Locked" else "Camera Free", fontSize = 11.sp) }
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    OutlinedButton(
                        onClick = onUndoStroke,
                        contentPadding = PaddingValues(horizontal = 8.dp, vertical = 4.dp)
                    ) {
                        Text("Undo", fontSize = 12.sp)
                    }
                    Spacer(modifier = Modifier.width(4.dp))
                    OutlinedButton(
                        onClick = onRedoStroke,
                        contentPadding = PaddingValues(horizontal = 8.dp, vertical = 4.dp)
                    ) {
                        Text("Redo", fontSize = 12.sp)
                    }
                    Spacer(modifier = Modifier.width(8.dp))
                    TextButton(onClick = onClose) {
                        Text("✕", fontSize = 16.sp, color = theme.textColor)
                    }
                }
            }

            Spacer(modifier = Modifier.height(10.dp))

            // Tool Chips Scroll Row
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                SculptMode.values().forEach { mode ->
                    FilterChip(
                        selected = selectedMode == mode,
                        onClick = { selectedMode = mode },
                        label = { Text(mode.label, fontSize = 12.sp) }
                    )
                }
            }

            Spacer(modifier = Modifier.height(12.dp))

            // Layer Swatches if Paint Layer is chosen
            if (selectedMode == SculptMode.PAINT_LAYER) {
                Text("Texture Layers (Doc 10 §A1)", fontSize = 12.sp, color = theme.secondaryTextColor)
                Spacer(modifier = Modifier.height(6.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(10.dp)
                ) {
                    layerColors.forEachIndexed { index, (name, color) ->
                        Column(
                            horizontalAlignment = Alignment.CenterHorizontally,
                            modifier = Modifier.clickable { selectedLayerIndex = index }
                        ) {
                            Box(
                                modifier = Modifier
                                    .size(36.dp)
                                    .clip(CircleShape)
                                    .background(color)
                                    .then(
                                        if (selectedLayerIndex == index)
                                            Modifier.border(2.dp, theme.primaryColor, CircleShape)
                                        else Modifier
                                    )
                            )
                            Text(name, fontSize = 10.sp, color = theme.textColor)
                        }
                    }
                }
                Spacer(modifier = Modifier.height(12.dp))
            }

            // Brush Size Slider
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text("Size: ${brushSize.toInt()}m", fontSize = 12.sp, color = theme.textColor, modifier = Modifier.width(70.dp))
                Slider(
                    value = brushSize,
                    onValueChange = { brushSize = it },
                    valueRange = 1.0f..100.0f,
                    modifier = Modifier.weight(1.0f)
                )
            }

            // Brush Strength Slider
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text("Strength: ${(brushStrength * 100).toInt()}%", fontSize = 12.sp, color = theme.textColor, modifier = Modifier.width(70.dp))
                Slider(
                    value = brushStrength,
                    onValueChange = { brushStrength = it },
                    valueRange = 0.05f..1.0f,
                    modifier = Modifier.weight(1.0f)
                )
            }

            // Falloff Selector
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text("Falloff:", fontSize = 12.sp, color = theme.textColor)
                FalloffOption.values().forEach { fo ->
                    FilterChip(
                        selected = selectedFalloff == fo,
                        onClick = { selectedFalloff = fo },
                        label = { Text(fo.label, fontSize = 11.sp) }
                    )
                }
            }
        }
    }
}
