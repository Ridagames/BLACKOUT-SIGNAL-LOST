"""Run inside Unreal Editor (Window > Python > Execute Script).
Creates a first-pass 24s intro Level Sequence and a simple dark corridor staging set.
This is editor automation source; it must be previewed and adjusted in the actual UE 5.4 editor.
"""
import unreal

FPS = 24
DURATION_SECONDS = 24
SEQUENCE_PATH = "/Game/Cinematics/LS_BLACKOUT_Intro"

def log(message):
    unreal.log("[BLACKOUT INTRO] " + str(message))

def ensure_folder(path):
    unreal.EditorAssetLibrary.make_directory(path)

def spawn(actor_class, location, rotation=unreal.Rotator(0, 0, 0), label=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class, location, rotation, transient=False
    )
    if actor and label:
        actor.set_actor_label(label)
    return actor

def add_camera_keys(sequence, camera):
    binding = sequence.add_possessable(camera)
    track = binding.add_track(unreal.MovieScene3DTransformTrack)
    section = track.add_section()
    section.set_range(0, FPS * DURATION_SECONDS)
    channels = section.get_channels()
    # Transform channel order: translation XYZ, rotation XYZ, scale XYZ.
    positions = [
        (0, unreal.Vector(0, 0, 95)),
        (3 * FPS, unreal.Vector(180, 0, 95)),
        (8 * FPS, unreal.Vector(520, 0, 100)),
        (12 * FPS, unreal.Vector(700, 0, 100)),
        (18 * FPS, unreal.Vector(760, 0, 100)),
        (24 * FPS, unreal.Vector(820, 0, 100)),
    ]
    for frame, value in positions:
        channels[0].add_key(unreal.FrameNumber(frame), value.x)
        channels[1].add_key(unreal.FrameNumber(frame), value.y)
        channels[2].add_key(unreal.FrameNumber(frame), value.z)
    return binding

def main():
    ensure_folder("/Game/Cinematics")
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    sequence = tools.create_asset(
        "LS_BLACKOUT_Intro",
        "/Game/Cinematics",
        unreal.LevelSequence,
        unreal.LevelSequenceFactoryNew()
    )
    if not sequence:
        raise RuntimeError("Could not create Level Sequence asset.")
    sequence.set_display_rate(unreal.FrameRate(FPS, 1))
    sequence.set_playback_start(0)
    sequence.set_playback_end(FPS * DURATION_SECONDS)

    # Dark industrial corridor: simple blocking geometry, to be replaced by final art.
    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    if cube:
        for i in range(7):
            wall = spawn(unreal.StaticMeshActor,
                         unreal.Vector(250 + i * 220, 360, 180),
                         label="INTRO_Blockout_Wall_%02d" % i)
            if wall:
                wall.static_mesh_component.set_static_mesh(cube)
                wall.set_actor_scale3d(unreal.Vector(1.8, 0.15, 3.6))
            opposite = spawn(unreal.StaticMeshActor,
                             unreal.Vector(250 + i * 220, -360, 180),
                             label="INTRO_Blockout_Wall_B_%02d" % i)
            if opposite:
                opposite.static_mesh_component.set_static_mesh(cube)
                opposite.set_actor_scale3d(unreal.Vector(1.8, 0.15, 3.6))
        floor = spawn(unreal.StaticMeshActor, unreal.Vector(900, 0, -20),
                      label="INTRO_Blockout_Floor")
        if floor:
            floor.static_mesh_component.set_static_mesh(cube)
            floor.set_actor_scale3d(unreal.Vector(12, 2.5, 0.2))

    camera = spawn(unreal.CineCameraActor, unreal.Vector(0, 0, 95),
                   unreal.Rotator(0, 0, 0), "INTRO_CineCamera")
    if camera:
        add_camera_keys(sequence, camera)
        try:
            cut_track = sequence.add_track(unreal.MovieSceneCameraCutTrack)
            cut_section = cut_track.add_section()
            cut_section.set_range(0, FPS * DURATION_SECONDS)
            cut_section.set_camera_binding_id(
                unreal.MovieSceneObjectBindingID()
            )
        except Exception as exc:
            log("Camera-cut binding needs editor-version adjustment: " + str(exc))

    # Practical cold lights along the corridor; final lighting/VFX pass remains to do.
    for i in range(4):
        light = spawn(unreal.PointLight,
                      unreal.Vector(350 + i * 380, 0, 260),
                      label="INTRO_Practical_Light_%02d" % i)
        if light:
            component = light.get_component_by_class(unreal.PointLightComponent)
            if component:
                component.set_editor_property("intensity", 1800.0)
                component.set_editor_property("attenuation_radius", 650.0)
                component.set_editor_property("light_color",
                                              unreal.LinearColor(0.55, 0.72, 1.0, 1.0))

    unreal.EditorAssetLibrary.save_loaded_asset(sequence)
    log("Created " + SEQUENCE_PATH)
    log("Next: open the sequence, assign the camera cut binding if required by this UE build, add title typography, fog, glitch VFX and original audio.")
    log("This is a first blockout, not a final cinematic.")

main()
