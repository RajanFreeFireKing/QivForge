package io.qivora.qivforge.ui.editor

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.LocalQivTheme

data class ActorItem(
    val id: Int,
    val name: String,
    val type: String = "Actor",
    val isHidden: Boolean = false,
    val isLocked: Boolean = false,
    val level: Int = 0
)

@Composable
fun SceneHierarchyPanel(
    actors: List<ActorItem>,
    selectedActorId: Int?,
    onSelectActor: (Int) -> Unit,
    onAddActor: (name: String, type: String) -> Unit,
    onDeleteActor: (Int) -> Unit,
    modifier: Modifier = Modifier
) {
    val theme = LocalQivTheme.current
    var searchQuery by remember { mutableStateOf("") }
    var showAddMenu by remember { mutableStateOf(false) }

    val filtered = actors.filter {
        searchQuery.isBlank() || it.name.contains(searchQuery.trim(), ignoreCase = true)
    }

    Surface(
        color = theme.panel,
        modifier = modifier
            .fillMaxHeight()
            .width(220.dp)
            .border(1.dp, theme.outline)
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(8.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            // Header with + Add button
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text("Hierarchy", color = theme.onSurface, fontSize = 14.sp)

                Box {
                    IconButton(
                        onClick = { showAddMenu = true },
                        modifier = Modifier.size(28.dp)
                    ) {
                        Icon(Icons.Default.Add, contentDescription = "Add Actor", tint = theme.primary)
                    }

                    DropdownMenu(
                        expanded = showAddMenu,
                        onDismissRequest = { showAddMenu = false }
                    ) {
                        DropdownMenuItem(
                            text = { Text("3D Cube") },
                            onClick = {
                                onAddActor("Cube", "CUBE")
                                showAddMenu = false
                            }
                        )
                        DropdownMenuItem(
                            text = { Text("3D Plane") },
                            onClick = {
                                onAddActor("Plane", "PLANE")
                                showAddMenu = false
                            }
                        )
                        DropdownMenuItem(
                            text = { Text("Sun Directional Light") },
                            onClick = {
                                onAddActor("Sun", "LIGHT")
                                showAddMenu = false
                            }
                        )
                        DropdownMenuItem(
                            text = { Text("Camera") },
                            onClick = {
                                onAddActor("Camera", "CAMERA")
                                showAddMenu = false
                            }
                        )
                        DropdownMenuItem(
                            text = { Text("Empty Actor") },
                            onClick = {
                                onAddActor("EmptyActor", "EMPTY")
                                showAddMenu = false
                            }
                        )
                    }
                }
            }

            // Search Bar
            OutlinedTextField(
                value = searchQuery,
                onValueChange = { searchQuery = it },
                placeholder = { Text("Search...", color = theme.onSurfaceMuted, fontSize = 12.sp) },
                singleLine = true,
                modifier = Modifier
                    .fillMaxWidth()
                    .height(42.dp),
                colors = OutlinedTextFieldDefaults.colors(
                    focusedTextColor = theme.onSurface,
                    unfocusedTextColor = theme.onSurface,
                    focusedBorderColor = theme.primary,
                    unfocusedBorderColor = theme.outline
                )
            )

            // Actor List
            LazyColumn(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                items(filtered, key = { it.id }) { actor ->
                    val isSelected = actor.id == selectedActorId
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(start = (actor.level * 12).dp)
                            .clip(RoundedCornerShape(6.dp))
                            .background(if (isSelected) theme.accent.copy(alpha = 0.2f) else theme.panelAlt)
                            .border(
                                1.dp,
                                if (isSelected) theme.accent else theme.outline.copy(alpha = 0.5f),
                                RoundedCornerShape(6.dp)
                            )
                            .clickable { onSelectActor(actor.id) }
                            .padding(horizontal = 8.dp, vertical = 6.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                            modifier = Modifier.weight(1f)
                        ) {
                            Icon(
                                imageVector = when (actor.type) {
                                    "LIGHT" -> Icons.Default.WbSunny
                                    "CAMERA" -> Icons.Default.Videocam
                                    else -> Icons.Default.Widgets
                                },
                                contentDescription = null,
                                tint = if (isSelected) theme.accent else theme.onSurfaceMuted,
                                modifier = Modifier.size(16.dp)
                            )
                            Text(
                                text = actor.name,
                                color = theme.onSurface,
                                fontSize = 13.sp,
                                maxLines = 1
                            )
                        }

                        // Delete icon
                        IconButton(
                            onClick = { onDeleteActor(actor.id) },
                            modifier = Modifier.size(20.dp)
                        ) {
                            Icon(
                                Icons.Default.Close,
                                contentDescription = "Delete",
                                tint = theme.error,
                                modifier = Modifier.size(14.dp)
                            )
                        }
                    }
                }
            }
        }
    }
}
