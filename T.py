#!/usr/bin/env python

import os
from os import path

cwd = os.getcwd();

weems = []
wisps = []
banks = []
oggs = []
autos = []
files = (weems, wisps, banks, oggs, autos)

cmdargs = []

test = "test"

for r, ds, fs in os.walk(test):
    for f in sorted(fs):
        F = f.lower()
        p = path.join(test, f)
        #p=f
        if F.startswith("unknown"): autos.append(p)
        elif F.endswith(".ogg"): oggs.append(p)
        elif F.endswith(".wem"): weems.append(p)
        elif F.endswith(".wsp"): wisps.append(p)
        elif F.endswith(".bnk"): banks.append(p)
        else: autos.append(p)

for i,o in enumerate(oggs):
    arg = ["-ogg "]

    if (i&1):arg.append("-ri")

    arg.append(o)
    cmdargs.append(arg)
for i,w in enumerate(weems):
    arg = ["-weem"]

    if (i&1):arg.append("-oi")
    if (i&2):arg.append("-r")
    if (i&4):arg.append("-ri")

    arg.append(w)
    cmdargs.append(arg)
for i,W in enumerate(wisps):
    arg = ["-wisp"]

    if (i&1):arg.append("-O")
    if (i&2):arg.append("-oi")
    if (i&4):arg.append("-r")
    if (i&8):arg.append("-ri")

    arg.append(W)
    cmdargs.append(arg)
for i,b in enumerate(banks):
    arg = ["-bank"]

    if (i& 1):arg.append("-R")
    if (i& 2):arg.append("-O")
    if (i& 4):arg.append("-oi")
    if (i& 8):arg.append("-r")
    if (i&16):arg.append("-ri")

    arg.append(b)
    cmdargs.append(arg)
for i,a in enumerate(autos):
    arg = ["-auto"]

    if (i& 1):arg.append("-R")
    if (i& 2):arg.append("-O")
    if (i& 4):arg.append("-oi")
    if (i& 8):arg.append("-r")
    if (i&16):arg.append("-ri")

    arg.append(a)
    cmdargs.append(arg)

for i in range(len(cmdargs)):
    arg = []
    k = int((i)/2)%6

    if (i&1):[arg.append("-q") for _ in range(k)]
    elif (i-1&1):
        if(i-1&2):arg.append("-d")
        else:arg.append("-Q")
        if (i-1&4):arg.append("-c")

    [cmdargs[i].insert(1+l, j) for l,j in enumerate(arg)]

for i in range(len(cmdargs)):
    cmdargs[i] = " ".join([f"{j}" for j in cmdargs[i]])
cmd = "./NAeP " +  " ".join(cmdargs)
print(cmd)
os.system(cmd)

"""
     | 0 | 1 | 2 |
0 -w |   |   |   | 0 0 0
1 -w |-oi|   |   | 1 0 0
2 -w |   |-r |   | 0 1 0
3 -w |-oi|-r |   | 1 1 0
4 -w |   |   |-ri| 0 0 1
5 -w |-oi|   |-ri| 1 0 1
6 -w |   |-r |-ri| 0 1 1
7 -w |-oi|-r |-ri| 1 1 1
"""
