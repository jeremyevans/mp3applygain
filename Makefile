#
# Quick 'n dirty unix Makefile
#
# Mike Oliphant (oliphant@gtk.org)
#

CC?= cc
CFLAGS+= -Wall
INSTALL_PATH= /usr/local/bin
LIBS= -lm
VERSION = 1.0.0

all: mp3applygain

mp3applygain: mp3applygain.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o mp3applygain mp3applygain.c $(LIBS)

install: mp3applygain
	cp -p mp3applygain "$(INSTALL_PATH)"

uninstall:
	rm -f "$(INSTALL_PATH)/mp3applygain"

clean: 
	rm -f mp3applygain

dist: clean
	cd .. && tar -s '/mp3applygain/mp3applygain-${VERSION}/' \
		-zcf mp3applygain-${VERSION}.tar.gz \
		mp3applygain/{README,lgpl.txt,mp3applygain.c,Makefile} && \
		mv mp3applygain-${VERSION}.tar.gz mp3applygain
