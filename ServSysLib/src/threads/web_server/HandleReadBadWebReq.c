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
#include "BaseWebServer.h"
#include "ThrReportMen.h"

extern char ThrWebServName[];
extern PARAMWEBSERV *ParWebServPtr;
extern char *MemWebPageGenPtr;
extern STATS_INFO ServerStats;

static void PrintBadHttpReqLog(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, char *IssueName);
//---------------------------------------------------------------------------
void HandleReadBadWebReq(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb)
{
    unsigned short WebPort;

    if (ParReadWeb->WebChanId == PRIMARY_WEB_CHAN)  WebPort = ParWebServ->ServCustomCfg.PrimWebAccIPPort;
    else                                            WebPort = ParWebServ->ServCustomCfg.SecondWebAccIPPort;
    
#ifdef WIN32
	DebugLogPrint(NULL,  "%s: Http request load is failed: IP:%d.%d.%d.%d; Code=%d, (Sct:%d)\r\n", 
		ThrWebServName, 
		(int)(ParReadWeb->HttpClientIP&0x000000ff), 
		(int)((ParReadWeb->HttpClientIP&0x0000ff00)>>8), 
		(int)((ParReadWeb->HttpClientIP&0x00ff0000)>>16),
		(int)((ParReadWeb->HttpClientIP&0xff000000)>>24),
		ParReadWeb->Status, ParReadWeb->HttpSocket);
#else
	DebugLogPrint(NULL, "%s: Http request load is failed: IP:%d.%d.%d.%d; Code=%d, (Sct:%d)\r\n", 
		ThrWebServName, 

  #ifdef _SUN_BUILD_
                (int)((ParReadWeb->HttpClientIP&0xff000000)>>24),
                (int)((ParReadWeb->HttpClientIP&0x00ff0000)>>16),
                (int)((ParReadWeb->HttpClientIP&0x0000ff00)>>8),
		(int)(ParReadWeb->HttpClientIP&0x000000ff), 
		ParReadWeb->Status, ParReadWeb->HttpSocket);
  #else
 		(int)(ParReadWeb->HttpClientIP&0x000000ff), 
		(int)((ParReadWeb->HttpClientIP&0x0000ff00)>>8), 
		(int)((ParReadWeb->HttpClientIP&0x00ff0000)>>16),
		(int)((ParReadWeb->HttpClientIP&0xff000000)>>24),
		ParReadWeb->Status, ParReadWeb->HttpSocket);

  #endif
#endif
	switch(ParReadWeb->Status)
	{
		case URP_INV_FILE_NAME:
			PrintBadHttpReqLog(ParWebServ, ParReadWeb, "Invalid file name");
	        CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift], 
				ParWebServ->LocalAddrIP, WebPort);
			ParReadWeb->isKeepAlive = false;
			if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
				(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
			break;

		case URP_NOT_SUPP_CONT_TYPE:
			PrintBadHttpReqLog(ParWebServ, ParReadWeb, "Wrong content type");
			CreateRespWrongData(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
			ParReadWeb->isKeepAlive = false;
			if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
				(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
		    break;

		case URP_CONNECT_CLOSE_ERROER:
		case URP_HTTP_RX_TIMEOUT:
			CloseHttpReader(&ParWebServ->ReaderWorker, ParReadWeb);
			break;

		case URP_FILE_NAME_TOO_LONG:
			PrintBadHttpReqLog(ParWebServ, ParReadWeb, "Too long name");
	        CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				ParWebServ->LocalAddrIP, WebPort);
			ParReadWeb->isKeepAlive = false;
			if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
				(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
			break;

		case URP_NOT_SUPP_FILE_TYPE:
			PrintBadHttpReqLog(ParWebServ, ParReadWeb, "Wrong file type");
	        CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				ParWebServ->LocalAddrIP, WebPort);
			ParReadWeb->isKeepAlive = false;
			if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
				(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
			break;

		case URP_HTTP_REQ_DATA_LARGE:
			PrintBadHttpReqLog(ParWebServ, ParReadWeb, "Large data");
			CreateRespTooLargeData(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
			ParReadWeb->isKeepAlive = false;
			if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
				(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
			break;

		case URP_NOT_SUPPORT_METHOD:
			PrintBadHttpReqLog(ParWebServ, ParReadWeb, "Unknown method req");
			CreateRespNoAcess(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
			ParReadWeb->isKeepAlive = false;
			if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
				(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
			break;

		default:
			PrintBadHttpReqLog(ParWebServ, ParReadWeb, "Other wrong:");
			CreateRespNoAcess(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
			ParReadWeb->isKeepAlive = false;
			if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
				(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
			break;
	}   
	ServerStats.IntBadUserReq++;
}
//---------------------------------------------------------------------------
static void PrintBadHttpReqLog(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, char *IssueName)
{
	char IpAddrBuf[32];

    if (ParReadWeb->HttpReqPtr)
	{
		SetIpAddrToString(&IpAddrBuf[0], ParReadWeb->HttpClientIP);
		DebugLogStrBufPrint(NULL, ParReadWeb->HttpReqPtr, "%s: %s: IP:%s; ", 
		    ThrWebServName, IssueName, &IpAddrBuf[0]);
	    FreeMemory(ParReadWeb->HttpReqPtr);
		ParReadWeb->HttpReqPtr = NULL;
	}
}
//---------------------------------------------------------------------------
