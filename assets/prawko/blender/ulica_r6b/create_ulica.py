# SPDX-License-Identifier: Apache-2.0
"""R6b original full-geometry street-front assets for PrawkoRoadStudio.
All assets are Blender-authored, metres, Z-up; GLB contains meshes only.
"""
import bpy, math, json, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
rng=random.Random(20261116)
COL=None
def reset(name):
    global COL
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    COL=bpy.data.collections.new(name+' EXPORT')
    bpy.context.scene.collection.children.link(COL)
def mat(name,c,rough=.9,metal=0):
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.use_nodes=True
    m.diffuse_color=(*c,1)
    bs=m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value=(*c,1)
    bs.inputs['Roughness'].default_value=rough
    bs.inputs['Metallic'].default_value=metal
    return m
def own(o):
    for c in list(o.users_collection):c.objects.unlink(o)
    COL.objects.link(o)
def box(name,pos,dim,m,b=.0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=pos)
    o=bpy.context.object;o.name=name;o.dimensions=dim
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if b:
        bevel=o.modifiers.new('True chamfer','BEVEL');bevel.width=b;bevel.segments=2
        bpy.ops.object.modifier_apply(modifier=bevel.name)
    o.data.materials.append(m);own(o)
def pipe(name,start,end,r,m):
    a=Vector(start);b=Vector(end)
    bpy.ops.mesh.primitive_cylinder_add(vertices=12,radius=r,depth=(b-a).length,location=(a+b)/2)
    o=bpy.context.object;o.name=name
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler()
    o.data.materials.append(m);own(o)
def ico(name,pos,scale,m,subdiv=2):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdiv,radius=1,location=pos)
    o=bpy.context.object;o.name=name;o.scale=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.data.materials.append(m);own(o)
def finalize(stem,label,camera,look,ortho):
    groups={}
    for o in list(COL.objects):
        assert o.type=='MESH' and o.active_material,o.name
        groups.setdefault(o.active_material.name,[]).append(o)
    for name,objects in groups.items():
        bpy.ops.object.select_all(action='DESELECT')
        for o in objects:o.select_set(True)
        bpy.context.view_layer.objects.active=objects[0]
        bpy.ops.object.join()
        bpy.context.object.name='R6B '+name
    parts=list(COL.objects)
    assert 2<=len(parts)<=20,(label,len(parts))
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    glb=ROOT/(stem+'.glb')
    bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',
        use_selection=True,export_apply=True,export_materials='EXPORT',
        export_normals=True,export_texcoords=True,export_cameras=False,export_lights=False)
    # Camera/lights/preview-only ground never exported.
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.object.camera_add(location=camera)
    cam=bpy.context.object
    cam.rotation_euler=(Vector(look)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.type='ORTHO';cam.data.ortho_scale=ortho
    bpy.context.scene.camera=cam
    bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,-.05))
    ground=bpy.context.object;ground.name='PREVIEW GROUND ONLY'
    ground.scale=(20,15,1)
    ground.data.materials.append(mat('R6B Preview ground',(.32,.33,.32)))
    for pos,power,diam in [((9,-10,20),2700,12),((-9,8,12),1300,9)]:
        bpy.ops.object.light_add(type='AREA',location=pos)
        l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=diam
        l.rotation_euler=(Vector(look)-l.location).to_track_quat('-Z','Y').to_euler()
    scene=bpy.context.scene
    scene.render.engine='CYCLES';scene.cycles.samples=18
    scene.render.resolution_x=1000;scene.render.resolution_y=650
    scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'
    scene.render.filepath=str(ROOT/(stem+'-blender.png'))
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/(stem+'.blend')))
    bpy.ops.render.render(write_still=True)
    lo=[min((o.matrix_world@Vector(v))[i] for o in parts for v in o.bound_box) for i in range(3)]
    hi=[max((o.matrix_world@Vector(v))[i] for o in parts for v in o.bound_box) for i in range(3)]
    data={'title':label,'dimensions_m':[round(hi[i]-lo[i],3) for i in range(3)],
          'parts':len(parts),'units':'metres','axes':'Z up','blender_version':bpy.app.version_string,
          'materials':[o.active_material.name for o in parts],
          'note':'GLB importer centers XY at bounding-box center; z=0 at lowest part'}
    (ROOT/(stem+'-metadata.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    assert glb.stat().st_size>15000
    print('R6B_ASSET_PASS',stem,json.dumps(data,ensure_ascii=False))

# Part 1: tactile real 0.75-m pavement tiles, joints, stone edging, drain grates.
reset('R6B Chodnik miejski')
foundation=mat('R6B podbudowa kruszywo',(.32,.33,.32))
pave=[mat('R6B plyta betonowa %d'%i,v) for i,v in enumerate(
    [(.54,.54,.51),(.57,.57,.53),(.51,.52,.50),(.59,.58,.54)])]
edge=mat('R6B krawedz piaskowca',(.64,.63,.59))
iron=mat('R6B wpust grafit',(.20,.22,.22),.56,.62)
box('Podbudowa caly przekroj',(0,0,.055),(20,2.26,.11),foundation,.012)
for i in range(26):
    for j in range(4):
        x=(i-12.5)*.75;y=(j-1.5)*.497
        box(f'Plyta {i}-{j}',(x,y,.14),(.724,.476,.074),pave[(i*7+j*11)%4],.006)
for y in (-1.09,1.09):
    box('Granitowa listwa brzegowa '+str(y),(0,y,.119),(20,.18,.16),edge,.015)
for x in (-9.81,9.81):
    box('Czolowe obrzeze '+str(x),(x,0,.12),(.25,2.21,.155),edge,.008)
for x in (-6.1,1.35,8.15):
    box('Korpus odwodnienia '+str(x),(x,1.02,.186),(.59,.16,.023),iron,.006)
    for t in range(6):
        box('Zebro kraty %s-%s'%(x,t),(x-.23+t*.091,1.02,.207),(.018,.17,.016),edge,.002)
finalize('r6b-chodnik-20m','R6b Chodnik kamienny 20 m',(16,-18,14),(0,0,.05),24)

# Part 2: full 3D city tree with compact stone planter, bark, branches and leaves.
reset('R6B drzewo uliczne')
bark=mat('R6B kora',(.24,.19,.15),.97)
earth=mat('R6B ziemia urodzajna',(.19,.17,.13))
granite=mat('R6B kamienne obrzeze misy',(.57,.56,.54))
foliage=[mat('R6B ulistnienie %d'%i,c) for i,c in enumerate([
    (.23,.32,.22),(.31,.39,.25),(.28,.37,.24),(.36,.42,.29)])]
grass=mat('R6B zdzbla trawy',(.29,.37,.24))
box('Misa glebowa',(0,0,.065),(1.93,1.93,.13),earth,.028)
for y in (-.90,.90):box('Kamienny brzeg '+str(y),(0,y,.13),(2.04,.20,.25),granite,.014)
for x in (-.90,.90):box('Kamienny brzeg '+str(x),(x,0,.13),(.20,1.84,.25),granite,.014)
bpy.ops.mesh.primitive_cone_add(vertices=16,radius1=.21,radius2=.12,depth=3.7,location=(0,0,2.01))
trunk=bpy.context.object;trunk.name='Pien drzewa';trunk.data.materials.append(bark);own(trunk)
for i,(a,b) in enumerate([
    ((0,0,2.4),(-1.05,-.3,4.55)),((0,0,2.8),(1.05,-.35,4.8)),
    ((0,0,3.0),(-.35,.84,5.12)),((0,0,3.0),(.68,.78,5.34))]):
    pipe('Konar '+str(i),a,b,.078,bark)
# Irregular multi-cluster crown instead of 7 spherical toy-like lobes.
# Compact leaf clusters cast real shadow and are merged by 4 materials for GPU.
for i in range(42):
    azimuth=rng.uniform(0,math.tau)
    radius=rng.uniform(.25,1.55)
    z=rng.uniform(4.55,6.95)
    x=math.cos(azimuth)*radius
    y=math.sin(azimuth)*radius*.80
    crown_scale=(rng.uniform(.37,.69),rng.uniform(.35,.64),rng.uniform(.34,.76))
    ico('Rozgalęziona korona %02d'%i,(x,y,z),crown_scale,foliage[i%4],2)
    if i<13:
        pipe('Drobna galaz %02d'%i,(0,0,3.45),(x*.82,y*.82,z*.80),.028,bark)
# Real crossed slender triangles, no billboard texture.
vertices=[];faces=[]
for i in range(95):
    x=rng.uniform(-.79,.79);y=rng.uniform(-.79,.79)
    height=rng.uniform(.12,.36);angle=rng.random()*math.tau
    for j in range(3):
        a=angle+j*math.pi/3;dx=.038*math.cos(a);dy=.038*math.sin(a)
        k=len(vertices)
        vertices +=[(x-dx,y-dy,.14),(x+dx,y+dy,.14),(x,y,.14+height)]
        faces.append((k,k+1,k+2))
mesh=bpy.data.meshes.new('Prawdziwe trojkaty zdzbel')
mesh.from_pydata(vertices,[],faces);mesh.update()
o=bpy.data.objects.new('Gesta trawa wokol drzewa',mesh);COL.objects.link(o);o.data.materials.append(grass)
finalize('r6b-drzewo','R6b Drzewo przyuliczne z misa',(10,-14,10),(0,0,3.5),12)

# Part 3: independent 5.8m pole with luminaire (visual mesh only, no GPU light).
reset('R6B latarnia uliczna')
steel=mat('R6B latarnia metal antracyt',(.17,.18,.18),.43,.75)
detail=mat('R6B klamry metaliczne',(.39,.40,.39),.52,.66)
lens=mat('R6B klosz szklo jasne',(.69,.68,.59),.25,.05)
box('Stopa fundamentowa',(0,0,.14),(.43,.43,.28),steel,.023)
bpy.ops.mesh.primitive_cone_add(vertices=16,radius1=.12,radius2=.054,depth=5.28,location=(0,0,2.92))
o=bpy.context.object;o.name='Zwezajacy sie stalowy slup'
o.data.materials.append(steel);own(o)
for z in (.30,5.48):pipe('Kolnierz '+str(z),(0,0,z-.04),(0,0,z+.04),.138,detail)
pipe('Wygięty wysiegnik',(0,0,5.45),(0,1.34,5.65),.064,steel)
box('Oprawa uliczna',(0,1.50,5.60),(.29,.92,.20),steel,.03)
box('Mleczny klosz dolny',(0,1.50,5.484),(.21,.76,.039),lens,.006)
for y in (1.12,1.89):box('Mocowanie oprawy '+str(y),(0,y,5.61),(.31,.04,.22),detail,.005)
finalize('r6b-latarnia','R6b Latarnia uliczna',(8,-10,9),(0,.4,3),12)
print('PRAWKO_R6B_THREE_EXPORTS_PASS')
