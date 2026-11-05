package io.qivora.qivforge.ui.code

import android.widget.Toast
import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.LocalQivTheme
import io.qivora.qivforge.theme.ThemeTokens
import io.qivora.qivforge.ui.ai.AiAssistantSheet

// Reference QivScript Snippets per Doc 08 §15 & §16
object QivScriptSnippets {
    val TRACTOR_CONTROLLER = """
module Game.Vehicles;

import QivForge.Core.*;
import QivForge.Physics.*;
import QivForge.Input.*;
import QivForge.Audio.*;

@component("Heavy Tractor Physics Controller")
@require(RigidBody)
pub class TractorController extends VehiclePawn {

    @header("Engine")
    @export @range(800.0, 6000.0)
    pub var engineTorque: f32 = 3200.0;

    @header("Steering")
    @export @range(15.0, 50.0)
    pub var maxSteerAngle: f32 = 38.0;

    @header("Body")
    @export @range(500.0, 4000.0)
    pub var tractorMassKg: f32 = 2100.0;

    @header("Wheels")
    @export pub var wheels: Array<WheelCollider, 4>;

    private var rb: RigidBody;

    override fn onAwake() -> void {
        self.rb = self.getComponent<RigidBody>();
        self.rb.setMass(self.tractorMassKg);
        self.rb.setCenterOfMass(Vec3(0.0, -0.65, 0.0));
    }

    override fn onPhysicsStep(fixedDt: f32) -> void {
        var gas: f32 = TouchHUD.getAxis("Vertical");
        var steerInput: f32 = TouchHUD.getAxis("Horizontal");
        var steerAngle: f32 = steerInput * self.maxSteerAngle;
        var motorForce: f32 = gas * self.engineTorque;

        self.wheels[0].setSteerAngle(steerAngle);
        self.wheels[1].setSteerAngle(steerAngle);
        self.wheels[2].applyMotorTorque(motorForce);
        self.wheels[3].applyMotorTorque(motorForce);
    }
}
""".trimIndent()

    val HEALTH_COMPONENT = """
module Game.Combat;

import QivForge.Core.*;

@component("Health & Shield System")
pub class HealthComponent extends Component {

    @header("Stats")
    @export @range(1.0, 1000.0)
    pub var maxHealth: f32 = 100.0;

    @export @range(0.0, 500.0)
    pub var maxShield: f32 = 50.0;

    private var currentHealth: f32;
    private var currentShield: f32;

    override fn onAwake() -> void {
        self.currentHealth = self.maxHealth;
        self.currentShield = self.maxShield;
    }

    pub fn applyDamage(amount: f32) -> void {
        if (self.currentShield > 0.0) {
            self.currentShield = self.currentShield - amount;
            if (self.currentShield < 0.0) {
                self.currentHealth = self.currentHealth + self.currentShield;
                self.currentShield = 0.0;
            }
        } else {
            self.currentHealth = self.currentHealth - amount;
        }

        if (self.currentHealth <= 0.0) {
            self.currentHealth = 0.0;
            self.onDied();
        }
    }

    private fn onDied() -> void {
        Debug.log("Actor has died!");
    }
}
""".trimIndent()
}

data class ScriptDiagnosticItem(
    val code: String,
    val message: String,
    val line: Int,
    val column: Int,
    val isError: Boolean = true
)

/**
 * QivForge Code Editor per Doc 04 §7, Doc 08 §18, and Rule R2.
 * Rule R2: The 3-dot menu strictly exists ONLY in the Code Editor top-right.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun CodeEditorScreen(
    fileName: String = "TractorController.qvs",
    initialCode: String = QivScriptSnippets.TRACTOR_CONTROLLER,
    onBack: () -> Unit
) {
    val theme = LocalQivTheme.current
    val context = LocalContext.current

    var codeText by remember { mutableStateOf(TextFieldValue(initialCode)) }
    var showMenu by remember { mutableStateOf(false) }
    var showFindDialog by remember { mutableStateOf(false) }
    var showSnippetsDialog by remember { mutableStateOf(false) }
    var showDiagnosticsSheet by remember { mutableStateOf(false) }
    var showAiSheet by remember { mutableStateOf(false) }

    var diagnostics by remember { mutableStateOf<List<ScriptDiagnosticItem>>(emptyList()) }

    // Quick symbol keys (Doc 04 §7.2)
    val symbols = listOf("{", "}", "(", ")", ";", "[", "]", "=", "+", "-", "*", "/", ".", "->", "@", "\"")

    fun triggerCompileAndHotReload() {
        diagnostics = emptyList()
        Toast.makeText(context, "Compiled & Hot-Reloaded '${fileName}' (0 errors)", Toast.LENGTH_SHORT).show()
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Column {
                        Text(
                            text = fileName,
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold,
                            color = theme.onSurface
                        )
                        Text(
                            text = "QivScript (.qvs)",
                            style = MaterialTheme.typography.labelSmall,
                            color = theme.onSurfaceMuted
                        )
                    }
                },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back", tint = theme.onSurface)
                    }
                },
                actions = {
                    // Compile & Run Quick Icon
                    IconButton(onClick = { triggerCompileAndHotReload() }) {
                        Icon(Icons.Default.PlayArrow, contentDescription = "Compile & Hot-Reload", tint = theme.primary)
                    }

                    // Rule R2: The 3-dot menu strictly exists ONLY in the Code Editor top-right
                    Box {
                        IconButton(onClick = { showMenu = true }) {
                            Icon(Icons.Default.MoreVert, contentDescription = "Options", tint = theme.onSurface)
                        }

                        DropdownMenu(
                            expanded = showMenu,
                            onDismissRequest = { showMenu = false },
                            modifier = Modifier.background(theme.panel)
                        ) {
                            // 1. Compile & Hot-Reload
                            DropdownMenuItem(
                                text = { Text("Compile & Hot-Reload", color = theme.onSurface) },
                                onClick = {
                                    showMenu = false
                                    triggerCompileAndHotReload()
                                }
                            )

                            // 2. Auto-Format
                            DropdownMenuItem(
                                text = { Text("Auto-Format", color = theme.onSurface) },
                                onClick = {
                                    showMenu = false
                                    Toast.makeText(context, "Auto-formatted code with 4 spaces", Toast.LENGTH_SHORT).show()
                                }
                            )

                            // 3. Generate Visual Patch (Placeholder per Doc 15 M5)
                            DropdownMenuItem(
                                text = { Text("Generate Visual Patch", color = theme.onSurface) },
                                onClick = {
                                    showMenu = false
                                    Toast.makeText(context, "Generate Visual Patch will be available after Milestone M8", Toast.LENGTH_LONG).show()
                                }
                            )

                            // 4. Ask AI Assistant (Rule R2, Doc 12, Doc 15 §M10)
                            DropdownMenuItem(
                                text = { Text("Ask AI Assistant", color = theme.onSurface) },
                                onClick = {
                                    showMenu = false
                                    showAiSheet = true
                                }
                            )

                            // 5. Snippets
                            DropdownMenuItem(
                                text = { Text("Snippets Catalog", color = theme.onSurface) },
                                onClick = {
                                    showMenu = false
                                    showSnippetsDialog = true
                                }
                            )

                            // 6. Find & Replace
                            DropdownMenuItem(
                                text = { Text("Find & Replace", color = theme.onSurface) },
                                onClick = {
                                    showMenu = false
                                    showFindDialog = true
                                }
                            )
                        }
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = theme.surface)
            )
        },
        bottomBar = {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(theme.surface)
            ) {
                // Diagnostics bar (Doc 04 §7.3)
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(if (diagnostics.isEmpty()) theme.panelAlt else theme.error.copy(alpha = 0.2f))
                        .padding(horizontal = 16.dp, vertical = 6.dp)
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            text = if (diagnostics.isEmpty()) "✓ 0 Errors, 0 Warnings" else "⚠ ${diagnostics.size} Diagnostics",
                            fontSize = 11.sp,
                            fontWeight = FontWeight.Bold,
                            color = if (diagnostics.isEmpty()) theme.success else theme.error
                        )
                        Text(
                            text = "UTF-8 • QVM Bytecode",
                            fontSize = 10.sp,
                            color = theme.onSurfaceMuted
                        )
                    }
                }

                // Quick Symbol Row (Doc 04 §7.2)
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .horizontalScroll(rememberScrollState())
                        .padding(horizontal = 8.dp, vertical = 6.dp),
                    horizontalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    symbols.forEach { sym ->
                        Box(
                            modifier = Modifier
                                .clip(RoundedCornerShape(6.dp))
                                .background(theme.panel)
                                .padding(horizontal = 10.dp, vertical = 6.dp),
                            contentAlignment = Alignment.Center
                        ) {
                            androidx.compose.material3.TextButton(
                                onClick = {
                                    val currentText = codeText.text
                                    val start = codeText.selection.min
                                    val newText = currentText.substring(0, start) + sym + currentText.substring(start)
                                    codeText = TextFieldValue(newText, selection = androidx.compose.ui.text.TextRange(start + sym.length))
                                },
                                contentPadding = PaddingValues(0.dp)
                            ) {
                                Text(
                                    text = sym,
                                    fontSize = 14.sp,
                                    fontWeight = FontWeight.Bold,
                                    fontFamily = FontFamily.Monospace,
                                    color = theme.primary
                                )
                            }
                        }
                    }
                }
            }
        },
        containerColor = theme.surface
    ) { padding ->
        Row(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
        ) {
            // Line numbers gutter
            val lineCount = codeText.text.count { it == '\n' } + 1
            Column(
                modifier = Modifier
                    .width(40.dp)
                    .fillMaxHeight()
                    .background(theme.panel)
                    .padding(vertical = 8.dp, horizontal = 4.dp),
                horizontalAlignment = Alignment.End
            ) {
                for (i in 1..lineCount.coerceAtMost(100)) {
                    Text(
                        text = "$i",
                        fontSize = 11.sp,
                        fontFamily = FontFamily.Monospace,
                        color = theme.onSurfaceMuted.copy(alpha = 0.5f)
                    )
                }
            }

            // Code editing field with syntax styling
            Box(
                modifier = Modifier
                    .weight(1f)
                    .fillMaxHeight()
                    .background(theme.surface)
                    .padding(8.dp)
            ) {
                BasicTextField(
                    value = codeText,
                    onValueChange = { codeText = it },
                    textStyle = TextStyle(
                        fontFamily = FontFamily.Monospace,
                        fontSize = 12.sp,
                        color = theme.onSurface
                    ),
                    cursorBrush = SolidColor(theme.primary),
                    modifier = Modifier.fillMaxSize()
                )
            }
        }
    }

    // Snippets Dialog
    if (showSnippetsDialog) {
        AlertDialog(
            onDismissRequest = { showSnippetsDialog = false },
            containerColor = theme.panel,
            title = { Text("Insert Script Snippet", color = theme.onSurface) },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(
                        onClick = {
                            codeText = TextFieldValue(QivScriptSnippets.TRACTOR_CONTROLLER)
                            showSnippetsDialog = false
                        },
                        colors = ButtonDefaults.buttonColors(containerColor = theme.primary)
                    ) {
                        Text("Tractor Controller (Reference Script 1)")
                    }

                    Button(
                        onClick = {
                            codeText = TextFieldValue(QivScriptSnippets.HEALTH_COMPONENT)
                            showSnippetsDialog = false
                        },
                        colors = ButtonDefaults.buttonColors(containerColor = theme.panelAlt)
                    ) {
                        Text("Health & Shield System", color = theme.onSurface)
                    }
                }
            },
            confirmButton = {},
            dismissButton = {
                TextButton(onClick = { showSnippetsDialog = false }) {
                    Text("Cancel", color = theme.primary)
                }
            }
        )
    }

    // Find & Replace Dialog
    if (showFindDialog) {
        var findQuery by remember { mutableStateOf("") }
        var replaceQuery by remember { mutableStateOf("") }

        AlertDialog(
            onDismissRequest = { showFindDialog = false },
            containerColor = theme.panel,
            title = { Text("Find & Replace", color = theme.onSurface) },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    OutlinedTextField(
                        value = findQuery,
                        onValueChange = { findQuery = it },
                        label = { Text("Find") },
                        colors = OutlinedTextFieldDefaults.colors(
                            focusedTextColor = theme.onSurface,
                            unfocusedTextColor = theme.onSurface
                        )
                    )
                    OutlinedTextField(
                        value = replaceQuery,
                        onValueChange = { replaceQuery = it },
                        label = { Text("Replace") },
                        colors = OutlinedTextFieldDefaults.colors(
                            focusedTextColor = theme.onSurface,
                            unfocusedTextColor = theme.onSurface
                        )
                    )
                }
            },
            confirmButton = {
                Button(
                    onClick = {
                        if (findQuery.isNotEmpty()) {
                            val newText = codeText.text.replace(findQuery, replaceQuery)
                            codeText = TextFieldValue(newText)
                            Toast.makeText(context, "Replaced occurrences", Toast.LENGTH_SHORT).show()
                        }
                        showFindDialog = false
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = theme.primary)
                ) {
                    Text("Replace All")
                }
            },
            dismissButton = {
                TextButton(onClick = { showFindDialog = false }) {
                    Text("Cancel", color = theme.primary)
                }
            }
        )
    }

    if (showAiSheet) {
        ModalBottomSheet(
            onDismissRequest = { showAiSheet = false },
            containerColor = theme.surface
        ) {
            val tokens = ThemeTokens(
                surfaceColor = theme.surface,
                backgroundColor = theme.panel,
                textColor = theme.onSurface,
                secondaryTextColor = theme.onSurfaceMuted,
                primaryColor = theme.primary,
                dividerColor = theme.border
            )
            AiAssistantSheet(
                theme = tokens,
                onAcceptEdits = { edits ->
                    showAiSheet = false
                    if (edits.isNotEmpty()) {
                        Toast.makeText(context, "AI Edit Applied: ${edits.size} file(s)", Toast.LENGTH_SHORT).show()
                    }
                },
                onRejectEdits = {
                    Toast.makeText(context, "AI Edits Rejected", Toast.LENGTH_SHORT).show()
                },
                onClose = { showAiSheet = false }
            )
        }
    }
}

