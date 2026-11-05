#include "qivforge/scene/scene.h"
#include "qivforge/scene/undo_stack.h"
#include <cassert>
#include <iostream>

using namespace qivforge::scene;

void TestActorHierarchyAndMatrix() {
    Scene scene;
    Actor* parent = scene.CreateActor("Parent");
    assert(parent != nullptr);
    assert(parent->GetId() == 1);
    assert(scene.GetRootActorIds().size() == 1);

    parent->GetTransform().SetPosition(10.0f, 0.0f, 0.0f);

    Actor* child = scene.CreateActor("Child", parent->GetId());
    assert(child != nullptr);
    assert(child->GetId() == 2);
    assert(child->GetParentId() == parent->GetId());
    assert(parent->GetChildren().size() == 1);

    child->GetTransform().SetPosition(5.0f, 0.0f, 0.0f);

    // World matrix of child should combine parent translation: 10 + 5 = 15 on X
    const Mat4& parentWorld = parent->GetTransform().GetWorldMatrix();
    const Mat4& childWorld = child->GetTransform().GetWorldMatrix(&parentWorld);

    // In column-major or row-major translation is at index 3 or 12
    float childWorldX = childWorld[3];
    assert(std::abs(childWorldX - 15.0f) < 0.001f);

    std::cout << "[PASS] TestActorHierarchyAndMatrix" << std::endl;
}

void Test100StepUndoRedoStress() {
    // Doc 15 §3 M3 Exit: "undo of 100 steps works"
    Scene scene;
    UndoStack stack(200);

    assert(scene.GetActorCount() == 0);
    assert(!stack.CanUndo());
    assert(!stack.CanRedo());

    // Execute 100 commands
    for (int i = 1; i <= 100; ++i) {
        std::string name = "Actor_" + std::to_string(i);
        auto cmd = std::make_unique<CreateActorCommand>(scene, name);
        stack.PushAndExecute(std::move(cmd));
    }

    assert(scene.GetActorCount() == 100);
    assert(stack.UndoCount() == 100);
    assert(stack.CanUndo());
    assert(!stack.CanRedo());

    // Undo all 100 steps
    for (int i = 0; i < 100; ++i) {
        assert(stack.Undo());
    }

    assert(scene.GetActorCount() == 0);
    assert(!stack.CanUndo());
    assert(stack.CanRedo());
    assert(stack.RedoCount() == 100);

    // Redo all 100 steps
    for (int i = 0; i < 100; ++i) {
        assert(stack.Redo());
    }

    assert(scene.GetActorCount() == 100);
    assert(stack.CanUndo());
    assert(!stack.CanRedo());

    std::cout << "[PASS] Test100StepUndoRedoStress (100 steps undo and redo verified)" << std::endl;
}

void TestSceneSerialization() {
    Scene scene;
    scene.CreateActor("Sun");
    scene.CreateActor("Camera");
    scene.CreateActor("Cube");

    std::string json = scene.SerializeToJson();
    assert(!json.empty());
    assert(json.find("Sun") != std::string::npos);
    assert(json.find("Camera") != std::string::npos);
    assert(json.find("Cube") != std::string::npos);

    std::cout << "[PASS] TestSceneSerialization" << std::endl;
}

int main() {
    std::cout << "Running QivForge Scene Model & Undo System Tests..." << std::endl;
    TestActorHierarchyAndMatrix();
    Test100StepUndoRedoStress();
    TestSceneSerialization();
    std::cout << "All Scene Model & Undo System Tests Passed Successfully!" << std::endl;
    return 0;
}
