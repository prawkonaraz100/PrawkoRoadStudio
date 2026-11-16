# SPDX-License-Identifier: Apache-2.0
"""Original streetlamp upgraded to a genuine glTF emissive luminaire.
Blender 5.3, uses only the existing Prawko-authored GLB. No stock assets.
Independent, draggable and rotatable library model.
"""
import bpy, math, json
from pathlib import Path
from mathutils import Vector
root=Path(r'F:\PrawkoRoadStudio')
src=root/'assets/prawko/blender/ulica_r6b/r6b-latarnia.glb'
out=Path(__file__).resolve().parent
stem='r6b2g-latarnia-emisyjna'
assert src.exists()
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(src))
parts=[o for o in bpy.context.scene.objects if o.type=='MESH']
assert 3<=len(parts)<=20,len(parts)
lens_parts=[]
for o in parts:
 for material in o.data.materials:
  if material and ('klosz' in material.name.lower() or 'szklo jasne' in material.name.lower()):
   lens_parts.append(o)
   # Blender glTF exporter maps Principled BSDF emission to glTF emissiveFactor.
   material.use_nodes=True
   bsdf=material.node_tree.nodes.get('Principled BSDF')
   assert bsdf
   bsdf.inputs['Base Color'].default_value=(.82,.75,.59,1)
   bsdf.inputs['Roughness'].default_value=.28
   bsdf.inputs['Emission Color'].default_value=(1.0,.79,.48,1.0)
   bsdf.inputs['Emission Strength'].default_value=1.0
assert lens_parts,'Lens mesh/material missing in original Blender asset'
bpy.ops.object.select_all(action='DESELECT')
for obj in parts:obj.select_set(True)
bpy.context.view_layer.objects.active=parts[0]
glb=out/(stem+'.glb')
bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',
 use_selection=True,export_apply=True,export_materials='EXPORT',
 export_normals=True,export_texcoords=True,export_cameras=False,export_lights=False)
# Preview studio: render actual Blender model with visible warm emissive lens.
bpy.ops.object.select_all(action='DESELECT')
bpy.ops.object.camera_add(location=(8,-11,9))
cam=bpy.context.object
cam.rotation_euler=(Vector((0,0,3.0))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=9.5
bpy.context.scene.camera=cam
bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,-.035))
floor=bpy.context.object;floor.scale=(9,9,1)
ground=bpy.data.materials.new('PREVIEW ONLY concrete');ground.diffuse_color=(.32,.34,.34,1)
floor.data.materials.append(ground)
for pos,power in [((7,-10,13),900),((-5,4,10),450)]:
 bpy.ops.object.light_add(type='AREA',location=pos)
 o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=6
 o.rotation_euler=(Vector((0,0,3))-o.location).to_track_quat('-Z','Y').to_euler()
scene=bpy.context.scene
scene.render.engine='CYCLES';scene.cycles.samples=20
scene.render.resolution_x=960;scene.render.resolution_y=720
scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(out/(stem+'-blender.png'))
bpy.ops.wm.save_as_mainfile(filepath=str(out/(stem+'.blend')))
bpy.ops.render.render(write_still=True)
meta={'title':'Latarnia uliczna z emisyjnym kloszem','source':str(src.relative_to(root)),
 'format':'glTF2 GLB','emissive_factor': [1.0,.79,.48],
 'lens_meshes':len(lens_parts),'model_meshes':len(parts),
 'glb_bytes':glb.stat().st_size,'creator':'Blender '+bpy.app.version_string}
(out/(stem+'-metadata.json')).write_text(json.dumps(meta,ensure_ascii=False,indent=2),encoding='utf-8')
print('R6B2G_EMISSIVE_GLB_PASS',json.dumps(meta,ensure_ascii=False))
