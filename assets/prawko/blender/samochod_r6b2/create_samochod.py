# SPDX-License-Identifier: Apache-2.0
"""R6b2 original compact road car; metres, Z-up, +X forward, bounds 4.58x1.83x1.44.
Exported GLB contains only 3D geometry; preview cameras/lighting never leave .blend.
"""
import bpy,math,random,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
collection=bpy.data.collections.new('R6B2 SAMOCHOD GLB')
bpy.context.scene.collection.children.link(collection)
def mat(name,rgb,metal=0.0,rough=.52):
 m=bpy.data.materials.new(name);m.use_nodes=True;m.diffuse_color=(*rgb,1)
 p=m.node_tree.nodes.get('Principled BSDF')
 p.inputs['Base Color'].default_value=(*rgb,1);p.inputs['Metallic'].default_value=metal
 p.inputs['Roughness'].default_value=rough
 return m
paint=mat('Lakier granatowy',(.095,.155,.23),.49,.29)
black=mat('Uszczelki gumowe i zderzak',(.055,.064,.07),.08,.78)
tire=mat('Opony gumowe',(.035,.039,.041),.03,.94)
silver=mat('Felgi szczotkowane aluminium',(.55,.58,.60),.72,.30)
glass=mat('Ciemne szklo samochodowe',(.20,.285,.34),.22,.16)
lights=mat('Reflektory biale',(.82,.85,.83),.14,.23)
red=mat('Lampy tylne',(.64,.09,.07),.10,.24)
bodytrim=mat('Detale nadwozia',(.25,.30,.32),.6,.40)
def own(o):
 for c in list(o.users_collection):c.objects.unlink(o)
 collection.objects.link(o)
def block(name,loc,dim,m,bev=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
 o=bpy.context.object;o.name=name;o.dimensions=dim
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bev:
  v=o.modifiers.new('Prawdziwe fazy karoserii','BEVEL');v.width=bev;v.segments=3
  bpy.ops.object.modifier_apply(modifier=v.name)
 o.data.materials.append(m);own(o)
 return o
def profile(name,points,halfwidth,m):
 # Irregular 2D profile extruded along Y, positive x = car front.
 verts=[(x,-halfwidth,z) for x,z in points]+[(x,halfwidth,z) for x,z in points]
 n=len(points);faces=[]
 faces.append(tuple(range(n-1,-1,-1)))
 faces.append(tuple(range(n,n*2)))
 for i in range(n):
  j=(i+1)%n;faces.append((i,j,j+n,i+n))
 mesh=bpy.data.meshes.new(name+' mesh');mesh.from_pydata(verts,[],faces);mesh.update()
 o=bpy.data.objects.new(name,mesh);collection.objects.link(o)
 o.data.materials.append(m)
 for p in mesh.polygons:p.use_smooth=False
 return o
def panel(name,x0,x1,z0,z1,y,m):
 block(name,((x0+x1)/2,y,(z0+z1)/2),(x1-x0,.015,z1-z0),m,.002)
def wheel(name,x,y):
 # Solid tire cylinder with axis along Y; wheel does not float above street.
 bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=.320,depth=.235,location=(x,y,.325))
 o=bpy.context.object;o.name=name+' gumowa opona';o.rotation_euler=(math.pi/2,0,0)
 o.data.materials.append(tire);own(o)
 sy=1 if y>0 else -1
 bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=.208,depth=.018,location=(x,y+sy*.123,.325))
 o=bpy.context.object;o.name=name+' felga 18';o.rotation_euler=(math.pi/2,0,0)
 o.data.materials.append(silver);own(o)
 bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=.068,depth=.029,location=(x,y+sy*.140,.325))
 o=bpy.context.object;o.name=name+' piasta';o.rotation_euler=(math.pi/2,0,0)
 o.data.materials.append(black);own(o)
 # Five physical spokes. No decal textures.
 for i in range(5):
  a=i*math.tau/5
  x2=x+math.cos(a)*.117;z2=.325+math.sin(a)*.117
  block(name+' szprycha '+str(i),(x2,y+sy*.15,z2),(.068,.023,.034),silver,.009)
# Outer shell at correct width and wheel arch level. True faceted body geometry.
profile('Nadwozie przedni pas i maska',[
 (-2.20,.32),(-2.29,.49),(-2.20,.79),(-1.70,.89),(-.68,.91),(.65,.91),
 (1.54,.88),(2.20,.70),(2.29,.55),(2.29,.34),(1.67,.30),(-1.70,.30)
 ],.79,paint)
profile('Niski dolny prog i zderzaki',[
 (-2.27,.24),(-2.29,.32),(-1.7,.32),(1.67,.32),(2.29,.34),
 (2.29,.24),(1.61,.18),(-1.60,.18)],.75,black)
# Cabin tapers toward roof, with angled front and rear pillars rather than a cube.
profile('Dach i slupek karoserii',[
 (-1.45,.89),(-.78,1.35),(-.45,1.43),(.88,1.43),(1.26,1.22),
 (1.45,.89)],.67,paint)
profile('Zespol przednia szyba',[
 (.89,1.377),(1.218,1.18),(1.36,.929),(1.18,.929)],.642,glass)
profile('Zespol tylna szyba',[
 (-1.395,.924),(-.8,1.30),(-.55,1.377),(-.48,1.35),(-1.215,.924)],.632,glass)
# Side glass panels, real lateral glass surfaces + B pillar.
for sign in (-1,1):
 y=sign*.673
 # Driver left/right main glass panes follow lower roof line.
 profile('Boczna szyba placeholder '+str(sign),[
    (-.76,1.27),(-.45,1.38),(.11,1.38),(.11,.995),
    (-1.12,.995)],.001,glass).location.y=y
 profile('Przednia szyba boczna '+str(sign),[
    (.16,1.379),(.81,1.379),(1.185,1.18),(1.35,.995),(.16,.995)],.001,glass).location.y=y+sign*.012
 block('Slupek B '+str(sign),(.14,y+sign*.015,1.185),(.064,.05,.43),black,.008)
 block('Listwa uszczelek '+str(sign),(-.01,y+sign*.02,.989),(2.60,.035,.027),black,.004)
 for x in (-1.34,.19):
  block('Klamka zamek '+str(sign)+str(x),(x,sign*.801,.83),(.15,.028,.025),silver,.005)
 block('Lusterko boczne '+str(sign),(1.08,sign*.852,1.038),(.22,.12,.112),black,.027)
 block('Szklo lusterka '+str(sign),(1.065,sign*.918,1.039),(.161,.009,.075),silver,.01)
# Front/rear lamps, grille, plates, bumper elements.
for sign in (-1,1):
 block('Reflektor przod '+str(sign),(2.258,sign*.545,.655),(.044,.297,.102),lights,.019)
 block('Lampa tyl '+str(sign),(-2.256,sign*.560,.655),(.045,.262,.113),red,.021)
 block('Blotnik nad kolem przod '+str(sign),(1.32,sign*.788,.64),(.88,.025,.059),paint,.020)
 block('Blotnik nad kolem tyl '+str(sign),(-1.35,sign*.788,.65),(.88,.025,.045),paint,.018)
for x,label in [(2.283,'Przednia'),(-2.283,'Tylna')]:
 block(label+' tablica rejestracyjna',(x,0,.51),(.018,.43,.105),lights,.006)
for y in (-.15,.15):
 block('Wlot powietrza atrapa '+str(y),(2.287,y,.420),(.024,.13,.078),black,.009)
for x in (-1.36,1.34):
 for y in (-.798,.798):wheel('Kolo '+str(x)+str(y),x,y)
# Exact size limits, all geometry retained in export.
objects=list(collection.objects)
# Per-material join to control GL 3.3 draw calls; ensure object transforms are applied.
for name in sorted(set(o.active_material.name for o in objects)):
 group=[o for o in collection.objects if o.active_material.name==name]
 bpy.ops.object.select_all(action='DESELECT')
 for o in group:o.select_set(True)
 bpy.context.view_layer.objects.active=group[0]
 bpy.ops.object.join()
 bpy.context.object.name='Samochod material '+name
bpy.ops.object.select_all(action='DESELECT')
parts=list(collection.objects)
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0]
dst=ROOT/'r6b2-samochod.glb'
bpy.ops.export_scene.gltf(filepath=str(dst),export_format='GLB',use_selection=True,
  export_apply=True,export_normals=True,export_texcoords=True,export_materials='EXPORT',
  export_cameras=False,export_lights=False)
# Standalone Cycles preview for QA; no ground/camera or lamp exported to GLB.
bpy.ops.object.select_all(action='DESELECT')
bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,-.025))
ground=bpy.context.object;ground.scale=(9,7,1)
ground.data.materials.append(mat('Preview dark tarmac',(.27,.28,.28)))
bpy.ops.object.camera_add(location=(7,-8,4))
cam=bpy.context.object
cam.rotation_euler=(Vector((0,0,.8))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=7.0
bpy.context.scene.camera=cam
for p,power,diam in [((1,-8,9),2100,7),((-5,4,7),1450,6)]:
 bpy.ops.object.light_add(type='AREA',location=p)
 l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=diam
 l.rotation_euler=(Vector((0,0,.7))-l.location).to_track_quat('-Z','Y').to_euler()
scene=bpy.context.scene
scene.render.engine='CYCLES';scene.cycles.samples=28
scene.render.resolution_x=1200;scene.render.resolution_y=800
scene.render.resolution_percentage=100
scene.render.filepath=str(ROOT/'r6b2-samochod-blender.png')
scene.render.image_settings.file_format='PNG'
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'r6b2-samochod.blend'))
bpy.ops.render.render(write_still=True)
lo=[min((o.matrix_world@Vector(v))[i] for o in parts for v in o.bound_box) for i in range(3)]
hi=[max((o.matrix_world@Vector(v))[i] for o in parts for v in o.bound_box) for i in range(3)]
dimensions=[round(hi[i]-lo[i],3) for i in range(3)]
assert 4.56<=dimensions[0]<=4.60 and 1.82<=dimensions[1]<=1.95 and 1.40<=dimensions[2]<=1.50,dimensions # width including mirrors and wheel-hub trim
metadata={'name':'R6b2 compact sedan','dimensions_m':dimensions,'parts':len(parts),
          'units':'metres','forward':'+X','up':'+Z','blender':bpy.app.version_string,
          'glb_bytes':dst.stat().st_size,'orientation':'fixed normalized actor bounding box',
          'material_count':len(parts)}
(ROOT/'r6b2-samochod-metadata.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2),encoding='utf-8')
print('R6B2_CAR_BLENDER_EXPORT_PASS '+json.dumps(metadata,ensure_ascii=False))
