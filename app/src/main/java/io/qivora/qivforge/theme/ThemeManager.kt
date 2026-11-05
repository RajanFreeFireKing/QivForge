package io.qivora.qivforge.theme

import android.content.Context
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.compositionLocalOf
import androidx.compose.runtime.mutableStateOf
import java.io.File

val LocalQivTheme = compositionLocalOf<ThemeTokens> { ThemePresets.Dark }

/**
 * Singleton theme controller managing runtime theme changes and persistence
 */
object ThemeManager {
    private const val THEME_FILE_NAME = "current_theme.json"

    val currentTheme = mutableStateOf(ThemePresets.Dark)

    fun initialize(context: Context) {
        val file = File(context.filesDir, THEME_FILE_NAME)
        if (file.exists()) {
            try {
                val json = file.readText()
                val loaded = ThemeTokens.fromJson(json, ThemePresets.Dark)
                currentTheme.value = loaded
            } catch (e: Exception) {
                currentTheme.value = ThemePresets.Dark
            }
        }
    }

    fun setTheme(newTheme: ThemeTokens, context: Context? = null) {
        currentTheme.value = newTheme
        if (context != null) {
            persist(context)
        }
    }

    fun applyPreset(presetName: String, context: Context? = null) {
        val preset = ThemePresets.AllPresets.firstOrNull { it.name.equals(presetName, ignoreCase = true) }
            ?: ThemePresets.Dark
        setTheme(preset, context)
    }

    private fun persist(context: Context) {
        try {
            val file = File(context.filesDir, THEME_FILE_NAME)
            file.writeText(currentTheme.value.toJson())
        } catch (e: Exception) {
            // Ignore persistence errors
        }
    }

    fun exportThemeJson(): String = currentTheme.value.toJson()

    fun importThemeJson(jsonStr: String, context: Context? = null): Boolean {
        return try {
            val imported = ThemeTokens.fromJson(jsonStr, currentTheme.value)
            setTheme(imported, context)
            true
        } catch (e: Exception) {
            false
        }
    }
}

@Composable
fun QivForgeTheme(
    theme: ThemeTokens = ThemeManager.currentTheme.value,
    content: @Composable () -> Unit
) {
    CompositionLocalProvider(LocalQivTheme provides theme) {
        content()
    }
}
