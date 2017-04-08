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
#include "SysWebFunction.h"
#include "ThrConnWeb.h"
#include "SysMessages.h"
#include "TrTimeOutThread.h"
#include "ServerPorts.h"
#include "SessionIpHash.h"
#include "FileNameMapBase.h"
#include "HtmlPageHash.h"
#include "HtmlTemplateParser.h"

#ifdef _LINUX_X86_
#include <dirent.h>
#endif

extern unsigned char gLanguageType;
extern bool gFileNotFoundFlag;
extern char ThrWebServName[];
extern char KeyFormSessionId[];
extern PARAMWEBSERV *ParWebServPtr;
extern STATS_INFO   ServerStats;
extern char *MemWebPageGenPtr;
extern char *EndHtmlPageGenPtr;
extern READWEBSOCK *ParReadHttpSocketPtr;
extern HTML_PAGE_HASH_CHAR_HOP GetHtmlPageHashHop;
extern HTML_PAGE_HASH_CHAR_HOP PostHtmlPageHashHop;
extern FILE_HASH_CHAR_HOP FileHashHop;
extern unsigned int LastReqLoadTime[];
extern unsigned int LastPageGenTime[];

#ifdef _SERVER_PERF_MEASURE_
extern unsigned int AvearageSummTime;
extern unsigned int MaxTime;
extern unsigned int SummCount;
#endif
//---------------------------------------------------------------------------
void CreateResponse(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, SOCKET UserSocket, USER_SESSION *SessionPtr, 
	char *BufAnsw, char *LocalAdress, unsigned long CurrRemAddr,int LocalPortServ, HashDirTypeT DirType)
{
	int             j;
	unsigned int	i;
	DWORD           StartTick;
	unsigned int    DeltaTime;
	FILE			*HandelFIL;       
	char			*StrCmdHTTP = NULL;
	int				BlockRead;
	bool            isHtmlPageTx = false;
	bool            BadRequest = false;
	bool            ParseDone = false;
    unsigned int    *SrcTimePtr, *DestTimePtr;
	unsigned int    SessionId = 0;
	unsigned int    HeaderLen = 0;
	unsigned int    SrcPageLen;
	unsigned int    HtmlUserTypeMask = 0x01;
    unsigned int    LogStrLine;
#ifdef _SERVER_PERF_MEASURE_
    unsigned int           DeltaTime2;
    unsigned long long int StartTime, EndTime;
    struct timespec        spec;
#endif
	HTML_HANDLER_ARRAY *HtmlHandlerArrPtr = NULL;
	FILE_HASH_RECORD   *FileInfoPtr = NULL;
	HTML_PAGE_HASH_RECORD *HtmlHashRecPtr = NULL;
	USER_SESSION    *SelSessionPtr = NULL;
	char            *ComprDataBufPtr = NULL;
    DWORD           WaitResult;
	bool            StartSendRes;
	char            *CwdPtr = NULL;
    char            *MapNamePtr = NULL;
#ifdef WIN32
	DWORD         SizeFile;        
	HANDLE        HFSndMess;
#else       
    unsigned int  SizeFile;
    struct stat   st;        
#endif
	char          CapchaName[16];

#ifdef _SERVER_PERF_MEASURE_ /*
    clock_gettime(CLOCK_REALTIME, &spec);
    StartTime = (unsigned long long int)spec.tv_sec * 1000000;
    StartTime += (unsigned long long int)(spec.tv_nsec / 1000); */
#endif

#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Http req (%d) file: %s\r\n",
		ThrWebServName, UserSocket, ParReadWeb->LocalFileName);
#endif
	isHtmlPageTx = false;
	*MemWebPageGenPtr = 0;
	ParReadHttpSocketPtr = ParReadWeb;
	ParWebServPtr = ParWebServ;
    if ((ParReadWeb->FileType == FRT_HTML_PAGE) && SessionPtr)
        gLanguageType = SessionPtr->LanguageType;
	gFileNotFoundFlag = false;
	StrCmdHTTP = ParReadWeb->StrCmdHTTP;
	ParReadWeb->IsExtGen = false;
    ParReadWeb->RespDelay = false;
    ParReadWeb->isCompressRequired = false;

    if (!ParReadWeb->isUserAuthReq &&
		((ParReadWeb->FileType == FRT_HTML_PAGE) || 
		 (ParReadWeb->FileType == FRT_HTR_PAGE) || 
		 ParWebServ->ServCustomCfg.FileRequestLogging))
	{    
        EndHtmlPageGenPtr = MemWebPageGenPtr;
        *EndHtmlPageGenPtr = 0;
        AddStrWebPage(ParWebServ->LogTimeStampStr);
        AddStrWebPage(ThrWebServName);
        AddLenStrWebPage(": ", 2);
        AddStrWebPage(ParReadWeb->RequestInfoStr);
        AddLenStrWebPage("; Cmd=", 6);
        AddStrWebPage(ParReadWeb->StrCmdHTTP);
        AddLenStrWebPage("\r\n", 2);
        DebugLogStrPrint(MemWebPageGenPtr); 
        EndHtmlPageGenPtr = MemWebPageGenPtr;
	}   

	/* HTML request load time calculation */
	DeltaTime = (unsigned int)(GetTickCount() - ParReadWeb->StartReqHandleTick);
    if (DeltaTime > ServerStats.MaxReqLoadTime) ServerStats.MaxReqLoadTime = DeltaTime;

	ParWebServ->SummHtmlReqLoadTime += DeltaTime;
	ParWebServ->HtmlReqLoadCount++;
	ServerStats.AvgReqLoadTime = (unsigned int)(ParWebServ->SummHtmlReqLoadTime/ParWebServ->HtmlReqLoadCount);
    
    SrcTimePtr = &LastReqLoadTime[2];
    DestTimePtr = (SrcTimePtr + 1);
    *DestTimePtr-- = *SrcTimePtr--;
    *DestTimePtr-- = *SrcTimePtr--;
    *DestTimePtr-- = *SrcTimePtr--;
    *DestTimePtr = DeltaTime;

	if ((ParReadWeb->FileType == FRT_HTML_PAGE) || 
        (ParReadWeb->FileType == FRT_HTR_PAGE))
	{
	    if (ParReadWeb->BotType == BOT_NONE && !ParReadWeb->isUserAuthReq && 
            ParReadWeb->FileType != FRT_HTR_PAGE)
		{
            EndHtmlPageGenPtr = MemWebPageGenPtr;
            *EndHtmlPageGenPtr = 0;
            AddStrWebPage(ParWebServ->LogTimeStampStr);
            AddStrWebPage(ThrWebServName);
            AddLenStrWebPage(": Message body: {", 17);
            LogStrLine = (unsigned int)(tulong)(EndHtmlPageGenPtr - MemWebPageGenPtr);
            if (strlen(ParReadWeb->HttpReqPtr) < (MAX_LOG_LINE_LEN - LogStrLine)) AddStrWebPage(ParReadWeb->HttpReqPtr);
            else
            {
                memcpy(EndHtmlPageGenPtr, ParReadWeb->HttpReqPtr, (MAX_LOG_LINE_LEN - LogStrLine));
                EndHtmlPageGenPtr += (MAX_LOG_LINE_LEN - LogStrLine);
                AddLenStrWebPage("...", 3);
            }
            AddLenStrWebPage("}\r\n", 3);
            DebugLogStrPrint(MemWebPageGenPtr); 
            EndHtmlPageGenPtr = MemWebPageGenPtr;
		}
		*ParWebServ->UserTitle = 0;
		*ParWebServ->UserMetaData = 0;
		SessionPtr->isMainPageReq = false;

        if (ParReadWeb->DeviceType == SDT_MOBILE)
		{
            if (ParReadWeb->MobileType)
			{
			    if (ParReadWeb->MobileType->AspectWidth < 320) SessionPtr->MobileScreenWidth = 320;
			    else SessionPtr->MobileScreenWidth = ParReadWeb->MobileType->AspectWidth;
			}
			else
			{
                SessionPtr->MobileScreenWidth = 320;
			}
		}

		StartTick = GetTickCount();
		if (SessionPtr->UserPtr)
		{
			if (ParWebServ->OnGetHtmlUserTypeMask)
				 HtmlUserTypeMask = (ParWebServ->OnGetHtmlUserTypeMask)(SessionPtr->UserPtr->UserType);
			else HtmlUserTypeMask = 0x02;
		}

#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Find function for html request %d\r\n", 
				ThrWebServName, SessionPtr->SessionId);
#endif
	    if ((ParReadWeb->ReqestType == HRT_GET) || (ParReadWeb->ReqestType == HTR_HEAD)) 
		{
		    HtmlHashRecPtr = FindHtmlPageHash(&GetHtmlPageHashHop,
					&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
					HtmlUserTypeMask);
		}
		else
		{
		    HtmlHashRecPtr = FindHtmlPageHash(&PostHtmlPageHashHop,
					&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
					HtmlUserTypeMask);
		}
                
		if (HtmlHashRecPtr)
		{
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Function for HTML request handle was found %d\r\n", 
				ThrWebServName, SessionPtr->SessionId);
#endif

			(((DEF_HTML_HANDLER*)(HtmlHashRecPtr->HtmlPageHandlerPtr))->FuncOnHandle)
				    (MemWebPageGenPtr, SessionPtr, StrCmdHTTP);                    
		    isHtmlPageTx = true;            
		}
	    if (isHtmlPageTx)
	    {
			if (!gFileNotFoundFlag)
			{
			    ServerStats.IntGoodUserReq++;
			    if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
			    else                                 ServerStats.IntGoodBotReq++;
			    if (!ParReadWeb->IsExtGen && !ParReadWeb->RespDelay)
				{
				    SrcPageLen = strlen(MemWebPageGenPtr);
				    if ((ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
					    ((SrcPageLen > ParWebServ->GeneralCfg.MinHtmlSizeCompress) ||
                        (ParReadWeb->isCompressRequired)) &&
					    (ParReadWeb->isEncodingAccept) && (ParReadWeb->BotType != BOT_YANDEX))
					{
				        StartSendRes = SendHttpHtmlPageHost(ParWebServ, ParReadWeb, SessionPtr,
					        (char*)MemWebPageGenPtr, SrcPageLen, 1, "page.html");
					}
				    else
					{
					    StartSendRes = SendHttpHtmlPageHost(ParWebServ, ParReadWeb, SessionPtr,
					        (char*)MemWebPageGenPtr, SrcPageLen, 0, "page.html");
					}
                    
		            if (!StartSendRes)
					{
						CloseHttpChan(ParReadWeb);
						ParReadWeb->isKeepAlive = false;
					}
			        else
					{
				        ServerStats.SendHtmlPages++;
					    ServerStats.IntSendHtmlPages++;
					}
				}
			}
			else
			{
                if (ParReadWeb->WebChanId == PRIMARY_WEB_CHAN)
                {
	                CreateHttpInvalidFileNameResp(MemWebPageGenPtr, ParReadWeb->LocalFileName, 
				        ParWebServ->LocalAddrIP, ParWebServ->ServCustomCfg.PrimWebAccIPPort);
                }
                else
                {
	                CreateHttpInvalidFileNameResp(MemWebPageGenPtr, ParReadWeb->LocalFileName, 
				        ParWebServ->LocalAddrIP, ParWebServ->ServCustomCfg.SecondWebAccIPPort);                
                }
				
				ParReadWeb->isKeepAlive = false;
				if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
					(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
					CloseHttpChan(ParReadWeb);
				ServerStats.IntBadUserReq++;
			}
	    }
		else
		{
			/* Try to find HTML page in html_data directory */
	        if ((ParReadWeb->ReqestType == HRT_GET) || (ParReadWeb->ReqestType == HTR_HEAD))
			{
				if (strlen(ParReadWeb->HtmlFileName) > 0)
				{
                    FileInfoPtr = FindFileHash(&FileHashHop, DirType, ParReadWeb->HtmlFileName);
                    if (FileInfoPtr)
					{
						if (ParReadWeb->FileType == FRT_HTML_PAGE)
						{
							/* external HTML file was requested */
	                        EndHtmlPageGenPtr = MemWebPageGenPtr;
	                        *MemWebPageGenPtr = 0;
							if (HandleHtmlTemplate(ParWebServ, ParReadWeb, SessionPtr, FileInfoPtr))
							{
			                    ServerStats.IntGoodUserReq++;
			                    if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
			                    else                                 ServerStats.IntGoodBotReq++;			
				                SrcPageLen = strlen(MemWebPageGenPtr);
				                if ((ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
					                ((SrcPageLen > ParWebServ->GeneralCfg.MinHtmlSizeCompress) ||
                                     (ParReadWeb->isCompressRequired)) &&
					                (ParReadWeb->isEncodingAccept) && (ParReadWeb->BotType != BOT_YANDEX))
								{
				                    StartSendRes = SendHttpHtmlPageHost(ParWebServ, ParReadWeb, SessionPtr,
					                    (char*)MemWebPageGenPtr, SrcPageLen, 1, &ParReadWeb->LocalFileName[i]);
								}
				                else
								{
					                StartSendRes = SendHttpHtmlPageHost(ParWebServ, ParReadWeb, SessionPtr,
					                    (char*)MemWebPageGenPtr, SrcPageLen, 0, &ParReadWeb->LocalFileName[i]);
								}
		                        if (!StartSendRes)
								{
									CloseHttpChan(ParReadWeb);
									ParReadWeb->isKeepAlive = true;
								}
			                    else
								{
				                    ServerStats.SendHtmlPages++;
					                ServerStats.IntSendHtmlPages++;
								}								
								isHtmlPageTx = true;										
							}
							else
							{
								ServerStats.IntBadUserReq++;
							}
						}
                        else
						{
							if ((ParReadWeb->IfModifyedSince > 0) && (ParReadWeb->IfModifyedSince < FileInfoPtr->LastUpdate))
							{
								SendFileNotModifyedStr(ParWebServ, ParReadWeb, FileInfoPtr->LastUpdateStr, FileInfoPtr->FileEtag);
							}							
							else
							{
								if(!SendHttpHashFileHost(ParWebServ, ParReadWeb, FileInfoPtr))
								{
									CloseHttpChan(ParReadWeb);
									ParReadWeb->isKeepAlive = false;
								}							
							}
							ServerStats.SendFiles++;
							ServerStats.IntSendFiles++;
							if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
							else                                 ServerStats.IntGoodBotReq++;
							isHtmlPageTx = true;
						}
					}
				}
			}
            
			if (!isHtmlPageTx)
			{
                if (ParReadWeb->WebChanId == PRIMARY_WEB_CHAN)
                {
	                CreateHttpInvalidFileNameResp(MemWebPageGenPtr, 
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift], 
				        ParWebServ->LocalAddrIP, ParWebServ->ServCustomCfg.PrimWebAccIPPort);
                }
                else
                {
	                CreateHttpInvalidFileNameResp(MemWebPageGenPtr, 
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				        ParWebServ->LocalAddrIP, ParWebServ->ServCustomCfg.SecondWebAccIPPort);                
                }
				
				ParReadWeb->isKeepAlive = false;
				if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
					(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
					CloseHttpChan(ParReadWeb);
				ServerStats.IntBadUserReq++;				
			}
		}
		DeltaTime = (unsigned int)(GetTickCount() - StartTick);
        if (DeltaTime > ServerStats.MaxPageGenTime) ServerStats.MaxPageGenTime = DeltaTime;

	    ParWebServ->SummHtmlPageGenTime += DeltaTime;
		ParWebServ->HtmlPageGenCount++;
	    ServerStats.AvgPageGenTime = (unsigned int)(ParWebServ->SummHtmlPageGenTime/ParWebServ->HtmlPageGenCount);

        SrcTimePtr = &LastPageGenTime[2];
        DestTimePtr = (SrcTimePtr + 1);
        *DestTimePtr-- = *SrcTimePtr--;
        *DestTimePtr-- = *SrcTimePtr--;
        *DestTimePtr-- = *SrcTimePtr--;
        *DestTimePtr = DeltaTime;
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (html)\r\n", ThrWebServName);
#endif
#ifdef _SERVER_PERF_MEASURE_
/*
	clock_gettime(CLOCK_REALTIME, &spec);
    EndTime = (unsigned long long int)spec.tv_sec * 1000000;
    EndTime += (unsigned long long int)(spec.tv_nsec / 1000);
    DeltaTime2 = (unsigned int)(EndTime - StartTime);
//    if (DeltaTime > 20)
        printf("DT: %u MSG: (%s)\n", (unsigned int)DeltaTime2, ParReadWeb->LocalFileName);
    if (DeltaTime2 > MaxTime) MaxTime = DeltaTime2;
    AvearageSummTime += DeltaTime2;
    SummCount++;*/
#endif
	    return;
	}

	if (ParReadWeb->FileType == FRT_CSV_DATA)
	{
		if (!CsvFileAccessValidate(ParWebServ, MemWebPageGenPtr, StrCmdHTTP))
		{
		    CreateHttpInvalidFileNameResp(MemWebPageGenPtr, 
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				LocalAdress, LocalPortServ );
			ParReadWeb->isKeepAlive = false;
		    if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			      (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
		    ServerStats.IntBadUserReq++;
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (csv)\r\n", ThrWebServName);
#endif
			return;
		}
	}
	else if (ParReadWeb->FileType == FRT_PDF_DOC)
	{
		if (!PdfFileAccessValidate(ParWebServ, MemWebPageGenPtr, StrCmdHTTP))
		{
		    CreateHttpInvalidFileNameResp(MemWebPageGenPtr, ParReadWeb->LocalFileName, LocalAdress, LocalPortServ );
			ParReadWeb->isKeepAlive = false;
		    if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			      (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
		    ServerStats.IntBadUserReq++;
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (pdf)\r\n", ThrWebServName);
#endif
			return;
		}
	}

	//Any files transmit.
	if (ParReadWeb->isFileRedirect) // File path redirection  prevention
	{
		CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
			&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
			LocalAdress, LocalPortServ );
		ParReadWeb->isKeepAlive = false;
		if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			      (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
			CloseHttpChan(ParReadWeb);
		ServerStats.IntBadUserReq++;
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (file 1)\r\n", ThrWebServName);
#endif
		return;
	}
    
	/* Check png image for capcha image */
	if ((ParReadWeb->FileType == FRT_PNG_PIC) &&
		(ParWebServ->CapchaTableSize > 0) && ParReadWeb->isCapchaFile)
	{
		for(;;)
		{
	        j = FindCmdRequest(ParReadWeb->StrCmdHTTP, KeyFormSessionId);
	        if (j == -1) break;
	        if (strlen(&ParReadWeb->StrCmdHTTP[j+1]) < SESSION_ID_KEY_LEN) break;
            SessionId = FindSessionByKey(&ParReadWeb->StrCmdHTTP[j+1]);
			if (!SessionId) break;
			SelSessionPtr = GetSessionBySessionId(&ParWebServ->SessionManager, SessionId);
			if (!SelSessionPtr) break;
		    if (SelSessionPtr->CapchaCode < ParWebServ->CapchaTableSize)
			{
	  #ifdef WIN32
			    sprintf(CapchaName, ".\\%04d.png",
	  #else
			    sprintf(CapchaName, "./%04d.png",
	  #endif
                    ParWebServ->CapchaTable[SelSessionPtr->CapchaCode]);
		        FileInfoPtr = FindFileHash(&FileHashHop, BASE_HASH_LIST, CapchaName);
		        if (FileInfoPtr) ParReadWeb->FileType = FRT_CAPCHA;
				ParseDone = true;
			}
			break;
		}
		if (!ParseDone) FileInfoPtr = NULL;

	}
	else
	{
        /* Try to find file in internal cache */
        if ((ParReadWeb->FileType == FRT_CSV_DATA) || (ParReadWeb->FileType == FRT_PDF_DOC)) FileInfoPtr = NULL;
        else
        {
            FileInfoPtr = FindFileHash(&FileHashHop, DirType, ParReadWeb->LocalFileName);
            if (!FileInfoPtr)
            {		
                /* Try to map ext file name to local file name */
                MapNamePtr = GetLocalFileNameByExtName(ParReadWeb->LocalFileName);
                if (MapNamePtr) FileInfoPtr = FindFileHash(&FileHashHop, DirType, MapNamePtr);
            }
        }
	}

    if (!FileInfoPtr)
    {    
#ifdef _LINUX_X86_
        HandelFIL = fopen(ParReadWeb->LocalFileName,"rb");
        if (!HandelFIL) 
        {		
		    CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				LocalAdress, LocalPortServ );
			ParReadWeb->isKeepAlive = false;
		    if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			    (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ServerStats.IntBadUserReq++;
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (file 2)\r\n", ThrWebServName);
#endif
	        return;
        }        
		
        stat(ParReadWeb->LocalFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
        {
            SizeFile = (unsigned int)st.st_size;
        }
        else
        {
            /* Pointed object is not file */
		    CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				LocalAdress, LocalPortServ );
			ParReadWeb->isKeepAlive = false;
		    if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			    (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			fclose(HandelFIL);
			ServerStats.IntBadUserReq++;
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (file 3)\r\n", ThrWebServName);
#endif
            return;
        }

		if (SizeFile > MAX_DWLD_HTTP_FILE_LEN)
		{
		    CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				LocalAdress, LocalPortServ );
			ParReadWeb->isKeepAlive = false;
		    if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			    (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
            fclose(HandelFIL);
			ServerStats.IntBadUserReq++;
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (file 4)\r\n", ThrWebServName);
#endif
			return;
		}

		if ((ParReadWeb->FileType == FRT_CSS_SCRIPT) ||
			(ParReadWeb->FileType == FRT_JS_SCRIPT) ||
			(ParReadWeb->FileType == FRT_TXT_DATA) ||
			(ParReadWeb->FileType == FRT_XML_DATA))
		{
			if ((ParReadWeb->IfModifyedSince > 0) && (ParReadWeb->IfModifyedSince < st.st_mtime))
			{
				SendFileNotModifyedTime(ParWebServ, ParReadWeb, st.st_mtime, 0);
				if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
				else                                 ServerStats.IntGoodBotReq++;
				fclose(HandelFIL);
			}
			else			
			{
				BlockRead = fread((unsigned char*)MemWebPageGenPtr, 1, SizeFile, HandelFIL);  			
				fclose(HandelFIL);
				if ((ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
					((SizeFile > ParWebServ->GeneralCfg.MinHtmlSizeCompress) ||
					(ParReadWeb->isCompressRequired)) && (ParReadWeb->isEncodingAccept))
				{
					StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb,
					MemWebPageGenPtr, SizeFile, NULL, 1, &ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
				}
				else
				{
					StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb,
					MemWebPageGenPtr, SizeFile, NULL, 0, &ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
				}
                    
				if (!StartSendRes)
				{
					CloseHttpChan(ParReadWeb);
					ParReadWeb->isKeepAlive = false;
				}
				else
				{
					if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
					else                                 ServerStats.IntGoodBotReq++;
				}
			}
		}
		else					
		{
			if ((ParReadWeb->FileType != FRT_CAPCHA) && (ParReadWeb->IfModifyedSince > 0) && 
				(ParReadWeb->IfModifyedSince < st.st_mtime))
			{
				SendFileNotModifyedTime(ParWebServ, ParReadWeb, st.st_mtime, 0);
				if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
				else                                 ServerStats.IntGoodBotReq++;
			}
			else
			{
				DataFileHttpSentHeaderGen(ParWebServ, ParReadWeb, MemWebPageGenPtr, (unsigned int)SizeFile, 
					ParReadWeb->FileType, 0, ParReadWeb->isKeepAlive, false);
				HeaderLen = strlen(MemWebPageGenPtr);
				if (ParReadWeb->ReqestType == HTR_HEAD)
				{
					fclose(HandelFIL);
					if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb,
						(char*)MemWebPageGenPtr, HeaderLen))
					{
						CloseHttpChan(ParReadWeb);
						ParReadWeb->isKeepAlive = false;
					}
				}
				else
				{
					BlockRead = fread((unsigned char*)&MemWebPageGenPtr[HeaderLen], 1, SizeFile, HandelFIL);  			
					fclose(HandelFIL);
					if (!SendHttpFileHost(&ParWebServ->SenderWorker, ParReadWeb,
						(char*)MemWebPageGenPtr, HeaderLen, SizeFile))
					{
						CloseHttpChan(ParReadWeb);
						ParReadWeb->isKeepAlive = false;
					}
					else
					{
						ServerStats.IntGoodUserReq++;
					}					
				}
			}
		}
    }
    else
    {
		if ((ParReadWeb->FileType != FRT_CAPCHA) && (ParReadWeb->IfModifyedSince > 0) && 
			(ParReadWeb->IfModifyedSince < FileInfoPtr->LastUpdate))
		{
			SendFileNotModifyedStr(ParWebServ, ParReadWeb, FileInfoPtr->LastUpdateStr, FileInfoPtr->FileEtag);
			if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
			else                                 ServerStats.IntGoodBotReq++;
		}
		else
		{
			if ((ParReadWeb->FileType == FRT_CSS_SCRIPT) ||
				(ParReadWeb->FileType == FRT_JS_SCRIPT) ||
				(ParReadWeb->FileType == FRT_TXT_DATA) ||
				(ParReadWeb->FileType == FRT_XML_DATA))
			{	
				if ((ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
					((FileInfoPtr->FileLen > ParWebServ->GeneralCfg.MinHtmlSizeCompress) ||
					(ParReadWeb->isCompressRequired)) && (ParReadWeb->isEncodingAccept))
				{
					StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb, NULL, 0, FileInfoPtr, 1, 
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
				}
				else
				{
					StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb, NULL, 0, FileInfoPtr, 0, 
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
				}
                    
				if (!StartSendRes)
				{
					CloseHttpChan(ParReadWeb);
					ParReadWeb->isKeepAlive = false;
				}
				else
				{
					if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
					else                                 ServerStats.IntGoodBotReq++;
				}			
			}		
			else
			{
				if(!SendHttpHashFileHost(ParWebServ, ParReadWeb, FileInfoPtr))
				{
					CloseHttpChan(ParReadWeb);
					ParReadWeb->isKeepAlive = false;
				}
				else
				{
					if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
					else                                 ServerStats.IntGoodBotReq++;
				}	
			}
		}
    }
#ifdef _SERVER_PERF_MEASURE_ /*
	clock_gettime(CLOCK_REALTIME, &spec);
    EndTime = (unsigned long long int)spec.tv_sec * 1000000;
    EndTime += (unsigned long long int)(spec.tv_nsec / 1000);
    DeltaTime2 = (unsigned int)(EndTime - StartTime);
        printf("DT: %u MSG: (%s)\n", (unsigned int)DeltaTime2, ParReadWeb->LocalFileName);
    if (DeltaTime2 > MaxTime) MaxTime = DeltaTime2;
    AvearageSummTime += DeltaTime2;
    SummCount++; */
#endif
	ServerStats.SendFiles++;
    ServerStats.IntSendFiles++;
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, 
				"%s: Handle of CreateResponse is done (file 5)\r\n", ThrWebServName);
#endif
	return;
} 
#endif
                   
#ifdef WIN32
        WaitResult = WaitForSingleObject(gFileMutex, INFINITE);
        switch(WaitResult)
	    {
	        case WAIT_OBJECT_0:
	            HFSndMess = CreateFile( ParReadWeb->LocalFileName,0,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	            if (HFSndMess == INVALID_HANDLE_VALUE)
		        {
		            CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
						LocalAdress, LocalPortServ );
					ParReadWeb->isKeepAlive = false;
		            if (!SendHttpRespUser(ParWebServ, ParReadWeb, 
			            (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
	                    CloseHttpChan(ParReadWeb);
                    if (!ReleaseMutex(gFileMutex)) 
		            { 
                        printf("Fail to release mutex (Web Server) in report manager task\r\n");
		            }
					ServerStats.IntBadUserReq++;
	                return;
		        }        
                SizeFile = GetFileSize(HFSndMess, NULL);                                
	            CloseHandle(HFSndMess);
	            HandelFIL = fopen( ParReadWeb->LocalFileName,"rb");
			    if (SizeFile > MAX_DWLD_HTTP_FILE_LEN)
			    {
		            CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
						LocalAdress, LocalPortServ );
					ParReadWeb->isKeepAlive = false;
		            if (!SendHttpRespUser(ParWebServ, ParReadWeb, 
			            (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
						CloseHttpChan(ParReadWeb);
                    fclose(HandelFIL);
                    if (!ReleaseMutex(gFileMutex)) 
		            { 
                        printf("Fail to release mutex (Web Server) in report manager task\r\n");
		            }
					ServerStats.IntBadUserReq++;
				    return;
			    }
				
                DataFileHttpSentHeaderGen(ParWebServ, ParReadWeb, 
					MemWebPageGenPtr, (unsigned int)SizeFile, 
					ParReadWeb->FileType, 0, ParReadWeb->isKeepAlive, false);
	            HeaderLen = strlen(MemWebPageGenPtr);
                if (ParReadWeb->ReqestType == HTR_HEAD)
				{
	                fclose(HandelFIL);
                    if (!ReleaseMutex(gFileMutex)) 
					{ 
                        printf("Fail to release mutex (Web Server) in report manager task\r\n");
					}
	                if (!SendHttpRespUser(ParWebServ, ParReadWeb,
		                (char*)MemWebPageGenPtr, HeaderLen))
	                    CloseHttpChan(ParReadWeb);
				}
				else
				{
					BlockRead = fread((unsigned char*)MemWebPageGenPtr, 1, SizeFile, HandelFIL);
					fclose(HandelFIL);
					if ((ParReadWeb->FileType == FRT_CSS_SCRIPT) ||
						(ParReadWeb->FileType == FRT_JS_SCRIPT) ||
						(ParReadWeb->FileType == FRT_TXT_DATA) ||
						(ParReadWeb->FileType == FRT_XML_DATA) ||
						(ParReadWeb->FileType == FRT_CSV_DATA))
					{
						if ((ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
							((FileInfoPtr->FileLen > ParWebServ->GeneralCfg.MinHtmlSizeCompress) ||
							(ParReadWeb->isCompressRequired)) && (ParReadWeb->isEncodingAccept))
						{
							StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb, MemWebPageGenPtr, SizeFile,
							    NULL, 1, &ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
						}
						else
						{
							StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb, MemWebPageGenPtr, SizeFile,
							    NULL, 0, &ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
						}
                    
						if (!StartSendRes) CloseHttpChan(ParReadWeb);
						else
						{
							ServerStats.IntGoodUserReq++;
						}
					}
					else					
					{
						if (!ReleaseMutex(gFileMutex)) 
						{ 
							printf("Fail to release mutex (Web Server) in report manager task\r\n");
						}
						if (!SendHttpRespUser(ParWebServ, ParReadWeb,
							(char*)MemWebPageGenPtr, (HeaderLen + SizeFile)))
							CloseHttpChan(ParReadWeb);
					}
				}
                break;

            case WAIT_ABANDONED: 
		        printf("The other thread that using mutex is closed in locked state of mutex\r\n");
                break;

		    default:
			    printf("Report manager (Web Server) mutex is fialed with error: %d\r\n", GetLastError());
			    break;
	    }
    }    
    else
    {
		if(!SendHttpHashFileHost(ParWebServ, ParReadWeb, FileInfoPtr))
		{
            CloseHttpChan(ParReadWeb);
		}
    }
	ServerStats.SendFiles++;
    ServerStats.IntSendFiles++;
	return;
}            
#endif
//---------------------------------------------------------------------------
void CreateContentResponse(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, SOCKET UserSocket,
	char *BufAnsw, char *LocalAdress, unsigned long CurrRemAddr, int LocalPortServ, HashDirTypeT DirType)
{
	int             j;
	unsigned int	i;
	unsigned int    DeltaTime;
	FILE			*HandelFIL;       
	char			*StrCmdHTTP = NULL;
	int				BlockRead;
	bool            isHtmlPageTx = false;
	bool            BadRequest = false;
	bool            ParseDone = false;
    unsigned int    *SrcTimePtr, *DestTimePtr;
	unsigned int    HeaderLen = 0;
	unsigned int    SrcPageLen;
	unsigned int    HtmlUserTypeMask = 0x01;
    unsigned int    LogStrLine;
#ifdef _SERVER_PERF_MEASURE_
    unsigned int           DeltaTime2;
    unsigned long long int StartTime, EndTime;
    struct timespec        spec;
#endif
	FILE_HASH_RECORD   *FileInfoPtr = NULL;
	char            *ComprDataBufPtr = NULL;
    DWORD           WaitResult;
	bool            StartSendRes;
	char            *CwdPtr = NULL;
    char            *MapNamePtr = NULL;
#ifdef WIN32
	DWORD         SizeFile;        
	HANDLE        HFSndMess;
#else       
    unsigned int  SizeFile;
    struct stat   st;        
#endif

#ifdef _SERVER_PERF_MEASURE_ /*
    clock_gettime(CLOCK_REALTIME, &spec);
    StartTime = (unsigned long long int)spec.tv_sec * 1000000;
    StartTime += (unsigned long long int)(spec.tv_nsec / 1000); */
#endif

#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Content http req (%d) file: %s\r\n",
		ThrWebServName, UserSocket, ParReadWeb->LocalFileName);
#endif
	isHtmlPageTx = false;
	*MemWebPageGenPtr = 0;
	ParReadHttpSocketPtr = ParReadWeb;
	ParWebServPtr = ParWebServ;

	EndHtmlPageGenPtr = MemWebPageGenPtr;
    *EndHtmlPageGenPtr = 0;
	
	if (ParWebServ->ServCustomCfg.FileRequestLogging)
	{
		AddStrWebPage(ParWebServ->LogTimeStampStr);
		AddStrWebPage(ThrWebServName);
		AddLenStrWebPage(": ", 2);
		AddStrWebPage(ParReadWeb->RequestInfoStr);
		AddLenStrWebPage("; Cmd=", 6);
		AddStrWebPage(ParReadWeb->StrCmdHTTP);
		AddLenStrWebPage("\r\n", 2);
		DebugLogStrPrint(MemWebPageGenPtr); 
		EndHtmlPageGenPtr = MemWebPageGenPtr;
	}
	
	/* HTML request load time calculation */
	DeltaTime = (unsigned int)(GetTickCount() - ParReadWeb->StartReqHandleTick);
    if (DeltaTime > ServerStats.MaxReqLoadTime) ServerStats.MaxReqLoadTime = DeltaTime;
	ParWebServ->SummHtmlReqLoadTime += DeltaTime;
	ParWebServ->HtmlReqLoadCount++;
	ServerStats.AvgReqLoadTime = (unsigned int)(ParWebServ->SummHtmlReqLoadTime/ParWebServ->HtmlReqLoadCount);
    
    SrcTimePtr = &LastReqLoadTime[2];
    DestTimePtr = (SrcTimePtr + 1);
    *DestTimePtr-- = *SrcTimePtr--;
    *DestTimePtr-- = *SrcTimePtr--;
    *DestTimePtr-- = *SrcTimePtr--;
    *DestTimePtr = DeltaTime;

	//Any files transmit.
	if (ParReadWeb->isFileRedirect) // File path redirection  prevention
	{
		CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
			&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
			LocalAdress, LocalPortServ );
		ParReadWeb->isKeepAlive = false;
		if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			      (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
			CloseHttpChan(ParReadWeb);
		ServerStats.IntBadUserReq++;
#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateContextResponse is done (file 1)\r\n", ThrWebServName);
#endif
		return;
	}
	
    /* Try to find file in internal cache */
    FileInfoPtr = FindFileHash(&FileHashHop, DirType, &ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
    if (!FileInfoPtr)
    {
        // Try to map ext file name to local file name
        MapNamePtr = GetLocalFileNameByExtName(&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
        if (MapNamePtr) FileInfoPtr = FindFileHash(&FileHashHop, DirType, MapNamePtr);
    }
	
    if (!FileInfoPtr)
    {	
#ifdef _LINUX_X86_
        HandelFIL = fopen(ParReadWeb->LocalFileName,"rb");
        if (!HandelFIL) 
        {
		    CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				LocalAdress, LocalPortServ );
			ParReadWeb->isKeepAlive = false;
		    if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			    (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
			ServerStats.IntBadUserReq++;
  #ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (file 2)\r\n", ThrWebServName);
  #endif
	        return;
        }        
        stat(ParReadWeb->LocalFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
        {
            SizeFile = (unsigned int)st.st_size;
        }
        else
        {
            /* Pointed object is not file */
		    CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				LocalAdress, LocalPortServ );
			ParReadWeb->isKeepAlive = false;
		    if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			    (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
            fclose(HandelFIL);
			ServerStats.IntBadUserReq++;
  #ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (file 3)\r\n", ThrWebServName);
  #endif
            return;
        }

		if (SizeFile > MAX_DWLD_HTTP_FILE_LEN)
		{
		    CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
				&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
				LocalAdress, LocalPortServ );
			ParReadWeb->isKeepAlive = false;
		    if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
			    (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
				CloseHttpChan(ParReadWeb);
            fclose(HandelFIL);
			ServerStats.IntBadUserReq++;
  #ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (file 4)\r\n", ThrWebServName);
  #endif
			return;
		}
	
		if ((ParReadWeb->IfModifyedSince > 0) && (ParReadWeb->IfModifyedSince < st.st_mtime))
		{
			SendFileNotModifyedTime(ParWebServ, ParReadWeb, st.st_mtime, 0);
			if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
			else                                 ServerStats.IntGoodBotReq++;
			fclose(HandelFIL);
			return;
		}
		
	    if ((ParReadWeb->FileType == FRT_HTML_PAGE) || 
            (ParReadWeb->FileType == FRT_HTR_PAGE) ||
			(ParReadWeb->FileType == FRT_CSS_SCRIPT) ||
			(ParReadWeb->FileType == FRT_JS_SCRIPT) ||
			(ParReadWeb->FileType == FRT_TXT_DATA) ||
			(ParReadWeb->FileType == FRT_XML_DATA) ||
			(ParReadWeb->FileType == FRT_CSV_DATA))
	    {
	        BlockRead = fread((unsigned char*)MemWebPageGenPtr, 1, SizeFile, HandelFIL);
	        fclose(HandelFIL);
			if (BlockRead)
			{
				if ((ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
					((SizeFile > ParWebServ->GeneralCfg.MinHtmlSizeCompress) ||
					(ParReadWeb->isCompressRequired)) && (ParReadWeb->isEncodingAccept))
				{
					StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb,
						MemWebPageGenPtr, SizeFile, NULL, 1, &ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
				}
				else
				{
					StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb,
						MemWebPageGenPtr, SizeFile, NULL, 0, &ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
				}
                    
				if (!StartSendRes)
				{
					CloseHttpChan(ParReadWeb);
					ParReadWeb->isKeepAlive = false;
				}
				else
				{
					ServerStats.IntGoodUserReq++;
				}				
			}
			else
			{
				CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
					&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
					LocalAdress, LocalPortServ );
				ParReadWeb->isKeepAlive = false;
				if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
					(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
					CloseHttpChan(ParReadWeb);
				fclose(HandelFIL);
				ServerStats.IntBadUserReq++;
  #ifdef _SERVDEBUG_
				DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (file 5)\r\n", ThrWebServName);
  #endif		
				return;
			}
		}
        else		
		{						
            DataFileHttpSentHeaderGen(ParWebServ, ParReadWeb, MemWebPageGenPtr, (unsigned int)SizeFile, 
			    ParReadWeb->FileType, 0, ParReadWeb->isKeepAlive, false);
	        HeaderLen = strlen(MemWebPageGenPtr);
		    if (ParReadWeb->ReqestType == HTR_HEAD)
		    {
			    fclose(HandelFIL);
                if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb,
	                (char*)MemWebPageGenPtr, HeaderLen))
				{
                    CloseHttpChan(ParReadWeb);
					ParReadWeb->isKeepAlive = false;
				}			
		    }
		    else
		    {
	            BlockRead = fread((unsigned char*)&MemWebPageGenPtr[HeaderLen], 1, SizeFile, HandelFIL);
	            fclose(HandelFIL);    
				if (BlockRead)
				{
					if (!SendHttpFileHost(&ParWebServ->SenderWorker, ParReadWeb,
						(char*)MemWebPageGenPtr, HeaderLen, SizeFile))
					{
						CloseHttpChan(ParReadWeb);
						ParReadWeb->isKeepAlive = false;
					}					
					ServerStats.IntGoodUserReq++;
				}
				else
				{
					CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
						LocalAdress, LocalPortServ );
					ParReadWeb->isKeepAlive = false;
					if (!SendHttpRespUser(&ParWebServ->SenderWorker, ParReadWeb, 
						(char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
						CloseHttpChan(ParReadWeb);
					fclose(HandelFIL);
					ServerStats.IntBadUserReq++;
  #ifdef _SERVDEBUG_
					DebugLogPrint(NULL, "%s: Handle of CreateResponse is done (file 5)\r\n", ThrWebServName);
  #endif		
					return;
				}
		    }
		}
    }
    else
    {
		if ((ParReadWeb->IfModifyedSince > 0) && (ParReadWeb->IfModifyedSince < FileInfoPtr->LastUpdate))
		{
			SendFileNotModifyedStr(ParWebServ, ParReadWeb, FileInfoPtr->LastUpdateStr, FileInfoPtr->FileEtag);
			if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
			else                                 ServerStats.IntGoodBotReq++;
		}
		else
		{
			if ((ParReadWeb->FileType == FRT_HTML_PAGE) || 
				(ParReadWeb->FileType == FRT_HTR_PAGE) ||
				(ParReadWeb->FileType == FRT_CSS_SCRIPT) ||
				(ParReadWeb->FileType == FRT_JS_SCRIPT) ||
				(ParReadWeb->FileType == FRT_TXT_DATA) ||
				(ParReadWeb->FileType == FRT_XML_DATA) ||
				(ParReadWeb->FileType == FRT_CSV_DATA))
			{
				if ((ParWebServ->GeneralCfg.HtmlPageComprssEnable) &&
				((FileInfoPtr->FileLen > ParWebServ->GeneralCfg.MinHtmlSizeCompress) ||
				(ParReadWeb->isCompressRequired)) && (ParReadWeb->isEncodingAccept))
				{
					StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb, NULL, 0, FileInfoPtr, 1, 
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
				}
				else
				{
					StartSendRes = SendHttpTextDataFileHost(ParWebServ, ParReadWeb, NULL, 0, FileInfoPtr, 0, 
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift]);
				}
                    
				if (!StartSendRes)
				{
					CloseHttpChan(ParReadWeb);
					ParReadWeb->isKeepAlive = false;
				}
				else
				{
					ServerStats.IntGoodUserReq++;
				}				
			}
			else
			{
				if(!SendHttpHashFileHost(ParWebServ, ParReadWeb, FileInfoPtr))
				{
					CloseHttpChan(ParReadWeb);
					ParReadWeb->isKeepAlive = false;
				}
				else
				{
					if (ParReadWeb->BotType == BOT_NONE) ServerStats.IntGoodUserReq++;
					else                                 ServerStats.IntGoodBotReq++;
				}				
			}
		}
    }
  #ifdef _SERVER_PERF_MEASURE_ /*
	clock_gettime(CLOCK_REALTIME, &spec);
    EndTime = (unsigned long long int)spec.tv_sec * 1000000;
    EndTime += (unsigned long long int)(spec.tv_nsec / 1000);
    DeltaTime2 = (unsigned int)(EndTime - StartTime);
        printf("DT: %u MSG: (%s)\n", (unsigned int)DeltaTime2, ParReadWeb->LocalFileName);
    if (DeltaTime2 > MaxTime) MaxTime = DeltaTime2;
    AvearageSummTime += DeltaTime2;
    SummCount++; */
  #endif
	ServerStats.SendFiles++;
    ServerStats.IntSendFiles++;
  #ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Handle of CreateContextResponse is done (file 5)\r\n", ThrWebServName);
  #endif
	return;
} 
#endif
                   
#ifdef WIN32
        WaitResult = WaitForSingleObject(gFileMutex, INFINITE);
        switch(WaitResult)
	    {
	        case WAIT_OBJECT_0:
	            HFSndMess = CreateFile( ParReadWeb->LocalFileName,0,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	            if (HFSndMess == INVALID_HANDLE_VALUE)
		        {
		            CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
						LocalAdress, LocalPortServ );
					ParReadWeb->isKeepAlive = false;
		            if (!SendHttpRespUser(ParWebServ, ParReadWeb, 
			            (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
	                    CloseHttpChan(ParReadWeb);
                    if (!ReleaseMutex(gFileMutex)) 
		            { 
                        printf("Fail to release mutex (Web Server) in report manager task\r\n");
		            }
					ServerStats.IntBadUserReq++;
	                return;
		        }        
                SizeFile = GetFileSize(HFSndMess, NULL);                                
	            CloseHandle(HFSndMess);
	            HandelFIL = fopen( ParReadWeb->LocalFileName,"rb");
			    if (SizeFile > MAX_DWLD_HTTP_FILE_LEN)
			    {
		            CreateHttpInvalidFileNameResp(MemWebPageGenPtr,
						&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift],
						LocalAdress, LocalPortServ );
					ParReadWeb->isKeepAlive = false;
		            if (!SendHttpRespUser(ParWebServ, ParReadWeb, 
			            (char*)MemWebPageGenPtr, strlen(MemWebPageGenPtr)))
						CloseHttpChan(ParReadWeb);
                    fclose(HandelFIL);
                    if (!ReleaseMutex(gFileMutex)) 
		            { 
                        printf("Fail to release mutex (Web Server) in report manager task\r\n");
		            }
					ServerStats.IntBadUserReq++;
				    return;
			    }            
				
                DataFileHttpSentHeaderGen(ParWebServ, ParReadWeb, 
					MemWebPageGenPtr, (unsigned int)SizeFile, 
					ParReadWeb->FileType, 0, ParReadWeb->isKeepAlive, false);
	            HeaderLen = strlen(MemWebPageGenPtr);
                if (ParReadWeb->ReqestType == HTR_HEAD)
				{
	                fclose(HandelFIL);
                    if (!ReleaseMutex(gFileMutex)) 
					{ 
                        printf("Fail to release mutex (Web Server) in report manager task\r\n");
					}
	                if (!SendHttpRespUser(ParWebServ, ParReadWeb,
		                (char*)MemWebPageGenPtr, HeaderLen))
					{
	                    CloseHttpChan(ParReadWeb);			
						ParReadWeb->isKeepAlive = false;
					}
				}
				else
				{
	                BlockRead = fread((unsigned char*)&MemWebPageGenPtr[HeaderLen], 1, SizeFile, HandelFIL);
	                fclose(HandelFIL);
                    if (!ReleaseMutex(gFileMutex)) 
					{ 
                        printf("Fail to release mutex (Web Server) in report manager task\r\n");
					}
	                if (!SendHttpRespUser(ParWebServ, ParReadWeb,
		                (char*)MemWebPageGenPtr, (HeaderLen + SizeFile)))
					{
	                    CloseHttpChan(ParReadWeb);
						ParReadWeb->isKeepAlive = false;
					}
				}
                break;

            case WAIT_ABANDONED: 
		        printf("The other thread that using mutex is closed in locked state of mutex\r\n");
                break;

		    default:
			    printf("Report manager (Web Server) mutex is fialed with error: %d\r\n", GetLastError());
			    break;
	    }
    }    
    else
    {
		if(!SendHttpHashFileHost(ParWebServ, ParReadWeb, FileInfoPtr))
		{
            CloseHttpChan(ParReadWeb);
			ParReadWeb->isKeepAlive = false;
		}
    }
	ServerStats.SendFiles++;
    ServerStats.IntSendFiles++;
}            
#endif
//---------------------------------------------------------------------------
