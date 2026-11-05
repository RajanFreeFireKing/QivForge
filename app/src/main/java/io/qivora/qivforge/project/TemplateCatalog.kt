package io.qivora.qivforge.project

data class TemplateInfo(
    val id: String,
    val title: String,
    val description: String,
    val tag: String,
    val defaultSceneJson: String
)

object TemplateCatalog {

    val Blank = TemplateInfo(
        id = "BLANK",
        title = "Blank 3D World",
        description = "Flat terrain plane, directional sunlight, skybox, and a default perspective camera.",
        tag = "Minimal",
        defaultSceneJson = """{"actors": [{"name": "Sun", "type": "DirectionalLight"}, {"name": "Camera", "type": "Camera"}]}"""
    )

    val Tractor = TemplateInfo(
        id = "TRACTOR",
        title = "Tractor & Vehicle Simulator",
        description = "Open-world terrain, 4-wheel drive tractor with Jolt physics, follow camera, and touch pedals.",
        tag = "Vehicle",
        defaultSceneJson = """{"actors": [{"name": "Tractor", "type": "VehicleController"}, {"name": "FollowCamera", "type": "Camera"}]}"""
    )

    val Tps = TemplateInfo(
        id = "TPS",
        title = "TPS Survival Arena",
        description = "Third-person character controller, touch analog joystick, weapon raycasts, and health HUD.",
        tag = "Shooter",
        defaultSceneJson = """{"actors": [{"name": "Hero", "type": "CharacterController"}, {"name": "ArenaMesh", "type": "StaticMesh"}]}"""
    )

    val Racing = TemplateInfo(
        id = "RACING",
        title = "Racing Track",
        description = "Curved road spline, timing checkpoints, lap counter, and speed HUD.",
        tag = "Racing",
        defaultSceneJson = """{"actors": [{"name": "Racer", "type": "VehicleController"}, {"name": "TrackSpline", "type": "Spline"}]}"""
    )

    val HudPlayground = TemplateInfo(
        id = "HUD_PLAYGROUND",
        title = "HUD Playground",
        description = "Testbed for multi-touch virtual joysticks, buttons, steering wheels, and dials.",
        tag = "UI/Controls",
        defaultSceneJson = """{"actors": [{"name": "UIAnchor", "type": "UIContainer"}]}"""
    )

    val All = listOf(Blank, Tractor, Tps, Racing, HudPlayground)
}
