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

extern ListItsTask  StatsClrRecList;
extern CLIENT_IDENT_HASH_OCTET_HOP ClientIdentHash;

static void StatRegisterHandle(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode);
//---------------------------------------------------------------------------
static void StatRegisterHandle(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode)
{
    STATS_COLLECT_RECORD *NewStatsRecord = NULL;

    if (ClientType == SGT_GENERAL)
    {
        NewStatsRecord = FindStatsRecordHash(&ClientIdentHash,
            ClientType, ClientId, 1, StatsCode);
        if (NewStatsRecord)
        {
            NewStatsRecord->GeneralRegCount++;
            DebugLogPrint(NULL, "Stats register count: (%d), CT:%d, CI:%d, DI:%d, SC:%d\n", 
                NewStatsRecord->GeneralRegCount, ClientType, ClientId, DevId, StatsCode);
            return;
        }
    }
    
    NewStatsRecord = (STATS_COLLECT_RECORD*)AllocateMemory(sizeof(STATS_COLLECT_RECORD));
    if (!NewStatsRecord)
    {     
        DebugLogPrint(NULL, "Fail to memory allocate for new stats record\n");      
        return;
    }  
    DebugLogPrint(NULL, "Stats register: CT:%d, CI:%d, DI:%d, SC:%d\n", 
		ClientType, ClientId, DevId, StatsCode);

    NewStatsRecord->ClientType  = ClientType;
    NewStatsRecord->ClientId    = ClientId;
    NewStatsRecord->DevId       = DevId;    
    NewStatsRecord->StatsCode   = StatsCode;
    NewStatsRecord->StatsCount  = 0;
    NewStatsRecord->UpdateCount = 0;
    NewStatsRecord->GeneralRegCount = 1;
    NewStatsRecord->StatsListObjPtr = NULL;
    
    if (!AddClientIdentHash(&ClientIdentHash, 
        ClientType, ClientId, DevId, NewStatsRecord))
    {
        DebugLogPrint(NULL, "Fail to add stats record (CT:%d, CI:%d, DI:%d, SC:%d) to client identity hash\n",
            ClientType, ClientId, DevId, StatsCode);
        FreeMemory(NewStatsRecord);
        return;
    }
    NewStatsRecord->StatsListObjPtr = AddStructListObj(&StatsClrRecList, NewStatsRecord);    
}
//---------------------------------------------------------------------------
void StatsRegister(unsigned char ClientType, unsigned int ClientId, 
	unsigned short StatsCode)
{
	StatRegisterHandle(ClientType, ClientId, 1, StatsCode);
}
//---------------------------------------------------------------------------
void StatsDevRegister(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode)
{
    StatRegisterHandle(ClientType, ClientId, DevId, StatsCode);
}
//---------------------------------------------------------------------------
