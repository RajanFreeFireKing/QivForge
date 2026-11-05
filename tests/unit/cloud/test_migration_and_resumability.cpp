#include "qivforge/cloud/migration_wizard.h"
#include "qivforge/cloud/hf_client.h"
#include "qivforge/scene/local_store.h"
#include "qivforge/scene/hf_store.h"
#include <iostream>
#include <cassert>

using namespace qivforge::cloud;
using namespace qivforge::scene;
using namespace qivforge::core;

class MockMigrationTransport : public IHttpTransport {
public:
    HttpResponse Request(const std::string& method,
                         const std::string& url,
                         const std::vector<std::pair<std::string, std::string>>& headers,
                         const std::string& body) override {
        (void)method; (void)headers; (void)body;
        HttpResponse resp;

        if (failAtStep && url.find("/commit/") != std::string::npos) {
            resp.statusCode = 500;
            resp.body = "{\"error\":\"Simulated server glitch\"}";
            return resp;
        }

        resp.statusCode = 200;
        if (url.find("/whoami-v2") != std::string::npos) {
            resp.body = "{\"name\":\"test-user\",\"email\":\"test@hf.co\"}";
        } else if (url.find("/tree/") != std::string::npos) {
            resp.body = "[{\"path\":\".gitattributes\",\"type\":\"file\",\"size\":150},"
                        "{\"path\":\"README.md\",\"type\":\"file\",\"size\":80},"
                        "{\"path\":\"scenes/main.scene.json\",\"type\":\"file\",\"size\":512}]";
        } else {
            resp.body = "{}";
        }
        return resp;
    }

    bool failAtStep = false;
};

void TestLocalStoreAtomicWritesAndDiskLimit() {
    std::cout << "[RUN] TestLocalStoreAtomicWritesAndDiskLimit (Doc 11 §2.2 & QF-STORE-001)..." << std::endl;

    auto store = std::make_shared<LocalStore>();

    // Normal write
    auto errWrite = store->WriteAsset("scenes/main.scene.json", "{\"actors\":[]}");
    assert(errWrite == ErrorCode::OK);
    assert(store->HasAsset("scenes/main.scene.json"));

    auto [errRead, data] = store->ReadAsset("scenes/main.scene.json");
    assert(errRead == ErrorCode::OK);
    assert(data == "{\"actors\":[]}");

    // Disk full simulation (< 200 MB free)
    store->SetAvailableDiskSpaceMb(150);
    auto errDiskFull = store->WriteAsset("scenes/heavy.scene.json", "{\"heavy\":true}");
    assert(errDiskFull == ErrorCode::ERR_STORE_DISK_FULL);
    assert(std::string(GetDocErrorCode(errDiskFull)) == "QF-STORE-001");

    std::cout << "[PASS] TestLocalStoreAtomicWritesAndDiskLimit" << std::endl;
}

void TestHfStoreRuleR4ExplicitCloud() {
    std::cout << "[RUN] TestHfStoreRuleR4ExplicitCloud (Rule R4)..." << std::endl;

    HfStore hfStore("", "qivora/my-tractor-game");
    assert(hfStore.IsCloudMode());
    assert(!hfStore.HasPendingChanges());
    assert(hfStore.GetPendingCount() == 0);

    // Save must write locally and mark pending WITHOUT network operations (Rule R4)
    auto err = hfStore.WriteAsset("scenes/tractor.scene.json", "{\"speed\":45}");
    assert(err == ErrorCode::OK);
    assert(hfStore.HasPendingChanges());
    assert(hfStore.GetPendingCount() == 1);
    assert(hfStore.GetPendingFiles()[0] == "scenes/tractor.scene.json");

    // Clear pending
    hfStore.ClearPending();
    assert(!hfStore.HasPendingChanges());

    std::cout << "[PASS] TestHfStoreRuleR4ExplicitCloud" << std::endl;
}

void TestMigrationLocalToCloudAndResume() {
    std::cout << "[RUN] TestMigrationLocalToCloudAndResume (Doc 11 §5 & AC9)..." << std::endl;

    auto transport = std::make_shared<MockMigrationTransport>();
    auto client = std::make_shared<HfClient>(transport);
    client->SetToken("hf_MigrationTokenValid12345");

    auto localStore = std::make_shared<LocalStore>();
    localStore->WriteAsset("scenes/main.scene.json", "{\"scene\":\"data\"}");
    localStore->WriteAsset("scripts/Main.qvs", "print(\"hello\");");

    MigrationWizard wizard(client, localStore);

    // 1. Simulate failure during migration (e.g. at 70%)
    transport->failAtStep = true;
    auto failErr = wizard.MigrateLocalToCloud("qf-test-project", true);
    assert(failErr != ErrorCode::OK);
    // Project must remain in valid Local state
    assert(wizard.GetProgress().currentStep == MigrationStep::FAILED);

    // 2. Resume migration (Doc 11 §5.1 step 3 & AC9)
    transport->failAtStep = false;
    auto resumeErr = wizard.ResumeMigration();
    assert(resumeErr == ErrorCode::OK);
    assert(wizard.GetProgress().currentStep == MigrationStep::COMPLETED);
    assert(wizard.GetProgress().progressFraction == 1.0f);

    // Verify project.qivproj was updated to cloud mode
    auto [errProj, projData] = localStore->ReadAsset("project.qivproj");
    assert(errProj == ErrorCode::OK);
    assert(projData.find("\"mode\":\"cloud\"") != std::string::npos);

    std::cout << "[PASS] TestMigrationLocalToCloudAndResume" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  MIGRATION & RESUMABILITY SUITE (M7)   " << std::endl;
    std::cout << "========================================" << std::endl;

    TestLocalStoreAtomicWritesAndDiskLimit();
    TestHfStoreRuleR4ExplicitCloud();
    TestMigrationLocalToCloudAndResume();

    std::cout << "ALL MIGRATION & RESUMABILITY TESTS PASSED!" << std::endl;
    return 0;
}
