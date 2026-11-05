package io.qivora.qivforge.theme

import androidx.compose.ui.graphics.Color

/**
 * Built-in Theme Presets per Doc 04 §2.2
 */
object ThemePresets {

    val Dark = ThemeTokens(
        name = "Dark",
        isDark = true,
        version = 1,
        primary = Color(0xFF2563EB),
        onPrimary = Color(0xFFFFFFFF),
        secondary = Color(0xFF475569),
        accent = Color(0xFF06B6D4),
        surface = Color(0xFF121418),
        panel = Color(0xFF1E222A),
        panelAlt = Color(0xFF262B35),
        onSurface = Color(0xFFF8FAFC),
        onSurfaceMuted = Color(0xFF94A3B8),
        outline = Color(0xFF334155),
        success = Color(0xFF10B981),
        warning = Color(0xFFF59E0B),
        error = Color(0xFFEF4444),
        nodeEvent = Color(0xFFEF4444),
        nodePhysics = Color(0xFF3B82F6),
        nodeAnim = Color(0xFFA855F7),
        nodeCloud = Color(0xFF10B981),
        codeKeyword = Color(0xFFC084FC),
        codeType = Color(0xFF38BDF8),
        codeString = Color(0xFF4ADE80),
        codeNumber = Color(0xFFFB923C),
        codeComment = Color(0xFF64748B),
        codeError = Color(0xFFF87171)
    )

    val Light = ThemeTokens(
        name = "Light",
        isDark = false,
        version = 1,
        primary = Color(0xFF1D4ED8),
        onPrimary = Color(0xFFFFFFFF),
        secondary = Color(0xFF64748B),
        accent = Color(0xFF0284C7),
        surface = Color(0xFFF8FAFC),
        panel = Color(0xFFFFFFFF),
        panelAlt = Color(0xFFF1F5F9),
        onSurface = Color(0xFF0F172A),
        onSurfaceMuted = Color(0xFF475569),
        outline = Color(0xFFCBD5E1),
        success = Color(0xFF059669),
        warning = Color(0xFFD97706),
        error = Color(0xFFDC2626),
        nodeEvent = Color(0xFFDC2626),
        nodePhysics = Color(0xFF2563EB),
        nodeAnim = Color(0xFF9333EA),
        nodeCloud = Color(0xFF059669),
        codeKeyword = Color(0xFF7E22CE),
        codeType = Color(0xFF0369A1),
        codeString = Color(0xFF15803D),
        codeNumber = Color(0xFFC2410C),
        codeComment = Color(0xFF64748B),
        codeError = Color(0xFFB91C1C)
    )

    val AMOLED = ThemeTokens(
        name = "AMOLED",
        isDark = true,
        version = 1,
        primary = Color(0xFF2563EB), // 5.17:1 vs White
        onPrimary = Color(0xFFFFFFFF),
        secondary = Color(0xFF525252),
        accent = Color(0xFF38BDF8),
        surface = Color(0xFF000000),
        panel = Color(0xFF121212),
        panelAlt = Color(0xFF1E1E1E),
        onSurface = Color(0xFFFFFFFF),
        onSurfaceMuted = Color(0xFFA3A3A3),
        outline = Color(0xFF2E2E2E),
        success = Color(0xFF10B981),
        warning = Color(0xFFF59E0B),
        error = Color(0xFFEF4444),
        nodeEvent = Color(0xFFEF4444),
        nodePhysics = Color(0xFF3B82F6),
        nodeAnim = Color(0xFFA855F7),
        nodeCloud = Color(0xFF10B981),
        codeKeyword = Color(0xFFC084FC),
        codeType = Color(0xFF38BDF8),
        codeString = Color(0xFF4ADE80),
        codeNumber = Color(0xFFFB923C),
        codeComment = Color(0xFF737373),
        codeError = Color(0xFFF87171)
    )

    val Forest = ThemeTokens(
        name = "Forest",
        isDark = true,
        version = 1,
        primary = Color(0xFF047857), // 5.48:1 vs White
        onPrimary = Color(0xFFFFFFFF),
        secondary = Color(0xFF4D7C0F),
        accent = Color(0xFF10B981),
        surface = Color(0xFF0A140F),
        panel = Color(0xFF12231A),
        panelAlt = Color(0xFF1B3326),
        onSurface = Color(0xFFECFDF5),
        onSurfaceMuted = Color(0xFF6EE7B7),
        outline = Color(0xFF064E3B),
        success = Color(0xFF10B981),
        warning = Color(0xFFF59E0B),
        error = Color(0xFFEF4444),
        nodeEvent = Color(0xFFEF4444),
        nodePhysics = Color(0xFF3B82F6),
        nodeAnim = Color(0xFFA855F7),
        nodeCloud = Color(0xFF10B981),
        codeKeyword = Color(0xFFC084FC),
        codeType = Color(0xFF38BDF8),
        codeString = Color(0xFF4ADE80),
        codeNumber = Color(0xFFFB923C),
        codeComment = Color(0xFF6B7280),
        codeError = Color(0xFFF87171)
    )

    val Ocean = ThemeTokens(
        name = "Ocean",
        isDark = true,
        version = 1,
        primary = Color(0xFF0369A1), // 5.93:1 vs White
        onPrimary = Color(0xFFFFFFFF),
        secondary = Color(0xFF0284C7),
        accent = Color(0xFF38BDF8),
        surface = Color(0xFF0A1624),
        panel = Color(0xFF122238),
        panelAlt = Color(0xFF1A304E),
        onSurface = Color(0xFFF0F9FF),
        onSurfaceMuted = Color(0xFF7DD3FC),
        outline = Color(0xFF075985),
        success = Color(0xFF10B981),
        warning = Color(0xFFF59E0B),
        error = Color(0xFFEF4444),
        nodeEvent = Color(0xFFEF4444),
        nodePhysics = Color(0xFF3B82F6),
        nodeAnim = Color(0xFFA855F7),
        nodeCloud = Color(0xFF10B981),
        codeKeyword = Color(0xFFC084FC),
        codeType = Color(0xFF38BDF8),
        codeString = Color(0xFF4ADE80),
        codeNumber = Color(0xFFFB923C),
        codeComment = Color(0xFF64748B),
        codeError = Color(0xFFF87171)
    )

    val AllPresets = listOf(Dark, Light, AMOLED, Forest, Ocean)
}
