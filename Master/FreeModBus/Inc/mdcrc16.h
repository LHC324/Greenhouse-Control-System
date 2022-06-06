#ifndef __MDCRC16_H__
#define __MDCRC16_H__

#include "mdtype.h"

#if (USING_TABLE_CRC)
mdExport mdU16 mdCrc16(mdU8 *pucFrame, mdU32 usLen);
#else
mdExport mdU16 Get_Crc16(mdU8 *ptr, mdU16 length, mdU16 init_dat);
#define mdCrc16(__ptr, __len)(Get_Crc16(__ptr, __len, 0xFFFF))
#endif

#endif
