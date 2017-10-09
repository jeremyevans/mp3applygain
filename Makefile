#
# Quick 'n dirty unix Makefile
#
# Mike Oliphant (oliphant@gtk.org)
#

CC?= cc
CFLAGS+= -Wall
INSTALL_PATH= /usr/local/bin
LIBS= -lm

all: mp3applygain

mp3applygain: mp3applygain.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o mp3applygain mp3applygain.c $(LIBS)

install: mp3applygain
	cp -p mp3applygain "$(INSTALL_PATH)"

uninstall:
	rm -f "$(INSTALL_PATH)/mp3applygain"

clean: 
	rm -f mp3applygain
