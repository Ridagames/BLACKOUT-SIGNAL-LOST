import bpy
import math
import sys
from mathutils import Vector

# BLACKOUT: SIGNAL LOST — photorealistic horror-menu environment.
# Runs headlessly in Blender on the GitHub Actions build worker.

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
output = args[0] if args else "/workspace/BLACKOUT_MENU.png"

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for datablocks in (bpy.data.materials, bpy.data.meshes, bpy.data.curves, bpy.data.cameras, bpy.data.lights):
    pass

def material(name, color, roughness=0.8, metallic=0.0):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1.0)
    m.use_nodes = True
    bs = m.node_tree.nodes.get("Principled BSDF")
    bs.inputs["Base Color"].default_value = (*color, 1.0)
    bs.inputs["Roughness"].default_value = roughness
    bs.inputs["Metallic"].default_value = metallic
    return m

concrete = material("Damp stained concrete", (0.105, 0.115, 0.112), 0.96)
floor_mat = material("Wet dark concrete", (0.045, 0.052, 0.051), 0.28)
paint = material("Peeling institutional paint", (0.16, 0.17, 0.155), 0.88)
metal = material("Oxidised steel", (0.075, 0.085, 0.082), 0.48, 0.72)
rust = material("Old rust", (0.16, 0.055, 0.027), 0.82)
glass = material("Dirty glass", (0.018, 0.027, 0.028), 0.18, 0.3)
red = material("Emergency red", (0.22, 0.012, 0.006), 0.5)
black = material("Near black", (0.008, 0.011, 0.011), 0.95)

# Procedural surface detail: mottled plaster, grime and damp concrete.
def add_weathering(mat, scale, roughness=0.9, bump_strength=0.22):
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    bs = nodes.get("Principled BSDF")
    tex = nodes.new("ShaderNodeTexNoise")
    tex.inputs["Scale"].default_value = scale
    tex.inputs["Detail"].default_value = 5.0
    tex.inputs["Roughness"].default_value = 0.78
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.22
    ramp.color_ramp.elements[0].color = (0.025,0.030,0.029,1)
    ramp.color_ramp.elements[1].position = 0.78
    ramp.color_ramp.elements[1].color = (0.24,0.25,0.23,1)
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = bump_strength
    bump.inputs["Distance"].default_value = 0.075
    links.new(tex.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], bs.inputs["Base Color"])
    links.new(tex.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bs.inputs["Normal"])
    bs.inputs["Roughness"].default_value = roughness

add_weathering(concrete, 3.8, 0.94, 0.30)
add_weathering(paint, 5.5, 0.91, 0.24)
add_weathering(floor_mat, 2.0, 0.34, 0.12)

def cube(name, loc, scale, mat, bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.object
    o.name = name
    o.dimensions = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if mat: o.data.materials.append(mat)
    if bevel:
        mod = o.modifiers.new("Worn softened edges", "BEVEL")
        mod.width = bevel
        mod.segments = 2
        o.modifiers.new("Weighted corner normals", "WEIGHTED_NORMAL")
    return o

def cylinder(name, loc, radius, depth, mat, rot=(0,0,0)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=24, radius=radius, depth=depth, location=loc, rotation=rot)
    o = bpy.context.object
    o.name = name
    if mat: o.data.materials.append(mat)
    return o

# Long abandoned institutional corridor, with a readable dark zone on the left for menu text.
cube("Corridor floor", (0, 5, -0.18), (8, 25, 0.35), floor_mat, 0.04)
cube("Left concrete wall", (-4.05, 5, 1.65), (0.3, 25, 3.6), concrete, 0.04)
cube("Right concrete wall", (4.05, 5, 1.65), (0.3, 25, 3.6), paint, 0.04)
cube("Ceiling", (0, 5, 3.45), (8.2, 25, 0.3), black, 0.03)
# Distant end wall and heavy security door.
cube("End wall", (0, 17.2, 1.65), (8.2, 0.35, 3.6), concrete, 0.04)
cube("Security door frame", (0.7, 16.98, 1.38), (1.9, 0.18, 2.75), metal, 0.035)
cube("Security door", (0.7, 16.86, 1.38), (1.62, 0.08, 2.5), black, 0.025)
cube("Door vision slit", (0.7, 16.80, 1.65), (0.72, 0.025, 0.16), glass, 0.008)
cube("Door handle", (1.32, 16.77, 1.12), (0.035, 0.05, 0.28), rust, 0.01)

# Recessed side doorways, wall damage, pipes, conduit and service fixtures.
for y in (1.5, 6.5, 11.0):
    cube("Dark side doorway", (-3.87, y, 1.18), (0.035, 1.45, 2.25), black, 0.01)
    cube("Door jamb", (-3.78, y-0.79, 1.2), (0.13, 0.12, 2.45), metal, 0.015)
    cube("Door jamb", (-3.78, y+0.79, 1.2), (0.13, 0.12, 2.45), metal, 0.015)
    cube("Lintel", (-3.78, y, 2.39), (0.13, 1.7, 0.12), metal, 0.015)
for z, x in ((2.9,-3.7),(2.72,3.72),(0.35,3.72)):
    cylinder("Long corroded pipe", (x,5,z), 0.045, 24, rust if z < 1 else metal, (math.pi/2,0,0))
for y in (-3, 2, 8, 13):
    cube("Ceiling cable tray", (3.45,y,3.18), (0.12,0.12,0.12), metal, 0.015)
    for x in (-3.5, 3.5):
        cube("Wall conduit", (x,y,2.0), (0.09,0.09,2.8), metal, 0.01)

# Floor grates, scattered debris and a maintenance trolley silhouette.
for y in (0.2, 4.8, 9.5, 14.2):
    cube("Drain channel", (-0.8,y,0.012), (1.0,0.32,0.018), metal, 0.008)
    for n in range(7):
        cube("Drain slot", (-1.22+n*0.14,y-0.01,0.025), (0.025,0.27,0.012), black)
for loc, scale in [((-2.5,3.0,0.03),(0.45,0.18,0.05)),((2.2,7.2,0.04),(0.25,0.12,0.07)),((-1.8,12.2,0.03),(0.32,0.22,0.05))]:
    cube("Broken plaster debris", loc, scale, paint, 0.02)
cube("Abandoned trolley body", (2.65,10.5,0.55), (0.72,1.0,0.85), metal, 0.05)
for x in (2.3,3.0):
    for y in (10.15,10.85):
        cylinder("Trolley wheel", (x,y,0.14), 0.12, 0.07, black, (0,math.pi/2,0))

# Ceiling fixtures: cold, uneven practical lighting.
for y, power in ((-1,260),(5,115),(11,70),(15,45)):
    cube("Fluorescent fixture", (0.15,y,3.22), (1.5,0.34,0.09), glass, 0.025)
    bpy.ops.object.light_add(type="AREA", location=(0.15,y,3.05))
    light = bpy.context.object
    light.name = "Flickering cold fluorescent"
    light.data.energy = power
    light.data.shape = "RECTANGLE"
    light.data.size = 1.25
    light.data.size_y = 0.22
    light.data.color = (0.62,0.72,0.78)
    light.rotation_euler = (0,0,0)

# A weak red emergency lamp deep in the corridor.
cube("Emergency lamp housing", (-3.72,14.0,2.55), (0.08,0.45,0.22), metal, 0.025)
cube("Emergency red lens", (-3.665,14.0,2.55), (0.025,0.30,0.10), red, 0.01)
bpy.ops.object.light_add(type="POINT", location=(-3.25,14.0,2.45))
bpy.context.object.data.energy = 34
bpy.context.object.data.color = (1.0,0.025,0.008)
bpy.context.object.data.shadow_soft_size = 1.2

# Camera: low eye-level view, strong perspective into the corridor.
bpy.ops.object.camera_add(location=(-0.2,-10.7,1.72))
camera = bpy.context.object
camera.name = "BLACKOUT main menu camera"
target = Vector((0.0,11.5,1.45))
camera.rotation_euler = (target - camera.location).to_track_quat("-Z","Y").to_euler()
camera.data.lens = 34
camera.data.sensor_width = 36
bpy.context.scene.camera = camera

# Atmospheric haze in the far end of the corridor.
fogmat = bpy.data.materials.new("Subtle corridor haze")
fogmat.use_nodes = True
nodes = fogmat.node_tree.nodes
nodes.clear()
out = nodes.new("ShaderNodeOutputMaterial")
vol = nodes.new("ShaderNodeVolumePrincipled")
vol.inputs["Color"].default_value = (0.38,0.44,0.47,1)
vol.inputs["Density"].default_value = 0.012
fogmat.node_tree.links.new(vol.outputs["Volume"], out.inputs["Volume"])
cube("Atmospheric haze volume", (0,8,1.6), (7.5,18,3.0), fogmat)

# Dark cinematic grade and restrained film grain-like roughness via compositor.
scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE_NEXT" if bpy.app.version >= (4,2,0) else "BLENDER_EEVEE"
scene.render.resolution_x = 1920
scene.render.resolution_y = 1080
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGB"
scene.render.image_settings.color_depth = "8"
scene.render.filepath = output
scene.world.color = (0.008,0.010,0.012)
scene.view_settings.view_transform = "AgX" if "AgX" in bpy.context.scene.view_settings.bl_rna.properties["view_transform"].enum_items.keys() else "Standard"
scene.view_settings.look = "AgX - Medium High Contrast" if hasattr(scene.view_settings, "look") else "Medium High Contrast"
scene.camera.data.dof.use_dof = True
scene.camera.data.dof.focus_distance = 17
scene.camera.data.dof.aperture_fstop = 8.0
scene.render.image_settings.compression = 20
scene.render.film_transparent = False
scene.camera.data.lens = 36
bpy.ops.wm.save_as_mainfile(filepath=output.rsplit("/",1)[0] + "/blackout_menu_scene.blend")
bpy.ops.render.render(write_still=True)
