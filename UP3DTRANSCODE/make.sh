#!/bin/sh

gcc -O3 -o transcode transcode.c
strip transcode

./transcode TESTDATA/im.gcode TESTDATA/im.umc
