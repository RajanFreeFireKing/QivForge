package io.qivora.qivforge.ui.hub

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Cloud
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.PhoneAndroid
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.project.ProjectMetadata
import io.qivora.qivforge.project.StorageMode
import io.qivora.qivforge.theme.LocalQivTheme
import java.text.SimpleDateFormat
import java.util.*

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun ProjectCard(
    project: ProjectMetadata,
    onClick: () -> Unit,
    onLongClick: () -> Unit
) {
    val theme = LocalQivTheme.current
    val dateStr = SimpleDateFormat("MMM dd, yyyy HH:mm", Locale.getDefault()).format(Date(project.lastModifiedAt))
    val sizeKb = (project.sizeBytes / 1024).coerceAtLeast(1)

    Card(
        colors = CardDefaults.cardColors(containerColor = theme.panel),
        shape = RoundedCornerShape(12.dp),
        modifier = Modifier
            .fillMaxWidth()
            .border(1.dp, theme.outline, RoundedCornerShape(12.dp))
            .clip(RoundedCornerShape(12.dp))
            .combinedClickable(
                onClick = onClick,
                onLongClick = onLongClick
            )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(12.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            // Thumbnail / Icon box
            Box(
                modifier = Modifier
                    .size(64.dp)
                    .clip(RoundedCornerShape(8.dp))
                    .background(theme.panelAlt),
                contentAlignment = Alignment.Center
            ) {
                Icon(
                    imageVector = Icons.Default.Folder,
                    contentDescription = null,
                    tint = theme.accent,
                    modifier = Modifier.size(32.dp)
                )
            }

            // Info Column
            Column(
                modifier = Modifier.weight(1f),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    Text(
                        text = project.name,
                        color = theme.onSurface,
                        fontSize = 16.sp,
                        maxLines = 1
                    )

                    // Mode Badge (Doc 04 §5)
                    AssistChip(
                        onClick = {},
                        label = {
                            Text(
                                text = if (project.storageMode == StorageMode.LOCAL) "Local" else "Cloud",
                                fontSize = 10.sp,
                                color = theme.onSurface
                            )
                        },
                        leadingIcon = {
                            Icon(
                                imageVector = if (project.storageMode == StorageMode.LOCAL) Icons.Default.PhoneAndroid else Icons.Default.Cloud,
                                contentDescription = null,
                                tint = theme.primary,
                                modifier = Modifier.size(12.dp)
                            )
                        },
                        modifier = Modifier.height(22.dp),
                        colors = AssistChipDefaults.assistChipColors(containerColor = theme.panelAlt)
                    )
                }

                Text(
                    text = "Template: ${project.templateId}",
                    color = theme.onSurfaceMuted,
                    fontSize = 12.sp
                )

                Text(
                    text = "$dateStr · ${sizeKb} KB",
                    color = theme.onSurfaceMuted,
                    fontSize = 11.sp
                )
            }
        }
    }
}
