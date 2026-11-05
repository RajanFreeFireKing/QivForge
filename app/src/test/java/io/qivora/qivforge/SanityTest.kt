package io.qivora.qivforge

import io.qivora.qivforge.core.ErrorCode
import io.qivora.qivforge.engine.NativeBridge
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Test

class SanityTest {

    @Test
    fun testErrorCodeMappings() {
        assertEquals(0, ErrorCode.OK.code)
        assertEquals("Success", ErrorCode.OK.description)

        assertEquals(1001, ErrorCode.ERR_CORE_NOT_INITIALIZED.code)
        assertEquals(2000, ErrorCode.ERR_RENDER_VULKAN_INIT_FAILED.code)
        assertEquals(3000, ErrorCode.ERR_PHYSICS_INIT_FAILED.code)
        assertEquals(4000, ErrorCode.ERR_SCRIPT_LEXER_ERROR.code)
        assertEquals(5000, ErrorCode.ERR_STORAGE_FILE_NOT_FOUND.code)
        assertEquals(6000, ErrorCode.ERR_SECURITY_KEYSTORE_FAIL.code)

        // Test fromCode resolution
        assertEquals(ErrorCode.OK, ErrorCode.fromCode(0))
        assertEquals(ErrorCode.ERR_RENDER_SURFACE_LOST, ErrorCode.fromCode(2002))
        assertEquals(ErrorCode.ERR_CORE_GENERIC_FAILURE, ErrorCode.fromCode(9999))
    }

    @Test
    fun testNativeBridgeHostSafety() {
        // On host JVM unit test without native lib loaded, methods should return safe fallback error
        if (!NativeBridge.isNativeLoaded()) {
            val res = NativeBridge.init()
            assertEquals(ErrorCode.ERR_CORE_NOT_INITIALIZED, res)
            assertEquals(0.0f, NativeBridge.getFps(), 0.001f)
            assertEquals(0.0f, NativeBridge.getFrameMs(), 0.001f)
        }
    }
}
