# SPDX-License-Identifier: Apache-2.0
"""R6 urban building: authored geometry, meters, Z-up, facade faces -Y."""
import bpy, json, math, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
rng=random.Random(20261116)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
collection=bpy.data.collections.new("Prawko | R6 Kamienica | EXPORT")
bpy.context.scene.collection.children.link(collection)
def material(name,color,metal=0,rough=.8,alpha=1,transmission=0):
    m=bpy.data.materials.new(name)
    m.use_nodes=True
    m.diffuse_color=(*color,alpha)
    b=m.node_tree.nodes.get('Principled BSDF')
    for key,value in [('Base Color',(*color,1)),('Metallic',metal),('Roughness',rough),('Alpha',alpha),('Transmission Weight',transmission),('IOR',1.45)]:
        b.inputs[key].default_value=value
    if alpha<1:
        try:m.surface_render_method='DITHERED'
        except AttributeError:pass
    return m
wall=material('Tynk mineralny',(.62,.615,.56),rough=.96)
sand=material('Kamienny jasny gzyms',(.72,.70,.64),rough=.9)
concrete=material('Cokol beton ciemnoszary',(.25,.27,.28),rough=.93)
frame=material('Aluminiowe ramy antracyt',(.11,.12,.13),metal=.65,rough=.38)
glass=material('Szklo ciemne fizycznie przyblizone',(.18,.27,.31),rough=.16,alpha=.57,transmission=.42)
steel=material('Balustrada stal ocynk',(.40,.42,.42),metal=.75,rough=.32)
wood=material('Drewno drzwi',(.27,.19,.14),rough=.64)
black=material('Wnetrze i wglebienia',(.07,.09,.10),rough=.98)
roof=material('Papa dachowa',(.18,.19,.19),rough=.97)
def texture():
    n=256
    image=bpy.data.images.new('Autorska ziarnista faktura tynku',width=n,height=n,alpha=True)
    p=[0.0]*(n*n*4)
    for y in range(n):
        for x in range(n):
            grain=rng.uniform(-.045,.045)+.012*math.sin(x*.16+y*.21)
            color=[max(0,min(1,v+grain)) for v in (.65,.635,.58)]
            p[(y*n+x)*4:(y*n+x)*4+4]=color+[1]
    image.pixels[:]=p
    image.filepath_raw=str(ROOT/'r6_tynk_albedo.png')
    image.file_format='PNG'
    image.save()
    b=wall.node_tree.nodes.get('Principled BSDF')
    node=wall.node_tree.nodes.new('ShaderNodeTexImage')
    node.image=image
    wall.node_tree.links.new(node.outputs['Color'],b.inputs['Base Color'])
texture()
def move(o):
    for c in list(o.users_collection): c.objects.unlink(o)
    collection.objects.link(o)
def box(name,loc,dim,mat,bevel=0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    o=bpy.context.object;o.name=name
    o.dimensions=dim
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        m=o.modifiers.new('Zaokraglone krawedzie','BEVEL')
        m.width=bevel;m.segments=2
        bpy.ops.object.modifier_apply(modifier=m.name)
    o.data.materials.append(mat);move(o)
    return o
def pipe(name,loc,radius,height,mat):
    bpy.ops.mesh.primitive_cylinder_add(vertices=12,radius=radius,depth=height,location=loc)
    o=bpy.context.object;o.name=name;o.data.materials.append(mat);move(o)
# Shell 13.8 x 9 x 12.2 m, front face y=0, pivot at ground.
box('Bryla pelnej elewacji',(0,4.5,6.1),(13.8,9,12.2),wall,.025)
box('Cokol front',(0,-.04,.35),(14.1,.13,.70),concrete,.015)
box('Przyziemie stopa',(0,4.5,.14),(14.15,9.20,.27),sand,.025)
box('Stropodach',(0,4.5,12.4),(14.3,9.3,.28),roof,.025)
for name,loc,dim in [('Front',(0,-.15,12.83),(14.35,.24,.85)),
                     ('Tyl',(0,9.1,12.83),(14.35,.24,.85)),
                     ('Lewo',(-7.07,4.5,12.83),(.22,9.35,.85)),
                     ('Prawo',(7.07,4.5,12.83),(.22,9.35,.85))]:
    box('Attyka '+name,loc,dim,sand,.01)
for level in [3.45,6.25,9.09,11.92]:
    box('Wysuniety gzyms '+str(level),(0,-.19,level),(14.1,.33,.13),sand,.018)
for x in [-6.62,6.62]:
    box('Pilaster pionowy '+str(x),(x,-.18,6.2),(.31,.33,10.9),sand,.015)
# Deep modelled window reveals, glass, lintels, sills, three floors.
def window(x,z,balcony=False):
    w,h=1.45,1.72
    name=f'Okno {x} {z}'
    box(name+' wneka',(x,-.105,z),(w+.20,.20,h+.20),black)
    box(name+' szklo',(x,-.215,z),(w,.045,h),glass,.006)
    for a in [-w/2,w/2]:
        box(name+f' futryna {a}',(x+a,-.27,z),(.068,.095,h+.13),frame,.007)
    for a in [-h/2,h/2]:
        box(name+f' gora dol {a}',(x,-.275,z+a),(w+.08,.095,.068),frame,.007)
    box(name+' szpros pion',(x,-.29,z),(.038,.07,h),frame)
    box(name+' szpros poziom',(x,-.29,z+.23),(w,.07,.035),frame)
    box(name+' parapet',(x,-.32,z-h/2-.10),(w+.30,.42,.12),sand,.013)
    box(name+' nadproze',(x,-.24,z+h/2+.135),(w+.20,.22,.12),sand,.014)
    if balcony:
        bz=z-h/2-.24
        box(name+' plyta balkon',(x,-.82,bz),(2.12,1.5,.17),sand,.022)
        for a in [-.96,-.7,-.45,-.23,.23,.45,.7,.96]:
            box(name+f' szczebel {a}',(x+a,-1.50,bz+.52),(.019,.03,.95),steel)
        for a in [-.95,0,.95]:
            pipe(name+f' slup {a}',(x+a,-1.50,bz+.53),.025,1.0,steel)
        box(name+' porecz',(x,-1.50,bz+1.05),(2.1,.065,.065),steel,.008)
for level in [4.45,7.25,10.04]:
    for x in [-5.0,-2.5,0.0,2.5,5.0]:
        window(x,level,balcony=(level==7.25 and abs(x)==2.5))
# Real ground-floor shop windows, door and a canopy.
for x in [-5.0,-3.0,3.0,5.0]:
    box(f'Witryna {x} gleboka',(x,-.11,1.89),(1.72,.23,2.1),black)
    box(f'Witryna {x} szklo',(x,-.23,1.89),(1.59,.045,1.98),glass,.006)
    for a in [-.81,.81]:
        box(f'Witryna {x} rama {a}',(x+a,-.30,1.89),(.075,.10,2.12),frame)
    box(f'Witryna {x} prog',(x,-.33,.82),(1.8,.32,.085),sand)
box('Wejscie wnekowe',(0,-.135,1.73),(2.2,.25,3.00),black)
box('Drzwi drewniane',(0,-.29,1.60),(1.67,.12,2.75),wood,.018)
box('Nadswietle drzwi',(0,-.38,2.80),(1.60,.04,.31),glass)
for x in [-.87,.87]:
    box('Drzwi oscierznica '+str(x),(x,-.41,1.70),(.085,.14,3.0),frame)
pipe('Klamka',(.65,-.46,1.42),.028,.11,steel)
box('Stopien do drzwi',(0,-.69,.13),(2.25,1.12,.26),sand,.022)
# Roofing drains, rain gutters and brackets.
for x in [-6.85,6.85]:
    pipe('Rura spustowa '+str(x),(x,-.47,5.9),.064,11.8,steel)
    for z in [1.5,3.5,5.5,7.5,9.5,11.0]:
        box(f'Obejma rynny {x} {z}',(x,-.48,z),(.19,.13,.07),frame,.008)
for x in [-3.0,3.0]:
    box('Wentylacja dach '+str(x),(x,4.2,13.1),(.72,.75,.85),steel,.03)
    box('Czapka wentylacji '+str(x),(x,4.2,13.57),(.95,.98,.14),roof,.02)
# Material variation at the corners plus side-elevation recesses.
for z in [4.3,7.1,9.8]:
    for y in [2.5,6.5]:
        for x,sign in [(-6.96,-1),(6.96,1)]:
            box(f'Boczne okno nisza {x} {y} {z}',(x+sign*.06,y,z),(.16,1.5,1.70),black)
            box(f'Boczne okno szklo {x} {y} {z}',(x+sign*.14,y,z),(.055,1.38,1.56),glass)
            box(f'Boczne okno parapet {x} {y} {z}',(x+sign*.23,y,z-.9),(.32,1.67,.11),sand)
# R6 importer budget: <=256 separate mesh parts. Merge hundreds of window
# details BY identical PBR material, preserving true geometry and authored UVs.
# This lowers GPU draw calls without flattening the facade to fake textures.
by_material={}
for obj in list(collection.objects):
    if obj.type=='MESH':
        by_material.setdefault(obj.active_material.name,[]).append(obj)
for mat_name,group in by_material.items():
    bpy.ops.object.select_all(action='DESELECT')
    for obj in group: obj.select_set(True)
    bpy.context.view_layer.objects.active=group[0]
    bpy.ops.object.join()
    bpy.context.object.name='R6_geometry_'+mat_name
# Authoring proof: GLB contains only the actual building.
entities=list(collection.objects)
assert len(entities)<20, f'R6_parts_budget_exceeded:{len(entities)}'
bpy.ops.object.select_all(action='DESELECT')
for o in entities:o.select_set(True)
bpy.context.view_layer.objects.active=entities[0]
glb=ROOT/'r6_kamienica_miejska.glb'
bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',
                          use_selection=True,export_apply=True,
                          export_materials='EXPORT',export_normals=True,
                          export_texcoords=True,export_cameras=False,export_lights=False)
# Blender source contains an independent grounded preview (not in GLB).
bpy.ops.object.select_all(action='DESELECT')
bpy.ops.mesh.primitive_plane_add(size=2,location=(0,3,-.04))
ground=bpy.context.object
ground.name='PREVIEW ONLY ground'
ground.scale=(18,15,1)
ground.data.materials.append(sand)
bpy.ops.object.camera_add(location=(20,-28,17))
cam=bpy.context.object
cam.rotation_euler=(Vector((0,3,6))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=26
bpy.context.scene.camera=cam
for loc,power,size in [((0,-13,27),2600,12),((12,8,18),1400,10)]:
    bpy.ops.object.light_add(type='AREA',location=loc)
    light=bpy.context.object
    light.data.energy=power;light.data.shape='DISK';light.data.size=size
    light.rotation_euler=(Vector((0,3,6))-light.location).to_track_quat('-Z','Y').to_euler()
scene=bpy.context.scene
scene.render.engine='CYCLES'
scene.cycles.samples=20
scene.render.resolution_x=1200;scene.render.resolution_y=800
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(ROOT/'r6_kamienica_blender_preview.png')
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'r6_kamienica_miejska.blend'))
bpy.ops.render.render(write_still=True)
mins=[min((o.matrix_world@Vector(v))[i] for o in entities for v in o.bound_box) for i in range(3)]
maxs=[max((o.matrix_world@Vector(v))[i] for o in entities for v in o.bound_box) for i in range(3)]
metadata={'title':'R6 Kamienica Miejska','units':'metres','axes':'Z-up, front -Y',
          'dimensions_m':[round(maxs[i]-mins[i],3) for i in range(3)],
          'objects':len(entities),'blender_version':bpy.app.version_string,
          'glb':glb.name,'preview':'r6_kamienica_blender_preview.png'}
(ROOT/'model-metadata.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2),encoding='utf-8')
print('PRAWKO_R6_BUILDING_BLENDER_PASS '+json.dumps(metadata))
