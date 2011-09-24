/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                             AdamemSDL.h	                           **/
/**                                                                        **/
/** This file contains Simple DirectMedia Layer specific definitions.      **/
/** It does not include the sound emulation definitions.                   **/
/**                                                                        **/
/** Copyright (C) Geoff Oltmans 2006                                       **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <keyboard.h>

#ifndef _ADAMEMSDL_H
#define _ADAMEMSDL_H
/* Screen width and height */
#define WIDTH  256
#define HEIGHT 212

extern byte *DisplayBuf;          /* Screen buffer                          */
extern char szJoystickFileName[]; /* File holding joystick information      */
extern char szBitmapFile[];       /* Next screen shot file                  */
extern char szSnapshotFile[];     /* Next snapshot file                     */
extern char *szKeys;              /* Key scancodes                          */
extern int  mouse_sens;           /* Mouse/Joystick sensitivity             */
extern int  keypadmode;           /* 1 if keypad should be reversed         */
extern int  joystick;             /* Joystick support                       */
extern int  calibrate;            /* Set to 1 to force joystick calibration */
extern int  swapbuttons;          /* 1=joystick, 2=keyboard, 4=mouse        */
extern int  expansionmode;        /* Expansion module emulated              */
extern int  syncemu;              /* 0 if emulation shouldn't be synced     */
extern int  SaveCPU;              /* If 1, CPU is saved when focus is out   */
extern int  videomode;            /* 0=1x1  1=2x1                           */
extern int  PausePressed;
extern char scale2xMode;          /* scale2x mode 0 = off, 1 = on           */

void setFullScreen(int on);

#define NR_KEYS		SDLK_LAST

#define SCANCODE_LEFTCONTROL		SDLK_LCTRL
#define SCANCODE_RIGHTCONTROL		SDLK_RCTRL
#define SCANCODE_INSERT			SDLK_INSERT
#define SCANCODE_HOME			SDLK_HOME
#define SCANCODE_PAGEUP			SDLK_PAGEUP
#define SCANCODE_REMOVE			SDLK_DELETE
#define SCANCODE_END			SDLK_END
#define SCANCODE_PAGEDOWN		SDLK_PAGEDOWN
#define SCANCODE_CAPSLOCK		SDLK_CAPSLOCK
#define SCANCODE_F1			SDLK_F1
#define SCANCODE_F2			SDLK_F2
#define SCANCODE_F3			SDLK_F3
#define SCANCODE_F4			SDLK_F4
#define SCANCODE_F5			SDLK_F5
#define SCANCODE_F6			SDLK_F6
#define SCANCODE_F7			SDLK_F7
#define SCANCODE_F8			SDLK_F8
#define SCANCODE_F9			SDLK_F9
#define SCANCODE_F10			SDLK_F10
#define SCANCODE_F11			SDLK_F11
#define SCANCODE_F12			SDLK_F12
#define SCANCODE_LEFTSHIFT		SDLK_LSHIFT
#define SCANCODE_RIGHTSHIFT		SDLK_RSHIFT
#define SCANCODE_KEYPAD0		SDLK_KP0
#define SCANCODE_KEYPAD1		SDLK_KP1
#define SCANCODE_KEYPAD2		SDLK_KP2
#define SCANCODE_KEYPAD3		SDLK_KP3
#define SCANCODE_KEYPAD4		SDLK_KP4
#define SCANCODE_KEYPAD5		SDLK_KP5
#define SCANCODE_KEYPAD6		SDLK_KP6
#define SCANCODE_KEYPAD7		SDLK_KP7
#define SCANCODE_KEYPAD8		SDLK_KP8
#define SCANCODE_KEYPAD9		SDLK_KP9
#define SCANCODE_KEYPADPERIOD		SDLK_KP_PERIOD
#define SCANCODE_KEYPADENTER		SDLK_KP_ENTER
#define SCANCODE_KEYPADPLUS		SDLK_KP_PLUS
#define SCANCODE_KEYPADMINUS		SDLK_KP_MINUS
#define SCANCODE_CURSORLEFT		SDLK_LEFT
#define SCANCODE_CURSORBLOCKLEFT	SDLK_LEFT
#define SCANCODE_CURSORRIGHT		SDLK_RIGHT
#define SCANCODE_CURSORBLOCKRIGHT	SDLK_RIGHT
#define SCANCODE_CURSORUP		SDLK_UP
#define SCANCODE_CURSORBLOCKUP		SDLK_UP
#define SCANCODE_CURSORDOWN		SDLK_DOWN
#define SCANCODE_CURSORBLOCKDOWN	SDLK_DOWN
#define SCANCODE_0			SDLK_0
#define SCANCODE_1			SDLK_1
#define SCANCODE_2			SDLK_2
#define SCANCODE_3			SDLK_3
#define SCANCODE_4			SDLK_4
#define SCANCODE_5			SDLK_5
#define SCANCODE_6			SDLK_6
#define SCANCODE_7			SDLK_7
#define SCANCODE_8			SDLK_8
#define SCANCODE_9			SDLK_9
#define SCANCODE_EQUAL			SDLK_EQUALS
#define SCANCODE_MINUS			SDLK_MINUS
#define SCANCODE_LEFTALT		SDLK_LALT
#define SCANCODE_RIGHTALT		SDLK_RALT
#endif // _ADAMEM_SDL_H

