# SPDX-License-Identifier: Apache-2.0
import bpy,math
from mathutils import Vector
from pathlib import Path
folder=Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(folder/'prawko_wiata_przystankowa.blend'))
scene=bpy.context.scene
# Evaluation backdrop is NOT written into the source blend or GLB.
mat=bpy.data.materials.new("Demo studio slate floor");mat.diffuse_color=(.29,.32,.33,1)
bpy.ops.mesh.primitive_plane_add(size=18,location=(0,0,-.02))
bpy.context.object.data.materials.append(mat)
def point(obj,target):
 obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(7.5,-9.0,5.1))
camera=bpy.context.object;point(camera,(0,0,1.27));camera.data.type='PERSP';camera.data.lens=33
scene.camera=camera
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.render.resolution_x=1280;scene.render.resolution_y=850;scene.render.resolution_percentage=100
scene.world.color=(.64,.67,.70)
for name,loc,power,size in [('Softbox',(0,-4,9),1800,6),('Rim',(5,4,6),980,4)]:
 bpy.ops.object.light_add(type='AREA',location=loc)
 lamp=bpy.context.object;lamp.name=name;lamp.data.energy=power;lamp.data.shape='DISK';lamp.data.size=size
 point(lamp,(0,0,1.1))
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(folder/'prawko_wiata_blender_preview.png')
bpy.ops.render.render(write_still=True)
print('PRAWKO_A1_BLENDER_RENDER_PASS',scene.render.filepath)
