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
#include "SysWebFunction.h"
#include "SessionKeyHash.h"

#ifdef _VCL60ENV_
HANDLE gSessionKeyMutex;
#endif

#ifdef _LINUX_X86_
sem_t  gSessionKeyMutex;
#endif

unsigned int SessionKeyHashEntityCount = 0;

SESSION_KEY_HASH_CHAR_HOP SessionKeyHashHop;
POOL_RECORD_BASE          SessionKeyRecPool;
POOL_RECORD_BASE          SessionKeyHashPool;

extern char *EndHtmlPageGenPtr;
extern char KeyFormSessionId[];

unsigned char AsciiSessKeyCharToIndex[] ={
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,   0,   0,   0,   0,   0,   0,
   0,  11,  12,  13,  14,  15,  16,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };

char HexConvTable[] = {"0123456789ABCDEF"};
//---------------------------------------------------------------------------
void InitSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(SESSION_KEY_HASH_CHAR_HOP));	
}
//---------------------------------------------------------------------------
bool AddSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *RootNameHop, char *SessionKey,
					  unsigned int SessionId)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex;
	SESSION_KEY_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	SESSION_KEY_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	SESSION_KEY_HASH_RECORD   *NewRecordPtr = NULL;
	POOL_RECORD_STRUCT        *SelRecordBlkPtr = NULL;

	NewRecordPtr = FindSessionKeyHash(RootNameHop, SessionKey);
	if (NewRecordPtr) return false;
	
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < SESSION_ID_KEY_LEN;CharIndex++)
	{
	   HopIndex = AsciiSessKeyCharToIndex[(unsigned char)SessionKey[CharIndex]];
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
           SelRecordBlkPtr = GetBuffer(&SessionKeyHashPool);
		   if (!SelRecordBlkPtr)
		   {
		       Result = false;
		       break;
		   }
		   NewHashHopCharPtr = (SESSION_KEY_HASH_CHAR_HOP*)SelRecordBlkPtr->DataPtr;
		   NewHashHopCharPtr->PoolRecPtr = SelRecordBlkPtr;
		   InitSessionKeyHash(NewHashHopCharPtr);
		   SelHashHopCharPtr->HashCharHop[HopIndex] = NewHashHopCharPtr;
		   SelHashHopCharPtr->UsedCharCount++;
		   NewHashHopCharPtr->ParentHashCharPtr = (void*)SelHashHopCharPtr;
		   NewHashHopCharPtr->ParentHashCharIndex = HopIndex;
		   SelHashHopCharPtr = NewHashHopCharPtr;
		   SessionKeyHashEntityCount++;
	   }
	   else
	   {
	       /* Existing char in hash for this hop is detected */
		   SelHashHopCharPtr = SelHashHopCharPtr->HashCharHop[HopIndex];
	   }
	}
	if (Result)
	{
	    /* File hash fillout */
		SelRecordBlkPtr = GetBuffer(&SessionKeyRecPool);
		if (!SelRecordBlkPtr)
		{		
		    Result = false;
		}
		else
		{
			NewRecordPtr = (SESSION_KEY_HASH_RECORD*)SelRecordBlkPtr->DataPtr;
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
			NewRecordPtr->SessionId = SessionId;
            NewRecordPtr->PoolRecPtr = SelRecordBlkPtr;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
SESSION_KEY_HASH_RECORD* FindSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *RootNameHop, char *SessionKey)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex;
	SESSION_KEY_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	SESSION_KEY_HASH_RECORD   *SelRecordPtr = NULL;

	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < SESSION_ID_KEY_LEN;CharIndex++)
	{
	    HopIndex = AsciiSessKeyCharToIndex[(unsigned char)SessionKey[CharIndex]];
		if (!HopIndex)
		{
	        Result = false;
            break;
		}
	    if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	    {
	        Result = false;
            break;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopCharPtr = SelHashHopCharPtr->HashCharHop[HopIndex];
	    }
	}
	if (!Result) return NULL;
	return SelHashHopCharPtr->Record;
}
//---------------------------------------------------------------------------
bool RemSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *RootNameHop, char *SessionKey)
{
    SESSION_KEY_HASH_RECORD   *SessionKeyRecPtr = NULL;
	SESSION_KEY_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	SESSION_KEY_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    SessionKeyRecPtr = FindSessionKeyHash(RootNameHop, SessionKey);
	if (!SessionKeyRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (SESSION_KEY_HASH_CHAR_HOP*)SessionKeyRecPtr->HashCharHopPtr;
    FreeBuffer(&SessionKeyRecPool, SessionKeyRecPtr->PoolRecPtr);
	SelHashHopCharPtr->Record = NULL;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (SESSION_KEY_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
		    ParentHashHopCharPtr->HashCharHop[SelHashHopCharPtr->ParentHashCharIndex] = NULL;
			FreeBuffer(&SessionKeyHashPool, SelHashHopCharPtr->PoolRecPtr);
			SessionKeyHashEntityCount--;
		    ParentHashHopCharPtr->UsedCharCount--;
			SelHashHopCharPtr = ParentHashHopCharPtr;
	    }
	    else
	    {
	        break;
	    }
	}
	return true;
}
//---------------------------------------------------------------------------
void CloseSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *RootNameHop)
{
	CloseSessionKeyHashHop(RootNameHop);
}
//---------------------------------------------------------------------------
void CloseSessionKeyHashHop(SESSION_KEY_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	SESSION_KEY_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MAX_SESSKEY_CHAR_HASH_INDEX;index++)
		{
			if (SelHashHopCharPtr->HashCharHop[index])
			{
                NextHashHopCharPtr = SelHashHopCharPtr->HashCharHop[index];
				if (!NextHashHopCharPtr->UsedCharCount)
				{
					if (NextHashHopCharPtr->Record)
                        FreeBuffer(&SessionKeyRecPool, NextHashHopCharPtr->Record->PoolRecPtr);
					FreeBuffer(&SessionKeyHashPool, NextHashHopCharPtr->PoolRecPtr);
					SelHashHopCharPtr->HashCharHop[index] = NULL;
					SelHashHopCharPtr->UsedCharCount--;
					if (!SelHashHopCharPtr->UsedCharCount) break;
				}
				else
				{
                    CloseSessionKeyHashHop(NextHashHopCharPtr);
					if (!NextHashHopCharPtr->UsedCharCount)
					{
					    if (NextHashHopCharPtr->Record)
							FreeBuffer(&SessionKeyRecPool, NextHashHopCharPtr->Record->PoolRecPtr);
						FreeBuffer(&SessionKeyHashPool, NextHashHopCharPtr->PoolRecPtr);
					    SelHashHopCharPtr->HashCharHop[index] = NULL;
					    SelHashHopCharPtr->UsedCharCount--;
					    if (!SelHashHopCharPtr->UsedCharCount) break;
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void InitSessionKey()
{
    InitSessionKeyHash(&SessionKeyHashHop);
#ifdef _VCL60ENV_
    gSessionKeyMutex = CreateMutex(NULL, FALSE, NULL);
#endif

#ifdef _LINUX_X86_
    sem_init(&gSessionKeyMutex, 0, 0);
	sem_post(&gSessionKeyMutex);
#endif
	CreatePool(&SessionKeyRecPool, INIT_SESSION_KEY_REC_GRP, sizeof(SESSION_KEY_HASH_RECORD));
	CreatePool(&SessionKeyHashPool, INIT_SESSION_KEY_HASH_GRP, sizeof(SESSION_KEY_HASH_CHAR_HOP));
}
//---------------------------------------------------------------------------
void CloseSessionKey()
{
	CloseSessionKeyHash(&SessionKeyHashHop);
#ifdef _VCL60ENV_
		CloseHandle(gSessionKeyMutex);
#endif
#ifdef _LINUX_X86_
	sem_destroy(&gSessionKeyMutex);
#endif
	DestroyPool(&SessionKeyRecPool);
	DestroyPool(&SessionKeyHashPool);
}
//---------------------------------------------------------------------------
bool LockSessionKeyMutex() 
{
#ifdef _VCL60ENV_
	DWORD       WaitResult;
	bool        isMemAllocReady = false;

    WaitResult = WaitForSingleObject(gSessionKeyMutex, INFINITE);
    switch(WaitResult)
	{
	    case WAIT_OBJECT_0:
			isMemAllocReady = true;
		    break;

        case WAIT_ABANDONED: 
			printf("The other thread that using mutex is closed in locked state of mutex\r\n");
            break;

		default:
			printf("Session key hash mutex is fialed with error: %d\r\n", GetLastError());
			break;
	}
    if (!isMemAllocReady)
	{
        if (!ReleaseMutex(gSessionKeyMutex)) 
		{ 
            printf("Fail to release mutex (session key hash)\r\n");
		}
	    return false;
	}
#endif
#ifdef _LINUX_X86_
    sem_wait(&gSessionKeyMutex);
#endif
	return true;
}
//---------------------------------------------------------------------------
void UnlockSessionKeyMutex()
{
#ifdef _VCL60ENV_
    if (!ReleaseMutex(gSessionKeyMutex)) 
	{ 
        printf("Fail to release mutex (session key hash)\r\n");
	}
#endif
#ifdef _LINUX_X86_
    sem_post(&gSessionKeyMutex);
#endif
}
//---------------------------------------------------------------------------
bool AddSessionKey(char *SessionKey, unsigned int SessionId)
{
	bool Result;

	if (!LockSessionKeyMutex()) return false;
    Result = AddSessionKeyHash(&SessionKeyHashHop, SessionKey, SessionId);
    UnlockSessionKeyMutex();
	return Result;
}
//---------------------------------------------------------------------------
unsigned int FindSessionByKey(char *SessionKey)
{
	unsigned int            SessionId = 0;
	SESSION_KEY_HASH_RECORD *SessonKeyRecPtr = NULL;

	if (!LockSessionKeyMutex()) return SessionId;
    SessonKeyRecPtr = FindSessionKeyHash(&SessionKeyHashHop, SessionKey);
	if (SessonKeyRecPtr) SessionId = SessonKeyRecPtr->SessionId;
	UnlockSessionKeyMutex();
	return SessionId;
}
//---------------------------------------------------------------------------
bool RemSessionKey(char *SessionKey)
{
	bool Result;

	if (!LockSessionKeyMutex()) return false;
	Result = RemSessionKeyHash(&SessionKeyHashHop, SessionKey);
	UnlockSessionKeyMutex();
	return Result;
}
//---------------------------------------------------------------------------
void GenSessionIdKey(char *BufAnsw, unsigned int SessionId)
{
	unsigned int  index, grp;
	unsigned int  RandKey;
	unsigned long CurrTick;
	char          *KeyGenPtr;

	if (BufAnsw) KeyGenPtr = BufAnsw;
	else         KeyGenPtr = EndHtmlPageGenPtr;
    CurrTick = GetTickCount();
	*KeyGenPtr++ = 'C';
    *KeyGenPtr++ = 'F';
    *KeyGenPtr++ = '9';
	*KeyGenPtr++ = 'A';
    for(index=0;index < 8;index++)
	{
		*KeyGenPtr++ = HexConvTable[CurrTick & 0xf];
		CurrTick >>= 4;
	}
    for(index=0;index < 4;index++)
	{
		*KeyGenPtr++ = HexConvTable[SessionId & 0xf];
		SessionId >>= 4;
	}
	for(grp=0;grp < 2;grp++)
	{
        RandKey = (unsigned int)(rand()%0xffff);
        for(index=0;index < 4;index++)
		{
		    *KeyGenPtr++ = HexConvTable[RandKey & 0xf];
		    RandKey >>= 4;
		}
	}
	*KeyGenPtr = 0;
	if (!BufAnsw) EndHtmlPageGenPtr = KeyGenPtr;
}
//---------------------------------------------------------------------------
bool SessionIdCheck(char *CmdBufPtr, unsigned int ValidSessionId)
{
	int          i;
	bool         Result = false;
	unsigned int SessionId;
	char         *FText = NULL;

	for(;;)
	{
	    i = FindCmdRequest(CmdBufPtr, KeyFormSessionId);
	    if (i == -1) break;
        FText = ParseParForm(&CmdBufPtr[i]);
        if (!FText) break;
	    if (strlen(FText) < SESSION_ID_KEY_LEN) break;
        SessionId = FindSessionByKey(FText);
	    if (SessionId != ValidSessionId) break;
		Result = true;
		break;
	}
	return Result;
}
//---------------------------------------------------------------------------
