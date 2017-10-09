/*
 *  ReplayGainAnalysis Heder - DLL for Glen Sawyer's MP3GAIN.C source
 *  Copyright (C) 2002 John Zitterkopf (zitt@bigfoot.com) 
 *                     (http://www.zittware.com)
 *
 *  These comments must remain intact in all copies of the source code.
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
 *  This header for VC++5.0 by John Zitterkopf (zitt@bigfoot.com)
 *    -- blame him for nothing. This work evolves as needed.
 *
 *  V0.0 - jzitt9/4/2002
 *  * header needed to eliminate 'changeGain' undefined compiler warning
 */

#ifndef MP3GAIN_H
#define MP3GAIN_H

#define MP3GAIN_VERSION "1.5.2"
/* jzitt: moved from mp3gain.c */

#define M3G_ERR_CANT_MODIFY_FILE -1
#define M3G_ERR_CANT_MAKE_TMP -2
#define M3G_ERR_NOT_ENOUGH_TMP_SPACE -3
#define M3G_ERR_RENAME_TMP -4
#define M3G_ERR_FILEOPEN   -5
#define M3G_ERR_READ       -6
#define M3G_ERR_WRITE      -7
#define M3G_ERR_TAGFORMAT  -8

#include "rg_error.h"


typedef enum {
    storeTime,
    setStoredTime
} timeAction;

struct MP3GainTagInfo {
    int haveTrackGain;
    int haveTrackPeak;
    int haveAlbumGain;
    int haveAlbumPeak;
    int haveUndo;
    int haveMinMaxGain;
	int haveAlbumMinMaxGain;
    double trackGain;
    double trackPeak;
    double albumGain;
    double albumPeak;
    int undoLeft;
    int undoRight;
    int undoWrap;
    /* undoLeft and undoRight will be the same 95% of the time.
       mp3gain DOES have a command-line switch to adjust the gain on just
       one channel, though.
       The "undoWrap" field indicates whether or not "wrapping" was turned on
       when the mp3 was adjusted
     */
    unsigned char minGain;
    unsigned char maxGain;
	unsigned char albumMinGain;
	unsigned char albumMaxGain;
    /* minGain and maxGain are the current minimum and maximum values of
       the "global gain" fields in the frames of the mp3 file
     */
	int dirty; /* flag if data changes after loaded from file */
  int recalc; /* Used to signal if recalculation is required */
};

void passError(MMRESULT lerrnum, int numStrings, ...);

#endif
