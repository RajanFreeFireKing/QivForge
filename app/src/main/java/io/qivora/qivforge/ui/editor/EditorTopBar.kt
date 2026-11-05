package io.qivora.qivforge.ui.editor

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.LocalQivTheme

enum class GizmoMode {
    SELECT,
    MOVE,
    ROTATE,
    SCALE
}

enum class TransformSpace {
    LOCAL,
    WORLD
}

@Composable
fun EditorTopBar(
    projectName: String,
    isDirty: Boolean,
    gizmoMode: GizmoMode,
    transformSpace: TransformSpace,
    canUndo: Boolean,
    canRedo: Boolean,
    isPlaying: Boolean,
    onBack: () -> Unit,
    onSave: () -> Unit,
    onUndo: () -> Unit,
    onRedo: () -> Unit,
    onGizmoChange: (GizmoMode) -> Unit,
    onSpaceToggle: () -> Unit,
    onPlayToggle: () -> Unit,
    onStop: () -> Unit,
    modifier: Modifier = Modifier
) {
    val theme = LocalQivTheme.current

    Surface(
        color = theme.panel,
        modifier = modifier
            .fillMaxWidth()
            .height(48.dp)
            .border(1.dp, theme.outline)
    ) {
        Row(
            modifier = Modifier
                .fillMaxSize()
                .padding(horizontal = 8.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            // Left Group: Back, Name + Dirty Dot, Save
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                IconButton(onClick = onBack, modifier = Modifier.size(36.dp)) {
                    Icon(Icons.Default.ArrowBack, contentDescription = "Back", tint = theme.onSurface)
                }

                Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                    Text(projectName, color = theme.onSurface, fontSize = 14.sp)
                    if (isDirty) {
                        Box(
                            modifier = Modifier
                                .size(6.dp)
                                .clip(CircleShape)
                                .background(theme.warning)
                        )
                    }
                }

                IconButton(onClick = onSave, modifier = Modifier.size(36.dp)) {
                    Icon(Icons.Default.Save, contentDescription = "Save", tint = theme.primary)
                }
            }

            // Center Group: Undo/Redo & Gizmo Buttons (Doc 04 §6.2)
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                IconButton(onClick = onUndo, enabled = canUndo, modifier = Modifier.size(32.dp)) {
                    Icon(Icons.Default.Undo, contentDescription = "Undo", tint = if (canUndo) theme.onSurface else theme.outline)
                }
                IconButton(onClick = onRedo, enabled = canRedo, modifier = Modifier.size(32.dp)) {
                    Icon(Icons.Default.Redo, contentDescription = "Redo", tint = if (canRedo) theme.onSurface else theme.outline)
                }

                Spacer(modifier = Modifier.width(6.dp))

                // Gizmo Radio Group
                Row(
                    modifier = Modifier
                        .background(theme.panelAlt, RoundedCornerShape(6.dp))
                        .padding(2.dp),
                    horizontalArrangement = Arrangement.spacedBy(2.dp)
                ) {
                    val gizmos = listOf(
                        Pair(GizmoMode.SELECT, Icons.Default.NearMe),
                        Pair(GizmoMode.MOVE, Icons.Default.OpenWith),
                        Pair(GizmoMode.ROTATE, Icons.Default.Refresh),
                        Pair(GizmoMode.SCALE, Icons.Default.AspectRatio)
                    )
                    for ((mode, icon) in gizmos) {
                        val isSelected = gizmoMode == mode
                        IconButton(
                            onClick = { onGizmoChange(mode) },
                            modifier = Modifier
                                .size(28.dp)
                                .background(
                                    if (isSelected) theme.primary else Color.Transparent,
                                    RoundedCornerShape(4.dp)
                                )
                        ) {
                            Icon(
                                icon,
                                contentDescription = mode.name,
                                tint = if (isSelected) theme.onPrimary else theme.onSurfaceMuted,
                                modifier = Modifier.size(16.dp)
                            )
                        }
                    }
                }

                // Local / World Space Toggle
                TextButton(
                    onClick = onSpaceToggle,
                    contentPadding = PaddingValues(horizontal = 6.dp, vertical = 2.dp)
                ) {
                    Text(
                        text = if (transformSpace == TransformSpace.LOCAL) "Local" else "World",
                        color = theme.accent,
                        fontSize = 11.sp
                    )
                }
            }

            // Right Group: Play / Pause / Stop
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                IconButton(
                    onClick = onPlayToggle,
                    modifier = Modifier
                        .size(32.dp)
                        .background(if (isPlaying) theme.warning else theme.success, CircleShape)
                ) {
                    Icon(
                        imageVector = if (isPlaying) Icons.Default.Pause else Icons.Default.PlayArrow,
                        contentDescription = "Play/Pause",
                        tint = Color.Black,
                        modifier = Modifier.size(18.dp)
                    )
                }

                IconButton(
                    onClick = onStop,
                    enabled = isPlaying,
                    modifier = Modifier.size(32.dp)
                ) {
                    Icon(
                        Icons.Default.Stop,
                        contentDescription = "Stop",
                        tint = if (isPlaying) theme.error else theme.outline
                    )
                }
            }
        }
    }
}
