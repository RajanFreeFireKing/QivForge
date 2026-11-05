package io.qivora.qivforge.core

/**
 * Standard QivForge Error Codes mirror of C++ error_codes.h (Doc 02/15)
 */
enum class ErrorCode(val code: Int, val description: String) {
    OK(0, "Success"),

    // Core & Lifecycle (1000 - 1999)
    ERR_CORE_GENERIC_FAILURE(1000, "Generic failure"),
    ERR_CORE_NOT_INITIALIZED(1001, "Engine core not initialized"),
    ERR_CORE_ALREADY_INITIALIZED(1002, "Engine core already initialized"),
    ERR_CORE_INVALID_ARGUMENT(1003, "Invalid argument provided"),
    ERR_CORE_OUT_OF_MEMORY(1004, "Out of memory"),
    ERR_CORE_TIMEOUT(1005, "Operation timed out"),
    ERR_CORE_BUFFER_OVERFLOW(1006, "Shared ring buffer overflow"),
    ERR_CORE_THREAD_VIOLATION(1007, "Called on incorrect thread"),

    // Rendering Subsystem (2000 - 2999)
    ERR_RENDER_VULKAN_INIT_FAILED(2000, "Vulkan 1.1 initialization failed"),
    ERR_RENDER_GLES_FALLBACK_FAILED(2001, "OpenGL ES 3.1 fallback failed"),
    ERR_RENDER_SURFACE_LOST(2002, "Render surface was lost"),
    ERR_RENDER_SWAPCHAIN_FAILED(2003, "Swapchain creation failed"),
    ERR_RENDER_SHADER_COMPILE(2004, "Filament shader compile error"),
    ERR_RENDER_TEXTURE_LOAD_FAILED(2005, "Texture transcode/load failed"),
    ERR_RENDER_MESH_INVALID(2006, "Mesh geometry invalid or corrupt"),

    // Physics Subsystem (3000 - 3999)
    ERR_PHYSICS_INIT_FAILED(3000, "Jolt physics initialization failed"),
    ERR_PHYSICS_BODY_FAILED(3001, "Rigid body creation failed"),
    ERR_PHYSICS_COLLIDER_INVALID(3002, "Collider geometry invalid"),
    ERR_PHYSICS_STEP_OVERFLOW(3003, "Physics accumulator exceeded step limit"),

    // Scripting & VM (4000 - 4999)
    ERR_SCRIPT_LEXER_ERROR(4000, "QivScript lexical analysis error"),
    ERR_SCRIPT_PARSE_ERROR(4001, "QivScript syntax error"),
    ERR_SCRIPT_BYTECODE_INVALID(4002, "QVM bytecode corrupted or incompatible"),
    ERR_SCRIPT_GAS_EXCEEDED(4003, "QVM instruction budget exceeded"),
    ERR_SCRIPT_STACK_OVERFLOW(4004, "QVM execution stack overflow"),
    ERR_SCRIPT_HOT_RELOAD_FAILED(4005, "Script state preserved reload failed"),

    // Storage & Cloud (5000 - 5999)
    ERR_STORAGE_FILE_NOT_FOUND(5000, "Project or scene file not found"),
    ERR_STORAGE_READ_FAILED(5001, "Failed to read project storage"),
    ERR_STORAGE_WRITE_FAILED(5002, "Failed to write project storage"),
    ERR_STORAGE_CORRUPT_JOURNAL(5003, "Autosave journal corrupt or truncated"),
    ERR_CLOUD_AUTH_FAILED(5004, "Hugging Face token invalid or expired"),
    ERR_CLOUD_NETWORK_ERROR(5005, "Hugging Face network operation failed"),
    ERR_CLOUD_MERGE_CONFLICT(5006, "Three-way semantic merge conflict"),
    ERR_CLOUD_TOKEN_INVALID(5010, "Hugging Face token invalid or expired (QF-CLOUD-001)"),
    ERR_CLOUD_NETWORK_UNAVAILABLE(5011, "Network unavailable (QF-CLOUD-002)"),
    ERR_CLOUD_RATE_LIMITED(5012, "Hugging Face API rate limited or service unavailable (QF-CLOUD-003)"),
    ERR_CLOUD_HASH_MISMATCH(5013, "Downloaded asset SHA-256 hash mismatch (QF-CLOUD-004)"),
    ERR_CLOUD_QUOTA_EXCEEDED(5014, "Repository quota exceeded or file too large (QF-CLOUD-005)"),
    ERR_CLOUD_BRANCH_MOVED(5015, "Remote branch head moved, sync required (QF-CLOUD-006)"),
    ERR_CLOUD_PERMISSION_DENIED(5016, "Token lacks write permission on repository (QF-CLOUD-007)"),
    ERR_CLOUD_REPO_NOT_FOUND(5017, "Hugging Face repository not found or renamed (QF-CLOUD-008)"),
    ERR_STORE_DISK_FULL(5020, "Local storage disk full (QF-STORE-001)"),
    ERR_STORE_CORRUPT_PROJECT(5021, "Project manifest or scene file corrupt (QF-STORE-002)"),

    // Security & Vault (6000 - 6999)
    ERR_SECURITY_KEYSTORE_FAIL(6000, "Android Keystore hardware unavailable"),
    ERR_SECURITY_DECRYPT_FAIL(6001, "Tink decryption failed"),
    ERR_SECURITY_PERMISSION_DENIED(6002, "Android permission denied");

    val docCode: String
        get() = when (this) {
            ERR_CLOUD_TOKEN_INVALID -> "QF-CLOUD-001"
            ERR_CLOUD_NETWORK_UNAVAILABLE -> "QF-CLOUD-002"
            ERR_CLOUD_RATE_LIMITED -> "QF-CLOUD-003"
            ERR_CLOUD_HASH_MISMATCH -> "QF-CLOUD-004"
            ERR_CLOUD_QUOTA_EXCEEDED -> "QF-CLOUD-005"
            ERR_CLOUD_BRANCH_MOVED -> "QF-CLOUD-006"
            ERR_CLOUD_PERMISSION_DENIED -> "QF-CLOUD-007"
            ERR_CLOUD_REPO_NOT_FOUND -> "QF-CLOUD-008"
            ERR_STORE_DISK_FULL -> "QF-STORE-001"
            ERR_STORE_CORRUPT_PROJECT -> "QF-STORE-002"
            else -> "QF-GENERIC"
        }

    companion object {
        fun fromCode(code: Int): ErrorCode =
            values().firstOrNull { it.code == code } ?: ERR_CORE_GENERIC_FAILURE
    }
}
