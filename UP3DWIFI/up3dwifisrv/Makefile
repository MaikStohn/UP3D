CC ?= gcc
RM ?= rm -rf

CFLAGS += -Os -std=c99 -D_BSD_SOURCE
CFLAGS += -I../../UP3DCOMMON/
CFLAGS += $(shell pkg-config --cflags libusb-1.0)/..
LDFLAGS += $(shell pkg-config --libs-only-other --libs-only-L libusb-1.0)
LIBS += $(shell pkg-config --libs-only-l libusb-1.0)

SRCS= ../../UP3DCOMMON/up3d.c
SRCS+=../../UP3DCOMMON/up3dcomm.c
SRCS+=../../UP3DCOMMON/up3ddata.c
SRCS+=up3dwifisrv.c

OBJS=$(subst .c,.o,$(SRCS))

all: up3dwifi

up3dwifi: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o up3dwifisrv $(OBJS) $(LIBS)

clean:
	$(RM) $(OBJS)
	$(RM) up3dwifisrv

dist-clean: clean
