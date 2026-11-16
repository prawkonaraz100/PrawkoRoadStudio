# SPDX-License-Identifier: Apache-2.0
"""R6b2h authored reusable shallow puddles with physically separate water + rim.
Export GLB only from asset collection; +Z up, metres, origin at roadway contact.
Water surface material marker "PrawkoWater." is recognized by the renderer.
"""
import bpy, math, random, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parent
COL=None

def reset():
 global COL
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 COL=bpy.data.collections.new('R6b2h authored GLB ONLY')
 bpy.context.scene.collection.children.link(COL)

def material(name,rgba,rough,metal=0):
 m=bpy.data.materials.new(name);m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF')
 p.inputs['Base Color'].default_value=rgba
 p.inputs['Roughness'].default_value=rough
 p.inputs['Metallic'].default_value=metal
 p.inputs['Alpha'].default_value=rgba[3]
 p.inputs['IOR'].default_value=1.333 if name.startswith('PrawkoWater.') else 1.50
 if rgba[3]<.99:
  m.surface_render_method='DITHERED'
 return m

water=material('PrawkoWater.Surface | planar reflective glTF',(.22,.29,.33,.69),.085,.05)
mud=material('R6b2h wet stone border | mineral',(.20,.19,.17,1),.74)
edge=material('R6b2h soaked thin rim',(.12,.13,.12,1),.38)
def mesh(name,positions,faces,uvs,mat):
 d=bpy.data.meshes.new(name+' mesh')
 d.from_pydata(positions,[],faces);d.update()
 o=bpy.data.objects.new(name,d);COL.objects.link(o)
 d.materials.append(mat)
 layer=d.uv_layers.new(name='UVMap')
 for f in d.polygons:
  for l in f.loop_indices:
   idx=d.loops[l].vertex_index
   layer.data[l].uv=uvs[idx]
 return o
def outline(seed,rx,ry,n=40):
 rng=random.Random(seed)
 radii=[max(.79,min(1.17,1.0+.08*math.sin(3*i*math.tau/n+seed)
                                  +.042*math.sin(7*i*math.tau/n+.7*seed)
                                  +rng.uniform(-.031,.031))) for i in range(n)]
 coords=[]
 for i in range(n):
  a=2*math.pi*i/n
  coords.append((rx*math.cos(a)*radii[i],ry*math.sin(a)*radii[i]))
 return coords

def water_disk(boundary,seed):
 n=len(boundary)
 points=[(0,0,.011)];uv=[(.5,.5)]
 for x,y in boundary:
  # calm water with gentle sub-centimetre ripples for normal perturbation
  z=.010+.0016*math.sin(x*7.1+y*5.3+seed*.17)
  points.append((x*.99,y*.99,z));uv.append((.5+x/5,.5+y/4))
 faces=[(0,1+i,1+(i+1)%n) for i in range(n)]
 return mesh('PrawkoWater.actual-surface',points,faces,uv,water)

def annulus(boundary,low,high,mat,label,seed):
 n=len(boundary);v=[];uv=[];f=[]
 for idx,ratio in enumerate((low,high)):
  for i,(x,y) in enumerate(boundary):
   z=(-.012 if ratio >1.02 else -.004)
   z+=.001*math.sin(x*8+y*5+seed)
   v.append((x*ratio,y*ratio,z))
   uv.append((.5+x*ratio/5,.5+y*ratio/3))
 for i in range(n):
  j=(i+1)%n
  f.extend(((i,j,n+i),(n+i,j,n+j)))
 return mesh(label,v,f,uv,mat)

def grass_tufts(boundary,seed):
 rng=random.Random(seed+11)
 v=[];uv=[];f=[]
 for k in range(45):
  i=rng.randrange(len(boundary));x,y=boundary[i];spread=rng.uniform(1.065,1.16)
  x*=spread;y*=spread
  if rng.random()>.42:continue
  h=rng.uniform(.026,.06);sz=rng.uniform(.008,.015)
  j=len(v)
  v.extend([(x-sz,y,-.003),(x+sz,y,-.003),(x+rng.uniform(-.02,.02),y,h)])
  uv.extend([(0,0),(1,0),(.5,1)])
  f.append((j,j+1,j+2))
 if f:mesh('R6b2h micro stems',v,f,uv,edge)

def export(stem,title,seed,rx,ry):
 reset()
 boundary=outline(seed,rx,ry)
 water_disk(boundary,seed)
 annulus(boundary,.996,1.06,edge,'Wetted meniscus rim',seed)
 annulus(boundary,1.06,1.16,mud,'Sediment feathered to surrounding stone',seed)
 grass_tufts(boundary,seed)
 originals=list(COL.objects)
 assert 3<=len(originals)<=6
 bpy.ops.object.select_all(action='DESELECT')
 for o in originals:o.select_set(True)
 bpy.context.view_layer.objects.active=originals[0]
 glb=ROOT/(stem+'.glb')
 bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',
  use_selection=True,export_apply=True,export_normals=True,
  export_texcoords=True,export_materials='EXPORT',
  export_cameras=False,export_lights=False)
 # Preview only ground + reflected buildings provided by Cycles material.
 bpy.ops.object.select_all(action='DESELECT')
 bpy.ops.object.camera_add(location=(3,-4.6,5.7))
 cam=bpy.context.object
 cam.rotation_euler=(Vector((0,0,0))-cam.location).to_track_quat('-Z','Y').to_euler()
 cam.data.type='ORTHO';cam.data.ortho_scale=max(5.8,2.6*rx) if rx>1.0 else 3.7
 bpy.context.scene.camera=cam
 bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,-.022))
 floor=bpy.context.object;floor.scale=(5,5,1)
 fmat=material('PREVIEW ONLY sidewalk',(.41,.39,.34,1),.83)
 floor.data.materials.append(fmat)
 for pos,power,size in [((5,-5,9),1300,5),((-4,4,10),750,7)]:
  bpy.ops.object.light_add(type='AREA',location=pos)
  light=bpy.context.object;light.data.energy=power;light.data.size=size
  light.rotation_euler=(Vector((0,0,0))-light.location).to_track_quat('-Z','Y').to_euler()
 sc=bpy.context.scene;sc.render.engine='CYCLES'
 sc.cycles.samples=24
 sc.render.resolution_x=960;sc.render.resolution_y=720
 sc.render.resolution_percentage=100
 sc.render.image_settings.file_format='PNG'
 sc.render.filepath=str(ROOT/(stem+'-blender.png'))
 bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/(stem+'.blend')))
 bpy.ops.render.render(write_still=True)
 metadata={'name':title,'glb':stem+'.glb','units':'metres','up':'+Z',
    'dimensions_m':[round(2.32*rx,3),round(2.32*ry,3),.07],
    'water_material':water.name,'water_alpha':.69,'glb_bytes':glb.stat().st_size,
    'parts':len(originals),'license':'Apache-2.0, PrawkoNaRaz authored',
    'note':'planar reflection is a renderer feature enabled only during rain'}
 (ROOT/(stem+'-metadata.json')).write_text(json.dumps(metadata,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 assert glb.stat().st_size>15000
 print('R6B2H_BLENDER_GLB_PASS',stem,json.dumps(metadata,ensure_ascii=False))

if __name__=='__main__':
 export('r6b2h-kaluza-nieregularna-240','Kałuża nieregularna 2,4 m',62024,1.04,.60)
 export('r6b2h-kaluza-placowa-140','Kałuża płytka 1,4 m',62025,.61,.34)
 print('R6B2H_TWO_ASSETS_EXPORTED_PASS')
