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
#include "BadIpHash.h"
#include "ImageNameHash.h"
#include "HtmlPageHash.h"
#include "HtmlMacrosHash.h"

extern char ThrWebServName[];
extern PARAMWEBSERV *ParWebServPtr;
extern char *MemWebPageGenPtr;
//---------------------------------------------------------------------------
bool HandlerStopServerReq(PARAMWEBSERV *ParWebServ, InterThreadInfoMsg* WebServMsgPtr)
{
	READWEBSOCK        *ParReadWeb = NULL;
	SENDWEBSOCK        *ParSendWeb = NULL;
	SENDWEBSOCK        *ParSendWebPtr = NULL;
	bool               NeedsWait = true;

    switch (WebServMsgPtr->MsgTag)
    {
		case WSU_CONNECTUSER:
    #ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Enter HandlerStopServerReq function (%d) (WSU_CONNECTUSER)\r\n", 
				ThrWebServName, ((READWEBSOCK*)WebServMsgPtr->WParam)->HttpSocket);
    #endif
			HandleNewUserConnect(ParWebServ, (READWEBSOCK*)WebServMsgPtr->WParam);
			break;

		case WSU_USERDATA:
			ParReadWeb = (READWEBSOCK*)WebServMsgPtr->WParam;
    #ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Enter HandlerStopServerReq function (%d) (WSU_USERDATA)\r\n",
				ThrWebServName, ParReadWeb->HttpSocket);
    #endif
			HttpLoadRemList(&ParWebServPtr->ReaderWorker, ParReadWeb);
			if(ParReadWeb->Status != URP_SUCCESS)
			{
				HandleReadBadWebReq(ParWebServ, ParReadWeb);
				break;
			}
            if (ParReadWeb->WebChanId == PRIMARY_WEB_CHAN)
            {
			    CreateRespServerShutdown(MemWebPageGenPtr, ParWebServ->LocalAddrIP, 
				    ParWebServ->ServCustomCfg.PrimWebAccIPPort);
            }
            else
            {
			    CreateRespServerShutdown(MemWebPageGenPtr, ParWebServ->LocalAddrIP, 
				    ParWebServ->ServCustomCfg.SecondWebAccIPPort);            
            }
            SendHttpPage(ParReadWeb, MemWebPageGenPtr);
            CloseHttpReader(&ParWebServ->ReaderWorker, ParReadWeb);
			break;

		case WSU_RESPSENDCMPLT:                
			ParSendWeb = (SENDWEBSOCK*)WebServMsgPtr->WParam;
    #ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Enter HandlerStopServerReq function (WSU_RESPSENDCMPLT)\r\n", ThrWebServName);
    #endif
			HttpSentRemList(&ParWebServ->SenderWorker, ParSendWeb);
			ParWebServ->SentRespDoneCount++;
			if(!ParSendWeb->isDeliverySuccess)
			{
				DebugLogPrint(NULL, "%s: Http data delivery is failed (%d)\r\n", 
					ThrWebServName, ParSendWeb->HttpSocket);
			}
#ifdef _SERVDEBUG_
			else
			{
				DebugLogPrint(NULL, "%s: HTML data delivery is done Time: %d (%d)\r\n", 
					ThrWebServName, (unsigned int)(GetTickCount() - ParSendWeb->PageProcessStartTick),
					ParSendWeb->HttpSocket);
			}
#endif
            
            if (ParSendWeb->SslPtr)
            {
                SSL_set_shutdown(ParSendWeb->SslPtr, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
                SSL_free(ParSendWeb->SslPtr);
            }
            
            ParSendWeb->SslPtr = NULL;
		    CloseHttpSocket(ParSendWeb->HttpSocket);
		    ParSendWeb->HttpSocket = -1;
		    if (ParSendWeb->ReadWebPtr)
		    {
                ParSendWeb->ReadWebPtr->SslPtr = NULL;
			    FreeReadPoolBuf(&ParWebServ->ReaderWorker, ParSendWeb->ReadWebPtr);
			    ParSendWeb->ReadWebPtr = NULL;
		    }
	        FreeBuffer(&ParWebServ->SenderWorker.SendWebPool, (POOL_RECORD_STRUCT*)ParSendWeb->SendBufPoolPtr);
			break;

        case WSU_STATEWEBSERV:
            break;

        case WSU_HBACTIONTHR:
            switch((unsigned int)(tulong)WebServMsgPtr->LParam)
            {
                case WHN_READER_IND:
			        DebugLogPrint(NULL, "%s: Received HB message from reader thread (Port: %u)\r\n", 
				        ThrWebServName, ((READER_WEB_INFO*)WebServMsgPtr->WParam)->NotifyPort);                
                    break;
                    
                default: // WHN_SENDER_IND:
			        DebugLogPrint(NULL, "%s: Received HB message from sender thread (Port: %u)\r\n", 
				        ThrWebServName, ((SENDER_WEB_INFO*)WebServMsgPtr->WParam)->NotifyPort);
                    break;
            }
            break;
            
		case TM_ONTIMEOUT:
			switch((unsigned int)(tulong)WebServMsgPtr->WParam)
			{
			    case INTER_MAIL_DELAY_TMR_ID:
					HandleTimeoutNextMailSent(ParWebServ);
				    break;

				case USER_SESSION_ACTIVITY_TMR_ID:
					HandleSessionActivTmrExp(&ParWebServ->SessionManager);
					break;

				case STATS_CHECK_TMR_ID:
					break;

				default:
					HandleSessionTimerExp(&ParWebServ->SessionManager, (unsigned int)(tulong)WebServMsgPtr->WParam);
					break;
			}
			break;

		case WSU_RESPSMSSENT:
            HandleSmsThrDone(&ParWebServ->SmsWorker, (PARSENDSMS*)WebServMsgPtr->WParam);
			break;

		case WSU_RESPMAILSENT:
			HandleMailThrDone(&ParWebServ->MailWorker, (PARSENDMAIL*)WebServMsgPtr->WParam);
			break;

        default:
			DebugLogPrint(NULL, "%s: received unexpected message %d\r\n", 
				ThrWebServName, WebServMsgPtr->MsgTag);
            break;
    }
	if (!ParWebServ->SmsWorker.SmsSendThrList.Count &&
		!ParWebServ->MailWorker.MailSendThrList.Count &&
		!ParWebServ->SenderWorker.HttpSendThrList.Count &&
		!GetNumberActiveReaders(&ParWebServ->ReaderWorker)) NeedsWait = false;
#ifdef _SERVDEBUG_
    DebugLogPrint(NULL, "%s: Leave HandlerStopServerReq function (%u, %u, %u, %u - Wait=%d)\r\n", 
        ThrWebServName, (unsigned int)ParWebServ->SmsWorker.SmsSendThrList.Count,
        (unsigned int)ParWebServ->MailWorker.MailSendThrList.Count,
        (unsigned int)ParWebServ->SenderWorker.HttpSendThrList.Count, 
        GetNumberActiveReaders(&ParWebServ->ReaderWorker), (unsigned char)NeedsWait);
#endif
	return NeedsWait;
}
//---------------------------------------------------------------------------
