package io.qivora.qivforge.editor

import org.json.JSONObject
import java.io.File

/**
 * Autosave Journal & Crash Recovery per Rule R7 and Doc 02 §11 / §16 AC5
 */
object JournalManager {

    private const val JOURNAL_DIR_NAME = ".journal"
    private const val JOURNAL_FILE_NAME = "journal.qivj"
    private const val SCENE_FILE_NAME = "scene.qivscene"

    fun getJournalDir(projectDir: File): File {
        val dir = File(projectDir, JOURNAL_DIR_NAME)
        if (!dir.exists()) {
            dir.mkdirs()
        }
        return dir
    }

    fun recordEdit(projectDir: File, sceneJson: String) {
        try {
            val journalFile = File(getJournalDir(projectDir), JOURNAL_FILE_NAME)
            val record = JSONObject().apply {
                put("timestamp", System.currentTimeMillis())
                put("sceneJson", sceneJson)
                put("isCommitted", false)
            }
            journalFile.writeText(record.toString())
        } catch (e: Exception) {
            // Non-fatal
        }
    }

    fun hasCrashRecovery(projectDir: File): Boolean {
        val journalFile = File(getJournalDir(projectDir), JOURNAL_FILE_NAME)
        if (!journalFile.exists()) return false

        return try {
            val record = JSONObject(journalFile.readText())
            val isCommitted = record.optBoolean("isCommitted", true)
            !isCommitted
        } catch (e: Exception) {
            false
        }
    }

    fun recoverScene(projectDir: File): String? {
        val journalFile = File(getJournalDir(projectDir), JOURNAL_FILE_NAME)
        if (!journalFile.exists()) return null

        return try {
            val record = JSONObject(journalFile.readText())
            val sceneJson = record.optString("sceneJson")
            if (sceneJson.isNotBlank()) {
                // Apply recovered scene to project
                File(projectDir, SCENE_FILE_NAME).writeText(sceneJson)
                clearJournal(projectDir)
                sceneJson
            } else {
                null
            }
        } catch (e: Exception) {
            null
        }
    }

    fun clearJournal(projectDir: File) {
        val journalFile = File(getJournalDir(projectDir), JOURNAL_FILE_NAME)
        if (journalFile.exists()) {
            journalFile.delete()
        }
    }
}
