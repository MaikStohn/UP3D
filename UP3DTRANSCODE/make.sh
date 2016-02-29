#!/bin/sh

gcc -O3 -o up3dtranscode hoststepper.cpp hostplanner.cpp gcodeparser.cpp up3ddata.cpp umcwriter.cpp up3dtranscode.cpp
strip up3dtranscode
