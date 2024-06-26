TARGET_LIB = libpspgraphics.a
OBJS = graphics.o font.o sp_chars.o

INCDIR = 
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
#-fno-jump-tables
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
