#!/bin/bash

gcc -std=c99 -O3 -o parse parse.c
gcc -std=c99 -O3 -o convg convg.c

if [[ "$OSTYPE" == "msys" ]]; then
strip parse.exe
strip convg.exe
else
strip parse
strip convg
fi
