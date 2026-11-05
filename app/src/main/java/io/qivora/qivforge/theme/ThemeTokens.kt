package io.qivora.qivforge.theme

import androidx.compose.ui.graphics.Color
import org.json.JSONObject

/**
 * QivForge Dynamic Theme Tokens per Doc 04 §2.1
 */
data class ThemeTokens(
    val name: String = "Dark",
    val isDark: Boolean = true,
    val version: Int = 1,

    // Primary & Accents
    val primary: Color,
    val onPrimary: Color,
    val secondary: Color,
    val accent: Color,

    // Surfaces & Panels
    val surface: Color,
    val panel: Color,
    val panelAlt: Color,
    val onSurface: Color,
    val onSurfaceMuted: Color,
    val outline: Color,

    // Status Colors
    val success: Color,
    val warning: Color,
    val error: Color,

    // Visual Patch Nodes (Doc 04 §2.1)
    val nodeEvent: Color,
    val nodePhysics: Color,
    val nodeAnim: Color,
    val nodeCloud: Color,

    // Code Editor Syntax Colors (Doc 04 §2.1)
    val codeKeyword: Color,
    val codeType: Color,
    val codeString: Color,
    val codeNumber: Color,
    val codeComment: Color,
    val codeError: Color
) {
    // Compatibility accessors for UI components
    val surfaceColor: Color get() = surface
    val textColor: Color get() = onSurface
    val primaryColor: Color get() = primary
    val onPrimaryColor: Color get() = onPrimary
    val backgroundColor: Color get() = surface
    val secondaryTextColor: Color get() = onSurfaceMuted
    val onSurfaceColor: Color get() = onSurface
    val onBackgroundColor: Color get() = onSurface
    val borderColor: Color get() = outline
    val border: Color get() = outline
    val dividerColor: Color get() = outline
    val errorColor: Color get() = error

    fun toJson(): String {
        val obj = JSONObject()
        obj.put("name", name)
        obj.put("isDark", isDark)
        obj.put("version", version)

        val tokens = JSONObject()
        tokens.put("primary", primary.toHex())
        tokens.put("onPrimary", onPrimary.toHex())
        tokens.put("secondary", secondary.toHex())
        tokens.put("accent", accent.toHex())

        tokens.put("surface", surface.toHex())
        tokens.put("panel", panel.toHex())
        tokens.put("panelAlt", panelAlt.toHex())
        tokens.put("onSurface", onSurface.toHex())
        tokens.put("onSurfaceMuted", onSurfaceMuted.toHex())
        tokens.put("outline", outline.toHex())

        tokens.put("success", success.toHex())
        tokens.put("warning", warning.toHex())
        tokens.put("error", error.toHex())

        tokens.put("nodeEvent", nodeEvent.toHex())
        tokens.put("nodePhysics", nodePhysics.toHex())
        tokens.put("nodeAnim", nodeAnim.toHex())
        tokens.put("nodeCloud", nodeCloud.toHex())

        tokens.put("codeKeyword", codeKeyword.toHex())
        tokens.put("codeType", codeType.toHex())
        tokens.put("codeString", codeString.toHex())
        tokens.put("codeNumber", codeNumber.toHex())
        tokens.put("codeComment", codeComment.toHex())
        tokens.put("codeError", codeError.toHex())

        obj.put("tokens", tokens)
        return obj.toString(2)
    }

    companion object {
        fun Color.toHex(): String {
            val a = (alpha * 255).toInt()
            val r = (red * 255).toInt()
            val g = (green * 255).toInt()
            val b = (blue * 255).toInt()
            return String.format("#%02X%02X%02X%02X", a, r, g, b)
        }

        fun parseHexColor(hex: String, fallback: Color): Color {
            return try {
                val clean = hex.removePrefix("#")
                val colorLong = clean.toLong(16)
                if (clean.length == 6) {
                    Color(0xFF000000 or colorLong)
                } else if (clean.length == 8) {
                    Color(colorLong)
                } else {
                    fallback
                }
            } catch (e: Exception) {
                fallback
            }
        }

        fun fromJson(jsonStr: String, fallback: ThemeTokens = ThemePresets.Dark): ThemeTokens {
            return try {
                val obj = JSONObject(jsonStr)
                val tokens = obj.optJSONObject("tokens") ?: return fallback

                fallback.copy(
                    name = obj.optString("name", fallback.name),
                    isDark = obj.optBoolean("isDark", fallback.isDark),
                    version = obj.optInt("version", fallback.version),
                    primary = parseHexColor(tokens.optString("primary"), fallback.primary),
                    onPrimary = parseHexColor(tokens.optString("onPrimary"), fallback.onPrimary),
                    secondary = parseHexColor(tokens.optString("secondary"), fallback.secondary),
                    accent = parseHexColor(tokens.optString("accent"), fallback.accent),
                    surface = parseHexColor(tokens.optString("surface"), fallback.surface),
                    panel = parseHexColor(tokens.optString("panel"), fallback.panel),
                    panelAlt = parseHexColor(tokens.optString("panelAlt"), fallback.panelAlt),
                    onSurface = parseHexColor(tokens.optString("onSurface"), fallback.onSurface),
                    onSurfaceMuted = parseHexColor(tokens.optString("onSurfaceMuted"), fallback.onSurfaceMuted),
                    outline = parseHexColor(tokens.optString("outline"), fallback.outline),
                    success = parseHexColor(tokens.optString("success"), fallback.success),
                    warning = parseHexColor(tokens.optString("warning"), fallback.warning),
                    error = parseHexColor(tokens.optString("error"), fallback.error),
                    nodeEvent = parseHexColor(tokens.optString("nodeEvent"), fallback.nodeEvent),
                    nodePhysics = parseHexColor(tokens.optString("nodePhysics"), fallback.nodePhysics),
                    nodeAnim = parseHexColor(tokens.optString("nodeAnim"), fallback.nodeAnim),
                    nodeCloud = parseHexColor(tokens.optString("nodeCloud"), fallback.nodeCloud),
                    codeKeyword = parseHexColor(tokens.optString("codeKeyword"), fallback.codeKeyword),
                    codeType = parseHexColor(tokens.optString("codeType"), fallback.codeType),
                    codeString = parseHexColor(tokens.optString("codeString"), fallback.codeString),
                    codeNumber = parseHexColor(tokens.optString("codeNumber"), fallback.codeNumber),
                    codeComment = parseHexColor(tokens.optString("codeComment"), fallback.codeComment),
                    codeError = parseHexColor(tokens.optString("codeError"), fallback.codeError)
                )
            } catch (e: Exception) {
                fallback
            }
        }
    }
}

// Extensions for State<ThemeTokens> (when used via ThemeManager.currentTheme)
val androidx.compose.runtime.State<ThemeTokens>.surfaceColor: Color get() = value.surfaceColor
val androidx.compose.runtime.State<ThemeTokens>.onSurfaceColor: Color get() = value.onSurfaceColor
val androidx.compose.runtime.State<ThemeTokens>.onBackgroundColor: Color get() = value.onBackgroundColor
val androidx.compose.runtime.State<ThemeTokens>.primaryColor: Color get() = value.primaryColor
val androidx.compose.runtime.State<ThemeTokens>.onPrimaryColor: Color get() = value.onPrimaryColor
val androidx.compose.runtime.State<ThemeTokens>.backgroundColor: Color get() = value.backgroundColor
val androidx.compose.runtime.State<ThemeTokens>.textColor: Color get() = value.textColor
val androidx.compose.runtime.State<ThemeTokens>.secondaryTextColor: Color get() = value.secondaryTextColor
val androidx.compose.runtime.State<ThemeTokens>.borderColor: Color get() = value.borderColor
val androidx.compose.runtime.State<ThemeTokens>.border: Color get() = value.border
val androidx.compose.runtime.State<ThemeTokens>.dividerColor: Color get() = value.dividerColor
val androidx.compose.runtime.State<ThemeTokens>.errorColor: Color get() = value.errorColor
val androidx.compose.runtime.State<ThemeTokens>.surface: Color get() = value.surface
val androidx.compose.runtime.State<ThemeTokens>.panel: Color get() = value.panel
val androidx.compose.runtime.State<ThemeTokens>.panelAlt: Color get() = value.panelAlt
val androidx.compose.runtime.State<ThemeTokens>.primary: Color get() = value.primary
val androidx.compose.runtime.State<ThemeTokens>.secondary: Color get() = value.secondary
val androidx.compose.runtime.State<ThemeTokens>.accent: Color get() = value.accent
val androidx.compose.runtime.State<ThemeTokens>.outline: Color get() = value.outline

