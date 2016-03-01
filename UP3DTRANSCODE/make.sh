#!/bin/sh

# note: for windows get MSYS2, install gcc for mingw using pacman and compile using the mingw shell

gcc -std=c99 -O3 -o up3dtranscode hoststepper.c hostplanner.c gcodeparser.c up3ddata.c umcwriter.c up3dtranscode.c -lm

if [[ "$OSTYPE" == "msys" ]]; then
strip up3dtranscode.exe
else
strip up3dtranscode
fi
