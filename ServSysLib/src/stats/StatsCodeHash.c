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

#include "StatsCodeHash.h"
//---------------------------------------------------------------------------
void InitStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *NameHop)
{
    unsigned int i;
	
	for(i=0;i < MAX_STATS_CODE_OCTET_HASH_INDEX;i++) 
	   NameHop->HashOctetHop[i] = NULL;
	NameHop->ParentHashOctetPtr = NULL;
    NameHop->Record = NULL;
    NameHop->UsedOctetCount = 0;
    NameHop->ParentHashOctetIndex = 0;	
}
//---------------------------------------------------------------------------
bool AddStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *RootNameHop, STATS_COLLECT_RECORD* RecordPtr)
{
    bool Result = true;
    unsigned char *ClientPtr;
    unsigned char Shift = 0x04;    
    unsigned int  OctetIndex, HopIndex;
	STATS_CODE_HASH_OCTET_HOP *NewHashHopOctetPtr = NULL;
	STATS_CODE_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	STATS_CODE_HASH_RECORD    *NewRecordPtr = NULL;
    unsigned char StatsBlk[3];

    ClientPtr = &StatsBlk[0];
    *ClientPtr++ = (unsigned char)((RecordPtr->StatsCode & 0xff00) >> 8);
    *ClientPtr = (unsigned char)(RecordPtr->StatsCode & 0xff);
	 
    ClientPtr = &StatsBlk[0];     
	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 4;OctetIndex++)
	{
        HopIndex = (unsigned int)((*ClientPtr >> Shift) & 0x0f);
	    if (!SelHashHopOctetPtr->HashOctetHop[HopIndex])
	    {
	        /* New char in hash for this hop is detected */
		    NewHashHopOctetPtr = (STATS_CODE_HASH_OCTET_HOP*)AllocateMemory(sizeof(STATS_CODE_HASH_OCTET_HOP));
		    if (!NewHashHopOctetPtr)
		    {
		        Result = false;
		        break;
		    }
		    InitStatsCodeHash(NewHashHopOctetPtr);
		    SelHashHopOctetPtr->HashOctetHop[HopIndex] = NewHashHopOctetPtr;
		    SelHashHopOctetPtr->UsedOctetCount++;
		    NewHashHopOctetPtr->ParentHashOctetPtr = (void*)SelHashHopOctetPtr;
		    NewHashHopOctetPtr->ParentHashOctetIndex = HopIndex;
		    SelHashHopOctetPtr = NewHashHopOctetPtr;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopOctetPtr = SelHashHopOctetPtr->HashOctetHop[HopIndex];
	    }
        if (Shift > 0)
        {
            Shift = 0x00;
        }
        else
        {
            Shift = 0x04;
            ClientPtr++;
        }
	}
	if (Result)
	{
		if (SelHashHopOctetPtr->Record) return false;
		NewRecordPtr = (STATS_CODE_HASH_RECORD*)AllocateMemory(sizeof(STATS_CODE_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopOctetPtr->Record = NewRecordPtr;
            NewRecordPtr->HashOctetHopPtr = SelHashHopOctetPtr;
            NewRecordPtr->StatsRecordPtr =  RecordPtr;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
STATS_CODE_HASH_RECORD* FindStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *RootNameHop,
    unsigned short StatsCode)
{
    bool Result = true;
    unsigned char Shift = 0x04;    
    unsigned char *ClientPtr;
    unsigned int  OctetIndex, HopIndex;
	STATS_CODE_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	STATS_CODE_HASH_RECORD   *SelRecordPtr = NULL;
    unsigned char StatsBlk[2];

    ClientPtr = &StatsBlk[0];
    *ClientPtr++ = (unsigned char)((StatsCode & 0xff00) >> 8);
    *ClientPtr = (unsigned char)(StatsCode & 0xff);
	 
    ClientPtr = &StatsBlk[0];     
	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 4;OctetIndex++)
	{
        HopIndex = (unsigned int)((*ClientPtr >> Shift) & 0x0f);
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
        if (Shift > 0)
        {
            Shift = 0x00;
        }
        else
        {
            Shift = 0x04;
            ClientPtr++;
        }        
	}
	if (!Result) return NULL;
	return SelHashHopOctetPtr->Record;
}
//---------------------------------------------------------------------------
bool RemStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *RootNameHop, 
         unsigned short StatsCode)
{
    STATS_CODE_HASH_RECORD    *StatsCodeRecPtr = NULL;
	STATS_CODE_HASH_OCTET_HOP *ParentHashHopOctetPtr = NULL;
	STATS_CODE_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;

    StatsCodeRecPtr = FindStatsCodeHash(RootNameHop, StatsCode);
	if (!StatsCodeRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopOctetPtr = (STATS_CODE_HASH_OCTET_HOP*)StatsCodeRecPtr->HashOctetHopPtr;   
	FreeMemory(StatsCodeRecPtr);
	SelHashHopOctetPtr->Record = NULL;
	while(!SelHashHopOctetPtr || (SelHashHopOctetPtr != RootNameHop))
	{
	    if (!SelHashHopOctetPtr->UsedOctetCount)
	    {
	        ParentHashHopOctetPtr = (STATS_CODE_HASH_OCTET_HOP*)SelHashHopOctetPtr->ParentHashOctetPtr;
		    ParentHashHopOctetPtr->HashOctetHop[SelHashHopOctetPtr->ParentHashOctetIndex] = NULL;
		    FreeMemory(SelHashHopOctetPtr);
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
void CloseStatsCodeHash(STATS_CODE_HASH_OCTET_HOP *RootNameHop)
{
	CloseStatsCodeHashHop(RootNameHop);
}
//---------------------------------------------------------------------------
void CloseStatsCodeHashHop(STATS_CODE_HASH_OCTET_HOP *SelHashHopOctetPtr)
{
	unsigned int index;
    STATS_CODE_HASH_RECORD    *RecordPtr = NULL;
	STATS_CODE_HASH_OCTET_HOP *NextHashHopOctetPtr = NULL;
	ObjListTask             *SelObjPtr = NULL;

	if (SelHashHopOctetPtr->UsedOctetCount)
	{
		for(index=0;index < MAX_STATS_CODE_OCTET_HASH_INDEX;index++)
		{
			if (SelHashHopOctetPtr->HashOctetHop[index])
			{
                NextHashHopOctetPtr = SelHashHopOctetPtr->HashOctetHop[index];
				if (!NextHashHopOctetPtr->UsedOctetCount)
				{
					if (NextHashHopOctetPtr->Record) FreeMemory(NextHashHopOctetPtr->Record);
					FreeMemory(NextHashHopOctetPtr);
					SelHashHopOctetPtr->HashOctetHop[index] = NULL;
					SelHashHopOctetPtr->UsedOctetCount--;
					if (!SelHashHopOctetPtr->UsedOctetCount) break;
				}
				else
				{
                    CloseStatsCodeHashHop(NextHashHopOctetPtr);
					if (!NextHashHopOctetPtr->UsedOctetCount)
					{
					    if (NextHashHopOctetPtr->Record) FreeMemory(NextHashHopOctetPtr->Record);
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
