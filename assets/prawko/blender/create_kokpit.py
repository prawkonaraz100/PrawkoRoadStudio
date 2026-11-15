# Copyright 2026 PrawkoRoadStudio — Apache-2.0
# Deterministic, original Blender 5.3 vehicle cabin, authored around DRIVER EYE.
# Coordinates: +X camera-right, +Y forward, +Z up. Eye = (0,0,0).
# The glTF prop reader centers XY and lifts min Z to 0, so an export-time
# metadata file carries the inverse offsets for exact camera anchoring.
import bpy
import json
import math
from pathlib import Path
from mathutils import Vector

BASE = Path(__file__).resolve().parent
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for mat in list(bpy.data.materials):
    bpy.data.materials.remove(mat)

def material(name, rgb, metallic=0.0, roughness=0.8):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*rgb, 1.0)
    m.use_nodes = True
    p = m.node_tree.nodes.get("Principled BSDF")
    p.inputs["Base Color"].default_value = (*rgb, 1.0)
    p.inputs["Metallic"].default_value = metallic
    p.inputs["Roughness"].default_value = roughness
    return m

dark = material("Prawko charcoal soft-touch", (0.065,0.080,0.098))
dash = material("Dashboard graphite", (0.095,0.117,0.143))
cowl = material("Instrument binnacle", (0.035,0.045,0.060))
trim = material("Brushed aluminium", (0.40,0.44,0.48),.55,.36)
glass = material("Unlit instrument display", (0.018,0.047,0.070),.0,.38)
teal = material("Discrete teal instrumentation", (0.018,0.40,0.32),.0,.45)
amber = material("Prawko warm yellow", (0.94,0.68,0.18),.12,.35)
leather = material("Black leather", (0.050,0.055,0.067))
fabric = material("Seats textile", (0.094,0.115,0.138))
rubber = material("Wiper rubber", (0.015,0.022,0.028))
shell_objs, wheel_objs, wiper_objs = [], [], []

def track(obj, group, mat):
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    group.append(obj)
    return obj

def block(name, loc, dimensions, mat, group=shell_objs, bevel=0.0, rotation=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj=bpy.context.object
    obj.name=name
    obj.dimensions=dimensions
    if rotation is not None:
        obj.rotation_euler=rotation
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod=obj.modifiers.new("Soft industrial radius", "BEVEL")
        mod.width=bevel
        mod.segments=3
        mod.affect="EDGES"
        mod2=obj.modifiers.new("Weighted corner normals", "WEIGHTED_NORMAL")
    return track(obj,group,mat)

def beam(name,a,b,r,mat,group=shell_objs,vertices=12):
    delta=Vector(b)-Vector(a)
    center=(Vector(a)+Vector(b))*0.5
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=r,
                                         depth=delta.length,location=center)
    obj=bpy.context.object
    obj.name=name
    obj.rotation_euler=delta.to_track_quat("Z","Y").to_euler()
    return track(obj,group,mat)

def torus(name, center, major, minor, mat, group=shell_objs):
    bpy.ops.mesh.primitive_torus_add(major_radius=major,
                                     minor_radius=minor, major_segments=48,
                                     minor_segments=10, location=center,
                                     rotation=(math.pi/2,0,0))
    return track(bpy.context.object,group,mat)

# Structural cabin: dash face with stepped, bevelled cowl, not a screen-space quad.
block("Full-width sculpted dashboard",(.20, .86,-.58),(2.55,.73,.27),dash,bevel=.08)
block("Dashboard top padded lip",(.20,1.15,-.36),(2.44,.53,.14),dark,bevel=.055)
block("Windscreen base trough",(.18,1.37,-.29),(2.42,.11,.07),cowl,bevel=.018)
block("Sill accent satin strip",(.16,.56,-.55),(2.35,.035,.018),trim,bevel=.008)
block("Lower steering column",(-.16,.59,-.63),(.21,.40,.20),cowl,bevel=.065)
block("Instrument binnacle hood",(-.19,.97,-.24),(.91,.48,.28),cowl,bevel=.067)
block("Recessed speedometer screen",(-.23,.715,-.27),(.67,.024,.19),glass,bevel=.014)
block("Instrument light bar",(-.25,.694,-.18),(.41,.012,.010),teal,bevel=.004)
block("Center infotainment plinth",(.75,.85,-.42),(.56,.30,.31),cowl,bevel=.036)
block("Screen glass",(.75,.680,-.36),(.45,.025,.23),glass,bevel=.012)
block("Screen header",(.70,.660,-.277),(.23,.007,.008),teal,bevel=.0)
for i in range(5):
    block("Touch control "+str(i),(.58+i*.077,.652,-.40),(.032,.009,.007),trim,bevel=.003)
for i in range(3):
    torus("Vent right bezel "+str(i),(.24+i*.11,.640,-.365),.042,.006,trim)
    beam("Vent spindle "+str(i),(.24+i*.11-.026,.627,-.365),
         (.24+i*.11+.026,.627,-.365),.003,dark)
for i in range(10):
    x=-.48+i*.062
    block("Gauge graduated edge "+str(i),(x,.689,-.326),(.018,.009,.008),
          amber if i in (3,7) else trim,bevel=.003)

# Door panels, seat shoulders, front glazing pillars and roof reveal.
block("Near door card left",(-.93,.22,-.59),(.23,1.18,.50),dark,bevel=.035)
block("Far door card right",(1.37,.32,-.62),(.22,1.08,.38),dark,bevel=.035)
block("Left armrest",(-.80,.30,-.40),(.31,.55,.09),fabric,bevel=.035)
block("Right passenger armrest",(1.25,.32,-.42),(.27,.52,.08),fabric,bevel=.032)
# Windshield sits in the opening: opaque 3-D frame, intentionally no glass quad.
beam("Left A pillar",(-.90,.72,-.48),(-.87,1.31,.47),.047,dark)
beam("Right A pillar",(1.35,.78,-.50),(1.25,1.37,.47),.049,dark)
beam("Left pillar satin seam",(-.858,.70,-.47),(-.828,1.31,.45),.010,trim)
beam("Right pillar satin seam",(1.30,.76,-.48),(1.20,1.37,.43),.010,trim)
beam("Windscreen roof cross-member",(-.92,1.32,.47),(1.28,1.38,.47),.061,dark)
block("Roof front liner",(.18,1.05,.57),(2.38,.44,.12),fabric,bevel=.05)
block("Sun visor left",(-.38,.87,.45),(.63,.24,.045),dark,bevel=.02)
block("Sun visor right",(.92,.87,.45),(.67,.24,.045),dark,bevel=.02)
block("Roof console",(.24,.99,.46),(.27,.21,.06),cowl,bevel=.021)
# Console between front seats, shifter and physical console switches.
block("Center tunnel",(.73,.18,-.76),(.42,1.05,.20),cowl,bevel=.055)
beam("Gear selector stem",(.72,.37,-.64),(.71,.33,-.48),.025,trim)
block("Gear knob",(.71,.32,-.47),(.10,.11,.085),leather,bevel=.035)
for i in range(4):
    block("Console switch "+str(i),(.53+i*.095,.41,-.66),(.046,.055,.012),trim,bevel=.012)

# Separate moving steering wheel, centered on the ORIGIN (its true rotation pivot).
torus("Leather steering rim",(0,0,0),.242,.034,leather,wheel_objs)
torus("Steering rim satin seam",(0,-.010,0),.198,.005,trim,wheel_objs)
for angle in (30,150,270):
    a=math.radians(angle)
    beam("Wheel forged spoke "+str(angle),(0,.005,0),
         (.193*math.cos(a),0,.193*math.sin(a)),.030,dash,wheel_objs)
beam("Steering hub inner",(0,-.022,-.07),(0,-.022,.07),.055,cowl,wheel_objs)
block("Steering crest",(0,-.052,0),(.113,.047,.094),dash,wheel_objs,bevel=.026)
block("Wheel insignia",(0,-.079,.008),(.044,.010,.010),amber,wheel_objs,bevel=.003)
for side in (-1,1):
    for i in range(2):
        block("Wheel multifunction control "+str(side)+"-"+str(i),
              (side*(.09+i*.033),-.04,-.054),(.025,.015,.018),
              trim,wheel_objs,bevel=.005)
# A separate motor-actuated wiper component with pivot at (0,0,0):
# blade lies along +X and swings in XZ plane about camera-forward Y.
beam("Wiper arm",(0,0,0),(.60,0,.13),.008,trim,wiper_objs)
beam("Long rubber blade",(.12,-.007,.026),(.62,-.007,.13),.011,rubber,wiper_objs)
block("Wiper bearing",(0,0,0),(.07,.048,.045),dark,wiper_objs,bevel=.016)

# Physically sized, forward-set cabin: keep the forward view open and the
# steering rim below the road sightline at the editor's fixed 50-degree FOV.
# The original near-eye roof slabs occupied the windshield in perspective.
for obj in list(shell_objs):
    if obj.name in {"Roof front liner", "Sun visor left", "Sun visor right", "Roof console"}:
        shell_objs.remove(obj)
        bpy.data.objects.remove(obj, do_unlink=True)
for obj in shell_objs:
    obj.location.y += 0.78
    # Keep the thin windscreen posts at eye level. Put the real instrument
    # surfaces 20 cm BELOW the sightline, so the road remains visible even
    # at the editor's relatively narrow 50-degree vertical field of view.
    if not ("pillar" in obj.name.lower() or "windscreen roof" in obj.name.lower()):
        obj.location.z -= 0.22
for obj in wheel_objs:
    obj.scale = (0.78, 0.78, 0.78)

# Apply bevel modifiers for exact consistent GLB bounds.
for obj in shell_objs+wheel_objs+wiper_objs:
    bpy.context.view_layer.objects.active = obj
    for mod in list(obj.modifiers):
        bpy.ops.object.modifier_apply(modifier=mod.name)

def bbox(items):
    pts=[obj.matrix_world @ Vector(v) for obj in items for v in obj.bound_box]
    low=[min(p[i] for p in pts) for i in range(3)]
    high=[max(p[i] for p in pts) for i in range(3)]
    return {"center_xy":[(low[0]+high[0])/2,(low[1]+high[1])/2],
            "min_z":low[2], "bounds":[low,high]}

data={}
for key, group in (("shell",shell_objs),("wheel",wheel_objs),("wiper",wiper_objs)):
    data[key]=bbox(group)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in group: obj.select_set(True)
    bpy.context.view_layer.objects.active=group[0]
    bpy.ops.export_scene.gltf(
        filepath=str(BASE/f"prawko_kokpit_{key}.glb"),
        export_format="GLB",use_selection=True,export_apply=True,
        export_materials="EXPORT",export_normals=True)
# Editable Blender source includes all three independent assemblies.
bpy.ops.object.select_all(action="SELECT")
bpy.ops.wm.save_as_mainfile(filepath=str(BASE/"prawko_kokpit.blend"))
(BASE/"kokpit_layout.json").write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding="utf-8")
print("PRAWKO_COCKPIT_BLENDER_PASS",json.dumps(data))
