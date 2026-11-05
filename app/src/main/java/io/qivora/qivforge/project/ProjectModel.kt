package io.qivora.qivforge.project

import org.json.JSONObject
import java.util.UUID

enum class StorageMode {
    LOCAL,
    CLOUD
}

enum class ProjectOrientation {
    LANDSCAPE,
    PORTRAIT
}

enum class TargetQuality {
    LOW,
    MEDIUM,
    HIGH,
    AUTO
}

data class ProjectMetadata(
    val id: String = UUID.randomUUID().toString(),
    val name: String,
    val templateId: String,
    val storageMode: StorageMode = StorageMode.LOCAL,
    val orientation: ProjectOrientation = ProjectOrientation.LANDSCAPE,
    val targetQuality: TargetQuality = TargetQuality.AUTO,
    val targetFps: Int = 60,
    val engineVersion: String = "0.1.0-alpha",
    val createdAt: Long = System.currentTimeMillis(),
    val lastModifiedAt: Long = System.currentTimeMillis(),
    val sizeBytes: Long = 0L,
    val thumbnailPath: String? = null
) {
    fun toJson(): String {
        val obj = JSONObject()
        obj.put("id", id)
        obj.put("name", name)
        obj.put("templateId", templateId)
        obj.put("storageMode", storageMode.name)
        obj.put("orientation", orientation.name)
        obj.put("targetQuality", targetQuality.name)
        obj.put("targetFps", targetFps)
        obj.put("engineVersion", engineVersion)
        obj.put("createdAt", createdAt)
        obj.put("lastModifiedAt", lastModifiedAt)
        obj.put("sizeBytes", sizeBytes)
        if (thumbnailPath != null) {
            obj.put("thumbnailPath", thumbnailPath)
        }
        return obj.toString(2)
    }

    companion object {
        fun fromJson(jsonStr: String): ProjectMetadata {
            val obj = JSONObject(jsonStr)
            return ProjectMetadata(
                id = obj.getString("id"),
                name = obj.getString("name"),
                templateId = obj.optString("templateId", "BLANK"),
                storageMode = StorageMode.valueOf(obj.optString("storageMode", "LOCAL")),
                orientation = ProjectOrientation.valueOf(obj.optString("orientation", "LANDSCAPE")),
                targetQuality = TargetQuality.valueOf(obj.optString("targetQuality", "AUTO")),
                targetFps = obj.optInt("targetFps", 60),
                engineVersion = obj.optString("engineVersion", "0.1.0-alpha"),
                createdAt = obj.optLong("createdAt", System.currentTimeMillis()),
                lastModifiedAt = obj.optLong("lastModifiedAt", System.currentTimeMillis()),
                sizeBytes = obj.optLong("sizeBytes", 0L),
                thumbnailPath = if (obj.has("thumbnailPath")) obj.getString("thumbnailPath") else null
            )
        }
    }
}
