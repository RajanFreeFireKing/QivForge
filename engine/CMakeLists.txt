cmake_minimum_required(VERSION 3.22.1)
project(qivengine_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(qivengine_core STATIC
    core/src/engine.cpp
    render/src/render_device.cpp
    render/src/render_scene.cpp
    render/src/mesh_primitives.cpp
    scene/src/actor.cpp
    scene/src/transform.cpp
    scene/src/scene.cpp
    scene/src/undo_stack.cpp
    scene/src/template_factory.cpp
    render/src/gltf_importer.cpp
    physics/src/rigid_body.cpp
    physics/src/physics_world.cpp
    physics/src/vehicle_controller.cpp
    physics/src/character_controller.cpp
    script/src/lexer.cpp
    script/src/parser.cpp
    script/src/compiler.cpp
    script/src/qvm.cpp
    script/src/script_instance.cpp
    core/src/vault.cpp
    core/src/secret_scanner.cpp
    core/src/sha256.cpp
    core/src/ndjson_builder.cpp
    core/src/hf_client.cpp
    core/src/sync_engine.cpp
    core/src/asset_streamer.cpp
    core/src/migration_wizard.cpp
    scene/src/local_store.cpp
    scene/src/hf_store.cpp
    anim/src/skeleton.cpp
    anim/src/animation_clip.cpp
    anim/src/avatar_mask.cpp
    anim/src/blend_tree.cpp
    anim/src/anim_graph.cpp
    anim/src/foot_ik.cpp
    patch/src/patch_model.cpp
    patch/src/patch_validator.cpp
    patch/src/patch_compiler.cpp
    patch/src/patch_converter.cpp
    terrain/src/terrain_chunk.cpp
    terrain/src/terrain_sculptor.cpp
    terrain/src/vegetation_scatter.cpp
    terrain/src/navmesh_baker.cpp
    ui2d/src/hud_widget.cpp
    ui2d/src/hud_layout.cpp
    vfx/src/particle_emitter.cpp
    vfx/src/vfx_manager.cpp
    audio/src/audio_mixer.cpp
    audio/src/audio_manager.cpp
    render/src/post_process.cpp
    render/src/quality_governor.cpp
    ai/src/ai_context_builder.cpp
    ai/src/ai_tool_router.cpp
    ai/src/ai_edit_set.cpp
    ai/src/mock_ai_provider.cpp
    export/src/qpk_builder.cpp
    export/src/qpk_reader.cpp
    export/src/export_security.cpp
    export/src/project_baker.cpp
    export/src/axml_patcher.cpp
    export/src/apk_signer.cpp
    export/src/apk_assembler.cpp
    export/src/cloud_build_adapter.cpp
    core/src/fuzz_target.cpp
)

target_include_directories(qivengine_core PUBLIC
    core/include
    render/include
    scene/include
    physics/include
    script/include
    audio/include
    anim/include
    patch/include
    terrain/include
    ui2d/include
    vfx/include
    ai/include
    export/include
)

target_compile_options(qivengine_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fexceptions
    -frtti
)
