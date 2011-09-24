/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                           AdamemSDL.c                                  **/
/**                                                                        **/
/** This file contains the driver for the SDL support. It does not         **/
/** include the sound emulation code                                       **/
/**                                                                        **/
/** Copyright (C) Geoff Oltmans, Dale Wick 2006                            **/
/****************************************************************************/

#include "Coleco.h"
#include "Sound.h"
#include "AdamSDLSound.h"

#include "AdamemSDL.h"

#include "Bitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* Title for -help output */
char Title[]="ADAMEm SDL 1.8";

char szBitmapFile[256];           /* Next screen shot file                  */
char szSnapshotFile[256];         /* Next snapshot file                     */
int  videomode;                   /* 0=1x1  1=2x1                           */
static int makesnap=0;            /* 1 if snapshot should be written        */
static int makeshot=0;            /* Save a screenshot                      */
static int bpp;			  /* Bits per pixel of the display          */
static SDL_Surface *screen;
static SDL_Surface *scale2xSurface;

static int White,Black;		  /* White and black colour values	    */
static char fullscreen = 0;
char scale2xMode = 0;

/* These functions are used to put a pixel on the image buffer */
static void PutPixel8_0  (int offset,int c);
static void PutPixel8_1  (int offset,int c);
static void PutPixel8_2  (int offset,int c);
static void (*PutPixelProc)(int offset,int c);
#define PutPixel(P,C)   (*PutPixelProc)(P,C)

static int ReadTimer (void);

/* Key mapping */
static int KEY_LEFT,KEY_RIGHT,KEY_UP,KEY_DOWN,
   KEY_BUTTONA,KEY_BUTTONB,KEY_BUTTONC,KEY_BUTTOND;

static byte VGA_Palette[16*3];    			/* Coleco Palette                         */
static int PalBuf[16],Pal0;       			/* Palette buffer                         */
static int default_mouse_sens=200;			/* Default mouse sensitivity              */
static int keyboardmode;          			/* 0=joystick, 1=keyboard                 */
static word MouseJoyState[2];     			/* Current mouse status                   */
static word JoystickJoyState[2];  			/* Current joystick status                */
static int OldTimer=0;            			/* Last value of timer                    */
static int NewTimer=0;            			/* New value of timer                     */
static int calloptions=0;         			/* If 1, OptionsDialogue() is called      */
static int width,height;	  	  			/* width & height of the window           */

byte *DisplayBuf;                 			/* Screen buffer                          */
char szJoystickFileName[256];               /* File holding joystick information      */
char *szKeys="11411311111213213312F020";    /* Key scancodes                          */

int  mouse_sens=200;              			/* Mouse sensitivity                      */
int  keypadmode=0;                			/* 1 if keypad should be reversed         */
int  joystick=1;                  			/* Joystick support                       */
int  calibrate=0;                 			/* Set to 1 to force joystick calibration */
int  swapbuttons=0;               			/* 1=joystick, 2=keyboard, 4=mouse        */
int  expansionmode=0;                 	    /* Expansion module emulated              */
int  syncemu=1;                   			/* 0 if emulation shouldn't be synced     */
int  SaveCPU=1;                   			/* If 1, save CPU when focus is out       */


void scale2x(SDL_Surface *src, SDL_Surface *dst);

static void PutImage (void)
{
	//  	SDL_LockSurface(screen);
	//  	memcpy(screen->pixels,DisplayBuf,width*height);
	//
	//  	SDL_UnlockSurface(screen);
	if (scale2xMode == 1) {
		scale2x(screen,scale2xSurface);
		SDL_UpdateRect(scale2xSurface, 0, 0, 0, 0); // Update the entire screen.
	} else {
		SDL_UpdateRect(screen, 0, 0, 0, 0); // Update the entire screen.
	}
}


//static void PutImage (void)
//{
//  SDL_UpdateRect(screen, 0, 0, 0, 0); // Update the entire screen.
//
//}

/****************************************************************************/
/** Keyboard routines                                                      **/
/****************************************************************************/
int PausePressed=0;
static byte keybstatus[NR_KEYS];

byte keyboard_buffer[16];

static int keyboard_buffer_count=0;

static void LocalAddToKeyboardBuffer (byte ch)
{
	keyboard_buffer[keyboard_buffer_count]=ch;
	keyboard_buffer_count=(keyboard_buffer_count+1)&15;
	keyboard_buffer[keyboard_buffer_count]=0;
}

static int LocalGetKeyboardChar (void)
{
	int retval;
	keyboard_buffer_count=(keyboard_buffer_count-1)&15;
	retval=keyboard_buffer[keyboard_buffer_count];
	keyboard_buffer[keyboard_buffer_count]=0;
	return retval;
}


static const byte scan2ascii[NR_KEYS] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,	/* 00 */
	0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,	/* 10 */
	0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,	/* 20 */
	0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,	/* 30 */
	0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,	/* 40 */
	0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,	/* 50 */
	0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,	/* 60 */
	0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,	/* 70 */
	0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E, 151,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 80 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 90 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* A0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* B0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* C0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* D0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* E0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* F0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	 '0', '1', '2', '3', '4', '5', '6', '7',	/* 100 */
	 '8', '9', '.', '/', '*', '-', '+',0x0D,
	0x00, 160, 162, 161, 163, 148, 146, 147,	/* 110 */
	 150, 149, 129, 130, 131, 132, 133, 134,
	 144, 145,0x00,0x00,0x00,0x00,0x00,0x00,	/* 120 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 130 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,    							/* 140 */
};

static const byte scan2ascii_shift[NR_KEYS] =
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,	/* 00 */
	0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,	/* 10 */
	0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,  34,	/* 20 */
	0x28,0x29,0x2A,0x2B, '<', '_', '>', '?',
	 ')', '!', '@', '#', '$', '%', '^', '&',	/* 30 */
	 '*', '(',0x3A, ':',0x3C, '+',0x3E,0x3F,
	0x40, 'a', 'b', 'c', 'd', 'e', 'f', 'g',	/* 40 */
	 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',	/* 50 */
	 'x', 'y', 'z', '{', '|', '}',0x5E,0x5F,
	 '~', 'A', 'B', 'C', 'D', 'E', 'F', 'G',	/* 60 */
	 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',	/* 70 */
	 'X', 'Y', 'Z',0x7B,0x7C,0x7D,0x7E, 159,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 80 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 90 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* A0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* B0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* C0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* D0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* E0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* F0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF00 */
	 184, 185, '.', '/', '*', '-', '+',0x0D,
	0x00,0x00,0x00,0x00,0x00, 156, 154, 155,	/* FF10 */
	158, 157, 137, 138, 139, 140, 141, 142,
	152, 153,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF20 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF30 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00     						    /* FF40 */
};

static const byte scan2ascii_ctrl[NR_KEYS] =
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 00 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 10 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 20 */
	0x00,0x00,0x00,0x00,0x00,  31,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,  30,0x00,	/* 30 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,   1,   2,   3,   4,   5,   6,   7,	/* 40 */
	   8,   9,  10,  11,  12,  13,  14,  15,
	  16,  17,  18,  19,  20,  21,  22,  23,	/* 50 */
	  24,  25,  26,  27,  28,  29,  30,  31,
	0x00,   1,   2,   3,   4,   5,   6,   7,	/* 60 */
	   8,   9,  10,  11,  12,  13,  14,  15,
	  16,  17,  18,  19,  20,  21,  22,  23,	/* 70 */
	  24,  25,  26,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 80 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* 90 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* A0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* B0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* C0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* D0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* E0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* F0 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF00 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00, 164, 166, 165, 167,0x00,0x00,0x00,	/* FF10 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF20 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* FF30 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00     							/* FF40 */
};

static int AltPressed (void)
{
	//blurt
		return keybstatus[SCANCODE_LEFTALT] || keybstatus[SCANCODE_RIGHTALT];
	//blurt
}

static int CtrlPressed (void)
{
	return keybstatus[SCANCODE_LEFTCONTROL] || keybstatus[SCANCODE_RIGHTCONTROL];
}


static void keyb_handler (int code,int newstatus)
{

	printf("code = %d\n", code);
	if (code<0 || code>NR_KEYS)
		return;

	if (newstatus) newstatus=1;

	if (!newstatus)
		keybstatus[code]=0;
	else
	{
		if (!keybstatus[code])
		{
			keybstatus[code]=1;

			if (CtrlPressed() && (code==SCANCODE_F11 || code==SCANCODE_F12))
			{
				if (code==SCANCODE_F11)
					PausePressed=2;
				else
					PausePressed=1;
				SDL_WM_GrabInput(SDL_GRAB_OFF);
				SDL_ShowCursor(SDL_ENABLE);
			}
			else {
				PausePressed=0;
				SDL_WM_GrabInput(SDL_GRAB_ON);
				SDL_ShowCursor(SDL_DISABLE);
			}

			if (AltPressed() && (code==SDLK_RETURN)) {
				setFullScreen(!fullscreen);
				
			}
			
		switch (code)
		{
			case SCANCODE_INSERT:
				if (!keyboardmode || AltPressed())
					joystick=1;
			break;
			case SCANCODE_HOME:
				if (!keyboardmode || AltPressed())
					joystick=2;
			break;
			case SCANCODE_PAGEUP:
				if (!keyboardmode || AltPressed())
					joystick=3;
			break;
			case SCANCODE_REMOVE:
				if (!keyboardmode || AltPressed())
					swapbuttons^=1;
			break;
			case SCANCODE_END:
				if (!keyboardmode || AltPressed())
					swapbuttons^=2;
			break;
			case SCANCODE_PAGEDOWN:
				if (!keyboardmode || AltPressed())
					swapbuttons^=4;
			break;
			case SCANCODE_F1:
			 if (!keyboardmode || AltPressed())
			  ToggleSoundChannel (0);
			 break;
			case SCANCODE_F2:
			 if (!keyboardmode || AltPressed())
			  ToggleSoundChannel (1);
			 break;
			case SCANCODE_F3:
			 if (!keyboardmode || AltPressed())
			  ToggleSoundChannel (2);
			 break;
			case SCANCODE_F4:
			 if (!keyboardmode || AltPressed())
			  ToggleSoundChannel (3);
			 break;
			case SCANCODE_F5:
			 if (!keyboardmode || AltPressed())
			  ToggleSound ();
			 break;
			case SCANCODE_F8:
				if (!keyboardmode || AltPressed())
					makeshot=1;
			break;
			case SCANCODE_F7:
				if (!keyboardmode || AltPressed())
					makesnap=1;
			break;
			case SCANCODE_F11:
			 if (!PausePressed && !AltPressed()) DecreaseSoundVolume ();
			 break;
			case SCANCODE_F12:
			 if (!PausePressed && !AltPressed()) IncreaseSoundVolume ();
			 break;
			case SCANCODE_F10:
				Z80_Running=0;
			break;
			case SCANCODE_F9:
#ifdef DEBUG
				if (keybstatus[SCANCODE_LEFTSHIFT] || keybstatus[SCANCODE_RIGHTSHIFT])
				{
					Z80_Trace=!Z80_Trace;
					break;
				}
#endif
				if (EmuMode)
				{
					if (CtrlPressed())
						calloptions=1;
					else
						keyboardmode=(keyboardmode)? 0:1;
				}
			break;
			}
		}
		if (keyboardmode && !AltPressed())  /* Modify keyboard buffer           */
		{
			/* Check for HOME+Cursor key and Cursor key combinations */
			if (code==SCANCODE_KEYPAD5)
			{
				if (keybstatus[SCANCODE_CURSORLEFT] || keybstatus[SCANCODE_CURSORBLOCKLEFT])
					{ code=175; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORRIGHT] ||	keybstatus[SCANCODE_CURSORBLOCKRIGHT])
					{ code=173; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORUP] || keybstatus[SCANCODE_CURSORBLOCKUP])
					{ code=172; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORDOWN] || keybstatus[SCANCODE_CURSORBLOCKDOWN])
					{ code=170; goto put_char_in_buf; }
			}
			if (keybstatus[SCANCODE_KEYPAD5])
			{
				if (code==SCANCODE_CURSORLEFT)
					{ code=175; goto put_char_in_buf; }
				if (code==SCANCODE_CURSORRIGHT)
					{ code=173; goto put_char_in_buf; }
				if (code==SCANCODE_CURSORUP)
					{ code=172; goto put_char_in_buf; }
				if (code==SCANCODE_CURSORDOWN)
					{ code=170; goto put_char_in_buf; }
			}
			if (code==SCANCODE_CURSORUP || code==SCANCODE_CURSORBLOCKUP)
			{
				if (keybstatus[SCANCODE_CURSORRIGHT] || keybstatus[SCANCODE_CURSORBLOCKRIGHT])
					{ code=168; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORLEFT] || keybstatus[SCANCODE_CURSORBLOCKLEFT])
					{ code=171; goto put_char_in_buf; }
			}
			if (code==SCANCODE_CURSORRIGHT || code==SCANCODE_CURSORBLOCKRIGHT)
			{
				if (keybstatus[SCANCODE_CURSORUP] || keybstatus[SCANCODE_CURSORBLOCKUP])
					{ code=168; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORDOWN] || keybstatus[SCANCODE_CURSORBLOCKDOWN])
					{ code=169; goto put_char_in_buf; }
			}
			if (code==SCANCODE_CURSORDOWN || code==SCANCODE_CURSORBLOCKDOWN)
			{
				if (keybstatus[SCANCODE_CURSORRIGHT] || keybstatus[SCANCODE_CURSORBLOCKRIGHT])
					{ code=169; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORLEFT] || keybstatus[SCANCODE_CURSORBLOCKLEFT])
					{ code=170; goto put_char_in_buf; }
			}
			if (code==SCANCODE_CURSORLEFT)
			{
				if (keybstatus[SCANCODE_CURSORUP] || keybstatus[SCANCODE_CURSORBLOCKUP])
					{ code=171; goto put_char_in_buf; }
				if (keybstatus[SCANCODE_CURSORDOWN] || keybstatus[SCANCODE_CURSORBLOCKDOWN])
					{ code=170; goto put_char_in_buf; }
			}

			if (keybstatus[SCANCODE_LEFTCONTROL] || keybstatus[SCANCODE_RIGHTCONTROL])
				code=scan2ascii_ctrl[code];
			else if (keybstatus[SCANCODE_LEFTSHIFT] || keybstatus[SCANCODE_RIGHTSHIFT])
				code=scan2ascii_shift[code];
			else
				code=scan2ascii[code];
			
	put_char_in_buf:
			if (keybstatus[SCANCODE_CAPSLOCK])
			{
				if (code>='a' && code<='z') code+='A'-'a';
				else if (code>='A' && code<='Z') code+='a'-'A';
			}
			if (code) LocalAddToKeyboardBuffer (code);
		}
	}
}

/* Look for keyboard events and pass them to the keyboard handler */
static void keyboard_update (void)
{
	SDL_Event event;
	int down = 0;


	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_JOYBUTTONDOWN:
				down=1;
			/* fall through */
			case SDL_JOYBUTTONUP:
				switch(event.jbutton.button) {
					blurt
					case 0: keyb_handler(SDLK_SPACE,down); break;	/* Triangle */
					case 1: keyb_handler(SCANCODE_LEFTCONTROL,down); break;/* Circle */
					case 2: keyb_handler(SCANCODE_1,down); break;	/* Cross */
					case 3: keyb_handler(SDLK_ESCAPE,down); break;	/* Square */
					case 4: keyb_handler(SDLK_BACKSPACE,down); break;	/* L trigger */
					case 5: keyb_handler(SDLK_RETURN,down); break;	/* R trigger */
					case 6: keyb_handler(SCANCODE_CURSORDOWN,down); break;
					case 7: keyb_handler(SCANCODE_CURSORLEFT,down); break;
					case 8: keyb_handler(SCANCODE_CURSORUP,down); break;
					case 9: keyb_handler(SCANCODE_CURSORRIGHT,down); break;
					case 10: keyb_handler(SCANCODE_F9,down); break;	/* Select */
					case 11: keyb_handler(SCANCODE_F10,down); break;/* Start */
				}
			break;
			case SDL_JOYAXISMOTION:
			//TODO: add joystick axis motion handler for spinner.
			printf("axis %d: value %d\n",event.jaxis.axis,event.jaxis.value);

//			if(event.jaxis.axis==0)
//			{ // Assumed to be X
//				if(event.jaxis.value<-16000) {
//					keyb_handler(SCANCODE_CURSORLEFT,1);
//					keyb_handler(SCANCODE_CURSORRIGHT,0);
//				}
//				else
//				if(event.jaxis.value>16000)
//				{
//					keyb_handler(SCANCODE_CURSORRIGHT,1);
//					keyb_handler(SCANCODE_CURSORLEFT,0);
//				}
//				else
//				{
//					keyb_handler(SCANCODE_CURSORRIGHT,0);
//					keyb_handler(SCANCODE_CURSORLEFT,0);
//				}
//			}
//			else
//			if(event.jaxis.axis==1)
//			{
//				if(event.jaxis.value<-16000)
//				{
//					keyb_handler(SCANCODE_CURSORUP,1);
//					keyb_handler(SCANCODE_CURSORDOWN,0);
//				}
//				else
//				if(event.jaxis.value>16000)
//				{
//					keyb_handler(SCANCODE_CURSORDOWN,1);
//					keyb_handler(SCANCODE_CURSORUP,0);
//				}
//				else
//				{
//					keyb_handler(SCANCODE_CURSORDOWN,0);
//					keyb_handler(SCANCODE_CURSORUP,0);
//				}
//			}

			break;
			case SDL_KEYDOWN:
				keyb_handler(event.key.keysym.sym,1);
			break;
			case SDL_KEYUP:
				keyb_handler(event.key.keysym.sym,0);
			break;
			case SDL_QUIT :
				keyb_handler(SCANCODE_F10,1);
			break;
			case SDL_VIDEOEXPOSE :
				PutImage();
			break;
//			case SDL_VIDEORESIZE :
//				width = event.resize.w;
//				height = event.resize.h;
//				screen = SDL_SetVideoMode(width, height, bpp, SDL_SWSURFACE|SDL_VIDEORESIZE); //Create a window
//				DisplayBuf = screen->pixels;
//				PutImage();
//				break;
				
			default:
				if (Verbose) printf("Unhandled SDL Event 0x%x\n",event.type);
				fflush(stdout);
			break;
		}
	}
}

/****************************************************************************/
/** Mouse routines                                                         **/
/****************************************************************************/
static int mouse_buttons=0;       /* mouse buttons pressed                  */
static int mouse_xpos=500;        /* horizontal position (0 - 1000)         */
static int mouse_ypos=500;        /* vertical position (0 - 1000)           */
static int mouse_x=0;             /* horizontal position (-500 - 500)       */
static int mouse_y=0;             /* vertical position (-500 - 500)         */
static int got_mouse=0;           /* 1 if mouse was properly initialised    */

static void Mouse_SetJoyState ()
{
	if (mouse_buttons&1)
		MouseJoyState[0]&=0xBFFF;

	if (mouse_buttons&2)
		MouseJoyState[0]&=0xFFBF;

	if (mouse_buttons&4)
		MouseJoyState[1]&=0xBFFF;

	if (mouse_buttons&8)
		MouseJoyState[1]&=0xFFBF;

	SpinnerPosition[0]=(default_mouse_sens*mouse_x*(abs(mouse_x)+mouse_sens))/
	                   (mouse_sens*mouse_sens);
	SpinnerPosition[1]=(default_mouse_sens*mouse_y*(abs(mouse_y)+mouse_sens))/
	                   (mouse_sens*mouse_sens);
}

static void Mouse_GetPosition (void)
{
	unsigned char tmp_buttonstate;
	tmp_buttonstate = SDL_GetMouseState(&mouse_x,&mouse_y);
	mouse_buttons = 0;
	if ( tmp_buttonstate&SDL_BUTTON(1) )
	{
		mouse_buttons |= 1;
	}
	if ( tmp_buttonstate&SDL_BUTTON(2) )
	{
		mouse_buttons |= 2;
	}
	if ( tmp_buttonstate&SDL_BUTTON(3) )
	{
		mouse_buttons |= 4;
	}
	if ( tmp_buttonstate&SDL_BUTTON(4) )
	{
		mouse_buttons |= 8;
	}

}

static void Mouse_SetPosition (int x,int y)
{
	SDL_WarpMouse(x,y);
}

static void Mouse_SetHorizontalRange (int x,int y)
{
}

static void Mouse_SetVerticalRange (int x,int y)
{
}

static void Mouse_Check (void)
{
	int tmp;

	if (!got_mouse)
		return;

	Mouse_GetPosition ();
	mouse_x=mouse_xpos-500;
	mouse_y=mouse_ypos-500;
	tmp=mouse_buttons;

	if (mouse_x || mouse_y)
		Mouse_SetPosition (500,500);

	switch (expansionmode)
	{
		case 4:                         /* emulate driving module                 */
			if (mouse_buttons&7)
				MouseJoyState[0]&=0xFEFF;
			mouse_x/=4;
			mouse_y=0;
			mouse_buttons=0;
		break;
		case 5:                         /* emulate SA speed roller on both ports  */
			mouse_x/=4;
			mouse_y=mouse_x;
			mouse_buttons=0;
		break;
		case 6:                         /* emulate SA speed roller on port 1      */
			mouse_x/=4;
			mouse_y=0;
			mouse_buttons=0;
		break;
		case 7:                         /* emulate SA speed roller on port 2      */
			mouse_y=mouse_x/4;
			mouse_x=0;
			mouse_buttons=0;
		break;
	}

	if (swapbuttons&4)
		mouse_buttons=(mouse_buttons&(~3))|((mouse_buttons&1)<<1)|((mouse_buttons&2)>>1);
	Mouse_SetJoyState();
	mouse_buttons=tmp;
}

static int Mouse_Detect (void)
{
	// For SDL, assume we always have the mouse installed!
	got_mouse = 1;
	return 1;
}

static void Mouse_Init (void)
{
	if (!got_mouse)
		Mouse_SetHorizontalRange (0,1000);
	Mouse_SetVerticalRange (0,1000);
	Mouse_SetPosition (500,500);
	return;
}

/****************************************************************************/
/** Joystick routines                                                      **/
/****************************************************************************/
typedef struct _joypos_t
{
	Sint16 x;
	Sint16 y;
} joypos_t;

static int gotjoy=0;              /* 1 if joystick was properly initialised */
static joypos_t joycentre;        /* joystick centre position               */
static joypos_t joymin;           /* left-upper corner position             */
static joypos_t joymax;           /* right-lower corner position            */
//static joypos_t joy_lowmargin;    /* start of 'dead' region                 */
//static joypos_t joy_highmargin;   /* end of 'dead' region                   */
static SDL_Joystick *joy;
static int fourbuttons = 0;       /* do we have four buttons? */

static int Joy_Init (void)
{
	int numbuttons;
	int numjoysticks;

	// Check for joystick
	if(numjoysticks = SDL_NumJoysticks()>0){
		// Open joystick
		joy=SDL_JoystickOpen(0);

		if(joy)
		{
			printf("Opened Joystick 0\n");
			printf("Name: %s\n", SDL_JoystickName(0));
			printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
			numbuttons = SDL_JoystickNumButtons(joy);
			if (numbuttons>=4)
				fourbuttons = 1;
			printf("Number of Buttons: %d\n", numbuttons);
			printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joy));
		}
		else
		{
			printf("Couldn't open Joystick 0\n");
			return 0;
		}
	}

	joycentre.x=64;
	joycentre.y=64;
	gotjoy=1;
	return 1;
}

static void CalibrateJoystick (void)
{
  joymin.x=0;
  joymin.y=0;
  joymax.x=127;
  joymax.y=127;

//  FILE *joyfile = NULL;
//  Sint16 x, y;
//  Uint8 button1, button2;
//  fflush(stdout);
  if (!gotjoy)
return;
//  SDL_JoystickUpdate();
//  x = SDL_JoystickGetAxis(joy,0);
//  y = SDL_JoystickGetAxis(joy,1);
//  button1 = SDL_JoystickGetButton(joy,0);

//  if (!calibrate)
//  joyfile=fopen(szJoystickFileName,"rb");

//  if (!joyfile) {
//    fprintf (stderr,"Move joystick to top left and press enter\n");
//    fflush(stderr);
//    fflush (stdin); fgetc (stdin);
//    SDL_JoystickUpdate();
//    x = SDL_JoystickGetAxis(joy,0);
//    y = SDL_JoystickGetAxis(joy,1);
//    joymin.x = x;
//    joymin.y = y;

//    fprintf (stderr,"Move joystick to bottom right and press enter\n");
//    fflush(stderr);
//    fflush (stdin); fgetc (stdin);
//    SDL_JoystickUpdate();
//    x = SDL_JoystickGetAxis(joy,0);
//    y = SDL_JoystickGetAxis(joy,1);
//    joymax.x = x;
//    joymax.y = y;

//    joyfile=fopen(szJoystickFileName,"wb");
//    if (joyfile)
//      {
//	fwrite (&joymin,sizeof(joymin),1,joyfile);
//	fwrite (&joymax,sizeof(joymax),1,joyfile);
//	fclose (joyfile);
//      }
//  }
//  else
//  {
//    fread (&joymin,sizeof(joymin),1,joyfile);
//    fread (&joymax,sizeof(joymax),1,joyfile);
//   fclose (joyfile);
//  }
//  joy_lowmargin.x=joycentre.x-(joycentre.x-joymin.x)/8;
//  joy_lowmargin.y=joycentre.y-(joycentre.y-joymin.y)/8;
//  joy_highmargin.x=joycentre.x+(joymax.x-joycentre.x)/8;
//  joy_highmargin.y=joycentre.y+(joymax.y-joycentre.y)/8;
 /* prevent a divide by zero in JoySortOutAnalogue() */
//  if (joy_lowmargin.x-joymin.x==0)
//    joy_lowmargin.x++;
//  if (joymax.x-joy_highmargin.x==0)
//    joy_highmargin.x--;
//  if (joy_lowmargin.y-joymin.y==0)
//    joy_lowmargin.y++;
//  if (joymax.y-joy_highmargin.y==0)
//    joy_highmargin.y--;
}

#define JOY_RANGE       128
//static void JoySortOutAnalogue (joypos_t *jp)
//{
//  if (jp->x < joymin.x)
//  jp->x=-JOY_RANGE;
// else
//  if (jp->x > joymax.x)
//   jp->x=JOY_RANGE;
//  else
//   if ((jp->x > joy_lowmargin.x) && (jp->x < joy_highmargin.x))
//jp->x=0;
//   else
//if (jp->x < joy_lowmargin.x)
// jp->x=-JOY_RANGE+(jp->x-joymin.x)*JOY_RANGE/(joy_lowmargin.x-joymin.x);
//else
// jp->x=JOY_RANGE-(joymax.x-jp->x)*JOY_RANGE/(joymax.x-joy_highmargin.x);

// if (jp->y < joymin.y)
//  jp->y=-JOY_RANGE;
// else
//  if (jp->y > joymax.y)
//   jp->y=JOY_RANGE;
//  else
//   if ((jp->y > joy_lowmargin.y) && (jp->y < joy_highmargin.y))
//jp->y=0;
//   else
//if (jp->y < joy_lowmargin.y)
// jp->y=-JOY_RANGE+(jp->y-joymin.y)*JOY_RANGE/(joy_lowmargin.y-joymin.y);
//else
// jp->y=JOY_RANGE-(joymax.y-jp->y)*JOY_RANGE/(joymax.y-joy_highmargin.y);

//}

static void Joy_Check (void)
{
	Sint16 x, y;
	Uint8 button1, button2, button3 = 0, button4 = 0;

	if (!gotjoy)
		return;

	SDL_JoystickUpdate();
	x = SDL_JoystickGetAxis(joy,0)/512+64;
	y = SDL_JoystickGetAxis(joy,1)/512+64;
	button1 = SDL_JoystickGetButton(joy,0);
	button2 = SDL_JoystickGetButton(joy,1);
	if (fourbuttons) {
		button3 = SDL_JoystickGetButton(joy,2);
		button4 = SDL_JoystickGetButton(joy,3);
	}


	switch (expansionmode)
	{
		case 0:
		case 5:
		case 6:
		case 7:
			if (x<(joycentre.x*3/4))
			{
				JoystickJoyState[0]&=0xF7FF;
			}
			else
			if (x>(joycentre.x*5/4))
			{
				JoystickJoyState[0]&=0xFDFF;
			}
			if (y<(joycentre.y*3/4))
			{
				JoystickJoyState[0]&=0xFEFF;
			}
			else
			if (y>(joycentre.y*5/4))
			{
				JoystickJoyState[0]&=0xFBFF;
			}

			if (button1)
				JoystickJoyState[0]&=(swapbuttons&1)? 0xFFBF:0xBFFF;
			if (button2)
				JoystickJoyState[0]&=(swapbuttons&1)? 0xBFFF:0xFFBF;
			if (fourbuttons) {
				if (button3)
					JoystickJoyState[0]=(JoyState[0]&0xFFF0)|12;
				if (button4)
					JoystickJoyState[0]=(JoyState[0]&0xFFF0)|13;
			}
		break;
		case 2:
			//JoySortOutAnalogue (&jp);
			mouse_x=x;
			mouse_y=y;
			mouse_buttons=0;
			if (button1)
				mouse_buttons|=(swapbuttons&1)? 2:1;
			if (button2)
				mouse_buttons|=(swapbuttons&1)? 1:2;
			if (fourbuttons) {
				if (button3)
					mouse_buttons|=4;
				if (button4)
					mouse_buttons|=8;
			}
			Mouse_SetJoyState ();
		break;
		case 3:
			if (y<0)
				JoystickJoyState[0]&=0xFEFF;
			//JoySortOutAnalogue (&jp);
			mouse_x=(x/2);
			mouse_y=0;
			Mouse_SetJoyState ();
			if (button1)
				JoystickJoyState[1]&=(swapbuttons&1)? 0xFBFF:0xFEFF;
			if (button2)
				JoystickJoyState[1]&=(swapbuttons&1)? 0xFEFF:0xFBFF;
			if (fourbuttons) {
				if (button3)
					mouse_buttons|=4;
				if (button4)
					mouse_buttons|=8;
			}
		break;
	}
}

/****************************************************************************/
/** Deallocate all resources taken by InitMachine()                        **/
/****************************************************************************/
void TrashMachine(void)
{
	SDL_Quit();
}

//static unsigned SwapBytes (unsigned val,unsigned depth)
//{
//	if (depth==8)
//		return val;

//	if (depth==16)
//		return ((val>>8)&0xFF)+((val<<8)&0xFF00);

//	return ((val>>24)&0xFF)+((val>>8)&0xFF00)+
//		((val<<8)&0xFF0000)+((val<<24)&0xFF000000);
//}

static int NextFile (char *s)
{
	char *p;

	p=s+strlen(s)-1;
	if (*p=='9')
	{
		*p='0';
		--p;
		if (*p=='9')
		{
			(*p)++;
			if (*p=='0')
				return 0;
		}
		else
			(*p)++;
	}
	else
		(*p)++;

	return 1;
}

static int NextBitmapFile (void)
{
	return NextFile (szBitmapFile);
}

static int NextSnapshotFile (void)
{
	return NextFile (szSnapshotFile);
}

/****************************************************************************/
/** Allocate resources needed by SDL driver code                           **/
/****************************************************************************/
int InitMachine(void)
{
	int i;
	FILE *snapshotfile;
	SDL_Color colors[16];
	
	

	memset (VGA_Palette,0,sizeof(VGA_Palette));
	memcpy (VGA_Palette,Coleco_Palette,16*3);

	width=WIDTH; height=HEIGHT;
	Black = 0;
	White = 0xff;
	bpp = 8;

	switch (videomode)
	{
		case 1:
			width*=2;
		break;
		case 2:
			width*=2;
		 	height*=2;
		break;
		default:
			videomode=0;
	}

	if (Verbose) printf ("Initialising SDL drivers...");
	if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 )
	{
		if (Verbose) printf("FAILED: %s\n", SDL_GetError());
		//return 0;
	}
	if (Verbose) printf ("OK\n  Opening display... ");
	if (Verbose) printf ("OK\n  Opening window... ");


	SDL_EnableKeyRepeat(150, 50);
	SDL_WM_SetCaption(Title,NULL);
//	screen = SDL_SetVideoMode(width, height, bpp, SDL_SWSURFACE|SDL_FULLSCREEN); //Create a window
	if (scale2xMode == 0) {
//		screen = SDL_SetVideoMode(width, height, bpp, SDL_SWSURFACE|SDL_VIDEORESIZE); //Create a window
		screen = SDL_SetVideoMode(width, height, bpp, SDL_SWSURFACE|SDL_FULLSCREEN); //Create a window
	} else {
//		scale2xSurface = SDL_SetVideoMode(width*2,height*2,bpp, SDL_SWSURFACE|SDL_VIDEORESIZE);
		scale2xSurface = SDL_SetVideoMode(width*2,height*2,bpp, SDL_SWSURFACE|SDL_FULLSCREEN);
		screen = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,bpp,0,0,0,0);
	}
	
	if(!screen) //Couldn't create window?
	{
		printf("Couldn't create screen\n"); //Output to stderr and quit
		return 0;
	}

	DisplayBuf=screen->pixels;
	//if (Verbose) printf ("  Allocating screen buffer... ");
	//DisplayBuf=malloc(bpp*width*height/8);
	//if (!DisplayBuf)
	//{
	// if (Verbose) printf ("FAILED\n");
	// return 0;
	//}

	switch (videomode)
	{
		case 0:
			PutPixelProc=PutPixel8_0;
		break;
		case 1:
			PutPixelProc=PutPixel8_1;
		break;
		case 2:
			PutPixelProc=PutPixel8_2;
		break;
	}

	if (Verbose) printf ("OK\n  Allocating colours... ");
	for (i=0;i<16;++i)
	{
		colors[i].r=Coleco_Palette[i*3+0];
		colors[i].g=Coleco_Palette[i*3+1];
		colors[i].b=Coleco_Palette[i*3+2];
		PalBuf[i] = i;
//		if (!SDL_SetColors(screen,colors,0,16))
//		{
//			if (Verbose) puts ("FAILED");
//			return 0;
//		}
		if (scale2xMode == 0 && !SDL_SetColors(screen,colors,0,16))
		{
			if (Verbose) puts ("FAILED");
			return 0;
		}
		if ((scale2xMode == 1) && !SDL_SetColors(scale2xSurface,colors,0,16))
		{
			if (Verbose) puts ("FAILED");
			return 0;
		}
		
	}

	if (Verbose) puts ("OK");
	while ((snapshotfile=fopen(szSnapshotFile,"rb"))!=NULL)
	{
		fclose (snapshotfile);
		if (!NextSnapshotFile())
			break;
	}
	if (Verbose)
	printf ("  Next snapshot will be %s\n",szSnapshotFile);

	JoyState[0]=JoyState[1]=MouseJoyState[0]=MouseJoyState[1]=
	JoystickJoyState[0]=JoystickJoyState[1]=0x7F7F;

	InitSound ();
	if (expansionmode==2 || expansionmode==3 || (expansionmode!=1 && expansionmode!=4 && joystick))
		Joy_Init ();

	if (expansionmode==1 || expansionmode==4 ||
		expansionmode==5 || expansionmode==6 ||
		expansionmode==7)
		i=Mouse_Detect ();

	if (mouse_sens<=0 || mouse_sens>1000)
		mouse_sens=default_mouse_sens;

	switch (expansionmode)
	{
		case 2:
		case 3:
			if (!gotjoy)
			{
				expansionmode=0;
				break;
			}
			CalibrateJoystick ();
		break;
		case 1:
		case 4:
		case 5:
		case 6:
		case 7:
			if (!got_mouse)
				expansionmode=0;
		break;
		default:
			expansionmode=0;
			default_mouse_sens*=5;
		break;
	}
	if (syncemu)
		OldTimer=ReadTimer ();

	/* Parse keyboard mapping string */
	sscanf (szKeys,"%03X%03X%03X%03X%03X%03X%03X%03X", &KEY_LEFT,&KEY_RIGHT,&KEY_UP,&KEY_DOWN,
												       &KEY_BUTTONA,&KEY_BUTTONB,&KEY_BUTTONC,&KEY_BUTTOND);
	Mouse_Init ();
	switch (EmuMode)
	{
		case 0:
			printf("**In Colecovision Emulation Mode**\n");
			break;
		case 1:
			printf("**In Adam Emulation Mode**\n");
			break;
	}
	keyboardmode=(EmuMode)? 1:0;

	//Confine the keyboard and mouse input to the emulator window
//	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_ShowCursor(SDL_DISABLE);

	//Ignore various events...
	SDL_EventState(SDL_ACTIVEEVENT,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP,SDL_IGNORE);

	return 1;
}

/****************************************************************************/
/** Routines to modify the Coleco gameport status                          **/
/****************************************************************************/
static int numkeypressed (int nScanCode)
{
	int nOr;

	switch (nScanCode)
	{
		case SCANCODE_1:
		case SCANCODE_KEYPAD1:
			nOr=1;
		break;

		case SCANCODE_2:
		case SCANCODE_KEYPAD2:
			nOr=2;
		break;

		case SCANCODE_3:
		case SCANCODE_KEYPAD3:
			nOr=3;
		break;

		case SCANCODE_4:
		case SCANCODE_KEYPAD4:
			nOr=4;
		break;

		case SCANCODE_5:
		case SCANCODE_KEYPAD5:
			nOr=5;
		break;

		case SCANCODE_6:
		case SCANCODE_KEYPAD6:
			nOr=6;
		break;

		case SCANCODE_7:
		case SCANCODE_KEYPAD7:
			nOr=7;
		break;

		case SCANCODE_8:
		case SCANCODE_KEYPAD8:
			nOr=8;
		break;

		case SCANCODE_9:
		case SCANCODE_KEYPAD9:
			nOr=9;
		break;

		case SCANCODE_EQUAL:
		case SCANCODE_KEYPADENTER:
			nOr=10;
		break;

		case SCANCODE_MINUS:
		case SCANCODE_KEYPADPERIOD:
			nOr=11;
		break;

		default:
		nOr=0;
	}

	return nOr;
}

static void Joysticks (void)
{
	int i,tmp,tmp2;

	MouseJoyState[0]|=0x7F7F;
	MouseJoyState[1]|=0x7F7F;
	Mouse_Check ();

	JoystickJoyState[0]|=0x7F7F;
	JoystickJoyState[1]|=0x7F7F;
	Joy_Check ();
	JoyState[1]=(MouseJoyState[0] & JoystickJoyState[0]);
	JoyState[0]=(MouseJoyState[1] & JoystickJoyState[1]);

	if (!keyboardmode)
	{
		if (keybstatus[KEY_BUTTONA])
			JoyState[0]&=(swapbuttons&2)? 0xFFBF:0xBFFF;
		if (keybstatus[KEY_BUTTONB])
			JoyState[0]&=(swapbuttons&2)? 0xBFFF:0xFFBF;
		if (keybstatus[KEY_DOWN])
			JoyState[0]&=0xFBFF;
		if (keybstatus[KEY_UP])
			JoyState[0]&=0xFEFF;
		if (keybstatus[KEY_LEFT])
			JoyState[0]&=0xF7FF;
		if (keybstatus[KEY_RIGHT])
			JoyState[0]&=0xFDFF;
		for (i=SCANCODE_0;i<=SCANCODE_9;++i)
		if (keybstatus[i])
		{
			tmp=numkeypressed(i);
			JoyState[0]=(JoyState[0]&0xFFF0)|tmp;
		}
		if (keybstatus[SCANCODE_MINUS])
		{
			tmp=numkeypressed(SCANCODE_MINUS);
			JoyState[0]=(JoyState[0]&0xFFF0)|tmp;
		}
		if (keybstatus[SCANCODE_EQUAL])
		{
			tmp=numkeypressed(SCANCODE_EQUAL);
			JoyState[0]=(JoyState[0]&0xFFF0)|tmp;
		}
		if (keybstatus[KEY_BUTTONC])
			JoyState[0]=(JoyState[0]&0xFFF0)|12;
		if (keybstatus[KEY_BUTTOND])
			JoyState[0]=(JoyState[0]&0xFFF0)|13;
		for (i=SCANCODE_KEYPAD7;i<=SCANCODE_KEYPAD9;++i)
		if (keybstatus[i])
		{
			tmp=numkeypressed((keypadmode)? (i-SCANCODE_KEYPAD7+SCANCODE_KEYPAD1) : i);
			JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
		}
		for (i=SCANCODE_KEYPAD4;i<=SCANCODE_KEYPAD6;++i)
		if (keybstatus[i])
		{
			tmp=numkeypressed(i);
			JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
		}
		for (i=SCANCODE_KEYPAD1;i<=SCANCODE_KEYPAD3;++i)
		if (keybstatus[i])
		{
			tmp=numkeypressed((keypadmode)? (i-SCANCODE_KEYPAD1+SCANCODE_KEYPAD7) : i);
			JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
		}
		if (keybstatus[SCANCODE_KEYPAD0])
		{
			tmp=numkeypressed((keypadmode)? SCANCODE_KEYPADPERIOD : SCANCODE_KEYPAD0);
			JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
		}
		if (keybstatus[SCANCODE_KEYPADPERIOD])
		{
			tmp=numkeypressed((keypadmode)? SCANCODE_KEYPAD0 : SCANCODE_KEYPADPERIOD);
			JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
		}
		if (keybstatus[SCANCODE_KEYPADENTER])
		{
			tmp=numkeypressed(SCANCODE_KEYPADENTER);
			JoyState[1]=(JoyState[1]&0xFFF0)|tmp;
		}
		if (keybstatus[SCANCODE_KEYPADPLUS])
			JoyState[1]=(JoyState[1]&0xFFF0)|13;
		if (keybstatus[SCANCODE_KEYPADMINUS])
			JoyState[1]=(JoyState[1]&0xFFF0)|12;
	}

	switch (expansionmode)
	{
		case 1:                         /* emulate roller controller with mouse   */
		case 2:                         /* emulate RC with joystick               */
			if ((JoyState[1]&0x0F)==12 || (JoyState[1]&0x0F)==13)
				JoyState[1]|=0x0F;
			if ((JoyState[0]&0x0F)==12 || (JoyState[0]&0x0F)==13)
				JoyState[0]|=0x0F;
			tmp=JoyState[1];
			JoyState[1]=(JoyState[0]&0x707F)|0x0F00;
			JoyState[0]=(tmp&0x7F7F);
			if ((JoyState[1]&0xF)!=0xF)
				JoyState[0]=(JoyState[0]&0xFFF0)|(JoyState[1]&0x0F);
			JoyState[1]=(JoyState[1]&0xFFF0)|(JoyState[0]&0x0F);
		break;
		case 3:                         /* emulate driving module with joystick   */
		case 4:                         /* emulate driving module with mouse      */
			if ((JoyState[1]&0x0F)==12 || (JoyState[1]&0x0F)==13)
				JoyState[1]|=0x0F;
			if ((JoyState[0]&0x0F)==12 || (JoyState[0]&0x0F)==13)
				JoyState[0]|=0x0F;
			if ((JoyState[1]&0xF)!=0xF)
				JoyState[0]=(JoyState[0]&0xFFF0)|(JoyState[1]&0x0F);
			if ((JoyState[1]&0x0100)==0)
				JoyState[1]&=0xBFFF;
			else
				JoyState[1]|=0x4000;
				JoyState[1]|=0x0F7F;
				tmp=JoyState[1];
				JoyState[1]=JoyState[0];
				JoyState[0]=tmp|0x0040;
		break;
		default:
			switch (joystick)
			{
				case 1:                       /* Joystick 1=Joystick 2                  */
					tmp=JoyState[0]&0x0F;
					tmp2=JoyState[1]&0x0F;
					if (tmp==12 || tmp==13)
						JoyState[1]=(JoyState[1]&0xFFF0)|(JoyState[0]&0xF);
					else
					if (tmp2==12 || tmp2==13)
						JoyState[0]=(JoyState[0]&0xFFF0)|(JoyState[1]&0xF);
					else
					if (tmp2!=15)
					{
						JoyState[0]=(JoyState[0]&0xFFF0)|(JoyState[1]&0x0F);
						JoyState[1]|=15;
					}
					JoyState[0]&=(JoyState[1]|0xF);
					JoyState[1]&=(JoyState[0]|0xF);
				break;

				case 2:                       /* Joystick 1=keyb, Joystick 2=joystick   */
				break;

				case 3:                       /* Joystick 1=joystick, Joystick 2=keyb   */
					tmp=JoyState[0];
					JoyState[0]=JoyState[1];
					JoyState[1]=tmp;
				break;
			}
		break;
	}
}

/****************************************************************************/
/*** Parse keyboard events                                                ***/
/****************************************************************************/
void Keyboard (void)
{
	//blurt
		int tmp;
	keyboard_update ();
	//blurt

	/* Check if reset combination is pressed */
	//blurt
	if (AltPressed())
	{
	//blurt
		if (keybstatus[SCANCODE_F12]) 
		{
			blurt			
			ResetColeco (0);
		}
		if (keybstatus[SCANCODE_F11])
		{
			blurt		
			ResetColeco (1);
		}
	//blurt
	}

	/* Update keyboard buffer */
	//blurt
	do
	{
	//blurt
		tmp=LocalGetKeyboardChar ();
	//blurt
		if (tmp) AddToKeyboardBuffer (tmp);
	} while (tmp);
	//blurt

	/* Check mouse and joystick events */
	//blurt
	Joysticks ();
	//blurt

	/* Check is PAUSE is pressed */
	if (PausePressed)
	{
	//blurt
		StopSound ();
	//blurt
		tmp=0;
	//blurt
		while (PausePressed) keyboard_update();
	//blurt
		ResumeSound ();
	//blurt
		if (syncemu)
			OldTimer=ReadTimer ();
	//blurt
	}

	 /* Check if a screen shot should be taken */
	//blurt
	if (makeshot)
	{
	//blurt
		WriteBitmap (szBitmapFile,8,17,WIDTH,WIDTH,HEIGHT,
					 (char *)DisplayBuf,(char *)VGA_Palette);
	//blurt
		NextBitmapFile ();
	//blurt
		makeshot--;
	}

	/* Check if a snapshot should be taken */
	//blurt
	if (makesnap)
	{
	//blurt
		SaveSnapshotFile (szSnapshotFile);
	//blurt
		NextSnapshotFile ();
	//blurt
		makesnap--;
	}

	/* Check if OptionsDialogue() should be called */
	//blurt
	if (calloptions)
	{
	//blurt
		calloptions=0;
	//blurt
		StopSound ();
	//blurt
		OptionsDialogue ();
	//blurt
		ResumeSound ();
	//blurt
		if (syncemu) OldTimer=ReadTimer();
	//blurt
	}
	//blurt

}

/****************************************************************************/
/** Interrupt routines                                                     **/
/****************************************************************************/
/* Gets called 50 times per second */
int CheckScreenRefresh (void)
{
	static int skipped=0;
	if (syncemu)
	{
		NewTimer=ReadTimer ();
		OldTimer+=1000000/IFreq;
		if ((OldTimer-NewTimer)>0)
		{
			do
				NewTimer=ReadTimer ();
			while ((NewTimer-OldTimer)<0);
			skipped=0;
			return 1;
		}
		else
		if (++skipped>=UPeriod)
		{
			OldTimer=ReadTimer ();
			skipped=0;
			return 1;
		}
		else
			return 0;
	}
	return 2;
}

//static void FASTCALL PutPixel8_0 (int offset,int c)
static void PutPixel8_0 (int offset,int c)
{
	SDL_LockSurface(screen);
	DisplayBuf[offset]=c;
	SDL_UnlockSurface(screen);
}

static void PutPixel8_1 (int offset,int c)
{
	SDL_LockSurface(screen);
	DisplayBuf[offset*2]=DisplayBuf[offset*2+1]=c;
	SDL_UnlockSurface(screen);
}

static void PutPixel8_2 (int offset,int c)
{
	SDL_LockSurface(screen);
	offset=(offset&0x00FF)+((offset&0xFF00)<<1);
	DisplayBuf[offset*2]=DisplayBuf[offset*2+1]=c;
	DisplayBuf[(offset+0x0100)*2]=DisplayBuf[(offset+0x0100)*2+1]=c;
	SDL_UnlockSurface(screen);
}

/* Return the time elapsed in microseconds */
static int ReadTimer (void)
{
	return SDL_GetTicks()*1000;
//	return (int)((float)clock()*1000000.0/(float)CLOCKS_PER_SEC);

}

void setFullScreen(int on)
{
	int i=0;
	if (on && !fullscreen) {
		if (scale2xMode == 1) {
			scale2xSurface = SDL_SetVideoMode(width*2,height*2,bpp, SDL_SWSURFACE|SDL_FULLSCREEN);
			screen = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,bpp,0,0,0,0);
			
		} else {
			screen = SDL_SetVideoMode(width, height, bpp, SDL_SWSURFACE|SDL_FULLSCREEN); //Create a window
		}
		fullscreen = 1;
	} else {
		if (scale2xMode == 1) {
			scale2xSurface = SDL_SetVideoMode(width*2,height*2,bpp, SDL_SWSURFACE);
			screen = SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,bpp,0,0,0,0);
		} else {
			screen = SDL_SetVideoMode(width, height, bpp, SDL_SWSURFACE); //Create a window
			fullscreen = 0;
		}
	}
	
	
	
	DisplayBuf = screen->pixels;
	
	for (i=0;i<16;++i)
	{
		SDL_Color colors[16];
		colors[i].r=Coleco_Palette[i*3+0];
		colors[i].g=Coleco_Palette[i*3+1];
		colors[i].b=Coleco_Palette[i*3+2];
		PalBuf[i] = i;
		if (!SDL_SetColors(screen,colors,0,16))
		{
			if (Verbose) puts ("FAILED");
			return;
		}
	}
	
	ColourScreen();
	VDP.ScreenChanged=1;
	RefreshScreen(VDP.ScreenChanged);
	
}

/****************************************************************************/
/** Screen refresh drivers                                                 **/
/****************************************************************************/
#include "Common.h"
