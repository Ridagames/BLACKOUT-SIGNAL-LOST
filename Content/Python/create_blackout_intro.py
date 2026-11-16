"""BLACKOUT: SIGNAL LOST — cinematic intro generator for Unreal Engine 5.4.

Run inside Unreal Editor (not system Python):
Tools > Execute Python Script > this file.

Builds a complete procedural industrial-horror intro level, authored materials,
environment props, lighting, camera animation, title animation, original audio,
and a Level Sequence. All generated content is saved under /Game.
"""
import unreal
import math
import os
import random
import wave
import struct

FPS = 24
DURATION = 27
END_FRAME = FPS * DURATION
MAP_PATH = "/Game/Maps/L_Intro"
SEQUENCE_PATH = "/Game/Cinematics/LS_BLACKOUT_Intro"
AUDIO_PATH = "/Game/Audio/SW_BLACKOUT_Intro"
PREFIX = "BLK_"

def log(message):
    unreal.log("[BLACKOUT CINEMATIC] " + str(message))

def safe_set(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as exc:
        log("Property " + prop + " skipped: " + str(exc))
        return False

def folder(path):
    unreal.EditorAssetLibrary.make_directory(path)

def asset(path):
    return unreal.EditorAssetLibrary.load_asset(path)

def spawn(cls, loc, rot=None, label=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        cls, loc, rot or unreal.Rotator(0, 0, 0))
    if actor:
        actor.set_actor_label(PREFIX + (label or cls.__name__))
    return actor

def primitive(path):
    obj = asset(path)
    if not obj:
        raise RuntimeError("Missing engine primitive: " + path)
    return obj

def material(name, rgb, roughness=0.65, metallic=0.0, emissive=None):
    """Create a real Material asset with editable PBR constants."""
    path = "/Game/Materials/" + name
    existing = asset(path)
    if existing:
        return existing
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(name, "/Game/Materials",
                             unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        log("Material creation failed: " + name)
        return None
    try:
        lib = unreal.MaterialEditingLibrary
        base = lib.create_material_expression(
            mat, unreal.MaterialExpressionConstant3Vector, -420, -80)
        base.set_editor_property("constant", unreal.LinearColor(
            rgb[0], rgb[1], rgb[2], 1.0))
        lib.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = lib.create_material_expression(
            mat, unreal.MaterialExpressionConstant, -420, 100)
        rough.set_editor_property("r", roughness)
        lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        metal = lib.create_material_expression(
            mat, unreal.MaterialExpressionConstant, -420, 220)
        metal.set_editor_property("r", metallic)
        lib.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
        if emissive:
            glow = lib.create_material_expression(
                mat, unreal.MaterialExpressionConstant3Vector, -420, 360)
            glow.set_editor_property("constant", unreal.LinearColor(
                emissive[0], emissive[1], emissive[2], 1.0))
            lib.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        lib.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)
        return mat
    except Exception as exc:
        log("Material graph setup failed for " + name + ": " + str(exc))
        return mat

def mesh_actor(mesh, name, loc, scale, mat=None, rot=None, collision=True):
    actor = spawn(unreal.StaticMeshActor, loc, rot, name)
    if not actor:
        return None
    comp = actor.get_component_by_class(unreal.StaticMeshComponent)
    if comp:
        comp.set_static_mesh(mesh)
        actor.set_actor_scale3d(scale)
        if mat:
            try:
                comp.set_material(0, mat)
            except Exception:
                pass
        if not collision:
            safe_set(comp, "collision_enabled", unreal.CollisionEnabled.NO_COLLISION)
    return actor

def box(cube, name, loc, scale, mat, rot=None):
    return mesh_actor(cube, name, loc, scale, mat, rot)

def cylinder(cyl, name, loc, scale, mat, rot=None):
    return mesh_actor(cyl, name, loc, scale, mat, rot)

def sphere(sph, name, loc, scale, mat):
    return mesh_actor(sph, name, loc, scale, mat, collision=False)

def text_actor(text, name, loc, rot, size, color):
    actor = spawn(unreal.TextRenderActor, loc, rot, name)
    if actor:
        c = actor.get_component_by_class(unreal.TextRenderComponent)
        if c:
            safe_set(c, "text", text)
            safe_set(c, "world_size", size)
            safe_set(c, "horizontal_alignment", unreal.HorizTextAligment.CENTER)
            safe_set(c, "vertical_alignment", unreal.VertTextAligment.TEXT_CENTER)
            safe_set(c, "text_render_color", color)
            safe_set(c, "text", text)
    return actor

def make_audio():
    """Create original 27-second stereo industrial score/SFX bed."""
    out_dir = unreal.Paths.project_saved_dir()
    os.makedirs(out_dir, exist_ok=True)
    wav_path = os.path.join(out_dir, "BLACKOUT_Intro_Original_Score.wav")
    sr = 24000
    rng = random.Random(341127)
    total = sr * DURATION
    events = [(7.8, 8.5), (9.0, 9.45), (10.2, 10.9), (18.0, 18.25)]
    with wave.open(wav_path, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(sr)
        chunk = bytearray()
        for i in range(total):
            t = i / float(sr)
            fadein = min(1.0, t / 2.2)
            fadeout = min(1.0, max(0.0, (DURATION - t) / 3.2))
            pulse = 0.5 + 0.5 * math.sin(2 * math.pi * 0.16 * t)
            drone = (0.12 * math.sin(2*math.pi*38*t)
                     + 0.055 * math.sin(2*math.pi*57*t + 0.2)
                     + 0.025 * math.sin(2*math.pi*76*t + math.sin(t*0.31)))
            machinery = 0.025 * math.sin(2*math.pi*(92 + 3*math.sin(t*.4))*t)
            sub = 0.055 * math.sin(2*math.pi*29*t) * pulse
            static = 0.0
            for a, b in events:
                if a <= t <= b:
                    env = min(1.0, (t-a)*18.0, (b-t)*16.0)
                    static += rng.uniform(-0.24, 0.24) * max(0.0, env)
            # Deep metallic hit and decaying resonant tail.
            dt = t - 12.65
            hit = 0.0
            if dt >= 0:
                env = math.exp(-dt*2.8)
                hit = env * (0.22*math.sin(2*math.pi*48*dt)
                             + 0.09*math.sin(2*math.pi*137*dt))
            # A short reverse swell before the title reveal.
            swell = 0.0
            if 10.7 < t < 12.65:
                x = (t-10.7)/1.95
                swell = 0.045 * x*x * math.sin(2*math.pi*(65+80*x)*t)
            mono = max(-0.88, min(0.88, (drone+machinery+sub+static+hit+swell)*fadein*fadeout))
            left = int(max(-1,min(1,mono*(0.96+0.04*math.sin(t*0.7))))*32767)
            right = int(max(-1,min(1,mono*(0.96+0.04*math.sin(t*0.7+0.4))))*32767)
            chunk.extend(struct.pack("<hh", left, right))
            if len(chunk) >= 32768:
                w.writeframesraw(chunk)
                chunk.clear()
        if chunk:
            w.writeframesraw(chunk)
    try:
        task = unreal.AssetImportTask()
        safe_set(task, "filename", wav_path)
        safe_set(task, "destination_path", "/Game/Audio")
        safe_set(task, "destination_name", "SW_BLACKOUT_Intro")
        safe_set(task, "automated", True)
        safe_set(task, "replace_existing", True)
        safe_set(task, "save", True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        sound = asset(AUDIO_PATH)
        if sound:
            safe_set(sound, "volume", 0.82)
            log("Original stereo ambience imported.")
            return sound
    except Exception as exc:
        log("Audio import issue: " + str(exc))
    return None

def add_audio(sequence, sound):
    if not sound:
        return
    try:
        track = sequence.add_master_track(unreal.MovieSceneAudioTrack)
        section = track.add_section()
        safe_set(section, "sound", sound)
        section.set_range(0, END_FRAME)
        log("Audio track added to sequence.")
    except Exception as exc:
        log("Audio track needs UE-version adjustment: " + str(exc))

def key(channel, frame, value, cubic=True):
    k = channel.add_key(unreal.FrameNumber(int(frame)), value)
    if cubic:
        try:
            channel.set_interpolation_mode(k, unreal.MovieSceneKeyInterpolation.CUBIC)
        except Exception:
            pass

def add_transform_track(sequence, actor, frames):
    binding = sequence.add_possessable(actor)
    track = binding.add_track(unreal.MovieScene3DTransformTrack)
    section = track.add_section()
    section.set_range(0, END_FRAME)
    channels = section.get_channels()
    for frame, loc, rot, scale in frames:
        values = [loc[0], loc[1], loc[2], rot[0], rot[1], rot[2],
                  scale[0], scale[1], scale[2]]
        for idx, value in enumerate(values):
            key(channels[idx], frame, value)
    return binding

def animate_text(sequence, actor, start, peak, end, max_scale=1.0):
    if not actor:
        return
    transform = actor.get_actor_transform()
    loc = transform.translation
    rot = transform.rotation.rotator()
    add_transform_track(sequence, actor, [
        (0, (loc.x,loc.y,loc.z), (rot.roll,rot.pitch,rot.yaw), (0.001,0.001,0.001)),
        (start, (loc.x,loc.y,loc.z), (rot.roll,rot.pitch,rot.yaw), (0.001,0.001,0.001)),
        (peak, (loc.x,loc.y,loc.z), (rot.roll,rot.pitch,rot.yaw), (max_scale,max_scale,max_scale)),
        (end, (loc.x,loc.y,loc.z), (rot.roll,rot.pitch,rot.yaw), (max_scale,max_scale,max_scale))
    ])

def fade_track(sequence):
    try:
        track = sequence.add_master_track(unreal.MovieSceneFadeTrack)
        section = track.add_section()
        section.set_range(0, END_FRAME)
        channels = section.get_channels()
        ch = channels[0]
        for f,v in [(0,1.0),(2*FPS,0.0),(24*FPS,0.0),(27*FPS,1.0)]:
            key(ch,f,v)
    except Exception as exc:
        log("Fade track unavailable in this UE patch: " + str(exc))

def create_sequence():
    folder("/Game/Cinematics")
    old = asset(SEQUENCE_PATH)
    if old:
        unreal.EditorAssetLibrary.delete_asset(SEQUENCE_PATH)
    seq = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "LS_BLACKOUT_Intro", "/Game/Cinematics",
        unreal.LevelSequence, unreal.LevelSequenceFactoryNew())
    if not seq:
        raise RuntimeError("Could not create Level Sequence.")
    seq.set_display_rate(unreal.FrameRate(FPS,1))
    seq.set_tick_resolution(unreal.FrameRate(24000,1))
    seq.set_playback_start(0)
    seq.set_playback_end(END_FRAME)
    return seq


def import_source_meshes():
    """Import authored OBJ source meshes from Content/SourceMeshes into /Game/Meshes."""
    folder("/Game/Meshes")
    source_dir = os.path.join(unreal.Paths.project_content_dir(), "SourceMeshes")
    names = ["SM_BLK_Generator", "SM_BLK_WallTerminal",
             "SM_BLK_CeilingLamp", "SM_BLK_DoorFrame", "SM_BLK_FloorGrate"]
    result = {}
    for name in names:
        obj_path = os.path.join(source_dir, name + ".obj")
        if not os.path.isfile(obj_path):
            log("Optional source mesh missing, using procedural fallback: " + obj_path)
            continue
        try:
            task = unreal.AssetImportTask()
            safe_set(task, "filename", obj_path)
            safe_set(task, "destination_path", "/Game/Meshes")
            safe_set(task, "destination_name", name)
            safe_set(task, "automated", True)
            safe_set(task, "replace_existing", True)
            safe_set(task, "save", True)
            unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            imported = asset("/Game/Meshes/" + name)
            if imported:
                result[name] = imported
                log("Imported authored OBJ mesh: " + name)
        except Exception as exc:
            log("OBJ import fallback for " + name + ": " + str(exc))
    return result

def create_environment():
    folder("/Game/Materials")
    cube = primitive("/Engine/BasicShapes/Cube")
    cyl = primitive("/Engine/BasicShapes/Cylinder")
    sph = primitive("/Engine/BasicShapes/Sphere")
    source_meshes = import_source_meshes()
    concrete = material("M_BLK_Concrete", (0.075,0.09,0.105), .88, .08)
    dark = material("M_BLK_PaintedSteel", (0.018,0.027,0.035), .52, .62)
    metal = material("M_BLK_BrushedMetal", (0.16,0.19,0.21), .34, .82)
    rust = material("M_BLK_OxidizedMetal", (0.19,0.065,0.035), .78, .35)
    yellow = material("M_BLK_WarningYellow", (0.52,0.31,0.035), .55, .25)
    cyan = material("M_BLK_CyanEmitter", (.02,.12,.16), .3, .2, (.02,.42,.65))
    red = material("M_BLK_RedAlarm", (.16,.012,.018), .28, .1, (.9,.012,.018))
    white = material("M_BLK_ColdEmitter", (.25,.31,.35), .3, .1, (.5,.68,.85))
    # Main corridor: 2,200 cm long, 620 cm wide, 460 cm high.
    box(cube,"Floor",(1100,0,-35),(12.2,3.3,.28),concrete)
    box(cube,"Ceiling",(1100,0,460),(12.2,3.3,.22),dark)
    box(cube,"LeftWall",(1100,-330,210),(12.2,.18,4.8),concrete)
    box(cube,"RightWall",(1100,330,210),(12.2,.18,4.8),concrete)
    box(cube,"FarBulkhead",(2240,0,210),(.22,3.3,4.8),dark)
    # Heavy structural frames, wall recesses and lower kick plates.
    for i in range(13):
        x=100+i*165
        box(cube,"FrameTop_%02d"%i,(x,0,420),(.16,3.25,.18),metal)
        box(cube,"FrameLeft_%02d"%i,(x,-305,210),(.16,.14,4.1),metal)
        box(cube,"FrameRight_%02d"%i,(x,305,210),(.16,.14,4.1),metal)
        box(cube,"KickLeft_%02d"%i,(x+75,-315,28),(.58,.035,.22),dark)
        box(cube,"KickRight_%02d"%i,(x+75,315,28),(.58,.035,.22),dark)
    # Ceiling cable trays, pipes and cross-bracing.
    for j,z in enumerate([360,382,402]):
        cylinder(cyl,"CeilingPipe_%02d"%j,(1100,-170+j*165,z),(22,.055,.055),metal,unreal.Rotator(0,90,0))
    for i in range(18):
        x=120+i*115
        cylinder(cyl,"CableDrop_%02d"%i,(x,-215,390),(.035,.035,.68),dark)
        cylinder(cyl,"CableDropR_%02d"%i,(x,215,390),(.035,.035,.68),dark)
    # Recessed wall panels, vents, hazard stripes and access doors.
    for side in [-1,1]:
        y=side*319
        for i in range(10):
            x=180+i*205
            box(cube,"WallPanel_%s_%02d"%(side,i),(x,y,220),(.72,.025,1.35),dark)
            box(cube,"PanelInset_%s_%02d"%(side,i),(x,y-side*2,220),(.62,.012,1.17),metal)
            box(cube,"PanelSlotA_%s_%02d"%(side,i),(x-22,y-side*4,220),(.018,.008,.82),cyan)
            box(cube,"PanelSlotB_%s_%02d"%(side,i),(x+22,y-side*4,220),(.018,.008,.82),cyan)
            box(cube,"DoorFrame_%s_%02d"%(side,i),(x+78,y,205),(.12,.07,3.3),rust)
            box(cube,"HazardBand_%s_%02d"%(side,i),(x+78,y-side*5,78),(.12,.012,.16),yellow)
    # Floor drainage channels and grates.
    for side in [-1,1]:
        y=side*265
        box(cube,"DrainChannel_%s"%side,(1100,y,-5),(11.4,.13,.025),dark)
        grate_mesh = source_meshes.get("SM_BLK_FloorGrate")
        if grate_mesh:
            for i in range(17):
                x=65+i*130
                mesh_actor(grate_mesh, "AuthoredFloorGrate_%s_%02d"%(side,i),
                           (x,y,-1), (1,1,1), metal)
        else:
            for i in range(36):
                x=65+i*59
                box(cube,"DrainGrate_%s_%02d"%(side,i),(x,y,-1),(.025,.12,.018),metal)
    # Industrial machines and generator housings along both sides.
    for i in range(6):
        x=300+i*310
        for side in [-1,1]:
            y=side*245
            box(cube,"MachineBase_%s_%02d"%(side,i),(x,y,55),(1.05,.55,.5),dark)
            generator = source_meshes.get("SM_BLK_Generator")
            if generator:
                mesh_actor(generator, "DetailedGenerator_%s_%02d"%(side,i),
                           (x,y,0), (1,1,1), metal)
            else:
                box(cube,"MachineBody_%s_%02d"%(side,i),(x,y,155),(.82,.43,1.55),metal)
            box(cube,"MachineFront_%s_%02d"%(side,i),(x,y-side*23,155),(.62,.025,1.15),dark)
            for n in range(4):
                box(cube,"MachineVent_%s_%02d_%02d"%(side,i,n),
                    (x-30+n*20,y-side*40,185),(.018,.01,.62),rust)
            sphere(sph,"MachineStatus_%s_%02d"%(side,i),(x+22,y-side*43,235),(.055,.025,.055),red if i%3==0 else cyan)
    # Authored industrial access-door frames, imported from the OBJ source library.
    door_mesh = source_meshes.get("SM_BLK_DoorFrame")
    if door_mesh:
        for x in [430, 1030, 1630]:
            for side in [-1, 1]:
                mesh_actor(door_mesh, "AuthoredDoorFrame_%s_%s"%(x,side),
                           (x,side*300,0), (1,1,1), rust)
    # Warning bollards, overhead lamps and emergency beacons.
    for i in range(8):
        x=190+i*270
        cylinder(cyl,"LampStem_%02d"%i,(x,0,433),(.07,.07,.22),dark)
        lamp_mesh = source_meshes.get("SM_BLK_CeilingLamp")
        if lamp_mesh:
            mesh_actor(lamp_mesh, "AuthoredCeilingLamp_%02d"%i,
                       (x,0,405), (1,1,1), metal)
        else:
            box(cube,"LampHousing_%02d"%i,(x,0,405),(.5,.42,.09),metal)
            box(cube,"LampLens_%02d"%i,(x,0,398),(.36,.28,.025),white)
        for side in [-1,1]:
            cylinder(cyl,"SafetyPost_%s_%02d"%(side,i),(x,side*280,58),(.08,.08,.58),yellow)
    # Terminal at the end of the corridor gives the camera a focal point.
    terminal_mesh = source_meshes.get("SM_BLK_WallTerminal")
    if terminal_mesh:
        mesh_actor(terminal_mesh, "AuthoredSignalTerminal",
                   (2050,0,0), (1,1,1), dark)
    else:
        box(cube,"SignalTerminal",(2050,0,185),(.75,.62,1.9),dark)
    box(cube,"TerminalScreen",(2010,-2,240),(.025,.42,.68),cyan,unreal.Rotator(0,0,0))
    text_actor("NO SIGNAL","TerminalText",(1990,-2,240),unreal.Rotator(0,90,0),24,unreal.Color(90,210,235,255))
    # Wet floor reflections are suggested by dark metallic inset plates.
    for i in range(15):
        x=230+i*130
        box(cube,"FloorPlate_%02d"%i,(x,random.uniform(-180,180),-18),
            (random.uniform(.35,.9),random.uniform(.25,.7),.012),metal)
    # Small dust particles suspended in the light cones.
    rng=random.Random(771)
    for i in range(85):
        loc=unreal.Vector(rng.uniform(100,2100),rng.uniform(-240,240),rng.uniform(75,385))
        size=rng.uniform(.008,.022)
        sphere(sph,"DustMote_%03d"%i,loc,(size,size,size),white)
    return cube,cyl,sph

def create_lighting():
    # Cold overhead pools plus alternating failing red warning fixtures.
    for i in range(8):
        x=190+i*270
        light=spawn(unreal.PointLight,unreal.Vector(x,0,375),label="ColdCeilingLight_%02d"%i)
        if light:
            c=light.get_component_by_class(unreal.PointLightComponent)
            safe_set(c,"intensity",3200.0 if i in [2,5] else 2100.0)
            safe_set(c,"attenuation_radius",680.0)
            safe_set(c,"light_color",unreal.LinearColor(.48,.66,.82,1))
            safe_set(c,"use_inverse_squared_falloff",True)
    for i in [3,7,10]:
        l=spawn(unreal.PointLight,unreal.Vector(100+i*165,-280,260),label="FailingRedAlarm_%02d"%i)
        if l:
            c=l.get_component_by_class(unreal.PointLightComponent)
            safe_set(c,"intensity",950.0)
            safe_set(c,"attenuation_radius",330.0)
            safe_set(c,"light_color",unreal.LinearColor(1,.018,.025,1))
    fog=spawn(unreal.ExponentialHeightFog,unreal.Vector(0,0,0),label="AtmosphericHaze")
    if fog:
        c=fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        safe_set(c,"fog_density",.018)
        safe_set(c,"fog_height_falloff",.19)
        safe_set(c,"fog_max_opacity",.72)
    pp=spawn(unreal.PostProcessVolume,unreal.Vector(1100,0,180),label="CinematicPostProcess")
    if pp:
        safe_set(pp,"b_unbound",True)
        settings=pp.get_editor_property("settings")
        for prop,val in [
            ("bloom_intensity",.72),("vignette_intensity",.48),
            ("film_slope",.88),("film_toe",.58),("film_shoulder",.24),
            ("film_black_clip",.015),("film_white_clip",.035),
            ("chromatic_aberration_intensity",.35)]:
            safe_set(settings,prop,val)
        safe_set(pp,"settings",settings)

def create_titles(sequence):
    # Text faces the camera; high contrast and restrained cold palette.
    facing=unreal.Rotator(0,180,0)
    engine=text_actor("UNREAL ENGINE","StudioCredit",(1670,0,295),facing,27,
                      unreal.Color(170,195,210,255))
    title=text_actor("BLACKOUT","MainTitle",(1670,0,205),facing,105,
                     unreal.Color(225,235,240,255))
    sub=text_actor("SIGNAL LOST","Subtitle",(1670,0,115),facing,34,
                   unreal.Color(100,190,215,255))
    credit=text_actor("A RIDAGAMES PRODUCTION","ProductionCredit",(1670,0,55),facing,19,
                      unreal.Color(175,190,198,255))
    animate_text(sequence,engine,2*FPS,4*FPS,7*FPS,.9)
    animate_text(sequence,title,11*FPS,14*FPS,26*FPS,1.0)
    animate_text(sequence,sub,12*FPS,15*FPS,26*FPS,1.0)
    animate_text(sequence,credit,16*FPS,19*FPS,26*FPS,1.0)
    # RGB split flashes, keyed to the static bursts.
    cyan=text_actor("SIGNAL LOST","GlitchCyan",(1666,-7,115),facing,34,
                    unreal.Color(40,210,245,255))
    red=text_actor("SIGNAL LOST","GlitchRed",(1674,7,115),facing,34,
                   unreal.Color(245,45,55,255))
    for actor in [cyan,red]:
        if actor:
            animate_text(sequence,actor,8*FPS,8*FPS+3,8*FPS+7,.9)
            animate_text(sequence,actor,9*FPS+2,9*FPS+4,9*FPS+6,.9)
            animate_text(sequence,actor,10*FPS+4,10*FPS+6,10*FPS+8,.9)

def create_camera(sequence):
    cam=spawn(unreal.CineCameraActor,unreal.Vector(-80,0,145),unreal.Rotator(0,0,0),
              "IntroCamera")
    if cam:
        cine=cam.get_cine_camera_component()
        safe_set(cine,"current_focal_length",27.0)
        safe_set(cine,"current_aperture",2.0)
        safe_set(cine,"focus_method",unreal.CameraFocusMethod.MANUAL)
        safe_set(cine,"manual_focus_distance",1600.0)
        binding=add_transform_track(sequence,cam,[
            (0,(-80,0,145),(0,0,0),(1,1,1)),
            (3*FPS,(90,0,145),(0,0,0),(1,1,1)),
            (8*FPS,(410,0,150),(0,0,0),(1,1,1)),
            (12*FPS,(680,0,150),(0,0,0),(1,1,1)),
            (18*FPS,(850,0,150),(0,0,0),(1,1,1)),
            (24*FPS,(900,0,150),(0,0,0),(1,1,1)),
            (27*FPS,(900,0,150),(0,0,0),(1,1,1))])
        try:
            track=sequence.add_master_track(unreal.MovieSceneCameraCutTrack)
            section=track.add_section()
            section.set_range(0,END_FRAME)
            section.set_camera_binding_id(unreal.MovieSceneObjectBindingID(guid=binding.get_id()))
        except Exception as exc:
            log("Camera cut binding needs editor validation: "+str(exc))
        return cam

def set_autoplay(sequence):
    try:
        actor=spawn(unreal.LevelSequenceActor,unreal.Vector(0,0,0),label="AutoPlayIntroSequence")
        if actor:
            safe_set(actor,"sequence",sequence)
            safe_set(actor,"b_auto_play",True)
            safe_set(actor,"b_auto_play",True)
            log("Sequence actor configured for autoplay (verify in editor).")
    except Exception as exc:
        log("Autoplay actor setup issue: "+str(exc))

def build():
    folder("/Game/Maps")
    folder("/Game/Cinematics")
    folder("/Game/Audio")
    folder("/Game/Materials")
    if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
        raise RuntimeError("Could not create "+MAP_PATH)
    create_environment()
    create_lighting()
    sequence=create_sequence()
    fade_track(sequence)
    create_titles(sequence)
    create_camera(sequence)
    add_audio(sequence,make_audio())
    set_autoplay(sequence)
    unreal.EditorLevelLibrary.save_current_level()
    unreal.EditorAssetLibrary.save_loaded_asset(sequence)
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True,True)
    except Exception as exc:
        log("Bulk save skipped: "+str(exc))
    log("BUILD COMPLETE: "+MAP_PATH+" | "+SEQUENCE_PATH)
    log("27 seconds / 24 fps / 16:9 cinematic intro generated.")
    log("Review Sequencer camera cut, audio and autoplay in UE 5.4 before rendering.")

if __name__ == "__main__":
    build()
