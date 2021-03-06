# if ! defined( StatsCodeHashH )
#	define StatsCodeHashH	/* only include me once */

/*
Copyright (c) 2012-2017 MFBS, Fedor Malakhov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef CommonPlatformH
#include "CommonPlatform.h"
#endif

#ifndef vistypesH
#include "vistypes.h"
#endif

#ifndef SysLibToolH
#include "SysLibTool.h"
#endif

#define MAX_STATS_CODE_OCTET_HASH_INDEX   16

typedef struct {
    unsigned short DevId;
    unsigned char  ClientType;
    unsigned int   ClientId;
    unsigned short StatsCode;
    unsigned int   StatsCount;  /* Summary count of increment stats calls */
    unsigned int   UpdateCount; /* Number of calls for stats incriment */
    unsigned int   GeneralRegCount;
    ObjListTask    *StatsListObjPtr;
} STATS_COLLECT_RECORD;

typedef struct {
    STATS_COLLECT_RECORD *StatsRecordPtr;
	void         *HashOctetHopPtr;    
} STATS_CODE_HASH_RECORD;

typedef struct {
   unsigned int     UsedOctetCount;
   unsigned int     ParentHashOctetIndex;
   void             *ParentHashOctetPtr;
   STATS_CODE_HASH_RECORD *Record;
   void             *HashOctetHop[MAX_STATS_CODE_OCTET_HASH_INDEX];
} STATS_CODE_HASH_OCTET_HOP;

void InitStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *NameHop);
void CloseStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *RootNameHop);
bool AddStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *RootNameHop, STATS_COLLECT_RECORD* RecordPtr);
STATS_CODE_HASH_RECORD* FindStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *RootNameHop, unsigned short StatsCode);
bool RemStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *RootNameHop, unsigned short StatsCode);
void CloseStatsCodeHashHop(STATS_CODE_HASH_OCTET_HOP *SelHashHopCharPtr);
//---------------------------------------------------------------------------
#endif  /* if ! defined( StatsCodeHashH ) */
