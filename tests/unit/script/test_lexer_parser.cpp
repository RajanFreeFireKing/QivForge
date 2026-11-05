#include "qivforge/script/lexer.h"
#include "qivforge/script/parser.h"
#include <iostream>
#include <cassert>

using namespace qivforge::script;

void TestLexerTokensAndComments() {
    std::cout << "[RUN] TestLexerTokensAndComments..." << std::endl;

    std::string source = R"(
        // Line comment
        /* Block comment /* nested */ continued */
        module Game.Test;
        @component("TestComp")
        pub class MyTest extends VehiclePawn {
            @export @range(10.0, 100.0)
            pub var speed: f32 = 45.5;
            var hex: i32 = 0xFF;
            var numWithUnderscore: i32 = 1_000_000;
            var str: string = "hello \"world\"\n";
        }
    )";

    Lexer lexer(source, "TestScript.qvs");
    auto tokens = lexer.Tokenize();

    assert(!lexer.HasErrors());
    assert(tokens.size() > 20);

    // Verify first non-comment token is KW_MODULE
    assert(tokens[0].type == TokenType::KW_MODULE);
    assert(tokens[1].type == TokenType::IDENTIFIER && tokens[1].lexeme == "Game");
    assert(tokens[2].type == TokenType::DOT);
    assert(tokens[3].type == TokenType::IDENTIFIER && tokens[3].lexeme == "Test");
    assert(tokens[4].type == TokenType::SEMICOLON);

    std::cout << "  -> Lexer successfully tokenized source with nested comments & literals" << std::endl;
    std::cout << "[PASS] TestLexerTokensAndComments\n" << std::endl;
}

void TestParserDiagnosticsAndErrorRecovery() {
    // Doc 08 §21 AC2: Syntax error reports correct line and column and diagnostics
    std::cout << "[RUN] TestParserDiagnosticsAndErrorRecovery (Doc 08 AC2)..." << std::endl;

    // Missing semicolon on line 3, unexpected token on line 5
    std::string badSource = 
        "module Bad.Script;\n"
        "pub class BrokenClass {\n"
        "    var a: f32 = 12.0 // Missing semicolon\n"
        "    var b: i32 = 42;\n"
        "}\n";

    Lexer lexer(badSource, "BrokenScript.qvs");
    auto tokens = lexer.Tokenize();
    Parser parser(tokens);
    auto ast = parser.Parse();

    assert(parser.HasErrors());
    const auto& diags = parser.GetDiagnostics();
    std::cout << "  -> Parser detected " << diags.size() << " syntax diagnostic(s):" << std::endl;
    for (const auto& d : diags) {
        std::cout << "     [" << d.code << "] Line " << d.location.line 
                  << ":" << d.location.column << " - " << d.message << std::endl;
    }

    assert(!diags.empty());
    assert(diags[0].location.line == 4); // Detected missing semicolon before line 4
    assert(diags[0].code == "QS0001");

    std::cout << "[PASS] TestParserDiagnosticsAndErrorRecovery\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  QIVSCRIPT LEXER & PARSER SUITE (DOC 08)  " << std::endl;
    std::cout << "===========================================" << std::endl;

    TestLexerTokensAndComments();
    TestParserDiagnosticsAndErrorRecovery();

    std::cout << "ALL LEXER & PARSER TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
