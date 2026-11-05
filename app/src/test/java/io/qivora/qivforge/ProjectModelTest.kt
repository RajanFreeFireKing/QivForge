package io.qivora.qivforge

import io.qivora.qivforge.project.ProjectMetadata
import io.qivora.qivforge.project.ProjectOrientation
import io.qivora.qivforge.project.StorageMode
import io.qivora.qivforge.project.TargetQuality
import io.qivora.qivforge.project.TemplateCatalog
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test

class ProjectModelTest {

    @Test
    fun testProjectMetadataJsonRoundTrip() {
        val meta = ProjectMetadata(
            name = "Desert Racer",
            templateId = "RACING",
            storageMode = StorageMode.LOCAL,
            orientation = ProjectOrientation.LANDSCAPE,
            targetQuality = TargetQuality.HIGH,
            targetFps = 60
        )

        val json = meta.toJson()
        assertNotNull(json)

        val parsed = ProjectMetadata.fromJson(json)
        assertEquals("Desert Racer", parsed.name)
        assertEquals("RACING", parsed.templateId)
        assertEquals(StorageMode.LOCAL, parsed.storageMode)
        assertEquals(ProjectOrientation.LANDSCAPE, parsed.orientation)
        assertEquals(TargetQuality.HIGH, parsed.targetQuality)
        assertEquals(60, parsed.targetFps)
    }

    @Test
    fun testAllFiveTemplatesDefined() {
        // Doc 01 §7 requires 5 starter templates
        assertEquals(5, TemplateCatalog.All.size)
        val templateIds = TemplateCatalog.All.map { it.id }.toSet()
        assertTrue(templateIds.contains("BLANK"))
        assertTrue(templateIds.contains("TRACTOR"))
        assertTrue(templateIds.contains("TPS"))
        assertTrue(templateIds.contains("RACING"))
        assertTrue(templateIds.contains("HUD_PLAYGROUND"))

        // Each template must have non-empty title and valid default scene JSON
        for (template in TemplateCatalog.All) {
            assertTrue(template.title.isNotBlank())
            assertTrue(template.defaultSceneJson.contains("actors"))
        }
    }
}
