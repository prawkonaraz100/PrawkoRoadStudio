# SPDX-License-Identifier: Apache-2.0
"""R6b2c: original mature street linden, river birch and foundation planting.
Real world-space branches/leaves; Blender 5.3 -> GLB; metre units, Z up.
No billboard spheres, external images, or stock models. Reproducible seed.
"""
import bpy, math, random, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
rnd=random.Random(20261116)
COL=None
def reset(label):
 global COL
 bpy.ops.object.select_all(action='SELECT')
 bpy.ops.object.delete(use_global=False)
 COL=bpy.data.collections.new(label+' EXPORT ONLY')
 bpy.context.scene.collection.children.link(COL)
def material(label,rgb,rough=.87,metal=.0):
 m=bpy.data.materials.new('R6C '+label)
 m.use_nodes=True;m.diffuse_color=(*rgb,1)
 bs=m.node_tree.nodes.get('Principled BSDF')
 bs.inputs['Base Color'].default_value=(*rgb,1)
 bs.inputs['Roughness'].default_value=rough
 bs.inputs['Metallic'].default_value=metal
 return m
def mesh(name,v,f,mat):
 assert v and f,name
 d=bpy.data.meshes.new(name+' geo')
 d.from_pydata(v,[],f);d.update()
 o=bpy.data.objects.new(name,d);COL.objects.link(o);o.data.materials.append(mat)
 return o
def block(name,location,dimensions,mat,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=location)
 o=bpy.context.object;o.name=name;o.dimensions=dimensions
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('Actual bevel','BEVEL')
  mod.width=bevel;mod.segments=2
  bpy.ops.object.modifier_apply(modifier=mod.name)
 o.data.materials.append(mat)
 for c in tuple(o.users_collection):c.objects.unlink(o)
 COL.objects.link(o)
 return o
def tube(name,a,b,ra,rb,mat,sides=9):
 a=Vector(a);b=Vector(b)
 axis=(b-a).normalized()
 c=axis.cross(Vector((0,0,1)))
 if c.length<.1:c=axis.cross(Vector((0,1,0)))
 u=c.normalized();v=axis.cross(u).normalized()
 verts=[]
 for q,r in ((a,ra),(b,rb)):
  for i in range(sides):
   angle=math.tau*i/sides
   p=q+r*(u*math.cos(angle)+v*math.sin(angle))
   verts.append(tuple(p))
 faces=[]
 for i in range(sides):
  nxt=(i+1)%sides
  faces.extend(((i,nxt,nxt+sides),(i,nxt+sides,i+sides)))
 faces.extend((tuple(range(sides-1,-1,-1)),tuple(range(sides,2*sides))))
 return mesh(name,verts,faces,mat)
def soil_bed(name,width,depth,earth,stone,roundness=.03):
 block(name+' loam',(0,0,.05),(width,depth,.10),earth,.028)
 for sy in (-1,1):
  block(name+' edge y'+str(sy),(0,sy*(depth/2-.06),.105),
        (width,.16,.175),stone,.02)
 for sx in (-1,1):
  block(name+' edge x'+str(sx),(sx*(width/2-.06),0,.105),
        (.16,depth-.10,.175),stone,.02)
def real_grass(stem,n,xmax,ymax,colors):
 # Long tapered folded grass blades, double-sided, no alpha cutouts.
 buckets=[([],[]) for _ in colors]
 for i in range(n):
  x=rnd.uniform(-xmax,xmax);y=rnd.uniform(-ymax,ymax)
  h=rnd.uniform(.07,.25);angle=rnd.uniform(0,math.tau)
  v,f=buckets[i%len(colors)]
  for j in range(2):
   a=angle+j*math.pi*.5;dx=.024*math.cos(a);dy=.024*math.sin(a)
   z=.13;idx=len(v)
   v.extend(((x-dx,y-dy,z),(x+dx,y+dy,z),
             (x+.4*dx,y+.4*dy,z+h),(x-.4*dx,y-.4*dy,z+h)))
   f.extend(((idx,idx+1,idx+2),(idx,idx+2,idx+3)))
 for k,(v,f) in enumerate(buckets):
  if v:mesh(stem+str(k),v,f,colors[k])
def leaf_geometry(prefix,centers,leaves,leaf_size,colors,birch=False):
 buckets=[([],[]) for _ in colors]
 for i in range(leaves):
  center=rnd.choice(centers)
  radius=center[3]
  # Multiple clusters with irregular edges, never an ellipsoid surface shell.
  dx=rnd.gauss(0,radius*.39)
  dy=rnd.gauss(0,radius*.41)
  dz=rnd.gauss(0,radius*.32)
  if abs(dx)>radius or abs(dy)>radius or abs(dz)>radius*.75:continue
  x=center[0]+dx;y=center[1]+dy;z=center[2]+dz
  s=leaf_size*rnd.uniform(.67,1.31)
  # Crescent/almond leaf with a folded midrib and a little natural twist.
  a=rnd.random()*math.tau
  ux=math.cos(a);uy=math.sin(a);vx=-uy;vy=ux
  broad=s*(.48 if birch else .60)
  slope=rnd.uniform(-.45,.45)
  v,f=buckets[i%len(colors)]
  k=len(v)
  v.extend((
   (x-ux*s,y-uy*s,z-.12*s),
   (x+ux*s*.89,y+uy*s*.89,z+.12*s),
   (x+vx*broad,y+vy*broad,z+s*(.20+slope)),
   (x-vx*broad,y-vy*broad,z+s*(.16-slope))
  ))
  # front and back; texture-free silhouette works in all viewing directions.
  f.extend(((k,k+2,k+1),(k,k+1,k+3)))
 for i,(v,f) in enumerate(buckets):
  if v:mesh(prefix+' foliage '+str(i),v,f,colors[i])
def make_tree(kind):
 assert kind in ('lipa','brzoza')
 birch=kind=='brzoza'
 reset('R6b2c '+kind)
 earth=material('Ciemna ziemia '+kind,(.20,.17,.13))
 stone=material('Ciosany kamien '+kind,(.52,.52,.48))
 bark=material('Kora '+kind,((.71,.69,.64) if birch else (.28,.225,.17)),.94)
 dark=material('Spękania kory '+kind,((.22,.225,.21) if birch else (.195,.145,.115)))
 leaf_colours=[material('Liscie '+kind+' '+str(i),c) for i,c in enumerate(
   ([(.245,.355,.18),(.29,.40,.21),(.36,.445,.25),(.205,.325,.165)]
    if not birch else
    [(.315,.425,.205),(.37,.47,.24),(.25,.365,.17),(.41,.49,.265)]))]
 herb=[material('Runo '+kind+' '+str(i),c) for i,c in enumerate([
   (.26,.35,.175),(.34,.39,.20)])]
 soil_bed('Misa korzeniowa',1.93,1.93,earth,stone)
 trunk_end=5.8 if birch else 4.55
 # Curved, genuinely tapered trunk with flared root-neck and 3D branching.
 trunk_points=[(0,0,.11),(.07,.0,.52),(-.05,.055,2.15),
               (.10,-.025,3.45),(.06,.06,trunk_end)]
 trunk_radii=([.18,.145,.112,.085,.026] if birch else
              [.25,.21,.16,.11,.035])
 for i in range(len(trunk_points)-1):
  tube('Pień odcinek '+str(i),trunk_points[i],trunk_points[i+1],
       trunk_radii[i],trunk_radii[i+1],bark,sides=12)
 if birch:
  # White/dark mottled birch lenticels on opposite trunk sides.
  for i in range(46):
   z=rnd.uniform(.6,4.5)
   x=.10*math.cos(z*5.1)+rnd.uniform(-.045,.045)
   y=.10*math.sin(z*5.1)
   tube('Poprzeczny wzor kory '+str(i),(x,y-.035,z),
        (x+.055,y+.037,z+rnd.uniform(-.025,.025)),.008,.006,dark,sides=5)
 # Main and secondary network, canopy records for thousands of individual leaves.
 centers=[]
 main_count=(14 if birch else 12)
 for i in range(main_count):
  az=(math.tau*i/main_count)+rnd.uniform(-.14,.14)
  z0=(2.55+rnd.random()*2.25 if not birch else
      3.4+rnd.random()*2.5)
  a=(.08*math.sin(i),.06*math.cos(i),z0)
  radial=rnd.uniform(1.1,2.45 if not birch else 1.9)
  end_z=z0+rnd.uniform(1.1,2.65) if not birch else z0+rnd.uniform(1.35,3.2)
  b=(math.cos(az)*radial,math.sin(az)*radial*.92,end_z)
  tube('Konar glowny '+str(i),a,b,.075 if birch else .105,.022,bark)
  for j in range(3):
   subaz=az+rnd.uniform(-.62,.62)
   rr=radial+rnd.uniform(.15,.62)
   zz=end_z+rnd.uniform(.25,1.0)
   if birch:
    zz=min(10.1,zz)
   else:
    zz=min(8.8,zz)
   c=(math.cos(subaz)*rr,math.sin(subaz)*rr*.94,zz)
   mid=tuple(Vector(a).lerp(Vector(b),rnd.uniform(.55,.85)))
   tube('Gałązka '+str(i)+'-'+str(j),mid,c,.028,.007,bark,sides=7)
   if birch:
    # The drooping terminal branches make a birch silhouette
    pendant=(c[0]+math.cos(subaz)*.14,c[1]+math.sin(subaz)*.2,c[2]-.46)
    tube('Zwisajaca gałązka '+str(i)+'-'+str(j),c,pendant,.008,.004,bark,sides=6)
    centers.append((pendant[0],pendant[1],pendant[2],rnd.uniform(.35,.58)))
    centers.append((c[0],c[1],c[2]-.10,rnd.uniform(.25,.4)))
   else:
    centers.append((c[0],c[1],c[2],rnd.uniform(.44,.72)))
    centers.append((mid[0],mid[1],mid[2]+.55,rnd.uniform(.35,.52)))
 centers.extend(((0,.18,8.3,.55),(-.3,-.2,8.5,.5)) if not birch
                else ((0,0,9.15,.4),(.1,.1,9.4,.37)))
 leaf_geometry('Liście z geometria 3D',centers,
               6200 if birch else 7600,
               .153 if birch else .194,leaf_colours,birch=birch)
 real_grass('Rosliny u korzeni',120,.72,.72,herb)
 return 'r6b2c-brzoza-10m' if birch else 'r6b2c-lipa-9m'

def make_foundation():
 reset('R6b2c strefa przybudynkowa')
 darksoil=material('Cienisty podklad',(.23,.21,.18))
 edge=material('Obrzeze przyfundamentowe',(.45,.45,.43))
 chips=[material('Naturalny mulcz '+str(i),c) for i,c in enumerate(
   [(.34,.28,.22),(.43,.33,.245),(.26,.21,.18)])]
 wood=material('Zdrewiale pedy',(.32,.235,.18))
 greens=[material('Liscie okrywowe '+str(i),c) for i,c in enumerate(
   [(.19,.285,.16),(.27,.36,.205),(.34,.40,.23)])]
 # Real slightly wavy raised bark ground, close to a rear wall, no half metre curb.
 n=36;v=[];f=[]
 for i in range(n+1):
  x=12*(i/n-.5)
  for j in range(5):
   y=1.55*(j/4-.5)+.018*math.sin(i*.3)
   z=.083+.015*math.sin(i*.7+j*.9)
   v.append((x,y,z))
 for i in range(n):
  for j in range(4):
   q=i*5+j;f.extend(((q,q+5,q+1),(q+1,q+5,q+6)))
 mesh('Podklad mulczowy z nierownosciami',v,f,darksoil)
 for sign in (-1,1):
  block('Opaska krawedziowa '+str(sign),(0,sign*.78,.085),
        (12,.08,.13),edge,.016)
 # Chips are real small surfaces with varied height, not texture decals.
 chips_buckets=[([],[]) for _ in chips]
 for i in range(430):
  x=rnd.uniform(-5.8,5.8);y=rnd.uniform(-.68,.68)
  length=rnd.uniform(.028,.092)
  angle=rnd.random()*math.tau
  dx=length*math.cos(angle);dy=length*math.sin(angle)
  verts,faces=chips_buckets[i%3];k=len(verts)
  verts.extend([(x-dx,y-dy,.105),(x+dx,y+dy,.105),
                (x+.23*dy,y-.23*dx,.118)])
  faces.append((k,k+1,k+2))
 for i,(verts,faces) in enumerate(chips_buckets):
  mesh('Mulcz drobiny '+str(i),verts,faces,chips[i])
 leaf_centers=[]
 for i in range(28):
  x=-5.5+i*.42+rnd.uniform(-.11,.11)
  y=rnd.uniform(-.48,.48)
  z=.13+rnd.uniform(.06,.22)
  if i%3==0:
   tube('Zdrewialy ped '+str(i),(x,y,.1),(x+.08,y,z+.15),
        .021,.004,wood,sides=6)
  leaf_centers.append((x,y,z,rnd.uniform(.16,.32)))
 leaf_geometry('Niskie rosliny cieniste',leaf_centers,650,.08,greens)
 return 'r6b2c-pas-fundamentowy-12m'

def save(stem,label,cam,look,scale,kind):
 groups={}
 for o in COL.objects:
  assert o.type=='MESH' and o.active_material,o.name
  groups.setdefault(o.active_material.name,[]).append(o)
 for material_name,items in groups.items():
  bpy.ops.object.select_all(action='DESELECT')
  for o in items:o.select_set(True)
  bpy.context.view_layer.objects.active=items[0]
  bpy.ops.object.join();bpy.context.object.name=label+' '+material_name
 parts=list(COL.objects)
 assert 4<=len(parts)<=16,(stem,len(parts))
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=parts[0]
 out=ROOT/(stem+'.glb')
 bpy.ops.export_scene.gltf(filepath=str(out),export_format='GLB',
    use_selection=True,export_apply=True,export_materials='EXPORT',
    export_normals=True,export_texcoords=True,export_cameras=False,export_lights=False)
 bpy.ops.object.select_all(action='DESELECT')
 bpy.ops.object.camera_add(location=cam)
 camera=bpy.context.object
 camera.rotation_euler=(Vector(look)-camera.location).to_track_quat('-Z','Y').to_euler()
 camera.data.type='ORTHO';camera.data.ortho_scale=scale
 bpy.context.scene.camera=camera
 bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,-.06))
 ground=bpy.context.object;ground.scale=(18,14,1)
 ground.data.materials.append(material('PREVIEW ONLY asphalt',(.31,.32,.30)))
 for location,power,size in [((9,-12,19),2600,9),((-9,7,14),1800,11)]:
  bpy.ops.object.light_add(type='AREA',location=location)
  light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size
  light.rotation_euler=(Vector(look)-light.location).to_track_quat('-Z','Y').to_euler()
 sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=14
 sc.render.resolution_x=1100;sc.render.resolution_y=750
 sc.render.resolution_percentage=100
 sc.render.image_settings.file_format='PNG'
 sc.render.filepath=str(ROOT/(stem+'-blender.png'))
 bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/(stem+'.blend')))
 bpy.ops.render.render(write_still=True)
 minimum=[min((o.matrix_world@Vector(c))[i] for o in parts for c in o.bound_box) for i in range(3)]
 maximum=[max((o.matrix_world@Vector(c))[i] for o in parts for c in o.bound_box) for i in range(3)]
 dims=[round(maximum[i]-minimum[i],3) for i in range(3)]
 assert out.stat().st_size>20000
 assert minimum[2]>=-.002,minimum
 assert (7.<dims[2]<12. if kind=='tree' else .1<dims[2]<.65),dims
 meta={'title':label,'units':'metres','axes':'Z up',
       'size_m':dims,'parts':len(parts),'min_height_m':minimum[2],
       'glb_bytes':out.stat().st_size,'blender':bpy.app.version_string}
 (ROOT/(stem+'-metadata.json')).write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8')
 print('R6B2C_GLTF_AND_CYCLES_PASS',stem,json.dumps(meta,ensure_ascii=False))

s=make_tree('lipa');save(s,'Lipa uliczna okolo 9 m',(13,-13,12),(0,0,4.5),13,'tree')
s=make_tree('brzoza');save(s,'Brzoza miejska okolo 10 m',(13,-13,13),(0,0,5.2),14,'tree')
s=make_foundation();save(s,'Rabata przy fundamencie 12 m',(11,-13,9),(0,0,.17),17,'bed')
print('PRAWKO_R6B2C_ALL_THREE_GLBS_PASS')
