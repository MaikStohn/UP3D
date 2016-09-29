#!/bin/bash
if [[ $# -ne 3 ]]; then
  echo  usage: $0 \[mini\|classic\|plus\|box\|Cetus\] input.gcode nozzle_height
  echo         transcodes the input.gcode file
  echo         uploads it to the printer and captures the position data
  echo         finally opens the capture file in textedit
  exit 1
fi

up3dtranscode $1 $2 ${2%.*}.$1 $3

if [[ $? -ne 0 ]] ; then
    exit 1
fi

up3dload ${2%.*}.$1

if [[ $? -ne 0 ]] ; then
    exit 1
fi

sleep 3

./up3dcapture >${2%.*}.$1.capture
open -e ${2%.*}.$1.capture
