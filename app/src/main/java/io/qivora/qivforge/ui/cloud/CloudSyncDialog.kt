package io.qivora.qivforge.ui.cloud

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeManager

@Composable
fun CloudSyncDialog(
    pendingCount: Int,
    isSyncing: Boolean,
    onDismiss: () -> Unit,
    onConfirmSync: (commitMessage: String) -> Unit
) {
    val theme = ThemeManager.currentTheme
    var message by remember { mutableStateOf("QivForge project sync") }

    AlertDialog(
        onDismissRequest = { if (!isSyncing) onDismiss() },
        title = {
            Text(
                "Sync with Hugging Face",
                fontWeight = FontWeight.Bold,
                color = theme.onSurfaceColor
            )
        },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                Text(
                    text = "You have $pendingCount pending change(s) ready to push to your repository.",
                    color = theme.onSurfaceColor.copy(alpha = 0.8f),
                    fontSize = 14.sp
                )

                OutlinedTextField(
                    value = message,
                    onValueChange = { message = it },
                    label = { Text("Commit summary") },
                    singleLine = true,
                    enabled = !isSyncing,
                    modifier = Modifier.fillMaxWidth()
                )

                if (isSyncing) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.Center
                    ) {
                        CircularProgressIndicator(color = theme.primaryColor)
                    }
                }
            }
        },
        confirmButton = {
            Button(
                onClick = { onConfirmSync(message) },
                enabled = !isSyncing,
                colors = ButtonDefaults.buttonColors(containerColor = theme.primaryColor)
            ) {
                Text(if (isSyncing) "Syncing..." else "Sync Now", color = theme.onPrimaryColor)
            }
        },
        dismissButton = {
            if (!isSyncing) {
                TextButton(onClick = onDismiss) {
                    Text("Cancel", color = theme.onSurfaceColor)
                }
            }
        },
        containerColor = theme.surfaceColor,
        shape = RoundedCornerShape(16.dp)
    )
}
