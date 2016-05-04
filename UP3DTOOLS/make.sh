#!/bin/bash

if [ -z "$CC" ]; then
    CC=gcc
fi

if [ -z "$STRIP" ]; then
    STRIP=strip
fi


if [[ "$OSTYPE" == "msys" ]]; then

# note: windows: use msys2, install with pacman mingw_libusb + mingw_libncurses and compile using mingw_shell, run in normal windows

$CC -Os -std=c99 -static \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o up3dload.exe ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upload.c \
    -lusb-1.0

$STRIP up3dload.exe

$CC -Os -std=c99 -static \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o up3dinfo.exe ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upinfo.c \
    -lusb-1.0

$STRIP up3dinfo.exe

$CC -Os -std=c99 -static \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o up3dshell.exe ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upshell.c \
    -lusb-1.0 -lncurses

$STRIP up3dshell.exe

elif [[ "$OSTYPE" == "darwin"* ]]; then

# note: OSX: use homebrew, install with brew libusb-devel + libncurses-devel and compile

$CC -Os -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    `pkg-config --cflags libusb-1.0`/.. \
    -I../UP3DCOMMON/ \
    -lobjc \
    `pkg-config --libs-only-L libusb-1.0|cut -c3-`/libusb-1.0.a \
    -o up3dload ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upload.c

$STRIP up3dload

$CC -Os -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    `pkg-config --cflags libusb-1.0`/.. \
    -I../UP3DCOMMON/ \
    -lobjc \
    `pkg-config --libs-only-L libusb-1.0|cut -c3-`/libusb-1.0.a \
    -o up3dinfo ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upinfo.c

$STRIP up3dinfo

$CC -Os -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    `pkg-config --cflags libusb-1.0`/.. \
    -I../UP3DCOMMON/ \
    -lobjc \
    -lncurses \
    `pkg-config --libs-only-L libusb-1.0|cut -c3-`/libusb-1.0.a \
    -o up3dshell ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upshell.c

$STRIP up3dshell

elif [[ "$OSTYPE" == "linux-gnu" ]]; then

# note: LINUX: install libudev-dev libusb-1.0.0-dev + libncurses-dev and compile

$CC -Os -std=c99 \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o up3dload ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upload.c \
    -lusb-1.0 -lpthread -lm

$STRIP up3dload

$CC -Os -std=c99 \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o up3dinfo ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upinfo.c \
    -lusb-1.0 -lpthread -lm

$STRIP up3dinfo

$CC -Os -std=c99 \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o up3dshell ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upshell.c \
    -lusb-1.0 -lpthread -lncurses -lm

$STRIP up3dshell

fi
