import bpy
from math import radians
from pathlib import Path
from mathutils import Vector

ROOT = Path(r"F:\PrawkoRoadStudio")
OUT = ROOT / "assets" / "prawko" / "blender"
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)

def material(name,color,metal=0.0,rough=0.5):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1)
    m.use_nodes=True
    b=m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value=(*color,1)
    b.inputs["Metallic"].default_value=metal
    b.inputs["Roughness"].default_value=rough
    return m

white=material("Prawko • biel odblaskowa",(0.94,0.94,0.87),0.02,0.34)
red=material("Prawko • czerwien ostrzegawcza",(0.73,0.047,0.039),0.08,0.33)
orange=material("Prawko • ochra obudowy",(0.8,0.27,0.016),0.18,0.35)
steel=material("Prawko • stal ocynkowana",(0.36,0.43,0.48),0.8,0.28)
dark=material("Prawko • gumowe podstawy",(0.065,0.07,0.074),0.06,0.74)
black=material("Prawko • czarny polimer",(0.029,0.036,0.044),0.04,0.63)
bolt=material("Prawko • sruby stalowe",(0.62,0.65,0.68),0.9,0.2)

def box(name, loc, dims, mat, bevel=0.012):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    ob=bpy.context.object;ob.name=name
    ob.dimensions=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    ob.data.materials.append(mat)
    if bevel:
        mod=ob.modifiers.new("Rounded manufactured edges","BEVEL")
        mod.width=bevel;mod.segments=2
        norm=ob.modifiers.new("Weighted normals","WEIGHTED_NORMAL")
        bpy.context.view_layer.objects.active=ob
        bpy.ops.object.modifier_apply(modifier=mod.name)
        bpy.ops.object.modifier_apply(modifier=norm.name)
    return ob
def cyl(name,loc,radius,depth,mat,vertices=16):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=loc)
    ob=bpy.context.object;ob.name=name;ob.data.materials.append(mat)
    bev=ob.modifiers.new("Safe rolled edges","BEVEL");bev.width=0.005;bev.segments=2
    ob.modifiers.new("Weighted normal","WEIGHTED_NORMAL")
    bpy.context.view_layer.objects.active=ob
    bpy.ops.object.modifier_apply(modifier=bev.name)
    return ob
def stripe(name, center_x, front_y, z, width=0.185):
    # Diagonal reflecting red/white board bands, fully modeled geometry.
    x=center_x
    v=[(x-width/2,-front_y,z-0.19),(x+width/2,-front_y,z-0.19),
       (x+width/2+0.17,-front_y,z+0.19),(x-width/2+0.17,-front_y,z+0.19)]
    me=bpy.data.meshes.new(name);me.from_pydata(v,[],[(0,1,2,3)]);me.update()
    ob=bpy.data.objects.new(name,me);bpy.context.collection.objects.link(ob);me.materials.append(red)
    sol=ob.modifiers.new("Raised reflective film","SOLIDIFY");sol.thickness=0.002
    bpy.context.view_layer.objects.active=ob;ob.select_set(True)
    bpy.ops.object.modifier_apply(modifier=sol.name);ob.select_set(False)

# Asymmetric portable lane-closure barricade, 2.2 m wide, 1.35 high.
for x,label in [(-0.86,"L"),(0.86,"R")]:
    box(f"{label} – stabilizujaca stopa z gumy",(x,0.0,0.072),(0.52,0.68,0.145),dark,0.04)
    box(f"{label} – stalowa plyta stopki",(x,0,0.155),(0.25,0.33,0.025),steel,0.01)
    cyl(f"{label} – rura pionowa",(x,0.0,0.655),0.044,0.95,steel,20)
    cyl(f"{label} – gorny korek",(x,0.0,1.14),0.051,0.027,black,20)
    for dx in [-0.095,0.095]:
        cyl(f"{label} – kotwa stalowa",(x+dx,-0.10,0.178),0.014,0.018,bolt,12)

box("Rama – poziomy profil gorny",(0,0,1.125),(2.03,0.075,0.085),steel,0.015)
box("Rama – profil dolny",(0,0,0.64),(2.03,0.068,0.052),steel,0.01)
box("Tablica – profil aluminiowy",(0,-0.045,0.895),(2.05,0.072,0.47),orange,0.024)
box("Tablica – odblaskowa biala powierzchnia",(0,-0.087,0.9),(1.955,0.013,0.392),white,0.006)
for i in range(8):
    stripe(f"Czerwony pas odblaskowy {i+1}",-0.93+i*0.255,0.096,0.9,width=0.142)
# Both approaches to a temporary barrier should show the reflective chevrons.
box("Tablica – tylny bialy odblask",(0,0.087,0.9),(1.955,0.013,0.392),white,0.006)
for i in range(8):
    stripe(f"Czerwony pas tylny {i+1}",-0.93+i*0.255,-0.102,0.9,width=0.142)
# Deliberately asymmetrical blue/black marker and an orange safety lamp.
box("Prawko – identyfikator producenta",(-0.67,-0.103,1.085),(0.23,0.004,0.040),black,0.003)
cyl("Jedna lampa ostrzegawcza – obudowa",(0.79,0.0,1.218),0.067,0.1,orange,24)
cyl("Jedna lampa ostrzegawcza – pokrywa",(0.79,0.0,1.271),0.052,0.012,white,24)
for x in (-0.86,0.86):
    cyl("Gniazdo rury ze sruba",(x,-0.046,0.65),0.027,0.07,bolt,12)

# GLB must contain only the actual prefab geometry, not the camera or studio lights.
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/"prawko_bariera_drogowa.blend"))
bpy.ops.export_scene.gltf(filepath=str(OUT/"prawko_bariera_drogowa.glb"),
                          export_format='GLB',export_yup=True,
                          export_materials='EXPORT',
                          export_cameras=False,export_lights=False)
print("PRAWKO_BLENDER_MODEL_OK",len(bpy.data.objects),
      (OUT/"prawko_bariera_drogowa.glb").stat().st_size)
