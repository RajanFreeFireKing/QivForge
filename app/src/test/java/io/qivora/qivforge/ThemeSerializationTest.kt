package io.qivora.qivforge

import io.qivora.qivforge.theme.ThemePresets
import io.qivora.qivforge.theme.ThemeTokens
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Test

class ThemeSerializationTest {

    @Test
    fun testJsonRoundTripPreservesTokens() {
        val original = ThemePresets.Forest
        val json = original.toJson()
        assertNotNull(json)

        val parsed = ThemeTokens.fromJson(json, ThemePresets.Dark)
        assertEquals("Forest", parsed.name)
        assertEquals(original.primary, parsed.primary)
        assertEquals(original.surface, parsed.surface)
        assertEquals(original.panel, parsed.panel)
        assertEquals(original.onSurface, parsed.onSurface)
    }

    @Test
    fun testCorruptJsonFallback() {
        val fallback = ThemePresets.AMOLED
        val result = ThemeTokens.fromJson("{ corrupt json invalid }", fallback)
        assertEquals("AMOLED", result.name)
    }
}
