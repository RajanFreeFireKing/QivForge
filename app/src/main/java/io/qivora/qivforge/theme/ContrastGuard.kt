package io.qivora.qivforge.theme

import androidx.compose.ui.graphics.Color
import kotlin.math.max
import kotlin.math.min
import kotlin.math.pow

/**
 * WCAG 2.1 Contrast Guard & Calculator per Doc 04 §2.2 / §14 AC7
 */
object ContrastGuard {

    private const val MIN_CONTRAST_RATIO = 4.5f

    /**
     * Calculates the relative luminance of a Color according to WCAG 2.1
     */
    fun getLuminance(color: Color): Float {
        fun channelLuminance(c: Float): Float {
            return if (c <= 0.03928f) {
                c / 12.92f
            } else {
                ((c + 0.055f) / 1.055f).pow(2.4f)
            }
        }

        val r = channelLuminance(color.red)
        val g = channelLuminance(color.green)
        val b = channelLuminance(color.blue)

        return 0.2126f * r + 0.7152f * g + 0.0722f * b
    }

    /**
     * Calculates contrast ratio between foreground and background. Range is 1.0 to 21.0.
     */
    fun getContrastRatio(foreground: Color, background: Color): Float {
        val l1 = getLuminance(foreground)
        val l2 = getLuminance(background)

        val lighter = max(l1, l2)
        val darker = min(l1, l2)

        return (lighter + 0.05f) / (darker + 0.05f)
    }

    /**
     * Returns true if contrast meets or exceeds WCAG AA standard (4.5:1)
     */
    fun isAccessible(foreground: Color, background: Color): Boolean {
        return getContrastRatio(foreground, background) >= MIN_CONTRAST_RATIO
    }

    /**
     * Suggests an automatic fix for text color to guarantee >= 4.5:1 contrast
     */
    fun fixAutomatically(background: Color): Color {
        val whiteContrast = getContrastRatio(Color.White, background)
        val blackContrast = getContrastRatio(Color(0xFF121212), background)

        return if (whiteContrast >= blackContrast) {
            Color.White
        } else {
            Color(0xFF121212)
        }
    }
}
