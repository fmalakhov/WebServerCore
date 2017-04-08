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
#include "BaseHtmlConstData.h"
#include "SysMessages.h"
#include "TrTimeOutThread.h"
#include "ServerPorts.h"
#include "SessionIpHash.h"
#include "HtmlPageHash.h"
#include "BadIpHash.h"

#ifdef _LINUX_X86_
#include <dirent.h>
#endif

extern char ThrWebServName[];
extern PARAMWEBSERV *ParWebServPtr;
extern STATS_INFO   ServerStats;
extern char *MemWebPageGenPtr;
extern ListItsTask  UserSessionList;
extern char KeySessionId[];
extern char WebServerName[];
extern char *EndHtmlPageGenPtr;
extern SESSION_IP_HASH_OCTET_HOP SessionIpHashHop;

#ifdef _SERVER_PERF_MEASURE_
extern unsigned int AvearageSummTime;
extern unsigned int MaxTime;
extern unsigned int SummCount;
#endif
//---------------------------------------------------------------------------
static char NoAvailSessionResp[] = 
"\r\nETag: \"97fe-354-ll-windows-1251\"\r\n"
"Accept-Range: bytes\r\n"
"Connection: close\r\n"
"Content-Type: text/html;charset=windows-1251\r\n"
"Vary:accept-charset\r\n\r\n"
"<HTML>\r\n<HEAD>\r\n<TITLE>Access result</TITLE>\r\n"
"<meta HTTP-EQUIV=\"Cache-Control\" CONTENT=\"no-cache\">\r\n</HEAD>\r\n"
"<BODY>\r\n<center><H2>Result of user's access to WEB Server:</H2></center><center>\r\n"
"There is no free sessions for server access!"
"</center><P>\r\n"
"<center>Future processing of this requiest is canceled.</center>\r\n"
"<center>Please try to server access again via 15 minutes.</center>\r\n"
"<P>\r\n<HR>\r\n"
"</BODY>\r\n</HTML>\r\n";

/*--------------------------------- MACROS -----------------------------------*/
/*--------------------------- FUNCTION DEFINITION -----------------------------*/
/*-------------------------- LOCAL VARIABLES ----------------------------------*/
/*------------------------------- CODE ----------------------------------------*/
void HandleReaderDataInd(PARAMWEBSERV *ParWebServ, InterThreadInfoMsg* WebServMsgPtr, unsigned int SysShowHostIP)
{
	bool            isSessionFind;
	unsigned int    KeyStatus, i;
    int             ReqSessionId;
    unsigned short  WebPort;
	HashDirTypeT    DirType;
	READWEBSOCK     *ParReadWeb;
	USER_SESSION    *SessionPtr;
	SESSION_IP_HASH_RECORD *SessionIpRecPtr;
#ifdef _SERVER_PERF_MEASURE_
    unsigned int           DeltaTime;
    unsigned long long int StartTime, EndTime;
    struct timespec        spec;
#endif
	char            IpAddr1Buf[32];
	char            IpAddr2Buf[32];
	char            HtmlFileName[1024];
    
    ParReadWeb = (READWEBSOCK*)WebServMsgPtr->WParam;
    if (ParReadWeb->WebChanId == PRIMARY_WEB_CHAN)
	{
		WebPort = ParWebServ->ServCustomCfg.PrimWebAccIPPort;
		if (ParWebServ->ServCustomCfg.PrimContentDelivery)	DirType = PRIM_CONTENT_LIST;
		else 												DirType = BASE_HASH_LIST;
	}
    else if (ParReadWeb->WebChanId == SECONDARY_WEB_CHAN)
	{
		WebPort = ParWebServ->ServCustomCfg.SecondWebAccIPPort;
		if (ParWebServ->ServCustomCfg.SecondContentDelivery)	DirType = SECOND_CONTENT_LIST;
		else													DirType = BASE_HASH_LIST;
	}
	else
	{		
		WebPort = ParWebServ->ServCustomCfg.ForwardIpPort;
	}

#ifdef _SERVER_PERF_MEASURE_
    clock_gettime(CLOCK_REALTIME, &spec);
    StartTime = (unsigned long long int)spec.tv_sec * 1000000;
    StartTime += (unsigned long long int)(spec.tv_nsec / 1000);
#endif

#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Begin HandlerWebServerReq function (%d) (WSU_USERDATA)\r\n",
		ThrWebServName, ParReadWeb->HttpSocket);
#endif
	HttpLoadRemList(&ParWebServPtr->ReaderWorker, ParReadWeb);
    ParWebServ->DataReadyCount++;            
	if(ParReadWeb->Status != URP_SUCCESS)
	{
		HandleReadBadWebReq(ParWebServ, ParReadWeb);
		return;
	}

	/* Test IP address of client for bad IP list */
	if(isIpInBadList(ParReadWeb->HttpClientIP))
	{
		DebugLogPrint(NULL, "%s: Access denied for client from %08x IP address (IP is in bad list)\r\n",
		ThrWebServName, ParReadWeb->HttpClientIP);

		CreateRespNoAcess(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
		ParReadWeb->isKeepAlive = false;
		if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
			CloseHttpChan(ParReadWeb);
		ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
		ServerStats.IntBadUserReq++;
		return;
	}

    if (ParReadWeb->WebChanId == FORWARD_WEB_CHAN)
	{
        EndHtmlPageGenPtr = MemWebPageGenPtr;
        *EndHtmlPageGenPtr = 0;
		AddLenStrWebPage("HTTP/1.1 302 Found\r\nLocation: ", 30);
		AddStrWebPage(ParWebServ->ServCustomCfg.ForwardUrl);
		AddLenStrWebPage("\r\nCache-Control: private\r\n", 26);
        AddStrWebPage(ParWebServ->ReqTimeStr);
		EndHtmlPageGenPtr = &MemWebPageGenPtr[strlen(MemWebPageGenPtr)];
        AddLenStrWebPage("Server: ", 8);
        AddStrWebPage(WebServerName);
        AddLenStrWebPage(NoAvailSessionResp, (sizeof(NoAvailSessionResp)-1));
		ParReadWeb->isKeepAlive = false;
		if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
			CloseHttpChan(ParReadWeb);
		ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
		ParWebServ->HtmlPageGenCount++;
	    ServerStats.SendHtmlPages++;
	    ServerStats.IntSendHtmlPages++;		
	    return;
	}

	if (ParReadWeb->WebChanId == PRIMARY_WEB_CHAN)
	{
		if (ParWebServ->ServCustomCfg.PrimContentDelivery)
		{
			if ((ParReadWeb->ReqestType == HRT_GET) || (ParReadWeb->ReqestType == HTR_HEAD))
			{
		        CreateContentResponse(ParWebServ, ParReadWeb, ParReadWeb->HttpSocket, 
			        ParReadWeb->HttpReqPtr, ParWebServ->LocalAddrIP, 
			        ParReadWeb->HttpClientIP, WebPort, DirType);
					
				if (!ParWebServ->SenderWorker.KeepAliveEnable || !ParReadWeb->isKeepAlive)
					ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
			}
			else
			{
                DebugLogStrBufPrint(NULL, ParReadWeb->StrCmdHTTP, 
                    "%s: Unexpected request type in content request {Soc: %d , ReqType: %u}, CMD=",
				    ThrWebServName, ParReadWeb->HttpSocket, ParReadWeb->ReqestType, ParReadWeb->StrCmdHTTP);
                CreateRespWrongData(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
				ParReadWeb->isKeepAlive = false;
				if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
					(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
					CloseHttpChan(ParReadWeb);
				ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
				ServerStats.IntBadUserReq++;
			}
			return;
		}
	}
	else
	{
		if (ParWebServ->ServCustomCfg.SecondContentDelivery)
		{
			if ((ParReadWeb->ReqestType == HRT_GET) || (ParReadWeb->ReqestType == HTR_HEAD))
			{
		        CreateContentResponse( ParWebServ, ParReadWeb, ParReadWeb->HttpSocket, 
			        ParReadWeb->HttpReqPtr, ParWebServ->LocalAddrIP, 
			        ParReadWeb->HttpClientIP, WebPort, DirType);
					
				if (!ParWebServ->SenderWorker.KeepAliveEnable || !ParReadWeb->isKeepAlive)					
					ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
			}
			else
			{
                DebugLogStrBufPrint(NULL, ParReadWeb->StrCmdHTTP, 
                    "%s: Unexpected request type in content request {Soc: %d , ReqType: %u}, CMD=",
				    ThrWebServName, ParReadWeb->HttpSocket, ParReadWeb->ReqestType, ParReadWeb->StrCmdHTTP);
                CreateRespWrongData(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
				ParReadWeb->isKeepAlive = false;
				if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
					(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
					CloseHttpChan(ParReadWeb);
				ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);		
				ServerStats.IntBadUserReq++;
			}
			return;
		}		
	}
	
#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Session detection is started (%d)\r\n",
		ThrWebServName, ParReadWeb->HttpSocket);
#endif
	isSessionFind = false;
	KeyStatus = 0;
	SessionPtr = NULL;
	if (UserSessionList.Count > 0)
	{
		if ((ParReadWeb->FileType == FRT_HTML_PAGE) ||
            (ParReadWeb->FileType == FRT_HTR_PAGE))
		{
			for(;;)
			{
				for(;;)
				{
		            if (!ParReadWeb->SessioIdOffset)
					{
						KeyStatus = 1; /* Key identifier was not found */
						break;
					}
                    
                    ReqSessionId = FindSessionByKey(ParReadWeb->SessioIdOffset);
		            if (!ReqSessionId)
					{                    
						KeyStatus = 4; /* Key not found */
						break;
					}
					break;
				}
				if ((KeyStatus == 1) && (ParReadWeb->CookieSessionId > 0))
				{
					ReqSessionId = ParReadWeb->CookieSessionId;
				}
				else
				{
					if (KeyStatus > 0) break;
				}

				SessionPtr = GetSessionBySessionId(&ParWebServ->SessionManager, ReqSessionId);
				if (!SessionPtr)
				{
					KeyStatus = 3; /* Session with set key was not found */
					break;
				}
				if (SessionPtr->UserIpAddr != ParReadWeb->HttpClientIP)
				{
					SetIpAddrToString(&IpAddr1Buf[0], SessionPtr->UserIpAddr);
					SetIpAddrToString(&IpAddr2Buf[0], ParReadWeb->HttpClientIP);

#ifdef WIN32
					if ((SessionPtr->UserIpAddr & 0x0000ffff) != (ParReadWeb->HttpClientIP & 0x0000ffff))
#else
  #ifdef _SUN_BUILD_
					if ((SessionPtr->UserIpAddr & 0x0000ffff) != (ParReadWeb->HttpClientIP & 0x0000ffff))
  #else
					if ((SessionPtr->UserIpAddr & 0xffff0000) != (ParReadWeb->HttpClientIP & 0xffff0000))
  #endif
#endif
					{
					   // KeyStatus = 2; /* Key is corrupted */
						KeyStatus = 3; /* Session with set key used by other session */
		                DebugLogPrint(NULL, "%s: Session %d IP session & client mismatch %s %s\r\n", 
		                    ThrWebServName, SessionPtr->SessionId, &IpAddr1Buf[0], &IpAddr2Buf[0]);
						SessionPtr = NULL;
					}
					else
					{
		                DebugLogPrint(NULL, "%s: Session %d IP was changed from %s to %s\r\n", 
		                ThrWebServName, SessionPtr->SessionId, &IpAddr1Buf[0], &IpAddr2Buf[0]);

						SessionIpRecPtr = FindSessionIpHash(&SessionIpHashHop, SessionPtr->UserIpAddr);
						if (SessionIpRecPtr)
						{
							SessionIpRecPtr->SessionCount--;
							if (!SessionIpRecPtr->SessionCount)
							{
			                    if (!RemSessionIpHash(&SessionIpHashHop, SessionPtr->UserIpAddr))
								{
				                    printf("Failed to remove Session IP hash record for %08x IP\n", (unsigned int)SessionPtr->UserIpAddr);
								}
							}
                            SessionPtr->UserIpAddr = ParReadWeb->HttpClientIP;
                            SessionIpRecPtr = FindSessionIpHash(&SessionIpHashHop, SessionPtr->UserIpAddr);
							if (SessionIpRecPtr)
							{
								SessionIpRecPtr->SessionCount++;
							}
							else
							{
							    if (!AddSessionIpHash(&SessionIpHashHop, ParReadWeb->HttpClientIP))
								{
								    printf("Failed to add %08x IP address to the list of session's IPs\n", (unsigned int)ParReadWeb->HttpClientIP);
								}
							}
						    KeyStatus = 0; /* Key is fine */
						    isSessionFind = true;
						}
						else
						{
							printf("Corruption of Session IP is detected\r\n");
							KeyStatus = 2; /* Key is corrupted */
						}
					}
					break;
				}
				KeyStatus = 0; /* Key is fine */
				isSessionFind = true;
				break;
			}
		}
        else
		{
			/* For case of non HTML resource request */
			SessionPtr = NULL;
			isSessionFind = true;
		}
	}

	if (KeyStatus == 2)
	{
		DebugLogPrint(NULL, "%s: Session detection is failed - status=2 (%d)\r\n",
			ThrWebServName, ParReadWeb->HttpSocket);

		CreateRespNoAcess(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
		ParReadWeb->isKeepAlive = false;
		if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
			CloseHttpChan(ParReadWeb);
		ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
		ServerStats.IntBadUserReq++;
		return;
	}
	else if (KeyStatus == 4)
	{
		DebugLogPrint(NULL, "%s: Session detection is completed (not existed key) - status=4 (%d)\r\n",
			ThrWebServName, ParReadWeb->HttpSocket);
	}
    
	if (!isSessionFind)
	{
        if (ParReadWeb->FileType == FRT_HTR_PAGE)
        {
            if ((ParReadWeb->HttpClientIP == SysShowHostIP) &&
                (FindCmdRequest(ParReadWeb->LocalFileName, GenPageDynStatusShowSessIdReq) != -1))
            {
			    DebugLogPrint(NULL, "%s:Received request for session create for trusted host (Status:%d, Socet:%d)\r\n",
				    ThrWebServName, KeyStatus, ParReadWeb->HttpSocket);           
            }
            else
            {
			    DebugLogPrint(NULL, "%s: Requested dynamical content block for unexpected session (Status:%d, Socet:%d, File: %s)\r\n",
				    ThrWebServName, KeyStatus, ParReadWeb->HttpSocket, ParReadWeb->LocalFileName);

                DebugLogStrBufPrint(NULL, ParReadWeb->StrCmdHTTP, 
                    "%s: Unexp. dyn. content block body {Soc: %d}, CMD=",
				    ThrWebServName, KeyStatus, ParReadWeb->HttpSocket, ParReadWeb->StrCmdHTTP);

                CreateRespWrongData(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
				ParReadWeb->isKeepAlive = false;
				if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
					(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
					CloseHttpChan(ParReadWeb);
				ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
				ServerStats.IntBadUserReq++;
                return;
            }
        }
        SessionIpRecPtr = FindSessionIpHash(&SessionIpHashHop, ParReadWeb->HttpClientIP);
		if ((UserSessionList.Count < ParWebServ->GeneralCfg.MaxOpenSessions) &&
			((!SessionIpRecPtr) || (SessionIpRecPtr->SessionCount < ParWebServ->GeneralCfg.MaxSesionPerIP)))
		{
            SessionPtr = UserSessionCreate(&ParWebServ->SessionManager, ParReadWeb, SessionIpRecPtr);
		}
		else
		{
			DebugLogPrint(NULL, "%s: Request from %08x client's IP address was canceled due to limit of active sessions per IP is reached\r\n", 
				ThrWebServName, ParReadWeb->HttpClientIP);

            EndHtmlPageGenPtr = MemWebPageGenPtr;
            *EndHtmlPageGenPtr = 0;
			AddLenStrWebPage("HTTP/1.1 200 OK\r\n", 17);
            AddStrWebPage(ParWebServ->ReqTimeStr);
			EndHtmlPageGenPtr = &MemWebPageGenPtr[strlen(MemWebPageGenPtr)];
            AddLenStrWebPage("Server: ", 8);
            AddStrWebPage(WebServerName);
            AddLenStrWebPage(NoAvailSessionResp, (sizeof(NoAvailSessionResp)-1));
			ParReadWeb->isKeepAlive = false;
			if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
				(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
			ServerStats.IntBadUserReq++;
			return;
		}
	}
	else
	{
	  if (ParReadWeb->FileType == FRT_HTML_PAGE) SessionPtr->isActive = true;
	}
    
#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Session detection is done\r\n", ThrWebServName);
#endif
	if (!ParWebServ->ActivServer)
	{
		UserSessionDelete(&ParWebServ->SessionManager, SessionPtr);
		CreateRespNoAcess(MemWebPageGenPtr, ParWebServ->LocalAddrIP, WebPort);
		ParReadWeb->isKeepAlive = false;
		if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
			CloseHttpChan(ParReadWeb);
		ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
		ServerStats.IntBadUserReq++;
		return;
	}
	else
	{
#ifdef _SERVDEBUG_
		DebugLogPrint(NULL, "%s: Start response generation\r\n", ThrWebServName);
#endif
		CreateResponse( ParWebServ, ParReadWeb, ParReadWeb->HttpSocket, SessionPtr, 
			ParReadWeb->HttpReqPtr, ParWebServ->LocalAddrIP, ParReadWeb->HttpClientIP, WebPort, DirType);	
	}

    if (ParReadWeb->RespDelay)
    {
        ParReadWeb->RespDelay = false;
        return;
    }

	if ((ParReadWeb->BotType != BOT_NONE) && ((!ParWebServ->GeneralCfg.KeepAliveEnable) ||
		(!ParReadWeb->isKeepAlive))) UserSessionDelete(&ParWebServ->SessionManager, SessionPtr);
		
	if (!ParWebServ->SenderWorker.KeepAliveEnable || !ParReadWeb->isKeepAlive)
		ReaderActionDone(&ParWebServ->ReaderWorker, ParReadWeb);
	
#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Done handel valid html request\r\n", ThrWebServName);
#endif

#ifdef _SERVER_PERF_MEASURE_
	clock_gettime(CLOCK_REALTIME, &spec);
    EndTime = (unsigned long long int)spec.tv_sec * 1000000;
    EndTime += (unsigned long long int)(spec.tv_nsec / 1000);
    DeltaTime = (unsigned int)(EndTime - StartTime);
//    if (DeltaTime > 20)
        printf("DT: %u MSG: (%s)\n", (unsigned int)DeltaTime, ParReadWeb->LocalFileName);
    if (DeltaTime > MaxTime) MaxTime = DeltaTime;
    AvearageSummTime += DeltaTime;
    SummCount++;
#endif
}
//---------------------------------------------------------------------------
