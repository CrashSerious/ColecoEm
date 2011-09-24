/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                ADAMEm.c                                **/
/**                                                                        **/
/** This file contains the main() function starting the emulator           **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Coleco.h"

#include <SDL/SDL.h>
#include <sys/process.h>

#ifdef MSDOS
#include "MSDOS.h"
#include <sys/stat.h>
#include <conio.h>
#endif
#ifdef LINUX_SVGA
#include "SVGALib.h"
#endif
#ifdef UNIX_X
#include "X.h"
#endif
#ifdef SDL
#include "AdamemSDL.h"
#endif
#include "Help.h"

SYS_PROCESS_PARAM( 1001, 0x100000 ) ;

/* Program title for -help output */
extern char Title[];
extern int  videomode; 
extern int  joystick;
extern int  calibrate;                 			/* Set to 1 to force joystick calibration */
extern int  swapbuttons;               			/* 1=joystick, 2=keyboard, 4=mouse        */
extern int  expansionmode;                 	    /* Expansion module emulated              */
extern int  mouse_sens;              			/* Mouse sensitivity                      */
extern char *szKeys;
extern int keypadmode;
extern int syncemu;
extern char scale2xMode;
extern char szJoystickFileName[256];

#define MAX_CONFIG_FILE_SIZE    1024
#define MAX_FILE_NAME           256

char *Options[]=
{
  "verbose","help","cpuspeed","ifreq",
  "cheat","video","sound","joystick","swapbuttons",
  "expansion","calibrate","overscan","volume","soundtrack",
  "trap","os7","sensitivity","reverb","chorus","uperiod",
  "soundquality","speakerchannels","keys","printer","keypad",
  "eos","wp","diska","diskb","diskc","diskd",
  "tapea","tapeb","tapec","taped","sync","adam","cv","stereo",
  "printertype","chipset","sprite","shm","savecpu","palette",
  "ram","snap","autosnap","diskspeed","tapespeed","lpt","tdos",
  "cart","exprom","scale2x","harddisk",
  NULL
};

char *AbvOptions[]=
{
  "vb","he","cs","if",
  "cheat","vi","so","js","sb",
  "ex","ca","os","vo","st",
  "trap","os7","se","re","ch","up",
  "sq","sc","ke","pr","kp",
  "eos","wp","da","db","dc","dd",
  "ta","tb","tc","td","sy","adam","cv","stereo",
  "pt","chipset","sprite","shm","savecpu","pal",
  "ram","sn","asn","ds","ts","lpt","tdos",
  "ct","er","2x","hd",
  NULL
};

#define PROGRAM_ADAMEM  0
#define PROGRAM_CVEM    1
static int Program;
static int CPUSpeed;
static int _argc;
static char *_argv[256];
#ifdef OSX
static char *osxConfigPath = "/Library/Preferences/";
#endif
static unsigned char MainConfigFile[MAX_CONFIG_FILE_SIZE];
static unsigned char SubConfigFile[MAX_CONFIG_FILE_SIZE];
static char szTempFileName[MAX_FILE_NAME];
static char _CartName[MAX_FILE_NAME];
static char CartNameNoExt[MAX_FILE_NAME];
static char _SnapshotName[MAX_FILE_NAME];
static char _OS7File[MAX_FILE_NAME];
static char _EOSFile[MAX_FILE_NAME];
static char _WPFile[MAX_FILE_NAME];
static char _ExpRomFile[MAX_FILE_NAME];
static char _HardDiskFile[MAX_FILE_NAME];
static char ProgramPath[MAX_FILE_NAME];
static char ProgramName[MAX_FILE_NAME];
static int  CartNameSupplied=0;
static int  ExpRomSupplied=0;

#ifndef MSDOS
/* Get full path name, convert all backslashes to UNIX style slashes */
static void _fixpath (char *old,char *new)
{
 strcpy (new,old);
}
#endif

#ifdef MSDOS
static void ParseSpeakerChannels (char *p)
{
 int i,a;
 memset (spk_channels,0,sizeof(spk_channels));
 for (i=0;i<4&&p;++i)
 {
  a=atoi(p);
  if (a<0 || a>4)
   a=0;
  spk_channels[i]=a;
  p=strchr(p,',');
  if (p)
  {
   *p='\0';
   ++p;
  }
 }
}
#endif

static int ParseOptions (int argc,char *argv[])
{
 int N,I,J,tmp;
 int misparm;
 for(N=1,I=0;N<argc;N++)
 {
  misparm=0;
  if(*argv[N]!='-')
   switch(I++)
   {
    case 0:  /* CartName=argv[N]; */
             break;
    default: printf("Excessive filename '%s'\n",argv[N]);
             return 0;
   }
  else
  {    
   for(J=0;Options[J];J++)
    if(!strcmp(argv[N]+1,Options[J])) break;
   if (!Options[J])
    for(J=0;AbvOptions[J];J++)
     if(!strcmp(argv[N]+1,AbvOptions[J])) break;
   switch(J)
   {
    case 0:  N++;
             if(N<argc)
              Verbose=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 1:  printf ("%s\n"
                     "Copyright (C) 1996  Marcel de Kogel\n"
                     "Usage: %s [-option1 [-option2...]] [filename]\n"
                     "[filename] = name of the file to load as a cartridge [CART.ROM]\n"
                     "[-option]  =\n",Title,ProgramName);
             tmp=5;
             for(J=0;HelpText[J];J++)
             {
              if (HelpText[J][0]=='A')
              {
               if (Program==PROGRAM_ADAMEM)
               {
                puts (HelpText[J]+1);
                ++tmp;
               }
              }
              else if (HelpText[J][0]=='C')
              {
               if (Program==PROGRAM_CVEM)
               {
                puts (HelpText[J]+1);
                ++tmp;
               }
              }
              else
              {
               puts(HelpText[J]);
               ++tmp;
              }
#if defined(MSDOS) || defined(LINUX_SVGA)
              if (tmp==24)
              {
               printf ("-- More --");
               fflush (stdout);
#ifdef MSDOS
               getch();
#else
               fgetc (stdin);
#endif
               fflush (stdin);
               printf ("\n\n");
               tmp=1;
              }
#endif
             }
             return 0;
    case 2:  N++;
             if(N<argc)
              CPUSpeed=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 3:  N++;
             if(N<argc)
              IFreq=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 13: N++;
             if(N<argc)
             {
#ifdef MSDOS
              strcpy (szTempFileName,argv[N]);
              strupr (szTempFileName);
              if (!strcmp(szTempFileName,"NULL"))
               SoundName=NULL;
              else
#endif
               SoundName=argv[N];
             }
             else
              misparm=1;
             break;
    case 15: N++;
             if(N<argc)
              OS7File=argv[N];
             else
              misparm=1;
             break;
    case 19: N++;
             if(N<argc)
              UPeriod=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 25: N++;
             if(N<argc)
              EOSFile=argv[N];
             else
              misparm=1;
             break;
    case 26: N++;
             if(N<argc)
              WPFile=argv[N];
             else
              misparm=1;
             break;
    case 27: case 28: case 29: case 30:
             N++;
             if(N<argc)
              DiskName[J-27]=argv[N];
             else
              return 0;
             break;
    case 31: case 32: case 33: case 34:
             N++;
             if(N<argc)
              TapeName[J-31]=argv[N];
             else
              return 0;
             break;
    case 23: N++;
             if(N<argc)
              PrnName=argv[N];
             else
              misparm=1;
             break;
    case 36: 
			 printf("Setting Emulation Mode to ADAM.\n");
			 EmuMode=1;
             break;
    case 37: 
			 printf("Setting Emulation Mode to COLECOVISION.\n");
			 EmuMode=0;
             break;
#ifdef SOUND
    case 6:  N++;
             if(N<argc)
              soundmode=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 12: N++;
             if(N<argc)
              mastervolume=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 20: N++;
             if(N<argc)
              soundquality=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 38: N++;
             if(N<argc)
              panning=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 17: N++;
             if(N<argc)
              reverb=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 18: N++;
             if(N<argc)
              chorus=atoi(argv[N]);
             else
              misparm=1;
             break;
#ifdef MSDOS
    case 21: N++;
             if(N<argc)
              ParseSpeakerChannels(argv[N]);
             else
              misparm=1;
             break;
#endif
#endif /* SOUND */
    case 4:  N++;
             if(N<argc)
             {
              if (CheatCount<MAX_CHEATS)
               Cheats[CheatCount++]=strtoul(argv[N],NULL,16);
              else
               printf ("WARNING: Maximum number of cheats (%d) reached\n",
                       MAX_CHEATS);
             }
             else
              misparm=1;
             break;
    case 5:  N++;
             if(N<argc)
              videomode=atoi(argv[N]);
             else
              misparm=1;
             break;
#if defined(MSDOS) || defined(LINUX_SVGA)
    case 11: N++;
             if(N<argc)
              useoverscan=atoi(argv[N]);
             else
              misparm=1;
             break;
#endif
    case 7:  N++;
             if(N<argc)
              joystick=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 8:  N++;
             if(N<argc)
              swapbuttons=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 9:  N++;
             if(N<argc)
              expansionmode=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 10: N++;
             if(N<argc)
              calibrate=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 16: N++;
             if(N<argc)
              mouse_sens=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 22: N++;
             if(N<argc)
              szKeys=argv[N];
             else
              misparm=1;
             break;
    case 24: N++;
             if(N<argc)
              keypadmode=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 35: N++;
             if(N<argc)
              syncemu=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 39: N++;
             if(N<argc)
              PrnType=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 44: N++;
             if(N<argc)
              PalNum=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 45: N++;
             if(N<argc)
              RAMPages=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 46: N++;
             if(N<argc)
              SnapshotName=argv[N];
             else
              misparm=1;
             break;
    case 47: N++;
             if(N<argc)
              SaveSnapshot=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 48: N++;
             if(N<argc)
              DiskSpeed=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 49: N++;
             if(N<argc)
              TapeSpeed=atoi(argv[N]);
             else
              misparm=1;
             break;
    case 50: N++;
             if(N<argc)
              LPTName=argv[N];
             else
              misparm=1;
             break;
#ifdef TEXT80
    case 51: N++;
             if(N<argc)
              AutoText80=atoi(argv[N]);
             else
              misparm=1;
             break;
#endif
     case 52: N++;
 	if(N<argc) {
 	 CartName=argv[N];
 	 CartNameSupplied=1;
 	} else {
 	  misparm=1;
 	}
 	break;
#ifdef LINUX_SVGA
    case 40: N++;
             if(N<argc)
              chipset=atoi(argv[N]);
             else
              misparm=1;
             break;
#endif
    case 41: N++;
             if (N<argc)
              Support5thSprite=atoi(argv[N]);
             else
              misparm=1;
             break;
#ifdef UNIX_X
 #ifdef MITSHM
    case 42: N++;
             if(N<argc)
              UseSHM=atoi(argv[N]);
             else
              misparm=1;
             break;
 #endif
    case 43: N++;
             if(N<argc)
              SaveCPU=atoi(argv[N]);
             else
              misparm=1;
             break;
#endif
#ifdef DEBUG
    case 14: N++;
             if (!strcmp(argv[N],"now")) Z80_Trace=1;
             else Z80_Trap=strtoul(argv[N],NULL,16);
             break;
#endif
	case 53: N++;
		if(N<argc) {
	 ExpRomFile=argv[N];
	 ExpRomSupplied=1;
	} else {
      misparm=1;
	}
	break;
	case 54: N++;
	 scale2xMode = 1;
	break;
	case 55: N++;
		if(N<argc) {
	 HardDiskFile=argv[N];
	} else {
      misparm=1;
	}
	break;

    default: printf("Wrong option '%s'\n",argv[N]);
             return 0;
   }
   if (misparm)
   {
    printf("%s: Missing parameter\n",argv[N-1]);
    return 0;
   }
  }
 }
 return 1;
}

static int GetCartName (int argc,char *argv[])
{
 int N,I,J,tmp;
 for(N=1,I=0;N<argc;N++)
 {
  if(*argv[N]!='-')
   switch(I++)
   {
    case 0:  CartName=argv[N];
             CartNameSupplied=1;
             break;
    default: return 0;
   }
  else
  {    
   for(J=0;Options[J];J++)
    if(!strcmp(argv[N]+1,Options[J])) break;
   if (!Options[J])
    for(J=0;AbvOptions[J];J++)
     if(!strcmp(argv[N]+1,AbvOptions[J])) break;
   switch(J)
   {
    case 1:  return 0;
    case 2:  N++;
             tmp=0;
             if(N<argc)
              tmp=atoi(argv[N]);
             if (!tmp)
              return 0;
             break;
    case 36: case 37:
             break;
    case 27: case 28: case 29: case 30:
             N++;
             if(N<argc)
              DiskName[J-27]=argv[N];
             else
              return 0;
             break;
    case 31: case 32: case 33: case 34:
             N++;
             if(N<argc)
              TapeName[J-31]=argv[N];
             else
              return 0;
             break;
#if defined(MSDOS) || defined(LINUX_SVGA)
    case 4: case 11:
#endif
#ifdef LINUX_SVGA
    case 40:
#endif
#ifdef UNIX_X
 #ifdef MITSHM
    case 42:
 #endif
    case 43:
#endif
#ifdef SOUND
    case 6: case 12: case 20:
    case 38: case 17: case 18:
#ifdef MSDOS
    case 21:
#endif
#endif
    case 5: case 7: case 8: case 9: case 10:
    case 16: case 22: case 24: case 35:
    case 0: case 3: case 13: case 15: case 19:
    case 23: case 25: case 26: case 39: case 41: case 44:
    case 45: case 46: case 47: case 48: case 49: case 50:
#ifdef TEXT80
    case 51:
#endif
#ifdef DEBUG
    case 14:
#endif
             N++;
             if (N>=argc)
              return 0;
             break;
    default: return 0;
   }
  }
 }
 return 1;
}

static void LoadConfigFile (char *szFileName,unsigned char *ptr)
{
 FILE *infile;
 infile=fopen (szFileName,"rb");
 if (infile==NULL)
  return;
 fread (ptr,1,MAX_CONFIG_FILE_SIZE,infile);
 fclose (infile);
 while (*ptr)
 {
  while (*ptr && *ptr<=' ')
   ++ptr;
  if (*ptr)
  {
   _argv[_argc++]=(char *)ptr;
   while (*ptr && *ptr>' ')
    ++ptr;
   if (*ptr)
    *ptr++='\0';
  }
 }
}

static void FixFileNames (void)
{
 char *p=NULL,*q=NULL,*x=NULL;
 int i;
 CartNameNoExt[0]='\0';
 if (!CartNameSupplied)
 {
  for (i=0;i<2;++i)
   if (DiskName[i]) { _fixpath (DiskName[i],CartNameNoExt); break; }
  if (CartNameNoExt[0]=='\0')
   for (i=0;i<2;++i)
    if (TapeName[i]) { _fixpath (TapeName[i],CartNameNoExt); break; }
 }
 if (CartNameNoExt[0]=='\0')
  _fixpath (CartName,CartNameNoExt);
 p=CartNameNoExt;
 q=strchr(CartNameNoExt,'/');
 while (q)                      /* get last '/' */
 {
  p=++q;
  q=strchr(q,'/');
 };
 q=NULL;
 while ((p=strchr(p,'.'))!=NULL) /* get last '.' */
 {
  q=p;
  ++p;
 }
 if (q)                         /* remove extension */
  *q='\0';

 p=CartName;
 q=strchr(CartName,'/'); if (!q) strchr(CartName,'\\');
 while (q)                      /* get last '/' */
 {
  p=++q;
  x=strchr(q,'/'); if (!x) x=strchr(q,'\\');
  q=x;
 };
 q=NULL;
 while ((p=strchr(p,'.'))!=NULL) /* get last '.' */
 {
  q=p;
  ++p;
 }
 strcpy (_CartName,CartName);
 if (!q)
  strcat (_CartName,".rom");
 CartName=_CartName;
}

static void FixROMPath (char **file,char *storage)
{
 char *p=NULL,*q=NULL;
 if (!strchr(*file,'/') && !strchr(*file,'\\'))
 {      /* If no path is given, assume emulator path */
  strcpy (storage,ProgramPath);
  strcat (storage,*file);
 }
 else
 {
  _fixpath (*file,storage);
 }
 p=storage;
 q=strchr(storage,'/');
 while (q)                      /* get last '/' */
 {
  p=++q;
  q=strchr(q,'/');
 };
 q=NULL;
 while ((p=strchr(p,'.'))!=NULL) /* get last '.' */
 {
  q=p;
  ++p;
 }
 if (!q)                       /* Default extension='.rom' */
  strcat (storage,".rom");
 *file=storage;
}

static void GetPath (char *szFullPath,char *szPath,char *szFile)
{
 char *p,*q;
 strcpy (szPath,szFullPath);
 p=szPath;
 q=strchr(p,'/');
 while (q)                      /* get last '/' */
 {
  p=++q;
  q=strchr(q,'/');
 };
 if (szFile)
 {
  strcpy (szFile,p);
  *p='\0';                      /* remove filename */
  p=szFile;
  q=NULL;
  while ((p=strchr(p,'.'))!=NULL) /* get last '.' */
  {
   q=p;
   ++p;
  }
  if (q) *q='\0';               /* remove extension */
 }
 else
  *p='\0';                      /* remove filename */

}

#ifdef OSX
int SDL_main (int argc, char *argv[])
#else
int main (int argc,char *argv[])
#endif
{
 
 Verbose=1;
 CPUSpeed=100;
 IFreq=50;
 UPeriod=3;
 CheatCount=0;
#ifdef MSDOS
 PrnName=LPTName="PRN";
#endif
 memset (MainConfigFile,0,sizeof(MainConfigFile));
 memset (SubConfigFile,0,sizeof(SubConfigFile));
 //GetPath (argv[0],ProgramPath,ProgramName);
 strcpy (ProgramPath,"/dev_hdd0/game/PS3ADAMEm/USRDIR/");
 strcpy (ProgramName,"Coleco");
 printf("ProgramPath = %s.\n",ProgramPath);
 printf("ProgramName = %s.\n",ProgramName);
 if (!strcmp(ProgramName,"Adam")) // the . seems to be needed. bug?
 {
  printf("Setting Emulation Mode to Colecovision!.\n");
  blurt
  EmuMode=0;
  Program=PROGRAM_CVEM;
 }
 else
 {
  printf("Setting Emulation Mode to ADAM!.\n");
  blurt
  EmuMode=1;
  Program=PROGRAM_ADAMEM;
 }
 _argc=1;
 _argv[0]=argv[0];
/* Load Config Files */
#ifdef OSX 
 strcpy (szTempFileName,getenv("HOME"));
 strcat (szTempFileName,osxConfigPath);
#else
 strcpy (szTempFileName,ProgramPath);
#endif	
 strcat (szTempFileName,ProgramName);
 strcat (szTempFileName,".cfg");

 printf("Loading config from %s\n",szTempFileName);

 strcpy (szJoystickFileName,ProgramPath);
 strcat (szJoystickFileName,"adamem.joy");
#ifdef SOUND
 strcpy (szSoundFileName,ProgramPath);
 strcat (szSoundFileName,"adamem.snd");
#endif
 LoadConfigFile (szTempFileName,MainConfigFile);
 if (!ParseOptions(_argc,_argv))
  return 1;
 GetCartName (argc,argv);
 FixFileNames ();
#if defined(MSDOS) || defined(LINUX_SVGA) || defined(SDL)
 strcpy (szBitmapFile,CartNameNoExt);
 strcat (szBitmapFile,".b00");
#endif
#if defined(MSDOS) || defined(LINUX_SVGA) || defined(UNIX_X) || defined(SDL)
 strcpy (szSnapshotFile,CartNameNoExt);
 strcat (szSnapshotFile,".s00");
#endif
 strcpy (szTempFileName,CartNameNoExt);
 strcat (szTempFileName,".cfg");
 _argc=1;
 LoadConfigFile (szTempFileName,SubConfigFile);
 if (!ParseOptions(_argc,_argv))
  return 1;
 if (!ParseOptions(argc,argv))
  return 1;
 if (SaveSnapshot && !SnapshotName)
 {
  strcpy (_SnapshotName,CartNameNoExt);
  strcat (_SnapshotName,".snp");
  SnapshotName=_SnapshotName;
 }
 FixROMPath (&OS7File,_OS7File);
 FixROMPath (&EOSFile,_EOSFile);
 FixROMPath (&WPFile,_WPFile);
 FixROMPath (&ExpRomFile,_ExpRomFile);
 FixROMPath (&HardDiskFile,_HardDiskFile);	
 if (!UPeriod) UPeriod=3;
 if (UPeriod>10) UPeriod=10;
 if (IFreq<10) IFreq=10;
 if (IFreq>200) IFreq=200;
 if (CPUSpeed>1000) CPUSpeed=1000;
 if (CPUSpeed<10) CPUSpeed=10;
 Z80_IPeriod=(3579545*CPUSpeed)/(100*1000);
 if (PalNum<0) PalNum=0;
 if (PalNum>=NR_PALETTES) PalNum=NR_PALETTES-1;
#ifndef MSDOS
 if (!InitMachine()) return 0;
#endif
 StartColeco();
 TrashColeco();
#ifndef MSDOS
 TrashMachine ();
#endif
 return 0;
}

