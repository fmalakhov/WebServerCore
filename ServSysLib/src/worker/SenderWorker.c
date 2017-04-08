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

#include "SenderWorker.h"
#include "BaseWebServer.h"
#include "ThrReportMen.h"

extern char CapchaFileIdName[];
extern char CsvTablePathBaseName[];
extern char WebServerName[];

static char HTTP_200_OK[] = "HTTP/1.1 200 OK\r\n";
static char HTTP_304_NOT_MODIF[] = "HTTP/1.1 304 Not Modified\r\n";
static char HTTP_RESP_SERVER[] = "Server: ";
static char HTTP_RESP_CONT_LEN[] = "Content-Length: ";
static char HTTP_RESP_CON_KEEP_ALIVE[] = "Connection: keep-alive\r\n";
static char HTTP_RESP_CON_CLOSE[] = "Connection: close\r\n";
static char HTTP_RESP_ACCEPT_RANGE[] = "\r\nAccept-Range: bytes\r\n";
static char HTTP_RESP_CACHE_CNTR_PRVT[] = "Cache-Control: private\r\n";
static char HTTP_RESP_CONTENT_TYPE[] = "Content-Type: ";
static char HTTP_RESP_CT_PNG[] = "image/png\r\n";
static char HTTP_RESP_CT_JPG[] = "image/jpg\r\n";
static char HTTP_RESP_CT_GIF[] = "image/gif\r\n";
static char HTTP_RESP_CT_HTML[] = "text/html;charset=windows-1251\r\n";
static char HTTP_RESP_VARY[] = "Vary:accept-charset\r\n\r\n";
static char HTTP_RESP_CC_MAX_AGE[] = "Cache-Control: max-age=864000\r\n";
static char HTTP_RESP_CC_NO_CACHE[] = "Cache-Control: no-cache\r\n";
static char HTTP_RESP_CONT_ENC_GZIP[] = "Content-Encoding: gzip\r\n";
static char HTTP_RESP_CONT_DISTR[] = "Content-Disposition: attachment;filename=";
static char HTTP_RESP_ETAG_1[] = "ETag: \"";
static char HTTP_RESP_ETAG_2[] = "-iss-windows-1251\"\r\n";
static char HTTP_RESP_GZIP_LEN[] =  "        \r\n";
//---------------------------------------------------------------------------
void StopSendThreads(SENDER_WORKER_INFO *SenderWorkerPtr)
{      
    ObjListTask		 *PointTask = NULL;
	SENDER_WEB_INFO  *SenderInfoPtr = NULL;
 
	PointTask = (ObjListTask*)GetFistObjectList(&SenderWorkerPtr->SenderThrList);
	while(PointTask)
	{
		SenderInfoPtr = (SENDER_WEB_INFO*)PointTask->UsedTask;
		if (SenderInfoPtr->isThreadReady) StopSenderThread(SenderInfoPtr);
        FreeMemory(SenderInfoPtr);
		RemStructList(&SenderWorkerPtr->SenderThrList, PointTask);
		PointTask = (ObjListTask*)GetFistObjectList(&SenderWorkerPtr->SenderThrList);
	}
}
//---------------------------------------------------------------------------
void CreateSenderThreads(SENDER_WORKER_INFO *SenderWorkerPtr)
{
	unsigned int index;
	unsigned int InitCreateCount = 0;
	SENDWEBSOCK *ParSendWeb = NULL;
	SENDER_WEB_INFO    *SenderInfoPtr = NULL;
    ObjListTask		   *PointTask = NULL;
    POOL_RECORD_STRUCT *SenderPoolRecPtr[START_SIMULT_WEB_SENT_HDR+1];

	InitCreateCount = SenderWorkerPtr->SendWebPool.m_NumUsedRecords;
    if (InitCreateCount >= START_SIMULT_WEB_SENT_HDR) return;
    for(;;)
    {
		SenderPoolRecPtr[InitCreateCount] = GetBuffer(&SenderWorkerPtr->SendWebPool);
        ParSendWeb = (SENDWEBSOCK*)SenderPoolRecPtr[InitCreateCount]->DataPtr;
        ParSendWeb->SendBufPoolPtr = (void*)SenderPoolRecPtr[InitCreateCount];
	    ParSendWeb->HttpRespPtr = NULL;
	    ParSendWeb->isBodyCompress = false;
	    ParSendWeb->isHashData = true;
		InitCreateCount++;
		if (InitCreateCount == START_SIMULT_WEB_SENT_HDR)
		{
			Sleep(10);
			printf("%d sender tasks are ready to handle requests\n", 
				START_SIMULT_WEB_SENT_HDR);
			break;
		}
    }

	for (index=0;index < InitCreateCount;index++)
	    FreeBuffer(&SenderWorkerPtr->SendWebPool, SenderPoolRecPtr[index]);

	/* List of sender worker threads creation */
	for(index=0; index < CREATE_SIMULT_WEB_SENT_THR;index++)
	{
		SenderInfoPtr = (SENDER_WEB_INFO*)AllocateMemory(sizeof(SENDER_WEB_INFO));
		if (!SenderInfoPtr)
		{
			DebugLogPrint(NULL, "SenderWorker: Fail to memory allocation for sender worker instance\r\n");
			break;
		}
		memset(SenderInfoPtr, 0, sizeof(SENDER_WEB_INFO));
	    SenderInfoPtr->IDMessSendWeb = WSU_RESPSENDCMPLT;
#ifdef WIN32
        SenderInfoPtr->MsgNotifyId = SenderWorkerPtr->NextSendNotifyId;
		SenderWorkerPtr->NextSendNotifyId++;
#else
        SenderInfoPtr->NotifyPort = SenderWorkerPtr->NextSendNotifyPort;
        SenderInfoPtr->WebServPort = SenderWorkerPtr->WebServMsgPort;
		SenderWorkerPtr->NextSendNotifyPort++;
#endif
	    if (SendWebThreadCreate(SenderInfoPtr)) SenderInfoPtr->isThreadReady = true;
	    else
		{
		    DebugLogPrint(NULL, "SenderWorker: Fail to create HTTP send thread\r\n");
			break;
		}
		AddStructList(&SenderWorkerPtr->SenderThrList, SenderInfoPtr);
    }

	PointTask = (ObjListTask*)GetFistObjectList(&SenderWorkerPtr->SenderThrList);
	SenderWorkerPtr->SelSendWebInfoPtr = (SENDER_WEB_INFO*)PointTask->UsedTask;
}
//---------------------------------------------------------------------------
void SenderWorkerInit(SENDER_WORKER_INFO *SenderWorkerPtr)
{
	SenderWorkerPtr->isActive = true;
	SenderWorkerPtr->HttpSendThrList.Count = 0;
	SenderWorkerPtr->HttpSendThrList.CurrTask = NULL;
	SenderWorkerPtr->HttpSendThrList.FistTask = NULL;
	SenderWorkerPtr->SenderThrList.Count = 0;
	SenderWorkerPtr->SenderThrList.CurrTask = NULL;
	SenderWorkerPtr->SenderThrList.FistTask = NULL;
	CreatePool(&SenderWorkerPtr->SendWebPool, START_SIMULT_WEB_SENT_HDR, sizeof(SENDWEBSOCK));
	CreateSenderThreads(SenderWorkerPtr);
}
//---------------------------------------------------------------------------
void SenderWorkerClose(SENDER_WORKER_INFO *SenderWorkerPtr)
{
	if (!SenderWorkerPtr->isActive) return;
	StopSendThreads(SenderWorkerPtr);
	DestroyPool(&SenderWorkerPtr->SendWebPool);
}
//---------------------------------------------------------------------------
bool SendHttpFileHost(SENDER_WORKER_INFO *SenderWorkerPtr, READWEBSOCK *ReadWeb,
	char *HttpRespPtr, unsigned int HeaderLen, unsigned int DataLen)
{
	register SENDWEBSOCK *ParSendWeb = NULL;
	POOL_RECORD_STRUCT *SendWebBufPtr = NULL;

	SendWebBufPtr = GetBuffer(&SenderWorkerPtr->SendWebPool);
	if (!SendWebBufPtr)
    {
	    DebugLogPrint(NULL, "SenderWorker: Failed buffer allocation for data file transfer sent %d\r\n");
        return false;
    }
	ParSendWeb = (SENDWEBSOCK*)SendWebBufPtr->DataPtr;
    ParSendWeb->SendBufPoolPtr = (void*)SendWebBufPtr;

	ParSendWeb->HttpRespPtr = NULL;
	ParSendWeb->StartTick = GetTickCount();
    ParSendWeb->isBodyCompress = false;
	ParSendWeb->isHashData = false;
	ParSendWeb->SslPtr = ReadWeb->SslPtr;
	ParSendWeb->HttpSocket = ReadWeb->HttpSocket;
	ParSendWeb->PageProcessStartTick = ReadWeb->StartReqHandleTick;
	ParSendWeb->isDeliverySuccess = false;
	ParSendWeb->FileType = ReadWeb->FileType;
	ParSendWeb->HTTPRespLen = DataLen;
	if (ReadWeb->ReqestType == HTR_HEAD) ParSendWeb->isHeaderReq = true;
	else                                 ParSendWeb->isHeaderReq = false;
    
    memcpy(ParSendWeb->HttpRespHeader, HttpRespPtr, HeaderLen);
    ParSendWeb->HttpRespHeader[HeaderLen] = 0;
    ParSendWeb->HeaderLen = HeaderLen;

	ParSendWeb->HttpRespPtr = (char*)AllocateMemory(ParSendWeb->HTTPRespLen+1);
	if (!ParSendWeb->HttpRespPtr)
	{
	    DebugLogPrint(NULL, "SenderWorker: Failed memory allocation for http file body copy response %d\r\n");
		FreeBuffer(&SenderWorkerPtr->SendWebPool, (POOL_RECORD_STRUCT*)ParSendWeb->SendBufPoolPtr);
		return false;
	}
	memcpy(ParSendWeb->HttpRespPtr, &HttpRespPtr[HeaderLen], ParSendWeb->HTTPRespLen);

	if ((SenderWorkerPtr->KeepAliveEnable) &&
		(ReadWeb->isKeepAlive)) ParSendWeb->ReadWebPtr = ReadWeb;
    else                        ParSendWeb->ReadWebPtr = NULL;

	return SendHtmlDataUser(SenderWorkerPtr, ParSendWeb);
}
//---------------------------------------------------------------------------
bool SendHttpRespUser(SENDER_WORKER_INFO *SenderWorkerPtr, READWEBSOCK *ReadWeb,
	char *HttpRespPtr, unsigned int DataLen)
{
	register SENDWEBSOCK *ParSendWeb = NULL;
	POOL_RECORD_STRUCT *SendWebBufPtr = NULL;

	SendWebBufPtr = GetBuffer(&SenderWorkerPtr->SendWebPool);
	if (!SendWebBufPtr) return false;
	ParSendWeb = (SENDWEBSOCK*)SendWebBufPtr->DataPtr;
    ParSendWeb->SendBufPoolPtr = (void*)SendWebBufPtr;

	ParSendWeb->HttpRespPtr = NULL;
	ParSendWeb->ActListObjPtr = NULL;
	ParSendWeb->StartTick = GetTickCount();
    ParSendWeb->isBodyCompress = false;
	ParSendWeb->isHashData = false;
	ParSendWeb->isHeaderReq = false;
	ParSendWeb->SslPtr = ReadWeb->SslPtr;
	if (!SenderWorkerPtr->KeepAliveEnable || !ReadWeb->isKeepAlive) ReadWeb->SslPtr = NULL;
	ParSendWeb->HttpSocket = ReadWeb->HttpSocket;
	ParSendWeb->PageProcessStartTick = ReadWeb->StartReqHandleTick;
	ParSendWeb->isDeliverySuccess = false;
	ParSendWeb->FileType = FRT_HTML_PAGE;
	ParSendWeb->HTTPRespLen = DataLen;
	ParSendWeb->HeaderLen = 0;

	ParSendWeb->HttpRespPtr = (char*)AllocateMemory(ParSendWeb->HTTPRespLen+1);
	if (!ParSendWeb->HttpRespPtr)
	{
		FreeBuffer(&SenderWorkerPtr->SendWebPool, (POOL_RECORD_STRUCT*)ParSendWeb->SendBufPoolPtr);
		return false;
	}
	memcpy(ParSendWeb->HttpRespPtr, HttpRespPtr, ParSendWeb->HTTPRespLen);

	if ((SenderWorkerPtr->KeepAliveEnable) &&
		(ReadWeb->isKeepAlive)) ParSendWeb->ReadWebPtr = ReadWeb;
    else						ParSendWeb->ReadWebPtr = NULL;

	return SendHtmlDataUser(SenderWorkerPtr, ParSendWeb);
}
//---------------------------------------------------------------------------
bool SendHtmlDataUser(SENDER_WORKER_INFO *SenderWorkerPtr, SENDWEBSOCK *ParSendWeb)
{
	SENDER_WEB_INFO    *SenderInfoPtr = NULL;

	SenderInfoPtr = GetSenderInstance(SenderWorkerPtr);
	if (!SenderInfoPtr)
	{
		DebugLogPrint(NULL, "SenderWorker: Fail to get HTTP send thread (%d)\r\n", 
			ParSendWeb->HttpSocket);
		FreeMemory(ParSendWeb->HttpRespPtr);
        FreeBuffer(&SenderWorkerPtr->SendWebPool, (POOL_RECORD_STRUCT*)ParSendWeb->SendBufPoolPtr);
		return false;
	}
	ParSendWeb->ActListObjPtr = AddStructListObj(&SenderWorkerPtr->HttpSendThrList, ParSendWeb);
	if (SenderWorkerPtr->OnAddPageSendCB)
			(SenderWorkerPtr->OnAddPageSendCB)(SenderWorkerPtr->DataPtr);
	WebDataSentReq(SenderInfoPtr, ParSendWeb);
#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "SenderWorker: Complete handel SendHtmlDataUser\r\n", ThrWebServName);
#endif
	return true;
}
//---------------------------------------------------------------------------
void HttpSentRemList(SENDER_WORKER_INFO *SenderWorkerPtr, SENDWEBSOCK *ParSendWeb)
{
	if (ParSendWeb->ActListObjPtr)
	    RemStructList(&SenderWorkerPtr->HttpSendThrList, ParSendWeb->ActListObjPtr);
	ParSendWeb->ActListObjPtr = NULL;
}
//---------------------------------------------------------------------------
void SendFileNotModifyedTime(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, time_t LastModifyed, unsigned int ETagId)
{
	bool          isKeepAlive = false;
	unsigned int  len;
    CONNWEBSOCK   *ParConnWeb = NULL;
	char          FormMess[128];
	char          MsgBuf[1024];

    //Preparing header transmit file.
	EndHtmlPageGenPtr = MsgBuf;
	*MsgBuf = 0;
    AddLenStrWebPage(HTTP_304_NOT_MODIF, (sizeof(HTTP_304_NOT_MODIF)-1));
	AddLenStrWebPage(HTTP_RESP_ETAG_1, (sizeof(HTTP_RESP_ETAG_1)-1));
	if (!ETagId) AddLenStrWebPage("fe00fe", 6);
	{
        sprintf(FormMess,"%06x", ETagId);
		AddLenStrWebPage(FormMess, 6);
	}
	AddLenStrWebPage(HTTP_RESP_ETAG_2, (sizeof(HTTP_RESP_ETAG_2)-1));
	if ((ParReadWeb->FileType != FRT_CSV_DATA) && 
        (ParReadWeb->FileType != FRT_PDF_DOC) &&
		(ParReadWeb->FileType != FRT_CAPCHA))
	{
	    AddLenStrWebPage(HTTP_RESP_CC_MAX_AGE, (sizeof(HTTP_RESP_CC_MAX_AGE)-1)); /* Files expiration is 10 days */
	}
	else
	{
		AddLenStrWebPage(HTTP_RESP_CC_NO_CACHE, (sizeof(HTTP_RESP_CC_NO_CACHE)-1));
	}
#ifndef WIN32
	SetLastModifyedDate(FormMess, LastModifyed);
#endif
	AddStrWebPage(FormMess);
	AddStrWebPage(ParWebServ->ReqTimeStr);

	if ((ParWebServ->GeneralCfg.KeepAliveEnable) &&
		ParReadWeb->isKeepAlive)
	{
		AddLenStrWebPage(HTTP_RESP_CON_KEEP_ALIVE, (sizeof(HTTP_RESP_CON_KEEP_ALIVE)-1));
		isKeepAlive = true;
	}
    else
	{
		AddLenStrWebPage(HTTP_RESP_CON_CLOSE, (sizeof(HTTP_RESP_CON_CLOSE)-1));
	}

	AddLenStrWebPage("\r\n", 2);
	len = (unsigned int)(EndHtmlPageGenPtr - MsgBuf);

	if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, (char*)MsgBuf, len))
	{
		CloseHttpChan(ParReadWeb);
		ParReadWeb->isKeepAlive = false;
	}
	else
	{
		ParWebServ->SentRespDoneCount++;
	}
}
//---------------------------------------------------------------------------
void SendFileNotModifyedStr(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, char *LastModifyed, unsigned int ETagId)
{
	bool          isKeepAlive = false;
	unsigned int  len;
    CONNWEBSOCK   *ParConnWeb = NULL;
	char          FormMess[128];
	char          MsgBuf[1024];

    //Preparing header transmit file.
	EndHtmlPageGenPtr = MsgBuf;
	*MsgBuf = 0;
    AddLenStrWebPage(HTTP_304_NOT_MODIF, (sizeof(HTTP_304_NOT_MODIF)-1));
	AddLenStrWebPage(HTTP_RESP_ETAG_1, (sizeof(HTTP_RESP_ETAG_1)-1));
	if (!ETagId) AddLenStrWebPage("fe00fe", 6);
	{
        sprintf(FormMess,"%06x", ETagId);
		AddLenStrWebPage(FormMess, 6);
	}
	AddLenStrWebPage(HTTP_RESP_ETAG_2, (sizeof(HTTP_RESP_ETAG_2)-1));
	if ((ParReadWeb->FileType != FRT_CSV_DATA) && 
        (ParReadWeb->FileType != FRT_PDF_DOC) &&
		(ParReadWeb->FileType != FRT_CAPCHA))
	{
	    AddLenStrWebPage(HTTP_RESP_CC_MAX_AGE, (sizeof(HTTP_RESP_CC_MAX_AGE)-1)); /* Files expiration is 10 days */
	}
	else
	{
		AddLenStrWebPage(HTTP_RESP_CC_NO_CACHE, (sizeof(HTTP_RESP_CC_NO_CACHE)-1));
	}

	AddStrWebPage(LastModifyed);
	AddStrWebPage(ParWebServ->ReqTimeStr);

	if ((ParWebServ->GeneralCfg.KeepAliveEnable) &&
		ParReadWeb->isKeepAlive)
	{
		AddLenStrWebPage(HTTP_RESP_CON_KEEP_ALIVE, (sizeof(HTTP_RESP_CON_KEEP_ALIVE)-1));
		isKeepAlive = true;
	}
    else
	{
		AddLenStrWebPage(HTTP_RESP_CON_CLOSE, (sizeof(HTTP_RESP_CON_CLOSE)-1));
	}

	AddLenStrWebPage("\r\n", 2);
	len = (unsigned int)(EndHtmlPageGenPtr - MsgBuf);

	if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, (char*)MsgBuf, len))
	{
		CloseHttpChan(ParReadWeb);
		ParReadWeb->isKeepAlive = false;
	}
	else
	{
		ParWebServ->SentRespDoneCount++;
	}
}
//---------------------------------------------------------------------------
bool SendHttpHashFileHost(PARAMWEBSERV *ParWebServ, READWEBSOCK *ReadWeb, FILE_HASH_RECORD *HashFilePtr)
{
	register SENDWEBSOCK *ParSendWeb = NULL;
	POOL_RECORD_STRUCT *SendWebBufPtr = NULL;

	SendWebBufPtr = GetBuffer(&ParWebServ->SenderWorker.SendWebPool);
	if (!SendWebBufPtr) return false;
	ParSendWeb = (SENDWEBSOCK*)SendWebBufPtr->DataPtr;
    ParSendWeb->SendBufPoolPtr = (void*)SendWebBufPtr;

	ParSendWeb->HttpRespPtr = NULL;
	ParSendWeb->isBodyCompress = false;
	ParSendWeb->isHashData = true;
	ParSendWeb->FileType = ReadWeb->FileType;
	ParSendWeb->StartTick = GetTickCount();
	ParSendWeb->SslPtr = ReadWeb->SslPtr;
	if (!ParWebServ->GeneralCfg.KeepAliveEnable || !ReadWeb->isKeepAlive) ReadWeb->SslPtr = NULL;
	if (ReadWeb->ReqestType == HTR_HEAD) ParSendWeb->isHeaderReq = true;
	else                                 ParSendWeb->isHeaderReq = false;
	ParSendWeb->HttpSocket = ReadWeb->HttpSocket;
	ParSendWeb->PageProcessStartTick = ReadWeb->StartReqHandleTick;
	ParSendWeb->isDeliverySuccess = false;
	ParSendWeb->HTTPRespLen = HashFilePtr->FileLen;
	ParSendWeb->HttpRespPtr = (char*)HashFilePtr->FileBodyBuf;

	if ((ReadWeb->FileType == FRT_JS_SCRIPT) &&
		(ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
		(HashFilePtr->FileLen > ParWebServ->GeneralCfg.MinHtmlSizeCompress) &&
		(ReadWeb->isEncodingAccept))
	{
		strcpy(ParSendWeb->FileName, "script.js");
		ParSendWeb->isBodyCompress = true;
	}
	else if ((ReadWeb->FileType == FRT_XML_DATA) &&
		    (ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
		    (HashFilePtr->FileLen > ParWebServ->GeneralCfg.MinHtmlSizeCompress) &&
		    (ReadWeb->isEncodingAccept) && (ReadWeb->BotType != BOT_YANDEX))
	{
		strcpy(ParSendWeb->FileName, "sitemap.xml");
		ParSendWeb->isBodyCompress = true;
	}
	else if (ReadWeb->FileType == FRT_CAPCHA)
	{
		strcpy(ParSendWeb->FileName, CapchaFileIdName);
	}
	/*
	else if ((ReadWeb->FileType == FRT_JPG_PIC) &&
		    (ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
		    (HashFilePtr->FileLen > ParWebServ->GeneralCfg.MinHtmlSizeCompress) &&
		    (ReadWeb->isEncodingAccept) && (ReadWeb->BotType != BOT_YANDEX))
	{
		strcpy(ParSendWeb->FileName, "image.jpg");
		ParSendWeb->isBodyCompress = true;
	}
*/
	if ((ParWebServ->GeneralCfg.KeepAliveEnable) && (ReadWeb->isKeepAlive)) 
		                      ParSendWeb->ReadWebPtr = ReadWeb;
    else                      ParSendWeb->ReadWebPtr = NULL;

	ParSendWeb->HeaderBeginSetLenPtr = DataFileHttpSentHeaderGen(ParWebServ, ReadWeb, &ParSendWeb->HttpRespHeader[0], 
		HashFilePtr->FileLen, ReadWeb->FileType, HashFilePtr->FileEtag, 
		ReadWeb->isKeepAlive, ParSendWeb->isBodyCompress);
	ParSendWeb->HeaderLen = (unsigned int)(EndHtmlPageGenPtr - &ParSendWeb->HttpRespHeader[0]);

    return SendHtmlDataUser(&ParWebServ->SenderWorker, ParSendWeb);
}
//---------------------------------------------------------------------------
char* DataFileHttpSentHeaderGen(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, char *BufPtr, unsigned int SizeFile, 
    unsigned char FileType, unsigned int ETagId, bool isKeepAlive, bool isCompressBody)
{
    unsigned int  i;
#ifdef WIN32
	SYSTEMTIME    CurrTime;
#else
    struct timeb  hires_cur_time;
    struct tm     *cur_time;
#endif
	char          *StartLenPtr = NULL;
	char          FormMess[128];

    //Preparing header transmit file.
	EndHtmlPageGenPtr = BufPtr;
	*BufPtr = 0;
	*FormMess = 0;
    AddLenStrWebPage(HTTP_200_OK, (sizeof(HTTP_200_OK)-1));
	if ((ParReadWeb->FileType != FRT_CSV_DATA) && 
        (ParReadWeb->FileType != FRT_PDF_DOC) &&
		(ParReadWeb->FileType != FRT_CAPCHA))
	    AddLenStrWebPage(HTTP_RESP_CC_MAX_AGE, (sizeof(HTTP_RESP_CC_MAX_AGE)-1)); /* Files expiration is 10 days */
	AddStrWebPage(ParWebServ->ReqTimeStr);
	AddLenStrWebPage(HTTP_RESP_ETAG_1, (sizeof(HTTP_RESP_ETAG_1)-1));
	if (!ETagId) AddLenStrWebPage("fe00fe", 6);
	{
        sprintf(FormMess,"%06x", ETagId);
		AddLenStrWebPage(FormMess, 6);
	}
	AddLenStrWebPage(HTTP_RESP_ETAG_2, (sizeof(HTTP_RESP_ETAG_2)-1));
    AddLenStrWebPage(HTTP_RESP_SERVER, (sizeof(HTTP_RESP_SERVER)-1));
    AddStrWebPage(WebServerName);
    AddLenStrWebPage(HTTP_RESP_ACCEPT_RANGE, (sizeof(HTTP_RESP_ACCEPT_RANGE)-1));
    AddLenStrWebPage(HTTP_RESP_CONT_LEN, (sizeof(HTTP_RESP_CONT_LEN)-1));
    if (isCompressBody)
	{
		StartLenPtr = EndHtmlPageGenPtr;
		AddLenStrWebPage(HTTP_RESP_GZIP_LEN, (sizeof(HTTP_RESP_GZIP_LEN)-1));
	}
	else
	{
	    sprintf(FormMess,"%d\r\n", SizeFile);
		AddStrWebPage(FormMess);
		StartLenPtr = NULL;
	}

    /* Cache life time set for browser */
	if ((ParReadWeb->FileType == FRT_CSV_DATA) || 
        (ParReadWeb->FileType == FRT_PDF_DOC) ||
		(ParReadWeb->FileType == FRT_CAPCHA) ||
        (ParReadWeb->FileType == FRT_WAV_SOUND))
	{
		AddLenStrWebPage(HTTP_RESP_CC_NO_CACHE, (sizeof(HTTP_RESP_CC_NO_CACHE)-1));
	}
	else
	{
        AddLenStrWebPage(HTTP_RESP_CACHE_CNTR_PRVT, (sizeof(HTTP_RESP_CACHE_CNTR_PRVT)-1));
        AddStrWebPage(ParWebServ->BrowserCacheExpTime);
        AddStrWebPage(ParWebServ->ServerStartTime);
	}

	if ((ParWebServ->GeneralCfg.KeepAliveEnable) &&
		(isKeepAlive)) AddLenStrWebPage(HTTP_RESP_CON_KEEP_ALIVE, (sizeof(HTTP_RESP_CON_KEEP_ALIVE)-1));
    else 	           AddLenStrWebPage(HTTP_RESP_CON_CLOSE, (sizeof(HTTP_RESP_CON_CLOSE)-1));

    AddLenStrWebPage(HTTP_RESP_CONTENT_TYPE, (sizeof(HTTP_RESP_CONTENT_TYPE)-1));
    switch(FileType)
    {
	    case FRT_GIF_PIC:
		    AddLenStrWebPage(HTTP_RESP_CT_GIF, (sizeof(HTTP_RESP_CT_GIF)-1));
		    break;

	    case FRT_JPG_PIC:
		    AddLenStrWebPage(HTTP_RESP_CT_JPG, (sizeof(HTTP_RESP_CT_JPG)-1));
		    break;

	    case FRT_PNG_PIC:
		case FRT_CAPCHA:
		    AddLenStrWebPage(HTTP_RESP_CT_PNG, (sizeof(HTTP_RESP_CT_PNG)-1));
		    break;

	    case FRT_ICO_PIC:
		    AddLenStrWebPage("image/x-icon\r\n", 14);
		    break;
            
	    case FRT_CSS_SCRIPT:
		    AddLenStrWebPage("text/css\r\n", 10);
		    break;

	    case FRT_JS_SCRIPT:
		    AddLenStrWebPage("text/javascript\r\n", 17);
		    break;

		case FRT_TXT_DATA:
			AddLenStrWebPage("text/plain\r\n", 12);
			break;

		case FRT_XML_DATA:
			AddLenStrWebPage("text/xml\r\n", 10);
			break;

		case FRT_CSV_DATA:
			AddLenStrWebPage("text/csv\r\n", 10);
			AddLenStrWebPage(HTTP_RESP_CONT_DISTR, (sizeof(HTTP_RESP_CONT_DISTR)-1));
#ifdef WIN32
			GetLocalTime(&CurrTime);
			sprintf(FormMess, "%s%02d%02d%04d%02d%02d%02d.csv\r\n", CsvTablePathBaseName,
                CurrTime.wDay, CurrTime.wMonth, CurrTime.wYear,
		        CurrTime.wHour, CurrTime.wMinute, CurrTime.wSecond);
#else 
            ftime(&hires_cur_time);
            cur_time = localtime(&hires_cur_time.time);
			sprintf(FormMess, "%s%02d%02d%04d%02d%02d%02d.csv\r\n", CsvTablePathBaseName,
                cur_time->tm_mday, cur_time->tm_mon + 1, cur_time->tm_year+1900,
			    cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
#endif
            AddStrWebPage(FormMess);
			break;

		case FRT_PDF_DOC:
			AddLenStrWebPage("application/pdf\r\n", 17);
            AddLenStrWebPage(HTTP_RESP_CONT_DISTR, (sizeof(HTTP_RESP_CONT_DISTR)-1));
            /* File name without path extract */
            i = (unsigned int)strlen(ParReadWeb->LocalFileName);
            for(;i > 0;i--)
            {
#ifdef _VCL60ENV_             
                if (ParReadWeb->LocalFileName[i-1] == '/') break;
#else
                if (ParReadWeb->LocalFileName[i-1] == '\\') break;
#endif
            }
            if (i > 0) AddStrWebPage(&ParReadWeb->LocalFileName[i]);
            else       AddLenStrWebPage("document.pdf", 12);
            AddLenStrWebPage("\r\n", 2);
			break;

        case FRT_WAV_SOUND:
            AddLenStrWebPage("audio/x-wav\r\n", 13);
            break;

	    default:
		    AddLenStrWebPage(HTTP_RESP_CT_HTML, (sizeof(HTTP_RESP_CT_HTML)-1));
		    break;
    }
	if (isCompressBody) AddLenStrWebPage(HTTP_RESP_CONT_ENC_GZIP, (sizeof(HTTP_RESP_CONT_ENC_GZIP)-1));
    AddLenStrWebPage(HTTP_RESP_VARY, (sizeof(HTTP_RESP_VARY)-1));
	return StartLenPtr;
}
//---------------------------------------------------------------------------
SENDER_WEB_INFO* GetSenderInstance(SENDER_WORKER_INFO *SenderWorkerPtr)
{
	register unsigned int     Usage, MinUsage;
    register ObjListTask	  *PointTask = NULL;
	register SENDER_WEB_INFO  *MinSenderWebInfoPtr = NULL;
	register SENDER_WEB_INFO  *SenderWebInfoPtr = NULL;
	register ListItsTask      *ListTasks = NULL;

	PointTask = (ObjListTask*)GetFistObjectList(&SenderWorkerPtr->SenderThrList);
	while(PointTask)
	{
	    SenderWebInfoPtr = (SENDER_WEB_INFO*)PointTask->UsedTask;
		if (!MinSenderWebInfoPtr)
		{
			MinSenderWebInfoPtr = SenderWebInfoPtr;
			MinUsage = GetUsageSenderQueue(SenderWebInfoPtr);
			if (!MinUsage) break;
		}
		else
		{
		    Usage = GetUsageSenderQueue(SenderWebInfoPtr);
		    if (!Usage)
			{
				/* Found sender instance without messages */
				MinSenderWebInfoPtr = SenderWebInfoPtr;
				break;
			}
			if (MinUsage > Usage)
			{
				Usage = MinUsage;
				MinSenderWebInfoPtr = SenderWebInfoPtr;
			}
		}
		PointTask = (ObjListTask*)GetNextObjectList(&SenderWorkerPtr->SenderThrList);
	}
	return MinSenderWebInfoPtr;
}
//---------------------------------------------------------------------------
void SenderInstQueueUsageReport(SENDER_WORKER_INFO *SenderWorkerPtr)
{
	SENDER_WEB_INFO *SendWebInfoPtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	char            RecLine[16];
	char            StatusLine[2000];

	strcpy(StatusLine, "Sender queues: ");
	SelObjPtr = (ObjListTask*)GetFistObjectList(&SenderWorkerPtr->SenderThrList);
	while(SelObjPtr)
	{
		SendWebInfoPtr = (SENDER_WEB_INFO*)SelObjPtr->UsedTask;
#ifdef WIN32
		sprintf(RecLine, "%u %u/%u;", SendWebInfoPtr->MsgNotifyId, 
			GetMaxUsageSenderQueue(SendWebInfoPtr), GetUsageSenderQueue(SendWebInfoPtr));
#else
		sprintf(RecLine, "%u %u/%u;", SendWebInfoPtr->NotifyPort, 
			GetMaxUsageSenderQueue(SendWebInfoPtr), GetUsageSenderQueue(SendWebInfoPtr));
#endif
		strcat(StatusLine, RecLine);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&SenderWorkerPtr->SenderThrList);
	}
    DebugLogPrint(NULL, "%s\r\n", StatusLine);
}
//---------------------------------------------------------------------------
bool SendHttpTextDataFileHost(PARAMWEBSERV *ParWebServ, READWEBSOCK *ReadWeb,
    char *HttpRespPtr, unsigned int DataLen, FILE_HASH_RECORD *HashFilePtr, bool isZipBody, char *FileName)
{
	register SENDWEBSOCK *ParSendWeb = NULL;
	POOL_RECORD_STRUCT *SendWebBufPtr = NULL;
	char        LineBuf[256];

#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "Get buffer for data file send\r\n");
#endif
	SendWebBufPtr = GetBuffer(&ParWebServ->SenderWorker.SendWebPool);
	if (!SendWebBufPtr) 
	{	
	    DebugLogPrint(NULL, "Failed buffer allocation for data file sent\r\n");
		return false;
    }
	ParSendWeb = (SENDWEBSOCK*)SendWebBufPtr->DataPtr;
    ParSendWeb->SendBufPoolPtr = (void*)SendWebBufPtr;

	ParSendWeb->HttpRespPtr = NULL;
	ParSendWeb->isBodyCompress = false;
	ParSendWeb->FileType = ReadWeb->FileType;
	ParSendWeb->StartTick = GetTickCount();
    ParSendWeb->SslPtr = ReadWeb->SslPtr;
	if (ReadWeb->ReqestType == HTR_HEAD) ParSendWeb->isHeaderReq = true;
	else                                 ParSendWeb->isHeaderReq = false;
	ParSendWeb->HttpSocket = ReadWeb->HttpSocket;
	ParSendWeb->PageProcessStartTick = ReadWeb->StartReqHandleTick;
	ParSendWeb->isDeliverySuccess = false;
	strcpy(ParSendWeb->FileName, FileName);

	if (HashFilePtr)
	{
		if (HashFilePtr->ZipFileBodyBuf)
		{
			/* Compressed body is available in hash */
			ParSendWeb->HTTPRespLen = HashFilePtr->ZipFileLen;
		    ParSendWeb->HttpRespPtr = (char*)HashFilePtr->ZipFileBodyBuf;			
		}
		else
		{
		    ParSendWeb->HTTPRespLen = HashFilePtr->FileLen;
		    ParSendWeb->HttpRespPtr = (char*)HashFilePtr->FileBodyBuf;
		}
		ParSendWeb->isHashData = true;
	}
	else
	{
	    ParSendWeb->HTTPRespLen = DataLen;
		ParSendWeb->HttpRespPtr = (char*)AllocateMemory(DataLen+1);
		if (!ParSendWeb->HttpRespPtr)
		{
			FreeBuffer(&ParWebServ->SenderWorker.SendWebPool, (POOL_RECORD_STRUCT*)ParSendWeb->SendBufPoolPtr);
			DebugLogPrint(NULL, "Failed memory allocation for http response\r\n");
			return false;
		}
		memcpy(ParSendWeb->HttpRespPtr, HttpRespPtr, ParSendWeb->HTTPRespLen);
		ParSendWeb->isHashData = false;
	}

	EndHtmlPageGenPtr = &ParSendWeb->HttpRespHeader[0];
	*EndHtmlPageGenPtr = 0;
    AddLenStrWebPage(HTTP_200_OK, (sizeof(HTTP_200_OK)-1));
	AddStrWebPage(ParWebServ->ReqTimeStr);
    AddLenStrWebPage(HTTP_RESP_SERVER, (sizeof(HTTP_RESP_SERVER)-1));
    AddStrWebPage(WebServerName);
    AddLenStrWebPage("\r\nETag: \"", 9);
	if (HashFilePtr)
	{
        sprintf(LineBuf, "%06x", HashFilePtr->FileEtag);
		AddLenStrWebPage(LineBuf, 6);
	}
	else
	{
		AddLenStrWebPage("fe00fe", 6);
    }
	AddLenStrWebPage("-iss-windows-1251\"", 18);
    AddLenStrWebPage(HTTP_RESP_ACCEPT_RANGE, (sizeof(HTTP_RESP_ACCEPT_RANGE)-1));
    AddLenStrWebPage(HTTP_RESP_CACHE_CNTR_PRVT, (sizeof(HTTP_RESP_CACHE_CNTR_PRVT)-1));
    AddStrWebPage(ParWebServ->BrowserCacheExpTime);
    AddStrWebPage(ParWebServ->ServerStartTime);

	if ((ParWebServ->GeneralCfg.KeepAliveEnable) && (ReadWeb->isKeepAlive))
	{
		AddLenStrWebPage(HTTP_RESP_CON_KEEP_ALIVE, (sizeof(HTTP_RESP_CON_KEEP_ALIVE)-1));
		ParSendWeb->ReadWebPtr = ReadWeb;
	}
    else
	{
		AddLenStrWebPage(HTTP_RESP_CON_CLOSE, (sizeof(HTTP_RESP_CON_CLOSE)-1));
        ReadWeb->SslPtr = NULL;
		ParSendWeb->ReadWebPtr = NULL;
	}

    AddLenStrWebPage(HTTP_RESP_CONTENT_TYPE, (sizeof(HTTP_RESP_CONTENT_TYPE)-1));
    switch(ReadWeb->FileType)
    {
	    case FRT_CSS_SCRIPT:
		    AddLenStrWebPage("text/css\r\n", 10);
		    break;

	    case FRT_JS_SCRIPT:
		    AddLenStrWebPage("text/javascript\r\n", 17);
		    break;

		case FRT_TXT_DATA:
			AddLenStrWebPage("text/plain\r\n", 12);
			break;

		case FRT_XML_DATA:
			AddLenStrWebPage("text/xml\r\n", 10);
			break;

		case FRT_CSV_DATA:
			AddLenStrWebPage("text/csv\r\n", 10);
			break;

	    default:
		    AddLenStrWebPage(HTTP_RESP_CT_HTML, (sizeof(HTTP_RESP_CT_HTML)-1));
		    break;
    }

	AddLenStrWebPage(HTTP_RESP_CONT_LEN, (sizeof(HTTP_RESP_CONT_LEN)-1));
	ParSendWeb->HeaderBeginSetLenPtr = NULL;
	if (isZipBody)
	{
		if (HashFilePtr && HashFilePtr->ZipFileBodyBuf)
		{
	        sprintf(LineBuf,"%u\r\n", ParSendWeb->HTTPRespLen);
            AddStrWebPage(LineBuf);
		}
		else
		{
		    ParSendWeb->HeaderBeginSetLenPtr = EndHtmlPageGenPtr;
		    AddLenStrWebPage("        \r\n", 10);
		    ParSendWeb->isBodyCompress = true;
		}
		AddLenStrWebPage(HTTP_RESP_CONT_ENC_GZIP, (sizeof(HTTP_RESP_CONT_ENC_GZIP)-1));
	}
	else
	{
	    sprintf(LineBuf,"%u\r\n", ParSendWeb->HTTPRespLen);
        AddStrWebPage(LineBuf);
	}

    AddLenStrWebPage(HTTP_RESP_VARY, (sizeof(HTTP_RESP_VARY)-1));

	ParSendWeb->HeaderLen = (unsigned int)(EndHtmlPageGenPtr - &ParSendWeb->HttpRespHeader[0]);	
#ifdef _SERVDEBUG_
		DebugLogStrBufPrint(NULL, ParSendWeb->HttpRespHeader, "%s: Response HTTP header: ", ThrWebServName);
#endif
	return SendHtmlDataUser(&ParWebServ->SenderWorker, ParSendWeb);
}
//---------------------------------------------------------------------------
bool SendHttpHtmlPageHost(PARAMWEBSERV *ParWebServ, READWEBSOCK *ReadWeb,
						  USER_SESSION *SessionPtr, char *HttpRespPtr, 
						  unsigned int DataLen, bool isZipBody, char *FileName)
{
	unsigned int SessionId = 0;
	SENDWEBSOCK *ParSendWeb = NULL;
	POOL_RECORD_STRUCT *SendWebBufPtr = NULL;
	char        LineBuf[256];

	if (SessionPtr) SessionId = SessionPtr->SessionId;
#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Get buffer for HTML pages send %d\r\n", 
		ThrWebServName, SessionId);
#endif
	SendWebBufPtr = GetBuffer(&ParWebServ->SenderWorker.SendWebPool);
	if (!SendWebBufPtr) 
	{	
	    DebugLogPrint(NULL, "Failed buffer allocation for HTML page sent %d\r\n", SessionId);
		return false;
    }
	ParSendWeb = (SENDWEBSOCK*)SendWebBufPtr->DataPtr;
    ParSendWeb->SendBufPoolPtr = (void*)SendWebBufPtr;

	ParSendWeb->HttpRespPtr = NULL;
	ParSendWeb->isBodyCompress = false;
	ParSendWeb->isHashData = false;
	ParSendWeb->FileType = FRT_HTML_PAGE;
	ParSendWeb->StartTick = GetTickCount();
    ParSendWeb->SslPtr = ReadWeb->SslPtr;
	if (ReadWeb->ReqestType == HTR_HEAD) ParSendWeb->isHeaderReq = true;
	else                                 ParSendWeb->isHeaderReq = false;
	ParSendWeb->HttpSocket = ReadWeb->HttpSocket;
	ParSendWeb->PageProcessStartTick = ReadWeb->StartReqHandleTick;
	ParSendWeb->isDeliverySuccess = false;
	ParSendWeb->HTTPRespLen = DataLen;
	strcpy(ParSendWeb->FileName, FileName);

	ParSendWeb->HttpRespPtr = (char*)AllocateMemory(ParSendWeb->HTTPRespLen+1);
	if (!ParSendWeb->HttpRespPtr)
	{
		FreeBuffer(&ParWebServ->SenderWorker.SendWebPool, (POOL_RECORD_STRUCT*)ParSendWeb->SendBufPoolPtr);
	    DebugLogPrint(NULL, "Failed memory allocation for http response %d\r\n", SessionId);
		return false;
	}
	memcpy(ParSendWeb->HttpRespPtr, HttpRespPtr, ParSendWeb->HTTPRespLen);

	EndHtmlPageGenPtr = &ParSendWeb->HttpRespHeader[0];
	*EndHtmlPageGenPtr = 0;
    AddStrWebPage("HTTP/1.1 200 OK\r\n");
	AddStrWebPage(ParWebServ->ReqTimeStr);
    AddStrWebPage("Server: ");
    AddStrWebPage(WebServerName);
    AddStrWebPage("\r\nETag: \"97fe-354-ll-windows-1251\"\r\n");
    AddStrWebPage("Accept-Range: bytes\r\n");
	if ((ParWebServ->GeneralCfg.KeepAliveEnable)&&(ReadWeb->isKeepAlive))
	{
		AddStrWebPage("Connection: Keep-Alive\r\n");
		ParSendWeb->ReadWebPtr = ReadWeb;
	}
    else
	{
		AddStrWebPage("Connection: close\r\n");
        ReadWeb->SslPtr = NULL;
		ParSendWeb->ReadWebPtr = NULL;
	}
    AddStrWebPage("Content-Type: text/html;charset=windows-1251\r\n");
	if (isZipBody)
	{
        AddStrWebPage("Content-Length: ");
		ParSendWeb->HeaderBeginSetLenPtr = EndHtmlPageGenPtr;
		AddStrWebPage("        \r\n");
	}
	else
	{
	    sprintf(LineBuf,"Content-Length: %d\r\n", ParSendWeb->HTTPRespLen);
        AddStrWebPage(LineBuf);
		ParSendWeb->HeaderBeginSetLenPtr = NULL;
	}

	if (isZipBody > 0)
	{
		AddStrWebPage("Content-Encoding: gzip\r\n");
		ParSendWeb->isBodyCompress = true;
	}
	
	if (SessionPtr && (!SessionPtr->UserPtr) && (ReadWeb->BotType == BOT_NONE))
	{
        AddStrWebPage("Set-Cookie: ISSISESSIONID=");
		AddStrWebPage(SessionPtr->SesionIdKey);
		AddStrWebPage("; expires=Tue, ");
        AddStrWebPage(ParWebServ->ExpDateCookieStr);
		AddStrWebPage("\r\n");
        /* AddStrWebPage("; domain=");
	    SetServerHttpAddr(NULL);
	    AddStrWebPage("; path=/\r\n"); */
	}
	
    AddStrWebPage("Vary:accept-charset\r\n\r\n");

	ParSendWeb->HeaderLen = (unsigned int)(EndHtmlPageGenPtr - &ParSendWeb->HttpRespHeader[0]);
	return SendHtmlDataUser(&ParWebServ->SenderWorker, ParSendWeb);
}
//---------------------------------------------------------------------------
