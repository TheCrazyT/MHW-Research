#!/usr/bin/python2
import pefile
import sys
import sqlite3
import md5
import numpy as np
from binascii import hexlify
from struct import *

def findFunctionTable(off,relocSectionData):
    global pe,sectionInfos
    for i in range(off,off+30):
        #0xE8 = call
        #0x48 = lea
        if (ord(relocSectionData[i]) == 0xE8)and(ord(relocSectionData[i+5]) == 0x48):
            relOff = unpack("i",relocSectionData[i+8:i+8+4])[0]
            n = i+7+5+relOff
            n += pe.OPTIONAL_HEADER.ImageBase + sectionInfos[".reloc"][1]
            return n
    return None
def dict_factory(cursor, row):
    d = {}
    for idx, col in enumerate(cursor.description):
        d[col[0]] = row[idx]
    return d       

conn = sqlite3.connect('cache.db')
conn.row_factory = dict_factory
conn.execute('''CREATE TABLE IF NOT EXISTS raw_off_cache
             (s text, offset number)''')
conn.execute('''CREATE TABLE IF NOT EXISTS function_table_pointers_cache
             (s text, voffset number)''')
conn.execute('''CREATE TABLE IF NOT EXISTS config
             (name text,value text,value_int number)''')

file_hash = None
for row in conn.execute('SELECT value FROM config where name="file_hash"'):             
    file_hash = row["value"]

print("Loading ...")
sys.stdout.flush()
pefile.fast_load = True
pe =  pefile.PE('MonsterHunterWorld.exe')
print("\n\n")
#print(pe.dump_info())
#2e98b18 nDraw::Material
#0000000149558F51 | 48:8D15 C00F94F9         | lea rdx,qword ptr ds:[142E99F18]        | rdx:EntryPoint, 0000000142E99F18:"nDraw::Material"
#0x142E99F18-0x149558F51=0xFFFFFFFFF9940FC7
sectionInfos = {}
for section in pe.sections:
  name = section.Name
  if "\x00" in name:
    name = name[:name.index("\x00")]
  sectionInfos[name] = (section,section.VirtualAddress,section.PointerToRawData, section.SizeOfRawData )

tlsSectionData = sectionInfos[".tls"][0].get_data()
relocSectionData = sectionInfos[".reloc"][0].get_data()
m = md5.new()
m.update(tlsSectionData)
m.update(relocSectionData)

digest = hexlify(m.digest())

if(digest != file_hash):
    print("file changed,removing cache [%s != %s]" % (digest,file_hash))
    conn.execute('DELETE FROM raw_off_cache')
    conn.execute('DELETE FROM function_table_pointers_cache')
    if file_hash is None:
        conn.execute('INSERT INTO config (value,name) VALUES (?,?)',(digest,'file_hash'))
        conn.commit()
    else:
        conn.execute('UPDATE config SET value=? WHERE name="file_hash"',digest)
        conn.commit()

rawOffsCache = {}
fTblCache = {}
for row in conn.execute('SELECT s,offset FROM raw_off_cache'):
    rawOffsCache[row["s"]] = row["offset"]
for row in conn.execute('SELECT s,voffset FROM function_table_pointers_cache'):
    fTblCache[row["s"]] = row["voffset"]

    
    
virtualOffs = {}

with open("strings.txt") as infile:
#with open("test.txt") as infile:
    i = 0
    for s in infile:
        i += 1
        s = s.strip()
        k = 0
        if s in rawOffsCache:
            k = rawOffsCache[s]
        else:
            try:
                k = tlsSectionData.index("%s\x00" % s)
                conn.execute('INSERT INTO raw_off_cache VALUES (?,?)', (s,k))
                if(i % 50 == 0):
                    conn.commit()
            except ValueError as e:
                conn.execute('INSERT INTO raw_off_cache VALUES (?,?)', (s,None))
                if(i % 50 == 0):
                    conn.commit()
                continue
        if (not(k is None)) and (k > 0):
            virtualOffs[s] = pe.OPTIONAL_HEADER.ImageBase + k + sectionInfos[".tls"][1]

            
print("len fTblCache: %d\nlen virtualOffs: %d\nend: %d\n\n" % (len(fTblCache),len(virtualOffs),len(relocSectionData))) 
conn.commit()
print('"Name","FunctionTable offset"')
sys.stdout.flush()

N1 = (len(relocSectionData) / 4)
N2 = (len(relocSectionData[1:]) / 4)
N3 = (len(relocSectionData[2:]) / 4)
N4 = (len(relocSectionData[3:]) / 4)

# "splitting" this improves performance, sadly now you need about 9GB ram just for this python script.
cmpN1 = np.array(list(unpack("%di" % N1,relocSectionData[0:N1*4])))
cmpN2 = np.array(list(unpack("%di" % N2,relocSectionData[1:1+N2*4])))
cmpN3 = np.array(list(unpack("%di" % N3,relocSectionData[2:2+N3*4])))
cmpN4 = np.array(list(unpack("%di" % N4,relocSectionData[3:3+N4*4])))

X = pe.OPTIONAL_HEADER.ImageBase + sectionInfos[".reloc"][1] + 4
L1 = np.arange(0,len(cmpN1)*4,4)
L2 = np.arange(1,len(cmpN2)*4,4)
L3 = np.arange(2,len(cmpN3)*4,4)
L4 = np.arange(3,len(cmpN4)*4,4)

cmpN1 += X+L1
cmpN2 += X+L2
cmpN3 += X+L3
cmpN4 += X+L4

for s in fTblCache:
    fTbl = fTblCache[s]
    print("\"%s\",0x%016x" % (s,fTbl))
    sys.stdout.flush()
    del virtualOffs[s]

    
    
for s in virtualOffs:
    v = virtualOffs[s]
    x = 0
    X = np.where(cmpN1 == v)[0]
    if(len(X)>0):
        x = X[0]
    i = 0
    if x <= 0:
        X = np.where(cmpN2 == v)[0]
        if(len(X)>0):
            x = X[0]
        if x > 0:
            i = 1
        else:
            X = np.where(cmpN3 == v)[0]
            if(len(X)>0):
                x = X[0]
            if x > 0:
                i = 2
            else:
                X = np.where(cmpN4 == v)[0]
                if(len(X)>0):
                    x = X[0]
                if x > 0:
                    i = 3
    i += x*4
    
    if(x > 0):
        fTbl = findFunctionTable(i,relocSectionData)
        if not fTbl is None:
            conn.execute('INSERT INTO function_table_pointers_cache (s,voffset) VALUES (?,?)', (s,fTbl))
            conn.commit()
            print("\"%s\",0x%016x" % (s,fTbl))
            sys.stdout.flush()
        
