CC ?= gcc
RM ?= rm -rf

CFLAGS += -Os -std=c99 -D_BSD_SOURCE
# CFLAGS += -D_DEBUG_IN_OUT_
CFLAGS += -I../UP3DCOMMON/
CFLAGS += -Wall $(shell pkg-config --cflags libusb-1.0)/..

LDFLAGS += $(shell pkg-config --libs-only-other --libs-only-L libusb-1.0)
LIBS += $(shell pkg-config --libs-only-l libusb-1.0) -lpthread -lm

SRC_COMMON= ../UP3DCOMMON/up3d.c 
SRC_COMMON+=../UP3DCOMMON/up3dcomm.c 
SRC_COMMON+=../UP3DCOMMON/up3ddata.c 

SRC_UPLOAD=upload.c $(SRC_COMMON)
OBJ_UPLOAD=$(subst .c,.o,$(SRC_UPLOAD))

SRC_SHELL=upshell.c $(SRC_COMMON)
OBJ_SHELL=$(subst .c,.o,$(SRC_SHELL))

all: up3dload up3dshell

up3dload: $(OBJ_UPLOAD)
	$(CC) $(CFLAGS) $(LDFLAGS) -o up3dload $(OBJ_UPLOAD) $(LIBS)

up3dshell: $(OBJ_SHELL)
	$(CC) $(CFLAGS) $(LDFLAGS) -o up3dshell $(OBJ_SHELL) $(LIBS) -lncurses

clean:
	$(RM) $(OBJ_UPLOAD)
	$(RM) $(OBJ_SHELL)
	$(RM) up3dload
	$(RM) up3dshell

dist-clean: clean
