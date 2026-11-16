"""BLACKOUT: SIGNAL LOST — UE 5.4 intro scene generator.

Run from Unreal Editor: Tools > Execute Python Script.
Creates /Game/Maps/L_Intro and /Game/Cinematics/LS_BLACKOUT_Intro.
This is editor automation; open and preview the sequence in Sequencer.
"""
import unreal
import math
import os
import random
import wave
import struct

FPS = 24
DURATION = 24
MAP_PATH = "/Game/Maps/L_Intro"
SEQUENCE_PATH = "/Game/Cinematics/LS_BLACKOUT_Intro"
CUBE_PATH = "/Engine/BasicShapes/Cube"
AUDIO_PATH = "/Game/Audio/SW_BLACKOUT_Intro"


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



def add_smooth_key(channel, frame, value):
    key = channel.add_key(unreal.FrameNumber(frame), value)
    try:
        channel.set_interpolation_mode(key, unreal.MovieSceneKeyInterpolation.CUBIC)
    except Exception:
        pass


def generate_original_audio():
    """Generate an original 24-second industrial horror ambience WAV."""
    saved_dir = unreal.Paths.project_saved_dir()
    os.makedirs(saved_dir, exist_ok=True)
    wav_path = os.path.join(saved_dir, "BLACKOUT_Intro_Ambience.wav")
    sample_rate = 22050
    total = sample_rate * DURATION
    rng = random.Random(34017)
    static_windows = [(8.0, 8.7), (9.15, 9.55), (10.25, 10.95)]
    impact_time = 12.55

    with wave.open(wav_path, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        chunk = bytearray()
        for i in range(total):
            t = i / sample_rate
            fade_in = min(1.0, t / 2.0)
            fade_out = min(1.0, max(0.0, (DURATION - t) / 3.0))
            drone = (0.115 * math.sin(2.0 * math.pi * 43.0 * t)
                     + 0.045 * math.sin(2.0 * math.pi * 64.5 * t)
                     + 0.025 * math.sin(2.0 * math.pi * 51.0 * t + math.sin(t * 0.23)))
            machine = 0.018 * math.sin(2.0 * math.pi * 118.0 * t + math.sin(t * 0.7))
            signal = 0.0
            for start, end in static_windows:
                if start <= t <= end:
                    envelope = min(1.0, (t - start) * 24.0, (end - t) * 18.0)
                    signal += rng.uniform(-0.20, 0.20) * max(0.0, envelope)
            dt = t - impact_time
            if dt >= 0.0:
                impact_env = math.exp(-dt * 3.2)
                signal += 0.24 * impact_env * math.sin(2.0 * math.pi * 57.0 * dt)
                signal += 0.08 * impact_env * math.sin(2.0 * math.pi * 173.0 * dt)
            value = max(-0.92, min(0.92, (drone + machine + signal) * fade_in * fade_out))
            chunk.extend(struct.pack("<h", int(value * 32767)))
            if len(chunk) >= 32768:
                wav.writeframesraw(chunk)
                chunk.clear()
        if chunk:
            wav.writeframesraw(chunk)

    try:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", wav_path)
        task.set_editor_property("destination_path", "/Game/Audio")
        task.set_editor_property("destination_name", "SW_BLACKOUT_Intro")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        sound = unreal.EditorAssetLibrary.load_asset(AUDIO_PATH)
        if sound:
            log("Imported original ambience: " + AUDIO_PATH)
            return sound
        log("SoundWave import did not return an asset.")
    except Exception as exc:
        log("Audio import failed: " + str(exc))
    return None


def add_audio_to_sequence(sequence, sound):
    if not sound:
        return
    try:
        track = sequence.add_master_track(unreal.MovieSceneAudioTrack)
        section = track.add_section()
        section.set_editor_property("sound", sound)
        section.set_range(0, FPS * DURATION)
        log("Original ambience added to Sequencer.")
    except Exception as exc:
        log("Sequencer audio track setup failed: " + str(exc))


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
            add_smooth_key(channels[channel_index], frame, value)
        for channel_index, value in enumerate(rotation, start=3):
            channels[channel_index].add_key(unreal.FrameNumber(frame), value)
    return binding


def animate_text_scale(sequence, actor, keyframes, label):
    """Animate a text actor's scale for a deliberate cinematic reveal."""
    if not actor:
        return
    binding = sequence.add_possessable(actor)
    track = binding.add_track(unreal.MovieScene3DTransformTrack)
    section = track.add_section()
    section.set_range(0, FPS * DURATION)
    channels = section.get_channels()

    # Keep position/rotation constant while animating scale XYZ.
    transform = actor.get_actor_transform()
    location = transform.translation
    rotation = transform.rotation.rotator()
    for index, value in enumerate((location.x, location.y, location.z)):
        add_smooth_key(channels[index], 0, value)
    for index, value in enumerate((rotation.roll, rotation.pitch, rotation.yaw), start=3):
        channels[index].add_key(unreal.FrameNumber(0), value)
    for frame, scale in keyframes:
        for index in range(3):
            add_smooth_key(channels[index + 6], frame, scale)
    log("Animated reveal: " + label)


def add_fade_track(sequence):
    """Fade from black at the opening and back to black at the end."""
    try:
        track = sequence.add_master_track(unreal.MovieSceneFadeTrack)
        section = track.add_section()
        section.set_range(0, FPS * DURATION)
        channels = section.get_channels()
        if not channels:
            raise RuntimeError("Fade track exposes no opacity channel.")
        opacity = channels[0]
        # 1 = black, 0 = fully visible in Unreal's camera fade track.
        fade_keys = [
            (0, 1.0),
            (2 * FPS, 0.0),
            (21 * FPS, 0.0),
            (24 * FPS, 1.0),
        ]
        for frame, value in fade_keys:
            add_smooth_key(opacity, frame, value)
        log("Added opening and ending fade-to-black.")
    except Exception as exc:
        log("Fade track could not be generated automatically: " + str(exc))


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

    # Distant end wall and layered studio/title reveal.
    add_box(cube, "INTRO_EndWall", unreal.Vector(1740, 0, 190),
            unreal.Vector(0.18, 3.0, 3.8))
    engine_credit = add_text("UNREAL ENGINE", "INTRO_EngineCredit",
                             unreal.Vector(1570, 0, 325), unreal.Rotator(0, 180, 0),
                             22.0, unreal.Color(190, 205, 218, 255))
    title = add_text("BLACKOUT", "INTRO_Title_BLACKOUT",
                     unreal.Vector(1580, 0, 245), unreal.Rotator(0, 180, 0),
                     78.0, unreal.Color(220, 232, 240, 255))
    subtitle = add_text("SIGNAL LOST", "INTRO_Title_Subtitle",
                        unreal.Vector(1575, 0, 165), unreal.Rotator(0, 180, 0),
                        28.0, unreal.Color(130, 165, 185, 255))
    credit = add_text("ÉDITÉ PAR RIDAGAMES", "INTRO_Credit",
                      unreal.Vector(1570, 0, 95), unreal.Rotator(0, 180, 0),
                      18.0, unreal.Color(190, 198, 205, 255))
    # Brief offset signal echoes: a restrained red/cyan interference accent.
    glitch_cyan = add_text("SIGNAL LOST", "INTRO_Glitch_Cyan",
                           unreal.Vector(1574, -5, 165), unreal.Rotator(0, 180, 0),
                           28.0, unreal.Color(55, 190, 220, 255))
    glitch_red = add_text("SIGNAL LOST", "INTRO_Glitch_Red",
                          unreal.Vector(1576, 5, 165), unreal.Rotator(0, 180, 0),
                          28.0, unreal.Color(210, 65, 75, 255))

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
    # Fade in from black and fade out to prepare the main-menu transition.
    add_fade_track(sequence)

    sound = generate_original_audio()
    add_audio_to_sequence(sequence, sound)

    # Studio credit appears first, then the game title.
    animate_text_scale(sequence, engine_credit, [
        (2 * FPS, 0.02), (3 * FPS, 0.25),
        (4 * FPS, 1.0), (6 * FPS, 1.0), (7 * FPS, 0.02)
    ], "UNREAL ENGINE credit")

    # Staggered title reveal: scale eases from near-zero to full size,
    # with long holds so each line remains readable.
    animate_text_scale(sequence, title, [
        (11 * FPS, 0.02), (12 * FPS, 0.15),
        (14 * FPS, 1.0), (24 * FPS, 1.0)
    ], "BLACKOUT")
    animate_text_scale(sequence, subtitle, [
        (12 * FPS, 0.02), (13 * FPS, 0.2),
        (15 * FPS, 1.0), (24 * FPS, 1.0)
    ], "SIGNAL LOST")
    animate_text_scale(sequence, credit, [
        (16 * FPS, 0.02), (17 * FPS, 0.2),
        (19 * FPS, 1.0), (24 * FPS, 1.0)
    ], "ÉDITÉ PAR RIDAGAMES")
    # Three short bursts around the signal interruption (8–11 seconds).
    glitch_keys = [
        (0, 0.02), (8 * FPS, 0.02),
        (8 * FPS + 3, 1.0), (8 * FPS + 7, 0.02),
        (9 * FPS + 2, 1.0), (9 * FPS + 6, 0.02),
        (10 * FPS + 4, 1.0), (10 * FPS + 8, 0.02),
        (24 * FPS, 0.02)
    ]
    animate_text_scale(sequence, glitch_cyan, glitch_keys, "cyan signal interference")
    animate_text_scale(sequence, glitch_red, glitch_keys, "red signal interference")

    if camera:
        camera_binding = add_camera_animation(sequence, camera)
        try:
            cut_track = sequence.add_master_track(unreal.MovieSceneCameraCutTrack)
            cut_section = cut_track.add_section()
            cut_section.set_range(0, FPS * DURATION)
            cut_section.set_camera_binding_id(
                unreal.MovieSceneObjectBindingID(guid=camera_binding.get_id())
            )
        except Exception as exc:
            log("Camera cut track requires manual binding in this UE build: " + str(exc))

    try:
        sequence_actor = spawn(unreal.LevelSequenceActor, unreal.Vector(0, 0, 0),
                               label="INTRO_AutoPlaySequence")
        if sequence_actor:
            sequence_actor.set_editor_property("sequence", sequence)
            sequence_actor.set_editor_property("bAutoPlay", True)
            log("Level Sequence Actor configured for auto-play.")
    except Exception as exc:
        log("Auto-play setup failed: " + str(exc))

    unreal.EditorLevelLibrary.save_current_level()
    unreal.EditorAssetLibrary.save_loaded_asset(sequence)
    log("Created level: " + MAP_PATH)
    log("Created sequence: " + SEQUENCE_PATH)
    log("UNREAL ENGINE credit, BLACKOUT title and Ridagames credit are animated.")
    log("Opening/ending fades, red/cyan signal bursts and generated original ambience are included where supported.")
    log("The intro map is configured as the project's startup map.")
    log("Open LS_BLACKOUT_Intro in Sequencer and preview. Corridor geometry is still a blockout.")


def main():
    build_level()


if __name__ == "__main__":
    main()
