# if ! defined( IpAccessControlH )
#	define IpAccessControlH	/* only include me once */

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

#ifndef MemoryPoolH
#include "MemoryPool.h"
#endif

#ifndef PoolListH
#include "PoolList.h"
#endif

#define INIT_IP_ACCESS_OCTET_POOL_SIZE    10000
#define INIT_IP_ACCESS_REC_POOL_SIZE      1000
#define MAX_IP_ACCESS_OCTET_HASH_INDEX    16
#define INIT_IP_ACCESS_BLK_COUNT          1000
#define IP_ACCESS_REC_FREE_SESSION        100

typedef unsigned int (*TOnGetTimeMarkerCB)();

typedef struct {
	bool               isAccessLock;
	unsigned int       LastAccessTime;
	unsigned int       IpAddress;
	unsigned int       AccessCount;
	unsigned int       RejectCount;
    POOL_RECORD_STRUCT *BlkPoolPtr;
    ObjPoolListTask    *ObjPtr;
} IP_ACCESS_INFO;

typedef struct {
	IP_ACCESS_INFO     *IpAccessPtr;
	void               *HashOctetHopPtr;    
	POOL_RECORD_STRUCT *IpAccessHashRecordPtr;
} IP_ACCESS_HASH_RECORD;

typedef struct {
   unsigned int          UsedOctetCount;
   unsigned int          ParentHashOctetIndex;
   void                  *ParentHashOctetPtr;
   IP_ACCESS_HASH_RECORD *Record;
   POOL_RECORD_STRUCT    *IpAccessHashRecordPtr;
   void                  *HashOctetHop[MAX_IP_ACCESS_OCTET_HASH_INDEX];
} IP_ACCESS_HASH_OCTET_HOP;

void IpAccessControlImit(unsigned int Treshold, unsigned int LockTime, 
	unsigned int FreeTime, TOnGetTimeMarkerCB TimeMarkerCB);
void IpAccessControlClose();
bool CheckIpAccess(unsigned int IpAddress);
void OnActiveIpAccessListCheckTmrExp();
void OnIpAccessCountClearTmrExp();
//---------------------------------------------------------------------------
#endif  /* if ! defined( IpAccessControlH ) */
