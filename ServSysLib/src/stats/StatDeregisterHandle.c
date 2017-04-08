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

#include "CommonPlatform.h"
#include "SysLibTool.h"
#include "MemoryPool.h"
#include "ClientIdentHash.h"
#include "ThrReportMen.h"

extern CLIENT_IDENT_HASH_OCTET_HOP ClientIdentHash;
extern ListItsTask  StatsClrRecList;

static void StatDeregisterHandle(unsigned char ClientType, unsigned int ClientId, unsigned short DevId);
//---------------------------------------------------------------------------
static void StatDeregisterHandle(unsigned char ClientType, unsigned int ClientId, unsigned short DevId)
{
    unsigned int               RemReadyCount = 0;
	CLIENT_IDENT_HASH_RECORD   *SelRecordPtr = NULL;
	ObjListTask                *SelObjPtr = NULL;
    STATS_COLLECT_RECORD       *SelStatsRecPtr = NULL;
  
    DebugLogPrint(NULL, "Stats deregister: CT:%d, CI:%d, DI:%d\n",
        ClientType, ClientId, DevId);
        
    if (ClientType == SGT_GENERAL)
    {
        SelRecordPtr = FindClientIdentHash(&ClientIdentHash, 
            ClientType, ClientId, DevId);
        if (SelRecordPtr)
        {
	        SelObjPtr = (ObjListTask*)GetFistObjectList(&SelRecordPtr->StatsList);
	        while(SelObjPtr)
	        {
                SelStatsRecPtr = (STATS_COLLECT_RECORD*)SelObjPtr->UsedTask;
                if (SelStatsRecPtr->GeneralRegCount > 1)
                {
                    SelStatsRecPtr->GeneralRegCount--;                            
                    DebugLogPrint(NULL, "Stats deregister counter: (%d), CT:%d, CI:%d, DI:%d, LT:%d\n", 
                        SelStatsRecPtr->GeneralRegCount,
                        ClientType, ClientId, DevId, SelStatsRecPtr->StatsCode);
                }
                else    RemReadyCount++;
		        SelObjPtr = (ObjListTask*)GetNextObjectList(&SelRecordPtr->StatsList);
	        }
            if (RemReadyCount < (unsigned int)SelRecordPtr->StatsList.Count) return;
        }
    }
    DebugLogPrint(NULL, "Stats deregister ready: CT:%d, CI:%d, DI:%d\n", 
        ClientType, ClientId, DevId);
    if (!RemClientIdentHash(&ClientIdentHash, ClientType, ClientId, DevId))
    {
#ifdef _LMC_CLIENT_DEBUG_    
        DebugLogPrint(&LabClientInfo, "Fail to remove stats record (CT:%d, CI:%d, DI:%d) from client identity hash\n",
            ClientType, ClientId, DevId);
#endif            
    }
}
//---------------------------------------------------------------------------
void StatsDeregister(unsigned char ClientType, unsigned int ClientId)
{ 
    StatDeregisterHandle(ClientType, ClientId, 1);
}
//---------------------------------------------------------------------------
void StatsDevDeregister(unsigned char ClientType, unsigned int ClientId, unsigned short DevId)
{ 
    StatDeregisterHandle(ClientType, ClientId, DevId);
}
//---------------------------------------------------------------------------
void SingleStatsDevDeregister(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode)
{
    bool                       isStatsFound = false;
	ObjListTask                *SelObjPtr = NULL;
	CLIENT_IDENT_HASH_RECORD   *SelRecordPtr = NULL;
    STATS_COLLECT_RECORD       *SelStatsRecPtr = NULL;

    SelRecordPtr = FindClientIdentHash(&ClientIdentHash, ClientType, ClientId, DevId);
    if (SelRecordPtr)
    {
	    SelObjPtr = (ObjListTask*)GetFistObjectList(&SelRecordPtr->StatsList);
	    while(SelObjPtr)
	    {
            SelStatsRecPtr = (STATS_COLLECT_RECORD*)SelObjPtr->UsedTask;
            if (SelStatsRecPtr->StatsCode == StatsCode)
            {
                if (!RemStatsCodeHash(&SelRecordPtr->StatsCodeHash, StatsCode))
                {            
                    DebugLogPrint(NULL, "Fail to remove stats from stats code hash (CT:%d, CI:%d, DI:%d)\n",
                        ClientType, ClientId, DevId);
                }
                if (SelStatsRecPtr->StatsListObjPtr) 
                    RemStructList(&StatsClrRecList, SelStatsRecPtr->StatsListObjPtr);
                RemStructList(&SelRecordPtr->StatsList, SelObjPtr);
                FreeMemory(SelStatsRecPtr);
                isStatsFound = true;
                break;
            }
		    SelObjPtr = (ObjListTask*)GetNextObjectList(&SelRecordPtr->StatsList);
	    }
        if (!isStatsFound)
        { 
            DebugLogPrint(NULL, "Fail to find stats in stats list (CT:%d, CI:%d, DI:%d, SC: %d)\n",
                ClientType, ClientId, DevId, StatsCode);          
        }
    }
    else
    { 
        DebugLogPrint(NULL, "No client in client's hash (CT:%d, CI:%d, DI:%d, SC: %d)\n",
            ClientType, ClientId, DevId, StatsCode);
    }
}
//---------------------------------------------------------------------------

