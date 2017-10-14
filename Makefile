#
# Quick 'n dirty unix Makefile
#
# Mike Oliphant (oliphant@gtk.org)
#

CC?= cc
CFLAGS+= -Wall
INSTALL_PATH= /usr/local/bin
INSTALL_MAN_PATH= /usr/local/man/man1
LIBS= -lm
VERSION = 1.0.0

all: mp3applygain

mp3applygain: mp3applygain.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o mp3applygain mp3applygain.c $(LIBS)

install: mp3applygain
	cp -p mp3applygain "$(INSTALL_PATH)"
	cp -p mp3applygain.1 "$(INSTALL_MAN_PATH)"

uninstall:
	rm -f "$(INSTALL_PATH)/mp3applygain" "$(INSTALL_MAN_PATH)/mp3applygain.1"

clean: 
	rm -f mp3applygain mp3applygain-*.tar.gz

dist: clean
	cd .. && tar -s '/mp3applygain/mp3applygain-${VERSION}/' \
		-zcf mp3applygain-${VERSION}.tar.gz \
		mp3applygain/{README,lgpl.txt,mp3applygain.{c,1},Makefile} && \
		mv mp3applygain-${VERSION}.tar.gz mp3applygain
