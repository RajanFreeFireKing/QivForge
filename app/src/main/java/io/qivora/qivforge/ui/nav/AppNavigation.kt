package io.qivora.qivforge.ui.nav

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.project.ProjectMetadata
import io.qivora.qivforge.theme.LocalQivTheme
import io.qivora.qivforge.theme.ThemeScreen
import io.qivora.qivforge.ui.hub.ProjectHubScreen
import io.qivora.qivforge.ui.settings.SettingsScreen

sealed class Destination {
    object Hub : Destination()
    object Theme : Destination()
    object Settings : Destination()
    data class Workspace(val project: ProjectMetadata) : Destination()
}

@Composable
fun AppNavigation() {
    var currentScreen by remember { mutableStateOf<Destination>(Destination.Hub) }
    val theme = LocalQivTheme.current

    when (val dest = currentScreen) {
        is Destination.Hub -> {
            ProjectHubScreen(
                onOpenProject = { proj -> currentScreen = Destination.Workspace(proj) },
                onOpenTheme = { currentScreen = Destination.Theme },
                onOpenSettings = { currentScreen = Destination.Settings }
            )
        }
        is Destination.Theme -> {
            ThemeScreen(onBack = { currentScreen = Destination.Hub })
        }
        is Destination.Settings -> {
            SettingsScreen(
                onBack = { currentScreen = Destination.Hub },
                onNavigateTheme = { currentScreen = Destination.Theme }
            )
        }
        is Destination.Workspace -> {
            io.qivora.qivforge.ui.editor.EditorWorkspaceScreen(
                project = dest.project,
                onBackToHub = { currentScreen = Destination.Hub }
            )
        }
    }
}
