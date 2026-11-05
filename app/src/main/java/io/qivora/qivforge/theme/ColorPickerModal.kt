package io.qivora.qivforge.theme

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeTokens.Companion.toHex

@Composable
fun ColorPickerModal(
    tokenName: String,
    initialColor: Color,
    contrastAgainst: Color? = null,
    onColorSelected: (Color) -> Unit,
    onDismiss: () -> Unit
) {
    var red by remember { mutableStateOf(initialColor.red) }
    var green by remember { mutableStateOf(initialColor.green) }
    var blue by remember { mutableStateOf(initialColor.blue) }
    var hexText by remember { mutableStateOf(initialColor.toHex().removePrefix("#FF")) }

    val currentColor = Color(red = red, green = green, blue = blue, alpha = 1.0f)
    val theme = LocalQivTheme.current

    val contrastRatio = contrastAgainst?.let { ContrastGuard.getContrastRatio(currentColor, it) }
    val isAccessible = contrastRatio == null || contrastRatio >= 4.5f

    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = theme.panel,
        title = {
            Text(
                text = "Edit Color: $tokenName",
                color = theme.onSurface,
                fontSize = 18.sp
            )
        },
        text = {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(8.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                // Color Preview Box
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(56.dp)
                        .background(currentColor, RoundedCornerShape(8.dp))
                        .border(1.dp, theme.outline, RoundedCornerShape(8.dp)),
                    contentAlignment = Alignment.Center
                ) {
                    if (contrastAgainst != null) {
                        Text(
                            text = "Sample Text ($contrastRatio:1)",
                            color = contrastAgainst,
                            fontSize = 14.sp
                        )
                    }
                }

                // Contrast Guard Warning Chip
                if (contrastAgainst != null && !isAccessible) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .background(theme.warning.copy(alpha = 0.2f), RoundedCornerShape(6.dp))
                            .padding(8.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text(
                            text = "Low Contrast: %.1f:1 (< 4.5)".format(contrastRatio),
                            color = theme.warning,
                            fontSize = 12.sp
                        )
                        Button(
                            onClick = {
                                val fixed = ContrastGuard.fixAutomatically(contrastAgainst)
                                red = fixed.red
                                green = fixed.green
                                blue = fixed.blue
                                hexText = fixed.toHex().removePrefix("#FF")
                            },
                            colors = ButtonDefaults.buttonColors(containerColor = theme.warning),
                            contentPadding = PaddingValues(horizontal = 8.dp, vertical = 2.dp)
                        ) {
                            Text("Fix Auto", color = Color.Black, fontSize = 11.sp)
                        }
                    }
                }

                // Hex input
                OutlinedTextField(
                    value = hexText,
                    onValueChange = { input ->
                        hexText = input
                        if (input.length == 6) {
                            val parsed = ThemeTokens.parseHexColor(input, currentColor)
                            red = parsed.red
                            green = parsed.green
                            blue = parsed.blue
                        }
                    },
                    label = { Text("Hex Code (#)", color = theme.onSurfaceMuted) },
                    singleLine = true,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedTextColor = theme.onSurface,
                        unfocusedTextColor = theme.onSurface,
                        focusedBorderColor = theme.primary,
                        unfocusedBorderColor = theme.outline
                    ),
                    modifier = Modifier.fillMaxWidth()
                )

                // R, G, B Sliders
                Text("Red: ${(red * 255).toInt()}", color = theme.onSurfaceMuted, fontSize = 12.sp)
                Slider(
                    value = red,
                    onValueChange = { red = it; hexText = currentColor.toHex().removePrefix("#FF") },
                    colors = SliderDefaults.colors(thumbColor = theme.primary, activeTrackColor = theme.primary)
                )

                Text("Green: ${(green * 255).toInt()}", color = theme.onSurfaceMuted, fontSize = 12.sp)
                Slider(
                    value = green,
                    onValueChange = { green = it; hexText = currentColor.toHex().removePrefix("#FF") },
                    colors = SliderDefaults.colors(thumbColor = theme.primary, activeTrackColor = theme.primary)
                )

                Text("Blue: ${(blue * 255).toInt()}", color = theme.onSurfaceMuted, fontSize = 12.sp)
                Slider(
                    value = blue,
                    onValueChange = { blue = it; hexText = currentColor.toHex().removePrefix("#FF") },
                    colors = SliderDefaults.colors(thumbColor = theme.primary, activeTrackColor = theme.primary)
                )
            }
        },
        confirmButton = {
            Button(
                onClick = {
                    onColorSelected(currentColor)
                    onDismiss()
                },
                colors = ButtonDefaults.buttonColors(containerColor = theme.primary)
            ) {
                Text("Apply", color = theme.onPrimary)
            }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) {
                Text("Cancel", color = theme.onSurfaceMuted)
            }
        }
    )
}
