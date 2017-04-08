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

#include "ClientIdentHash.h"

extern ListItsTask StatsClrRecList;
//---------------------------------------------------------------------------
void InitClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *NameHop)
{
    unsigned int i;
	
	for(i=0;i < MAX_CLIENT_OCTET_HASH_INDEX;i++) 
	   NameHop->HashOctetHop[i] = NULL;
	NameHop->ParentHashOctetPtr = NULL;
    NameHop->Record = NULL;
    NameHop->UsedOctetCount = 0;
    NameHop->ParentHashOctetIndex = 0;	
}
//---------------------------------------------------------------------------
bool AddClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop, 
     unsigned char ClientType, unsigned int ClientId, unsigned short DevId,
     STATS_COLLECT_RECORD* RecordPtr)
{
    bool Result = true;
    unsigned char *ClientPtr;
    unsigned char Shift = 0x04;    
    unsigned int  OctetIndex, HopIndex;
	CLIENT_IDENT_HASH_OCTET_HOP *NewHashHopOctetPtr = NULL;
	CLIENT_IDENT_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	CLIENT_IDENT_HASH_RECORD    *NewRecordPtr = NULL;
    unsigned      char ClientIdent[6];

    ClientPtr = &ClientIdent[0];    
    *ClientPtr = (ClientType & 0x0f) << 4;
    *ClientPtr++ |= (unsigned char)((ClientId & 0xf0000) >> 16);
    *ClientPtr++ = (unsigned char)((ClientId & 0xff00) >> 8);
    *ClientPtr++ = (unsigned char)(ClientId & 0xff);
    *ClientPtr++ = (unsigned char)((DevId & 0xff00) >> 8);
    *ClientPtr++ = (unsigned char)(DevId & 0xff);          
	 
    ClientPtr = &ClientIdent[0];     
	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 10;OctetIndex++)
	{
        HopIndex = (unsigned int)((*ClientPtr >> Shift) & 0x0f);
	    if (!SelHashHopOctetPtr->HashOctetHop[HopIndex])
	    {
	        /* New char in hash for this hop is detected */
		    NewHashHopOctetPtr = (CLIENT_IDENT_HASH_OCTET_HOP*)AllocateMemory(sizeof(CLIENT_IDENT_HASH_OCTET_HOP));
		    if (!NewHashHopOctetPtr)
		    {
		        Result = false;
		        break;
		    }
		    InitClientIdentHash(NewHashHopOctetPtr);
		    SelHashHopOctetPtr->HashOctetHop[HopIndex] = NewHashHopOctetPtr;
		    SelHashHopOctetPtr->UsedOctetCount++;
		    NewHashHopOctetPtr->ParentHashOctetPtr = (void*)SelHashHopOctetPtr;
		    NewHashHopOctetPtr->ParentHashOctetIndex = HopIndex;
		    SelHashHopOctetPtr = NewHashHopOctetPtr;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopOctetPtr = (CLIENT_IDENT_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[HopIndex];
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
		if (SelHashHopOctetPtr->Record)
		{
            NewRecordPtr = SelHashHopOctetPtr->Record;
            if (!RecordPtr) return Result;
            if (!AddStatsCodeHash(&NewRecordPtr->StatsCodeHash, RecordPtr))
            {
                Result = false;
            }
            else
            {
                AddStructList(&NewRecordPtr->StatsList, RecordPtr);
            }
            return Result;
		}
        
		NewRecordPtr = (CLIENT_IDENT_HASH_RECORD*)AllocateMemory(sizeof(CLIENT_IDENT_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopOctetPtr->Record = NewRecordPtr;
            NewRecordPtr->HashOctetHopPtr = SelHashHopOctetPtr;
			NewRecordPtr->StatsList.Count = 0;
			NewRecordPtr->StatsList.CurrTask = NULL;
			NewRecordPtr->StatsList.FistTask = NULL;
            InitStatsCodeHash(&NewRecordPtr->StatsCodeHash);
            if (RecordPtr)
            {
                if (!AddStatsCodeHash(&NewRecordPtr->StatsCodeHash, RecordPtr))
                {
                    FreeMemory(NewRecordPtr);
                    SelHashHopOctetPtr->Record = NULL;
                    Result = false;
                }
                else
                {
                    AddStructList(&NewRecordPtr->StatsList, RecordPtr);
                }
            }            
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
CLIENT_IDENT_HASH_RECORD* FindClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop,
     unsigned char ClientType, unsigned int ClientId, unsigned short DevId)
{
    bool Result = true;
    unsigned char Shift = 0x04;    
    unsigned char *ClientPtr;
    unsigned int  OctetIndex, HopIndex;
	CLIENT_IDENT_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	unsigned      char ClientIdent[6];

    ClientPtr = &ClientIdent[0];    
    *ClientPtr = (ClientType & 0x0f) << 4;
    *ClientPtr++ |= (unsigned char)((ClientId & 0xf0000) >> 16);
    *ClientPtr++ = (unsigned char)((ClientId & 0xff00) >> 8);
    *ClientPtr++ = (unsigned char)(ClientId & 0xff);
    *ClientPtr++ = (unsigned char)((DevId & 0xff00) >> 8);
    *ClientPtr++ = (unsigned char)(DevId & 0xff);         
	 
    ClientPtr = &ClientIdent[0];             
	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 10;OctetIndex++)
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
		    SelHashHopOctetPtr = (CLIENT_IDENT_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[HopIndex];
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
STATS_COLLECT_RECORD* FindStatsRecordHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop,
     unsigned char ClientType, unsigned int ClientId, unsigned short DevId, unsigned short StatsCode)
{
	CLIENT_IDENT_HASH_RECORD   *SelRecordPtr = NULL;
    STATS_CODE_HASH_RECORD     *StatsCodeRecPtr = NULL;
    
    SelRecordPtr = FindClientIdentHash(RootNameHop, ClientType, ClientId,  DevId);
    if (!SelRecordPtr) return NULL;
    StatsCodeRecPtr = FindStatsCodeHash(&SelRecordPtr->StatsCodeHash, StatsCode);
    if (!StatsCodeRecPtr) return NULL;
	return StatsCodeRecPtr->StatsRecordPtr;
}
//---------------------------------------------------------------------------
bool RemClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop, 
          unsigned char ClientType, unsigned int ClientId, unsigned short DevId)
{
    CLIENT_IDENT_HASH_RECORD *ClientIdentRecPtr = NULL;
	CLIENT_IDENT_HASH_OCTET_HOP *ParentHashHopOctetPtr = NULL;
	CLIENT_IDENT_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	ObjListTask                 *SelObjPtr = NULL;
    STATS_COLLECT_RECORD                *SelStatsRecPtr = NULL;
    
    ClientIdentRecPtr = FindClientIdentHash(RootNameHop, ClientType, ClientId, DevId);
	if (!ClientIdentRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopOctetPtr = (CLIENT_IDENT_HASH_OCTET_HOP*)ClientIdentRecPtr->HashOctetHopPtr;   
	SelObjPtr = (ObjListTask*)GetFistObjectList(&ClientIdentRecPtr->StatsList);
	while(SelObjPtr)
	{
        SelStatsRecPtr = (STATS_COLLECT_RECORD*)SelObjPtr->UsedTask;
        if (SelStatsRecPtr->StatsListObjPtr) 
            RemStructList(&StatsClrRecList, SelStatsRecPtr->StatsListObjPtr);
        RemStructList(&ClientIdentRecPtr->StatsList, SelObjPtr);
        FreeMemory(SelStatsRecPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&ClientIdentRecPtr->StatsList);
	}
    CloseStatsCodeHash(&ClientIdentRecPtr->StatsCodeHash);
	FreeMemory(ClientIdentRecPtr);
	SelHashHopOctetPtr->Record = NULL;
	while(!SelHashHopOctetPtr || (SelHashHopOctetPtr != RootNameHop))
	{
	    if (!SelHashHopOctetPtr->UsedOctetCount)
	    {
	        ParentHashHopOctetPtr = (CLIENT_IDENT_HASH_OCTET_HOP*)SelHashHopOctetPtr->ParentHashOctetPtr;
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
void CloseClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop)
{
	CloseClientIdentHashHop(RootNameHop);
}
//---------------------------------------------------------------------------
void CloseClientIdentHashHop(CLIENT_IDENT_HASH_OCTET_HOP *SelHashHopOctetPtr)
{
	unsigned int index;
    CLIENT_IDENT_HASH_RECORD    *RecordPtr = NULL;
	CLIENT_IDENT_HASH_OCTET_HOP *NextHashHopOctetPtr = NULL;
	ObjListTask                 *SelObjPtr = NULL;
    STATS_COLLECT_RECORD                *SelStatsRecPtr = NULL;

	if (SelHashHopOctetPtr->UsedOctetCount)
	{
		for(index=0;index < MAX_CLIENT_OCTET_HASH_INDEX;index++)
		{
			if (SelHashHopOctetPtr->HashOctetHop[index])
			{
                NextHashHopOctetPtr = SelHashHopOctetPtr->HashOctetHop[index];
				if (!NextHashHopOctetPtr->UsedOctetCount)
				{
					if (NextHashHopOctetPtr->Record)
                    {
                        RecordPtr = NextHashHopOctetPtr->Record;
	                    SelObjPtr = (ObjListTask*)GetFistObjectList(&RecordPtr->StatsList);
	                    while(SelObjPtr)
	                    {
                            SelStatsRecPtr = (STATS_COLLECT_RECORD*)SelObjPtr->UsedTask;
                            if (SelStatsRecPtr->StatsListObjPtr)
                                RemStructList(&StatsClrRecList, SelStatsRecPtr->StatsListObjPtr); 
                            RemStructList(&RecordPtr->StatsList, SelObjPtr); 
                            FreeMemory(SelStatsRecPtr);
		                    SelObjPtr = (ObjListTask*)GetFistObjectList(&RecordPtr->StatsList);
	                    }           
                        CloseStatsCodeHash(&RecordPtr->StatsCodeHash);      
                        if (NextHashHopOctetPtr->Record) FreeMemory(NextHashHopOctetPtr->Record);
                        NextHashHopOctetPtr->Record = NULL;
                    }
                    FreeMemory(NextHashHopOctetPtr);
					SelHashHopOctetPtr->HashOctetHop[index] = NULL;
					SelHashHopOctetPtr->UsedOctetCount--;
					if (!SelHashHopOctetPtr->UsedOctetCount) break;
				}
				else
				{
                    CloseClientIdentHashHop(NextHashHopOctetPtr);
					if (!NextHashHopOctetPtr->UsedOctetCount)
					{
					    if (NextHashHopOctetPtr->Record)
                        {
                            RecordPtr = NextHashHopOctetPtr->Record;
	                        SelObjPtr = (ObjListTask*)GetFistObjectList(&RecordPtr->StatsList);
	                        while(SelObjPtr)
	                        {
                                SelStatsRecPtr = (STATS_COLLECT_RECORD*)SelObjPtr->UsedTask;
                                if (SelStatsRecPtr->StatsListObjPtr)
                                    RemStructList(&StatsClrRecList, SelStatsRecPtr->StatsListObjPtr);
                                RemStructList(&RecordPtr->StatsList, SelObjPtr);   
                                FreeMemory(SelStatsRecPtr);
		                        SelObjPtr = (ObjListTask*)GetFistObjectList(&RecordPtr->StatsList);
	                        }  
                            CloseStatsCodeHash(&RecordPtr->StatsCodeHash);                     
                            if (NextHashHopOctetPtr->Record) FreeMemory(NextHashHopOctetPtr->Record);
                            NextHashHopOctetPtr->Record = NULL;
                        }
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
