#!/bin/sh

gcc -O3 -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    -I /usr/local/Cellar/libusb/1.0.19/include/libusb-1.0 \
    -lobjc \
    /usr/local/Cellar/libusb/1.0.19/lib/libusb-1.0.a  \
    -o upload up3dcomm.c up3d.c up3ddata.c upload.c

strip upload

gcc -O3 -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    -I /usr/local/Cellar/libusb/1.0.19/include/libusb-1.0 \
    -lobjc \
    -lncurses \
    /usr/local/Cellar/libusb/1.0.19/lib/libusb-1.0.a  \
    -o upshell up3dcomm.c up3d.c up3ddata.c upshell.c

strip upshell

