#!/bin/sh

gcc -O3 -o parse parse.c
strip parse

gcc -O3 -o convg convg.cpp
strip convg