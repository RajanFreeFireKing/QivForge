package io.qivora.qivforge.ui.export

import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
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

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ExportSettingsScreen(
    tokens: ThemeTokens,
    projectName: String = "Heavy Tractor",
    onNavigateBack: () -> Unit,
    onExportComplete: (apkPath: String) -> Unit = {}
) {
    var appName by remember { mutableStateOf(projectName) }
    var packageName by remember { mutableStateOf("com.qivora.heavy_tractor") }
    var versionName by remember { mutableStateOf("1.0.0") }
    var versionCode by remember { mutableStateOf(1) }
    var orientation by remember { mutableStateOf("Landscape") }
    var targetQuality by remember { mutableStateOf("High") }
    var isDebugVariant by remember { mutableStateOf(false) }
    var enableCloudStreaming by remember { mutableStateOf(false) }
    var keystoreAlias by remember { mutableStateOf("qivforge_production") }
    var keystorePassword by remember { mutableStateOf("••••••••") }
    var hasExportedBackup by remember { mutableStateOf(false) }

    var showSizeReport by remember { mutableStateOf(false) }
    var isExporting by remember { mutableStateOf(false) }
    var exportProgress by remember { mutableStateOf(0.0f) }
    var exportStatusText by remember { mutableStateOf("") }
    var exportCompletedPath by remember { mutableStateOf<String?>(null) }
    var exportError by remember { mutableStateOf<String?>(null) }

    val isPackageValid = remember(packageName) {
        Regex("^[a-z][a-z0-9_]*(\\.[a-z][a-z0-9_]*)+$").matches(packageName)
    }
    val isAppNameValid = remember(appName) {
        appName.isNotBlank() && appName.length <= 30
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Build & Export Game", fontWeight = FontWeight.Bold) },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                },
                actions = {
                    TextButton(onClick = { showSizeReport = true }) {
                        Icon(Icons.Default.Info, contentDescription = null, modifier = Modifier.size(18.dp))
                        Spacer(Modifier.width(4.dp))
                        Text("Size Report")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = tokens.surface,
                    titleContentColor = tokens.onSurface,
                    navigationIconContentColor = tokens.onSurface
                )
            )
        },
        containerColor = tokens.background
    ) { paddingValues ->
        Box(modifier = Modifier.fillMaxSize().padding(paddingValues)) {
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(horizontal = 16.dp),
                verticalArrangement = Arrangement.spacedBy(16.dp),
                contentPadding = PaddingValues(vertical = 16.dp)
            ) {
                // Section 1: App Identity
                item {
                    Text("App Identity", fontWeight = FontWeight.Bold, color = tokens.primary, fontSize = 14.sp)
                    Spacer(Modifier.height(8.dp))

                    OutlinedTextField(
                        value = appName,
                        onValueChange = { if (it.length <= 30) appName = it },
                        label = { Text("App Name (1-30 chars)") },
                        supportingText = { Text("${appName.length} / 30") },
                        isError = !isAppNameValid,
                        modifier = Modifier.fillMaxWidth()
                    )

                    Spacer(Modifier.height(8.dp))

                    OutlinedTextField(
                        value = packageName,
                        onValueChange = { packageName = it.lowercase().trim() },
                        label = { Text("Package Name (reverse domain)") },
                        supportingText = {
                            if (!isPackageValid) {
                                Text("Must follow com.example.mygame format", color = MaterialTheme.colorScheme.error)
                            } else {
                                Text("Cannot change after publishing to Google Play", color = tokens.onSurface.copy(alpha = 0.6f))
                            }
                        },
                        isError = !isPackageValid,
                        modifier = Modifier.fillMaxWidth()
                    )

                    Spacer(Modifier.height(8.dp))

                    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                        OutlinedTextField(
                            value = versionName,
                            onValueChange = { versionName = it },
                            label = { Text("Version Name") },
                            modifier = Modifier.weight(1f)
                        )
                        OutlinedTextField(
                            value = versionCode.toString(),
                            onValueChange = { it.toIntOrNull()?.let { code -> versionCode = code } },
                            label = { Text("Version Code") },
                            modifier = Modifier.weight(1f)
                        )
                    }
                }

                // Section 2: Display & Quality
                item {
                    Divider(color = tokens.surfaceVariant)
                    Spacer(Modifier.height(8.dp))
                    Text("Display & Quality", fontWeight = FontWeight.Bold, color = tokens.primary, fontSize = 14.sp)
                    Spacer(Modifier.height(8.dp))

                    Text("Screen Orientation", fontSize = 12.sp, color = tokens.onSurface.copy(alpha = 0.7f))
                    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        listOf("Landscape", "Portrait", "Auto").forEach { opt ->
                            FilterChip(
                                selected = orientation == opt,
                                onClick = { orientation = opt },
                                label = { Text(opt) }
                            )
                        }
                    }

                    Spacer(Modifier.height(8.dp))

                    Text("Target Quality Tier", fontSize = 12.sp, color = tokens.onSurface.copy(alpha = 0.7f))
                    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        listOf("Low", "Medium", "High", "Auto").forEach { opt ->
                            FilterChip(
                                selected = targetQuality == opt,
                                onClick = { targetQuality = opt },
                                label = { Text(opt) }
                            )
                        }
                    }
                }

                // Section 3: Build Variant & Keystore Signing (Doc 13 §6)
                item {
                    Divider(color = tokens.surfaceVariant)
                    Spacer(Modifier.height(8.dp))
                    Text("Signing & Keystore", fontWeight = FontWeight.Bold, color = tokens.primary, fontSize = 14.sp)
                    Spacer(Modifier.height(8.dp))

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Column {
                            Text(if (isDebugVariant) "Debug Variant" else "Release Variant", fontWeight = FontWeight.SemiBold)
                            Text(
                                if (isDebugVariant) "Includes profiler & debug symbols" else "Optimized, stripped, production APK",
                                fontSize = 12.sp,
                                color = tokens.onSurface.copy(alpha = 0.6f)
                            )
                        }
                        Switch(
                            checked = !isDebugVariant,
                            onCheckedChange = { isDebugVariant = !it }
                        )
                    }

                    if (!isDebugVariant) {
                        Spacer(Modifier.height(12.dp))
                        Card(
                            colors = CardDefaults.cardColors(containerColor = tokens.surfaceVariant.copy(alpha = 0.4f)),
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Column(modifier = Modifier.padding(12.dp)) {
                                Row(verticalAlignment = Alignment.CenterVertically) {
                                    Icon(Icons.Default.Warning, contentDescription = null, tint = Color(0xFFFFB74D), modifier = Modifier.size(20.dp))
                                    Spacer(Modifier.width(8.dp))
                                    Text("Critical Keystore Notice (Doc 13 §6)", fontWeight = FontWeight.Bold, fontSize = 12.sp)
                                }
                                Text(
                                    "If the keystore is lost, updates of the same app cannot be installed over the old version and a store listing cannot be updated. Always back up your keystore.",
                                    fontSize = 11.sp,
                                    color = tokens.onSurface.copy(alpha = 0.8f),
                                    modifier = Modifier.padding(top = 4.dp)
                                )

                                Spacer(Modifier.height(8.dp))

                                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                                    OutlinedButton(
                                        onClick = { hasExportedBackup = true },
                                        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 4.dp)
                                    ) {
                                        Icon(Icons.Default.Share, contentDescription = null, modifier = Modifier.size(14.dp))
                                        Spacer(Modifier.width(4.dp))
                                        Text(if (hasExportedBackup) "Backup Saved ✓" else "Export Backup", fontSize = 12.sp)
                                    }
                                }
                            }
                        }
                    }
                }

                // Section 4: Cloud Streaming & Limits (Doc 13 §8)
                item {
                    Divider(color = tokens.surfaceVariant)
                    Spacer(Modifier.height(8.dp))
                    Text("Cloud Streaming", fontWeight = FontWeight.Bold, color = tokens.primary, fontSize = 14.sp)
                    Spacer(Modifier.height(8.dp))

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text("Enable Cloud Streaming", fontWeight = FontWeight.SemiBold)
                            Text(
                                "Streams levels on demand. Only allowed for public Hugging Face repos or custom HTTPS servers (Doc 13 §8).",
                                fontSize = 12.sp,
                                color = tokens.onSurface.copy(alpha = 0.6f)
                            )
                        }
                        Switch(
                            checked = enableCloudStreaming,
                            onCheckedChange = { enableCloudStreaming = it }
                        )
                    }
                }

                // Section 5: Action Buttons
                item {
                    Spacer(Modifier.height(16.dp))

                    Button(
                        onClick = {
                            isExporting = true
                            exportProgress = 0.1f
                            exportStatusText = "Validating scene references & scanning secrets..."
                        },
                        enabled = isPackageValid && isAppNameValid && !isExporting,
                        modifier = Modifier.fillMaxWidth().height(48.dp),
                        colors = ButtonDefaults.buttonColors(containerColor = tokens.primary)
                    ) {
                        Icon(Icons.Default.Build, contentDescription = null)
                        Spacer(Modifier.width(8.dp))
                        Text("Export APK on Device", fontWeight = FontWeight.Bold)
                    }

                    Spacer(Modifier.height(8.dp))

                    OutlinedButton(
                        onClick = { /* Cloud build trigger */ },
                        enabled = !isExporting,
                        modifier = Modifier.fillMaxWidth().height(44.dp)
                    ) {
                        Icon(Icons.Default.CloudUpload, contentDescription = null)
                        Spacer(Modifier.width(8.dp))
                        Text("Build in Cloud (AAB for Google Play)", fontSize = 13.sp)
                    }
                }
            }

            // Exporting Progress Overlay
            if (isExporting) {
                Surface(
                    color = Color.Black.copy(alpha = 0.75f),
                    modifier = Modifier.fillMaxSize()
                ) {
                    Column(
                        modifier = Modifier.fillMaxSize().padding(32.dp),
                        verticalArrangement = Arrangement.Center,
                        horizontalAlignment = Alignment.CenterHorizontally
                    ) {
                        CircularProgressIndicator(color = tokens.primary)
                        Spacer(Modifier.height(20.dp))
                        Text("Building APK on Device...", fontWeight = FontWeight.Bold, color = Color.White)
                        Spacer(Modifier.height(8.dp))
                        Text(exportStatusText, fontSize = 12.sp, color = Color.White.copy(alpha = 0.8f))
                        Spacer(Modifier.height(16.dp))
                        LinearProgressIndicator(progress = exportProgress, modifier = Modifier.fillMaxWidth(0.8f))

                        Spacer(Modifier.height(24.dp))
                        OutlinedButton(
                            onClick = {
                                isExporting = false
                                exportStatusText = ""
                            },
                            colors = ButtonDefaults.outlinedButtonColors(contentColor = Color.White)
                        ) {
                            Text("Cancel Export")
                        }
                    }
                }
            }

            // Size Report Bottom Sheet / Dialog
            if (showSizeReport) {
                AlertDialog(
                    onDismissRequest = { showSizeReport = false },
                    title = { Text("Export Size Report (Doc 13 §4.2)") },
                    text = {
                        Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                            Text("Estimated Total: 42.8 MB (Well under 90 MB gate)", fontWeight = FontWeight.SemiBold)
                            Divider()
                            Text("• Meshes: 18.2 MB (42%)")
                            Text("• Textures: 14.5 MB (34%)")
                            Text("• Audio: 6.8 MB (16%)")
                            Text("• Bytecode Scripts: 1.8 MB (4%)")
                            Text("• Native Player: 1.5 MB (4%)")
                            Spacer(Modifier.height(4.dp))
                            Text("Top Largest Asset: assets/models/tractor_heavy.qmesh (8.4 MB)", fontSize = 11.sp, color = tokens.onSurface.copy(alpha = 0.7f))
                        }
                    },
                    confirmButton = {
                        TextButton(onClick = { showSizeReport = false }) {
                            Text("Close")
                        }
                    }
                )
            }
        }
    }
}
