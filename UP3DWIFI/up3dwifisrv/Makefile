CC ?= gcc
RM ?= rm -rf

CFLAGS += -Os -std=c99 -D_BSD_SOURCE
#CFLAGS += -D_DEBUG_IN_OUT_
CFLAGS += -I../../UP3DCOMMON/
CFLAGS += $(shell pkg-config --cflags libusb-1.0)/..
LDFLAGS += $(shell pkg-config --libs-only-other --libs-only-L libusb-1.0)
LIBS += $(shell pkg-config --libs-only-l libusb-1.0) -lpthread -lm

SRCSSRV= ../../UP3DCOMMON/up3d.c
SRCSSRV+=../../UP3DCOMMON/up3dcomm.c
SRCSSRV+=../../UP3DCOMMON/up3ddata.c
SRCSSRV+=up3dwifisrv.c
OBJSSRV=$(subst .c,.o,$(SRCSSRV))

SRCSLGO= up3dwifioled.c
SRCSLGO+=up3dwifilogo.c
OBJSLGO=$(subst .c,.o,$(SRCSLGO))

all: up3dwifi up3dlogo

up3dwifi: $(OBJSSRV)
	$(CC) $(CFLAGS) $(LDFLAGS) -o up3dwifisrv $(OBJSSRV) $(LIBS)

up3dlogo: $(OBJSLGO)
	$(CC) $(CFLAGS) $(LDFLAGS) -o up3dwifilogo $(OBJSLGO) $(LIBS)

clean:
	$(RM) $(OBJSSRV)
	$(RM) up3dwifisrv
	$(RM) $(OBJSLGO)
	$(RM) up3dwifilogo

dist-clean: clean
