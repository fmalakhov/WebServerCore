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

#include "SmsWorker.h"
#include "TrTimeOutThread.h"
#include "ThrReportMen.h"
#include "WebServInfo.h"

static char SmsWorkerName[] = "SmsWorker";

static void SetServTypePhoneNumConv(char *DestNumPtr, char *SrcNumPtr);
//---------------------------------------------------------------------------
bool SendSmsToServer(SMS_WORKER_INFO *SmsWorkerPtr, char *PhoneNum, bool isRusLang, 
					 unsigned char *ConfirmKey, unsigned char *SmsText,
                     unsigned int CriticalCount, unsigned int MajorCount)
{
	PARSENDSMS *ParSendSmsPtr;
	bool        Result;

	ParSendSmsPtr = (PARSENDSMS*)AllocateMemory(sizeof(PARSENDSMS));
    if (!ParSendSmsPtr)
    {
		DebugLogPrint(NULL, "%s: Fail to allocate memory for SMS info.\r\n", 
			SmsWorkerName);    
        return false;
    }
	ParSendSmsPtr->IDMessSendSmsRes = WSU_RESPSMSSENT;
#ifdef WIN32  
	ParSendSmsPtr->ParentThrID = GetCurrentThreadId();
#endif
	ParSendSmsPtr->SmsCfgPtr = &SmsWorkerPtr->SmsClientCfg;
	ParSendSmsPtr->Status = 0;
	ParSendSmsPtr->UrlPtr = SmsWorkerPtr->URL;
    if (ConfirmKey)
    {
	    ParSendSmsPtr->CriticalAlarmsCount = 0;
        ParSendSmsPtr->MajorAlarmsCount = 0;    
        ParSendSmsPtr->SmsText[0] = 0;
        strcpy(ParSendSmsPtr->ConfirmKey, (const char*)ConfirmKey);
    }
    else if (SmsText)
    {
	    ParSendSmsPtr->CriticalAlarmsCount = 0;
        ParSendSmsPtr->MajorAlarmsCount = 0;    
        ParSendSmsPtr->ConfirmKey[0] = 0;
        strcpy(ParSendSmsPtr->SmsText, (const char*)SmsText);
    }
    else
    {
        ParSendSmsPtr->ConfirmKey[0] = 0;
        ParSendSmsPtr->SmsText[0] = 0;
	    ParSendSmsPtr->CriticalAlarmsCount = CriticalCount;
        ParSendSmsPtr->MajorAlarmsCount = MajorCount;
    }
    SetServTypePhoneNumConv(ParSendSmsPtr->PhoneNum.PhoneNum, PhoneNum);
    ParSendSmsPtr->PhoneNum.isRusLangInfo = isRusLang;
#ifdef _LINUX_X86_
    ParSendSmsPtr->NotifyPort = SmsWorkerPtr->WebServMsgPort;
	if (SendSmsThreadCreate(ParSendSmsPtr))
#endif
#ifdef WIN32
	ParSendSmsPtr->HTRSENDSMS = CreateThread( NULL, 0, 
		(LPTHREAD_START_ROUTINE)ThrSendSmsClient,
		(LPVOID)(ParSendSmsPtr), 0, (LPDWORD)&ParSendSmsPtr->ThrSendSmsID);
	if (ParSendSmsPtr->HTRSENDSMS)
#endif
	{
	    AddStructList(&SmsWorkerPtr->SmsSendThrList, ParSendSmsPtr);
#ifdef _SERVDEBUG_
		DebugLogPrint(NULL, "%s: Start send sms for %s\r\n", SmsWorkerName, 
            ParSendSmsPtr->PhoneNum.PhoneNum);
#endif
		Result = true;
	}
	else
	{
		DebugLogPrint(NULL, "%s: Fail to create sms send thread\r\n", 
			SmsWorkerName);
		FreeMemory(ParSendSmsPtr);
		Result = false;
	}
	return Result;
}
//---------------------------------------------------------------------------
void HandleSmsThrDone(SMS_WORKER_INFO *SmsWorkerPtr, PARSENDSMS *ParSendSmsPtr)
{
	ObjListTask *SelObjPtr;
	PARSENDSMS  *SelSendSmsPtr;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&SmsWorkerPtr->SmsSendThrList);
	while(SelObjPtr)
	{
	    SelSendSmsPtr = (PARSENDSMS*)SelObjPtr->UsedTask;
		if (ParSendSmsPtr == SelSendSmsPtr)
		{
			if (SmsWorkerPtr->OnSmsSendStatus)
			{
			    if (ParSendSmsPtr->Status == 0) (SmsWorkerPtr->OnSmsSendStatus)(true);
			    else                            (SmsWorkerPtr->OnSmsSendStatus)(true);
			}

			DebugLogPrint(NULL, "%s: SMS delivery to %s number is finished with %d status\r\n", 
		    SmsWorkerName, ParSendSmsPtr->PhoneNum.PhoneNum, ParSendSmsPtr->Status);

		    FreeMemory(ParSendSmsPtr);
		    RemStructList(&SmsWorkerPtr->SmsSendThrList, SelObjPtr);
			break;
		}
	    SelObjPtr = (ObjListTask*)GetNextObjectList(&SmsWorkerPtr->SmsSendThrList);
	}
}
//---------------------------------------------------------------------------
static void SetServTypePhoneNumConv(char *DestNumPtr, char *SrcNumPtr)
{
    unsigned int len = 0;

    while((*SrcNumPtr > 0) && (len < MAX_LEN_SMS_PHONE_NUM))
    {
        /* Digits only can be copied */
        if ((*SrcNumPtr >= '0') && (*SrcNumPtr <= '9'))
        {
            *DestNumPtr++ = *SrcNumPtr;
            len++;
        }
        SrcNumPtr++;
    }
    *DestNumPtr = 0;
}
//---------------------------------------------------------------------------
void SmsWorkerInit(SMS_WORKER_INFO *SmsWorkerPtr)
{
	PARSMSCLIENT *SmsCfgPtr = NULL;

	SmsWorkerPtr->isActive = true;
	SmsCfgPtr = &SmsWorkerPtr->SmsClientCfg;

	SmsWorkerPtr->SmsSendThrList.Count = 0;
	SmsWorkerPtr->SmsSendThrList.CurrTask = NULL;
	SmsWorkerPtr->SmsSendThrList.FistTask = NULL;

	/* SMS server configuration initialization */
	SmsWorkerPtr->SmsClientCfg.SmsIpPort = 80;
    SmsWorkerPtr->SmsClientCfg.SmsTimeout = 15;
	strcpy(SmsWorkerPtr->SmsClientCfg.SmsServerName, "sms.ru");
	memset(&SmsWorkerPtr->SmsClientCfg.SmsSrcName[0], 0, MAX_LEN_SMS_SRC_LEN);
	memset(&SmsWorkerPtr->SmsClientCfg.AccessId[0], 0, MAX_LEN_SMS_SERVER_ACCESS_ID);

	SmsWorkerPtr->SmsClientCfg.SmsDestNumList.Count = 0;
	SmsWorkerPtr->SmsClientCfg.SmsDestNumList.CurrTask = NULL;
	SmsWorkerPtr->SmsClientCfg.SmsDestNumList.FistTask = NULL;

}
//---------------------------------------------------------------------------
