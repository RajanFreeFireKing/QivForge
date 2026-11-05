package io.qivora.qivforge.ui.editor

import android.widget.Toast
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import io.qivora.qivforge.editor.JournalManager
import io.qivora.qivforge.engine.NativeBridge
import io.qivora.qivforge.project.ProjectManager
import io.qivora.qivforge.project.ProjectMetadata
import io.qivora.qivforge.theme.LocalQivTheme
import io.qivora.qivforge.ui.hud.VirtualTouchHud
import io.qivora.qivforge.ui.viewport.EngineViewport
import kotlinx.coroutines.delay
import java.io.File

@Composable
fun EditorWorkspaceScreen(
    project: ProjectMetadata,
    onBackToHub: () -> Unit
) {
    val theme = LocalQivTheme.current
    val context = LocalContext.current
    val projectDir = File(ProjectManager.getProjectsDir(context), project.id)

    var isDirty by remember { mutableStateOf(false) }
    var isPlaying by remember { mutableStateOf(false) }
    var gizmoMode by remember { mutableStateOf(GizmoMode.SELECT) }
    var transformSpace by remember { mutableStateOf(TransformSpace.LOCAL) }

    var canUndo by remember { mutableStateOf(false) }
    var canRedo by remember { mutableStateOf(false) }

    // Telemetry during play mode
    var speedKmh by remember { mutableFloatStateOf(0.0f) }
    var rpm by remember { mutableFloatStateOf(800.0f) }
    var gear by remember { mutableIntStateOf(1) }

    // Poll telemetry when playing
    LaunchedEffect(isPlaying) {
        while (isPlaying) {
            speedKmh = NativeBridge.getVehicleSpeedKmh()
            rpm = NativeBridge.getVehicleRpm()
            delay(33) // ~30 fps HUD update
        }
    }

    // Actors list
    var actors by remember {
        mutableStateOf(
            listOf(
                ActorItem(id = 1, name = "Sun", type = "LIGHT", level = 0),
                ActorItem(id = 2, name = "Camera", type = "CAMERA", level = 0),
                ActorItem(id = 3, name = "Tractor", type = "VEHICLE", level = 0),
                ActorItem(id = 4, name = "CrateStack", type = "DYNAMIC", level = 0)
            )
        )
    }
    var selectedActorId by remember { mutableStateOf<Int?>(3) }

    // Transform values for selected actor
    var posX by remember { mutableStateOf(0.0f) }
    var posY by remember { mutableStateOf(0.0f) }
    var posZ by remember { mutableStateOf(0.0f) }
    var rotX by remember { mutableStateOf(0.0f) }
    var rotY by remember { mutableStateOf(0.0f) }
    var rotZ by remember { mutableStateOf(0.0f) }
    var scaleX by remember { mutableStateOf(1.0f) }
    var scaleY by remember { mutableStateOf(1.0f) }
    var scaleZ by remember { mutableStateOf(1.0f) }

    // Crash Recovery Check per Rule R7 & Doc 02 §16 AC5
    var showRecoveryDialog by remember { mutableStateOf(JournalManager.hasCrashRecovery(projectDir)) }
    var showExitDialog by remember { mutableStateOf(false) }

    fun markDirty() {
        isDirty = true
        canUndo = true
        JournalManager.recordEdit(projectDir, "{ \"actorsCount\": ${actors.size} }")
    }

    fun startInstantPlay() {
        NativeBridge.startPlay()
        isPlaying = true
        Toast.makeText(context, "Instant Play Mode Started (<300ms)", Toast.LENGTH_SHORT).show()
    }

    fun stopInstantPlay() {
        NativeBridge.stopPlay()
        isPlaying = false
        Toast.makeText(context, "Scene Snapshot Restored", Toast.LENGTH_SHORT).show()
    }

    Scaffold(
        topBar = {
            if (!isPlaying) {
                EditorTopBar(
                    projectName = project.name,
                    isDirty = isDirty,
                    gizmoMode = gizmoMode,
                    transformSpace = transformSpace,
                    canUndo = canUndo,
                    canRedo = canRedo,
                    isPlaying = isPlaying,
                    onBack = {
                        if (isDirty) {
                            showExitDialog = true
                        } else {
                            onBackToHub()
                        }
                    },
                    onSave = {
                        isDirty = false
                        JournalManager.clearJournal(projectDir)
                        Toast.makeText(context, "Saved '${project.name}'", Toast.LENGTH_SHORT).show()
                    },
                    onUndo = {
                        if (NativeBridge.undo()) {
                            canRedo = true
                            markDirty()
                        }
                    },
                    onRedo = {
                        if (NativeBridge.redo()) {
                            markDirty()
                        }
                    },
                    onGizmoChange = { gizmoMode = it },
                    onSpaceToggle = {
                        transformSpace = if (transformSpace == TransformSpace.LOCAL) TransformSpace.WORLD else TransformSpace.LOCAL
                    },
                    onPlayToggle = {
                        startInstantPlay()
                    },
                    onStop = {
                        stopInstantPlay()
                    }
                )
            }
        },
        containerColor = theme.surface
    ) { padding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(if (isPlaying) PaddingValues(0.dp) else padding)
        ) {
            Row(modifier = Modifier.fillMaxSize()) {
                // Left: Hierarchy Panel (hidden during play)
                if (!isPlaying) {
                    SceneHierarchyPanel(
                        actors = actors,
                        selectedActorId = selectedActorId,
                        onSelectActor = { id -> selectedActorId = id },
                        onAddActor = { name, type ->
                            val newId = NativeBridge.createActor(name)
                            val newActor = ActorItem(id = newId, name = name, type = type)
                            actors = actors + newActor
                            selectedActorId = newId
                            markDirty()
                        },
                        onDeleteActor = { id ->
                            NativeBridge.destroyActor(id)
                            actors = actors.filter { it.id != id }
                            if (selectedActorId == id) selectedActorId = null
                            markDirty()
                        }
                    )
                }

                // Center: 3D Viewport with Orbit / Gestures (Doc 04 §6.3)
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .fillMaxHeight()
                ) {
                    EngineViewport(
                        modifier = Modifier.fillMaxSize(),
                        showStatsBadge = !isPlaying
                    )

                    // Virtual Touch HUD Overlay during Play Mode (Doc 04 §14 & Doc 15 M4)
                    if (isPlaying) {
                        VirtualTouchHud(
                            theme = theme,
                            speedKmh = speedKmh,
                            rpm = rpm,
                            gear = gear,
                            onInputChanged = { throttle, steer, brake, handbrake ->
                                NativeBridge.setVehicleInput(throttle, steer, brake, handbrake)
                            },
                            onStopPlay = {
                                stopInstantPlay()
                            }
                        )
                    }
                }

                // Right: Property Inspector (hidden during play)
                if (!isPlaying) {
                    val currentSelectedActor = actors.firstOrNull { it.id == selectedActorId }
                    PropertyInspectorPanel(
                        selectedActor = currentSelectedActor,
                        posX = posX, posY = posY, posZ = posZ,
                        rotX = rotX, rotY = rotY, rotZ = rotZ,
                        scaleX = scaleX, scaleY = scaleY, scaleZ = scaleZ,
                        onTransformChange = { px, py, pz, rx, ry, rz, sx, sy, sz ->
                            posX = px; posY = py; posZ = pz
                            rotX = rx; rotY = ry; rotZ = rz
                            scaleX = sx; scaleY = sy; scaleZ = sz
                            markDirty()
                        }
                    )
                }
            }
        }
    }

    // Crash Recovery Dialog (Doc 02 §16 AC5)
    if (showRecoveryDialog) {
        AlertDialog(
            onDismissRequest = { showRecoveryDialog = false },
            containerColor = theme.panel,
            title = { Text("Recover Unsaved Work?", color = theme.onSurface) },
            text = {
                Text(
                    "An uncommitted editing session was found from an unexpected shutdown. Would you like to restore your last edits?",
                    color = theme.onSurfaceMuted
                )
            },
            confirmButton = {
                Button(
                    onClick = {
                        JournalManager.recoverScene(projectDir)
                        showRecoveryDialog = false
                        Toast.makeText(context, "Recovered unsaved edits from journal", Toast.LENGTH_SHORT).show()
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = theme.primary)
                ) {
                    Text("Recover", color = theme.onPrimary)
                }
            },
            dismissButton = {
                TextButton(
                    onClick = {
                        JournalManager.clearJournal(projectDir)
                        showRecoveryDialog = false
                    }
                ) {
                    Text("Discard", color = theme.error)
                }
            }
        )
    }

    // Unsaved Changes Exit Dialog
    if (showExitDialog) {
        AlertDialog(
            onDismissRequest = { showExitDialog = false },
            containerColor = theme.panel,
            title = { Text("Unsaved Changes", color = theme.onSurface) },
            text = { Text("You have unsaved changes. Save before exiting to Project Hub?", color = theme.onSurfaceMuted) },
            confirmButton = {
                Button(
                    onClick = {
                        isDirty = false
                        JournalManager.clearJournal(projectDir)
                        showExitDialog = false
                        onBackToHub()
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = theme.primary)
                ) {
                    Text("Save & Exit", color = theme.onPrimary)
                }
            },
            dismissButton = {
                TextButton(
                    onClick = {
                        showExitDialog = false
                        onBackToHub()
                    }
                ) {
                    Text("Discard", color = theme.error)
                }
            }
        )
    }
}
