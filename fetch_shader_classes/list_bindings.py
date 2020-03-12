# -*- coding: utf-8 -*-
"""
Created on Tue Jan 21 17:25:58 2020

@author: AsteriskAmpersand
"""

from pathlib import Path

def removeWhitespace(string):
    return ' '.join(string.split())

htm = open("ListBindingNames.ryo","w")
print = htm.write
nameSet = set()
nameList = []

for filePath in Path(r"E:\MHW Shader Research\MHW-Research\shdr").rglob("*.src"):
    file = filePath.open("r")
    #print(filePath)
    for line in file:
        if "// Resource Bindings:" in line:
            next(file)
            header = next(file).replace("\r","").replace("\n","")
            header = removeWhitespace(header).split(" ")
            next(file)
            for line in file:
                if line.replace("\r","").replace("\n","") == "//":
                    break
                line = line.replace("\r","").replace("\n","")
                comment, bname,btype,bformat,bdim,bslot,belements = removeWhitespace(line).split(" ")
                if bname not in nameSet:
                    nameSet.add(bname)
                    nameList.append(bname)
print(','.join(nameList))
htm.close()
