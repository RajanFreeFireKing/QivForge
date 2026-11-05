#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace qivforge::core {

struct FuzzReport {
    size_t iterationsRan = 0;
    size_t safeRejections = 0;
    size_t crashesDetected = 0;
    size_t unhandledExceptions = 0;
};

class FuzzEngine {
public:
    // Fuzzes QPK pack archive parser (Doc 13 AC9 & Doc 14 §15.7)
    [[nodiscard]] static FuzzReport FuzzQpkReader(size_t iterations = 500);

    // Fuzzes binary mesh parser (.qmesh)
    [[nodiscard]] static FuzzReport FuzzQmeshParser(size_t iterations = 500);

    // Fuzzes skeletal animation parser (.qanim)
    [[nodiscard]] static FuzzReport FuzzQanimParser(size_t iterations = 500);

    // Fuzzes QVM bytecode interpreter (gas budget & register boundaries)
    [[nodiscard]] static FuzzReport FuzzQvmExecution(size_t iterations = 500);
};

} // namespace qivforge::core
