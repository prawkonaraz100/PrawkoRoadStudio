# SPDX-License-Identifier: Apache-2.0
"""R6b2d: two authored, editable 3D boundary assets with actual UV0 PBR maps.
All texture pixels are generated deterministically from code. Blender 5.3/Z up/metres.
"""
import bpy,math,random,json,struct,zlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
R=random.Random(62026)
COL=None

def png_write(path,size,pixels):
 def chunk(tag,payload):return struct.pack('>I',len(payload))+tag+payload+struct.pack('>I',zlib.crc32(tag+payload)&0xffffffff)
 rows=b''.join(b'\x00'+bytes(pixels[y*size*4:(y+1)*size*4]) for y in range(size))
 data=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',size,size,8,6,0,0,0))
 data+=chunk(b'IDAT',zlib.compress(rows,8))+chunk(b'IEND',b'')
 path.write_bytes(data)
def clamp(v):return max(0,min(255,int(v)))
def hash2(x,y,salt=0):
 n=(x*374761393+y*668265263+salt*1442695041)&0xffffffff
 n=(n^(n>>13))*1274126177&0xffffffff
 return ((n^(n>>16))&0xffff)/65535.
def smooth(x,y,cell,salt):
 gx=x/cell;gy=y/cell;ix=math.floor(gx);iy=math.floor(gy)
 a=gx-ix;b=gy-iy;a=a*a*(3-2*a);b=b*b*(3-2*b)
 c00=hash2(ix,iy,salt);c10=hash2(ix+1,iy,salt)
 c01=hash2(ix,iy+1,salt);c11=hash2(ix+1,iy+1,salt)
 return (c00*(1-a)+c10*a)*(1-b)+(c01*(1-a)+c11*a)*b
def make_pbr(prefix,rgb,soil=False):
 size=256
 al=[];nm=[];orm=[]
 h=[[0]*size for _ in range(size)]
 for y in range(size):
  for x in range(size):
   g=smooth(x,y,35,11)*.48+smooth(x,y,11,19)*.32+hash2(x,y,26)*.20
   crack=1 if (not soil and (x+int(2*math.sin(y*.1)))%113 in (0,1)) else 0
   h[y][x]=g
   fac=.75+.49*g-(.12 if crack else 0)
   al.extend([clamp(c*fac) for c in rgb]+[255])
   rough=(.72+.23*g if soil else .69+.22*g)
   orm.extend([255,clamp(rough*255),0,255])
 for y in range(size):
  for x in range(size):
   hx=h[y][(x+1)%size]-h[y][(x-1)%size]
   hy=h[(y+1)%size][x]-h[(y-1)%size][x]
   nm.extend([clamp(127.5-hx*20),clamp(127.5-hy*20),253,255])
 files=[]
 for suffix,pixels in (('albedo',al),('normal',nm),('orm',orm)):
  f=ROOT/(prefix+'-'+suffix+'.png')
  png_write(f,size,pixels);files.append(f)
 return files
def texnodes(name,prefix,rough=.85):
 m=bpy.data.materials.new('R6b2d '+name)
 m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF')
 p.inputs['Metallic'].default_value=0.
 p.inputs['Roughness'].default_value=rough
 p.inputs['Base Color'].default_value=(1,1,1,1)
 for slot in ('Base Color','Roughness','Normal'):
  for link in list(p.inputs[slot].links):m.node_tree.links.remove(link)
 files=[ROOT/(prefix+'-'+s+'.png') for s in ('albedo','normal','orm')]
 def load(path,nc):
  image=bpy.data.images.load(str(path),check_existing=True)
  if nc:image.colorspace_settings.name='Non-Color'
  n=m.node_tree.nodes.new('ShaderNodeTexImage');n.image=image;n.interpolation='Linear'
  n.label=path.name
  return n
 color=load(files[0],False)
 normal=load(files[1],True)
 orm=load(files[2],True)
 nmap=m.node_tree.nodes.new('ShaderNodeNormalMap');nmap.inputs['Strength'].default_value=.65
 sep=m.node_tree.nodes.new('ShaderNodeSeparateColor');sep.mode='RGB'
 m.node_tree.links.new(color.outputs['Color'],p.inputs['Base Color'])
 m.node_tree.links.new(normal.outputs['Color'],nmap.inputs['Color'])
 m.node_tree.links.new(nmap.outputs['Normal'],p.inputs['Normal'])
 m.node_tree.links.new(orm.outputs['Color'],sep.inputs['Color'])
 m.node_tree.links.new(sep.outputs['Green'],p.inputs['Roughness'])
 return m
def solid(name,rgb,rough=.95):
 m=bpy.data.materials.new('R6b2d '+name)
 m.use_nodes=True;m.diffuse_color=(*rgb,1)
 p=m.node_tree.nodes.get('Principled BSDF')
 p.inputs['Base Color'].default_value=(*rgb,1)
 p.inputs['Roughness'].default_value=rough
 return m
def own(obj):
 for coll in tuple(obj.users_collection):coll.objects.unlink(obj)
 COL.objects.link(obj)
def cube(name,p,d,mat,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p)
 o=bpy.context.object;o.name=name;o.dimensions=d
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('Small real bevel','BEVEL');mod.width=bevel;mod.segments=2
  bpy.ops.object.modifier_apply(modifier=mod.name)
 assert len(o.data.uv_layers)>0,'Cubes must have genuine UV0'
 o.data.materials.append(mat);own(o)
 return o
def make_mesh(name,v,f,uv,mat):
 d=bpy.data.meshes.new(name+' geometry')
 d.from_pydata(v,[],f);d.update()
 o=bpy.data.objects.new(name,d);COL.objects.link(o)
 o.data.materials.append(mat)
 layer=d.uv_layers.new(name='UVMap')
 for poly in d.polygons:
  for l in poly.loop_indices:
   vi=d.loops[l].vertex_index
   layer.data[l].uv=uv[vi]
 return o
def reset(label):
 global COL
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 COL=bpy.data.collections.new(label+' GLB ONLY');bpy.context.scene.collection.children.link(COL)
def grass(name,n,length,width,base_z,colours,edge=False):
 groups=[([],[],[]) for _ in colours]
 for i in range(n):
  x=R.uniform(-length*.48,length*.48)
  y=(R.choice([-1,1])*R.uniform(width*.27,width*.48) if edge else
     R.uniform(-width*.43,width*.43))
  h=R.uniform(.085,.24);w=R.uniform(.014,.026)
  a=R.random()*math.tau
  for k in range(2):
   theta=a+k*math.pi/2;dx=w*math.cos(theta);dy=w*math.sin(theta)
   v,f,uv=groups[i%len(colours)];st=len(v)
   v.extend(((x-dx,y-dy,base_z),(x+dx,y+dy,base_z),
             (x+.4*dx,y+.4*dy,base_z+h)))
   uv.extend(((0,0),(1,0),(.5,1)))
   f.append((st,st+1,st+2))
 for j,(v,f,uv) in enumerate(groups):
  if f:make_mesh(name+str(j),v,f,uv,colours[j])
def join_by_mat():
 groups={}
 for o in list(COL.objects):
  assert o.type=='MESH' and o.active_material,o.name
  groups.setdefault(o.active_material.name,[]).append(o)
 for key,items in groups.items():
  bpy.ops.object.select_all(action='DESELECT')
  for o in items:o.select_set(True)
  bpy.context.view_layer.objects.active=items[0]
  if len(items)>1:bpy.ops.object.join()
  bpy.context.view_layer.objects.active.name=key
 parts=list(COL.objects)
 assert 4<=len(parts)<=12,len(parts)
 return parts
def finish(stem,title,camera,target,ortho,min_size,max_height):
 parts=join_by_mat()
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=parts[0]
 glb=ROOT/(stem+'.glb')
 bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',use_selection=True,
    export_apply=True,export_normals=True,export_texcoords=True,export_materials='EXPORT',
    export_cameras=False,export_lights=False)
 bpy.ops.object.select_all(action='DESELECT')
 bpy.ops.object.camera_add(location=camera)
 cam=bpy.context.object
 cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
 cam.data.type='ORTHO';cam.data.ortho_scale=ortho
 bpy.context.scene.camera=cam
 bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,-.055))
 bg=bpy.context.object;bg.scale=(15,9,1)
 bg.data.materials.append(solid('PREVIEW ONLY asphalt',(.31,.32,.29)))
 for loc,power,size in [((5,-9,15),2000,7),((-6,7,9),1100,9)]:
  bpy.ops.object.light_add(type='AREA',location=loc)
  l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size
  l.rotation_euler=(Vector(target)-l.location).to_track_quat('-Z','Y').to_euler()
 sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=15
 sc.render.resolution_x=1150;sc.render.resolution_y=700
 sc.render.resolution_percentage=100;sc.render.image_settings.file_format='PNG'
 sc.render.filepath=str(ROOT/(stem+'-blender.png'))
 bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/(stem+'.blend')))
 bpy.ops.render.render(write_still=True)
 lo=[min((o.matrix_world@Vector(v))[a] for o in parts for v in o.bound_box) for a in range(3)]
 hi=[max((o.matrix_world@Vector(v))[a] for o in parts for v in o.bound_box) for a in range(3)]
 dims=[round(hi[a]-lo[a],3) for a in range(3)]
 assert dims[0]>min_size[0] and dims[1]>min_size[1] and dims[2]<max_height,dims
 assert glb.stat().st_size>60000
 meta={'title':title,'dimensions_m':dims,'parts':len(parts),
       'units':'metres','up':'+Z','uv0':True,'pbr':'embedded baseColor / tangent-space normal / ORM roughness',
       'blender':bpy.app.version_string,'glb_bytes':glb.stat().st_size}
 (ROOT/(stem+'-metadata.json')).write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf8')
 print('R6B2D_PBR_GLTF_ASSET_PASS',stem,json.dumps(meta,ensure_ascii=False))

make_pbr('r6b2d-beton',(145,143,133))
make_pbr('r6b2d-gleba',(91,79,62),True)
stone=texnodes('Plyta kamienna PBR UV','r6b2d-beton',.86)
soil=texnodes('Ziemia PBR UV','r6b2d-gleba',.96)
joint=solid('Naturalny spoiwo',(.18,.18,.17))
curb=solid('Obrzeze betonowe',(.46,.46,.44))
iron=solid('Odwodnienie',(.18,.20,.20),.57)
herb=[solid('Zdzbla '+str(i),v) for i,v in enumerate(
 [(.215,.335,.165),(.29,.39,.19),(.33,.40,.23)])]
# 12m paved pedestrian route with actual joints and tactile kerb edging.
reset('R6b2d przejscie podworkowe PBR')
cube('Warstwa filtracyjna',(0,0,.049),(12,1.74,.095),joint,.008)
for i in range(32):
 x=(i-15.5)*.375
 for j in range(5):
  y=(j-2)*.304
  variation=R.uniform(-.014,.014)
  cube('Faktura fizycznej plyty '+str(i)+'_'+str(j),
       (x+variation*.33,y,.111+R.uniform(-.002,.002)),
       (.363,.290,.054),stone,.007)
for sign in (-1,1):
 cube('Krawedz chodnika '+str(sign),(0,sign*.822,.108),
      (12,.11,.146),curb,.012)
for x in (-4.25,3.85):
 cube('Wpust odwadniajacy korpus '+str(x),(x,.713,.158),
      (.58,.14,.022),iron,.004)
 for i in range(5):
  cube('Kratka odwadniajaca '+str(i)+' '+str(x),
       (x-.22+i*.11,.713,.173),(.025,.15,.012),curb,.002)
grass('Trawa przy obrzezu',180,12,1.75,.145,herb,edge=True)
finish('r6b2d-przejscie-plyty-12m','Przejscie na podworko 12m: PBR',(13,-13,10),
       (0,0,.12),16,(11.9,1.7),.55)
# 20m transition ground at the OUTSIDE edge of a sidewalk. Textured mineral
# base gradually breaks into real soil with uneven outline and material.
reset('R6b2d styk chodnika i ziemi')
cube('Podloze spoiste',(0,0,.047),(20,.74,.094),soil,.012)
# 40 individually beveled stone edge pieces; real joints, varied roughness UV.
for i in range(40):
 x=(i-19.5)*.5
 cube('Fazowany modul krawedziowy '+str(i),(x,-.253,.106),
      (.488,.18,.118),stone,.009)
# Irregular exposed soil and moisture strips at organic border.
verts=[];faces=[];uv=[]
n=100
for i in range(n+1):
 x=20*(i/n-.5)
 upper=.36+.045*math.sin(i*.31)+.025*math.sin(i*.79+2)
 for y,z in ((-.147,.115),(upper,.099+.011*math.sin(i*.59))):
  verts.append((x,y,z));uv.append(((x+10)*.42,(y+.35)*3.2))
for i in range(n):
 k=2*i;faces.extend(((k,k+1,k+2),(k+1,k+3,k+2)))
make_mesh('Fizycznie nieregularny grunt za obrzezem',verts,faces,uv,soil)
grass('Losowe kepki na styku gleby',950,20,.70,.125,herb,edge=False)
finish('r6b2d-styk-chodnik-ziemia-20m','Styk chodnika i zieleni 20m: PBR',
       (17,-15,10),(0,0,.11),23,(19.9,.72),.53)
print('PRAWKO_R6B2D_TWO_PBR_ASSETS_PASS')
