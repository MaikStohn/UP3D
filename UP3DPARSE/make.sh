#!/bin/bash

gcc -std=c99 -Os -o parse parse.c
gcc -std=c99 -Os -o convg convg.c

if [[ "$OSTYPE" == "msys" ]]; then
strip parse.exe
strip convg.exe
else
strip parse
strip convg
fi
