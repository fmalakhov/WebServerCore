# if ! defined( SessionKeyHash )
#	define SessionKeyHash	/* only include me once */

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

#ifdef _LINUX_X86_
#include <time.h>
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

#define MAX_SESSKEY_CHAR_HASH_INDEX    16
#define SESSION_ID_KEY_LEN             24
#define INIT_SESSION_KEY_REC_GRP       100
#define INIT_SESSION_KEY_HASH_GRP      500

typedef struct {
	unsigned int    SessionId;
	void            *HashCharHopPtr;
	POOL_RECORD_STRUCT *PoolRecPtr;
} SESSION_KEY_HASH_RECORD;

typedef struct {
   unsigned int     UsedCharCount;
   unsigned int     ParentHashCharIndex;
   void             *ParentHashCharPtr;
   SESSION_KEY_HASH_RECORD *Record;
   POOL_RECORD_STRUCT      *PoolRecPtr;
   void             *HashCharHop[MAX_SESSKEY_CHAR_HASH_INDEX+1];
} SESSION_KEY_HASH_CHAR_HOP;

void InitSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *NameHop);
void CloseSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *RootNameHop);
void CloseSessionKeyHashHop(SESSION_KEY_HASH_CHAR_HOP *SelHashHopCharPtr);
bool AddSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *RootNameHop,  char *SessionKey, unsigned int SessionId);
SESSION_KEY_HASH_RECORD* FindSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *RootNameHop,char *SessionKey);
bool RemSessionKeyHash(SESSION_KEY_HASH_CHAR_HOP *RootNameHop, char *SessionKey);
void InitSessionKey();
void CloseSessionKey();
bool AddSessionKey(char *SessionKey, unsigned int SessionId);
unsigned int FindSessionByKey(char *SessionKey);
bool RemSessionKey(char *SessionKey);
void GenSessionIdKey(char *BufAnsw, unsigned int SessionId);
bool LockSessionKeyMutex();
void UnlockSessionKeyMutex();
bool SessionIdCheck(char *CmdBufPtr, unsigned int ValidSessionId);
//---------------------------------------------------------------------------
#endif  /* if ! defined( SessionKeyHashH ) */
