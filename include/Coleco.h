/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                Coleco.h                                **/
/**                                                                        **/
/** This file contains generic Coleco related definitions                  **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#define blurt  printf ("%s:%s():#%d ::\n", __FILE__, __func__, __LINE__);

#include "Z80.h"                      /* Z80 emulation declarations         */

#define BigSprites    (VDP.Reg[1]&0x01)   /* Magnified sprites              */
#define Sprites16x16  (VDP.Reg[1]&0x02)   /* Sprite size                    */
#define ScreenON      (VDP.Reg[1]&0x40)   /* Show screen                    */

typedef struct
{
 int Reg[8];                          /* VDP registers                      */
 int Status;                          /* VDP status register                */
 int Addr;                            /* Current memory offset              */
 int Mode;                            /* 0=Read VRAM, 1=Write VRAM          */
 byte *VRAM;                          /* VRAM pointer                       */
 int VRAMSize;                        /* Either 0x3FFF or 0x0FFF            */
 int FGColour,BGColour;               /* Current colours                    */
 int VKey;                            /* If 0, VDP is being accessed        */
 int VR;                              /* Last value written to VDP          */
 int ScreenChanged;                   /* 1 if VRAM or VDP regs are changed  */
} VDP_t;

extern VDP_t VDP;                     /* VDP parameters                     */
extern int  Verbose;                  /* Debug msgs ON/OFF                  */
extern int  EmuMode;                  /* 0=ColecoVision, 1=ADAM             */
extern int  IFreq;                    /* VDP interrupt frequency            */
extern int  UPeriod;                  /* Number of interrupts/screen update */
extern int  PrnType;                  /* Type of printer attached           */
extern int  DiskSpeed;                /* Time in ms it takes to read one... */
extern int  TapeSpeed;                /* ... block                          */
extern char *CartName;                /* Cartridge ROM file                 */
extern char *OS7File,*EOSFile,*WPFile;/* Main ROMs                          */
extern char *ExpRomFile;
extern char *HardDiskFile;
extern int  RAMPages;                 /* Number of 64K expansion RAM pages  */
extern char *DiskName[4];             /* Disk image file names              */
extern char *TapeName[4];             /* Tape image file names              */
extern char *PrnName;                 /* Printer log file                   */
extern char *LPTName;                 /* Parallel port log file             */
extern char *SoundName;               /* Sound log file                     */
extern int  JoyState[2];              /* Joystick status                    */
extern int  SpinnerPosition[2];       /* Spinner positions [0..500]         */
extern int  LastSprite[256];          /* Last sprite to be displayed        */
extern int  Support5thSprite;         /* Show only 4 sprites per row        */
#define NR_PALETTES     4
extern byte Palettes[NR_PALETTES][16*3];
extern int  PalNum;                   /* Palette number                     */
#define Coleco_Palette	Palettes[PalNum]
extern int  SaveSnapshot;             /* If 1, auto-save snapshot           */
extern char *SnapshotName;            /* Snapshot file name                 */
#define MAX_CHEATS      16            /* Maximum number of cheat codes      */
                                      /* supported                          */
extern int  Cheats[16];               /* Cheats to patch into game ROM      */
extern int  CheatCount;               /* Number of cheats                   */
extern byte *AddrTabl[256];           /* Currently mapped in pages          */
extern byte *WriteAddrTabl[256];      /* Used to write protect ROM          */

extern byte *CART;					  /* Pointer to the cart rom space      */

void DiskOpen(int i);
void DiskClose(int i);
void TapeOpen(int i);
void TapeClose(int i);

/****************************************************************************/
/*** Save current emulation status to snapshot file                       ***/
/****************************************************************************/
int SaveSnapshotFile (char *filename);

/****************************************************************************/
/*** This function puts a character in the keyboard buffer                ***/
/****************************************************************************/
void AddToKeyboardBuffer (byte ch);

/****************************************************************************/
/*** Change disk and tape images, printer log file, etc.                  ***/
/****************************************************************************/
#ifndef NO_OPTIONS_DIALOGUE
void OptionsDialogue (void);
#endif

/****************************************************************************/
/*** Reset the Coleco                                                     ***/
/*** If mode==0, EOS is booted. If mode==1, the cartridge is booted       ***/
/****************************************************************************/
void ResetColeco (int mode);

/****************************************************************************/
/*** This function allocates all resources necessary for the              ***/
/*** hardware-independent part of the code and starts the emulation. In   ***/
/*** case of a failure, this function returns 0                           ***/
/****************************************************************************/
int StartColeco(void);

/****************************************************************************/
/*** Free memory allocated by StartColeco()                               ***/
/****************************************************************************/
void TrashColeco(void);

/****************************************************************************/
/*** Allocate resources needed by the machine-dependent code              ***/
/************************************************** TO BE WRITTEN BY USER ***/
int InitMachine(void);

/****************************************************************************/
/*** Deallocate all resources taken by InitMachine()                      ***/
/************************************************** TO BE WRITTEN BY USER ***/
void TrashMachine(void);

/****************************************************************************/
/*** These functions are called to initialise the various screen modes    ***/
/*** and refresh the screen                                               ***/
/************************************************** TO BE WRITTEN BY USER ***/
void ColourScreen (void);
void RefreshScreen (int ScreenChanged);

/****************************************************************************/
/*** These functions are called when the modem I/O ports are accessed     ***/
/************************************************** TO BE WRITTEN BY USER ***/
void ModemOut (int reg,int value);
int  ModemIn  (int reg);

/****************************************************************************/
/*** This function is called to poll keyboard and joysticks               ***/
/************************************************** TO BE WRITTEN BY USER ***/
void Keyboard (void);

/****************************************************************************/
/*** This function is called on every VDP interrupt. It should return 0   ***/
/*** if the screen should not be refreshed, 1 if the screen should be     ***/
/*** refreshed, or 2 if the screen should be refreshed according to       ***/
/*** UPeriod setting                                                      ***/
/************************************************** TO BE WRITTEN BY USER ***/
int CheckScreenRefresh (void);

/****************************************************************************/
/*** Sound routines                                                       ***/
/****************************************************************************/
#include "Sound.h"
