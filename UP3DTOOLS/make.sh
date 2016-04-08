#!/bin/bash

if [[ "$OSTYPE" == "msys" ]]; then

# note: windows: use msys2, install with pacman mingw_libusb + mingw_libncurses and compile using mingw_shell, run in normal windows

gcc -O3 -std=c99 -static \
    -o upload up3dcomm.c up3d.c up3ddata.c upload.c \
    -lusb-1.0

strip upload.exe

gcc -O3 -std=c99 -static \
    -o upshell up3dcomm.c up3d.c up3ddata.c upshell.c \
    -lusb-1.0 -lncurses

strip upshell.exe

elif [[ "$OSTYPE" == "darwin"* ]]; then

# note: OSX: use homebrew, install with brew libusb-devel + libncurses-devel and compile

gcc -O3 -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    -I /usr/local/Cellar/libusb/1.0.20/include \
    -lobjc \
    /usr/local/Cellar/libusb/1.0.20/lib/libusb-1.0.a  \
    -o upload up3dcomm.c up3d.c up3ddata.c upload.c

strip upload

gcc -O3 -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    -I /usr/local/Cellar/libusb/1.0.20/include \
    -lobjc \
    -lncurses \
    /usr/local/Cellar/libusb/1.0.20/lib/libusb-1.0.a  \
    -o upshell up3dcomm.c up3d.c up3ddata.c upshell.c

strip upshell

elif [[ "$OSTYPE" == "linux-gnu" ]]; then

# note: LINUX: install libudev-dev libusb-1.0.0-dev + libncurses-dev and compile

gcc -O3 -std=c99 \
    -D_BSD_SOURCE \
    -o upload up3dcomm.c up3d.c up3ddata.c upload.c \
    -lusb-1.0 -lpthread

strip upload

gcc -O3 -std=c99 \
    -D_BSD_SOURCE \
    -o upshell up3dcomm.c up3d.c up3ddata.c upshell.c \
    -lusb-1.0 -lpthread -lncurses

strip upshell

fi