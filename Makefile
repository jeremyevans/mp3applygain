#
# Quick 'n dirty unix Makefile
#
# Mike Oliphant (oliphant@gtk.org)
#

CC?= cc
CFLAGS+= -Wall
INSTALL_PATH= /usr/local/bin
LIBS= -lm

OBJS=	mp3gain.o gain_analysis.o rg_error.o \
	mpglibDBL/common.o mpglibDBL/dct64_i386.o \
	mpglibDBL/decode_i386.o mpglibDBL/interface.o \
	mpglibDBL/layer3.o mpglibDBL/tabinit.o

all: mp3gain

mp3gain: $(OBJS)
	$(CC) $(LDFLAGS) -o mp3gain $(OBJS) $(LIBS)

install: mp3gain
	cp -p mp3gain "$(INSTALL_PATH)"

uninstall:
	rm -f "$(INSTALL_PATH)/mp3gain"

clean: 
	rm -f mp3gain $(OBJS)
