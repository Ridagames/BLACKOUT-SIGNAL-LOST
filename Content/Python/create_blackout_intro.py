"""BLACKOUT: SIGNAL LOST — UE 5.4 intro scene generator.

Run from Unreal Editor: Tools > Execute Python Script.
Creates /Game/Maps/L_Intro and /Game/Cinematics/LS_BLACKOUT_Intro.
This is editor automation; open and preview the sequence in Sequencer.
"""
import unreal

FPS = 24
DURATION = 24
MAP_PATH = "/Game/Maps/L_Intro"
SEQUENCE_PATH = "/Game/Cinematics/LS_BLACKOUT_Intro"
CUBE_PATH = "/Engine/BasicShapes/Cube"


def log(message):
    unreal.log("[BLACKOUT UE5] " + str(message))


def ensure_folder(path):
    unreal.EditorAssetLibrary.make_directory(path)


def spawn(actor_class, location, rotation=None, label=None):
    rotation = rotation or unreal.Rotator(0.0, 0.0, 0.0)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class, location, rotation
    )
    if actor and label:
        actor.set_actor_label(label)
    return actor


def set_mesh(actor, mesh):
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if component:
        component.set_static_mesh(mesh)
        component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
        return component
    return None


def add_box(mesh, label, location, scale):
    actor = spawn(unreal.StaticMeshActor, location, label=label)
    if actor:
        set_mesh(actor, mesh)
        actor.set_actor_scale3d(scale)
    return actor


def add_text(text, label, location, rotation, size, color):
    actor = spawn(unreal.TextRenderActor, location, rotation, label)
    if not actor:
        return None
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    if component:
        component.set_text(text)
        component.set_world_size(size)
        component.set_horizontal_alignment(unreal.HorizTextAligment.CENTER)
        component.set_vertical_alignment(unreal.VertTextAligment.TEXT_CENTER)
        component.set_text_render_color(color)
        component.set_editor_property("text", text)
    return actor


def add_camera_animation(sequence, camera):
    binding = sequence.add_possessable(camera)
    track = binding.add_track(unreal.MovieScene3DTransformTrack)
    section = track.add_section()
    section.set_range(0, FPS * DURATION)
    channels = section.get_channels()

    # Smooth forward dolly, then a restrained hold for the title reveal.
    keys = [
        (0, (0.0, 0.0, 105.0), (0.0, 0.0, 0.0)),
        (3 * FPS, (100.0, 0.0, 105.0), (0.0, 0.0, 0.0)),
        (8 * FPS, (390.0, 0.0, 108.0), (0.0, 0.0, 0.0)),
        (12 * FPS, (650.0, 0.0, 108.0), (0.0, 0.0, 0.0)),
        (18 * FPS, (820.0, 0.0, 108.0), (0.0, 0.0, 0.0)),
        (24 * FPS, (900.0, 0.0, 108.0), (0.0, 0.0, 0.0)),
    ]
    for frame, position, rotation in keys:
        for channel_index, value in enumerate(position):
            channels[channel_index].add_key(unreal.FrameNumber(frame), value)
        for channel_index, value in enumerate(rotation, start=3):
            channels[channel_index].add_key(unreal.FrameNumber(frame), value)
    return binding


def create_sequence():
    ensure_folder("/Game/Cinematics")
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    existing = unreal.EditorAssetLibrary.load_asset(SEQUENCE_PATH)
    if existing:
        unreal.EditorAssetLibrary.delete_asset(SEQUENCE_PATH)
    sequence = tools.create_asset(
        "LS_BLACKOUT_Intro",
        "/Game/Cinematics",
        unreal.LevelSequence,
        unreal.LevelSequenceFactoryNew()
    )
    if not sequence:
        raise RuntimeError("Unable to create Level Sequence.")
    sequence.set_display_rate(unreal.FrameRate(FPS, 1))
    sequence.set_tick_resolution(unreal.FrameRate(24000, 1))
    sequence.set_playback_start(0)
    sequence.set_playback_end(FPS * DURATION)
    return sequence


def build_level():
    ensure_folder("/Game/Maps")
    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        raise RuntimeError("Unable to create intro level: " + MAP_PATH)

    cube = unreal.EditorAssetLibrary.load_asset(CUBE_PATH)
    if not cube:
        raise RuntimeError("UE basic cube mesh not found: " + CUBE_PATH)

    # Long concrete corridor blockout (dimensions in Unreal centimetres).
    add_box(cube, "INTRO_Floor", unreal.Vector(800, 0, -35),
            unreal.Vector(11.0, 3.0, 0.25))
    add_box(cube, "INTRO_Ceiling", unreal.Vector(800, 0, 420),
            unreal.Vector(11.0, 3.0, 0.18))
    for index in range(11):
        x = 100 + index * 170
        add_box(cube, "INTRO_Wall_L_%02d" % index,
                unreal.Vector(x, -310, 190), unreal.Vector(0.9, 0.12, 3.8))
        add_box(cube, "INTRO_Wall_R_%02d" % index,
                unreal.Vector(x, 310, 190), unreal.Vector(0.9, 0.12, 3.8))
        # Recessed structural ribs give the corridor depth and rhythm.
        add_box(cube, "INTRO_Rib_%02d" % index,
                unreal.Vector(x, 0, 405), unreal.Vector(0.12, 3.0, 0.16))

    # Distant end wall and a readable title/credit reveal.
    add_box(cube, "INTRO_EndWall", unreal.Vector(1740, 0, 190),
            unreal.Vector(0.18, 3.0, 3.8))
    add_text("BLACKOUT", "INTRO_Title_BLACKOUT",
             unreal.Vector(1580, 0, 245), unreal.Rotator(0, 180, 0),
             78.0, unreal.Color(220, 232, 240, 255))
    add_text("SIGNAL LOST", "INTRO_Title_Subtitle",
             unreal.Vector(1575, 0, 165), unreal.Rotator(0, 180, 0),
             28.0, unreal.Color(130, 165, 185, 255))
    add_text("ÉDITÉ PAR RIDAGAMES", "INTRO_Credit",
             unreal.Vector(1570, 0, 95), unreal.Rotator(0, 180, 0),
             18.0, unreal.Color(190, 198, 205, 255))

    # Cold practical pools of light down the corridor.
    for index in range(5):
        light = spawn(unreal.PointLight,
                      unreal.Vector(260 + index * 300, 0, 335),
                      label="INTRO_ColdLight_%02d" % index)
        if light:
            component = light.get_component_by_class(unreal.PointLightComponent)
            if component:
                component.set_editor_property("intensity", 2600.0 if index == 2 else 1700.0)
                component.set_editor_property("attenuation_radius", 520.0)
                component.set_editor_property(
                    "light_color", unreal.LinearColor(0.48, 0.66, 0.82, 1.0)
                )

    # A little haze and restrained post-process contrast.
    fog = spawn(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0),
                label="INTRO_AtmosphericFog")
    if fog:
        component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if component:
            component.set_editor_property("fog_density", 0.025)
            component.set_editor_property("fog_height_falloff", 0.22)

    camera = spawn(unreal.CineCameraActor, unreal.Vector(0, 0, 105),
                   label="INTRO_CinematicCamera")
    if camera:
        cine = camera.get_cine_camera_component()
        if cine:
            cine.set_editor_property("current_focal_length", 32.0)
            cine.set_editor_property("current_aperture", 2.8)

    sequence = create_sequence()
    if camera:
        camera_binding = add_camera_animation(sequence, camera)
        try:
            cut_track = sequence.add_track(unreal.MovieSceneCameraCutTrack)
            cut_section = cut_track.add_section()
            cut_section.set_range(0, FPS * DURATION)
            cut_section.set_camera_binding_id(
                unreal.MovieSceneObjectBindingID(guid=camera_binding.get_id())
            )
        except Exception as exc:
            log("Camera cut track requires manual binding in this UE build: " + str(exc))

    unreal.EditorLevelLibrary.save_current_level()
    unreal.EditorAssetLibrary.save_loaded_asset(sequence)
    log("Created level: " + MAP_PATH)
    log("Created sequence: " + SEQUENCE_PATH)
    log("Open LS_BLACKOUT_Intro in Sequencer and preview. Geometry is a blockout; replace it with final art.")


def main():
    build_level()


if __name__ == "__main__":
    main()
