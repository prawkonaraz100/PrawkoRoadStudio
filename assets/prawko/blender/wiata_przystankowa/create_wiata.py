# SPDX-License-Identifier: Apache-2.0
"""Prawko Assets A1: own detailed roadside shelter, Blender 5.3, metres, Z up.
Real objects; GLB only selected ASSET collection; demo lighting never exported.
Front/open face = local -Y. Asset bbox around X=0, Y=0, bottom Z=0.
"""
import bpy, math, json, random
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parent
rng=random.Random(20261115)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for m in list(bpy.data.materials):
    bpy.data.materials.remove(m)
asset=bpy.data.collections.new("Prawko | Wiata Miejska 01 | EXPORT")
bpy.context.scene.collection.children.link(asset)

def rgba(rgb,alpha=1): return tuple(rgb)+(alpha,)
def simple_mat(name,color,metal=0,rough=.75,alpha=1,transmission=0,tex=None):
    mat=bpy.data.materials.new(name)
    mat.diffuse_color=rgba(color,alpha)
    mat.use_nodes=True
    bsdf=mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value=rgba(color)
    bsdf.inputs["Metallic"].default_value=metal
    bsdf.inputs["Roughness"].default_value=rough
    bsdf.inputs["Alpha"].default_value=alpha
    bsdf.inputs["Transmission Weight"].default_value=transmission
    bsdf.inputs["IOR"].default_value=1.45
    if alpha < 1:
        try: mat.surface_render_method='DITHERED'
        except Exception: pass
        mat.use_nodes=True
    if tex is not None:
        node=mat.node_tree.nodes.new('ShaderNodeTexImage')
        node.image=tex
        node.interpolation='Linear'
        mat.node_tree.links.new(node.outputs['Color'],bsdf.inputs['Base Color'])
    return mat

def texture(name,kind,dim=256):
    img=bpy.data.images.new(name,width=dim,height=dim,alpha=True)
    pix=[0.0]*(dim*dim*4)
    # Deterministic physical-surface variation, not a downloaded stock texture.
    for y in range(dim):
        for x in range(dim):
            n=rng.uniform(-1,1)
            if kind=='concrete':
                grain=(.65+.1*n+.04*math.sin((x+y)*.16))
                col=(grain*.69,grain*.71,grain*.72)
            elif kind=='wood':
                vein=math.sin(y*.7+math.sin(x*.1)*3)+.35*math.sin(y*2.1)
                grain=max(0.15,min(1,.60+.13*vein+.06*n))
                col=(grain*.76,grain*.49,grain*.26)
            else:
                grain=.5+.05*n
                col=(grain,grain,grain)
            off=(y*dim+x)*4
            pix[off:off+4]=[col[0],col[1],col[2],1.0]
    img.pixels[:]=pix
    img.filepath_raw=str(ROOT/(name+'.png'))
    img.file_format='PNG'
    img.save()
    img.pack()
    return img

concrete_tex=texture('prawko_wiata_beton_albedo','concrete')
wood_tex=texture('prawko_wiata_drewno_albedo','wood')
m={
 'metal':simple_mat('01 Powder coated steel RAL 7016',(0.085,.104,.123),.78,.27),
 'steel':simple_mat('02 Brushed stainless bolt steel',(.47,.54,.57),.9,.23),
 'rubber':simple_mat('03 Dark silicone EPDM',(.036,.047,.054),.02,.93),
 'concrete':simple_mat('04 Fine grained board-formed concrete',(.52,.54,.54),0,.88,tex=concrete_tex),
 'wood':simple_mat('05 Warm slatted treated hardwood',(.48,.31,.16),0,.62,tex=wood_tex),
 'glass':simple_mat('06 Clear safety glass light cyan',(.78,.90,.94),.0,.09,.23,.64),
 'roof':simple_mat('07 Matte graphite insulated roof',(.105,.118,.13),.3,.62),
 'poster':simple_mat('08 Route timetable paper',(.90,.91,.86),0,.78),
 'yellow':simple_mat('09 Prawko warm stop identifier',(.96,.71,.13),.15,.42),
 'red':simple_mat('10 Safety reflector red',(.71,.055,.058),.12,.4),
 'black':simple_mat('11 Route legend black',(.065,.073,.078),0,.91),
 'azure':simple_mat('12 Route legend blue',(.10,.39,.58),0,.69)
}
entities=[]
def gather(obj,mat,kind):
    # Non-destructive Blender modelling is preserved, but exporter applies mods.
    for coll in list(obj.users_collection): coll.objects.unlink(obj)
    asset.objects.link(obj)
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    if kind: obj['part_role']=kind
    entities.append(obj)
    return obj
def box(name, xyz, dims, mat, bevel=0, kind=None, rot=None):
    bpy.ops.mesh.primitive_cube_add(size=1,location=xyz)
    o=bpy.context.object
    o.name=name
    o.dimensions=dims
    if rot is not None:o.rotation_euler=rot
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new("Manufactured rounded edges","BEVEL")
        mod.width=bevel
        mod.segments=2
        mod.affect='EDGES'
        o.modifiers.new("Weighted corner normals","WEIGHTED_NORMAL")
    return gather(o,mat,kind)
def cylinder(name,loc,radius,depth,mat,verts=16,axis=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=radius,depth=depth,location=loc)
    o=bpy.context.object
    o.name=name
    if axis is not None:
        a,b=axis
        o.rotation_euler=(Vector(b)-Vector(a)).to_track_quat('Z','Y').to_euler()
    return gather(o,mat,None)
def pipe(name,a,b,radius,mat,verts=12):
    delta=Vector(b)-Vector(a)
    mid=(Vector(a)+Vector(b))/2
    return cylinder(name,mid,radius,delta.length,mat,verts,(a,b))
def sign_text(name, text,loc,size,mat,rotate=(math.pi/2,0,0),extrude=.001):
    font=bpy.data.curves.new(name,'FONT')
    font.body=text
    font.size=size
    font.align_x='CENTER'
    font.align_y='CENTER'
    font.extrude=extrude
    o=bpy.data.objects.new(name,font)
    asset.objects.link(o)
    o.location=loc
    o.rotation_euler=rotate
    bpy.context.view_layer.objects.active=o
    o.select_set(True)
    bpy.ops.object.convert(target='MESH')
    o=bpy.context.object
    o.select_set(False)
    o.data.materials.clear();o.data.materials.append(mat)
    entities.append(o)
    return o

# Foundation. Front of stop is negative Y; road lies beyond the front edge.
box("A00 Concrete paved pad 4.96m",(0,0,.065),(4.96,2.35,.13),m['concrete'],.025,'foundation')
# Raised curb-facing tactile paving with solid modelled inset ribs.
box("A01 Boarded tactile strip",(0,-.91,.136),(4.55,.31,.02),m['concrete'],.005)
for i in range(36):
    x=-2.15+i*.123
    box(f"A02 tactile relief {i:02d}",(x,-.96,.158),(.07,.08,.018),m['concrete'],.005)
# Floor stone joints as narrow strips of contrasting rubber (real geometry).
for i in range(1,9):
    x=-2.43+i*.54
    box(f"A03 slab joint {i}",(x,0,.133),(.007,2.18,.003),m['rubber'])
# Four 90mm hollow steel vertical structure members and their feet.
for side,x in [('L',-2.15),('R',2.15)]:
    for pos,y in [('front',-.83),('rear',.80)]:
        box(f"B00 {side} {pos} vertical steel column",(x,y,1.36),(.10,.12,2.55),m['metal'],.016,'post')
        box(f"B01 {side} {pos} welded floor anchor",(x,y,.18),(.24,.23,.052),m['steel'],.016)
        for ix in [-1,1]:
            for iy in [-1,1]:
                cylinder(f"B02 M10 floor anchor bolt {side} {pos} {ix} {iy}",
                         (x+ix*.08,y+iy*.075,.224),.012,.018,m['steel'],10)
# Roof: 4.78x2.02 with two different pitch levels and layered profile
box("C00 Roof integrated sandwich panel",(0,-.01,2.67),(4.82,2.07,.13),m['roof'],.047,'roof',(.033,0,0))
box("C01 Roof front rain gutter",(0,-1.0,2.615),(4.80,.057,.092),m['metal'],.016)
box("C02 Roof rear structural extrusion",(0,.86,2.594),(4.78,.08,.10),m['metal'],.012)
for x in [-2.30,-1.65,-.80,0,.8,1.65,2.30]:
    box(f"C03 roof rib {x}",(x,-.02,2.744),(.035,1.93,.037),m['steel'],.008)
for side,x in [('L',-2.15),('R',2.15)]:
    pipe(f"C04 cantilever roof brace {side}",(x,-.71,2.40),(x,.50,2.57),.027,m['steel'])
# Gutter downspout.
pipe("C05 rear downpipe",(-2.21,.88,2.59),(-2.21,.89,.26),.030,m['metal'])
cylinder("C06 downpipe discharge",(-2.21,.9,.30),.04,.06,m['steel'])

# Rear panels: three toughened clear glass bays with separating mullions.
for i,(xa,xb) in enumerate([(-2.04,-.75),(-.66,.66),(.75,2.04)]):
    xm=(xa+xb)/2
    box(f"D00 Rear glass laminated pane {i}",(xm,.82,1.42),(xb-xa,.016,1.93),m['glass'],.003,'glass')
    box(f"D01 Rear glass lower drip rail {i}",(xm,.792,.43),(xb-xa+.035,.028,.027),m['metal'],.006)
    for x in [xa,xb]:
        box(f"D02 rear structural seal {i} x{x}",(x,.809,1.42),(.014,.039,1.93),m['rubber'],.004)
# Side panels: mostly clear, strong frames, tiny compliance reflectors.
for side,x in [('L',-2.14),('R',2.14)]:
    box(f"D03 {side} side safety glazing",(x,.0,1.48),(.014,1.51,1.88),m['glass'],.002,'glass')
    box(f"D04 {side} top cap seal",(x,0,2.43),(.028,1.54,.031),m['rubber'],.004)
    box(f"D05 {side} bottom seal",(x,0,.54),(.028,1.53,.03),m['rubber'],.004)
    box(f"D06 {side} safety marking",(x,-.12,1.18),(.022,.24,.04),m['yellow'],.008)
    box(f"D07 {side} safety marking red",(x,-.12,1.12),(.022,.24,.027),m['red'],.006)

# Integral seating: fabricated metal bench with 6 real separate timber slats
# and 3 steel legs. Seat at 0.49m, backrest to 1.10m.
for x in [-1.50,0,1.50]:
    box(f"E00 Bench frame leg {x}",(x,.48,.34),(.065,.55,.55),m['metal'],.016)
    box(f"E01 Bench seat rear stay {x}",(x,.73,.77),(.055,.09,.82),m['metal'],.015)
for i in range(5):
    y=.20+i*.10
    box(f"E02 Bench hardwood seat slat {i}",(0,y,.508),(3.45,.09,.055),m['wood'],.018,'wood')
for i in range(4):
    z=.77+i*.105
    box(f"E03 Bench hardwood backrest {i}",(0,.80,z),(3.47,.065,.075),m['wood'],.017,'wood')
for x in [-1.68,1.68]:
    box(f"E04 Bench armrest rubber {x}",(x,.39,.69),(.09,.36,.08),m['rubber'],.03)

# Timetable wall cabinet: 1.08 x 1.16m at right back, facing front (-Y)
cab=(1.31,.725,1.56)
box("F00 Timetable frame",cab,(1.06,.095,1.21),m['metal'],.015,'timetable')
box("F01 Timetable white sheet",(1.31,.664,1.55),(.96,.012,1.08),m['poster'],.003)
box("F02 Timetable glass cover",(1.31,.642,1.55),(.96,.008,1.08),m['glass'],.002,'glass')
box("F03 Yellow route nameplate",(1.31,.626,1.97),(.86,.016,.16),m['yellow'],.006)
# Timetable printed as 3D thin colored bars (visible even with flattened textures)
for i in range(8):
    z=1.85-i*.09
    box(f"F04 Route schedule row {i}",(1.29,.621,z),(.70,.008,.013),m['black'] if i%2 else m['azure'])
for i in range(3):
    x=.96+i*.29
    box(f"F05 Route schedule time table {i}",(x,.62,1.27),(.19,.009,.06),m['azure'])
for x in [.81,1.81]:
    for z in [1.02,2.08]:
        cylinder(f"F06 Cabinet countersunk M6 {x} {z}",(x,.604,z),.015,.025,m['steel'],12,
                 ((x,.59,z),(x,.616,z)))
# Stop ID and high-frequency route label, no misleading state transit logos.
box("G00 Yellow roof identification panel",(0,-1.03,2.42),(.84,.025,.26),m['yellow'],.016,'identifier')
# for each letter, true mesh text (exportable as GLB), not a decal bitmap
sign_text("G01 Stop marker letter P","P",(0,-1.055,2.415),.17,m['black'])
# Shelter body bolts: structural hardware around glazing (not thousands of tris)
for side,x in [('L',-2.15),('R',2.15)]:
    for z in [.62,1.20,1.85,2.27]:
        cylinder(f"H00 Countersunk rear glazing screw {side} {z}",
                 (x,.775,z),.015,.018,m['steel'],8,
                 ((x,.766,z),(x,.784,z)))
# Emergency contrast: low-height visibility markers at edges.
for x in [-2.18,2.18]:
    for z in [.84,1.05]:
        box(f"H01 Door contrast strip x{x} z{z}",(x,-.847,z),(.014,.014,.09),m['yellow'],.003)

# Ensure no demo elements export, no unbounded modifiers and no accidental
# transparencies in opaque materials.
for obj in entities:
    obj.hide_render=False
bpy.ops.object.select_all(action='DESELECT')
for obj in entities: obj.select_set(True)
bpy.context.view_layer.objects.active=entities[0]

output=ROOT/'prawko_wiata_przystankowa.glb'
bpy.ops.export_scene.gltf(
 filepath=str(output),export_format='GLB',use_selection=True,
 export_apply=True,export_materials='EXPORT',
 export_normals=True,export_texcoords=True,export_cameras=False,
 export_lights=False
)
# Saved source includes only authored asset, not preview camera/lighting.
bpy.ops.object.select_all(action='SELECT')
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'prawko_wiata_przystankowa.blend'))
# Summary. Bounding box uses Blender units, not an unvalidated assumption.
mins=[min((obj.matrix_world @ Vector(v))[i] for obj in entities for v in obj.bound_box) for i in range(3)]
maxs=[max((obj.matrix_world @ Vector(v))[i] for obj in entities for v in obj.bound_box) for i in range(3)]
meta={"title":"Prawko Wiata Miejska A1",
      "units":"metres","axes":"Z up, front negative Y",
      "dimensions_m":[round(maxs[i]-mins[i],3) for i in range(3)],
      "objects":len(entities),
      "materials":[x.name for x in m.values()],
      "blender_version":bpy.app.version_string,
      "glass_alpha":.23,"glass_transmission":.64,
      "texture_files":[concrete_tex.filepath_raw,wood_tex.filepath_raw],
      "glb":output.name}
(ROOT/'model-metadata.json').write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf8')
print("PRAWKO_A1_BLEND_GLTF_EXPORT_PASS",json.dumps(meta,ensure_ascii=False))
