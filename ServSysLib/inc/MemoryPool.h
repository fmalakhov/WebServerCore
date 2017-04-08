# if ! defined( MemoryPoolH )
#	define MemoryPoolH	/* only include me once */

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

/* Structure of record for pool management */
typedef struct {
    void *PrevRecPtr;       /* Pointer to previous record in pool. */
    void *NextRecPtr;       /* Pointer to next record in pool. */
    unsigned char *DataPtr; /* Pointer to data block. */
} POOL_RECORD_STRUCT;

typedef struct {
    unsigned int m_DataBlockSize; /* Size of block data record. */
    POOL_RECORD_STRUCT *m_FreeFirstRecordList;
    POOL_RECORD_STRUCT *m_FreeLastRecordList;
    POOL_RECORD_STRUCT *m_UsedFirstRecordList;
    POOL_RECORD_STRUCT *m_UsedLastRecordList;
    POOL_RECORD_STRUCT *m_SelectRecordPtr;
    unsigned int m_NumFreeRecords; /* Number of free records in pool. */
    unsigned int m_NumUsedRecords; /* Number of used records in pool. */
} POOL_RECORD_BASE;

void CreatePool(POOL_RECORD_BASE *PoolBasePtr, unsigned int initPoolSize, unsigned int dataBlockSize);
void DestroyPool(POOL_RECORD_BASE *PoolBasePtr);
POOL_RECORD_STRUCT* GetBuffer(POOL_RECORD_BASE *PoolBasePtr);
void FreeBuffer(POOL_RECORD_BASE *PoolBasePtr, POOL_RECORD_STRUCT *in_FreeBufPtr);
POOL_RECORD_STRUCT* GetFirstRecord(POOL_RECORD_BASE *PoolBasePtr);
POOL_RECORD_STRUCT* GetNextRecord(POOL_RECORD_BASE *PoolBasePtr);
void ClearList(POOL_RECORD_BASE *PoolBasePtr);

//---------------------------------------------------------------------------
#endif  /* if ! defined( MemoryPoolH ) */
