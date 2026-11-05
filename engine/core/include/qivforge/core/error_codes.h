#pragma once

#include <cstdint>

namespace qivforge::core {

enum class ErrorCode : int32_t {
    OK = 0,

    // Core & Lifecycle (1000 - 1999)
    ERR_CORE_GENERIC_FAILURE        = 1000,
    ERR_CORE_NOT_INITIALIZED        = 1001,
    ERR_CORE_ALREADY_INITIALIZED    = 1002,
    ERR_CORE_INVALID_ARGUMENT       = 1003,
    ERR_CORE_OUT_OF_MEMORY          = 1004,
    ERR_CORE_TIMEOUT                = 1005,
    ERR_CORE_BUFFER_OVERFLOW        = 1006,
    ERR_CORE_THREAD_VIOLATION       = 1007,

    // Rendering Subsystem (2000 - 2999)
    ERR_RENDER_VULKAN_INIT_FAILED   = 2000,
    ERR_RENDER_GLES_FALLBACK_FAILED = 2001,
    ERR_RENDER_SURFACE_LOST         = 2002,
    ERR_RENDER_SWAPCHAIN_FAILED     = 2003,
    ERR_RENDER_SHADER_COMPILE       = 2004,
    ERR_RENDER_TEXTURE_LOAD_FAILED  = 2005,
    ERR_RENDER_MESH_INVALID         = 2006,

    // Physics Subsystem (3000 - 3999)
    ERR_PHYSICS_INIT_FAILED         = 3000,
    ERR_PHYSICS_BODY_FAILED         = 3001,
    ERR_PHYSICS_COLLIDER_INVALID    = 3002,
    ERR_PHYSICS_STEP_OVERFLOW       = 3003,

    // Scripting & VM (4000 - 4999)
    ERR_SCRIPT_LEXER_ERROR          = 4000,
    ERR_SCRIPT_PARSE_ERROR          = 4001,
    ERR_SCRIPT_BYTECODE_INVALID     = 4002,
    ERR_SCRIPT_GAS_EXCEEDED         = 4003,
    ERR_SCRIPT_STACK_OVERFLOW       = 4004,
    ERR_SCRIPT_HOT_RELOAD_FAILED    = 4005,

    // Storage & Cloud (5000 - 5999)
    ERR_STORAGE_FILE_NOT_FOUND      = 5000,
    ERR_STORAGE_READ_FAILED         = 5001,
    ERR_STORAGE_WRITE_FAILED        = 5002,
    ERR_STORAGE_CORRUPT_JOURNAL     = 5003,
    ERR_CLOUD_AUTH_FAILED           = 5004,
    ERR_CLOUD_NETWORK_ERROR         = 5005,
    ERR_CLOUD_MERGE_CONFLICT        = 5006,
    ERR_CLOUD_TOKEN_INVALID         = 5010, // QF-CLOUD-001
    ERR_CLOUD_NETWORK_UNAVAILABLE   = 5011, // QF-CLOUD-002
    ERR_CLOUD_RATE_LIMITED          = 5012, // QF-CLOUD-003
    ERR_CLOUD_HASH_MISMATCH         = 5013, // QF-CLOUD-004
    ERR_CLOUD_QUOTA_EXCEEDED        = 5014, // QF-CLOUD-005
    ERR_CLOUD_BRANCH_MOVED          = 5015, // QF-CLOUD-006
    ERR_CLOUD_PERMISSION_DENIED     = 5016, // QF-CLOUD-007
    ERR_CLOUD_REPO_NOT_FOUND        = 5017, // QF-CLOUD-008
    ERR_STORE_DISK_FULL             = 5020, // QF-STORE-001
    ERR_STORE_CORRUPT_PROJECT       = 5021, // QF-STORE-002

    // Security & Vault (6000 - 6999)
    ERR_SECURITY_KEYSTORE_FAIL      = 6000,
    ERR_SECURITY_DECRYPT_FAIL       = 6001,
    ERR_SECURITY_PERMISSION_DENIED  = 6002,

    // Build, Export & Packaging (7000 - 7999, Doc 13 §13)
    ERR_BUILD_VALIDATION_FAILED     = 7001, // QF-BUILD-001: Missing asset, script error, etc.
    ERR_BUILD_LICENSE_BLOCKED       = 7002, // QF-BUILD-002: Blocked licenses
    ERR_BUILD_NO_SPACE              = 7003, // QF-BUILD-003: Not enough free space
    ERR_BUILD_SIGNING_FAILED        = 7004, // QF-BUILD-004: Bad keystore or password
    ERR_BUILD_VERIFICATION_FAILED   = 7005, // QF-BUILD-005: Output verification failed
    ERR_BUILD_TEMPLATE_MISMATCH     = 7006, // QF-BUILD-006: Template version mismatch
    ERR_BUILD_PRIVATE_STREAM_BLOCKED= 7007, // QF-BUILD-007: Private repo streaming blocked
    ERR_BUILD_SECRET_DETECTED       = 7008, // QF-BUILD-008: Embedded secret detected in project files
    ERR_BUILD_CLOUD_FAILED          = 7009  // QF-BUILD-009: Cloud build failed or timed out
};

[[nodiscard]] constexpr const char* GetErrorMessage(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::OK: return "Success";
        case ErrorCode::ERR_CORE_NOT_INITIALIZED: return "Engine core not initialized";
        case ErrorCode::ERR_CORE_ALREADY_INITIALIZED: return "Engine core already initialized";
        case ErrorCode::ERR_CORE_INVALID_ARGUMENT: return "Invalid argument provided";
        case ErrorCode::ERR_CORE_OUT_OF_MEMORY: return "Out of memory";
        case ErrorCode::ERR_CORE_TIMEOUT: return "Operation timed out";
        case ErrorCode::ERR_CORE_BUFFER_OVERFLOW: return "Shared ring buffer overflow";
        case ErrorCode::ERR_CORE_THREAD_VIOLATION: return "Called on incorrect thread";

        case ErrorCode::ERR_RENDER_VULKAN_INIT_FAILED: return "Vulkan 1.1 initialization failed";
        case ErrorCode::ERR_RENDER_GLES_FALLBACK_FAILED: return "OpenGL ES 3.1 fallback failed";
        case ErrorCode::ERR_RENDER_SURFACE_LOST: return "Render surface was lost";
        case ErrorCode::ERR_RENDER_SWAPCHAIN_FAILED: return "Swapchain creation failed";
        case ErrorCode::ERR_RENDER_SHADER_COMPILE: return "Filament shader compile error";
        case ErrorCode::ERR_RENDER_TEXTURE_LOAD_FAILED: return "Texture transcode/load failed";
        case ErrorCode::ERR_RENDER_MESH_INVALID: return "Mesh geometry invalid or corrupt";

        case ErrorCode::ERR_PHYSICS_INIT_FAILED: return "Jolt physics initialization failed";
        case ErrorCode::ERR_PHYSICS_BODY_FAILED: return "Rigid body creation failed";
        case ErrorCode::ERR_PHYSICS_COLLIDER_INVALID: return "Collider geometry invalid";
        case ErrorCode::ERR_PHYSICS_STEP_OVERFLOW: return "Physics accumulator exceeded step limit";

        case ErrorCode::ERR_SCRIPT_LEXER_ERROR: return "QivScript lexical analysis error";
        case ErrorCode::ERR_SCRIPT_PARSE_ERROR: return "QivScript syntax error";
        case ErrorCode::ERR_SCRIPT_BYTECODE_INVALID: return "QVM bytecode corrupted or incompatible";
        case ErrorCode::ERR_SCRIPT_GAS_EXCEEDED: return "QVM instruction budget exceeded";
        case ErrorCode::ERR_SCRIPT_STACK_OVERFLOW: return "QVM execution stack overflow";
        case ErrorCode::ERR_SCRIPT_HOT_RELOAD_FAILED: return "Script state preserved reload failed";

        case ErrorCode::ERR_STORAGE_FILE_NOT_FOUND: return "Project or scene file not found";
        case ErrorCode::ERR_STORAGE_READ_FAILED: return "Failed to read project storage";
        case ErrorCode::ERR_STORAGE_WRITE_FAILED: return "Failed to write project storage";
        case ErrorCode::ERR_STORAGE_CORRUPT_JOURNAL: return "Autosave journal corrupt or truncated";
        case ErrorCode::ERR_CLOUD_AUTH_FAILED: return "Hugging Face token invalid or expired";
        case ErrorCode::ERR_CLOUD_NETWORK_ERROR: return "Hugging Face network operation failed";
        case ErrorCode::ERR_CLOUD_MERGE_CONFLICT: return "Three-way semantic merge conflict";
        case ErrorCode::ERR_CLOUD_TOKEN_INVALID: return "Hugging Face token invalid or expired (QF-CLOUD-001)";
        case ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE: return "Network unavailable (QF-CLOUD-002)";
        case ErrorCode::ERR_CLOUD_RATE_LIMITED: return "Hugging Face API rate limited or service unavailable (QF-CLOUD-003)";
        case ErrorCode::ERR_CLOUD_HASH_MISMATCH: return "Downloaded asset SHA-256 hash mismatch (QF-CLOUD-004)";
        case ErrorCode::ERR_CLOUD_QUOTA_EXCEEDED: return "Repository quota exceeded or file too large (QF-CLOUD-005)";
        case ErrorCode::ERR_CLOUD_BRANCH_MOVED: return "Remote branch head moved, sync required (QF-CLOUD-006)";
        case ErrorCode::ERR_CLOUD_PERMISSION_DENIED: return "Token lacks write permission on repository (QF-CLOUD-007)";
        case ErrorCode::ERR_CLOUD_REPO_NOT_FOUND: return "Hugging Face repository not found or renamed (QF-CLOUD-008)";
        case ErrorCode::ERR_STORE_DISK_FULL: return "Local storage disk full (QF-STORE-001)";
        case ErrorCode::ERR_STORE_CORRUPT_PROJECT: return "Project manifest or scene file corrupt (QF-STORE-002)";

        case ErrorCode::ERR_SECURITY_KEYSTORE_FAIL: return "Android Keystore hardware unavailable";
        case ErrorCode::ERR_SECURITY_DECRYPT_FAIL: return "Tink decryption failed";
        case ErrorCode::ERR_SECURITY_PERMISSION_DENIED: return "Android permission denied";

        case ErrorCode::ERR_BUILD_VALIDATION_FAILED: return "Validation failure (QF-BUILD-001)";
        case ErrorCode::ERR_BUILD_LICENSE_BLOCKED: return "Blocked asset licenses detected (QF-BUILD-002)";
        case ErrorCode::ERR_BUILD_NO_SPACE: return "Insufficient disk space for export (QF-BUILD-003)";
        case ErrorCode::ERR_BUILD_SIGNING_FAILED: return "APK signing failed (QF-BUILD-004)";
        case ErrorCode::ERR_BUILD_VERIFICATION_FAILED: return "APK verification failed (QF-BUILD-005)";
        case ErrorCode::ERR_BUILD_TEMPLATE_MISMATCH: return "Template engine version mismatch (QF-BUILD-006)";
        case ErrorCode::ERR_BUILD_PRIVATE_STREAM_BLOCKED: return "Private repository streaming blocked in APK (QF-BUILD-007)";
        case ErrorCode::ERR_BUILD_SECRET_DETECTED: return "Embedded secret detected in project (QF-BUILD-008)";
        case ErrorCode::ERR_BUILD_CLOUD_FAILED: return "Cloud build failed or timed out (QF-BUILD-009)";

        default: return "Unknown engine error";
    }
}

[[nodiscard]] constexpr const char* GetDocErrorCode(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::ERR_CLOUD_TOKEN_INVALID: return "QF-CLOUD-001";
        case ErrorCode::ERR_CLOUD_NETWORK_UNAVAILABLE: return "QF-CLOUD-002";
        case ErrorCode::ERR_CLOUD_RATE_LIMITED: return "QF-CLOUD-003";
        case ErrorCode::ERR_CLOUD_HASH_MISMATCH: return "QF-CLOUD-004";
        case ErrorCode::ERR_CLOUD_QUOTA_EXCEEDED: return "QF-CLOUD-005";
        case ErrorCode::ERR_CLOUD_BRANCH_MOVED: return "QF-CLOUD-006";
        case ErrorCode::ERR_CLOUD_PERMISSION_DENIED: return "QF-CLOUD-007";
        case ErrorCode::ERR_CLOUD_REPO_NOT_FOUND: return "QF-CLOUD-008";
        case ErrorCode::ERR_STORE_DISK_FULL: return "QF-STORE-001";
        case ErrorCode::ERR_STORE_CORRUPT_PROJECT: return "QF-STORE-002";
        case ErrorCode::ERR_BUILD_VALIDATION_FAILED: return "QF-BUILD-001";
        case ErrorCode::ERR_BUILD_LICENSE_BLOCKED: return "QF-BUILD-002";
        case ErrorCode::ERR_BUILD_NO_SPACE: return "QF-BUILD-003";
        case ErrorCode::ERR_BUILD_SIGNING_FAILED: return "QF-BUILD-004";
        case ErrorCode::ERR_BUILD_VERIFICATION_FAILED: return "QF-BUILD-005";
        case ErrorCode::ERR_BUILD_TEMPLATE_MISMATCH: return "QF-BUILD-006";
        case ErrorCode::ERR_BUILD_PRIVATE_STREAM_BLOCKED: return "QF-BUILD-007";
        case ErrorCode::ERR_BUILD_SECRET_DETECTED: return "QF-BUILD-008";
        case ErrorCode::ERR_BUILD_CLOUD_FAILED: return "QF-BUILD-009";
        default: return "QF-GENERIC";
    }
}

} // namespace qivforge::core
