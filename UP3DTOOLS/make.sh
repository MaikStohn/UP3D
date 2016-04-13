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
    -o upload.exe ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upload.c \
    -lusb-1.0

$STRIP upload.exe

$CC -Os -std=c99 -static \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upinfo.exe ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upinfo.c \
    -lusb-1.0

$STRIP upinfo.exe

$CC -Os -std=c99 -static \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upshell.exe ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upshell.c \
    -lusb-1.0 -lncurses

$STRIP upshell.exe

elif [[ "$OSTYPE" == "darwin"* ]]; then

# note: OSX: use homebrew, install with brew libusb-devel + libncurses-devel and compile

$CC -Os -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    `pkg-config --cflags libusb-1.0`/.. \
    -I../UP3DCOMMON/ \
    -lobjc \
    `pkg-config --libs-only-L libusb-1.0|cut -c3-`/libusb-1.0.a \
    -o upload ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upload.c

$STRIP upload

$CC -Os -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    `pkg-config --cflags libusb-1.0`/.. \
    -I../UP3DCOMMON/ \
    -lobjc \
    `pkg-config --libs-only-L libusb-1.0|cut -c3-`/libusb-1.0.a \
    -o upinfo ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upinfo.c

$STRIP upinfo

$CC -Os -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    `pkg-config --cflags libusb-1.0`/.. \
    -I../UP3DCOMMON/ \
    -lobjc \
    -lncurses \
    `pkg-config --libs-only-L libusb-1.0|cut -c3-`/libusb-1.0.a \
    -o upshell ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upshell.c

$STRIP upshell

elif [[ "$OSTYPE" == "linux-gnu" ]]; then

# note: LINUX: install libudev-dev libusb-1.0.0-dev + libncurses-dev and compile

$CC -Os -std=c99 \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upload ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upload.c \
    -lusb-1.0 -lpthread -lm

$STRIP upload

$CC -Os -std=c99 \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upinfo ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upinfo.c \
    -lusb-1.0 -lpthread -lm

$STRIP upinfo

$CC -Os -std=c99 \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upshell ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upshell.c \
    -lusb-1.0 -lpthread -lncurses -lm

$STRIP upshell

fi
