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

#include "MailWorker.h"
#include "TrTimeOutThread.h"
#include "ThrReportMen.h"
#include "WebServInfo.h"
#include "WebServMsgApi.h"
#include "BaseWebServer.h"

static char MailWorkerName[] = "MailWorker";
//---------------------------------------------------------------------------
bool SendMailSmtpServer(MAIL_WORKER_INFO *MailWorkerPtr, char *MailTo,
	char *MailSubject, char *MailBody)
{
	PARSENDMAIL *ParSendMailPtr = NULL;
	bool        Result;

	ParSendMailPtr = (PARSENDMAIL*)AllocateMemory(sizeof(PARSENDMAIL));
	if (!ParSendMailPtr) 
	{
	    if (MailBody) FreeMemory(MailBody);
	    return false;
	}
	ParSendMailPtr->IDMessSendMailRes = WSU_RESPMAILSENT;
#ifdef WIN32  
	ParSendMailPtr->ParentThrID = GetCurrentThreadId();
#endif
	ParSendMailPtr->ClientCfgPtr = &MailWorkerPtr->MailClientCfg;
	ParSendMailPtr->Status = 0;
	strcpy(ParSendMailPtr->MailFrom, ParSendMailPtr->ClientCfgPtr->MailFrom);
	strcpy((char*)&ParSendMailPtr->MailTo, MailTo);
	strcpy((char*)&ParSendMailPtr->MailSubject, MailSubject);
	ParSendMailPtr->MailBody = MailBody;
	if (MailWorkerPtr->MailSendThrList.Count)
	{
		AddStructList(&MailWorkerPtr->MailSendThrList, ParSendMailPtr);	
		return true;
	}
#ifdef _LINUX_X86_
    ParSendMailPtr->NotifyPort = MailWorkerPtr->WebServMsgPort;
	if (SendMailThreadCreate(ParSendMailPtr))
#endif
#ifdef WIN32
	ParSendMailPtr->HTRSENDMAILSMTP = CreateThread( NULL, 0, 
		(LPTHREAD_START_ROUTINE)THRSendMailClient,
		(LPVOID)(ParSendMailPtr), 0, (LPDWORD)&ParSendMailPtr->ThrSendMailSmtpID);
	if (ParSendMailPtr->HTRSENDMAILSMTP)
#endif
	{
	    AddStructList(&MailWorkerPtr->MailSendThrList, ParSendMailPtr);
#ifdef _SERVDEBUG_
        DebugLogPrint(NULL, "%s: Start send e-mail\r\n", MailWorkerName);
#endif
		Result = true;
	}
	else
	{
		DebugLogPrint(NULL, "%s: Fail to create email send thread\r\n", 
			MailWorkerName);
		if (MailBody) FreeMemory(MailBody);
		FreeMemory(ParSendMailPtr);
		Result = false;
	}
	return Result;
}
//---------------------------------------------------------------------------
void HandleMailThrDone(MAIL_WORKER_INFO *MailWorkerPtr, PARSENDMAIL *ParSendMailPtr)
{
	ObjListTask *SelObjPtr;
	PARSENDMAIL *SelSendMailPtr;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&MailWorkerPtr->MailSendThrList);
	while(SelObjPtr)
	{
	    SelSendMailPtr = (PARSENDMAIL*)SelObjPtr->UsedTask;
		if (ParSendMailPtr == SelSendMailPtr)
		{
			if (MailWorkerPtr->OnMailSendStatus)
			{
				if (ParSendMailPtr->Status == 0) (MailWorkerPtr->OnMailSendStatus)(true);
				else                             (MailWorkerPtr->OnMailSendStatus)(false);
			}
			if (ParSendMailPtr->MailBody) FreeMemory(ParSendMailPtr->MailBody);
		    FreeMemory(ParSendMailPtr);
		    RemStructList(&MailWorkerPtr->MailSendThrList, SelObjPtr);
			break;
		}
	    SelObjPtr = (ObjListTask*)GetNextObjectList(&MailWorkerPtr->MailSendThrList);
	}
	if (MailWorkerPtr->MailSendThrList.Count)
	{
#ifdef WIN32
		CreateThreadTimer(GetCurrentThreadId(), 
			MailWorkerPtr->MailClientCfg.MailSendInt*TMR_ONE_SEC,
			MailWorkerPtr->MailSendDelayTimerId, false);
#else
		CreateThreadTimerCB(OnWebServerTimerExp, MailWorkerPtr->WebServMsgPort, 
			MailWorkerPtr->MailClientCfg.MailSendInt*TMR_ONE_SEC,
			MailWorkerPtr->MailSendDelayTimerId, false);
#endif
	}
}
//---------------------------------------------------------------------------
void HandleTimeoutNextMailSent(PARAMWEBSERV *ParWebServ)
{
    ObjListTask *SelObjPtr = NULL;
    PARSENDMAIL *SelSendMailPtr = NULL;
    MAIL_WORKER_INFO *MailWorkerPtr = NULL;

#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Begin HandlerWebServerReq function (TM_ONTIMEOUT - INTER_MAIL_DELAY_TMR_ID)\r\n", MailWorkerName);
#endif
	MailWorkerPtr = &ParWebServ->MailWorker;
    SelObjPtr = (ObjListTask*)GetFistObjectList(&MailWorkerPtr->MailSendThrList);
    while(SelObjPtr)
    {
        SelSendMailPtr = (PARSENDMAIL*)SelObjPtr->UsedTask;
#ifdef _LINUX_X86_        
        SelSendMailPtr->NotifyPort = MailWorkerPtr->WebServMsgPort;
	    if (SendMailThreadCreate(SelSendMailPtr))
#endif
#ifdef WIN32
	    SelSendMailPtr->HTRSENDMAILSMTP = CreateThread( NULL, 0, 
		    (LPTHREAD_START_ROUTINE)THRSendMailClient,
		    (LPVOID)(SelSendMailPtr), 0, (LPDWORD)&SelSendMailPtr->ThrSendMailSmtpID);
	    if (SelSendMailPtr->HTRSENDMAILSMTP)
#endif
	    {
#ifdef _SERVDEBUG_
	        DebugLogPrint(NULL, "%s: Start send e-mail\r\n", MailWorkerName);
#endif
	        break;
	    }
	    else
        {
            DebugLogPrint(NULL, "%s: Fail to create email send thread\r\n", MailWorkerName);
	        if (SelSendMailPtr->MailBody) FreeMemory(SelSendMailPtr->MailBody);
	        FreeMemory(SelSendMailPtr);
	        RemStructList(&MailWorkerPtr->MailSendThrList, SelObjPtr);
	        SelObjPtr = (ObjListTask*)GetFistObjectList(&MailWorkerPtr->MailSendThrList);
	    }
    }
}
//---------------------------------------------------------------------------
void MailWorkerInit(MAIL_WORKER_INFO *MailWorkerPtr, char *LocalAddrIP, char *WebOwnerMail)
{
	PARMAILCLIENT *MailCfgPtr = NULL;

	MailWorkerPtr->isActive = true;
	MailCfgPtr = &MailWorkerPtr->MailClientCfg;
	MailWorkerPtr->MailSendThrList.Count = 0;
	MailWorkerPtr->MailSendThrList.CurrTask = NULL;
	MailWorkerPtr->MailSendThrList.FistTask = NULL;

    MailCfgPtr->SmtpEncodeType = SMTP_ENCODE_NONE;
	MailCfgPtr->SmtpIpPort = 25;
	MailCfgPtr->SmtpTimeout = 15;
	MailCfgPtr->MailSendInt = 60;
	strcpy(MailCfgPtr->LocalIpAddr, LocalAddrIP);
	strcpy(MailCfgPtr->SmtpServerName, "smtp.mail.ru");
    strcpy(MailCfgPtr->MailFrom, WebOwnerMail);
	strcpy(MailCfgPtr->MailLogin, "login");
	strcpy(MailCfgPtr->MailPasswd, "password");
}
//---------------------------------------------------------------------------
