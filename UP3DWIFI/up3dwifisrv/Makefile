CC ?= gcc
RM ?= rm -rf

CFLAGS += -Os -std=c99 -D_BSD_SOURCE
CFLAGS += $(shell pkg-config --cflags libusb-1.0)
LDFLAGS += $(shell pkg-config --libs-only-other --libs-only-L libusb-1.0)
LIBS += $(shell pkg-config --libs-only-l libusb-1.0)

SRCS= up3d.c
SRCS+=up3dcomm.c
SRCS+=up3ddata.c
SRCS+=up3dwifisrv.c

OBJS=$(subst .c,.o,$(SRCS))

all: up3dwifi

up3dwifi: $(OBJS)
	echo MAIK
	echo $(shell pkg-config --cflags libusb-1.0)
	echo MAIK
	$(CC) $(CFLAGS) $(LDFLAGS) -o up3dwifisrv $(OBJS) $(LIBS)

clean:
	$(RM) $(OBJS)

dist-clean: clean
