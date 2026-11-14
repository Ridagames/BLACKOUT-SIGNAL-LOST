#!/usr/bin/env python3
"""Convert the NEXUS-9 OBJ into a textured glTF 2.0 scene for the PS4 GNM renderer."""
import json, math, os, struct, sys, zlib
from collections import defaultdict

SRC = sys.argv[1] if len(sys.argv) > 1 else "assets/MODELS/NEXUS9_MENU.OBJ"
OUT = sys.argv[2] if len(sys.argv) > 2 else "build/gpu-model"
os.makedirs(OUT, exist_ok=True)

def png_chunk(kind, payload):
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xffffffff)

def make_png(kind, size=128):
    rows=[]
    for y in range(size):
        row=bytearray([0])
        for x in range(size):
            n=((x*37+y*71+(x*y*13))%19)-9
            seam=(x%32 in (0,1) or y%32 in (0,1))
            if kind in ("floor","wall","ceiling","metal","darkmetal"):
                base={"floor":(49,57,59),"wall":(65,75,78),"ceiling":(40,49,53),"metal":(92,104,108),"darkmetal":(24,31,35)}[kind]
                v=-24 if seam else n
                if kind=="floor" and (x%32 in (2,3) or y%32 in (2,3)): v-=7
                r,g,b=(max(0,min(255,c+v)) for c in base)
            elif kind=="warning":
                stripe=((x+y)//12)%2
                r,g,b=(190+n,76+n,27+n) if stripe==0 else (24+n,27+n,28+n)
            elif kind=="light":
                r,g,b=(205+n,190+n,151+n)
            elif kind=="glass":
                r,g,b=(17+n//2,39+n,47+n)
                if (x+y)%47<2: r,g,b=(80,118,123)
            elif kind=="bio":
                vein=(math.sin(x*0.16+math.sin(y*0.11)*2)+math.sin(y*0.21+x*0.05))>1.25
                r,g,b=(105+n,24+n//2,34+n//2) if vein else (38+n,12+n//2,20+n//2)
            else:
                r,g,b=(70+n,70+n,70+n)
            row.extend((max(0,min(255,int(r))),max(0,min(255,int(g))),max(0,min(255,int(b))),255))
        rows.append(bytes(row))
    raw=b"".join(rows)
    return b"\x89PNG\r\n\x1a\n"+png_chunk(b"IHDR",struct.pack(">IIBBBBB",size,size,8,6,0,0,0))+png_chunk(b"IDAT",zlib.compress(raw,9))+png_chunk(b"IEND",b"")

materials = ["floor","ceiling","wall","darkmetal","metal","warning","light","glass","bio"]
mat_props = {
 "floor":(0.72,0.78),"ceiling":(0.58,0.82),"wall":(0.62,0.72),
 "darkmetal":(0.88,0.42),"metal":(0.92,0.28),"warning":(0.35,0.62),
 "light":(0.05,0.3),"glass":(0.65,0.16),"bio":(0.02,0.86)
}
vertices=[]
groups=defaultdict(list)
active="wall"
with open(SRC,encoding="utf-8",errors="ignore") as f:
    for line in f:
        if line.startswith("v "):
            p=line.split()
            vertices.append(tuple(map(float,p[1:4])))
        elif line.startswith("usemtl "):
            active=line.split(None,1)[1].strip().lower()
            if active not in materials:
                active="metal"
        elif line.startswith("f "):
            refs=line.split()[1:]
            ids=[]
            for ref in refs:
                raw=ref.split("/")[0]
                idx=int(raw)
                ids.append(idx-1 if idx>0 else len(vertices)+idx)
            for k in range(1,len(ids)-1):
                tri=(ids[0],ids[k],ids[k+1])
                a,b,c=(vertices[i] for i in tri)
                ux,uy,uz=(b[i]-a[i] for i in range(3))
                vx,vy,vz=(c[i]-a[i] for i in range(3))
                nx,ny,nz=(uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx)
                ln=math.sqrt(nx*nx+ny*ny+nz*nz) or 1.0
                normal=(nx/ln,ny/ln,nz/ln)
                for idx in tri:
                    x,y,z=vertices[idx]
                    # Preserve the OBJ +Z corridor axis and the original renderer's +5m depth offset.
                    pos=(x,y,z+5.0)
                    uv=(x*0.22,z*0.22)
                    metal,rough=mat_props[active]
                    groups[active].append((pos,normal,uv,(metal,rough)))

blob=bytearray()
buffer_views=[]
accessors=[]
def add_view(data, stride=None):
    while len(blob)%4: blob.append(0)
    off=len(blob); blob.extend(data)
    view={"buffer":0,"byteOffset":off,"byteLength":len(data)}
    if stride: view["byteStride"]=stride
    buffer_views.append(view)
    return len(buffer_views)-1
def add_accessor(view, component, count, typ, offset=0, mn=None, mx=None):
    a={"bufferView":view,"byteOffset":offset,"componentType":component,"count":count,"type":typ}
    if mn is not None: a["min"]=mn
    if mx is not None: a["max"]=mx
    accessors.append(a); return len(accessors)-1

images=[]; textures=[]; gltf_materials=[]; primitives=[]
for mi,name in enumerate(materials):
    texbytes=make_png(name)
    view=add_view(texbytes)
    images.append({"bufferView":view,"mimeType":"image/png","name":name+"_surface"})
    textures.append({"sampler":0,"source":mi})
    metal,rough=mat_props[name]
    gltf_materials.append({
        "name":name.upper(),
        "pbrMetallicRoughness":{
            "baseColorFactor":[1.0,1.0,1.0,1.0],
            "baseColorTexture":{"index":mi},
            "metallicFactor":metal,"roughnessFactor":rough
        }
    })
    tris=groups.get(name,[])
    if not tris: continue
    packed=bytearray(); indices=[]; mins=[1e30]*3; maxs=[-1e30]*3
    for tri in tris:
        for pos,norm,uv,mr in tri:
            for j,v in enumerate(pos): mins[j]=min(mins[j],v); maxs[j]=max(maxs[j],v)
            packed.extend(struct.pack("<10f",*(pos+norm+uv+mr)))
            indices.append(len(indices))
    vview=add_view(bytes(packed),40)
    pacc=add_accessor(vview,5126,len(indices), "VEC3",0,mins,maxs)
    nacc=add_accessor(vview,5126,len(indices), "VEC3",12)
    uacc=add_accessor(vview,5126,len(indices), "VEC2",24)
    mracc=add_accessor(vview,5126,len(indices), "VEC2",32)
    iview=add_view(struct.pack("<"+"I"*len(indices),*indices))
    iacc=add_accessor(iview,5125,len(indices),"SCALAR")
    primitives.append({"attributes":{"POSITION":pacc,"NORMAL":nacc,"TEXCOORD_0":uacc,"TEXCOORD_1":mracc},"indices":iacc,"material":mi,"mode":4})

gltf={
 "asset":{"version":"2.0","generator":"BLACKOUT NEXUS-9 OBJ-to-glTF pipeline"},
 "scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"mesh":0,"name":"NEXUS-9 industrial corridor"}],
 "meshes":[{"name":"NEXUS-9 corridor","primitives":primitives}],
 "materials":gltf_materials,
 "textures":textures,"images":images,"samplers":[{"magFilter":9729,"minFilter":9987,"wrapS":10497,"wrapT":10497}],
 "buffers":[{"uri":"NEXUS9_MENU.bin","byteLength":len(blob)}],
 "bufferViews":buffer_views,"accessors":accessors
}
with open(os.path.join(OUT,"NEXUS9_MENU.gltf"),"w",encoding="utf-8") as f: json.dump(gltf,f,separators=(",",":"))
with open(os.path.join(OUT,"NEXUS9_MENU.bin"),"wb") as f: f.write(blob)
print("GLTF_READY materials=%d primitives=%d vertices=%d bytes=%d" % (len(materials),len(primitives),sum(len(v)//3 for v in groups.values()),len(blob)))
