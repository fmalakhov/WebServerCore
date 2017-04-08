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

static void StatIncHandle(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode, unsigned int StatsValue);
static void StatAddHandle(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode, unsigned int StatsValue);
//---------------------------------------------------------------------------
static void StatIncHandle(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode, unsigned int StatsValue)
{
    STATS_COLLECT_RECORD *StatsRecPtr = NULL;

    StatsRecPtr = FindStatsRecordHash(&ClientIdentHash, 
        ClientType, ClientId, DevId, StatsCode);
    if (!StatsRecPtr)
    {
        DebugLogPrint(NULL, "Not register stats inc request (CT:%d, CI:%d, DI:%d, SC:%d)\n", 
            ClientType, ClientId, DevId, StatsCode);          
        return;
    }
#ifdef _STATS_API_DEBUG_
    DebugLogPrint(NULL, "Stats inc: CT:%d, CI:%d, DI:%d, SC:%d\n", 
        ClientType, ClientId, DevId, StatsCode);
#endif
    if (StatsValue > 0) StatsRecPtr->StatsCount += StatsValue;
    else                StatsRecPtr->StatsCount++;
    StatsRecPtr->UpdateCount++;
}
//---------------------------------------------------------------------------
void StatsInc(unsigned char ClientType, unsigned int ClientId, unsigned short StatsCode)
{
    StatIncHandle(ClientType, ClientId, 1, StatsCode, 0);
}
//---------------------------------------------------------------------------
void StatsIncVal(unsigned char ClientType, unsigned int ClientId, unsigned short StatsCode, unsigned int StatsValue)
{
	StatIncHandle(ClientType, ClientId, 1, StatsCode, StatsValue);
}
//---------------------------------------------------------------------------
void StatsDevInc(unsigned char ClientType, unsigned int ClientId, unsigned short DevId, unsigned short StatsCode)
{
	StatIncHandle(ClientType, ClientId, DevId, StatsCode, 0);
}
//---------------------------------------------------------------------------
void StatsDevIncVal(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode, unsigned int StatsValue)
{
	StatIncHandle(ClientType, ClientId, DevId, StatsCode, StatsValue);
}
//---------------------------------------------------------------------------
static void StatAddHandle(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode, unsigned int StatsValue)
{
    STATS_COLLECT_RECORD *StatsRecPtr = NULL;

    StatsRecPtr = FindStatsRecordHash(&ClientIdentHash, 
        ClientType, ClientId, DevId, StatsCode);
    if (!StatsRecPtr)
    {

        DebugLogPrint(NULL, "Not register stats inc request (CT:%d, CI:%d, DI:%d, LT:%d)\n", 
            ClientType, ClientId, DevId, StatsCode);          
        return;
    }
#ifdef _STATS_API_DEBUG_
    DebugLogPrint(NULL, "Stats add: CT:%d, CI:%d, DI:%d, LT:%d\n", 
        ClientType, ClientId, DevId, StatsCode);
#endif
    if (StatsValue > 0) StatsRecPtr->StatsCount += StatsValue;
    else                StatsRecPtr->StatsCount++;
    StatsRecPtr->UpdateCount = 1;
}
//---------------------------------------------------------------------------
void StatsDevAddVal(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode, unsigned int StatsValue)
{
    StatAddHandle(ClientType, ClientId, DevId, StatsCode, StatsValue);
}
//---------------------------------------------------------------------------
