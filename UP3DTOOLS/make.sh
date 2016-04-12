#!/bin/bash

if [[ "$OSTYPE" == "msys" ]]; then

# note: windows: use msys2, install with pacman mingw_libusb + mingw_libncurses and compile using mingw_shell, run in normal windows

gcc -Os -std=c99 -static \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upload ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upload.c \
    -lusb-1.0

strip upload.exe

gcc -Os -std=c99 -static \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upinfo ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upinfo.c \
    -lusb-1.0

strip upinfo.exe

gcc -Os -std=c99 -static \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upshell ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upshell.c \
    -lusb-1.0 -lncurses

strip upshell.exe

elif [[ "$OSTYPE" == "darwin"* ]]; then

# note: OSX: use homebrew, install with brew libusb-devel + libncurses-devel and compile

gcc -Os -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    `pkg-config --cflags libusb-1.0`/.. \
    -I../UP3DCOMMON/ \
    -lobjc \
    `pkg-config --libs-only-L libusb-1.0|cut -c3-`/libusb-1.0.a \
    -o upload ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upload.c

strip upload

gcc -Os -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    `pkg-config --cflags libusb-1.0`/.. \
    -I../UP3DCOMMON/ \
    -lobjc \
    `pkg-config --libs-only-L libusb-1.0|cut -c3-`/libusb-1.0.a \
    -o upinfo ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upinfo.c

strip upinfo

gcc -Os -Wall \
    -framework IOKit \
    -framework CoreFoundation \
    `pkg-config --cflags libusb-1.0`/.. \
    -I/usr/local/opt/ncurses/include \
    -I../UP3DCOMMON/ \
    -lobjc \
    /usr/local/opt/ncurses/lib/libncurses.a \
    `pkg-config --libs-only-L libusb-1.0|cut -c3-`/libusb-1.0.a \
    -o upshell ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upshell.c

strip upshell

elif [[ "$OSTYPE" == "linux-gnu" ]]; then

# note: LINUX: install libudev-dev libusb-1.0.0-dev + libncurses-dev and compile

gcc -Os -std=c99 \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upload ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upload.c \
    -lusb-1.0 -lpthread -lm

strip upload

gcc -Os -std=c99 \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upinfo ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upinfo.c \
    -lusb-1.0 -lpthread -lm

strip upinfo

gcc -Os -std=c99 \
    -D_BSD_SOURCE \
    -I../UP3DCOMMON/ \
    -o upshell ../UP3DCOMMON/up3dcomm.c ../UP3DCOMMON/up3d.c ../UP3DCOMMON/up3ddata.c upshell.c \
    -lusb-1.0 -lpthread -lncurses -lm

strip upshell

fi