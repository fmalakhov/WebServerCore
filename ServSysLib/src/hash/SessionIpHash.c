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

#ifdef _LINUX_X86_
#include <dirent.h>
#endif
#include <sys/stat.h>
#include "CommonPlatform.h"
#include "SessionIpHash.h"
#include "SysWebFunction.h"

unsigned int SessIpHashEntityCount = 0;
unsigned int SessIpRecInHashCount = 0;
SESSION_IP_HASH_OCTET_HOP SessionIpHashHop;
//---------------------------------------------------------------------------
void InitSessionIpHash(SESSION_IP_HASH_OCTET_HOP *NameHop)
{
    unsigned int i;
	
	for(i=0;i < MAX_OCTET_HASH_INDEX;i++) 
	   NameHop->HashOctetHop[i] = NULL;
	NameHop->ParentHashOctetPtr = NULL;
    NameHop->Record = NULL;
    NameHop->UsedOctetCount = 0;
    NameHop->ParentHashOctetIndex = 0;	
}
//---------------------------------------------------------------------------
bool AddSessionIpHash(SESSION_IP_HASH_OCTET_HOP *RootNameHop, unsigned long SessionIp)
{
    bool Result = true;
	unsigned long IpOctetMask = 0x0f;
    unsigned int OctetIndex, HopIndex;
	SESSION_IP_HASH_OCTET_HOP *NewHashHopOctetPtr = NULL;
	SESSION_IP_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	SESSION_IP_HASH_RECORD   *NewRecordPtr = NULL;
	
	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 8;OctetIndex++)
	{
	   HopIndex = (unsigned int)((SessionIp & IpOctetMask) >> (OctetIndex << 2));
	   if (!SelHashHopOctetPtr->HashOctetHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopOctetPtr = (SESSION_IP_HASH_OCTET_HOP*)AllocateMemory(sizeof(SESSION_IP_HASH_OCTET_HOP));
		   if (!NewHashHopOctetPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitSessionIpHash(NewHashHopOctetPtr);
		   SelHashHopOctetPtr->HashOctetHop[HopIndex] = NewHashHopOctetPtr;
		   SelHashHopOctetPtr->UsedOctetCount++;
		   NewHashHopOctetPtr->ParentHashOctetPtr = (void*)SelHashHopOctetPtr;
		   NewHashHopOctetPtr->ParentHashOctetIndex = HopIndex;
		   SelHashHopOctetPtr = NewHashHopOctetPtr;
		   SessIpHashEntityCount++;
	   }
	   else
	   {
	       /* Existing char in hash for this hop is detected */
		   SelHashHopOctetPtr = SelHashHopOctetPtr->HashOctetHop[HopIndex];
	   }
       IpOctetMask <<= 4;
	}
	if (Result)
	{
		if (SelHashHopOctetPtr->Record)
		{
			printf("In cashe of session IPs %08x is already present\n", (unsigned int)SessionIp);
            return false;
		}
	    /* Sesion Ip Addr hash fillout */
		NewRecordPtr = (SESSION_IP_HASH_RECORD*)AllocateMemory(sizeof(SESSION_IP_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopOctetPtr->Record = NewRecordPtr;
			NewRecordPtr->HashOctetHopPtr = SelHashHopOctetPtr;
			NewRecordPtr->SessionCount = 1;
			SessIpRecInHashCount++;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
SESSION_IP_HASH_RECORD* FindSessionIpHash(SESSION_IP_HASH_OCTET_HOP *RootNameHop,
										  unsigned long SessionIp)
{
    bool Result = true;
	unsigned long IpOctetMask = 0x0f;
    unsigned int OctetIndex, HopIndex;
	SESSION_IP_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	SESSION_IP_HASH_RECORD   *SelRecordPtr = NULL;

	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 8;OctetIndex++)
	{
	    HopIndex = (unsigned int)((SessionIp & IpOctetMask) >> (OctetIndex << 2));
	    if (!SelHashHopOctetPtr->HashOctetHop[HopIndex])
	    {
	        Result = false;
            break;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopOctetPtr = SelHashHopOctetPtr->HashOctetHop[HopIndex];
	    }
		IpOctetMask <<= 4;
	}
	if (!Result) return NULL;
	return SelHashHopOctetPtr->Record;
}
//---------------------------------------------------------------------------
bool RemSessionIpHash(SESSION_IP_HASH_OCTET_HOP *RootNameHop, unsigned long SessionIp)
{
    SESSION_IP_HASH_RECORD *SessionIpRecPtr = NULL;
	SESSION_IP_HASH_OCTET_HOP *ParentHashHopOctetPtr = NULL;
	SESSION_IP_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	
    SessionIpRecPtr = FindSessionIpHash(RootNameHop, SessionIp);
	if (!SessionIpRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopOctetPtr = (SESSION_IP_HASH_OCTET_HOP*)SessionIpRecPtr->HashOctetHopPtr;
	if (SessIpRecInHashCount > 0) SessIpRecInHashCount--;
	FreeMemory(SessionIpRecPtr);
	SelHashHopOctetPtr->Record = NULL;
	while(!SelHashHopOctetPtr || (SelHashHopOctetPtr != RootNameHop))
	{
	    if (!SelHashHopOctetPtr->UsedOctetCount)
	    {
	        ParentHashHopOctetPtr = (SESSION_IP_HASH_OCTET_HOP*)SelHashHopOctetPtr->ParentHashOctetPtr;
		    ParentHashHopOctetPtr->HashOctetHop[SelHashHopOctetPtr->ParentHashOctetIndex] = NULL;
		    FreeMemory(SelHashHopOctetPtr);
			SessIpHashEntityCount--;
		    ParentHashHopOctetPtr->UsedOctetCount--;
			SelHashHopOctetPtr = ParentHashHopOctetPtr;
	    }
	    else
	    {
	        break;
	    }
	}
	return true;
}
//---------------------------------------------------------------------------
void CloseSessionIpHash(SESSION_IP_HASH_OCTET_HOP *RootNameHop)
{
	CloseSessionIpHashHop(RootNameHop);
}
//---------------------------------------------------------------------------
void CloseSessionIpHashHop(SESSION_IP_HASH_OCTET_HOP *SelHashHopOctetPtr)
{
	unsigned int index;
	SESSION_IP_HASH_OCTET_HOP *NextHashHopOctetPtr = NULL;

	if (SelHashHopOctetPtr->UsedOctetCount)
	{
		for(index=0;index < MAX_OCTET_HASH_INDEX;index++)
		{
			if (SelHashHopOctetPtr->HashOctetHop[index])
			{
                NextHashHopOctetPtr = SelHashHopOctetPtr->HashOctetHop[index];
				if (!NextHashHopOctetPtr->UsedOctetCount)
				{
					if (NextHashHopOctetPtr->Record)
                        FreeMemory(NextHashHopOctetPtr->Record);
					FreeMemory(NextHashHopOctetPtr);
					SelHashHopOctetPtr->HashOctetHop[index] = NULL;
					SelHashHopOctetPtr->UsedOctetCount--;
					if (!SelHashHopOctetPtr->UsedOctetCount) break;
				}
				else
				{
                    CloseSessionIpHashHop(NextHashHopOctetPtr);
					if (!NextHashHopOctetPtr->UsedOctetCount)
					{
					    if (NextHashHopOctetPtr->Record)
                            FreeMemory(NextHashHopOctetPtr->Record);
					    FreeMemory(NextHashHopOctetPtr);
					    SelHashHopOctetPtr->HashOctetHop[index] = NULL;
					    SelHashHopOctetPtr->UsedOctetCount--;
					    if (!SelHashHopOctetPtr->UsedOctetCount) break;
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
