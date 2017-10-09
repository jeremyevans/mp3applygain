/*
 *  mp3gain.c - analyzes mp3 files, determines the perceived volume, 
 *      and adjusts the volume of the mp3 accordingly
 *
 *  Copyright (C) 2001-2009 Glen Sawyer
 *  AAC support (C) 2004-2009 David Lasker, Altos Design, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  coding by Glen Sawyer (mp3gain@hotmail.com) 735 W 255 N, Orem, UT 84057-4505 USA
 *    -- go ahead and laugh at me for my lousy coding skillz, I can take it :)
 *       Just do me a favor and let me know how you improve the code.
 *       Thanks.
 *
 *  Unix-ification by Stefan Partheymüller
 *  (other people have made Unix-compatible alterations-- I just ended up using
 *   Stefan's because it involved the least re-work)
 *
 *  DLL-ification by John Zitterkopf (zitt@hotmail.com)
 *
 *  Additional tweaks by Artur Polaczynski, Mark Armbrust, and others
 */


/*
 *  General warning: I coded this in several stages over the course of several
 *  months. During that time, I changed my mind about my coding style and
 *  naming conventions many, many times. So there's not a lot of consistency
 *  in the code. Sorry about that. I may clean it up some day, but by the time
 *  I would be getting around to it, I'm sure that the more clever programmers
 *  out there will have come up with superior versions already...
 *
 *  So have fun dissecting.
 */

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HEADERSIZE 4
#define CRC16_POLYNOMIAL 0x8005
#define BUFFERSIZE 3000000
#define WRITEBUFFERSIZE 100000

typedef struct {
	unsigned long fileposition;
	unsigned char val[2];
} wbuffer;

wbuffer writebuffer[WRITEBUFFERSIZE];

unsigned long writebuffercnt;

unsigned char buffer[BUFFERSIZE];

int BadLayer = 0;
int LayerSet = 0;

long inbuffer;
unsigned long bitidx;
unsigned char *wrdpntr;
unsigned char *curframe;

char *filename;
FILE *inf;

unsigned long filepos;

static const double bitrate[4][16] = {
	{ 1,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, 1 },
	{ 1,  1,  1,  1,  1,  1,  1,  1,   1,   1,   1,   1,   1,   1,   1, 1 },
	{ 1,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, 1 },
	{ 1, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 1 }
};
static const double frequency[4][4] = {
	{ 11.025, 12,  8,  1 },
	{      1,  1,  1,  1 },
	{  22.05, 24, 16,  1 },
	{   44.1, 48, 32,  1 }
};

long arrbytesinframe[16];

/* instead of writing each byte change, I buffer them up */
static void flushWriteBuff() {
	unsigned long i;
	for (i = 0; i < writebuffercnt; i++) {
		fseek(inf,writebuffer[i].fileposition,SEEK_SET);
		fwrite(writebuffer[i].val,1,2,inf);
	}
	writebuffercnt = 0;
};

static void addWriteBuff(unsigned long pos, unsigned char *vals) {
	if (writebuffercnt >= WRITEBUFFERSIZE) {
		flushWriteBuff();
		fseek(inf,filepos,SEEK_SET);
	}
	writebuffer[writebuffercnt].fileposition = pos;
	writebuffer[writebuffercnt].val[0] = *vals;
	writebuffer[writebuffercnt].val[1] = vals[1];
	writebuffercnt++;
	
};

/* fill the mp3 buffer */
static unsigned long fillBuffer(long savelastbytes) {
	unsigned long i;
	unsigned long skip;
    unsigned long skipbuf;

	skip = 0;
	if (savelastbytes < 0) {
		skip = -savelastbytes;
		savelastbytes = 0;
	}

	if (savelastbytes != 0) /* save some of the bytes at the end of the buffer */
		memmove((void*)buffer,(const void*)(buffer+inbuffer-savelastbytes),savelastbytes);
	
	while (skip > 0) { /* skip some bytes from the input file */
        skipbuf = skip > BUFFERSIZE ? BUFFERSIZE : skip;

		i = (unsigned long)fread(buffer,1,skipbuf,inf);
        if (i != skipbuf)
            return 0;

		filepos += i;
        skip -= skipbuf;
	}
	i = (unsigned long)fread(buffer+savelastbytes,1,BUFFERSIZE-savelastbytes,inf);

	filepos = filepos + i;
	inbuffer = i + savelastbytes;
	return i;
}

static const unsigned char maskLeft8bits[8] = {
	0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };

static const unsigned char maskRight8bits[8] = {
	0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };

static void set8Bits(unsigned short val) {
	val <<= (8 - bitidx);
	wrdpntr[0] &= maskLeft8bits[bitidx];
	wrdpntr[0] |= (val  >> 8);
	wrdpntr[1] &= maskRight8bits[bitidx];
	wrdpntr[1] |= (val  & 0xFF);
	
	addWriteBuff(filepos-(inbuffer-(wrdpntr-buffer)),wrdpntr);
}

static void skipBits(int nbits) {

	bitidx += nbits;
	wrdpntr += (bitidx >> 3);
	bitidx &= 7;
}

static unsigned char peek8Bits() {
	unsigned short rval;

	rval = wrdpntr[0];
	rval <<= 8;
	rval |= wrdpntr[1];
	rval >>= (8 - bitidx);

	return (rval & 0xFF);
}

static unsigned long skipID3v2() {
/*
 *  An ID3v2 tag can be detected with the following pattern:
 *    $49 44 33 yy yy xx zz zz zz zz
 *  Where yy is less than $FF, xx is the 'flags' byte and zz is less than
 *  $80.
 */
	unsigned long ok;
	unsigned long ID3Size;

	ok = 1;

	if (wrdpntr[0] == 'I' && wrdpntr[1] == 'D' && wrdpntr[2] == '3' 
		&& wrdpntr[3] < 0xFF && wrdpntr[4] < 0xFF) {

		ID3Size = (long)(wrdpntr[9]) | ((long)(wrdpntr[8]) << 7) |
			((long)(wrdpntr[7]) << 14) | ((long)(wrdpntr[6]) << 21);

		ID3Size += 10;

		wrdpntr = wrdpntr + ID3Size;

		if ((wrdpntr+HEADERSIZE-buffer) > inbuffer) {
			ok = fillBuffer(inbuffer-(wrdpntr-buffer));
			wrdpntr = buffer;
		}
	}

	return ok;
}

static unsigned long frameSearch(int startup) {
	unsigned long ok;
	int done;
    static int startfreq;
    static int startmpegver;
	long tempmpegver;
	double bitbase;
	int i;

	done = 0;
	ok = 1;

	if ((wrdpntr+HEADERSIZE-buffer) > inbuffer) {
		ok = fillBuffer(inbuffer-(wrdpntr-buffer));
		wrdpntr = buffer;
		if (!ok) done = 1;
	}

	while (!done) {
		
		done = 1;

		if ((wrdpntr[0] & 0xFF) != 0xFF)
			done = 0;       /* first 8 bits must be '1' */
		else if ((wrdpntr[1] & 0xE0) != 0xE0)
			done = 0;       /* next 3 bits are also '1' */
		else if ((wrdpntr[1] & 0x18) == 0x08)
			done = 0;       /* invalid MPEG version */
		else if ((wrdpntr[2] & 0xF0) == 0xF0)
			done = 0;       /* bad bitrate */
		else if ((wrdpntr[2] & 0xF0) == 0x00)
			done = 0;       /* we'll just completely ignore "free format" bitrates */
		else if ((wrdpntr[2] & 0x0C) == 0x0C)
			done = 0;       /* bad sample frequency */
		else if ((wrdpntr[1] & 0x06) != 0x02) { /* not Layer III */
			if (!LayerSet) {
				switch (wrdpntr[1] & 0x06) {
					case 0x06:
						BadLayer = !0;
						fprintf(stderr, "%s is an MPEG Layer I file, not a layer III file\n", filename);
						return 0;
					case 0x04:
						BadLayer = !0;
						fprintf(stderr, "%s is an MPEG Layer II file, not a layer III file\n", filename);
						return 0;
				}
			}
			done = 0; /* probably just corrupt data, keep trying */
		}
        else if (startup) {
            startmpegver = wrdpntr[1] & 0x18;
            startfreq = wrdpntr[2] & 0x0C;
			tempmpegver = startmpegver >> 3;
			if (tempmpegver == 3)
				bitbase = 1152.0;
			else
				bitbase = 576.0;

			for (i = 0; i < 16; i++)
				arrbytesinframe[i] = (long)(floor(floor((bitbase*bitrate[tempmpegver][i])/frequency[tempmpegver][startfreq >> 2]) / 8.0));

        }
        else { /* !startup -- if MPEG version or frequency is different, 
                              then probably not correctly synched yet */
            if ((wrdpntr[1] & 0x18) != startmpegver)
                done = 0;
            else if ((wrdpntr[2] & 0x0C) != startfreq)
                done = 0;
            else if ((wrdpntr[2] & 0xF0) == 0) /* bitrate is "free format" probably just 
                                                  corrupt data if we've already found 
                                                  valid frames */
                done = 0;
        }

		if (!done) wrdpntr++;

		if ((wrdpntr+HEADERSIZE-buffer) > inbuffer) {
			ok = fillBuffer(inbuffer-(wrdpntr-buffer));
			wrdpntr = buffer;
			if (!ok) done = 1;
		}
	}

	if (ok) {
		if (inbuffer - (wrdpntr-buffer) < (arrbytesinframe[(wrdpntr[2] >> 4) & 0x0F] + ((wrdpntr[2] >> 1) & 0x01))) {
			ok = fillBuffer(inbuffer-(wrdpntr-buffer));
			wrdpntr = buffer;
		}
		bitidx = 0;
		curframe = wrdpntr;
	}
	return ok;
}

static int crcUpdate(int value, int crc)
{
    int i;
    value <<= 8;
    for (i = 0; i < 8; i++) {
		value <<= 1;
		crc <<= 1;

		if (((crc ^ value) & 0x10000))
			crc ^= CRC16_POLYNOMIAL;
	}
    return crc;
}

static void crcWriteHeader(int headerlength, char *header)
{
    int crc = 0xffff; /* (jo) init crc16 for error_protection */
    int i;

    crc = crcUpdate(((unsigned char*)header)[2], crc);
    crc = crcUpdate(((unsigned char*)header)[3], crc);
    for (i = 6; i < headerlength; i++) {
	crc = crcUpdate(((unsigned char*)header)[i], crc);
    }

    header[4] = crc >> 8;
    header[5] = crc & 255;
}

int main(int argc, char **argv) {
	unsigned long ok;
	int mode;
	int crcflag;
	unsigned char *Xingcheck;
	unsigned long frame;
	int nchan;
	int ch;
	int gr;
	unsigned char gain;
	int bitridx;
	int freqidx;
	long bytesinframe;
	int sideinfo_len;
	int mpegver;
	char *outfilename;

	outfilename = NULL;
	frame = 0;
	BadLayer = 0;
	LayerSet = 0;

	double arg_db, db;
	int arg_mod, gainchange;

	if(pledge("rpath stdio wpath", NULL) != 0) {
		err(1, NULL);
	}

	if (argc != 4) {
		fprintf(stderr, "Usage: mp3applygain gain mod filename\n");
		exit(1);
	}

	arg_db = atof(argv[1]);
	arg_mod = atoi(argv[2]);
	filename = argv[3];

	db = arg_db / (5.0 * log10(2.0));

	if (!(fabs(db) - (double)((int)(fabs(db))) < 0.5))
		db += (db < 0 ? -1 : 1);
	gainchange = arg_mod + (int)(db);

	if (gainchange == 0)
		return 0;

	inf = fopen(filename,"r+b");
	if (inf == NULL) {
		err(1, "%s", filename);
	}
	if(pledge("stdio", NULL) != 0) {
		err(1, NULL);
	}

	writebuffercnt = 0;
	inbuffer = 0;
	filepos = 0;
	bitidx = 0;
	ok = fillBuffer(0);
	if (ok) {

		wrdpntr = buffer;

		ok = skipID3v2();

		ok = frameSearch(!0);
		if (!ok) {
            if (!BadLayer)
				fprintf(stderr, "Can't find any valid MP3 frames in file %s\n", filename);
		}
		else {
			LayerSet = 1; /* We've found at least one valid layer 3 frame.
						   * Assume any later layer 1 or 2 frames are just
						   * bitstream corruption
						   */
			mode = (curframe[3] >> 6) & 3;

			if ((curframe[1] & 0x08) == 0x08) /* MPEG 1 */
				sideinfo_len = (mode == 3) ? 4 + 17 : 4 + 32;
			else                /* MPEG 2 */
				sideinfo_len = (mode == 3) ? 4 + 9 : 4 + 17;

			if (!(curframe[1] & 0x01))
				sideinfo_len += 2;

			Xingcheck = curframe + sideinfo_len;

			//LAME CBR files have "Info" tags, not "Xing" tags
			if ((Xingcheck[0] == 'X' && Xingcheck[1] == 'i' && Xingcheck[2] == 'n' && Xingcheck[3] == 'g') ||
					(Xingcheck[0] == 'I' && Xingcheck[1] == 'n' && Xingcheck[2] == 'f' && Xingcheck[3] == 'o')) {
				bitridx = (curframe[2] >> 4) & 0x0F;
				if (bitridx == 0) {
					fprintf(stderr, "%s is free format (not currently supported)\n", filename);
					ok = 0;
				}
				else {
					mpegver = (curframe[1] >> 3) & 0x03;
					freqidx = (curframe[2] >> 2) & 0x03;

					bytesinframe = arrbytesinframe[bitridx] + ((curframe[2] >> 1) & 0x01);

					wrdpntr = curframe + bytesinframe;

					ok = frameSearch(0);
				}
			}
			
			frame = 1;
		} /* if (!ok) else */
		
		while (ok) {
			bitridx = (curframe[2] >> 4) & 0x0F;
			if (bitridx == 0) {
				fprintf(stderr, "%s is free format (not currently supported)\n", filename);
				ok = 0;
			}
			if (ok) {
				mpegver = (curframe[1] >> 3) & 0x03;
				crcflag = curframe[1] & 0x01;
				freqidx = (curframe[2] >> 2) & 0x03;

				bytesinframe = arrbytesinframe[bitridx] + ((curframe[2] >> 1) & 0x01);
				mode = (curframe[3] >> 6) & 0x03;
				nchan = (mode == 3) ? 1 : 2;

				if (!crcflag) /* we DO have a crc field */
					wrdpntr = curframe + 6; /* 4-byte header, 2-byte CRC */
				else
					wrdpntr = curframe + 4; /* 4-byte header */

				bitidx = 0;

				if (mpegver == 3) { /* 9 bit main_data_begin */
					wrdpntr++;
					bitidx = 1;

					if (mode == 3)
						skipBits(5); /* private bits */
					else
						skipBits(3); /* private bits */

					skipBits(nchan*4); /* scfsi[ch][band] */
					for (gr = 0; gr < 2; gr++)
						for (ch = 0; ch < nchan; ch++) {
							skipBits(21);
							gain = peek8Bits();
                                if (gain != 0) {
                                    if ((int)(gain) + gainchange > 255)
                                        gain = 255;
                                    else if ((int)gain + gainchange < 0)
                                        gain = 0;
                                    else
                                        gain += (unsigned char)(gainchange);
                                }
							set8Bits(gain);
							skipBits(38);
						}
						if (!crcflag) {
							if (nchan == 1)
								crcWriteHeader(23,(char*)curframe);
							else
								crcWriteHeader(38,(char*)curframe);
							/* WRITETOFILE */
							addWriteBuff(filepos-(inbuffer-(curframe+4-buffer)),curframe+4);
						}
				}
				else { /* mpegver != 3 */
					wrdpntr++; /* 8 bit main_data_begin */

					if (mode == 3)
						skipBits(1);
					else
						skipBits(2);

					/* only one granule, so no loop */
					for (ch = 0; ch < nchan; ch++) {
						skipBits(21);
						gain = peek8Bits();
                            if (gain != 0) {
                                if ((int)(gain) + gainchange > 255)
                                    gain = 255;
                                else if ((int)gain + gainchange < 0)
                                    gain = 0;
                                else
                                    gain += (unsigned char)(gainchange);
                            }
						set8Bits(gain);
						skipBits(42);
					}
					if (!crcflag) {
						if (nchan == 1)
							crcWriteHeader(15,(char*)curframe);
						else
							crcWriteHeader(23,(char*)curframe);
						/* WRITETOFILE */
						addWriteBuff(filepos-(inbuffer-(curframe+4-buffer)),curframe+4);
					}

				}
				wrdpntr = curframe+bytesinframe;
				ok = frameSearch(0);
			}
		}
	}

	fflush(stderr);
	fflush(stdout);
	flushWriteBuff();
	fclose(inf);

	return 0;
}
