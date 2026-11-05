package io.qivora.qivforge.ui.hud

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeTokens

enum class DeviceAspectRatio(val label: String, val ratio: Float) {
    RATIO_16_9("16:9 Phone", 16f / 9f),
    RATIO_18_9("18:9 Modern", 18f / 9f),
    RATIO_20_9("20:9 Tall", 20f / 9f),
    RATIO_4_3("4:3 Tablet", 4f / 3f)
}

enum class DesignerPreset(val label: String) {
    RACING("Racing HUD"),
    TPS("TPS HUD"),
    TRACTOR("Tractor HUD"),
    MINIMAL("Minimal")
}

data class DesignerWidgetState(
    val id: String,
    val typeName: String,
    val x: Float,
    val y: Float,
    val widthDp: Float,
    val heightDp: Float,
    val actionName: String = "",
    val binding: String = ""
)

@Composable
fun HudDesignerScreen(
    theme: ThemeTokens,
    onSaveLayout: () -> Unit,
    onClose: () -> Unit,
    modifier: Modifier = Modifier
) {
    var selectedDevice by remember { mutableStateOf(DeviceAspectRatio.RATIO_16_9) }
    var selectedPreset by remember { mutableStateOf(DesignerPreset.RACING) }
    var showSafeArea by remember { mutableStateOf(true) }

    var widgets by remember {
        mutableStateOf(
            listOf(
                DesignerWidgetState("SteeringWheel", "SteeringWheel", 40f, 240f, 180f, 180f, "Steering"),
                DesignerWidgetState("BrakePedal", "Pedal", 480f, 260f, 60f, 120f, "Brake"),
                DesignerWidgetState("GasPedal", "Pedal", 560f, 240f, 60f, 140f, "Throttle"),
                DesignerWidgetState("Speedo", "Speedometer", 520f, 30f, 120f, 40f, "", "vehicle.speedKmh")
            )
        )
    }

    var selectedWidgetId by remember { mutableStateOf<String?>("SteeringWheel") }

    Column(
        modifier = modifier
            .fillMaxSize()
            .background(theme.backgroundColor)
    ) {
        // --- Top Bar ---
        Surface(
            modifier = Modifier.fillMaxWidth(),
            color = theme.surfaceColor,
            tonalElevation = 4.dp
        ) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp, vertical = 8.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "2D HUD Designer",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Bold,
                    color = theme.textColor
                )

                Row(
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    // Device switcher dropdown / chips
                    DeviceAspectRatio.values().forEach { dev ->
                        FilterChip(
                            selected = selectedDevice == dev,
                            onClick = { selectedDevice = dev },
                            label = { Text(dev.label, fontSize = 11.sp) }
                        )
                    }

                    FilterChip(
                        selected = showSafeArea,
                        onClick = { showSafeArea = !showSafeArea },
                        label = { Text("Safe Area", fontSize = 11.sp) }
                    )

                    Button(
                        onClick = onSaveLayout,
                        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 4.dp)
                    ) {
                        Text("Save HUD", fontSize = 12.sp)
                    }

                    TextButton(onClick = onClose) {
                        Text("✕", fontSize = 16.sp, color = theme.textColor)
                    }
                }
            }
        }

        // --- Main Body: Palette + Canvas + Inspector ---
        Row(
            modifier = Modifier
                .fillMaxSize()
                .weight(1.0f)
        ) {
            // Left Palette
            Surface(
                modifier = Modifier
                    .width(160.dp)
                    .fillMaxHeight(),
                color = theme.surfaceColor
            ) {
                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(8.dp)
                        .verticalScroll(rememberScrollState()),
                    verticalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    Text("Presets (Doc 10 §B6)", fontSize = 11.sp, fontWeight = FontWeight.Bold, color = theme.secondaryTextColor)
                    DesignerPreset.values().forEach { preset ->
                        OutlinedButton(
                            onClick = { selectedPreset = preset },
                            modifier = Modifier.fillMaxWidth(),
                            contentPadding = PaddingValues(4.dp)
                        ) {
                            Text(preset.label, fontSize = 11.sp)
                        }
                    }

                    Divider(modifier = Modifier.padding(vertical = 4.dp))
                    Text("Widget Library", fontSize = 11.sp, fontWeight = FontWeight.Bold, color = theme.secondaryTextColor)

                    listOf("Button", "Joystick", "SteeringWheel", "Pedal", "Speedometer", "ProgressBar", "Label").forEach { item ->
                        Surface(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clip(RoundedCornerShape(4.dp))
                                .border(1.dp, theme.dividerColor, RoundedCornerShape(4.dp))
                                .clickable {
                                    val newW = DesignerWidgetState(
                                        id = "${item}_${widgets.size + 1}",
                                        typeName = item,
                                        x = 100f,
                                        y = 100f,
                                        widthDp = 64f,
                                        heightDp = 64f,
                                        actionName = item
                                    )
                                    widgets = widgets + newW
                                    selectedWidgetId = newW.id
                                }
                                .padding(6.dp),
                            color = theme.backgroundColor
                        ) {
                            Text("+ $item", fontSize = 11.sp, color = theme.textColor)
                        }
                    }
                }
            }

            // Center: Canvas Preview
            Box(
                modifier = Modifier
                    .weight(1.0f)
                    .fillMaxHeight()
                    .padding(16.dp),
                contentAlignment = Alignment.Center
            ) {
                // Device Aspect Frame
                Box(
                    modifier = Modifier
                        .fillMaxWidth(0.95f)
                        .aspectRatio(selectedDevice.ratio)
                        .clip(RoundedCornerShape(12.dp))
                        .background(Color(0xFF1E1E24))
                        .border(2.dp, theme.primaryColor, RoundedCornerShape(12.dp))
                ) {
                    // Safe Area Overlay Guide
                    if (showSafeArea) {
                        Box(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(horizontal = 24.dp, vertical = 12.dp)
                                .border(1.dp, Color(0xFF00BCD4).copy(alpha = 0.5f), RoundedCornerShape(4.dp))
                        )
                    }

                    // Render Widgets on canvas
                    widgets.forEach { w ->
                        val isSelected = w.id == selectedWidgetId
                        val meets48dp = w.widthDp >= 48f && w.heightDp >= 48f

                        Box(
                            modifier = Modifier
                                .offset(x = w.x.dp, y = w.y.dp)
                                .size(w.widthDp.dp, w.heightDp.dp)
                                .clip(RoundedCornerShape(6.dp))
                                .background(if (isSelected) theme.primaryColor.copy(alpha = 0.35f) else Color(0xFF2A2A36))
                                .border(
                                    width = if (isSelected) 2.dp else 1.dp,
                                    color = if (isSelected) theme.primaryColor else if (!meets48dp) Color.Red else Color.Gray,
                                    shape = RoundedCornerShape(6.dp)
                                )
                                .clickable { selectedWidgetId = w.id }
                                .padding(4.dp),
                            contentAlignment = Alignment.Center
                        ) {
                            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                                Text(w.typeName, fontSize = 10.sp, fontWeight = FontWeight.Bold, color = Color.White)
                                if (w.actionName.isNotEmpty()) {
                                    Text(w.actionName, fontSize = 8.sp, color = Color.LightGray)
                                }
                            }
                        }
                    }
                }
            }

            // Right: Property Inspector Panel
            Surface(
                modifier = Modifier
                    .width(180.dp)
                    .fillMaxHeight(),
                color = theme.surfaceColor
            ) {
                val selectedWidget = widgets.find { it.id == selectedWidgetId }
                Column(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(10.dp)
                ) {
                    Text("Widget Inspector", fontSize = 12.sp, fontWeight = FontWeight.Bold, color = theme.textColor)
                    Spacer(modifier = Modifier.height(8.dp))

                    if (selectedWidget != null) {
                        Text("ID: ${selectedWidget.id}", fontSize = 11.sp, color = theme.textColor)
                        Text("Type: ${selectedWidget.typeName}", fontSize = 11.sp, color = theme.secondaryTextColor)
                        Spacer(modifier = Modifier.height(8.dp))

                        val meets48dp = selectedWidget.widthDp >= 48f && selectedWidget.heightDp >= 48f
                        if (!meets48dp) {
                            Text(
                                "⚠️ Fails Rule R8 minimum 48 dp touch target!",
                                fontSize = 10.sp,
                                color = Color(0xFFFF5252),
                                fontWeight = FontWeight.SemiBold
                            )
                        } else {
                            Text("✓ 48 dp touch size verified", fontSize = 10.sp, color = Color(0xFF4CAF50))
                        }

                        Spacer(modifier = Modifier.height(12.dp))
                        Text("Width: ${selectedWidget.widthDp.toInt()} dp", fontSize = 11.sp, color = theme.textColor)
                        Text("Height: ${selectedWidget.heightDp.toInt()} dp", fontSize = 11.sp, color = theme.textColor)

                        if (selectedWidget.actionName.isNotEmpty()) {
                            Text("Action: ${selectedWidget.actionName}", fontSize = 11.sp, color = theme.textColor)
                        }
                        if (selectedWidget.binding.isNotEmpty()) {
                            Text("Binding: ${selectedWidget.binding}", fontSize = 11.sp, color = theme.textColor)
                        }

                        Spacer(modifier = Modifier.height(16.dp))
                        OutlinedButton(
                            onClick = {
                                widgets = widgets.filter { it.id != selectedWidget.id }
                                selectedWidgetId = widgets.firstOrNull()?.id
                            },
                            colors = ButtonDefaults.outlinedButtonColors(contentColor = Color.Red)
                        ) {
                            Text("Delete Widget", fontSize = 11.sp)
                        }
                    } else {
                        Text("Select a widget to edit properties", fontSize = 11.sp, color = theme.secondaryTextColor)
                    }
                }
            }
        }
    }
}
