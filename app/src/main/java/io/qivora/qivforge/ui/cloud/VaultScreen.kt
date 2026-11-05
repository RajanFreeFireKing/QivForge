package io.qivora.qivforge.ui.cloud

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Lock
import androidx.compose.material.icons.filled.Visibility
import androidx.compose.material.icons.filled.VisibilityOff
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.qivora.qivforge.security.SecureVault
import io.qivora.qivforge.security.VaultKey
import io.qivora.qivforge.theme.ThemeManager
import kotlinx.coroutines.launch
import java.util.Arrays

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun VaultScreen(
    onNavigateBack: () -> Unit
) {
    val theme = ThemeManager.currentTheme
    val scope = rememberCoroutineScope()

    var tokenInput by remember { mutableStateOf("") }
    var passwordVisible by remember { mutableStateOf(false) }
    var isConnected by remember { mutableStateOf(SecureVault.instance.hasKey(VaultKey.HF_TOKEN)) }
    var statusMessage by remember { mutableStateOf(if (isConnected) "Connected: Hugging Face Token active" else "No token configured") }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Security Vault", fontWeight = FontWeight.Bold) },
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
            Card(
                shape = RoundedCornerShape(12.dp),
                colors = CardDefaults.cardColors(
                    containerColor = if (isConnected) Color(0xFF1E3A24) else Color(0xFF2C241E)
                ),
                modifier = Modifier.fillMaxWidth()
            ) {
                Row(
                    modifier = Modifier.padding(16.dp),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    Icon(
                        Icons.Default.Lock,
                        contentDescription = "Security Status",
                        tint = if (isConnected) Color(0xFF4CAF50) else Color(0xFFFF9800)
                    )
                    Column {
                        Text(
                            text = if (isConnected) "Vault Secured" else "Vault Unlocked",
                            fontWeight = FontWeight.Bold,
                            color = theme.onSurfaceColor,
                            fontSize = 16.sp
                        )
                        Text(
                            text = statusMessage,
                            color = theme.onSurfaceColor.copy(alpha = 0.8f),
                            fontSize = 13.sp
                        )
                    }
                }
            }

            Text(
                text = "Hugging Face Fine-Grained Token",
                fontWeight = FontWeight.SemiBold,
                color = theme.onBackgroundColor,
                fontSize = 14.sp
            )

            OutlinedTextField(
                value = tokenInput,
                onValueChange = { tokenInput = it },
                label = { Text("hf_...") },
                singleLine = true,
                visualTransformation = if (passwordVisible) VisualTransformation.None else PasswordVisualTransformation(),
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password, autoCorrect = false),
                trailingIcon = {
                    IconButton(onClick = { passwordVisible = !passwordVisible }) {
                        Icon(
                            imageVector = if (passwordVisible) Icons.Default.Visibility else Icons.Default.VisibilityOff,
                            contentDescription = if (passwordVisible) "Hide password" else "Show password"
                        )
                    }
                },
                modifier = Modifier.fillMaxWidth(),
                colors = OutlinedTextFieldDefaults.colors(
                    focusedTextColor = theme.onSurfaceColor,
                    unfocusedTextColor = theme.onSurfaceColor,
                    focusedBorderColor = theme.primaryColor,
                    unfocusedBorderColor = theme.borderColor
                )
            )

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Button(
                    onClick = {
                        if (tokenInput.isNotBlank()) {
                            scope.launch {
                                val charArray = tokenInput.toCharArray()
                                SecureVault.instance.put(VaultKey.HF_TOKEN, charArray)
                                Arrays.fill(charArray, '\u0000') // Zero local buffer
                                isConnected = true
                                statusMessage = "Saved to Hardware Keystore Vault"
                                tokenInput = ""
                            }
                        }
                    },
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(containerColor = theme.primaryColor)
                ) {
                    Text("Save to Vault", color = theme.onPrimaryColor)
                }

                if (isConnected) {
                    OutlinedButton(
                        onClick = {
                            scope.launch {
                                SecureVault.instance.remove(VaultKey.HF_TOKEN)
                                isConnected = false
                                statusMessage = "Token removed from Vault"
                            }
                        },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("Disconnect", color = theme.errorColor)
                    }
                }
            }

            Text(
                text = "Security Notice: Tokens are never written to project files, logs, or exports. Screens in this section enforce FLAG_SECURE protection.",
                color = theme.onBackgroundColor.copy(alpha = 0.6f),
                fontSize = 12.sp,
                lineHeight = 16.sp
            )
        }
    }
}
