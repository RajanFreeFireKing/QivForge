package io.qivora.qivforge.ui.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.LocalQivTheme

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    onBack: () -> Unit,
    onNavigateTheme: () -> Unit
) {
    val theme = LocalQivTheme.current

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Settings", color = theme.onSurface) },
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
            // Appearance Section
            item {
                Text("Appearance & Customization", color = theme.onSurfaceMuted, fontSize = 13.sp)
                Spacer(modifier = Modifier.height(8.dp))
                SettingsRow(
                    icon = Icons.Default.Palette,
                    title = "Dynamic Theme",
                    subtitle = "Current preset: ${theme.name}",
                    onClick = onNavigateTheme
                )
            }

            // Cloud & Sync Section (Doc 04 §11)
            item {
                Text("Cloud & Accounts", color = theme.onSurfaceMuted, fontSize = 13.sp)
                Spacer(modifier = Modifier.height(8.dp))
                SettingsRow(
                    icon = Icons.Default.Cloud,
                    title = "Hugging Face Hub",
                    subtitle = "Git LFS & commit integration (Unconnected - Offline mode)",
                    onClick = {}
                )
            }

            // Storage Section (Doc 04 §11)
            item {
                Text("Storage & Projects", color = theme.onSurfaceMuted, fontSize = 13.sp)
                Spacer(modifier = Modifier.height(8.dp))
                SettingsRow(
                    icon = Icons.Default.Folder,
                    title = "Local App Storage",
                    subtitle = "Scoped Storage & SAF compliant",
                    onClick = {}
                )
            }

            // About & Governance Section (Doc 04 §11 / Doc 14)
            item {
                Text("About & Governance", color = theme.onSurfaceMuted, fontSize = 13.sp)
                Spacer(modifier = Modifier.height(8.dp))
                SettingsRow(
                    icon = Icons.Default.Info,
                    title = "QivForge Version",
                    subtitle = "0.1.0-alpha (M1 milestone build)",
                    onClick = {}
                )
                Spacer(modifier = Modifier.height(8.dp))
                SettingsRow(
                    icon = Icons.Default.Description,
                    title = "Third-Party Open-Source Licenses",
                    subtitle = "Filament (Apache-2.0), Jolt (MIT), Oboe (Apache-2.0)",
                    onClick = {}
                )
            }
        }
    }
}

@Composable
private fun SettingsRow(
    icon: ImageVector,
    title: String,
    subtitle: String,
    onClick: () -> Unit
) {
    val theme = LocalQivTheme.current

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(10.dp))
            .background(theme.panel)
            .border(1.dp, theme.outline, RoundedCornerShape(10.dp))
            .clickable(onClick = onClick)
            .padding(16.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(14.dp)
    ) {
        Icon(icon, contentDescription = null, tint = theme.accent, modifier = Modifier.size(24.dp))
        Column(modifier = Modifier.weight(1f)) {
            Text(title, color = theme.onSurface, fontSize = 15.sp)
            Text(subtitle, color = theme.onSurfaceMuted, fontSize = 12.sp)
        }
        Icon(Icons.Default.ChevronRight, contentDescription = null, tint = theme.onSurfaceMuted)
    }
}
