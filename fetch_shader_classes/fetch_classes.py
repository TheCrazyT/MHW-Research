#/usr/bin/python
import os
import re

sg = open("mhwib_structures_generated.bt","w")
print = sg.write

print("#define row_major\n")
print("""typedef struct float4{
    float a[4];
};
typedef struct float3{
    float a[3];
};
typedef struct float2{
    float a[2];
};
typedef struct float4x4{
    float a[4*4];
};
typedef struct float3x4{
    float a[3*4];
};
typedef struct uint2{
    uint a[2];
};
typedef struct uint3{
    uint a[3];
};
typedef struct uint4{
    uint a[4];
};
typedef struct int2{
    int a[2];
};
typedef struct int3{
    int a[3];
};
typedef struct int4{
    int a[4];
};
typedef struct uint4x4{
    uint a[16];
};
typedef struct bool2{
    uint a[2];
};
typedef struct bool3{
    uint a[3];
};
typedef uint bool;""")

rootPath = r"..\shdr\src"
classFiles = {}
for root, dirs, files in os.walk(rootPath):
    for filename in files:
        with open(os.path.join(rootPath,filename), 'r') as content_file:
            content = content_file.read()
            result = re.findall("cbuffer ([^ ]+)\n",content)
            for n in result:
                classFiles[n] = filename

for key,filename in classFiles.items():
    print("struct %s;" % key);
print("")
print("""void createClassByHash(uint i){
    """)
for key,filename in classFiles.items():
    print("""   if(i == (crc%s & 0xFFFFF)){
                    %s data%s<bgcolor=0x00ffaa,optimize=false>;
                    return;
                }""" % (key,key,key));

print("}")
print("")
for key,filename in classFiles.items():
    print("local int crc%s<format=hex> = createJamcrc(\"%s\");" % (key,key));


print("")
for key,filename in classFiles.items():
    print("//%s : %s" % (key,filename))
    with open(os.path.join(rootPath,filename), 'r') as content_file:
        content = content_file.read()
        start = False
        pos = 0
        ai = 0
        for line in content.split("\n"):
            line = line.strip()
            line = line.replace("struct ","struct %s_" % key)
            if start == True:
                if line == "// {":
                    continue;
                if line == "// }":
                    print("};\n")
                    break;
                if not '=' in line:
                    m = re.search("Offset: +([0-9]+) Size: +([0-9]+)",line)
                    if(m):
                        offset = int(m.group(1))
                        if(offset != pos):
                            print("  // align: %d to %d" % (pos,offset))
                            print("  ubyte align%d[%d];" % (ai,offset-pos))
                            pos += offset-pos
                            ai += 1
                        pos += int(m.group(2));
                    else:
                        pass
                        #print("// no offset")
                    print (line[3:])
            else:
                if line == "// cbuffer "+key:
                    print("struct %s {\n" % key)
                    start = True
sg.close()
