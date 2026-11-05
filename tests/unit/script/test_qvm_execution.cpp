#include "qivforge/script/qvm.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <cmath>

using namespace qivforge::script;

void TestVectorMathOps() {
    std::cout << "[RUN] TestVectorMathOps (Doc 08 §13.2 SIMD-friendly Vector Math)..." << std::endl;
    Qvm vm;

    CompiledFunction fn;
    fn.name = "TestVecOps";

    // Setup:
    // r1 = Vec3(3.0, 4.0, 0.0)
    // r2 = Vec3(1.0, 2.0, 3.0)
    // r3 = V3ADD r1, r2 -> Vec3(4.0, 6.0, 3.0)
    // r4 = V3SUB r3, r2 -> Vec3(3.0, 4.0, 0.0)
    // r5 = V3MULS r4, 2.0 -> Vec3(6.0, 8.0, 0.0)
    // r6 = V3LEN r5 -> 10.0
    // r7 = V3NORM r5 -> Vec3(0.6, 0.8, 0.0)
    // r8 = V3DOT r7, r7 -> 1.0

    uint16_t cVec3 = fn.AddConstant(Value::String("Vec3"));
    uint16_t c2 = fn.AddConstant(Value::Float(2.0));

    // Vec3(3, 4, 0)
    fn.code.push_back(EncodeABC(Opcode::LOADINT, 10, 0, 0)); // x=3
    fn.code.back() = EncodeAImm(Opcode::LOADINT, 10, 3);
    fn.code.push_back(EncodeAImm(Opcode::LOADINT, 11, 4));
    fn.code.push_back(EncodeAImm(Opcode::LOADINT, 12, 0));
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 1, cVec3));
    fn.code.push_back(EncodeABC(Opcode::CALLNATIVE, 1, 10, 3));

    // Vec3(1, 2, 3)
    fn.code.push_back(EncodeAImm(Opcode::LOADINT, 13, 1));
    fn.code.push_back(EncodeAImm(Opcode::LOADINT, 14, 2));
    fn.code.push_back(EncodeAImm(Opcode::LOADINT, 15, 3));
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 2, cVec3));
    fn.code.push_back(EncodeABC(Opcode::CALLNATIVE, 2, 13, 3));

    // V3ADD r3 = r1 + r2
    fn.code.push_back(EncodeABC(Opcode::V3ADD, 3, 1, 2));

    // V3SUB r4 = r3 - r2
    fn.code.push_back(EncodeABC(Opcode::V3SUB, 4, 3, 2));

    // V3MULS r5 = r4 * 2.0
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 20, c2));
    fn.code.push_back(EncodeABC(Opcode::V3MULS, 5, 4, 20));

    // V3LEN r6 = len(r5)
    fn.code.push_back(EncodeABC(Opcode::V3LEN, 6, 5, 0));

    // V3NORM r7 = norm(r5)
    fn.code.push_back(EncodeABC(Opcode::V3NORM, 7, 5, 0));

    // V3DOT r8 = dot(r7, r7)
    fn.code.push_back(EncodeABC(Opcode::V3DOT, 8, 7, 7));

    // RET r8
    fn.code.push_back(EncodeABC(Opcode::RET, 8, 0, 0));

    auto res = vm.Execute(fn);
    assert(res.success);
    std::cout << "  -> V3DOT result: " << res.returnValue.AsDouble() << std::endl;
    assert(std::abs(res.returnValue.AsDouble() - 1.0) < 1e-4);

    std::cout << "[PASS] TestVectorMathOps\n" << std::endl;
}

void TestNativeStandardLibrary() {
    std::cout << "[RUN] TestNativeStandardLibrary (Doc 08 §10 Standard Library)..." << std::endl;
    Qvm vm;

    CompiledFunction fn;
    fn.name = "TestStdLib";

    // Call Math.sqrt(16.0) -> 4.0
    uint16_t cSqrt = fn.AddConstant(Value::String("Math.sqrt"));
    uint16_t c16 = fn.AddConstant(Value::Float(16.0));
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 1, cSqrt));
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 2, c16));
    fn.code.push_back(EncodeABC(Opcode::CALLNATIVE, 1, 2, 1));

    // Call TouchHUD.getAxis("Vertical") -> 1.0
    uint16_t cAxis = fn.AddConstant(Value::String("TouchHUD.getAxis"));
    uint16_t cVert = fn.AddConstant(Value::String("Vertical"));
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 3, cAxis));
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 4, cVert));
    fn.code.push_back(EncodeABC(Opcode::CALLNATIVE, 3, 4, 1));

    // Call Time.deltaTime() -> ~0.01666
    uint16_t cDt = fn.AddConstant(Value::String("Time.deltaTime"));
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 5, cDt));
    fn.code.push_back(EncodeABC(Opcode::CALLNATIVE, 5, 0, 0));

    // Return r1 (sqrt(16))
    fn.code.push_back(EncodeABC(Opcode::RET, 1, 0, 0));

    auto res = vm.Execute(fn);
    assert(res.success);
    assert(std::abs(res.returnValue.AsDouble() - 4.0) < 1e-5);
    std::cout << "  -> Math.sqrt(16.0) = " << res.returnValue.AsDouble() << std::endl;

    std::cout << "[PASS] TestNativeStandardLibrary\n" << std::endl;
}

void TestExecutionSpeed1000Iterations() {
    // Doc 08 §21 AC7: Benchmark 1,000 tick iterations completes in < 4 ms
    std::cout << "[RUN] TestExecutionSpeed1000Iterations (Doc 08 AC7)..." << std::endl;
    Qvm vm;

    // Simulate vehicle onTick physics calculation:
    // speed = speed + throttle * accel * dt
    CompiledFunction fn;
    fn.name = "VehicleTick";

    uint16_t cThrottle = fn.AddConstant(Value::Float(1.0));
    uint16_t cAccel = fn.AddConstant(Value::Float(8.5));
    uint16_t cDt = fn.AddConstant(Value::Float(0.016667));

    // r1 = speed (in/out)
    // r2 = throttle (1.0)
    // r3 = accel (8.5)
    // r4 = dt (0.016667)
    // r5 = throttle * accel
    // r6 = r5 * dt
    // r1 = r1 + r6
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 2, cThrottle));
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 3, cAccel));
    fn.code.push_back(EncodeAImm(Opcode::LOADK, 4, cDt));
    fn.code.push_back(EncodeABC(Opcode::MULF, 5, 2, 3));
    fn.code.push_back(EncodeABC(Opcode::MULF, 6, 5, 4));
    fn.code.push_back(EncodeABC(Opcode::ADDF, 1, 1, 6));
    fn.code.push_back(EncodeABC(Opcode::RET, 1, 0, 0));

    auto tStart = std::chrono::high_resolution_clock::now();
    Value currentSpeed = Value::Float(0.0);

    const int iterations = 1000;
    for (int i = 0; i < iterations; ++i) {
        auto res = vm.Execute(fn, {Value::Nil(), currentSpeed});
        assert(res.success);
        currentSpeed = res.returnValue;
    }

    auto tEnd = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

    std::cout << "  -> 1,000 script iterations completed in " << elapsedMs << " ms"
              << " (Budget: < 4.0 ms, Final Speed: " << currentSpeed.AsDouble() << " m/s)" << std::endl;

    assert(elapsedMs < 4.0);
    std::cout << "[PASS] TestExecutionSpeed1000Iterations\n" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "  QIVSCRIPT QVM EXECUTION SUITE (DOC 08)   " << std::endl;
    std::cout << "===========================================" << std::endl;

    TestVectorMathOps();
    TestNativeStandardLibrary();
    TestExecutionSpeed1000Iterations();

    std::cout << "ALL QVM EXECUTION TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
