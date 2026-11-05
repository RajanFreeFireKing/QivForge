package io.qivora.qivforge.ui.cloud

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeManager

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MigrationWizardScreen(
    projectName: String,
    onNavigateBack: () -> Unit
) {
    val theme = ThemeManager.currentTheme
    var repoSlug by remember { mutableStateOf("qf-${projectName.lowercase().replace(" ", "-")}") }
    var isPrivate by remember { mutableStateOf(true) }
    var isMigrating by remember { mutableStateOf(false) }
    var progressFraction by remember { mutableStateOf(0.0f) }
    var statusText by remember { mutableStateOf("Ready to migrate local project to Hugging Face cloud.") }
    var isComplete by remember { mutableStateOf(false) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Cloud Migration Wizard", fontWeight = FontWeight.Bold) },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = theme.surfaceColor,
                    titleContentColor = theme.onSurfaceColor,
                    navigationIconContentColor = theme.onSurfaceColor
                )
            )
        },
        containerColor = theme.backgroundColor
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(24.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Text(
                text = "Migrate '$projectName' to Hugging Face",
                fontWeight = FontWeight.Bold,
                color = theme.onBackgroundColor,
                fontSize = 18.sp
            )

            Text(
                text = "This will create a private Hugging Face dataset repository and package your scenes, scripts, and 3D assets.",
                color = theme.onBackgroundColor.copy(alpha = 0.8f),
                fontSize = 14.sp
            )

            OutlinedTextField(
                value = repoSlug,
                onValueChange = { repoSlug = it },
                label = { Text("Repository Name") },
                singleLine = true,
                enabled = !isMigrating && !isComplete,
                modifier = Modifier.fillMaxWidth()
            )

            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Checkbox(
                    checked = isPrivate,
                    onCheckedChange = { isPrivate = it },
                    enabled = !isMigrating && !isComplete
                )
                Text("Private repository (recommended)", color = theme.onBackgroundColor, fontSize = 14.sp)
            }

            if (isMigrating || isComplete) {
                LinearProgressIndicator(
                    progress = progressFraction,
                    modifier = Modifier.fillMaxWidth().height(8.dp),
                    color = theme.primaryColor,
                    trackColor = theme.borderColor
                )
            }

            Card(
                shape = RoundedCornerShape(8.dp),
                colors = CardDefaults.cardColors(containerColor = theme.surfaceColor),
                modifier = Modifier.fillMaxWidth()
            ) {
                Row(
                    modifier = Modifier.padding(16.dp),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    if (isComplete) {
                        Icon(Icons.Default.CheckCircle, contentDescription = "Success", tint = Color(0xFF4CAF50))
                    }
                    Text(
                        text = statusText,
                        color = theme.onSurfaceColor,
                        fontSize = 13.sp
                    )
                }
            }

            Spacer(modifier = Modifier.weight(1f))

            if (!isComplete) {
                Button(
                    onClick = {
                        isMigrating = true
                        progressFraction = 0.3f
                        statusText = "Creating repository on Hugging Face..."
                    },
                    enabled = !isMigrating,
                    modifier = Modifier.fillMaxWidth().height(48.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = theme.primaryColor)
                ) {
                    Text(if (isMigrating) "Migrating..." else "Start Migration", color = theme.onPrimaryColor)
                }
            } else {
                Button(
                    onClick = onNavigateBack,
                    modifier = Modifier.fillMaxWidth().height(48.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = theme.primaryColor)
                ) {
                    Text("Done", color = theme.onPrimaryColor)
                }
            }
        }
    }
}
