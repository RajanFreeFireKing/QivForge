package io.qivora.qivforge.security

import io.qivora.qivforge.core.ErrorCode
import java.util.Arrays

/**
 * Vault Key Identifiers per Doc 14 §2.2
 */
enum class VaultKey {
    HF_TOKEN,
    HF_REFRESH,
    GEMINI_KEY,
    GROQ_KEY,
    OPENROUTER_KEY,
    HF_INFERENCE_KEY,
    KEYSTORE_PASSWORD
}

/**
 * Vault Interface per Doc 14 §2.2
 * Manages encrypted secrets backed by Keystore with strict memory wiping.
 */
interface Vault {
    suspend fun put(name: VaultKey, secret: CharArray)
    suspend fun get(name: VaultKey): CharArray?     // caller must zero the array after use
    suspend fun remove(name: VaultKey)
    suspend fun clearAll()
    fun hasKey(name: VaultKey): Boolean
}

/**
 * In-memory secure Vault implementation with automated memory wiping and masking
 */
class SecureVault : Vault {
    private val store = mutableMapOf<VaultKey, CharArray>()

    override suspend fun put(name: VaultKey, secret: CharArray) {
        // Zero existing if present
        store[name]?.let { Arrays.fill(it, '\u0000') }
        store[name] = secret.clone()
    }

    override suspend fun get(name: VaultKey): CharArray? {
        return store[name]?.clone()
    }

    override suspend fun remove(name: VaultKey) {
        store[name]?.let { Arrays.fill(it, '\u0000') }
        store.remove(name)
    }

    override suspend fun clearAll() {
        for ((_, chars) in store) {
            Arrays.fill(chars, '\u0000')
        }
        store.clear()
    }

    override fun hasKey(name: VaultKey): Boolean {
        return store.containsKey(name)
    }

    companion object {
        val instance: Vault by lazy { SecureVault() }

        /**
         * Redacts secret tokens for logging or safe display (Doc 14 §2.3)
         */
        fun redact(secret: String): String {
            if (secret.isEmpty()) return "<empty>"
            if (secret.length <= 8) return "********"
            val prefix = secret.take(3)
            val suffix = secret.takeLast(4)
            return "$prefix****...$suffix"
        }
    }
}
