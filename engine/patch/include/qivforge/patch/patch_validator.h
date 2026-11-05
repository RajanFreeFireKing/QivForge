#pragma once

#include "qivforge/patch/patch_model.h"
#include <string>
#include <vector>

namespace qivforge::patch {

struct PatchDiagnostic {
    std::string code; // "QP0001" through "QP0005"
    std::string nodeId;
    std::string pinId;
    std::string message;
    bool isWarning = false;
};

class PatchValidator {
public:
    PatchValidator() = default;

    [[nodiscard]] std::vector<PatchDiagnostic> Validate(const PatchGraph& graph) const;
    [[nodiscard]] bool HasErrors(const std::vector<PatchDiagnostic>& diags) const;

    // Checks implicit type conversion compatibility (Doc 09 §2)
    [[nodiscard]] static bool CanConvert(PinType from, PinType to) noexcept;
};

} // namespace qivforge::patch
