package io.qivora.qivforge.project

import android.content.Context
import java.io.File
import java.util.UUID

object ProjectManager {

    private const val PROJECTS_DIR_NAME = "projects"
    const val METADATA_FILE_NAME = "project.qivproj"
    const val SCENE_FILE_NAME = "scene.qivscene"

    fun getProjectsDir(context: Context): File {
        val dir = File(context.filesDir, PROJECTS_DIR_NAME)
        if (!dir.exists()) {
            dir.mkdirs()
        }
        return dir
    }

    fun listProjects(context: Context): List<ProjectMetadata> {
        val root = getProjectsDir(context)
        val result = mutableListOf<ProjectMetadata>()

        root.listFiles()?.forEach { dir ->
            if (dir.isDirectory) {
                val metaFile = File(dir, METADATA_FILE_NAME)
                if (metaFile.exists()) {
                    try {
                        val meta = ProjectMetadata.fromJson(metaFile.readText())
                        // Calculate directory size
                        val size = dir.walkTopDown().filter { it.isFile }.map { it.length() }.sum()
                        result.add(meta.copy(sizeBytes = size))
                    } catch (e: Exception) {
                        // Skip corrupted metadata
                    }
                }
            }
        }

        return result.sortedByDescending { it.lastModifiedAt }
    }

    fun createProject(
        context: Context,
        name: String,
        templateId: String = "BLANK",
        storageMode: StorageMode = StorageMode.LOCAL,
        orientation: ProjectOrientation = ProjectOrientation.LANDSCAPE,
        targetQuality: TargetQuality = TargetQuality.AUTO
    ): Result<ProjectMetadata> {
        val trimmed = name.trim()
        if (trimmed.length < 3 || trimmed.length > 40) {
            return Result.failure(IllegalArgumentException("Project name must be between 3 and 40 characters"))
        }

        val existing = listProjects(context)
        if (existing.any { it.name.equals(trimmed, ignoreCase = true) }) {
            return Result.failure(IllegalArgumentException("A project named '$trimmed' already exists"))
        }

        val id = UUID.randomUUID().toString()
        val projectDir = File(getProjectsDir(context), id)
        projectDir.mkdirs()

        // Create assets and cache directories
        File(projectDir, "assets").mkdirs()
        File(projectDir, "cache").mkdirs()

        val template = TemplateCatalog.All.firstOrNull { it.id == templateId } ?: TemplateCatalog.Blank

        val meta = ProjectMetadata(
            id = id,
            name = trimmed,
            templateId = template.id,
            storageMode = storageMode,
            orientation = orientation,
            targetQuality = targetQuality,
            targetFps = 60,
            createdAt = System.currentTimeMillis(),
            lastModifiedAt = System.currentTimeMillis()
        )

        // Write project.qivproj
        File(projectDir, METADATA_FILE_NAME).writeText(meta.toJson())

        // Write initial scene file
        File(projectDir, SCENE_FILE_NAME).writeText(template.defaultSceneJson)

        return Result.success(meta)
    }

    fun renameProject(context: Context, id: String, newName: String): Result<ProjectMetadata> {
        val trimmed = newName.trim()
        if (trimmed.length < 3 || trimmed.length > 40) {
            return Result.failure(IllegalArgumentException("Project name must be between 3 and 40 characters"))
        }

        val projectDir = File(getProjectsDir(context), id)
        val metaFile = File(projectDir, METADATA_FILE_NAME)
        if (!metaFile.exists()) {
            return Result.failure(IllegalStateException("Project not found"))
        }

        val meta = ProjectMetadata.fromJson(metaFile.readText())
        val updated = meta.copy(name = trimmed, lastModifiedAt = System.currentTimeMillis())
        metaFile.writeText(updated.toJson())

        return Result.success(updated)
    }

    fun duplicateProject(context: Context, id: String): Result<ProjectMetadata> {
        val srcDir = File(getProjectsDir(context), id)
        val metaFile = File(srcDir, METADATA_FILE_NAME)
        if (!metaFile.exists()) {
            return Result.failure(IllegalStateException("Project not found"))
        }

        val meta = ProjectMetadata.fromJson(metaFile.readText())
        val newId = UUID.randomUUID().toString()
        val destDir = File(getProjectsDir(context), newId)

        srcDir.copyRecursively(destDir, overwrite = true)

        val updated = meta.copy(
            id = newId,
            name = "${meta.name} (Copy)",
            createdAt = System.currentTimeMillis(),
            lastModifiedAt = System.currentTimeMillis()
        )

        File(destDir, METADATA_FILE_NAME).writeText(updated.toJson())
        return Result.success(updated)
    }

    fun deleteProject(context: Context, id: String): Boolean {
        val projectDir = File(getProjectsDir(context), id)
        return projectDir.deleteRecursively()
    }
}
