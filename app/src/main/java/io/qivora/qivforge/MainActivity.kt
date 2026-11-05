package io.qivora.qivforge

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import io.qivora.qivforge.engine.NativeBridge
import io.qivora.qivforge.theme.QivForgeTheme
import io.qivora.qivforge.theme.ThemeManager
import io.qivora.qivforge.ui.nav.AppNavigation

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Initialize dynamic theme manager
        ThemeManager.initialize(applicationContext)

        // Initialize baseline native bridge (graceful if native lib absent on host)
        NativeBridge.init(appName = "QivForge", targetFps = 60, useVulkan = true)

        setContent {
            QivForgeTheme {
                AppNavigation()
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        NativeBridge.shutdown()
    }
}
