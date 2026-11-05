package io.qivora.qivforge.ui.hub

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Check
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import io.qivora.qivforge.project.ProjectOrientation
import io.qivora.qivforge.project.StorageMode
import io.qivora.qivforge.project.TemplateCatalog
import io.qivora.qivforge.project.TemplateInfo
import io.qivora.qivforge.theme.LocalQivTheme

@Composable
fun NewProjectModal(
    existingNames: List<String>,
    onCreate: (name: String, templateId: String, storageMode: StorageMode, orientation: ProjectOrientation) -> Unit,
    onDismiss: () -> Unit
) {
    val theme = LocalQivTheme.current

    var projectName by remember { mutableStateOf("") }
    var selectedTemplate by remember { mutableStateOf<TemplateInfo>(TemplateCatalog.Blank) }
    var selectedStorageMode by remember { mutableStateOf(StorageMode.LOCAL) }
    var selectedOrientation by remember { mutableStateOf(ProjectOrientation.LANDSCAPE) }

    val nameTrimmed = projectName.trim()
    val isNameValid = nameTrimmed.length in 3..40
    val isNameUnique = existingNames.none { it.equals(nameTrimmed, ignoreCase = true) }
    val canCreate = isNameValid && isNameUnique

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(usePlatformDefaultWidth = false)
    ) {
        Surface(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp),
            shape = RoundedCornerShape(16.dp),
            color = theme.surface,
            border = BorderStroke(1.dp, theme.outline)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(20.dp)
            ) {
                // Header
                Text(
                    text = "New Project",
                    color = theme.onSurface,
                    fontSize = 20.sp
                )
                Spacer(modifier = Modifier.height(16.dp))

                // Scrollable Form
                LazyColumn(
                    modifier = Modifier.weight(1f),
                    verticalArrangement = Arrangement.spacedBy(16.dp)
                ) {
                    // Name Field
                    item {
                        OutlinedTextField(
                            value = projectName,
                            onValueChange = { projectName = it },
                            label = { Text("Project Name (3-40 characters)", color = theme.onSurfaceMuted) },
                            singleLine = true,
                            isError = projectName.isNotEmpty() && (!isNameValid || !isNameUnique),
                            supportingText = {
                                if (projectName.isNotEmpty() && !isNameUnique) {
                                    Text("A project with this name already exists", color = theme.error)
                                } else if (projectName.isNotEmpty() && !isNameValid) {
                                    Text("Must be 3 to 40 characters", color = theme.error)
                                }
                            },
                            colors = OutlinedTextFieldDefaults.colors(
                                focusedTextColor = theme.onSurface,
                                unfocusedTextColor = theme.onSurface,
                                focusedBorderColor = theme.primary,
                                unfocusedBorderColor = theme.outline
                            ),
                            modifier = Modifier.fillMaxWidth()
                        )
                    }

                    // Template Chooser (Doc 01 §7)
                    item {
                        Text("Select Starter Template", color = theme.onSurface, fontSize = 16.sp)
                    }

                    items(TemplateCatalog.All) { template ->
                        val isSelected = template.id == selectedTemplate.id
                        Card(
                            colors = CardDefaults.cardColors(
                                containerColor = if (isSelected) theme.panelAlt else theme.panel
                            ),
                            shape = RoundedCornerShape(10.dp),
                            modifier = Modifier
                                .fillMaxWidth()
                                .border(
                                    1.dp,
                                    if (isSelected) theme.primary else theme.outline,
                                    RoundedCornerShape(10.dp)
                                )
                                .clickable { selectedTemplate = template }
                        ) {
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(12.dp),
                                verticalAlignment = Alignment.CenterVertically,
                                horizontalArrangement = Arrangement.SpaceBetween
                            ) {
                                Column(modifier = Modifier.weight(1f)) {
                                    Row(
                                        verticalAlignment = Alignment.CenterVertically,
                                        horizontalArrangement = Arrangement.spacedBy(6.dp)
                                    ) {
                                        Text(template.title, color = theme.onSurface, fontSize = 15.sp)
                                        AssistChip(
                                            onClick = {},
                                            label = { Text(template.tag, fontSize = 10.sp, color = theme.accent) },
                                            modifier = Modifier.height(20.dp)
                                        )
                                    }
                                    Spacer(modifier = Modifier.height(2.dp))
                                    Text(template.description, color = theme.onSurfaceMuted, fontSize = 12.sp)
                                }
                                if (isSelected) {
                                    Icon(Icons.Default.Check, contentDescription = null, tint = theme.primary)
                                }
                            }
                        }
                    }

                    // Storage Mode Toggle (Doc 04 §5)
                    item {
                        Text("Storage Mode", color = theme.onSurface, fontSize = 16.sp)
                        Spacer(modifier = Modifier.height(6.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                            FilterChip(
                                selected = selectedStorageMode == StorageMode.LOCAL,
                                onClick = { selectedStorageMode = StorageMode.LOCAL },
                                label = { Text("Local Offline (No Account)", color = theme.onSurface) },
                                colors = FilterChipDefaults.filterChipColors(
                                    selectedContainerColor = theme.primary.copy(alpha = 0.2f),
                                    containerColor = theme.panel
                                )
                            )
                            FilterChip(
                                selected = selectedStorageMode == StorageMode.CLOUD,
                                onClick = { selectedStorageMode = StorageMode.CLOUD },
                                label = { Text("Hugging Face Cloud", color = theme.onSurface) },
                                colors = FilterChipDefaults.filterChipColors(
                                    selectedContainerColor = theme.primary.copy(alpha = 0.2f),
                                    containerColor = theme.panel
                                )
                            )
                        }
                    }
                }

                Spacer(modifier = Modifier.height(16.dp))

                // Action Buttons
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.End,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    TextButton(onClick = onDismiss) {
                        Text("Cancel", color = theme.onSurfaceMuted)
                    }
                    Spacer(modifier = Modifier.width(8.dp))
                    Button(
                        onClick = {
                            if (canCreate) {
                                onCreate(nameTrimmed, selectedTemplate.id, selectedStorageMode, selectedOrientation)
                            }
                        },
                        enabled = canCreate,
                        colors = ButtonDefaults.buttonColors(containerColor = theme.primary)
                    ) {
                        Text("Create Project", color = theme.onPrimary)
                    }
                }
            }
        }
    }
}
