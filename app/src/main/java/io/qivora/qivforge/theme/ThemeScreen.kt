package io.qivora.qivforge.theme

import android.widget.Toast
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Check
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeTokens.Companion.toHex

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ThemeScreen(onBack: () -> Unit) {
    val theme = LocalQivTheme.current
    val context = LocalContext.current

    var selectedTokenForEdit by remember { mutableStateOf<Pair<String, Color>?>(null) }
    var contrastAgainstColor by remember { mutableStateOf<Color?>(null) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Theme Customizer", color = theme.onSurface) },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back", tint = theme.onSurface)
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = theme.surface)
            )
        },
        containerColor = theme.surface
    ) { padding ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // Live Preview Card
            item {
                Text("Live Preview", color = theme.onSurface, fontSize = 16.sp)
                Spacer(modifier = Modifier.height(8.dp))
                Card(
                    colors = CardDefaults.cardColors(containerColor = theme.panel),
                    modifier = Modifier
                        .fillMaxWidth()
                        .border(1.dp, theme.outline, RoundedCornerShape(12.dp))
                ) {
                    Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        Text("Theme: ${theme.name}", color = theme.onSurface, fontSize = 18.sp)
                        Text("Sample secondary text", color = theme.onSurfaceMuted, fontSize = 14.sp)
                        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                            Button(
                                onClick = {},
                                colors = ButtonDefaults.buttonColors(containerColor = theme.primary)
                            ) {
                                Text("Primary", color = theme.onPrimary)
                            }
                            Button(
                                onClick = {},
                                colors = ButtonDefaults.buttonColors(containerColor = theme.secondary)
                            ) {
                                Text("Secondary", color = theme.onSurface)
                            }
                        }
                    }
                }
            }

            // Presets
            item {
                Text("Presets", color = theme.onSurface, fontSize = 16.sp)
                Spacer(modifier = Modifier.height(8.dp))
                LazyRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    items(ThemePresets.AllPresets) { preset ->
                        val isSelected = preset.name == theme.name
                        FilterChip(
                            selected = isSelected,
                            onClick = { ThemeManager.applyPreset(preset.name, context) },
                            label = { Text(preset.name, color = if (isSelected) preset.onPrimary else theme.onSurface) },
                            leadingIcon = if (isSelected) {
                                { Icon(Icons.Default.Check, contentDescription = null, tint = preset.onPrimary, modifier = Modifier.size(16.dp)) }
                            } else null,
                            colors = FilterChipDefaults.filterChipColors(
                                selectedContainerColor = preset.primary,
                                containerColor = theme.panel
                            )
                        )
                    }
                }
            }

            // Editable Token Rows
            item {
                Text("Color Tokens", color = theme.onSurface, fontSize = 16.sp)
            }

            val tokensList = listOf(
                Triple("primary", theme.primary, theme.onPrimary),
                Triple("onPrimary", theme.onPrimary, theme.primary),
                Triple("surface", theme.surface, theme.onSurface),
                Triple("panel", theme.panel, theme.onSurface),
                Triple("panelAlt", theme.panelAlt, theme.onSurface),
                Triple("onSurface", theme.onSurface, theme.surface),
                Triple("onSurfaceMuted", theme.onSurfaceMuted, theme.surface),
                Triple("accent", theme.accent, null),
                Triple("outline", theme.outline, null),
                Triple("success", theme.success, null),
                Triple("warning", theme.warning, null),
                Triple("error", theme.error, null)
            )

            items(tokensList) { (tokenName, color, contrastTarget) ->
                TokenRow(
                    name = tokenName,
                    color = color,
                    theme = theme,
                    onClick = {
                        selectedTokenForEdit = Pair(tokenName, color)
                        contrastAgainstColor = contrastTarget
                    }
                )
            }
        }
    }

    selectedTokenForEdit?.let { (tokenName, initialColor) ->
        ColorPickerModal(
            tokenName = tokenName,
            initialColor = initialColor,
            contrastAgainst = contrastAgainstColor,
            onColorSelected = { updatedColor ->
                val newTheme = when (tokenName) {
                    "primary" -> theme.copy(primary = updatedColor)
                    "onPrimary" -> theme.copy(onPrimary = updatedColor)
                    "surface" -> theme.copy(surface = updatedColor)
                    "panel" -> theme.copy(panel = updatedColor)
                    "panelAlt" -> theme.copy(panelAlt = updatedColor)
                    "onSurface" -> theme.copy(onSurface = updatedColor)
                    "onSurfaceMuted" -> theme.copy(onSurfaceMuted = updatedColor)
                    "accent" -> theme.copy(accent = updatedColor)
                    "outline" -> theme.copy(outline = updatedColor)
                    "success" -> theme.copy(success = updatedColor)
                    "warning" -> theme.copy(warning = updatedColor)
                    "error" -> theme.copy(error = updatedColor)
                    else -> theme
                }
                ThemeManager.setTheme(newTheme, context)
            },
            onDismiss = { selectedTokenForEdit = null }
        )
    }
}

@Composable
private fun TokenRow(
    name: String,
    color: Color,
    theme: ThemeTokens,
    onClick: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(8.dp))
            .background(theme.panel)
            .clickable(onClick = onClick)
            .padding(horizontal = 16.dp, vertical = 12.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column {
            Text(name, color = theme.onSurface, fontSize = 14.sp)
            Text(color.toHex(), color = theme.onSurfaceMuted, fontSize = 12.sp)
        }
        Box(
            modifier = Modifier
                .size(32.dp)
                .clip(CircleShape)
                .background(color)
                .border(1.dp, theme.outline, CircleShape)
        )
    }
}
