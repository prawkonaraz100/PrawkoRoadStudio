# SPDX-License-Identifier: Apache-2.0
"""Two original metric Blender GLB roadside assets. No downloaded stock models.
Run only via the bounded PrawkoBridge roadpack01 command, output to runner staging.
"""
import bpy, math, json, sys, random
from pathlib import Path
from mathutils import Vector

args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
if len(args)!=1: raise SystemExit("Expected one isolated output directory")
OUT=Path(args[0]).resolve()
if not OUT.is_dir(): raise SystemExit("Output staging does not exist")
if any(OUT.iterdir()): raise SystemExit("Refusing nonempty output staging")
random.seed(20261118)

def mat(name, rgb, metallic=0.0, roughness=.7, emit=None, strength=0):
    m=bpy.data.materials.new(name)
    m.diffuse_color=(*rgb,1.0)
    m.use_nodes=True
    p=m.node_tree.nodes.get("Principled BSDF")
    p.inputs['Base Color'].default_value=(*rgb,1.0)
    p.inputs['Metallic'].default_value=metallic
    p.inputs['Roughness'].default_value=roughness
    if emit:
        p.inputs['Emission Color'].default_value=(*emit,1.0)
        p.inputs['Emission Strength'].default_value=strength
    return m

def reset():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for m in list(bpy.data.materials): bpy.data.materials.remove(m)
    for i in list(bpy.data.images):
        if i.users==0: bpy.data.images.remove(i)
    return []

def make(obj,name,material,parts,bevel=0):
    obj.name=name
    for s in list(obj.data.materials): pass
    obj.data.materials.clear()
    obj.data.materials.append(material)
    if bevel:
        mod=obj.modifiers.new("Machined edge radius","BEVEL")
        mod.width=bevel
        mod.segments=3
        obj.modifiers.new("Weighted normals","WEIGHTED_NORMAL")
    parts.append(obj)
    return obj

def cube(name,loc,dims,material,parts,bevel=0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    obj=bpy.context.object
    obj.dimensions=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return make(obj,name,material,parts,bevel)

def cylinder(name,loc,r,depth,material,parts,segments=32,axis=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=depth,location=loc)
    obj=bpy.context.object
    if axis:
        vec=Vector(axis)
        obj.rotation_euler=vec.to_track_quat('Z','Y').to_euler()
    return make(obj,name,material,parts,0)

def torus(name,loc,major,minor,material,parts):
    bpy.ops.mesh.primitive_torus_add(major_segments=48,minor_segments=8,
       location=loc,rotation=(math.pi/2,0,0),major_radius=major,minor_radius=minor)
    return make(bpy.context.object,name,material,parts)

def pipe(name,a,b,radius,material,parts,segments=16):
    d=Vector(b)-Vector(a)
    return cylinder(name,tuple((Vector(a)+Vector(b))*.5),radius,d.length,
                    material,parts,segments,axis=d)

def wood_tex(name):
    # Original authored color texture; embedded in GLB via a glTF-compatible node.
    N=256
    img=bpy.data.images.new(name,width=N,height=N,alpha=True)
    px=[0.0]*(N*N*4)
    for y in range(N):
        for x in range(N):
            v=math.sin(y*.36+2.8*math.sin(x*.045))*.065
            v+=math.sin(y*1.15+1.4*math.sin(x*.12))*.025
            v+=random.uniform(-.018,.018)
            g=max(.18,min(.92,.57+v))
            at=(y*N+x)*4
            px[at:at+4]=[.72*g+.09,.44*g+.06,.23*g+.03,1.0]
    img.pixels[:]=px
    img.file_format="PNG"
    img.filepath_raw=str(OUT/"r7a-lawka-drewno-albedo.png")
    img.save()
    img.pack()
    return img

def preview(stem,parts,target,scale):
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.object.camera_add(location=(scale*1.1,-scale*1.55,scale*.9))
    c=bpy.context.object
    c.rotation_euler=(Vector(target)-c.location).to_track_quat('-Z','Y').to_euler()
    c.data.type='ORTHO'
    c.data.ortho_scale=scale*1.55
    bpy.context.scene.camera=c
    bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,-.035))
    floor=bpy.context.object
    floor.scale=(scale*.85,scale*.85,1)
    floor.data.materials.append(mat(stem+" preview floor",(.23,.245,.255),roughness=.93))
    for idx,(loc,power) in enumerate([((4,-5,8),950),((-5,2,6),650)]):
        bpy.ops.object.light_add(type='AREA',location=loc)
        lamp=bpy.context.object
        lamp.data.energy=power
        lamp.data.shape='DISK';lamp.data.size=5
        lamp.rotation_euler=(Vector(target)-lamp.location).to_track_quat('-Z','Y').to_euler()
    scene=bpy.context.scene
    scene.render.engine='CYCLES'
    scene.cycles.samples=12
    scene.render.resolution_x=512
    scene.render.resolution_y=512
    scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'
    scene.render.filepath=str(OUT/(stem+"-blender.png"))
    scene.camera.data.lens=50
    scene.render.film_transparent=False
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(stem+".blend")))
    bpy.ops.render.render(write_still=True)
    img=bpy.data.images.load(str(OUT/(stem+"-blender.png")),check_existing=False)
    img.scale(96,96)
    img.filepath_raw=str(OUT/(stem+"-thumbnail.png"))
    img.file_format='PNG'
    img.save()
    bpy.data.images.remove(img)

def publish(stem,parts,bbox,scale,target):
    # Export only genuine model geometry (not studio lighting/floor/cameras).
    bpy.ops.object.select_all(action='DESELECT')
    for obj in parts: obj.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    glb=OUT/(stem+".glb")
    bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',
       use_selection=True,export_apply=True,export_materials='EXPORT',
       export_normals=True,export_texcoords=True,export_cameras=False,
       export_lights=False)
    assert glb.stat().st_size>15000
    bpy.ops.object.select_all(action='DESELECT')
    preview(stem,parts,target,scale)
    triangles=sum(sum(len(poly.vertices)-2 for poly in obj.data.polygons)
                  for obj in parts if obj.type=='MESH')
    info={'model':stem,'units':'metres','up_axis':'Z','forward':'-Y',
          'bbox_m':bbox,'objects':len(parts),'triangles_before_modifiers':triangles,
          'materials':len(set(m.name for o in parts for m in o.data.materials)),
          'glb_bytes':glb.stat().st_size,'blender':bpy.app.version_string,
          'license':'Apache-2.0; original PrawkoNaRaz geometry and textures'}
    (OUT/(stem+"-metadata.json")).write_text(
        json.dumps(info,ensure_ascii=False,indent=2),encoding='utf-8')
    print("PRAWKO_ASSET_GENERATED",json.dumps(info,ensure_ascii=False),flush=True)

def signal():
    p=reset()
    graphite=mat('RAL 9005 matte polycarbonate',(.037,.047,.055),.12,.53)
    steel=mat('Hot dip galvanized steel',(.43,.46,.47),.78,.42)
    black=mat('Black rubber gaskets',(.013,.019,.023),.0,.93)
    rim=mat('Anodized dark lens surround',(.067,.073,.079),.44,.36)
    concrete=mat('Concrete foundation',(.43,.445,.45),.0,.86)
    red=mat('Red LED 625 nm',(.55,.018,.008),.0,.23,emit=(1,.018,.008),strength=.26)
    amber=mat('Unlit amber LED 590 nm',(.18,.095,.014),.0,.29)
    green=mat('Unlit green LED 525 nm',(.018,.085,.036),.0,.29)
    darkred=mat('Red unlit micro LEDs',(.22,.022,.019),.15,.44)
    cube('C00 buried base just above grade',(0,0,.055),(.36,.40,.11),concrete,p,.022)
    cylinder('P00 steel upright',(0,.045,1.68),.049,3.30,steel,p)
    cylinder('P01 base flange',(0,.045,.135),.12,.021,steel,p)
    for ix in (-.077,.077):
        for iy in (-.077,.077):
            cylinder('P02 M10 anchor head',(ix,.045+iy,.153),.012,.015,steel,p,12)
    for z in (2.71,3.38):
        cube('P03 clamp bracket',(0,-.015,z),(.20,.17,.058),steel,p,.015)
        for x in (-.075,.075):
            cylinder('P04 clamp bolt',(x,-.114,z),.012,.017,steel,p,12,axis=(0,1,0))
    cube('S00 IP65 three aspect case',(0,-.145,3.03),(.352,.239,.875),graphite,p,.045)
    cube('S01 rear hinged service cover',(0,-.008,3.03),(.314,.026,.826),graphite,p,.018)
    for z,glass in ((3.31,red),(3.03,amber),(2.75,green)):
        cylinder('S02 sealed black lamp pod',(0,-.277,z),.139,.027,black,p,48,axis=(0,1,0))
        torus('S03 circular bezel',(0,-.302,z),.124,.010,rim,p)
        cylinder('S04 LED lens diffuser',(0,-.309,z),.105,.009,glass,p,56,axis=(0,1,0))
        # Honeycomb-like insert of physical dots, no decal billboards.
        for row in range(-4,5):
            for col in range(-4,5):
                x=col*.021+(row%2)*.010
                zz=row*.020
                if x*x+zz*zz < .087**2:
                    cylinder('S05 individual reflector bead',(x,-.319,z+zz),.0045,.003,
                             darkred if glass==red else glass,p,8,axis=(0,1,0))
        # Half-round visors form actual solid shell, open below.
        n=24
        vs=[]
        for y,r in ((-.297,.148),(-.418,.163)):
            for i in range(n+1):
                t=math.pi*i/n
                vs.append((math.cos(t)*r,y,z+math.sin(t)*r))
        fs=[(i,i+1,n+2+i,n+1+i) for i in range(n)]
        mesh=bpy.data.meshes.new('hood cross-section mesh')
        mesh.from_pydata(vs,[],fs);mesh.update()
        o=bpy.data.objects.new('S06 curved rain hood over each lens',mesh)
        bpy.context.collection.objects.link(o)
        make(o,o.name,graphite,p)
    cube('S07 small serial plate',(0,.002,2.71),(.155,.004,.035),steel,p,.005)
    publish('r7a-sygnalizator-led-3',p,{'width':.40,'depth':.46,'height':3.47},
            3.65,(0,0,1.8))

def bench():
    p=reset()
    steel=mat('Anthracite powdercoat RAL 7016',(.074,.082,.09),.7,.34)
    zinc=mat('316 stainless fasteners',(.65,.68,.69),.87,.29)
    rubber=mat('Neoprene foot pads',(.036,.043,.047),0,.84)
    wood=mat('Thermally treated oak',(.58,.30,.13),0,.62)
    img=wood_tex('original oak texture')
    bsdf=wood.node_tree.nodes.get('Principled BSDF')
    node=wood.node_tree.nodes.new('ShaderNodeTexImage');node.image=img
    wood.node_tree.links.new(node.outputs['Color'],bsdf.inputs['Base Color'])
    for x in (-.70,.70):
        cube('F00 cast ground shoe',(x,0,.035),(.18,.40,.066),steel,p,.023)
        cube('F01 anti-corrosion gasket',(x,0,.008),(.18,.40,.015),rubber,p,.007)
        for y in (-.135,.135):
            cylinder('F02 zinc M12 anchor',(x,y,.074),.012,.013,zinc,p,12)
        pipe('F03 front welded upright',(x,-.18,.095),(x,-.18,.455),.030,steel,p)
        pipe('F04 angled rear support',(x,.17,.095),(x,.25,.88),.029,steel,p)
        pipe('F05 seat crossbeam',(x,-.24,.413),(x,.24,.413),.027,steel,p)
        pipe('F06 diagonal bracing',(x,-.19,.22),(x,.19,.48),.022,steel,p)
        pipe('F07 curved handrail proxy',(x,-.23,.625),(x,.18,.645),.028,steel,p)
        pipe('F08 armrest front support',(x,-.22,.455),(x,-.22,.625),.022,steel,p)
    # Four full-scale hardwood seat slats with shadow gaps, chamfers and bolts.
    for n,y in enumerate((-.195,-.075,.045,.165)):
        cube('W00 hardwood seat plank '+str(n),(0,y,.468),
             (1.80,.109,.041),wood,p,.013)
        for x in (-.73,.73):
            for offset in (-.029,.029):
                cylinder('W01 countersunk screw',(x,y+offset,.490),.006,.004,zinc,p,12)
    for n,z in enumerate((.61,.725,.84)):
        o=cube('W02 upper backrest plank '+str(n),(0,.277+(z-.61)*.10,z),
               (1.80,.048,.095),wood,p,.011)
        o.rotation_euler[0]=-.095
        for x in (-.70,.70):
            cylinder('W03 countersunk back screw',(x,.247+(z-.61)*.10,z),
                     .007,.005,zinc,p,12,axis=(0,1,0))
    cube('F09 steel diagonal underseat rail',(0,-.018,.399),(1.48,.036,.036),steel,p,.011)
    publish('r7a-lawka-miejska-drewno',p,
            {'width':1.84,'depth':.50,'height':.90},1.8,(0,0,.48))

signal()
bench()
print('PRAWKO_BLENDER_ROADPACK01_SUCCESS models=2',flush=True)
