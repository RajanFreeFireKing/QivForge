package io.qivora.qivforge.theme

import android.content.Context
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.compositionLocalOf
import androidx.compose.runtime.mutableStateOf
import java.io.File

val LocalQivTheme = compositionLocalOf<ThemeTokens> { ThemePresets.Dark }

class ThemeStateHolder(initial: ThemeTokens) : androidx.compose.runtime.State<ThemeTokens> {
    private val state = mutableStateOf(initial)
    override var value: ThemeTokens
        get() = state.value
        set(v) { state.value = v }

    val name get() = value.name
    val isDark get() = value.isDark
    val version get() = value.version

    val primary get() = value.primary
    val onPrimary get() = value.onPrimary
    val secondary get() = value.secondary
    val accent get() = value.accent

    val surface get() = value.surface
    val panel get() = value.panel
    val panelAlt get() = value.panelAlt
    val onSurface get() = value.onSurface
    val onSurfaceMuted get() = value.onSurfaceMuted
    val outline get() = value.outline

    val success get() = value.success
    val warning get() = value.warning
    val error get() = value.error

    val nodeEvent get() = value.nodeEvent
    val nodePhysics get() = value.nodePhysics
    val nodeAnim get() = value.nodeAnim
    val nodeCloud get() = value.nodeCloud

    val codeKeyword get() = value.codeKeyword
    val codeType get() = value.codeType
    val codeString get() = value.codeString
    val codeNumber get() = value.codeNumber
    val codeComment get() = value.codeComment
    val codeError get() = value.codeError

    val surfaceColor get() = value.surfaceColor
    val textColor get() = value.textColor
    val primaryColor get() = value.primaryColor
    val onPrimaryColor get() = value.onPrimaryColor
    val backgroundColor get() = value.backgroundColor
    val secondaryTextColor get() = value.secondaryTextColor
    val onSurfaceColor get() = value.onSurfaceColor
    val onBackgroundColor get() = value.onBackgroundColor
    val borderColor get() = value.borderColor
    val border get() = value.border
    val dividerColor get() = value.dividerColor
    val errorColor get() = value.errorColor
    val background get() = value.background
    val surfaceVariant get() = value.surfaceVariant
}

/**
 * Singleton theme controller managing runtime theme changes and persistence
 */
object ThemeManager {
    private const val THEME_FILE_NAME = "current_theme.json"

    val currentTheme = ThemeStateHolder(ThemePresets.Dark)

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
