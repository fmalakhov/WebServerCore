# if ! defined( StatsDataBaseH )
#	define StatsDataBaseH	/* only include me once */

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

#ifndef SysWebFunctionH
#include "SysWebFunction.h"
#endif

#define MAX_LEN_STATS_NAME      64
#define MAX_LEN_STATS_DESCR     256
#define MAX_LEN_STATS_BASE_LINE 512

typedef struct {
	unsigned int   StatsId;
    unsigned char  StatsType;
    unsigned char  StatsStyle;
    unsigned char  StatsGroup;
    unsigned short StatsCode;  
#ifdef WIN32         
	SYSTEMTIME     LastUpdateDate;
#else        
	struct tm      LastUpdateDate;
#endif
	ObjListTask    *ObjPtr;
    char           StatsName[MAX_LEN_STATS_NAME+1];
    char           StatsDescr[MAX_LEN_STATS_DESCR+1];
} DEV_STATS_INFO;

typedef struct {
	unsigned int MinSPGroup;
	unsigned int MaxSPGroup;
	unsigned int DefSPGroup;
	unsigned int MinSGStyle;
	unsigned int MaxSGStyle;
	unsigned int DefSGStyle;
	unsigned int NewStatsIndex;
} STATS_BASE_INFO;

void StatsDBLoad(unsigned int MinSPGroup, unsigned int MaxSPGroup,
	unsigned int DefSPGroup, unsigned int MinSGStyle,
	unsigned int MaxSGStyle, unsigned int DefSGStyle);
void StatsDbSave();
void StatsDBClear();
void StatsDbRemItem(DEV_STATS_INFO *RemStatsPtr);
DEV_STATS_INFO* StatsDbAddItem();
void SetStatsLastUpdateTime(DEV_STATS_INFO *StatsPtr);
DEV_STATS_INFO* GetStatsByStatsId(unsigned int StatsId);
DEV_STATS_INFO* GetStatsByStatsCode(unsigned short StatsCode);

//---------------------------------------------------------------------------
#endif  /* if ! defined( StatsDataBaseH ) */
