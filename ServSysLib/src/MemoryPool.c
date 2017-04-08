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

#include "MemoryPool.h"
//---------------------------------------------------------------------------
void CreatePool(POOL_RECORD_BASE *PoolBasePtr, unsigned int initPoolSize, unsigned int dataBlockSize)
{
    unsigned int i = 0;
    POOL_RECORD_STRUCT *l_NewRecPtr = NULL;
    
    PoolBasePtr->m_DataBlockSize = dataBlockSize;
    PoolBasePtr->m_FreeFirstRecordList = NULL;
    PoolBasePtr->m_FreeLastRecordList = NULL;
    PoolBasePtr->m_UsedFirstRecordList = NULL;
    PoolBasePtr->m_UsedLastRecordList = NULL;
    PoolBasePtr->m_SelectRecordPtr = NULL;
    PoolBasePtr->m_NumFreeRecords = 0;
    PoolBasePtr->m_NumUsedRecords = 0;
    for(i=0;i < initPoolSize;i++)
    {
        l_NewRecPtr = (POOL_RECORD_STRUCT*)AllocateMemory(sizeof(POOL_RECORD_STRUCT));
	    if(NULL != l_NewRecPtr)
		{
            l_NewRecPtr->PrevRecPtr = NULL;
	        l_NewRecPtr->NextRecPtr = NULL;
	        l_NewRecPtr->DataPtr = (unsigned char*)AllocateMemory(dataBlockSize);
	        if (l_NewRecPtr->DataPtr)
			{
			    memset(l_NewRecPtr->DataPtr, 0, dataBlockSize);
		        if (i == 0)
				{
	                //First pool record add
	                PoolBasePtr->m_FreeFirstRecordList = l_NewRecPtr;
	                PoolBasePtr->m_FreeLastRecordList = l_NewRecPtr;
				}
		        else
				{
	                //Next pool record add
	                l_NewRecPtr->PrevRecPtr = PoolBasePtr->m_FreeLastRecordList;
	                PoolBasePtr->m_FreeLastRecordList->NextRecPtr = l_NewRecPtr;
	                PoolBasePtr->m_FreeLastRecordList = l_NewRecPtr;
				}
		        PoolBasePtr->m_NumFreeRecords++;
			}
	        else
			{
	            FreeMemory((void*)l_NewRecPtr);
		        break;
			}	    
        }	
    }
}
//---------------------------------------------------------------------------
void DestroyPool(POOL_RECORD_BASE *PoolBasePtr)
{
    register POOL_RECORD_STRUCT *l_DelRecPtr = NULL;

    l_DelRecPtr = PoolBasePtr->m_FreeFirstRecordList;
    if ((PoolBasePtr->m_NumFreeRecords > 0) && (l_DelRecPtr != NULL))
    {
        while(l_DelRecPtr->NextRecPtr != NULL)
        {
            PoolBasePtr->m_FreeFirstRecordList = (POOL_RECORD_STRUCT*)l_DelRecPtr->NextRecPtr;
	        FreeMemory((void*)l_DelRecPtr->DataPtr);
	        FreeMemory((void*)l_DelRecPtr);
            if (!PoolBasePtr->m_FreeFirstRecordList) break;
	        l_DelRecPtr = PoolBasePtr->m_FreeFirstRecordList;
        };
    }
    l_DelRecPtr = PoolBasePtr->m_UsedFirstRecordList;
    if ((PoolBasePtr->m_NumUsedRecords > 0) && (l_DelRecPtr != NULL))
    {
        while(l_DelRecPtr->NextRecPtr != NULL)
        {
            PoolBasePtr->m_UsedFirstRecordList = (POOL_RECORD_STRUCT*)l_DelRecPtr->NextRecPtr;
	        FreeMemory((void*)l_DelRecPtr->DataPtr);
	        FreeMemory((void*)l_DelRecPtr);
            if (!PoolBasePtr->m_UsedFirstRecordList) break;
	        l_DelRecPtr = PoolBasePtr->m_UsedFirstRecordList;
        };
    }   
}
//---------------------------------------------------------------------------
POOL_RECORD_STRUCT* GetBuffer(POOL_RECORD_BASE *PoolBasePtr)
{
    register POOL_RECORD_STRUCT *l_FreeBuffPtr = NULL;
    
    if (PoolBasePtr->m_NumFreeRecords > 0)
    {
        l_FreeBuffPtr = PoolBasePtr->m_FreeLastRecordList;
	    if (PoolBasePtr->m_FreeLastRecordList != PoolBasePtr->m_FreeFirstRecordList)
	    {
	        PoolBasePtr->m_FreeLastRecordList = (POOL_RECORD_STRUCT*)(PoolBasePtr->m_FreeLastRecordList->PrevRecPtr);
            if (PoolBasePtr->m_FreeLastRecordList)
            {
	            PoolBasePtr->m_FreeLastRecordList->NextRecPtr = NULL;
            }
            else
            {
                printf("Pool issue is detected 1 - set Empty free (FR: %u)\n", PoolBasePtr->m_NumFreeRecords);
	            PoolBasePtr->m_FreeLastRecordList = NULL;
	            PoolBasePtr->m_FreeFirstRecordList = NULL;
                PoolBasePtr->m_NumFreeRecords = 1;
            }
	    }
	    else
	    {
	        PoolBasePtr->m_FreeLastRecordList = NULL;
	        PoolBasePtr->m_FreeFirstRecordList = NULL;
	    }
		if (PoolBasePtr->m_NumFreeRecords > 0) PoolBasePtr->m_NumFreeRecords--;
    }    
    else
    {   
        l_FreeBuffPtr = (POOL_RECORD_STRUCT*)AllocateMemory(sizeof(POOL_RECORD_STRUCT));
        if(l_FreeBuffPtr)
        {
	        l_FreeBuffPtr->DataPtr = (unsigned char*)AllocateMemory(PoolBasePtr->m_DataBlockSize+1);
	        if (!l_FreeBuffPtr->DataPtr)
	        {
	            FreeMemory(l_FreeBuffPtr);
		        l_FreeBuffPtr = NULL;
	        }
            else
            {
                memset(l_FreeBuffPtr->DataPtr, 0, PoolBasePtr->m_DataBlockSize);
            }
        }
    }
    
    if (l_FreeBuffPtr)
    {
	    l_FreeBuffPtr->NextRecPtr = NULL;
	    l_FreeBuffPtr->PrevRecPtr = NULL;
	    if (PoolBasePtr->m_NumUsedRecords > 0)
	    {
            if (PoolBasePtr->m_UsedLastRecordList)
            {
                PoolBasePtr->m_UsedLastRecordList->NextRecPtr = (void*)l_FreeBuffPtr;
                l_FreeBuffPtr->PrevRecPtr = (void*)PoolBasePtr->m_UsedLastRecordList;
                PoolBasePtr->m_UsedLastRecordList = l_FreeBuffPtr;
            }
            else
            {
                printf("Pool issue is detected 2 - set Empty free (FR: %u)\n", PoolBasePtr->m_NumFreeRecords);
                PoolBasePtr->m_UsedFirstRecordList = l_FreeBuffPtr;
                PoolBasePtr->m_UsedLastRecordList = l_FreeBuffPtr;
                PoolBasePtr->m_NumUsedRecords = 0;            
            }
	    }
	    else
	    {
            PoolBasePtr->m_UsedFirstRecordList = l_FreeBuffPtr;
            PoolBasePtr->m_UsedLastRecordList = l_FreeBuffPtr;
	    }
	    PoolBasePtr->m_NumUsedRecords++;
    }
    
    return l_FreeBuffPtr;
}
//---------------------------------------------------------------------------
void FreeBuffer(POOL_RECORD_BASE *PoolBasePtr, POOL_RECORD_STRUCT *in_FreeBufPtr)
{
    register POOL_RECORD_STRUCT *l_NextRecPtr = NULL;
    
    if (in_FreeBufPtr == NULL)
    {
        return;
    }
    if (in_FreeBufPtr->PrevRecPtr == NULL)    
    {
        if (in_FreeBufPtr->NextRecPtr == NULL)
		{
			//Just one record in busy list should be removed.
			PoolBasePtr->m_UsedFirstRecordList = NULL;
			PoolBasePtr->m_UsedLastRecordList = NULL;
			PoolBasePtr->m_SelectRecordPtr = NULL;
		}
		else
		{
			//First record in list of busy buffers should be removed.
			PoolBasePtr->m_UsedFirstRecordList = (POOL_RECORD_STRUCT*)(in_FreeBufPtr->NextRecPtr);
			PoolBasePtr->m_UsedFirstRecordList->PrevRecPtr = NULL;
			if (PoolBasePtr->m_SelectRecordPtr == in_FreeBufPtr)
			{
				PoolBasePtr->m_SelectRecordPtr = PoolBasePtr->m_UsedFirstRecordList;
			}
		}
    }
    else
    {
        if (in_FreeBufPtr->NextRecPtr == NULL)
		{
			//Last record in list of busy buffers should be removed.
			PoolBasePtr->m_UsedLastRecordList = (POOL_RECORD_STRUCT*)(in_FreeBufPtr->PrevRecPtr);
			PoolBasePtr->m_UsedLastRecordList->NextRecPtr = NULL;
			if (PoolBasePtr->m_SelectRecordPtr == in_FreeBufPtr)
			{
				PoolBasePtr->m_SelectRecordPtr = NULL;
			}	    
		}	
		else
		{
			//Middle record in list of busy buffers should be removed.
			l_NextRecPtr = (POOL_RECORD_STRUCT*)(in_FreeBufPtr->PrevRecPtr);
			l_NextRecPtr->NextRecPtr = in_FreeBufPtr->NextRecPtr;
			((POOL_RECORD_STRUCT*)(in_FreeBufPtr->NextRecPtr))->PrevRecPtr = (void*)l_NextRecPtr;
			if (PoolBasePtr->m_SelectRecordPtr == in_FreeBufPtr)
			{
				PoolBasePtr->m_SelectRecordPtr = l_NextRecPtr;
			}	    
		}
    }
    
    if (PoolBasePtr->m_NumFreeRecords > 0)
    {
        PoolBasePtr->m_FreeLastRecordList->NextRecPtr = (void*)in_FreeBufPtr;
	    in_FreeBufPtr->PrevRecPtr = (void*)PoolBasePtr->m_FreeLastRecordList;
	    in_FreeBufPtr->NextRecPtr = NULL;
	    PoolBasePtr->m_FreeLastRecordList = in_FreeBufPtr;	
    }
    else
    {
        PoolBasePtr->m_FreeLastRecordList = in_FreeBufPtr;
	    PoolBasePtr->m_FreeFirstRecordList = in_FreeBufPtr;
	    in_FreeBufPtr->PrevRecPtr = NULL;
	    in_FreeBufPtr->NextRecPtr = NULL;
    }    
    PoolBasePtr->m_NumFreeRecords++;
    if (PoolBasePtr->m_NumUsedRecords > 0) PoolBasePtr->m_NumUsedRecords--;
}
//---------------------------------------------------------------------------
POOL_RECORD_STRUCT* GetFirstRecord(POOL_RECORD_BASE *PoolBasePtr)
{
    PoolBasePtr->m_SelectRecordPtr = PoolBasePtr->m_UsedFirstRecordList;
    return PoolBasePtr->m_SelectRecordPtr;
}
//---------------------------------------------------------------------------
POOL_RECORD_STRUCT* GetNextRecord(POOL_RECORD_BASE *PoolBasePtr)
{
    if (PoolBasePtr->m_SelectRecordPtr == NULL) return NULL;
    if (PoolBasePtr->m_SelectRecordPtr->NextRecPtr != NULL)
    {
        PoolBasePtr->m_SelectRecordPtr = (POOL_RECORD_STRUCT*)(PoolBasePtr->m_SelectRecordPtr->NextRecPtr);
    }
    else
    {
        PoolBasePtr->m_SelectRecordPtr = NULL;
    }
    return PoolBasePtr->m_SelectRecordPtr;
}
//---------------------------------------------------------------------------
void ClearList(POOL_RECORD_BASE *PoolBasePtr)
{
    POOL_RECORD_STRUCT *l_RemRecord = NULL;
    
    while(1)
    {
        l_RemRecord = GetFirstRecord(PoolBasePtr);
	if (l_RemRecord == NULL) break;
	FreeBuffer(PoolBasePtr, l_RemRecord);
    }
}
//---------------------------------------------------------------------------
