package io.qivora.qivforge.ui.patch

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeManager
import kotlin.math.roundToInt

data class UiPatchNode(
    val id: String,
    val title: String,
    val category: String, // event, combat, physics, anim, math
    var x: Float,
    var y: Float
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun PatchEditorScreen(
    patchName: String = "FireWeapon.qpatch",
    onNavigateBack: () -> Unit
) {
    val theme = ThemeManager.currentTheme

    val nodes = remember {
        mutableStateListOf(
            UiPatchNode("n1", "OnTouchButtonDown", "event", 40f, 100f),
            UiPatchNode("n2", "RaycastLine", "physics", 260f, 100f),
            UiPatchNode("n3", "ApplyDamage", "combat", 480f, 100f)
        )
    }

    var compileStatus by remember { mutableStateOf("Valid (3 nodes compiled)") }
    var isCompiledOk by remember { mutableStateOf(true) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Text(patchName, fontWeight = FontWeight.Bold, fontSize = 16.sp)
                        Surface(
                            shape = RoundedCornerShape(12.dp),
                            color = if (isCompiledOk) Color(0xFF1E3A24) else Color(0xFF3A1E1E)
                        ) {
                            Row(
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp),
                                verticalAlignment = Alignment.CenterVertically,
                                horizontalArrangement = Arrangement.spacedBy(4.dp)
                            ) {
                                Icon(
                                    Icons.Default.Check,
                                    contentDescription = null,
                                    tint = if (isCompiledOk) Color(0xFF4CAF50) else Color(0xFFFF5252),
                                    modifier = Modifier.size(14.dp)
                                )
                                Text(
                                    compileStatus,
                                    fontSize = 11.sp,
                                    color = if (isCompiledOk) Color(0xFF4CAF50) else Color(0xFFFF5252)
                                )
                            }
                        }
                    }
                },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                },
                actions = {
                    IconButton(onClick = { /* Add node modal */ }) {
                        Icon(Icons.Default.Add, contentDescription = "Add Node")
                    }
                    IconButton(onClick = { /* Test execute in mini scene */ }) {
                        Icon(Icons.Default.PlayArrow, contentDescription = "Test Run")
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
        containerColor = Color(0xFF141619) // Infinite dark canvas
    ) { padding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
        ) {
            // Render Nodes on canvas
            for (node in nodes) {
                var offsetX by remember { mutableStateOf(node.x) }
                var offsetY by remember { mutableStateOf(node.y) }

                Card(
                    shape = RoundedCornerShape(8.dp),
                    colors = CardDefaults.cardColors(containerColor = Color(0xFF22262B)),
                    modifier = Modifier
                        .offset { IntOffset(offsetX.roundToInt(), offsetY.roundToInt()) }
                        .width(180.dp)
                        .border(1.dp, theme.borderColor, RoundedCornerShape(8.dp))
                        .pointerInput(node.id) {
                            detectDragGestures { change, dragAmount ->
                                change.consume()
                                offsetX += dragAmount.x
                                offsetY += dragAmount.y
                                node.x = offsetX
                                node.y = offsetY
                            }
                        }
                ) {
                    Column {
                        // Category Header (Doc 09 §4 colors: red event, blue physics/combat, purple anim)
                        val headerColor = when (node.category) {
                            "event" -> Color(0xFFD32F2F)
                            "physics" -> Color(0xFF1976D2)
                            "combat" -> Color(0xFF0288D1)
                            "anim" -> Color(0xFF7B1FA2)
                            else -> Color(0xFF455A64)
                        }

                        Box(
                            modifier = Modifier
                                .fillMaxWidth()
                                .background(headerColor)
                                .padding(horizontal = 10.dp, vertical = 6.dp)
                        ) {
                            Text(
                                text = node.title,
                                fontWeight = FontWeight.Bold,
                                color = Color.White,
                                fontSize = 12.sp
                            )
                        }

                        // Pins representation
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(horizontal = 8.dp, vertical = 10.dp),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Box(
                                    modifier = Modifier
                                        .size(10.dp)
                                        .background(Color.White, CircleShape) // Exec pin white per Doc 09 §2
                                )
                                Spacer(modifier = Modifier.width(6.dp))
                                Text("In", color = Color.White, fontSize = 10.sp)
                            }
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Text("Out", color = Color.White, fontSize = 10.sp)
                                Spacer(modifier = Modifier.width(6.dp))
                                Box(
                                    modifier = Modifier
                                        .size(10.dp)
                                        .background(Color.White, CircleShape)
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}
