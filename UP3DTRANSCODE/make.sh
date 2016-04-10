#!/bin/bash

# note: for windows get MSYS2, install gcc for mingw using pacman and compile using the mingw shell

gcc -m32 -std=c99 -Os \
    -I../UP3DCOMMON \
    -o up3dtranscode hoststepper.c hostplanner.c gcodeparser.c ../UP3DCOMMON/up3ddata.c umcwriter.c up3dtranscode.c -lm

if [[ "$OSTYPE" == "msys" ]]; then
strip up3dtranscode.exe
else
strip up3dtranscode
fi
