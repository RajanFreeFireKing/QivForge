#include "qivforge/core/vault.h"
#include "qivforge/core/secret_scanner.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace qivforge::core;

void TestSecureBufferWiping() {
    std::cout << "[RUN] TestSecureBufferWiping..." << std::endl;

    std::string secret = "hf_SecretToken1234567890abcdef";
    {
        SecureBuffer buf(secret);
        assert(!buf.Empty());
        assert(buf.Size() == secret.size());
        assert(std::string(buf.Data()) == secret);
        buf.Clear();
        assert(buf.Empty());
        assert(buf.Size() == 0);
    }

    std::cout << "[PASS] TestSecureBufferWiping" << std::endl;
}

void TestVaultStorageAndRedaction() {
    std::cout << "[RUN] TestVaultStorageAndRedaction..." << std::endl;

    Vault& vault = Vault::Instance();
    vault.ClearAll();

    assert(!vault.Has(VaultKey::HF_TOKEN));
    assert(!vault.Get(VaultKey::HF_TOKEN).has_value());

    std::string myToken = "hf_FineGrainedTokenQivForge2026Secure";
    vault.Put(VaultKey::HF_TOKEN, myToken);

    assert(vault.Has(VaultKey::HF_TOKEN));
    auto retrieved = vault.Get(VaultKey::HF_TOKEN);
    assert(retrieved.has_value());
    assert(retrieved.value() == myToken);

    // Verify redaction format: "hf_****...cure"
    std::string redacted = Vault::Redact(myToken);
    assert(redacted.find("hf_****...") == 0);
    assert(redacted.find("cure") != std::string::npos);
    assert(redacted.find("FineGrained") == std::string::npos); // Secret body never in redaction!

    vault.Remove(VaultKey::HF_TOKEN);
    assert(!vault.Has(VaultKey::HF_TOKEN));
    assert(!vault.Get(VaultKey::HF_TOKEN).has_value());

    std::cout << "[PASS] TestVaultStorageAndRedaction" << std::endl;
}

void TestSecretScannerPrevention() {
    std::cout << "[RUN] TestSecretScannerPrevention (Doc 14 AC1)..." << std::endl;

    SecretScanner scanner;

    // Test safe text
    std::string safeScript = "var speed = 45.0;\nprint(\"Hello QivForge!\");\n";
    assert(!scanner.ContainsSecrets(safeScript));
    auto findingsSafe = scanner.ScanText(safeScript);
    assert(findingsSafe.empty());

    // Test leaked Hugging Face token in code
    std::string leakedHf = "var token = \"hf_abcdefghijklmnopqrstuvwxyz1234567890\";\n";
    assert(scanner.ContainsSecrets(leakedHf));
    auto findingsHf = scanner.ScanText(leakedHf);
    assert(!findingsHf.empty());
    assert(findingsHf[0].rule == "HuggingFace_Token");
    assert(findingsHf[0].line == 1);

    // Test leaked Google API key
    std::string leakedGoogle = "const apiKey = \"AIzaSyD-1234567890abcdefghijklmnopqrstu\";\n";
    assert(scanner.ContainsSecrets(leakedGoogle));
    auto findingsGoogle = scanner.ScanText(leakedGoogle);
    assert(!findingsGoogle.empty());
    assert(findingsGoogle[0].rule == "Google_API_Key");

    // Test leaked PEM private key header
    std::string leakedPem = "-----BEGIN RSA PRIVATE KEY-----\nMIIEowIBAAKCAQEA...";
    assert(scanner.ContainsSecrets(leakedPem));
    auto findingsPem = scanner.ScanText(leakedPem);
    assert(!findingsPem.empty());
    assert(findingsPem[0].rule == "PrivateKey_Header");

    std::cout << "[PASS] TestSecretScannerPrevention" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  VAULT & SECURITY TEST SUITE (M7)     " << std::endl;
    std::cout << "========================================" << std::endl;

    TestSecureBufferWiping();
    TestVaultStorageAndRedaction();
    TestSecretScannerPrevention();

    std::cout << "ALL VAULT & SECURITY TESTS PASSED!" << std::endl;
    return 0;
}
