# SPDX-License-Identifier: Apache-2.0
"""Prawko PL signs P0: actual Blender-built 3D geometry, no stock models or raster billboards.
Four medium-size Polish sign faces on realistically galvanized mounts. Front is -Y.
Art is an authored geometric interpretation of official diagrams; regulatory graphic
audit must be completed before asserting legally exact reproduction.
"""
import bpy, math, sys, json, os
from pathlib import Path
from mathutils import Vector
argv=sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else []
if len(argv)!=1: raise SystemExit("Expected exactly one isolated output folder")
OUT=Path(argv[0]).resolve()
if not OUT.is_dir() or any(OUT.iterdir()): raise SystemExit("Output must be an existing empty staging directory")
Z=2.45

def material(name,rgb,metal=.0,rough=.45):
    m=bpy.data.materials.new(name);m.use_nodes=True
    b=m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value=(*rgb,1)
    b.inputs["Metallic"].default_value=metal
    b.inputs["Roughness"].default_value=rough
    m.diffuse_color=(*rgb,1)
    return m
def reset():
    bpy.ops.object.select_all(action="SELECT");bpy.ops.object.delete(use_global=False)
    return []
def attach(obj,name,mat,parts,bevel=0):
    obj.name=name;obj.data.materials.clear();obj.data.materials.append(mat)
    if bevel:
        m=obj.modifiers.new("Soft fabricated edge","BEVEL");m.width=bevel;m.segments=3
        obj.modifiers.new("Accurate weighted corner normals","WEIGHTED_NORMAL")
    parts.append(obj);return obj
def cube(name,position,dims,mat,parts,bevel=0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=position)
    o=bpy.context.object;o.dimensions=dims
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    return attach(o,name,mat,parts,bevel)
def cyl(name,position,r,depth,mat,parts,axis=None,n=48):
    bpy.ops.mesh.primitive_cylinder_add(vertices=n,radius=r,depth=depth,location=position)
    o=bpy.context.object
    if axis is not None:o.rotation_euler=Vector(axis).to_track_quat("Z","Y").to_euler()
    return attach(o,name,mat,parts)
def polygon(name,flatpoints,front_y,depth,mat,parts):
    # Flatpoints coordinates in sign local horizontal/vertical metres.
    n=len(flatpoints)
    verts=[(x,front_y,Z+z) for x,z in flatpoints]+[(x,front_y+depth,Z+z) for x,z in flatpoints]
    faces=[tuple(range(n-1,-1,-1)),tuple(range(n,n*2))]
    faces.extend((i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n))
    mesh=bpy.data.meshes.new(name+" mesh");mesh.from_pydata(verts,[],faces);mesh.update()
    o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o)
    return attach(o,name,mat,parts)
def octagon(r):
    t=r*(math.sqrt(2)-1)
    return [(-t,r),(t,r),(r,t),(r,-t),(t,-r),(-t,-r),(-r,-t),(-r,t)]
def r_square(side,r):
    h=side/2
    centers=[(h-r,h-r,0),(-h+r,h-r,90),(-h+r,-h+r,180),(h-r,-h+r,270)]
    coords=[]
    for cx,cz,deg in centers:
        for k in range(9):
            a=math.radians(deg+k*90/8)
            coords.append((cx+math.cos(a)*r,cz+math.sin(a)*r))
    return coords
def circle_polygon(r,n=96):
    return [(r*math.cos(i*2*math.pi/n),r*math.sin(i*2*math.pi/n)) for i in range(n)]
def text_mesh(name,value,pos,size,mat,parts,bold=False):
    bpy.ops.object.text_add(location=pos,rotation=(math.pi/2,0,0))
    o=bpy.context.object;o.name=name;o.data.body=value
    o.data.align_x="CENTER";o.data.align_y="CENTER";o.data.size=size
    o.data.space_character=1.00;o.data.extrude=.0004;o.data.bevel_depth=.00025
    paths=([r"C:\Windows\Fonts\arialbd.ttf",r"C:\Windows\Fonts\ARIALBD.TTF"] if bold else
           [r"C:\Windows\Fonts\arial.ttf",r"C:\Windows\Fonts\ARIAL.TTF"])
    for file in paths:
        if os.path.isfile(file):
            o.data.font=bpy.data.fonts.load(file,check_existing=True);break
    bpy.ops.object.convert(target="MESH")
    attach(bpy.context.object,name,mat,parts)
def common(parts,shape):
    galv=material("Hot dip galvanized steel / ISO 1461",(.48,.51,.53),.75,.38)
    steel=material("Steel cut edge RAL 7042",(.28,.30,.31),.7,.46)
    concrete=material("Ground collar / cast footing",(.32,.33,.33),.05,.91)
    cyl("Steel pole OD 60 mm hot-dip galvanized",(0,.075,1.345),.030,2.69,galv,parts,n=32)
    cyl("Steel pole weather cap",(0,.075,2.689),.032,.014,steel,parts,n=32)
    cyl("Mast foot flange 160mm",(0,.075,.065),.081,.020,galv,parts,n=32)
    cyl("Concrete base collar",(0,.075,.025),.13,.048,concrete,parts,n=40)
    for x in (-.065,.065):
        for y in (-.065,.065):
            cyl("Galvanized base M10 hex head",(x,.075+y,.080),.011,.013,galv,parts,n=12)
    for height in (2.30,2.58):
        cube("Folded sign-to-mast omega bracket",(0,-.001,height),(.175,.153,.038),galv,parts,.008)
        for x in (-.075,.075):
            cyl("Stainless M8 bolt and washer",(x,-.094,height),.011,.013,galv,parts,(0,1,0),n=12)
    return galv,steel
def face_materials():
    return {
      "white":material("Retroreflective-white type II (PBR proxy)",(.86,.88,.87),.06,.24),
      "red":material("Traffic red foil",(.67,.006,.012),.05,.23),
      "yellow":material("Polish yellow foil",(.99,.61,.012),.04,.26),
      "blue":material("Traffic blue foil",(.018,.11,.47),.04,.23),
      "black":material("Traffic black symbol",(.008,.012,.017),.01,.47),
      "rim":material("Rolled zinc coating edge",(.36,.395,.405),.78,.40),
      "back":material("Galvanized reverse sheet",(.44,.48,.50),.79,.51),
    }
def add_back(parts,shape,material,steel):
    if shape=="triangle":
        o=[(-.45,.260),(.45,.260),(0,-.520)]
    elif shape=="stop":o=octagon(.40)
    elif shape=="circle":o=circle_polygon(.40)
    else:o=r_square(.60,.031)
    polygon("3D stamped steel reverse sheet",o,-.068,.017,material["back"],parts)
    polygon("Extruded metal rolled lip",o,-.102,.029,material["rim"],parts)
    return o
def draw_a7(parts,m):
    o=[(-.45,.260),(.45,.260),(0,-.520)]
    polygon("A-7 full red reflector triangle",o,-.116,.012,m["red"],parts)
    inside=[(-.341,.203),(.341,.203),(0,-.389)]
    polygon("A-7 proper yellow centre",inside,-.121,.005,m["yellow"],parts)
def draw_stop(parts,m):
    polygon("B-20 thin white border",octagon(.374),-.117,.006,m["white"],parts)
    polygon("B-20 red field inset border",octagon(.352),-.125,.006,m["red"],parts)
    text_mesh("B-20 raised white STOP lettering","STOP",(0,-.137,Z+.002),.195,m["white"],parts)
def draw_b33(parts,m):
    polygon("B-33 white disc foil",circle_polygon(.388),-.116,.006,m["white"],parts)
    polygon("B-33 wide red surround",circle_polygon(.367),-.125,.006,m["red"],parts)
    polygon("B-33 white central disc",circle_polygon(.281),-.132,.006,m["white"],parts)
    text_mesh("B-33 black 50 km/h number","50",(0,-.145,Z-.005),.325,m["black"],parts,bold=True)
def draw_d6(parts,m):
    polygon("D-6 thin bright perimeter",r_square(.579,.028),-.116,.004,m["white"],parts)
    polygon("D-6 blue rectangular face",r_square(.555,.024),-.122,.004,m["blue"],parts)
    # Sign-specific Polish upward white triangular crossing field.
    polygon("D-6 white pedestrian-background triangle",
            [(-.274,-.232),(.274,-.232),(0,.265)],-.129,.004,m["white"],parts)
    # Three interrupted stripes at bottom: avoid human legs crossing strips.
    for k,(x0,x1) in enumerate([(-.235,-.090),(-.055,.055),(.115,.235)]):
        polygon("D-6 black crossing stripe "+str(k),
                [(x0,-.180),(x1,-.180),(x1+.014,-.158),(x0+.014,-.158)],
                -.136,.003,m["black"],parts)
    cyl("D-6 black circular pedestrian head",(-.004,-.140,Z+.149),.024,.004,m["black"],parts,axis=(0,1,0),n=40)
    # Author-created faceted silhouette: torso, swinging arms, striding legs.
    shapes=[
      [(-.006,.100),(.050,.079),(.083,.021),(.066,-.006),(.028,.036),(.029,-.035),(-.002,-.077),(-.033,-.019)],
      [(-.033,.061),(-.002,.049),(-.078,-.036),(-.103,-.021)],
      [(.055,.070),(.087,.015),(.085,-.069),(.065,-.083),(.064,-.002),(.028,.040)],
      [(-.004,-.060),(.028,-.073),(.119,-.214),(.077,-.238),(.018,-.136),(-.039,-.086)],
      [(-.024,-.061),(.012,-.073),(-.055,-.153),(-.074,-.222),(-.122,-.213),(-.103,-.194),(-.090,-.155)]
    ]
    for i,poly in enumerate(shapes):
        polygon("D-6 walking pedestrian silhouette part "+str(i),poly,-.140,.004,m["black"],parts)
def preview(stem,parts):
    bpy.ops.object.select_all(action="DESELECT")
    bpy.ops.object.camera_add(location=(.85,-4.50,3.16))
    cam=bpy.context.object;target=Vector((0,0,1.57))
    cam.rotation_euler=(target-cam.location).to_track_quat("-Z","Y").to_euler()
    cam.data.type="ORTHO";cam.data.ortho_scale=3.46;bpy.context.scene.camera=cam
    bpy.ops.mesh.primitive_plane_add(size=2,location=(0,0,-.015))
    floor=bpy.context.object;floor.scale=(2.0,2.0,1)
    floor.data.materials.append(material("Preview asphalt",(.16,.18,.20),.0,.92))
    for loc,power in [((3,-4,6),850),((-3,-2,4),525)]:
        bpy.ops.object.light_add(type="AREA",location=loc)
        light=bpy.context.object;light.data.energy=power;light.data.size=3.1
        light.rotation_euler=(target-light.location).to_track_quat("-Z","Y").to_euler()
    scene=bpy.context.scene;scene.render.engine="CYCLES";scene.cycles.samples=16
    scene.render.resolution_x=512;scene.render.resolution_y=512
    scene.render.resolution_percentage=100;scene.render.image_settings.file_format="PNG"
    scene.render.filepath=str(OUT/(stem+"-blender.png"))
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(stem+".blend")))
    bpy.ops.render.render(write_still=True)
    img=bpy.data.images.load(scene.render.filepath,check_existing=False)
    img.scale(96,96);img.filepath_raw=str(OUT/(stem+"-thumbnail.png"));img.file_format="PNG";img.save()
    bpy.data.images.remove(img)
def publish(stem,description,group,shape,func):
    parts=reset();m=face_materials();common(parts,shape)
    add_back(parts,shape,m,m["rim"]);func(parts,m)
    bpy.ops.object.select_all(action="DESELECT")
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    p=OUT/(stem+".glb")
    bpy.ops.export_scene.gltf(filepath=str(p),export_format="GLB",
        use_selection=True,export_apply=True,export_normals=True,export_texcoords=True,
        export_materials="EXPORT",export_cameras=False,export_lights=False)
    if p.stat().st_size<12000:raise RuntimeError("Invalid short GLB: "+stem)
    bpy.ops.object.select_all(action="DESELECT")
    preview(stem,parts)
    tris=sum(sum(len(poly.vertices)-2 for poly in o.data.polygons)
             for o in parts if o.type=="MESH")
    data={"model":stem,"code":description,"group":"średni (S)","units":"metres",
       "up_axis":"Z","face_direction":"-Y","front_center_height_m":Z,
       "nominal_face_size_m":group,"physical_meshes":len(parts),"triangles":tris,
       "blend_version":bpy.app.version_string,"glb_bytes":p.stat().st_size,
       "source":"Autorska geometria PrawkoNaRaz; akty Dz.U.2019.2310/2311 ze zmianami, porównać ze wzorem",
       "legal_audit":"VISUAL_REVIEW_REQUIRED","license":"Apache-2.0"}
    (OUT/(stem+"-metadata.json")).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding="utf-8")
    print("PRAWKO_PL_SIGN_GENERATED",json.dumps(data,ensure_ascii=False),flush=True)

publish("pl-a7-ustap-pierwszenstwa","A-7 Ustąp pierwszeństwa","triangle side 0.9", "triangle",draw_a7)
publish("pl-b20-stop","B-20 STOP","octagon outer 0.8", "stop",draw_stop)
publish("pl-d6-przejscie-pieszych","D-6 Przejście dla pieszych","square 0.6", "square",draw_d6)
publish("pl-b33-50","B-33 Ograniczenie prędkości 50 km/h","circle diameter 0.8", "circle",draw_b33)
print("PRAWKO_BLENDER_PL_SIGNS_P0_PASS models=4",flush=True)
