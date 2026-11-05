package io.qivora.qivforge.ui.hub

import android.widget.Toast
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Palette
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.project.ProjectManager
import io.qivora.qivforge.project.ProjectMetadata
import io.qivora.qivforge.project.StorageMode
import io.qivora.qivforge.theme.LocalQivTheme

enum class HubFilter {
    ALL,
    LOCAL,
    CLOUD
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ProjectHubScreen(
    onOpenProject: (ProjectMetadata) -> Unit,
    onOpenTheme: () -> Unit,
    onOpenSettings: () -> Unit
) {
    val theme = LocalQivTheme.current
    val context = LocalContext.current

    var projects by remember { mutableStateOf(ProjectManager.listProjects(context)) }
    var selectedFilter by remember { mutableStateOf(HubFilter.ALL) }
    var searchQuery by remember { mutableStateOf("") }
    var showNewProjectModal by remember { mutableStateOf(false) }

    var projectForActions by remember { mutableStateOf<ProjectMetadata?>(null) }
    var projectForRename by remember { mutableStateOf<ProjectMetadata?>(null) }
    var renameInput by remember { mutableStateOf("") }

    fun refreshProjects() {
        projects = ProjectManager.listProjects(context)
    }

    val filteredProjects = projects.filter { proj ->
        val matchesFilter = when (selectedFilter) {
            HubFilter.ALL -> true
            HubFilter.LOCAL -> proj.storageMode == StorageMode.LOCAL
            HubFilter.CLOUD -> proj.storageMode == StorageMode.CLOUD
        }
        val matchesSearch = searchQuery.isBlank() || proj.name.contains(searchQuery.trim(), ignoreCase = true)
        matchesFilter && matchesSearch
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        text = "QivForge",
                        color = theme.onSurface,
                        fontSize = 20.sp
                    )
                },
                actions = {
                    // Theme customizer button
                    IconButton(onClick = onOpenTheme) {
                        Icon(
                            imageVector = Icons.Default.Palette,
                            contentDescription = "Theme Settings",
                            tint = theme.accent
                        )
                    }
                    // Settings gear
                    IconButton(onClick = onOpenSettings) {
                        Icon(
                            imageVector = Icons.Default.Settings,
                            contentDescription = "Settings",
                            tint = theme.onSurface
                        )
                    }
                    // NOTE: Rule R2 - NO 3-dot menu here!
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = theme.surface)
            )
        },
        floatingActionButton = {
            FloatingActionButton(
                onClick = { showNewProjectModal = true },
                containerColor = theme.primary,
                contentColor = theme.onPrimary
            ) {
                Icon(Icons.Default.Add, contentDescription = "New Project")
            }
        },
        containerColor = theme.surface
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(horizontal = 16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            // Search Input
            OutlinedTextField(
                value = searchQuery,
                onValueChange = { searchQuery = it },
                placeholder = { Text("Search projects...", color = theme.onSurfaceMuted) },
                singleLine = true,
                colors = OutlinedTextFieldDefaults.colors(
                    focusedTextColor = theme.onSurface,
                    unfocusedTextColor = theme.onSurface,
                    focusedBorderColor = theme.primary,
                    unfocusedBorderColor = theme.outline
                ),
                modifier = Modifier.fillMaxWidth()
            )

            // Segmented Filter: All · Local · Cloud (Doc 04 §5)
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                FilterChip(
                    selected = selectedFilter == HubFilter.ALL,
                    onClick = { selectedFilter = HubFilter.ALL },
                    label = { Text("All", color = theme.onSurface) },
                    colors = FilterChipDefaults.filterChipColors(
                        selectedContainerColor = theme.panelAlt,
                        containerColor = theme.surface
                    )
                )
                FilterChip(
                    selected = selectedFilter == HubFilter.LOCAL,
                    onClick = { selectedFilter = HubFilter.LOCAL },
                    label = { Text("Local", color = theme.onSurface) },
                    colors = FilterChipDefaults.filterChipColors(
                        selectedContainerColor = theme.panelAlt,
                        containerColor = theme.surface
                    )
                )
                FilterChip(
                    selected = selectedFilter == HubFilter.CLOUD,
                    onClick = { selectedFilter = HubFilter.CLOUD },
                    label = { Text("Cloud", color = theme.onSurface) },
                    colors = FilterChipDefaults.filterChipColors(
                        selectedContainerColor = theme.panelAlt,
                        containerColor = theme.surface
                    )
                )
            }

            // Projects List or Empty State
            if (filteredProjects.isEmpty()) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f),
                    contentAlignment = Alignment.Center
                ) {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Text(
                            text = if (projects.isEmpty()) "No projects yet" else "No matching projects",
                            color = theme.onSurface,
                            fontSize = 18.sp
                        )
                        Text(
                            text = "Tap + New Project below to get started.",
                            color = theme.onSurfaceMuted,
                            fontSize = 14.sp
                        )
                    }
                }
            } else {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f),
                    verticalArrangement = Arrangement.spacedBy(10.dp)
                ) {
                    items(filteredProjects, key = { it.id }) { proj ->
                        ProjectCard(
                            project = proj,
                            onClick = { onOpenProject(proj) },
                            onLongClick = { projectForActions = proj }
                        )
                    }
                }
            }
        }
    }

    // New Project Modal
    if (showNewProjectModal) {
        NewProjectModal(
            existingNames = projects.map { it.name },
            onCreate = { name, templateId, storageMode, orientation ->
                val res = ProjectManager.createProject(context, name, templateId, storageMode, orientation)
                res.onSuccess { created ->
                    refreshProjects()
                    showNewProjectModal = false
                    Toast.makeText(context, "Created '${created.name}'", Toast.LENGTH_SHORT).show()
                }.onFailure { err ->
                    Toast.makeText(context, err.message ?: "Failed to create", Toast.LENGTH_SHORT).show()
                }
            },
            onDismiss = { showNewProjectModal = false }
        )
    }

    // Contextual Actions Bottom Sheet
    projectForActions?.let { proj ->
        ModalBottomSheet(
            onDismissRequest = { projectForActions = null },
            containerColor = theme.panel
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(20.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(proj.name, color = theme.onSurface, fontSize = 18.sp)
                Divider(color = theme.outline)

                TextButton(
                    onClick = {
                        projectForRename = proj
                        renameInput = proj.name
                        projectForActions = null
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Rename", color = theme.onSurface, modifier = Modifier.fillMaxWidth())
                }

                TextButton(
                    onClick = {
                        val res = ProjectManager.duplicateProject(context, proj.id)
                        res.onSuccess {
                            refreshProjects()
                            Toast.makeText(context, "Project duplicated", Toast.LENGTH_SHORT).show()
                        }
                        projectForActions = null
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Duplicate", color = theme.onSurface, modifier = Modifier.fillMaxWidth())
                }

                TextButton(
                    onClick = {
                        ProjectManager.deleteProject(context, proj.id)
                        refreshProjects()
                        projectForActions = null
                        Toast.makeText(context, "Deleted '${proj.name}'", Toast.LENGTH_SHORT).show()
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Delete", color = theme.error, modifier = Modifier.fillMaxWidth())
                }
            }
        }
    }

    // Rename Dialog
    projectForRename?.let { proj ->
        AlertDialog(
            onDismissRequest = { projectForRename = null },
            containerColor = theme.panel,
            title = { Text("Rename Project", color = theme.onSurface) },
            text = {
                OutlinedTextField(
                    value = renameInput,
                    onValueChange = { renameInput = it },
                    label = { Text("New Name", color = theme.onSurfaceMuted) },
                    singleLine = true,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedTextColor = theme.onSurface,
                        unfocusedTextColor = theme.onSurface,
                        focusedBorderColor = theme.primary,
                        unfocusedBorderColor = theme.outline
                    )
                )
            },
            confirmButton = {
                Button(
                    onClick = {
                        val res = ProjectManager.renameProject(context, proj.id, renameInput)
                        res.onSuccess {
                            refreshProjects()
                            projectForRename = null
                        }.onFailure { err ->
                            Toast.makeText(context, err.message ?: "Failed", Toast.LENGTH_SHORT).show()
                        }
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = theme.primary)
                ) {
                    Text("Save", color = theme.onPrimary)
                }
            },
            dismissButton = {
                TextButton(onClick = { projectForRename = null }) {
                    Text("Cancel", color = theme.onSurfaceMuted)
                }
            }
        )
    }
}
