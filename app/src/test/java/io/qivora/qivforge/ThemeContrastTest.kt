package io.qivora.qivforge

import androidx.compose.ui.graphics.Color
import io.qivora.qivforge.theme.ContrastGuard
import io.qivora.qivforge.theme.ThemePresets
import org.junit.Assert.assertTrue
import org.junit.Test

class ThemeContrastTest {

    @Test
    fun testAllPresetThemesSatisfyWcagContrast() {
        // Doc 04 AC7: "All screens pass the 4.5:1 contrast check in all five preset themes"
        for (preset in ThemePresets.AllPresets) {
            val surfaceContrast = ContrastGuard.getContrastRatio(preset.onSurface, preset.surface)
            assertTrue(
                "Preset '${preset.name}' onSurface contrast ($surfaceContrast) must be >= 4.5:1",
                surfaceContrast >= 4.5f
            )

            val primaryContrast = ContrastGuard.getContrastRatio(preset.onPrimary, preset.primary)
            assertTrue(
                "Preset '${preset.name}' onPrimary contrast ($primaryContrast) must be >= 4.5:1",
                primaryContrast >= 4.5f
            )
        }
    }

    @Test
    fun testContrastGuardAutoFix() {
        val lowContrastBg = Color(0xFF1E293B)
        val fixedText = ContrastGuard.fixAutomatically(lowContrastBg)
        val fixedRatio = ContrastGuard.getContrastRatio(fixedText, lowContrastBg)

        assertTrue(
            "Auto-fix text contrast ($fixedRatio) must achieve >= 4.5:1",
            fixedRatio >= 4.5f
        )
    }
}
