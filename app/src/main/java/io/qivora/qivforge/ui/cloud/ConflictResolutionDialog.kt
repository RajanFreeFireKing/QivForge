package io.qivora.qivforge.ui.cloud

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.theme.ThemeManager

data class ConflictEntry(
    val path: String,
    val isBinary: Boolean,
    val reason: String
)

@Composable
fun ConflictResolutionDialog(
    conflicts: List<ConflictEntry>,
    onDismiss: () -> Unit,
    onResolveBinary: (path: String, action: String) -> Unit,
    onResolveScript: (path: String) -> Unit
) {
    val theme = ThemeManager.currentTheme

    AlertDialog(
        onDismissRequest = onDismiss,
        title = {
            Text(
                "Sync Conflicts Detected",
                fontWeight = FontWeight.Bold,
                color = theme.onSurfaceColor
            )
        },
        text = {
            Column(
                modifier = Modifier.fillMaxWidth(),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(
                    text = "The following files have conflicting edits between your local workspace and remote Hugging Face branch:",
                    color = theme.onSurfaceColor.copy(alpha = 0.8f),
                    fontSize = 13.sp
                )

                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .heightIn(max = 300.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    items(conflicts) { conflict ->
                        Card(
                            shape = RoundedCornerShape(8.dp),
                            colors = CardDefaults.cardColors(containerColor = theme.surfaceColor),
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Column(modifier = Modifier.padding(12.dp)) {
                                Text(
                                    text = conflict.path,
                                    fontWeight = FontWeight.SemiBold,
                                    color = theme.onSurfaceColor,
                                    fontSize = 14.sp
                                )
                                Text(
                                    text = conflict.reason,
                                    color = theme.errorColor,
                                    fontSize = 12.sp
                                )

                                Spacer(modifier = Modifier.height(8.dp))

                                if (conflict.isBinary) {
                                    Row(
                                        modifier = Modifier.fillMaxWidth(),
                                        horizontalArrangement = Arrangement.spacedBy(6.dp)
                                    ) {
                                        Button(
                                            onClick = { onResolveBinary(conflict.path, "KEEP_MINE") },
                                            modifier = Modifier.weight(1f),
                                            contentPadding = PaddingValues(4.dp)
                                        ) {
                                            Text("Keep Mine", fontSize = 11.sp)
                                        }
                                        Button(
                                            onClick = { onResolveBinary(conflict.path, "KEEP_REMOTE") },
                                            modifier = Modifier.weight(1f),
                                            contentPadding = PaddingValues(4.dp)
                                        ) {
                                            Text("Keep Remote", fontSize = 11.sp)
                                        }
                                        OutlinedButton(
                                            onClick = { onResolveBinary(conflict.path, "KEEP_BOTH") },
                                            modifier = Modifier.weight(1f),
                                            contentPadding = PaddingValues(4.dp)
                                        ) {
                                            Text("Keep Both", fontSize = 11.sp)
                                        }
                                    }
                                } else {
                                    Button(
                                        onClick = { onResolveScript(conflict.path) },
                                        modifier = Modifier.fillMaxWidth(),
                                        colors = ButtonDefaults.buttonColors(containerColor = theme.primaryColor)
                                    ) {
                                        Text("Resolve in Code Editor", color = theme.onPrimaryColor, fontSize = 12.sp)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        },
        confirmButton = {
            Button(
                onClick = onDismiss,
                colors = ButtonDefaults.buttonColors(containerColor = theme.primaryColor)
            ) {
                Text("Close", color = theme.onPrimaryColor)
            }
        },
        containerColor = theme.backgroundColor,
        shape = RoundedCornerShape(16.dp)
    )
}
