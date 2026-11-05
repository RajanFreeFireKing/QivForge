package io.qivora.qivforge.ui.anim

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeManager

data class UiAnimState(
    val id: String,
    val name: String,
    val type: String // Clip or BlendTree
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AnimGraphEditorScreen(
    graphName: String = "HumanoidDefault.qanim",
    onNavigateBack: () -> Unit
) {
    val theme = ThemeManager.currentTheme

    val states = remember {
        listOf(
            UiAnimState("s1", "Idle", "Clip"),
            UiAnimState("s2", "Locomotion", "BlendTree 1D"),
            UiAnimState("s3", "Jump", "Clip"),
            UiAnimState("s4", "Aim & Shoot", "Layered Override"),
            UiAnimState("s5", "Death", "Ragdoll Hand-off")
        )
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(graphName, fontWeight = FontWeight.Bold, fontSize = 16.sp) },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                },
                actions = {
                    IconButton(onClick = { /* Add State */ }) {
                        Icon(Icons.Default.Add, contentDescription = "Add State")
                    }
                    IconButton(onClick = { /* Preview Live Play */ }) {
                        Icon(Icons.Default.PlayArrow, contentDescription = "Preview Play")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = theme.surfaceColor,
                    titleContentColor = theme.onSurfaceColor,
                    navigationIconContentColor = theme.onSurfaceColor,
                    actionIconContentColor = theme.onSurfaceColor
                )
            )
        },
        containerColor = Color(0xFF131518)
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Text(
                "AnimGraph State Machine & Layers (Doc 07)",
                fontWeight = FontWeight.Bold,
                color = theme.onBackgroundColor,
                fontSize = 14.sp
            )

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                states.take(3).forEach { state ->
                    Card(
                        shape = RoundedCornerShape(8.dp),
                        colors = CardDefaults.cardColors(containerColor = Color(0xFF22252A)),
                        modifier = Modifier
                            .weight(1f)
                            .border(1.dp, Color(0xFF7B1FA2), RoundedCornerShape(8.dp))
                    ) {
                        Column(modifier = Modifier.padding(12.dp)) {
                            Text(
                                text = state.name,
                                fontWeight = FontWeight.Bold,
                                color = Color.White,
                                fontSize = 13.sp
                            )
                            Text(
                                text = state.type,
                                color = Color(0xFFCE93D8),
                                fontSize = 11.sp
                            )
                        }
                    }
                }
            }

            Card(
                shape = RoundedCornerShape(8.dp),
                colors = CardDefaults.cardColors(containerColor = theme.surfaceColor),
                modifier = Modifier.fillMaxWidth()
            ) {
                Column(modifier = Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                    Text("Transitions & Crossfade Rules", fontWeight = FontWeight.SemiBold, fontSize = 13.sp, color = theme.onSurfaceColor)
                    Text("• Idle → Locomotion: MoveSpeed > 0.1 (Crossfade 0.15s smoothstep)", fontSize = 12.sp, color = theme.onSurfaceColor.copy(alpha = 0.8f))
                    Text("• Locomotion → Idle: MoveSpeed < 0.05 (Crossfade 0.15s)", fontSize = 12.sp, color = theme.onSurfaceColor.copy(alpha = 0.8f))
                    Text("• Any → Death: trigger 'Die' (Instant ragdoll hand-off)", fontSize = 12.sp, color = theme.onSurfaceColor.copy(alpha = 0.8f))
                }
            }
        }
    }
}
