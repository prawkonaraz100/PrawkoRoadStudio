# SPDX-License-Identifier: Apache-2.0
"""R6b2b: original editable 3D verges, courtyard garden, and gravel walkway.
Blender 5.3, metres, +Z up, XY-centred. All source geometry, no stock imagery.
"""
import bpy, json, math, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
R=random.Random(624162026)
COL=None
def reset(label):
 global COL
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 COL=bpy.data.collections.new(label+' GLB ONLY')
 bpy.context.scene.collection.children.link(COL)
def mat(name,rgb,rough=.95,metal=.0):
 m=bpy.data.materials.new('R6b2b '+name)
 m.use_nodes=True;m.diffuse_color=(*rgb,1)
 node=m.node_tree.nodes.get('Principled BSDF')
 node.inputs['Base Color'].default_value=(*rgb,1)
 node.inputs['Roughness'].default_value=rough
 node.inputs['Metallic'].default_value=metal
 return m
def own(o):
 for c in tuple(o.users_collection):c.objects.unlink(o)
 COL.objects.link(o)
def mesh(name,vertices,faces,m):
 data=bpy.data.meshes.new(name)
 data.from_pydata(vertices,[],faces);data.update()
 obj=bpy.data.objects.new(name,data);COL.objects.link(obj)
 obj.data.materials.append(m)
 return obj
def block(name,pos,size,m,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=pos)
 o=bpy.context.object;o.name=name;o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('Rounded machined edge','BEVEL')
  mod.width=bevel;mod.segments=2
  bpy.ops.object.modifier_apply(modifier=mod.name)
 o.data.materials.append(m);own(o)
 return o
def rod(name,start,end,radius,m):
 # Build actual geometry in FINAL world-aligned model coordinates. Blender
 # cylinder operators + rotated-object multi-joins produced false 3.7m-tall
 # twig bounds in the first prototype. Direct triangles cannot double-rotate.
 a=Vector(start);b=Vector(end)
 axis=(b-a).normalized()
 cross=axis.cross(Vector((0,0,1)))
 if cross.length<.1:cross=axis.cross(Vector((0,1,0)))
 u=cross.normalized();v=axis.cross(u).normalized()
 verts=[];faces=[];steps=8
 for endpoint in (a,b):
  for k in range(steps):
   t=k*math.tau/steps
   point=endpoint+radius*(math.cos(t)*u+math.sin(t)*v)
   verts.append(tuple(point))
 for k in range(steps):
  n=(k+1)%steps
  faces +=[(k,n,steps+n),(k,steps+n,steps+k)]
 faces.append(tuple(range(steps-1,-1,-1)))
 faces.append(tuple(range(steps,2*steps)))
 mesh(name,verts,faces,m)
def patch(name,length,width,m,nx=36,ny=7,base=.015,depth=.07,ragged=.12):
 # A continuous 3D mesh with uneven organically curved edges and fine ground
 # micro-relief (not a rectangular flat plane or transparent billboard).
 v=[];f=[]
 for i in range(nx+1):
  x=length*(i/nx-.5)
  l=1.-ragged*(.45+.55*math.sin(i*.61+1.)**2)
  for j in range(ny+1):
   ty=j/ny-.5
   y=ty*width*l + .07*math.sin(i*.47)*math.sin(j*math.pi/ny)
   z=base+depth + .020*math.sin(i*.84+j*.61)+.009*math.sin(i*1.9-j*1.7)
   v.append((x,y,z))
 for i in range(nx):
  for j in range(ny):
   k=i*(ny+1)+j
   f.extend(((k,k+ny+1,k+1),(k+1,k+ny+1,k+ny+2)))
 # Bottom cap; edges actually thick, not disembodied floating grass
 offset=len(v)
 for i in range(nx+1):
  for j in range(ny+1):
   x,y,_=v[i*(ny+1)+j]
   v.append((x,y,base))
 for i in range(nx):
  for j in range(ny):
   k=offset+i*(ny+1)+j
   f.extend(((k,k+1,k+ny+1),(k+1,k+ny+2,k+ny+1)))
 for i in range(nx):
  for j in (0,ny):
   a=i*(ny+1)+j;b=(i+1)*(ny+1)+j
   f.extend(((a,b,offset+b),(a,offset+b,offset+a)))
 for i in (0,nx):
  for j in range(ny):
   a=i*(ny+1)+j;b=a+1
   f.extend(((a,b,offset+b),(a,offset+b,offset+a)))
 return mesh(name,v,f,m)
def ellipse(name,cx,cy,rx,ry,z,m,n=21,angle=0):
 v=[(cx,cy,z)]
 for i in range(n):
  a=math.tau*i/n
  rough=1.+.12*math.sin(a*3+cx*5)+.06*math.sin(a*8+cy*3)
  x=rx*math.cos(a)*rough;y=ry*math.sin(a)*rough
  v.append((cx+x*math.cos(angle)-y*math.sin(angle),
            cy+x*math.sin(angle)+y*math.cos(angle),z+.008*math.sin(a*4)))
 f=[(0,1+i,1+(i+1)%n) for i in range(n)]
 return mesh(name,v,f,m)
def grass(name,n,length,width,materials,mask=None,h0=.09,h1=.29,at_z=.11):
 # Actual crossed tapered leaf blades; per-colour merged into 3 draw calls.
 buckets=[([],[]) for _ in materials]
 made=0
 for i in range(n*3):
  if made>=n:break
  x=R.uniform(-length*.48,length*.48)
  y=R.uniform(-width*.43,width*.43)
  if mask and not mask(x,y):continue
  made+=1
  m=R.randrange(len(materials));v,f=buckets[m]
  h=R.uniform(h0,h1)
  theta=R.uniform(0,math.tau)
  for k in range(3):
   a=theta+k*math.pi/3;dx=.023*math.cos(a);dy=.023*math.sin(a)
   s=len(v)
   v +=[(x-dx,y-dy,at_z),(x+dx,y+dy,at_z),
        (x+dx*.45,y+dy*.45,at_z+h),
        (x-dx*.45,y-dy*.45,at_z+h)]
   f +=[(s,s+1,s+2),(s,s+2,s+3)]
 for k,(v,f) in enumerate(buckets):
  if f:mesh(name+str(k),v,f,materials[k])
def foliage(name,cx,cy,cz,spread,height,colors,leaf_count=190):
 # Non-spherical, non-billboard plant: staggered, thin 3D leaves extending
 # in many directions; geometry preserved by GLB and casts geometric shadows.
 buckets=[([],[]) for _ in colors]
 for i in range(leaf_count):
  a=R.uniform(0,math.tau)
  t=math.sqrt(R.random())
  x=cx+spread*t*math.cos(a)
  y=cy+spread*.74*t*math.sin(a)
  z=cz+height*R.uniform(.15,1.) - .08*t
  size=R.uniform(.045,.095) if spread<=.6 else R.uniform(.065,.12)
  yaw=R.uniform(0,math.tau);pitch=R.uniform(-.5,.5)
  dx=math.cos(yaw)*size;dy=math.sin(yaw)*size
  m=i%len(colors);v,f=buckets[m];k=len(v)
  v.extend([(x-dx,y-dy,z),(x+dx,y+dy,z+size*.12),
            (x-.2*dx,y-.2*dy,z+size*(1.+pitch)),
            (x+.2*dx,y+.2*dy,z-size*.5)])
  f.extend(((k,k+1,k+2),(k+2,k+1,k),(k,k+3,k+1),(k+1,k+3,k)))
 for m,(v,f) in enumerate(buckets):
  if f:mesh(name+' leaves'+str(m),v,f,colors[m])
def shrubs(positions,wood,leaves):
 for i,(x,y,r) in enumerate(positions):
  h=.30+r*.7
  rod('Branch base '+str(i),(x,y,.115),(x+.05,y,h),.028,wood)
  for j in range(4):
   a=math.tau*j/4+i*.83
   rod('Twig '+str(i)+'_'+str(j),(x,y,h*.75),
       (x+math.cos(a)*r*.72,y+math.sin(a)*r*.55,h+R.uniform(.03,.16)),.012,wood)
  foliage('Natural bush '+str(i),x,y,h*.53,r,h*.56,leaves,125 if r<.5 else 205)
def save(stem,title,camera,target,ortho,expected_min):
 # Join by material (2..14 GPU draws/instance); model-space coords unchanged.
 groups={}
 for o in COL.objects:
  assert o.type=='MESH' and o.active_material,o.name
  groups.setdefault(o.active_material.name,[]).append(o)
 for name,items in groups.items():
  bpy.ops.object.select_all(action='DESELECT')
  for o in items:o.select_set(True)
  bpy.context.view_layer.objects.active=items[0]
  bpy.ops.object.join();bpy.context.object.name=title+' '+name
 parts=list(COL.objects)
 assert 3<=len(parts)<=18,(title,len(parts))
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=parts[0]
 out=ROOT/(stem+'.glb')
 bpy.ops.export_scene.gltf(filepath=str(out),export_format='GLB',
  use_selection=True,export_apply=True,export_materials='EXPORT',
  export_normals=True,export_texcoords=True,export_cameras=False,export_lights=False)
 bpy.ops.object.select_all(action='DESELECT')
 bpy.ops.object.camera_add(location=camera)
 cam=bpy.context.object;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
 cam.data.type='ORTHO';cam.data.ortho_scale=ortho
 bpy.context.scene.camera=cam
 bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,-.07))
 ground=bpy.context.object;ground.scale=(22,14,1)
 ground.data.materials.append(mat('Preview only floor',(.33,.34,.32)))
 for pos,power,size in [((8,-11,18),2600,11),((-8,7,12),1200,10)]:
  bpy.ops.object.light_add(type='AREA',location=pos)
  light=bpy.context.object;light.data.energy=power
  light.data.shape='DISK';light.data.size=size
  light.rotation_euler=(Vector(target)-light.location).to_track_quat('-Z','Y').to_euler()
 sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=12
 sc.render.resolution_x=1000;sc.render.resolution_y=650
 sc.render.resolution_percentage=100
 sc.render.image_settings.file_format='PNG'
 sc.render.filepath=str(ROOT/(stem+'-blender.png'))
 bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/(stem+'.blend')))
 bpy.ops.render.render(write_still=True)
 mi=[min((o.matrix_world@Vector(v))[a] for o in parts for v in o.bound_box) for a in range(3)]
 ma=[max((o.matrix_world@Vector(v))[a] for o in parts for v in o.bound_box) for a in range(3)]
 dimensions=[round(ma[a]-mi[a],3) for a in range(3)]
 assert dimensions[0]>=expected_min[0] and dimensions[1]>=expected_min[1]
 assert dimensions[2] < (.5 if 'sciezka' in stem else 1.7), (stem, dimensions)
 assert out.stat().st_size>10000
 meta={'title':title,'dimensions_m':dimensions,'mesh_parts':len(parts),
       'glb_bytes':out.stat().st_size,'blender':bpy.app.version_string,
       'metres_Z_up':True,'pivot':'GLB loader centres XY and base Z'}
 (ROOT/(stem+'-metadata.json')).write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8')
 print('R6B2B_ASSET_PASS',stem,json.dumps(meta,ensure_ascii=False))

# 1. Species-rich street-edge planting strip, 20m x ~3m, shaded loam and
# finely modelled grass, no large coplanar uniform rectangle.
reset('R6b2b pas zieleni 20m')
sod=mat('Darń oliwkowa',(.235,.305,.188))
soil=mat('Ziemia wilgotna',(.195,.171,.131))
leaves=[mat('Drobne liscie '+str(i),c) for i,c in enumerate([
 (.205,.295,.155),(.26,.35,.18),(.315,.405,.225),(.37,.41,.23)])]
blades=[mat('Zdzbla '+str(i),c) for i,c in enumerate([
 (.26,.37,.18),(.33,.41,.21),(.20,.30,.14)])]
wood=mat('Galazki krzewu',(.27,.20,.15))
patch('Nierowny nasyp i darń',20.0,3.15,sod,nx=48,ny=7,depth=.075)
for i,(x,y,rx,ry) in enumerate([(-7,-.3,1.45,.55),(-2.9,.4,1.0,.7),(2.5,-.35,1.6,.6),(7.1,.32,1.5,.59)]):
 ellipse('Odslonieta gleba i mulcz '+str(i),x,y,rx,ry,.116,soil,n=24,angle=.17*i)
shrubs([(-8.1,.18,.52),(-5.3,-.38,.37),(-2.1,.5,.45),(1.6,-.46,.56),
        (4.65,.37,.40),(8.3,-.22,.5)],wood,leaves)
grass('Prawdziwa trawa miejska',1100,20,3.15,blades,h0=.075,h1=.21)
save('r6b2b-pas-zieleni-20m','Pas zieleni 20 m', (16,-16,10),(0,0,.15),23,(19.9,2.7))

# 2. Courtyard planted bed, 14x7m: irregular planting substrate, living twigs,
# varied shrubs and meadow-grass height. Placed behind buildings, no road overlap.
reset('R6b2b rabata podworkowa')
sod=mat('Darń ogrodowa',(.25,.32,.185))
soil=mat('Naturalna ciemna ziemia',(.205,.18,.136))
gravel=mat('Jasny mulcz mineralny',(.46,.45,.39))
branch=mat('Kora drobnych krzewow',(.24,.185,.14))
colors=[mat('Ulistnienie ogrodowe '+str(i),c) for i,c in enumerate([
  (.19,.285,.145),(.275,.36,.185),(.32,.40,.22),(.40,.44,.24)])]
blades=[mat('Wysokie trawy '+str(i),c) for i,c in enumerate([
 (.28,.39,.21),(.22,.34,.17),(.38,.43,.23)])]
patch('Skrzywiona powierzchnia ogrodu',14.0,7.1,sod,nx=44,ny=10,depth=.084,ragged=.19)
for i,(x,y,rx,ry,m) in enumerate([
 (-4.5,.5,2.0,1.4,soil),(-.2,-.8,2.4,1.2,gravel),
 (3.65,.7,2.0,1.4,soil),(5.8,-1.5,.75,.78,soil)]):
 ellipse('Nieregularna wyspa upraw '+str(i),x,y,rx,ry,.132,m,n=27,angle=.2*i)
shrubs([(-5.15,.84,.65),(-3.6,1.24,.50),(-2.0,-1.8,.38),
        (1.2,.7,.57),(3.6,.65,.75),(5.2,-.4,.58),(6.0,1.3,.40)],branch,colors)
grass('Kepki naturalnego poszycia',1500,14,7.1,blades,h0=.11,h1=.33)
save('r6b2b-rabata-ogrodowa','Rabata ogrodowa 14 m',(12,-14,11),(0,0,.18),19,(13.9,6.0))

# 3. Narrow gravel pedestrian access path (12m x 1.6m) with genuine stone
# geometry and curved borders; source textures never used as fake pebble photos.
reset('R6b2b sciezka mineralna')
sand=mat('Mial kamienny',(.45,.425,.35))
edge=mat('Obrzeze piaskowe',(.55,.54,.48))
rocks=[mat('Zwirek '+str(i),c) for i,c in enumerate([
 (.53,.51,.45),(.38,.375,.33),(.60,.58,.53)])]
meadow=mat('Trawa w fugach',(.27,.365,.17))
patch('Utwardzona przepuszczalna sciezka',12.0,1.58,sand,nx=44,ny=5,base=.02,depth=.05,ragged=.06)
for sign in (-1,1):
 # Thin edging flush with mineral walking surface, not 20cm curbs.
 block('Zbite obrzeze ziemne '+str(sign),(0,sign*.765,.09),(12.0,.088,.11),edge,.017)
buckets=[([],[]) for _ in rocks]
for i in range(360):
 x=R.uniform(-5.8,5.8);y=R.uniform(-.69,.69)
 rad=R.uniform(.018,.053);zz=.093+R.uniform(-.012,.012)
 v,f=buckets[i%len(rocks)]
 st=len(v);v.extend([(x-rad,y,zz),(x+rad,y,zz),(x,y-rad,zz),(x,y+rad,zz),(x,y,zz+rad*.32)])
 f.extend(((st,st+2,st+4),(st+2,st+1,st+4),(st+1,st+3,st+4),(st+3,st,st+4)))
for i,(v,f) in enumerate(buckets):mesh('Naturalne kamienie '+str(i),v,f,rocks[i])
grass('Drobne zadarnienia brzegow',150,12,1.60,[meadow],mask=lambda x,y:abs(y)>.59,h0=.05,h1=.135,at_z=.115)
save('r6b2b-sciezka-zwirowa-12m','Sciezka zwirowa 12 m',(10,-10,7),(0,0,.1),15,(11.9,1.5))
print('PRAWKO_R6B2B_THREE_ORIGINAL_ASSETS_PASS')
