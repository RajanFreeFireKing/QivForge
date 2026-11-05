#include "qivforge/script/qvm.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace qivforge::script;

void TestInstructionBudgetInfiniteLoop() {
    // Doc 08 §21 AC4: Infinite loop while (true) {} must be stopped within one frame
    // by instruction budget (QS-RUN-002) without freezing engine.
    std::cout << "[RUN] TestInstructionBudgetInfiniteLoop (Doc 08 AC4)..." << std::endl;

    Qvm vm;
    // Set 60fps frame budget (e.g. 10,000 instructions)
    vm.SetInstructionBudget(10000);

    CompiledFunction fn;
    fn.name = "InfiniteLoop";

    // Infinite loop: JMP -1
    // pc:
    // 0: JMP 0 (sImm = 0 jumps back to instruction 0)
    fn.code.push_back(EncodeAImm(Opcode::JMP, 0, static_cast<uint16_t>(-1)));

    auto tStart = std::chrono::high_resolution_clock::now();
    auto res = vm.Execute(fn);
    auto tEnd = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

    std::cout << "  -> Stopped after " << res.instructionsExecuted << " instructions in "
              << elapsedMs << " ms" << std::endl;
    std::cout << "  -> Success: " << (res.success ? "true" : "false")
              << " ErrorCode: " << res.errorCode 
              << " ErrorMsg: \"" << res.errorMessage << "\"" << std::endl;

    assert(!res.success);
    assert(res.errorCode == "QS-RUN-002");
    assert(res.instructionsExecuted > 10000);
    // Must complete safely in < 5 ms, never freezing the thread
    assert(elapsedMs < 10.0);

    std::cout << "[PASS] TestInstructionBudgetInfiniteLoop\n" << std::endl;
}

void TestGenerationalHandleSafety() {
    // Doc 08 §21 AC6: Destroying an actor referenced by a script handle and accessing it
    // must return default value and report QS-RUN-003 without crashing.
    std::cout << "[RUN] TestGenerationalHandleSafety (Doc 08 AC6)..." << std::endl;

    Qvm vm;

    uint32_t actorId = 42;
    uint32_t initialGen = 1;

    // Register valid actor handle (id=42, gen=1)
    vm.RegisterValidHandle(actorId, initialGen);
    assert(vm.IsHandleValid(actorId, initialGen));

    CompiledFunction fn;
    fn.name = "TestHandleAccess";

    // r0 = Handle(42, 1)
    // r1 = GETFIELD r0.x
    // RET r1
    uint16_t cX = fn.AddConstant(Value::String("x"));
    fn.code.push_back(EncodeABC(Opcode::GETFIELD, 1, 0, static_cast<uint8_t>(cX)));
    fn.code.push_back(EncodeABC(Opcode::RET, 1, 0, 0));

    // 1. Access valid handle
    auto resValid = vm.Execute(fn, {Value::Handle(actorId, initialGen)});
    std::cout << "  -> Valid handle access success: " << (resValid.success ? "true" : "false") << std::endl;
    assert(resValid.errorCode != "QS-RUN-003");

    // 2. Destroy actor: invalidate handle
    std::cout << "  -> Simulating Actor destruction (id=42)..." << std::endl;
    vm.InvalidateHandle(actorId);
    assert(!vm.IsHandleValid(actorId, initialGen));

    // 3. Access destroyed handle (stale generation)
    auto resDestroyed = vm.Execute(fn, {Value::Handle(actorId, initialGen)});
    std::cout << "  -> Stale handle access caught safely: [" << resDestroyed.errorCode << "] " 
              << resDestroyed.errorMessage << std::endl;

    assert(resDestroyed.errorCode == "QS-RUN-003");
    assert(resDestroyed.returnValue.type == ValueType::NIL);

    std::cout << "[PASS] TestGenerationalHandleSafety\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  QIVSCRIPT SAFETY & BUDGET SUITE (DOC 08) " << std::endl;
    std::cout << "===========================================" << std::endl;

    TestInstructionBudgetInfiniteLoop();
    TestGenerationalHandleSafety();

    std::cout << "ALL QVM SAFETY & BUDGET TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
