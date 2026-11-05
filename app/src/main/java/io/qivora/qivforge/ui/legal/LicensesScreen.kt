package io.qivora.qivforge.ui.legal

import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.VerifiedUser
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

data class ThirdPartyLibrary(
    val name: String,
    val license: String,
    val copyright: String,
    val description: String,
    val notice: String
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LicensesScreen(
    tokens: ThemeTokens,
    onNavigateBack: () -> Unit,
    creatorAttributions: List<String> = listOf(
        "Heavy Tractor 3D Model — Created by Qivora Studio (CC0 Public Domain)",
        "Engine RPM Sound Pack — Recorded by Audio Lab (CC-BY-4.0 Attribution)"
    )
) {
    val libraries = remember {
        listOf(
            ThirdPartyLibrary(
                "Google Filament",
                "Apache-2.0",
                "Copyright (C) 2018-2026 Google LLC",
                "Real-time physically based rendering engine for Android.",
                "Licensed under the Apache License, Version 2.0."
            ),
            ThirdPartyLibrary(
                "Jolt Physics",
                "MIT",
                "Copyright (C) 2021-2026 Jorrit Rouwe",
                "Multi-threaded rigid body physics and vehicle simulation library.",
                "Permission is hereby granted, free of charge, to any person obtaining a copy."
            ),
            ThirdPartyLibrary(
                "Google Oboe",
                "Apache-2.0",
                "Copyright (C) 2016-2026 Google LLC",
                "High-performance low-latency audio library for Android.",
                "Licensed under the Apache License, Version 2.0."
            ),
            ThirdPartyLibrary(
                "miniaudio",
                "MIT-0 / Public Domain",
                "David Reid",
                "Audio playback, resampling, and spatialization backend.",
                "Dual-licensed under Public Domain and MIT-0."
            ),
            ThirdPartyLibrary(
                "Google Draco",
                "Apache-2.0",
                "Copyright (C) 2016-2026 Google LLC",
                "3D geometric mesh and point cloud compression library.",
                "Licensed under the Apache License, Version 2.0."
            ),
            ThirdPartyLibrary(
                "meshoptimizer",
                "MIT",
                "Copyright (C) 2016-2026 Arseny Kapoulkine",
                "Mesh vertex cache, overdraw, and compression optimization.",
                "Licensed under the MIT License."
            ),
            ThirdPartyLibrary(
                "Basis Universal",
                "Apache-2.0",
                "Copyright (C) 2019-2026 Binomial LLC",
                "GPU texture compression system (ETC2 and ASTC transcoder).",
                "Licensed under the Apache License, Version 2.0."
            ),
            ThirdPartyLibrary(
                "nlohmann/json",
                "MIT",
                "Copyright (C) 2013-2026 Niels Lohmann",
                "JSON for Modern C++ parser and serializer.",
                "Licensed under the MIT License."
            ),
            ThirdPartyLibrary(
                "llama.cpp",
                "MIT",
                "Copyright (C) 2023-2026 Georgi Gerganov",
                "Offline local GGUF large language model inference runtime.",
                "Licensed under the MIT License."
            ),
            ThirdPartyLibrary(
                "Google Tink",
                "Apache-2.0",
                "Copyright (C) 2017-2026 Google LLC",
                "Multi-platform cryptographic library for Android Keystore isolation.",
                "Licensed under the Apache License, Version 2.0."
            ),
            ThirdPartyLibrary(
                "apksig",
                "Apache-2.0",
                "Copyright (C) 2016-2026 The Android Open Source Project",
                "APK Signature Scheme v2 and v3 signing and verification.",
                "Licensed under the Apache License, Version 2.0."
            ),
            ThirdPartyLibrary(
                "JetBrains Mono",
                "SIL OFL 1.1",
                "Copyright (C) 2020-2026 JetBrains s.r.o.",
                "Code editor typeface designed for developers.",
                "Licensed under the SIL Open Font License, Version 1.1."
            ),
            ThirdPartyLibrary(
                "Jetpack Compose & Material 3",
                "Apache-2.0",
                "Copyright (C) 2018-2026 The Android Open Source Project",
                "Android native declarative UI toolkit.",
                "Licensed under the Apache License, Version 2.0."
            ),
            ThirdPartyLibrary(
                "Kotlin Standard Library",
                "Apache-2.0",
                "Copyright (C) 2010-2026 JetBrains s.r.o.",
                "Kotlin programming language and coroutines runtime.",
                "Licensed under the Apache License, Version 2.0."
            )
        )
    }

    var selectedLib by remember { mutableStateOf<ThirdPartyLibrary?>(null) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Licenses & Attributions", fontWeight = FontWeight.Bold) },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back")
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
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues)
                .padding(horizontal = 16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
            contentPadding = PaddingValues(vertical = 16.dp)
        ) {
            // Section 1: Creator Asset Attributions (Doc 14 §9.2)
            if (creatorAttributions.isNotEmpty()) {
                item {
                    Text(
                        "Project Asset Attributions (Doc 14 §9.2)",
                        fontWeight = FontWeight.Bold,
                        color = tokens.primary,
                        fontSize = 13.sp
                    )
                    Spacer(Modifier.height(6.dp))
                    Card(
                        colors = CardDefaults.cardColors(containerColor = tokens.surfaceVariant.copy(alpha = 0.5f)),
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Column(modifier = Modifier.padding(12.dp)) {
                            creatorAttributions.forEach { attr ->
                                Text("• $attr", fontSize = 12.sp, color = tokens.onSurface.copy(alpha = 0.85f))
                                Spacer(Modifier.height(4.dp))
                            }
                        }
                    }
                    Spacer(Modifier.height(12.dp))
                    Divider(color = tokens.surfaceVariant)
                }
            }

            // Section 2: Engine Third-Party Software (Doc 14 §9.1)
            item {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Default.VerifiedUser, contentDescription = null, tint = tokens.primary, modifier = Modifier.size(18.dp))
                    Spacer(Modifier.width(6.dp))
                    Text(
                        "Third-Party Open-Source Software (${libraries.size})",
                        fontWeight = FontWeight.Bold,
                        color = tokens.primary,
                        fontSize = 13.sp
                    )
                }
                Text(
                    "All third-party libraries use permissive open-source licenses. No GPL or AGPL code is linked into QivForge or QivPlayer.",
                    fontSize = 11.sp,
                    color = tokens.onSurface.copy(alpha = 0.6f),
                    modifier = Modifier.padding(top = 2.dp)
                )
            }

            items(libraries) { lib ->
                Card(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clickable { selectedLib = lib },
                    colors = CardDefaults.cardColors(containerColor = tokens.surface)
                ) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(14.dp),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text(lib.name, fontWeight = FontWeight.Bold, fontSize = 14.sp)
                            Text(lib.copyright, fontSize = 11.sp, color = tokens.onSurface.copy(alpha = 0.6f))
                            Text(lib.description, fontSize = 12.sp, color = tokens.onSurface.copy(alpha = 0.8f), modifier = Modifier.padding(top = 2.dp))
                        }
                        Surface(
                            shape = RoundedCornerShape(6.dp),
                            color = tokens.primary.copy(alpha = 0.12f),
                            modifier = Modifier.padding(start = 8.dp)
                        ) {
                            Text(
                                lib.license,
                                fontSize = 11.sp,
                                fontWeight = FontWeight.SemiBold,
                                color = tokens.primary,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp)
                            )
                        }
                    }
                }
            }
        }

        // License Text Dialog
        selectedLib?.let { lib ->
            AlertDialog(
                onDismissRequest = { selectedLib = null },
                title = { Text(lib.name) },
                text = {
                    Column(modifier = Modifier.fillMaxWidth()) {
                        Text(lib.copyright, fontWeight = FontWeight.SemiBold, fontSize = 12.sp)
                        Text("License: ${lib.license}", fontSize = 12.sp, color = tokens.primary)
                        Spacer(Modifier.height(8.dp))
                        Surface(
                            color = tokens.background,
                            shape = RoundedCornerShape(4.dp),
                            modifier = Modifier.fillMaxWidth().heightIn(max = 200.dp)
                        ) {
                            Text(
                                lib.notice,
                                fontFamily = FontFamily.Monospace,
                                fontSize = 11.sp,
                                modifier = Modifier.padding(8.dp)
                            )
                        }
                    }
                },
                confirmButton = {
                    TextButton(onClick = { selectedLib = null }) {
                        Text("Close")
                    }
                }
            )
        }
    }
}
