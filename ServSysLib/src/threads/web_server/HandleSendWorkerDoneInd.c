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
#include "BaseWebServer.h"

extern char ThrWebServName[];
//---------------------------------------------------------------------------
void HandleSendWorkerDoneInd(PARAMWEBSERV *ParWebServ, InterThreadInfoMsg* WebServMsgPtr)
{
	SENDWEBSOCK     *ParSendWeb = NULL;
    CONNWEBSOCK     *ParConnWeb = NULL;
    
#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Begin HandlerWebServerReq function (WSU_RESPSENDCMPLT)\r\n", ThrWebServName);
#endif
	ParSendWeb = (SENDWEBSOCK*)WebServMsgPtr->WParam;
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
	if (ParSendWeb->ReadWebPtr && ParSendWeb->isDeliverySuccess)
	{        
        if (ParSendWeb->ReadWebPtr->WebChanId == PRIMARY_WEB_CHAN)  ParConnWeb = &ParWebServ->PrimConnWeb;
        else                                                        ParConnWeb = &ParWebServ->SecondConnWeb;    
        HandleNextReqKeepAlive(ParConnWeb, ParSendWeb->ReadWebPtr);
	}
	else
	{
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
			FreeReadPoolBuf(&ParWebServ->ReaderWorker, ParSendWeb->ReadWebPtr);
			ParSendWeb->ReadWebPtr = NULL;
		}
	}
	FreeBuffer(&ParWebServ->SenderWorker.SendWebPool, (POOL_RECORD_STRUCT*)ParSendWeb->SendBufPoolPtr);
}
//---------------------------------------------------------------------------
