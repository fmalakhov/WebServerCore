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

#include <sys/stat.h>

#include "CommonPlatform.h"
#include "ThrWebServer.h"
#include "ThrCernel.h"
#include "SysWebFunction.h"
#include "ThrConnWeb.h"
#include "SysMessages.h"
#include "TrTimeOutThread.h"
#include "HttpPageGen.h"
#include "ServerPorts.h"
#include "SessionIpHash.h"
#include "HtmlPageHash.h"

#ifdef _LINUX_X86_
#include <dirent.h>
#endif

extern char ThrWebServName[];
extern bool gIsUserDbSaveNeeds;
extern bool gIsSiteMapGenNeeds;
extern STATS_INFO   ServerStats;
extern unsigned int SessIpRecInHashCount;
extern unsigned int SessIpHashEntityCount;
extern USER_DB_INFO SampleUserDbIfo;
//---------------------------------------------------------------------------
void HandleStatsCheckTimerExp(PARAMWEBSERV *ParWebServ)
{
    struct timeb    hires_cur_time;
    struct tm       *cur_time;
    int             Res;
	unsigned char   CurrDayOfWeek;

	DebugLogPrint(NULL, "%s: Thread is alive (RxThr: %d; TxThr: %d)\r\n", 
	   ThrWebServName, GetNumberActiveReaders(&ParWebServ->ReaderWorker),
	   ParWebServ->SenderWorker.HttpSendThrList.Count);
    
    ReportStatusPrint();
	ParWebServ->StatsIncCnt++;
	if (ParWebServ->StatsIncCnt < 15)
	{
        ftime(&hires_cur_time);
        cur_time = localtime(&hires_cur_time.time);
		CurrDayOfWeek = (unsigned char)(cur_time->tm_wday);
		if (((ParWebServ->StatsIncCnt == 6) || 
			 (ParWebServ->StatsIncCnt == 11)) && 
			 gIsUserDbSaveNeeds)
		{
			UserInfoDBSave(&SampleUserDbIfo);
            gIsUserDbSaveNeeds = false;
		}
		return;
	}
	ParWebServ->StatsIncCnt = 0;
	EventLogPrint(NULL, 
		"Stats Report - Sim.Read: %d; Sim.Send: %d; Sim.Open: %d; Usr.Sess: %d; Bot.Sess: %d; SendPg: %d; SendFl: %d; SucMail: %d; FailMail: %d; HtmlAvg: %d ms; HtmlMax: %d ms\r\n",
		ServerStats.SumultReadSessions, ServerStats.SimultSendSessions,
		ServerStats.SimultOpenSessions, ServerStats.UserSessions, ServerStats.BotSessions,
		ServerStats.SendHtmlPages, ServerStats.SendFiles, 
		ServerStats.SuccMailDelivery, ServerStats.FailMailDelivery,
		ServerStats.AvgPageGenTime, ServerStats.MaxPageGenTime);

	EventLogPrint(NULL, 
		"IP addresses  in list: %d, Session IP hash size: %d\r\n", 
		SessIpRecInHashCount, SessIpHashEntityCount);

	StatsDataSave();
	if (gIsSiteMapGenNeeds) SiteMapFileGen(ParWebServ);
}
//---------------------------------------------------------------------------
