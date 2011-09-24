/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/IoDevice/HarddiskIDE.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2006/06/15 00:26:20 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2004 Daniel Vik
**
**  This software is provhdd 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
******************************************************************************
*/
#include "HarddiskIDE.h"
//#include "Board.h"
//#include "SaveState.h"
#include "Disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATUS_ERR  0x01
#define STATUS_DRQ  0x08
#define STATUS_DSC  0x10
#define STATUS_DRDY 0x40



struct HarddiskIde {
    UInt8 errorReg;
    UInt8 sectorCountReg;
    UInt8 sectorNumReg;
    UInt8 cylinderLowReg;
    UInt8 cylinderHighReg;
    UInt8 devHeadReg;
    UInt8 statusReg;
    UInt8 featureReg;
    
    int transferRead;
    int transferWrite;
    UInt32 transferCount;
    UInt32 transferSectorNumber;

    int   sectorDataOffset;
    UInt8 sectorData[512 * 256];

    int diskId;
};


static UInt32 getSectorNumber(HarddiskIde* hd)
{
//    return hd->sectorNumReg | (hd->cylinderLowReg << 8) |
//        (hd->cylinderHighReg << 16) | ((hd->devHeadReg & 0x0f) << 24);
	UInt32 temp;
	temp = hd->sectorNumReg * (hd->cylinderLowReg | hd->cylinderHighReg << 8) * ((hd->devHeadReg & 0x0f) + 1);
	return temp;
}

static void setError(HarddiskIde* hd, UInt8 error)
{
    hd->errorReg = error;
    hd->statusReg |= STATUS_ERR;
    hd->statusReg &= ~STATUS_DRQ;
    hd->transferWrite = 0;
    hd->transferRead = 0;
}

static UInt32 getNumSectors(HarddiskIde* hd)
{
    return hd->sectorCountReg == 0 ? 256 : hd->sectorCountReg;
}

static void executeCommand(HarddiskIde* hd, UInt8 cmd)
{
    hd->statusReg &= ~(STATUS_DRQ | STATUS_ERR);
    hd->transferRead = 0;
    hd->transferWrite = 0;
    switch (cmd) {
        case 0xef: { // 239 Set Feature
            #ifdef DEBUG
            printf("Cmd %d, Cylinder %d, Head %d, Sector %d Count %d\n",cmd,(hd->cylinderHighReg*256+hd->cylinderLowReg),hd->devHeadReg,hd->sectorNumReg,hd->sectorCountReg);
            #endif
            if (hd->featureReg != 0x03) {
                setError(hd, 0x04);
            }
            break;
        }
        case 0xec: { // 236 ATA Identify Device
            #ifdef DEBUG
            printf("Cmd %d, Cylinder %d, Head %d, Sector %d Count %d\n",cmd,(hd->cylinderHighReg*256+hd->cylinderLowReg),hd->devHeadReg,hd->sectorNumReg,hd->sectorCountReg);
            #endif
            if (!diskReadSector(hd->diskId, hd->sectorData, -1, 0, 0, 0, NULL)) {
                setError(hd, 0x44);
                break;
            }
            hd->transferCount = 512/2;
            hd->sectorDataOffset = 0;
            hd->transferRead = 1;
            hd->statusReg |= STATUS_DRQ;
            break;
        }
        case 0x91: { // 145
            #ifdef DEBUG
            printf("Cmd %d, Cylinder %d, Head %d, Sector %d Count %d\n",cmd,(hd->cylinderHighReg*256+hd->cylinderLowReg),hd->devHeadReg,hd->sectorNumReg,hd->sectorCountReg);
            #endif
            break;
        }
        case 0x70: { // 112 Park HD
            #ifdef DEBUG
            printf("Park HD (%d), Cylinder %d, Head %d, Sector %d Count %d\n",cmd,(hd->cylinderHighReg*256+hd->cylinderLowReg),hd->devHeadReg,hd->sectorNumReg,hd->sectorCountReg);
            #endif
            break;
        }
        case 0xf8: { // 248
            UInt32 sectorCount = diskGetSectorsPerTrack(hd->diskId);
            #ifdef DEBUG
            printf("Cmd %d, Cylinder %d, Head %d, Sector %d Count %d\n",cmd,(hd->cylinderHighReg*256+hd->cylinderLowReg),hd->devHeadReg,hd->sectorNumReg,hd->sectorCountReg);
            #endif
            hd->sectorNumReg    = (UInt8)((sectorCount >>  0) & 0xff);
            hd->cylinderLowReg  = (UInt8)((sectorCount >>  8) & 0xff);
            hd->cylinderHighReg = (UInt8)((sectorCount >> 16) & 0xff);
            hd->devHeadReg      = (UInt8)((sectorCount >> 24) & 0x0f);
            break;
        }
        case 0x30: { // 48 Write Sector
            int sectorNumber = getSectorNumber(hd);
            int numSectors = getNumSectors(hd);
            #ifdef DEBUG
            printf("Write HD (%d), Cylinder %d, Head %d, Sector %d Count %d\n",cmd,(hd->cylinderHighReg*256+hd->cylinderLowReg),hd->devHeadReg,hd->sectorNumReg,hd->sectorCountReg);
            #endif
            if ((sectorNumber + numSectors) > (diskGetTracks(hd->diskId) * diskGetSides(hd->diskId) * diskGetSectorsPerTrack(hd->diskId))) {
                setError(hd, 0x14);
                break;
            }
            hd->transferSectorNumber = sectorNumber;
            hd->transferCount = 512/2 * numSectors;
            hd->sectorDataOffset = 0;
            hd->transferWrite = 1;
            hd->statusReg |= STATUS_DRQ;
            break;
        }
        case 0x20: { // 32 Read Sector
            int sectorNumber = getSectorNumber(hd);
            int numSectors = getNumSectors(hd);
            int i;
    
            #ifdef DEBUG
            printf("Read HD (%d), Cylinder %d, Head %d, Sector %d Count %d\n",cmd,(hd->cylinderHighReg*256+hd->cylinderLowReg),hd->devHeadReg,hd->sectorNumReg,hd->sectorCountReg);
            #endif
            if ((sectorNumber + numSectors) > (diskGetTracks(hd->diskId) * diskGetSides(hd->diskId) * diskGetSectorsPerTrack(hd->diskId))) {
                setError(hd, 0x14);
                break;
            }
              
            for (i = 0; i < numSectors; i++) {
                if (!diskReadSector(hd->diskId, hd->sectorData + i * 512, hd->sectorNumReg, hd->devHeadReg, (hd->cylinderHighReg*256+hd->cylinderLowReg), 0, NULL)) {
                    break;
                }
                hd->sectorNumReg++;
                if (hd->sectorNumReg > (diskGetSectorsPerTrack(hd->diskId) - 1)) {
                    hd->sectorNumReg = hd->sectorNumReg - diskGetSectorsPerTrack(hd->diskId);
                    hd->devHeadReg++;
                }
                if (hd->devHeadReg > (diskGetSides(hd->diskId) - 1)) {
                    hd->devHeadReg = hd->devHeadReg - (diskGetSides(hd->diskId) - 1);
                    if (hd->cylinderLowReg = 255) {
                        hd->cylinderLowReg = hd->cylinderLowReg - (256 - 1);
                        hd->cylinderHighReg++;
                    }
                    else {
                        hd->cylinderLowReg++;
                    }
                }
            }
            if (i != numSectors) {
                setError(hd, 0x44);
                break;
            }

            hd->transferCount = 512/2 * numSectors;
            hd->sectorDataOffset = 0;
            hd->transferRead = 1;
            hd->statusReg |= STATUS_DRQ;
            break;
        }
        case 0x10: { // 16
            #ifdef DEBUG
            printf("HD Parameters:\n%d Tracks, %d Heads, %d Sectors/Track\n\n",diskGetTracks(hd->diskId),diskGetSides(hd->diskId),diskGetSectorsPerTrack(hd->diskId));
            printf("Cmd %d, Cylinder %d, Head %d, Sector %d Count %d\n",cmd,(hd->cylinderHighReg*256+hd->cylinderLowReg),hd->devHeadReg,hd->sectorNumReg,hd->sectorCountReg);
            #endif
            break;
        }
        default: { // all others
            #ifdef DEBUG
            printf("Cmd %d, Cylinder %d, Head %d, Sector %d Count %d\n",cmd,(hd->cylinderHighReg*256+hd->cylinderLowReg),hd->devHeadReg,hd->sectorNumReg,hd->sectorCountReg);
            #endif
            setError(hd, 0x04);
        }
    }
}

void harddiskIdeReset(HarddiskIde* hd)
{
    hd->errorReg = 0x01;
    hd->sectorCountReg = 0x01;
    hd->sectorNumReg = 0x01;
    hd->cylinderLowReg = 0x00;
    hd->cylinderHighReg = 0x00;
    hd->devHeadReg = 0x00;
    hd->statusReg = STATUS_DSC | STATUS_DRDY;
    hd->featureReg = 0x00;
    hd->transferRead = 0;
    hd->transferWrite = 0;
}

UInt16 harddiskIdeRead(HarddiskIde* hd)
{
    UInt16 value;

    if (!hd->transferRead || !diskPresent(hd->diskId)) {
        return 0x7f7f;
    }

    value  = hd->sectorData[hd->sectorDataOffset++];
    value |= hd->sectorData[hd->sectorDataOffset++] << 8;
    if (--hd->transferCount == 0) {
        hd->transferRead = 0;
        hd->statusReg &= ~STATUS_DRQ;
    }
    return value;
}

UInt16 harddiskIdePeek(HarddiskIde* hd)
{
    UInt16 value;

    if (!hd->transferRead || !diskPresent(hd->diskId)) {
        return 0x7f7f;
    }

    value  = hd->sectorData[hd->sectorDataOffset];
    value |= hd->sectorData[hd->sectorDataOffset + 1] << 8;

    return value;
}

void harddiskIdeWrite(HarddiskIde* hd, UInt16 value)
{
    if (!hd->transferWrite || !diskPresent(hd->diskId)) {
        return;
    }

    hd->sectorData[hd->sectorDataOffset++] = value & 0xff;
    hd->sectorData[hd->sectorDataOffset++] = value >> 8;
    hd->transferCount--;
    if ((hd->transferCount & 255) == 0) {
        if (!diskWriteSector(hd->diskId, hd->sectorData, hd->sectorNumReg, hd->devHeadReg, (hd->cylinderHighReg*256+hd->cylinderLowReg), 0)) {
            setError(hd, 0x44);
            hd->transferWrite = 0;
            return;
        }
		hd->sectorNumReg++;
		if (hd->sectorNumReg > (diskGetSectorsPerTrack(hd->diskId) - 1)) {
			hd->sectorNumReg = hd->sectorNumReg - diskGetSectorsPerTrack(hd->diskId);
			hd->devHeadReg++;
		}
		if (hd->devHeadReg > (diskGetSides(hd->diskId) - 1)) {
			hd->devHeadReg = hd->devHeadReg - (diskGetSides(hd->diskId) - 1);
			if (hd->cylinderLowReg = 255) {
				hd->cylinderLowReg = hd->cylinderLowReg - (256 - 1);
				hd->cylinderHighReg++;
			}
			else {
				hd->cylinderLowReg++;
			}
		}
        hd->sectorDataOffset = 0;
	}
    if (hd->transferCount == 0) {
        hd->transferWrite = 0;
        hd->statusReg &= ~STATUS_DRQ;
    }
}

UInt8 harddiskIdeReadRegister(HarddiskIde* hd, UInt8 reg)
{
    if (!diskPresent(hd->diskId)) {
        return 0x7f;
    }

    switch (reg) {
    case 1:
        return hd->errorReg;
    case 2: 
        return hd->sectorCountReg;
    case 3: 
        return hd->sectorNumReg;
    case 4: 
        return hd->cylinderLowReg;
    case 5: 
        return hd->cylinderHighReg;
    case 6: 
        return hd->devHeadReg;
    case 7: 
        return hd->statusReg;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 15:
        return 0x7f;
    }

    return 0x7f;
}

UInt8 harddiskIdePeekRegister(HarddiskIde* hd, UInt8 reg)
{
    return harddiskIdeReadRegister(hd, reg);
}

void harddiskIdeWriteRegister(HarddiskIde* hd, UInt8 reg, UInt8 value)
{
    if (!diskPresent(hd->diskId)) {
        return;
    }
    switch (reg) {
    case 1:
        hd->featureReg = value;
        break;
    case 2:
        hd->sectorCountReg = value;
        break;
    case 3:
        hd->sectorNumReg = value;
        break;
    case 4:
        hd->cylinderLowReg = value;
        break;
    case 5:
        hd->cylinderHighReg = value;
        break;
    case 6:
        hd->devHeadReg = value;
        break;
    case 7:
        executeCommand(hd, value);
        break;
    }
}

//void harddiskIdeLoadState(HarddiskIde* ide)
//{
//    SaveState* state = saveStateOpenForRead("harddiskIde");
//
//    ide->errorReg               = (UInt8)saveStateGet(state, "errorReg",        0);
//    ide->sectorCountReg         = (UInt8)saveStateGet(state, "sectorCountReg",  0);
//    ide->sectorNumReg           = (UInt8)saveStateGet(state, "sectorNumReg",    0);
//    ide->cylinderLowReg         = (UInt8)saveStateGet(state, "cylinderLowReg",  0);
//    ide->cylinderHighReg        = (UInt8)saveStateGet(state, "cylinderHighReg", 0);
//    ide->devHeadReg             = (UInt8)saveStateGet(state, "devHeadReg",      0);
//    ide->statusReg              = (UInt8)saveStateGet(state, "statusReg",       0);
//    ide->featureReg             = (UInt8)saveStateGet(state, "featureReg",      0);
//    ide->transferRead           = saveStateGet(state, "transferRead",           0);
//    ide->transferWrite          = saveStateGet(state, "transferWrite",          0);
//    ide->transferCount          = saveStateGet(state, "transferCount",          0);
//    ide->transferSectorNumber   = saveStateGet(state, "transferSectorNumber",   0);
//
//    saveStateClose(state);
//}
//
//void harddiskIdeSaveState(HarddiskIde* ide)
//{
//    SaveState* state = saveStateOpenForWrite("harddiskIde");
//
//    saveStateSet(state, "errorReg",               ide->errorReg);
//    saveStateSet(state, "sectorCountReg",         ide->sectorCountReg);
//    saveStateSet(state, "sectorNumReg",           ide->sectorNumReg);
//    saveStateSet(state, "cylinderLowReg",         ide->cylinderLowReg);
//    saveStateSet(state, "cylinderHighReg",        ide->cylinderHighReg);
//    saveStateSet(state, "devHeadReg",             ide->devHeadReg);
//    saveStateSet(state, "statusReg",              ide->statusReg);
//    saveStateSet(state, "featureReg",             ide->featureReg);
//    saveStateSet(state, "transferRead",           ide->transferRead);
//    saveStateSet(state, "transferWrite",          ide->transferWrite);
//    saveStateSet(state, "transferCount",          ide->transferCount);
//    saveStateSet(state, "transferSectorNumber",   ide->transferSectorNumber);
//
//    saveStateClose(state);
//}

HarddiskIde* harddiskIdeCreate(int diskId)
{
    HarddiskIde* hd = malloc(sizeof(HarddiskIde));

    hd->diskId = diskId;
    hd->transferRead = 0;
    hd->transferWrite = 0;

    harddiskIdeReset(hd);

    return hd;
}

void harddiskIdeDestroy(HarddiskIde* hd)
{
    free(hd);
}
