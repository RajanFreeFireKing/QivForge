#include "qivforge/script/lexer.h"
#include "qivforge/script/parser.h"
#include "qivforge/script/compiler.h"
#include "qivforge/script/script_instance.h"
#include <iostream>
#include <cassert>

using namespace qivforge::script;

void TestInspectorFieldUpdateWithoutRestart() {
    // Doc 08 §21 AC5: Modifying exported field values in Inspector updates runtime values
    std::cout << "[RUN] TestInspectorFieldUpdateWithoutRestart (Doc 08 AC5)..." << std::endl;

    std::string source = R"(
        module Game.Props;
        pub class HoverCar {
            @export @range(10.0, 100.0)
            pub var maxSpeed: f32 = 45.0;

            @export
            pub var hoverHeight: f32 = 1.2;
        }
    )";

    Lexer lexer(source);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    auto ast = parser.Parse();
    Compiler compiler(*ast);
    auto module = std::shared_ptr<BytecodeModule>(compiler.Compile().release());
    assert(module != nullptr);

    uint32_t actorId = 1001;
    ScriptInstance instance(actorId, module);

    assert(instance.GetFieldValue("maxSpeed") == 45.0);
    assert(instance.GetFieldValue("hoverHeight") == 1.2);

    // Modify in Inspector
    std::cout << "  -> Modifying 'maxSpeed' to 75.0 and 'hoverHeight' to 2.5 via Inspector..." << std::endl;
    instance.SetFieldValue("maxSpeed", 75.0);
    instance.SetFieldValue("hoverHeight", 2.5);

    assert(instance.GetFieldValue("maxSpeed") == 75.0);
    assert(instance.GetFieldValue("hoverHeight") == 2.5);

    std::cout << "[PASS] TestInspectorFieldUpdateWithoutRestart\n" << std::endl;
}

void TestHotReloadFieldMigration() {
    // Doc 08 §21 AC5: Editing code and hot reloading migrates field values without resetting state
    std::cout << "[RUN] TestHotReloadFieldMigration (Doc 08 AC5)..." << std::endl;

    std::string v1Source = R"(
        module Game.Props;
        pub class Thruster {
            @export
            pub var thrustForce: f32 = 1500.0;

            @export
            pub var fuelCapacity: f32 = 100.0;

            override fn onTick(dt: f32) -> void {
                var x = 1;
            }
        }
    )";

    Lexer lexer1(v1Source);
    auto tokens1 = lexer1.Tokenize();
    Parser parser1(tokens1);
    auto ast1 = parser1.Parse();
    Compiler compiler1(*ast1);
    auto moduleV1 = std::shared_ptr<BytecodeModule>(compiler1.Compile().release());
    assert(moduleV1 != nullptr);

    uint32_t actorId = 55;
    ScriptInstance instance(actorId, moduleV1);

    // Inspector modified thrustForce during gameplay
    instance.SetFieldValue("thrustForce", 3800.0);
    assert(instance.GetFieldValue("thrustForce") == 3800.0);

    // Code edited: added new field 'efficiencyFactor' and kept 'thrustForce', removed 'fuelCapacity'
    std::string v2Source = R"(
        module Game.Props;
        pub class Thruster {
            @export
            pub var thrustForce: f32 = 1500.0;

            @export
            pub var efficiencyFactor: f32 = 0.95;

            override fn onTick(dt: f32) -> void {
                var x = 2;
            }
        }
    )";

    std::cout << "  -> Compiling modified script V2..." << std::endl;
    Lexer lexer2(v2Source);
    auto tokens2 = lexer2.Tokenize();
    Parser parser2(tokens2);
    auto ast2 = parser2.Parse();
    Compiler compiler2(*ast2);
    auto moduleV2 = std::shared_ptr<BytecodeModule>(compiler2.Compile().release());
    assert(moduleV2 != nullptr);

    // Hot reload
    std::cout << "  -> Applying HotReload to active instance..." << std::endl;
    bool reloaded = instance.HotReload(moduleV2);
    assert(reloaded);

    // Existing field must retain user-modified value!
    std::cout << "  -> Checking migrated fields: thrustForce = " << instance.GetFieldValue("thrustForce")
              << " efficiencyFactor = " << instance.GetFieldValue("efficiencyFactor") << std::endl;
    assert(instance.GetFieldValue("thrustForce") == 3800.0);
    // Newly added field must receive its new default value!
    assert(instance.GetFieldValue("efficiencyFactor") == 0.95);

    Qvm vm;
    auto tickRes = instance.OnTick(vm, 0.0166f);
    assert(tickRes.success);

    std::cout << "[PASS] TestHotReloadFieldMigration\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  QIVSCRIPT HOT RELOAD SUITE (DOC 08 AC5)  " << std::endl;
    std::cout << "===========================================" << std::endl;

    TestInspectorFieldUpdateWithoutRestart();
    TestHotReloadFieldMigration();

    std::cout << "ALL HOT RELOAD TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
