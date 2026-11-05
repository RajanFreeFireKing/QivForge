#pragma once

#include "qivforge/patch/patch_model.h"
#include <string>
#include <memory>

namespace qivforge::patch {

class PatchConverter {
public:
    PatchConverter() = default;

    // Converts QivScript source code into a visual PatchGraph (Doc 09 §12)
    [[nodiscard]] static std::shared_ptr<PatchGraph> ScriptToPatch(const std::string& scriptSource);

    // Converts a visual PatchGraph back to clean QivScript source code (Doc 09 §12)
    [[nodiscard]] static std::string PatchToScript(const PatchGraph& graph);
};

} // namespace qivforge::patch
