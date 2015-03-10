/*

    SEGA Zaxxon Hardware - Sound

*/

/*
 * Zaxxon Soundhardware Driver
 * Copyright Alex Judd 1997/98
 */

/***************************************************************************

Sound interface is provided by an 8255. The 8255 is a parallel peripheral
interface, used also in Scramble. It has three 8-bit outputs.
All sounds are generated by discrete circuits. Each sound is triggered by
an output pin of the 8255.

Zaxxon Sound Information: (from the schematics)
by Frank Palazzolo

There are four registers in the 8255. they are mapped to
(111x xxxx 0011 11pp) by Zaxxon.  Zaxxon writes to these
at FF3C-FF3F.

There are three modes of the 8255, but by the schematics I
can see that Zaxxon is using "Mode 0", which is very simple.

Important Note:
These are all Active-Low outputs.
A 1 De-activates the sound, while a 0 Activates/Triggers it

Port A Output:
FF3C bit7 Battleship
     bit6 Laser
     bit5 Base Missle
     bit4 Homing Missle
     bit3 Player Ship D
     bit2 Player Ship C
     bit1 Player Ship B
     bit0 Player Ship A

Port B Output:
FF3D bit7 Cannon
     bit6 N/C
     bit5 M-Exp
     bit4 S-Exp
     bit3 N/C
     bit2 N/C
     bit1 N/C
     bit0 N/C

Port C Output:
FF3E bit7 N/C
     bit6 N/C
     bit5 N/C
     bit4 N/C
     bit3 Alarm 3
     bit2 Alarm 2
     bit1 N/C
     bit0 Shot

Control Byte:
FF3F Should be written an 0x80 for Mode 0
     (Very Simple) operation of the 8255

***************************************************************************/

#include "driver.h"
#include "sound/samples.h"

#define TOTAL_SOUNDS 22
static int soundplaying[TOTAL_SOUNDS];

struct sa
{
	int channel;
	int num;
	int looped;
	int stoppable;
	int restartable;
};

static const struct sa sa[TOTAL_SOUNDS] =
{
	{  0,  0, 1, 1, 1 },	/* Line  4 - Homing Missile  (channel 1) */
	{  1,  1, 0, 1, 1 },	/* Line  5 - Base Missile */
	{  2,  2, 1, 1, 1 },	/* Line  6 - Laser (force field) (channel 1) */
	{  3,  3, 1, 1, 1 },	/* Line  7 - Battleship (end of level boss) (channel 1) */
	{ -1 },					/* Line  8 - unused */
	{ -1 },					/* Line  9 - unused */
	{ -1 },					/* Line 10 - unused */
	{ -1 },					/* Line 11 - unused */
	{  4,  4, 0, 0, 1 },	/* Line 12 - S-Exp (enemy explosion) */
	{  5,  5, 0, 0, 0 },	/* Line 13 - M-Exp (ship explosion) (channel 1) */
	{ -1 },					/* Line 14 - unused */
	{  6,  6, 0, 0, 1 },	/* Line 15 - Cannon (ship fire) */
	{  7,  7, 0, 0, 1 },	/* Line 16 - Shot (enemy fire) */
	{ -1 },					/* Line 17 - unused */
	{  8,  8, 0, 0, 1 },	/* Line 18 - Alarm 2 (target lock) */
	{  9,  9, 0, 0, 0 },	/* Line 19 - Alarm 3 (low fuel) (channel 1) */
	{ -1 },					/* Line 20 - unused */
	{ -1 },					/* Line 21 - unused */
	{ -1 },					/* Line 22 - unused */
	{ -1 },					/* Line 23 - unused */
	{ 10, 10, 1, 1, 1 },	/* background */
	{ 11, 11, 1, 1, 1 },	/* background */
};

WRITE8_HANDLER( zaxxon_sound_w )
{
	int line;
	int noise;

	switch (offset)
	{
	case 0:
		/* handle background rumble */
		switch (data & 0x0c)
		{
			case 0x04:
				soundplaying[20] = 0;
				sample_stop(sa[20].channel);
				if (soundplaying[21] == 0)
				{
					soundplaying[21] = 1;
					sample_start(sa[21].channel,sa[21].num,sa[21].looped);
				}
				sample_set_volume(sa[21].channel,0.5 + 0.157 * (data & 0x03));
				break;
			case 0x00:
			case 0x08:
				if (soundplaying[20] == 0)
				{
					soundplaying[20] = 1;
					sample_start(sa[20].channel,sa[20].num,sa[20].looped);
				}
				sample_set_volume(sa[20].channel,0.5 + 0.157 * (data & 0x03));
				soundplaying[21] = 0;
				sample_stop(sa[21].channel);
				break;
			case 0x0c:
				soundplaying[20] = 0;
				sample_stop(sa[20].channel);
				soundplaying[21] = 0;
				sample_stop(sa[21].channel);
				break;
		}

	case 1:
	case 2:
		for (line = 0;line < 8;line++)
		{
			noise = 8 * offset + line - 4;

			/* the first four sound lines are handled separately */
			if (noise >= 0)
			{
				if ((data & (1 << line)) == 0)
				{
					/* trigger sound */
					if (soundplaying[noise] == 0)
					{
						soundplaying[noise] = 1;
						if (sa[noise].channel != -1)
						{
							if (sa[noise].restartable || !sample_playing(sa[noise].channel))
								sample_start(sa[noise].channel,sa[noise].num,sa[noise].looped);
						}
					}
				}
				else
				{
					if (soundplaying[noise])
					{
						soundplaying[noise] = 0;
						if (sa[noise].channel != -1 && sa[noise].stoppable)
							sample_stop(sa[noise].channel);
					}
				}
			}
		}
	case 3:
		// control byte
		break;
	}
}
