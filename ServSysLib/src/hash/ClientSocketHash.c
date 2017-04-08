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

#include "ClientSocketHash.h"

static void InitClientSocketHash(CLIENT_SOCKET_HASH_OCTET_HOP *NameHop);
static unsigned char AddClientSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr, 
    void *ClientPtr, unsigned int ClientSocket);
static CLIENT_SOCKET_HASH_RECORD* FindClientSocketHash(CLIENT_SOCKET_HASH_OCTET_HOP *RootNameHop, unsigned int ClientSocket);
static bool RemClientSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr, unsigned int ClientSocket);
static void CloseClientSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr);
static void CloseClientSocketHashHop(CLIENT_SOCKET_HASH_CHAN *ChanPtr, CLIENT_SOCKET_HASH_OCTET_HOP *SelHashHopCharPtr);
//---------------------------------------------------------------------------
static void InitClientSocketHash(CLIENT_SOCKET_HASH_OCTET_HOP *NameHop)
{
    unsigned int i;
	
	for(i=0;i < MAX_SOCKET_OCTET_HASH_INDEX;i++) 
	   NameHop->HashOctetHop[i] = NULL;
	NameHop->ParentHashOctetPtr = NULL;
    NameHop->Record = NULL;
    NameHop->UsedOctetCount = 0;
    NameHop->ParentHashOctetIndex = 0;	
}
//---------------------------------------------------------------------------
static unsigned char AddClientSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr, 
    void *ClientPtr, unsigned int ClientSocket)
{
    unsigned char Result = SUCCES_ADD_SOCKET;
	unsigned long IpOctetMask = 0x0f;
    unsigned int OctetIndex, HopIndex;
	CLIENT_SOCKET_HASH_OCTET_HOP *NewHashHopOctetPtr = NULL;
	CLIENT_SOCKET_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	CLIENT_SOCKET_HASH_RECORD    *NewRecordPtr = NULL;
	POOL_RECORD_STRUCT           *PoolRecordPtr = NULL;

    ClientSocket &= 0xfffff;
	SelHashHopOctetPtr = &ChanPtr->RootSocketHop;
    for (OctetIndex=0;OctetIndex < MAX_SOCKET_INDEX_OCTET;OctetIndex++)
	{
	   HopIndex = (unsigned int)((ClientSocket & IpOctetMask) >> (OctetIndex << 2));
	   if (!SelHashHopOctetPtr->HashOctetHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   PoolRecordPtr = GetBuffer(&ChanPtr->SocketHashOctetPool);
		   if (!PoolRecordPtr)
		   {
		       Result = FAIL_ADD_SOCKET_MEMORY_ALLOC;
		       break;
		   }
           NewHashHopOctetPtr = (CLIENT_SOCKET_HASH_OCTET_HOP*)PoolRecordPtr->DataPtr;
		   NewHashHopOctetPtr->PoolRecordPtr = PoolRecordPtr;
		   InitClientSocketHash(NewHashHopOctetPtr);
		   SelHashHopOctetPtr->HashOctetHop[HopIndex] = NewHashHopOctetPtr;
		   SelHashHopOctetPtr->UsedOctetCount++;
		   NewHashHopOctetPtr->ParentHashOctetPtr = (void*)SelHashHopOctetPtr;
		   NewHashHopOctetPtr->ParentHashOctetIndex = HopIndex;
		   SelHashHopOctetPtr = NewHashHopOctetPtr;
	   }
	   else
	   {
	       /* Existing char in hash for this hop is detected */
		   SelHashHopOctetPtr = (CLIENT_SOCKET_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[HopIndex];
	   }
       IpOctetMask <<= 4;
	}
	if (Result == SUCCES_ADD_SOCKET)
	{
		if (SelHashHopOctetPtr->Record)
		{
            Result = FAIL_ADD_SOCKET_ALREADY_EXIST;
		}
        else
        {
			PoolRecordPtr = GetBuffer(&ChanPtr->SocketHashRecordPool);
		    if (!PoolRecordPtr) Result = FAIL_ADD_SOCKET_MEMORY_ALLOC;
		    else
		    {
                NewRecordPtr = (CLIENT_SOCKET_HASH_RECORD*)PoolRecordPtr->DataPtr;
			    NewRecordPtr->PoolRecordPtr = PoolRecordPtr;
		        SelHashHopOctetPtr->Record = NewRecordPtr;
		        NewRecordPtr->HashOctetHopPtr = SelHashHopOctetPtr;
		        NewRecordPtr->ClientPtr = ClientPtr;
            }
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
static CLIENT_SOCKET_HASH_RECORD* FindClientSocketHash(CLIENT_SOCKET_HASH_OCTET_HOP *RootNameHop,
										  unsigned int ClientSocket)
{
    bool Result = true;
	unsigned int IpOctetMask = 0x0f;
    unsigned int OctetIndex, HopIndex;
	CLIENT_SOCKET_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	CLIENT_SOCKET_HASH_RECORD   *SelRecordPtr = NULL;

    ClientSocket &= 0xfffff;
	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < MAX_SOCKET_INDEX_OCTET;OctetIndex++)
	{
	    HopIndex = (unsigned int)((ClientSocket & IpOctetMask) >> (OctetIndex << 2));
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
static bool RemClientSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr, unsigned int ClientSocket)
{
    CLIENT_SOCKET_HASH_RECORD *ClientSocketRecPtr = NULL;
	CLIENT_SOCKET_HASH_OCTET_HOP *ParentHashHopOctetPtr = NULL;
	CLIENT_SOCKET_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	
	ClientSocketRecPtr = FindClientSocketHash(&ChanPtr->RootSocketHop, ClientSocket);
	if (!ClientSocketRecPtr) return false;
	SelHashHopOctetPtr = (CLIENT_SOCKET_HASH_OCTET_HOP*)ClientSocketRecPtr->HashOctetHopPtr;
	FreeBuffer(&ChanPtr->SocketHashRecordPool, ClientSocketRecPtr->PoolRecordPtr);
	SelHashHopOctetPtr->Record = NULL;
	while(!SelHashHopOctetPtr || (SelHashHopOctetPtr != &ChanPtr->RootSocketHop))
	{
	    if (!SelHashHopOctetPtr->UsedOctetCount)
	    {
	        ParentHashHopOctetPtr = (CLIENT_SOCKET_HASH_OCTET_HOP*)SelHashHopOctetPtr->ParentHashOctetPtr;
		    ParentHashHopOctetPtr->HashOctetHop[SelHashHopOctetPtr->ParentHashOctetIndex] = NULL;
			FreeBuffer(&ChanPtr->SocketHashOctetPool, SelHashHopOctetPtr->PoolRecordPtr);
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
static void CloseClientSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr)
{
	CloseClientSocketHashHop(ChanPtr, &ChanPtr->RootSocketHop);
}
//---------------------------------------------------------------------------
static void CloseClientSocketHashHop(CLIENT_SOCKET_HASH_CHAN *ChanPtr, CLIENT_SOCKET_HASH_OCTET_HOP *SelHashHopOctetPtr)
{
	unsigned int index;
	CLIENT_SOCKET_HASH_OCTET_HOP *NextHashHopOctetPtr = NULL;

	if (SelHashHopOctetPtr->UsedOctetCount)
	{
		for(index=0;index < MAX_SOCKET_OCTET_HASH_INDEX;index++)
		{
			if (SelHashHopOctetPtr->HashOctetHop[index])
			{
                NextHashHopOctetPtr = (CLIENT_SOCKET_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[index];
				if (!NextHashHopOctetPtr->UsedOctetCount)
				{
					if (NextHashHopOctetPtr->Record)
						FreeBuffer(&ChanPtr->SocketHashRecordPool, NextHashHopOctetPtr->Record->PoolRecordPtr);
					FreeBuffer(&ChanPtr->SocketHashOctetPool, SelHashHopOctetPtr->PoolRecordPtr);
					SelHashHopOctetPtr->HashOctetHop[index] = NULL;
					SelHashHopOctetPtr->UsedOctetCount--;
					if (!SelHashHopOctetPtr->UsedOctetCount) break;
				}
				else
				{
                    CloseClientSocketHashHop(ChanPtr, NextHashHopOctetPtr);
					if (!NextHashHopOctetPtr->UsedOctetCount)
					{
					    if (NextHashHopOctetPtr->Record)
                            FreeBuffer(&ChanPtr->SocketHashRecordPool, NextHashHopOctetPtr->Record->PoolRecordPtr);
					    FreeBuffer(&ChanPtr->SocketHashOctetPool, SelHashHopOctetPtr->PoolRecordPtr);
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
void ClientSocketHashChanCreate(CLIENT_SOCKET_HASH_CHAN *ChanPtr)
{
	InitClientSocketHash(&ChanPtr->RootSocketHop);
	CreatePool(&ChanPtr->SocketHashOctetPool, INIT_CLSOCKET_OCTET_POOL_SIZE, sizeof(CLIENT_SOCKET_HASH_OCTET_HOP));
	CreatePool(&ChanPtr->SocketHashRecordPool, INIT_CLSOCKET_REC_POOL_SIZE, sizeof(CLIENT_SOCKET_HASH_RECORD));
}
//---------------------------------------------------------------------------
void ClientSocketHashChanClose(CLIENT_SOCKET_HASH_CHAN *ChanPtr)
{
	CloseClientSocketHash(ChanPtr);
	DestroyPool(&ChanPtr->SocketHashRecordPool);
	DestroyPool(&ChanPtr->SocketHashOctetPool);
}
//---------------------------------------------------------------------------
void* ClientInfoFindBySocket(CLIENT_SOCKET_HASH_CHAN *ChanPtr, unsigned int ClientSocket)
{
	CLIENT_SOCKET_HASH_RECORD* RecPtr = NULL;

    RecPtr = FindClientSocketHash(&ChanPtr->RootSocketHop, ClientSocket);
	if (RecPtr && RecPtr->ClientPtr) return RecPtr->ClientPtr;
	return NULL;
}
//---------------------------------------------------------------------------
unsigned char AddClientInfoSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr, 
	void *ClientPtr, unsigned int ClientSocket)
{
	return AddClientSocketHash(ChanPtr, ClientPtr, ClientSocket);
}
//---------------------------------------------------------------------------
bool RemClientInfoSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr, unsigned int ClientSocket)
{
	return RemClientSocketHash(ChanPtr, ClientSocket);
}
//---------------------------------------------------------------------------
