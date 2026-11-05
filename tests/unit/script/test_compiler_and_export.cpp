#include "qivforge/script/lexer.h"
#include "qivforge/script/parser.h"
#include "qivforge/script/compiler.h"
#include <iostream>
#include <cassert>

using namespace qivforge::script;

void TestExportedFieldMetadataExtraction() {
    // Doc 08 §21 AC3: @export and @range fields appear in Inspector metadata
    std::cout << "[RUN] TestExportedFieldMetadataExtraction (Doc 08 AC3)..." << std::endl;

    std::string source = R"(
        module Game.Vehicles;

        @component("Tractor Settings")
        pub class TractorSettings {
            @header("Powertrain")
            @export @range(800.0, 5000.0) @step(50.0) @tooltip("Engine torque output in Nm")
            pub var torqueNm: f32 = 3200.0;

            @header("Handling")
            @export @range(10.0, 45.0)
            pub var maxSteer: f32 = 38.0;

            @export
            pub var isAllWheelDrive: bool = true;

            private var internalCounter: i32 = 0; // Not exported
        }
    )";

    Lexer lexer(source);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    auto ast = parser.Parse();

    assert(!parser.HasErrors());
    assert(!ast->classes.empty());

    Compiler compiler(*ast);
    auto module = compiler.Compile();

    assert(!compiler.HasErrors());
    assert(module != nullptr);

    const auto& fields = module->exportedFields;
    std::cout << "  -> Extracted " << fields.size() << " exported field(s):" << std::endl;
    for (const auto& f : fields) {
        std::cout << "     Field: " << f.name << " (" << f.typeName << ")"
                  << " Range: [" << f.rangeMin << ", " << f.rangeMax << "]"
                  << " Header: \"" << f.header << "\" Default: " << f.defaultValue << std::endl;
    }

    assert(fields.size() == 3); // torqueNm, maxSteer, isAllWheelDrive (internalCounter is private & not exported)

    assert(fields[0].name == "torqueNm");
    assert(fields[0].hasRange);
    assert(fields[0].rangeMin == 800.0 && fields[0].rangeMax == 5000.0);
    assert(fields[0].header == "Powertrain");
    assert(fields[0].tooltip == "Engine torque output in Nm");
    assert(fields[0].defaultValue == 3200.0);

    assert(fields[1].name == "maxSteer");
    assert(fields[1].rangeMin == 10.0 && fields[1].rangeMax == 45.0);

    std::cout << "[PASS] TestExportedFieldMetadataExtraction\n" << std::endl;
}

void TestDecoratorValidationErrors() {
    // Doc 08 §14.1 QS0031: @range on non-numeric field is a compile error
    std::cout << "[RUN] TestDecoratorValidationErrors (Doc 08 §14.1 QS0031)..." << std::endl;

    std::string badSource = R"(
        module Game.Errors;
        pub class BadClass {
            @export @range(1.0, 10.0)
            pub var tag: string = "player"; // ERROR: @range on string!
        }
    )";

    Lexer lexer(badSource);
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    auto ast = parser.Parse();

    Compiler compiler(*ast);
    auto module = compiler.Compile();

    assert(compiler.HasErrors());
    const auto& diags = compiler.GetDiagnostics();
    std::cout << "  -> Compiler correctly caught semantic error: [" 
              << diags[0].code << "] " << diags[0].message << std::endl;
    assert(diags[0].code == "QS0031");

    std::cout << "[PASS] TestDecoratorValidationErrors\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  QIVSCRIPT COMPILER & EXPORT SUITE        " << std::endl;
    std::cout << "===========================================" << std::endl;

    TestExportedFieldMetadataExtraction();
    TestDecoratorValidationErrors();

    std::cout << "ALL COMPILER & EXPORT TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
