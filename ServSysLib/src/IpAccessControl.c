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

#include <time.h>
#include "IpAccessControl.h"
#include "ThrReportMen.h"

static IP_ACCESS_HASH_OCTET_HOP gIpAccessHash;
static POOL_RECORD_BASE gIpAccessHashOctetPool;
static POOL_RECORD_BASE gIpAccessHashRecordPool;

static PoolListItsTask IpAccessRecordList;
static POOL_RECORD_BASE IpAccessBlockPool;
static unsigned int LastAccessTimeMarker;
static unsigned int IpAccessLockTime;
static unsigned int NoIpAccessTime;
static unsigned int AttackDetectTreshold;
static TOnGetTimeMarkerCB OnGetTimeMarkerCB = NULL;

#ifdef WIN32
static HANDLE IpAccessSem;
#else
static pthread_mutex_t IpAccessSem;
#endif

static void InitIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *NameHop);
static void CloseIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *RootNameHop);
static bool AddIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *RootNameHop, IP_ACCESS_INFO* IpAccessPtr);
static IP_ACCESS_HASH_RECORD* FindIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *RootNameHop, unsigned int IpAddr);
static bool RemIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *RootNameHop, unsigned int IpAddr);
static void CloseIpAccessHashHop(IP_ACCESS_HASH_OCTET_HOP *SelHashHopCharPtr);
static void CreateIpAccessHash();
static void DestroyIpAccessHash();
static bool AddIpAccessControl(unsigned int IpAddress);
static void LastIpAccessTimeSet();
//---------------------------------------------------------------------------
static void CreateIpAccessHash()
{
	InitIpAccessHash(&gIpAccessHash);
	CreatePool(&gIpAccessHashOctetPool, INIT_IP_ACCESS_OCTET_POOL_SIZE, sizeof(IP_ACCESS_HASH_OCTET_HOP));
	CreatePool(&gIpAccessHashRecordPool, INIT_IP_ACCESS_REC_POOL_SIZE, sizeof(IP_ACCESS_HASH_RECORD));
}
//---------------------------------------------------------------------------
static void DestroyIpAccessHash()
{
	printf("IP access hash clean...\n");
	CloseIpAccessHash(&gIpAccessHash);
	DestroyPool(&gIpAccessHashRecordPool);
	DestroyPool(&gIpAccessHashOctetPool);
}
//---------------------------------------------------------------------------
static void InitIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *NameHop)
{
    unsigned int i;
	
	for(i=0;i < MAX_IP_ACCESS_OCTET_HASH_INDEX;i++) 
	   NameHop->HashOctetHop[i] = NULL;
	NameHop->ParentHashOctetPtr = NULL;
    NameHop->Record = NULL;
    NameHop->UsedOctetCount = 0;
    NameHop->ParentHashOctetIndex = 0;	
}
//---------------------------------------------------------------------------
static bool AddIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *RootNameHop, IP_ACCESS_INFO* IpAccessPtr)
{
    bool                     Result = true; 
	unsigned int             IpOctetMask = 0x0f;
    unsigned int             OctetIndex, HopIndex;
	IP_ACCESS_HASH_OCTET_HOP *NewHashHopOctetPtr = NULL;
	IP_ACCESS_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	IP_ACCESS_HASH_RECORD    *NewRecordPtr = NULL;
	POOL_RECORD_STRUCT       *IpAccessHashRecordPtr = NULL;
 
	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 8;OctetIndex++)
	{
		HopIndex = (unsigned int)((IpAccessPtr->IpAddress & IpOctetMask) >> (OctetIndex << 2));
	    if (!SelHashHopOctetPtr->HashOctetHop[HopIndex])
	    {
	        /* New char in hash for this hop is detected */
		    IpAccessHashRecordPtr = GetBuffer(&gIpAccessHashOctetPool);
		    if (!IpAccessHashRecordPtr)
		    {
		        Result = false;
		        break;
		    }
            NewHashHopOctetPtr = (IP_ACCESS_HASH_OCTET_HOP*)IpAccessHashRecordPtr->DataPtr;
		    if (!NewHashHopOctetPtr)
		    {
		        Result = false;
		        break;
		    }
		    InitIpAccessHash(NewHashHopOctetPtr);
			NewHashHopOctetPtr->IpAccessHashRecordPtr = IpAccessHashRecordPtr;
		    SelHashHopOctetPtr->HashOctetHop[HopIndex] = NewHashHopOctetPtr;
		    SelHashHopOctetPtr->UsedOctetCount++;
		    NewHashHopOctetPtr->ParentHashOctetPtr = (void*)SelHashHopOctetPtr;
		    NewHashHopOctetPtr->ParentHashOctetIndex = HopIndex;
		    SelHashHopOctetPtr = NewHashHopOctetPtr;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopOctetPtr = (IP_ACCESS_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[HopIndex];
	    }
		IpOctetMask <<= 4;
	}
	
	if (Result)
	{
		if (SelHashHopOctetPtr->Record) return false;   
		IpAccessHashRecordPtr = GetBuffer(&gIpAccessHashRecordPool);
		if (!IpAccessHashRecordPtr) Result = false;
		else
		{
            NewRecordPtr = (IP_ACCESS_HASH_RECORD*)IpAccessHashRecordPtr->DataPtr;
		    NewRecordPtr->IpAccessHashRecordPtr = IpAccessHashRecordPtr;
		    SelHashHopOctetPtr->Record = NewRecordPtr;
            NewRecordPtr->HashOctetHopPtr = SelHashHopOctetPtr;
			NewRecordPtr->IpAccessPtr = IpAccessPtr;           
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
static IP_ACCESS_HASH_RECORD* FindIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *RootNameHop, unsigned int IpAddr)
{
    bool Result = true;
	unsigned int IpOctetMask = 0x0f;
    register unsigned int  OctetIndex, HopIndex;
	register IP_ACCESS_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
        
	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 8;OctetIndex++)
	{
		HopIndex = (unsigned int)((IpAddr & IpOctetMask) >> (OctetIndex << 2));
	    if (!SelHashHopOctetPtr->HashOctetHop[HopIndex])
	    {
	        Result = false;
            break;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopOctetPtr = (IP_ACCESS_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[HopIndex];
	    }
		IpOctetMask <<= 4;
	}
	if (!Result) return NULL;
	return SelHashHopOctetPtr->Record;
}
//---------------------------------------------------------------------------
static bool RemIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *RootNameHop, unsigned int IpAddr)
{
    IP_ACCESS_HASH_RECORD    *IpAccessRecPtr = NULL;
	IP_ACCESS_HASH_OCTET_HOP *ParentHashHopOctetPtr = NULL;
	IP_ACCESS_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
    
    IpAccessRecPtr = FindIpAccessHash(RootNameHop, IpAddr);
	if (!IpAccessRecPtr)
	{
	    /* Call no found in hash */
	    return false;
	}
	SelHashHopOctetPtr = (IP_ACCESS_HASH_OCTET_HOP*)IpAccessRecPtr->HashOctetHopPtr;
	FreeBuffer(&gIpAccessHashRecordPool, IpAccessRecPtr->IpAccessHashRecordPtr);
	SelHashHopOctetPtr->Record = NULL;
	while(!SelHashHopOctetPtr || (SelHashHopOctetPtr != RootNameHop))
	{
	    if (!SelHashHopOctetPtr->UsedOctetCount)
	    {
	        ParentHashHopOctetPtr = (IP_ACCESS_HASH_OCTET_HOP*)SelHashHopOctetPtr->ParentHashOctetPtr;
			if (ParentHashHopOctetPtr)
			{
				ParentHashHopOctetPtr->HashOctetHop[SelHashHopOctetPtr->ParentHashOctetIndex] = NULL;
				FreeBuffer(&gIpAccessHashOctetPool, SelHashHopOctetPtr->IpAccessHashRecordPtr);
				if (ParentHashHopOctetPtr->UsedOctetCount > 0)
					ParentHashHopOctetPtr->UsedOctetCount--;
				SelHashHopOctetPtr = ParentHashHopOctetPtr;
			}
			else
			{
				FreeBuffer(&gIpAccessHashOctetPool, SelHashHopOctetPtr->IpAccessHashRecordPtr);
				break;
			}
	    }
	    else
	    {
	        break;
	    }
	}
	return true;
}
//---------------------------------------------------------------------------
static void CloseIpAccessHash(IP_ACCESS_HASH_OCTET_HOP *RootNameHop)
{
	CloseIpAccessHashHop(RootNameHop);
}
//---------------------------------------------------------------------------
static void CloseIpAccessHashHop(IP_ACCESS_HASH_OCTET_HOP *SelHashHopOctetPtr)
{
	unsigned int index;
    IP_ACCESS_HASH_RECORD    *RecordPtr = NULL;
	IP_ACCESS_HASH_OCTET_HOP *NextHashHopOctetPtr = NULL;

	if (SelHashHopOctetPtr->UsedOctetCount)
	{
		for(index=0;index < MAX_IP_ACCESS_OCTET_HASH_INDEX;index++)
		{
			if (SelHashHopOctetPtr->HashOctetHop[index])
			{
                NextHashHopOctetPtr = (IP_ACCESS_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[index];
				if (!NextHashHopOctetPtr->UsedOctetCount)
				{
					if (NextHashHopOctetPtr->Record)
                    {     
                        if (NextHashHopOctetPtr->Record)
						    FreeBuffer(&gIpAccessHashRecordPool, NextHashHopOctetPtr->Record->IpAccessHashRecordPtr);
                        NextHashHopOctetPtr->Record = NULL;
                    }
                    FreeBuffer(&gIpAccessHashOctetPool, SelHashHopOctetPtr->IpAccessHashRecordPtr);
					SelHashHopOctetPtr->HashOctetHop[index] = NULL;
					if (SelHashHopOctetPtr->UsedOctetCount > 0)
					    SelHashHopOctetPtr->UsedOctetCount--;
					if (!SelHashHopOctetPtr->UsedOctetCount) break;
				}
				else
				{
                    CloseIpAccessHashHop(NextHashHopOctetPtr);
					if (!NextHashHopOctetPtr->UsedOctetCount)
					{
					    if (NextHashHopOctetPtr->Record)
                        {                     
                            if (NextHashHopOctetPtr->Record) 
							    FreeBuffer(&gIpAccessHashRecordPool, NextHashHopOctetPtr->Record->IpAccessHashRecordPtr);
                            NextHashHopOctetPtr->Record = NULL;
                        }
					    FreeBuffer(&gIpAccessHashOctetPool, SelHashHopOctetPtr->IpAccessHashRecordPtr);
					    SelHashHopOctetPtr->HashOctetHop[index] = NULL;
						if (SelHashHopOctetPtr->UsedOctetCount > 0)
					        SelHashHopOctetPtr->UsedOctetCount--;
					    if (!SelHashHopOctetPtr->UsedOctetCount) break;
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void IpAccessControlImit(unsigned int Treshold, unsigned int LockTime, 
	unsigned int FreeTime, TOnGetTimeMarkerCB TimeMarkerCB)
{
	AttackDetectTreshold = Treshold;
    IpAccessLockTime = LockTime;
    NoIpAccessTime = FreeTime;
	OnGetTimeMarkerCB = TimeMarkerCB;
	CreateIpAccessHash();
    PoolListInit(&IpAccessRecordList, INIT_IP_ACCESS_BLK_COUNT);
    CreatePool(&IpAccessBlockPool, INIT_IP_ACCESS_BLK_COUNT, sizeof(IP_ACCESS_INFO));
#ifdef WIN32
    IpAccessSem = CreateMutex(NULL, FALSE, NULL);
    if (IpAccessSem == NULL) 
    {
        printf("Create IP control access mutex error: %d\r\n", GetLastError());
    }
#else
    pthread_mutex_init(&IpAccessSem, NULL);
#endif
    LastIpAccessTimeSet();
}
//---------------------------------------------------------------------------
void IpAccessControlClose()
{
	IP_ACCESS_INFO  *IpAccRecPtr = NULL;
    ObjPoolListTask *PointTask = NULL;
	
    while (IpAccessRecordList.Count)
    {
	    PointTask = (ObjPoolListTask*)IpAccessRecordList.FistTask;
	    IpAccRecPtr = (IP_ACCESS_INFO*)PointTask->UsedTask;
		RemIpAccessHash(&gIpAccessHash, IpAccRecPtr->IpAddress);
        RemPoolStructList(&IpAccessRecordList, PointTask);
        FreeBuffer(&IpAccessBlockPool, IpAccRecPtr->BlkPoolPtr);
    }
    DestroyPoolListStructs(&IpAccessRecordList);
    DestroyPool(&IpAccessBlockPool);
	DestroyIpAccessHash();
#ifdef WIN32
	CloseHandle(IpAccessSem);
#else
	pthread_mutex_destroy(&IpAccessSem);
#endif
}
//---------------------------------------------------------------------------
static bool AddIpAccessControl(unsigned int IpAddress)
{
	IP_ACCESS_INFO     *IpAccRecPtr = NULL;
    POOL_RECORD_STRUCT *ObjTaskPtr = NULL;
	
    ObjTaskPtr = GetBuffer(&IpAccessBlockPool);
    if (!ObjTaskPtr)
    {
		printf("Fail to get IP access block\n");
        return false;				
    }
    else
    {
        IpAccRecPtr = (IP_ACCESS_INFO*)ObjTaskPtr->DataPtr;
		IpAccRecPtr->BlkPoolPtr = ObjTaskPtr;
		IpAccRecPtr->IpAddress  = IpAddress;
		IpAccRecPtr->isAccessLock = false;
		IpAccRecPtr->LastAccessTime = LastAccessTimeMarker;
		IpAccRecPtr->AccessCount = 1;
		IpAccRecPtr->RejectCount = 0;
		IpAccRecPtr->ObjPtr = AddPoolStructListObj(&IpAccessRecordList, IpAccRecPtr);	
		AddIpAccessHash(&gIpAccessHash, IpAccRecPtr);
		return true;
	}
}	
//---------------------------------------------------------------------------
bool CheckIpAccess(unsigned int IpAddress)
{
	IP_ACCESS_INFO        *IpAccRecPtr = NULL;
	IP_ACCESS_HASH_RECORD *IpAccHashRecPtr = NULL;
#ifdef WIN32
	DWORD                 rc;

    if ((rc = WaitForSingleObject(IpAccessSem, INFINITE)) == WAIT_FAILED)
	{
        return true;
	}
#else
    pthread_mutex_lock(&IpAccessSem);
#endif	
    IpAccHashRecPtr = FindIpAccessHash(&gIpAccessHash, IpAddress);
	if (IpAccHashRecPtr)
	{
#ifdef WIN32
        if (!ReleaseMutex(IpAccessSem)) 
            printf("Fail to release mutex (CheckIpAccess()\r\n");
#else
        pthread_mutex_unlock(&IpAccessSem);
#endif
	    IpAccRecPtr = IpAccHashRecPtr->IpAccessPtr;
		if (IpAccRecPtr->isAccessLock)
		{
			IpAccRecPtr->RejectCount++;
			return false;
		}
		
		IpAccRecPtr->LastAccessTime = LastAccessTimeMarker;
		if (IpAccRecPtr->AccessCount < AttackDetectTreshold)
		{
		    IpAccRecPtr->AccessCount++;
		    return true;
	    }
		else
		{
			IpAccRecPtr->isAccessLock = true;
			IpAccRecPtr->RejectCount = 1;
            EventLogPrint(NULL, "DDos attack is detected from IP: (%u.%u.%u.%u)\r\n",
		        (unsigned int)(IpAccRecPtr->IpAddress & 0x000000ff), 
				(unsigned int)((IpAccRecPtr->IpAddress & 0x0000ff00) >> 8), 
			    (unsigned int)((IpAccRecPtr->IpAddress & 0x00ff0000) >> 16),
				(unsigned int)((IpAccRecPtr->IpAddress & 0xff000000) >> 24));
			return false;
		}
    }
	else
	{
		AddIpAccessControl(IpAddress);
	}
#ifdef WIN32
    if (!ReleaseMutex(IpAccessSem)) 
        printf("Fail to release mutex (CheckIpAccess()\r\n");
#else
    pthread_mutex_unlock(&IpAccessSem);
#endif
    return true;
}
//---------------------------------------------------------------------------
static void LastIpAccessTimeSet()
{
	if (OnGetTimeMarkerCB) LastAccessTimeMarker = (OnGetTimeMarkerCB)();
}
//---------------------------------------------------------------------------
/* This timer handler should be called every 30 seconds */
void OnActiveIpAccessListCheckTmrExp()
{
	unsigned int    DeltaTime;
	unsigned int    RemCount = 0;
	IP_ACCESS_INFO  *IpAccRecPtr = NULL;
    ObjPoolListTask *PointTask = NULL;
	ObjPoolListTask *NextPointTask = NULL;
#ifdef WIN32
	DWORD           rc;
	
    if ((rc = WaitForSingleObject(IpAccessSem, INFINITE)) == WAIT_FAILED) return;
#else
    pthread_mutex_lock(&IpAccessSem);
#endif		
	PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&IpAccessRecordList);
	while(PointTask)
	{
	    IpAccRecPtr = (IP_ACCESS_INFO*)PointTask->UsedTask;
		DeltaTime = LastAccessTimeMarker - IpAccRecPtr->LastAccessTime;
		if (IpAccRecPtr->isAccessLock)
		{
		    if (DeltaTime > IpAccessLockTime)
			{
                EventLogPrint(NULL, "DDos attack is cleared from IP: (%u.%u.%u.%u), RR: %u\r\n",
		            (unsigned int)(IpAccRecPtr->IpAddress & 0x000000ff), 
				    (unsigned int)((IpAccRecPtr->IpAddress & 0x0000ff00) >> 8), 
			        (unsigned int)((IpAccRecPtr->IpAddress & 0x00ff0000) >> 16),
				    (unsigned int)((IpAccRecPtr->IpAddress & 0xff000000) >> 24), IpAccRecPtr->RejectCount);
			    IpAccRecPtr->AccessCount = 0;
				IpAccRecPtr->RejectCount = 0;
				IpAccRecPtr->LastAccessTime = LastAccessTimeMarker;
				IpAccRecPtr->isAccessLock = false;
			}
		}
		else
		{
		    if ((DeltaTime > NoIpAccessTime) && (RemCount < IP_ACCESS_REC_FREE_SESSION))
			{
				RemIpAccessHash(&gIpAccessHash, IpAccRecPtr->IpAddress);
				NextPointTask = (ObjPoolListTask*)GetNextPoolObjectList(&IpAccessRecordList);
                RemPoolStructList(&IpAccessRecordList, PointTask);
                FreeBuffer(&IpAccessBlockPool, IpAccRecPtr->BlkPoolPtr);
				RemCount++;
				PointTask = NULL;
			}
	    }
		
		if (PointTask) PointTask = (ObjPoolListTask*)GetNextPoolObjectList(&IpAccessRecordList);
		else           PointTask = NextPointTask;
    }
#ifdef WIN32
    if (!ReleaseMutex(IpAccessSem)) 
        printf("Fail to release mutex (OnActiveIpAccessListCheckTmrExp()\r\n");
#else
    pthread_mutex_unlock(&IpAccessSem);
#endif
}
//---------------------------------------------------------------------------
/* This timer handler should be called every second */
void OnIpAccessCountClearTmrExp()
{
	IP_ACCESS_INFO  *IpAccRecPtr = NULL;
    ObjPoolListTask *PointTask = NULL;
#ifdef WIN32
	DWORD           rc;
#endif

	LastIpAccessTimeSet();
#ifdef WIN32
    if ((rc = WaitForSingleObject(IpAccessSem, INFINITE)) == WAIT_FAILED) return;
#else
    pthread_mutex_lock(&IpAccessSem);
#endif		
	PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&IpAccessRecordList);
	while(PointTask)
	{
	    IpAccRecPtr = (IP_ACCESS_INFO*)PointTask->UsedTask;
		if (!IpAccRecPtr->isAccessLock) IpAccRecPtr->AccessCount = 0;
		PointTask = (ObjPoolListTask*)GetNextPoolObjectList(&IpAccessRecordList);
    }
#ifdef WIN32
    if (!ReleaseMutex(IpAccessSem)) 
        printf("Fail to release mutex (OnIpAccessCountClearTmrExp()\r\n");
#else
    pthread_mutex_unlock(&IpAccessSem);
#endif
}
//---------------------------------------------------------------------------
