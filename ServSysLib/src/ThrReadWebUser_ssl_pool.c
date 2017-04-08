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

#include "ThrReadWebUser.h"
#include "ThrConnWeb.h"
#include "SysMessages.h"
#include "TrTimeOutThread.h"
#include "ThrReportMen.h"
#include "BotDataBase.h"
#include "SessionKeyHash.h"
#include "WebServMsgApi.h"

extern char KeyFormSessionId[];
extern char KeySessionId[];
extern char GenPageMain[];
extern char GenPageUserAuthReq[];
extern char HtmlDataPath[];
extern char CapchaFileIdName[];
extern unsigned char AsciiCharToHexOctet[];

#ifdef WIN32
extern HANDLE gMemoryMutex;
#else
extern sem_t  gMemoryMutex;
#endif

/* The list of tipes of files that are enabled for HTTP transmitting */
char DoubleNextLine[]  = "\r\n\r\n";
char SessionCookieId[] = "ISSISESSIONID=";
char CookieMarker[] = "Cookie:";
char BeginHttpReqMarker[] = "HTTP";
char ThrReadWebName[] = "ThrReadWebUser";

typedef struct {
	unsigned		BlockLen;
	unsigned char	*Data;
    POOL_RECORD_STRUCT *WebPoolPtr;
	POOL_RECORD_STRUCT *BlkPoolPtr;
} RECHTTPDATA;

#define TIME_OUT_READ_HTTP 15

static void HandleHttpReq(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb, char *BufAnsw);
static void HttpPageLoadListClean(READER_WEB_INFO *ReadWebInfoPtr, ListItsTask *ListPtr);
static void SendReaderThrCloseNotify(READER_WEB_INFO *ReadWebInfoPtr);
static void ReadWebMsgQueueCreate(ReadWebMsgQueue *p_msg_queue);
static void ReadWebMsgQueueDestroy(ReadWebMsgQueue *p_msg_queue);
static void ReadWebMsgQueuePost(ReadWebMsgQueue *p_msg_queue, void* p_message);
static bool GetReadWebMessageQueue(ReadWebMsgQueue *p_msg_queue, void **pp_message);
static void ReadWebMsgQueueMarkDestroy(ReadWebMsgQueue *p_msg_queue);
static ReadWebMessage* GetReadWebMsgPool(ReadWebChannel *ChannelPtr, unsigned char MsgTag);
static void FreeReadWebMsgPool(ReadWebChannel *ChannelPtr, ReadWebMessage *ReadWebMsgPtr);
static void OnWebReadHbTimerExp(unsigned int TimerId, void *DataPtr);
static void ValidHttpReqPostProcess(READWEBSOCK *ParReadWeb);
#ifdef _WEB_EPOOL_SOCKET_
static void SocketRemPool(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb);
#endif
//---------------------------------------------------------------------------
#ifdef WIN32
DWORD WINAPI THRReadWebUser(LPVOID Data)
{
	int				rs,i, j;
	char		    ContrEnd[5];
    READER_WEB_INFO	*ReadWebInfoPtr;
	READWEBSOCK     *ParReadWeb;
	RECHTTPDATA		*NewKadrData;
    ObjListTask		*PointTask;
	bool            EmptyRead = false;
	bool            MsgSentResult = false;
	bool			HeaderLoadDone;
	bool            HeadrCheck = false;
	bool			ErrReadHTTP;
	int             TcpErrCode;
	int             SslError;
	ListItsTask		ListHTTPData;
    unsigned char	*RecieveCMD;
	unsigned int    ContentLength = 0;
	unsigned int    BeginContentShift = 0;
	unsigned int    pars_read, ContentLdSize;
	POOL_RECORD_STRUCT *ReadWebBufPtr = NULL;
	POOL_RECORD_STRUCT *ReadBlkBufPtr = NULL;
    ReadWebChannel     *ChannelPtr = NULL;
	ReadWebMessage     *ReadWebMsgPtr = NULL;

    ReadWebInfoPtr = (READER_WEB_INFO*)Data;    
	ChannelPtr = &ReadWebInfoPtr->ReadChannel;
    printf("Reader worker thread startup\n");
	RecieveCMD = (unsigned char*)AllocateMemory((TCP_RX_BLOCK_SIZE+4)*sizeof(unsigned char));
	memset(&ContrEnd, 0, 5*sizeof(char));
	ListHTTPData.Count = 0;
	ListHTTPData.CurrTask = NULL;
	ListHTTPData.FistTask = NULL;
    CreatePool(&ReadWebInfoPtr->ReadWebPool, START_READ_WEB_BLK, TCP_RX_BLOCK_SIZE+4);
    CreatePool(&ReadWebInfoPtr->ReadBlkPool, START_READ_WEB_BLK, sizeof(RECHTTPDATA));
	HttpCmdMsgParserInit(&ReadWebInfoPtr->HttpCmdParserMessage, &ReadWebInfoPtr->HttpParserData);
	UserAgentMsgParserInit(&ReadWebInfoPtr->UserAgentParserMessage, &ReadWebInfoPtr->HttpParserData);

    CreateThreadTimerCBD(OnWebReadHbTimerExp, GetCurrentThreadId(), 30*TMR_ONE_SEC, WEB_READ_HB_CHECK_TMR_ID, true, ChannelPtr);
	DebugLogPrint(NULL, "Reader worker thread startup\n");

	while (GetReadWebMessageQueue(&ChannelPtr->ReadWebQueue, (void**)&ReadWebMsgPtr))
	{
        if (ReadWebMsgPtr->MsgTag == WRC_CLOSE_CHANNEL)
		{
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
			break;
		}
		else if (ReadWebMsgPtr->MsgTag == WRC_HB_REQ)
		{
			WebServerMsgSent(WSU_HBACTIONTHR, ReadWebInfoPtr, (void*)(tulong)(unsigned int)WHN_READER_IND);
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
			continue;
		}
		else if (ReadWebMsgPtr->MsgTag == WRC_WEB_READ_REQ)
		{
			ParReadWeb = (READWEBSOCK*)ReadWebMsgPtr->DataPtr;
		    FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
		}
		else
		{
            DebugLogPrint(NULL, "%s: Unexpected(0x%02X) message tag is received by WEB reader\n",
			    ThrReadWebName, ReadWebMsgPtr->MsgTag);
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
			continue;
		}

		if (ReadWebInfoPtr->isThreadStopReq) break;

		/* start of read Web request */
		EmptyRead = false;
		MsgSentResult = false;
		HeadrCheck = false;
        HeaderLoadDone = false;
	    memset(&ContrEnd, 0, 5*sizeof(char));
        ErrReadHTTP = false;
		ContentLength = 0;
		BeginContentShift = 0;
	    ParReadWeb->HTTPReqLen = 0;
	    ParReadWeb->ContentLen = 0;
	    ParReadWeb->ShiftBeginContent = 0;
	    ParReadWeb->Status = URP_BAD_REQ;
	    *ParReadWeb->LocalFileName = 0;

		if (!ParReadWeb->isSslAccept && ParReadWeb->SslPtr)
		{
			if (!ParReadWeb->SslPtr)
			{
				ParReadWeb->SslPtr = SSL_new(ParReadWeb->CtxPtr);
				if (!ParReadWeb->SslPtr)
				{
					DebugLogPrint(NULL, "%s: SSL (%d) fail to get SSL instance\r\n", 
						ThrReadWebName, ParReadWeb->HttpSocket);
					ParReadWeb->SslPtr = NULL;
					ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
					WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
					continue;
				}
			}

			rs = SSL_set_fd(ParReadWeb->SslPtr, ParReadWeb->HttpSocket);
			if (rs != 1)
			{
				SslError = SSL_get_error(ParReadWeb->SslPtr, rs); 				
				SSL_free(ParReadWeb->SslPtr);
				DebugLogPrint(NULL, "%s: SSL (%d) fail to set socket due to error: %d\r\n", 
					ThrReadWebName, ParReadWeb->HttpSocket, SslError);
				ParReadWeb->SslPtr = NULL;
				ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
				WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
				continue;
			}
			SSL_set_accept_state(ParReadWeb->SslPtr);

		    if (SSL_accept(ParReadWeb->SslPtr) == -1)
		    {
				SSL_free(ParReadWeb->SslPtr);
				ParReadWeb->SslPtr = NULL;
				DebugLogPrint(NULL, "%s: SSL (%d) fail to SSL accept\r\n", ThrReadWebName, ParReadWeb->HttpSocket);
			    HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
				ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
				WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
				continue;
		    }

			ParReadWeb->isSslAccept = true;
	    }

	    while( 1 )
		{
			if (ParReadWeb->SslPtr)
			{				
				rs = SSL_read(ParReadWeb->SslPtr, RecieveCMD, TCP_RX_BLOCK_SIZE);
				if (rs < 0)
				{
					SslError = SSL_get_error(ParReadWeb->SslPtr, rs); 
					if ((SslError == SSL_ERROR_WANT_READ) || (SslError == SSL_ERROR_WANT_WRITE))
					{
						Sleep(5);
						continue;
					}
					else
					{
						DebugLogPrint(NULL, "%s: SSL (%d) RX connection closed due to error: %d/%d (RX: %d)\r\n",
									 ThrReadWebName, ParReadWeb->HttpSocket, rs, SslError,  ParReadWeb->HTTPReqLen);
						HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
						ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
						break;
					}
				}
			}
			else
			{
				rs = recv(ParReadWeb->HttpSocket, (char*)RecieveCMD, TCP_RX_BLOCK_SIZE, 0 );
				if (rs == SOCKET_ERROR)
				{
					TcpErrCode = WSAGetLastError();
					if ((TcpErrCode == WSAEINPROGRESS) || (TcpErrCode == WSAENOBUFS))
					{
						 Sleep(1);
						 continue;
					}
		 			else if (TcpErrCode == WSAETIMEDOUT)
					{
						EmptyRead = true;
					}
					else
					{
						DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to error: %d (RX: %d)\r\n",
							 ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
							 ParReadWeb->HTTPReqLen);
						HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
						ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
						break;
					}
				}
			}

		    if ((rs == 0) || (EmptyRead == true)) // Check for no data was read from HTTP socket.
			{              
			    DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to timeout: %d (RX: %d)\r\n",
				    ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
				    ParReadWeb->HTTPReqLen);
			    HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
			    ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
		        break;
			}
		    else
			{
		        RecieveCMD[rs] = 0;
			    if ((rs + ParReadWeb->HTTPReqLen) > MAX_LOAD_HTML_PAGE_SIZE)
				{
			        DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due large page size: %d (RX: %d)\r\n",
					    ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
				        ParReadWeb->HTTPReqLen + rs);
			 	    HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
				    ParReadWeb->Status = URP_HTTP_REQ_DATA_LARGE;
			        break;
				}
#ifdef _HTTP_READ_DEBUG_
                printf("Sock: %d load header block (len=%d)\n", ParReadWeb->HttpSocket, rs);
#endif

	            ReadBlkBufPtr = GetBuffer(&ReadWebInfoPtr->ReadBlkPool);
	            NewKadrData = (RECHTTPDATA*)ReadBlkBufPtr->DataPtr;
                NewKadrData->BlkPoolPtr = ReadBlkBufPtr;
	            ReadWebBufPtr = GetBuffer(&ReadWebInfoPtr->ReadWebPool);
	            NewKadrData->Data = (unsigned char*)ReadWebBufPtr->DataPtr;
                NewKadrData->WebPoolPtr = ReadWebBufPtr;

		        NewKadrData->BlockLen = rs;
		        memcpy(NewKadrData->Data, RecieveCMD, rs);
		        AddStructList(&ListHTTPData, NewKadrData);
		        ParReadWeb->HTTPReqLen += (unsigned int)rs;
			    if (!HeaderLoadDone)
				{
		            for (i=0;i < rs;i++)
		            {
		                memcpy(&ContrEnd[0], &ContrEnd[1], 3);
                        if (!RecieveCMD[i])
                        {
                            ErrReadHTTP = true;
                            break;
                        }
			            ContrEnd[3] = RecieveCMD[i];
			            //if ( strncmp( ContrEnd, DoubleNextLine, 4 ) == 0 )
						if (memcmp(ContrEnd, DoubleNextLine, 4) == 0)
						{
			                HeaderLoadDone = true;
				            break;
						}
		            }

                    if (ErrReadHTTP)
                    {
			            DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due unexpected zero char: %d (RX: %d)\r\n",
				            ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
				            ParReadWeb->HTTPReqLen + rs);
				        HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
				        ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;                            
                        break;
                    }
				}
			    else
				{
				    ContentLdSize += rs;
				    if (ContentLength <= ContentLdSize)
					{
#ifdef _HTTP_READ_DEBUG_
                        printf("%s: Sock: %d load last content block (len=%d)\n", 
						    ThrReadWebName, ParReadWeb->HttpSocket, rs);
#endif              
			            ParReadWeb->HttpReqPtr = (char*)AllocateMemory(ParReadWeb->HTTPReqLen+4);
			            ParReadWeb->HTTPReqLen = 0;
			            while(ListHTTPData.Count)
						{
		                    PointTask = (ObjListTask*)ListHTTPData.FistTask;
				            NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
				            memcpy( &ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen], NewKadrData->Data, NewKadrData->BlockLen );
				            ParReadWeb->HTTPReqLen += (unsigned int)NewKadrData->BlockLen;
						    if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
							else                         FreeMemory(NewKadrData->Data);
                            FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
				            RemStructList( &ListHTTPData, PointTask );
						}
			            ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen] = 0;
					    ParReadWeb->Status = URP_SUCCESS;
#ifdef _HTTP_READ_DEBUG_
                        printf("%s: Sock: %d Content load done (len=%d)\n",
						    ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
#endif
					    break;
					}
#ifdef _HTTP_READ_DEBUG_
                    else
					{
                        printf("%s: Sock: %d load content block (len=%d)\n",
						    ThrReadWebName, ParReadWeb->HttpSocket, rs);
					}
#endif
				}
			}

		    if ((HeaderLoadDone = true) && (!HeadrCheck))
			{
			    ParReadWeb->HttpReqPtr = (char*)AllocateMemory(ParReadWeb->HTTPReqLen+4);
			    ParReadWeb->HTTPReqLen = 0;
			    while(ListHTTPData.Count)
				{
		            PointTask = (ObjListTask*)ListHTTPData.FistTask;
				    NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
				    memcpy( &ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen], NewKadrData->Data, NewKadrData->BlockLen );
				    ParReadWeb->HTTPReqLen += (unsigned int)NewKadrData->BlockLen;
					if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
					else                         FreeMemory(NewKadrData->Data);
                    FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
				    RemStructList( &ListHTTPData, PointTask );
				}
			    ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen] = 0;
			    HeadrCheck = true;
			    ParReadWeb->Status = URP_SUCCESS;
#ifdef _HTTP_READ_DEBUG_
                printf("%s: Sock: %d HTTP header read done (len=%d)\n",
				    ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
#endif
			    i = FindCmdRequestLine(ParReadWeb->HttpReqPtr, "POST");
			    if (i == -1) break;
#ifdef _HTTP_READ_DEBUG_
                printf("%s: HTTP header contains POST request\n". ThrReadWebName);
#endif
			    i = FindCmdRequest(ParReadWeb->HttpReqPtr, "Content-Length:");
	            pars_read = sscanf(&ParReadWeb->HttpReqPtr[i], "%d", &ContentLength);
			    if (!pars_read) break;
			    j = FindCmdRequest(&ParReadWeb->HttpReqPtr[i], DoubleNextLine);
			    if (j == -1) break;
			    ParReadWeb->ShiftBeginContent = i + j;
			    ContentLdSize = ParReadWeb->HTTPReqLen - ParReadWeb->ShiftBeginContent;
			    ParReadWeb->ContentLen = ContentLength;
#ifdef _HTTP_READ_DEBUG_
                printf("%s: Sock: %d POST request load %d, %d\n", 
				    ThrReadWebName, ParReadWeb->HttpSocket, ContentLength, ContentLdSize);
#endif
			    if (ContentLength <= ContentLdSize) break;

	            ReadBlkBufPtr = GetBuffer(&ReadWebInfoPtr->ReadBlkPool);
	            NewKadrData = (RECHTTPDATA*)ReadBlkBufPtr->DataPtr;
                NewKadrData->BlkPoolPtr = ReadBlkBufPtr;
                NewKadrData->WebPoolPtr = NULL;
		        NewKadrData->BlockLen = ParReadWeb->HTTPReqLen;
		        NewKadrData->Data = (unsigned char*)ParReadWeb->HttpReqPtr;
		        AddStructList(&ListHTTPData, NewKadrData);
			    ParReadWeb->HttpReqPtr = NULL;
			}
		}

	    if ((ParReadWeb->HttpReqPtr) && (ParReadWeb->Status == URP_SUCCESS)) 
		    HandleHttpReq(ReadWebInfoPtr, ParReadWeb, ParReadWeb->HttpReqPtr);
	    if (ParReadWeb->Status != URP_SUCCESS)
		{
	        if (ParReadWeb->BoundInd)      FreeMemory(ParReadWeb->BoundInd);
		    if (ParReadWeb->PicFileBufPtr) FreeMemory(ParReadWeb->PicFileBufPtr);
            if (ParReadWeb->XmlFileBufPtr) FreeMemory(ParReadWeb->XmlFileBufPtr);
		    if (ParReadWeb->StrCmdHTTP)    FreeMemory(ParReadWeb->StrCmdHTTP);		
		    ParReadWeb->BoundInd = NULL;
		    ParReadWeb->PicFileBufPtr = NULL;
            ParReadWeb->XmlFileBufPtr = NULL;
		    ParReadWeb->StrCmdHTTP = NULL;		
		    if (ParReadWeb->Status != URP_NOT_SUPPORT_METHOD)
			{
			    if (ParReadWeb->HttpReqPtr)    FreeMemory(ParReadWeb->HttpReqPtr);
			    ParReadWeb->HttpReqPtr = NULL;
			    ParReadWeb->HTTPReqLen = 0;
			}
		}
		else
		{
			ValidHttpReqPostProcess(ParReadWeb);
		}

    #ifdef _SERVDEBUG_
	    DebugLogPrint(NULL, "%s: Read HTTP request is done (%d)\r\n", 
		    ThrReadWebName, ParReadWeb->HttpSocket);
    #endif
	    WebServerMsgSent(ParReadWeb->IDMessReadWeb, ParReadWeb, 0);
        HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
	}

	CloseThreadTimer(GetCurrentThreadId(), WEB_READ_HB_CHECK_TMR_ID);
    SendReaderThrCloseNotify(ReadWebInfoPtr);
	FreeMemory(RecieveCMD);
	UserAgentMsgParserDestroy(&ReadWebInfoPtr->UserAgentParserMessage);
	HttpCmdMsgParserDestroy(&ReadWebInfoPtr->HttpCmdParserMessage);
	ReadWebInfoPtr->isThreadReady = false;
	WebServerMsgSent(ParReadWeb->IDMessReadWeb, ParReadWeb, 0);    
	WSASetLastError(0);
    ExitThread(0);
}
#endif

#ifdef _LINUX_X86_
void* THRReadWebUser(void *arg)
{
	int				rs, i, j;
	char		    ContrEnd[5];
    READER_WEB_INFO	*ReadWebInfoPtr;
	READWEBSOCK     *ParReadWeb;
	RECHTTPDATA		*NewKadrData;
    ObjListTask		*PointTask;
	bool            EmptyRead = false;
	bool            MsgSentResult = false;
	bool			HeaderLoadDone;
	bool            HeadrCheck = false;
	bool			ErrReadHTTP;
	int             TcpErrCode, SslError;
	ListItsTask		ListHTTPData;
    unsigned char	*RecieveCMD;
	unsigned char   *SrcPtr;
	unsigned char   *CntrPtr;
	unsigned int    ContentLength = 0;
	unsigned int    BeginContentShift = 0;
	unsigned int    pars_read, ContentLdSize;
    struct sockaddr_in reader_addr;
    ReadWebChannel     *ChannelPtr = NULL;
	ReadWebMessage     *ReadWebMsgPtr = NULL;
	POOL_RECORD_STRUCT *ReadWebBufPtr = NULL;
	POOL_RECORD_STRUCT *ReadBlkBufPtr = NULL;
#ifdef _WEB_EPOOL_SOCKET_
	bool               IsReadEnd;
    int                nfds, Status;
	unsigned int       PoolTimeOut;
#else
    fd_set          master_rset, work_rset;
    struct timeval  select_time;
    int             maxfdp = 0;
    int		        Select_result;
#endif
#ifdef _READER_PERF_MEASURE_
    unsigned int           DeltaTime;
    unsigned long long int StartTime, EndTime;
    struct timespec        spec;
#endif

	ReadWebInfoPtr = (READER_WEB_INFO*)arg;
	ChannelPtr = &ReadWebInfoPtr->ReadChannel;
	RecieveCMD = (unsigned char*)AllocateMemory((TCP_RX_BLOCK_SIZE+4)*sizeof(unsigned char));
	if (!RecieveCMD)
	{
	    printf("Fail to memory allocate for RecieveCMD in read thread\n");
	    pthread_exit((void *)0);
	}
	memset(&ContrEnd, 0, 5*sizeof(char));
	ListHTTPData.Count = 0;
	ListHTTPData.CurrTask = NULL;
	ListHTTPData.FistTask = NULL;

#ifdef _WEB_EPOOL_SOCKET_
    ReadWebInfoPtr->epollfd = epoll_create(MAX_WEB_SESSION_PER_THREAD);
    if (ReadWebInfoPtr->epollfd == -1) 
    {
        printf("\n%s Failed to epool create (errno %d)\n",
            SetTimeStampLine(), errno);
		FreeMemory(RecieveCMD);
        pthread_exit((void *)0);               
    }
#endif
    CreatePool(&ReadWebInfoPtr->ReadWebPool, START_READ_WEB_BLK, (TCP_RX_BLOCK_SIZE + 4));
    CreatePool(&ReadWebInfoPtr->ReadBlkPool, START_READ_WEB_BLK, sizeof(RECHTTPDATA));
	HttpCmdMsgParserInit(&ReadWebInfoPtr->HttpCmdParserMessage, &ReadWebInfoPtr->HttpParserData);
	UserAgentMsgParserInit(&ReadWebInfoPtr->UserAgentParserMessage, &ReadWebInfoPtr->HttpParserData);

	CreateThreadTimerCBD(OnWebReadHbTimerExp, ReadWebInfoPtr->NotifyPort, 30*TMR_ONE_SEC, WEB_READ_HB_CHECK_TMR_ID, true, ChannelPtr);
	DebugLogPrint(NULL, "Reader worker thread startup (NP: %d, WSP: %d)\n", 
        ReadWebInfoPtr->NotifyPort, ReadWebInfoPtr->WebServPort);

    /* Read the data from the WEB reader messages queue. */
    while (GetReadWebMessageQueue(&ChannelPtr->ReadWebQueue, (void**)&ReadWebMsgPtr))
    {
        if (ReadWebMsgPtr->MsgTag == WRC_CLOSE_CHANNEL)
		{
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
			break;
		}
		else if (ReadWebMsgPtr->MsgTag == WRC_HB_REQ)
		{
	        WebServerMsgSent(WSU_HBACTIONTHR, ReadWebInfoPtr, (void*)(tulong)(unsigned int)WHN_READER_IND);
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
			continue;
		}
		else if (ReadWebMsgPtr->MsgTag == WRC_WEB_READ_REQ)
		{
			ParReadWeb = (READWEBSOCK*)ReadWebMsgPtr->DataPtr;
		    FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
			if (!ParReadWeb) continue;
		}
		else
		{
            DebugLogPrint(NULL, "%s: Unexpected(0x%02X) message tag is received by WEB reader\n",
			    ThrReadWebName, ReadWebMsgPtr->MsgTag);
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
			continue;
		}

		if (ReadWebInfoPtr->isThreadStopReq) break;
#ifdef _READER_PERF_MEASURE_
        clock_gettime(CLOCK_REALTIME, &spec);
        StartTime = (unsigned long long int)spec.tv_sec * 1000000;
        StartTime += (unsigned long long int)(spec.tv_nsec / 1000);
#endif
		EmptyRead = false;
		MsgSentResult = false;
		HeadrCheck = false;
        HeaderLoadDone = false;
	    memset(&ContrEnd, 0, 5*sizeof(char));
        ErrReadHTTP = false;
		ContentLength = 0;
		BeginContentShift = 0;
	    ParReadWeb->HTTPReqLen = 0;
	    ParReadWeb->ContentLen = 0;
	    ParReadWeb->ShiftBeginContent = 0;
	    ParReadWeb->Status = URP_BAD_REQ;
	    *ParReadWeb->LocalFileName = 0;
		ParReadWeb->MobileType = NULL;

		if (!ParReadWeb->isSslAccept && ParReadWeb->CtxPtr)
		{
			if (!ParReadWeb->SslPtr)
			{
				ParReadWeb->SslPtr = SSL_new(ParReadWeb->CtxPtr);
				if (!ParReadWeb->SslPtr)
				{
					DebugLogPrint(NULL, "%s: SSL (%d) fail to get SSL instance\r\n", 
						ThrReadWebName, ParReadWeb->HttpSocket);
					ParReadWeb->SslPtr = NULL;
					ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
					WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
					continue;
				}
			}

			rs = SSL_set_fd(ParReadWeb->SslPtr, ParReadWeb->HttpSocket);
			if (rs != 1)
			{
				SslError = SSL_get_error(ParReadWeb->SslPtr, rs); 
				SSL_free(ParReadWeb->SslPtr);
				ParReadWeb->SslPtr = NULL;
				DebugLogPrint(NULL, "%s: SSL (%d) fail to set socket due to error: %d\r\n", 
					ThrReadWebName, ParReadWeb->HttpSocket, SslError);
				ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
				WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
				continue;
			}

			SSL_set_accept_state(ParReadWeb->SslPtr);
		    if (SSL_accept(ParReadWeb->SslPtr) == -1)
		    {
				SSL_free(ParReadWeb->SslPtr);
				ParReadWeb->SslPtr = NULL;
				DebugLogPrint(NULL, "%s: SSL (%d) fail to SSL accept\r\n",
					ThrReadWebName, ParReadWeb->HttpSocket);
				ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
				WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
				continue;
		    }
			ParReadWeb->isSslAccept = true;
	    }

		/* Start handle*/
//		if (!ParReadWeb->SslPtr)
//		{
#ifdef _WEB_EPOOL_SOCKET_ 
			ReadWebInfoPtr->Ev.events = EPOLLIN;
			ReadWebInfoPtr->Ev.data.fd = ParReadWeb->HttpSocket;
			if (epoll_ctl(ReadWebInfoPtr->epollfd, EPOLL_CTL_ADD, 
				ParReadWeb->HttpSocket, &ReadWebInfoPtr->Ev) == -1)
			{
				DebugLogPrint(NULL, "%s: Failed to epool ctl for listen thr %u socket (errno %d)\r\n",
					ThrReadWebName, ParReadWeb->HttpSocket, errno);
				ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
				WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
				continue;
			}
			IsReadEnd = false;
#else
            FD_ZERO (&master_rset);
            FD_SET (ParReadWeb->HttpSocket, &master_rset);
            maxfdp = ParReadWeb->HttpSocket;
#endif
//	    }

#ifdef _HTTP_READ_DEBUG_
		DebugLogPrint(NULL, "%s: HTTP headr load start: Sct=%d\r\n",
			ThrReadWebName, ParReadWeb->HttpSocket);
#endif

	    for(;;)
		{
			if (ParReadWeb->SslPtr)
			{

#ifdef _WEB_EPOOL_SOCKET_   
				if (ParReadWeb->isKeepAlive) PoolTimeOut = ParReadWeb->KeepAliveTime;
				else                         PoolTimeOut = TIME_OUT_READ_HTTP;
				nfds = epoll_wait(ReadWebInfoPtr->epollfd, ReadWebInfoPtr->Events, 
					MAX_WEB_SESSION_PER_THREAD, PoolTimeOut*1000);
				if (nfds == -1) 
				{
					DebugLogPrint(NULL, "%s: The epool_wait is failed %d\n", ThrReadWebName, errno);
					HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
                    SocketRemPool(ReadWebInfoPtr, ParReadWeb);
					ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
					break;            
				}

				if (nfds == 0)
				{
					// Timeout of wait for read is detected
					DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to timeout: (RX: %d)\r\n",
						ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
					HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
					SocketRemPool(ReadWebInfoPtr, ParReadWeb);
					ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
					break;
				}
				for (i = 0; i < nfds;i++)
				{
					if (ReadWebInfoPtr->Events[i].data.fd == ParReadWeb->HttpSocket)
#else           
				memcpy(&work_rset, &master_rset, sizeof(master_rset));    
				if (ParReadWeb->isKeepAlive) select_time.tv_sec = (long)ParReadWeb->KeepAliveTime;
				else                         select_time.tv_sec = TIME_OUT_READ_HTTP;
				select_time.tv_usec = 0;
				Select_result = select(maxfdp+1, &work_rset, NULL, NULL, &select_time);        
				if (Select_result < 0)
				{
					if ((errno == EAGAIN) || (errno == EINTR))
					{
						continue;
					}
					else
					{
						DebugLogPrint(NULL, "%s: The socket select operation data load is failed due to error: %d\r\n",
							 ThrReadWebName, errno);
						break;
					}
				}
				else if (Select_result > 0)
				{
					if (FD_ISSET(ParReadWeb->HttpSocket, &work_rset))
#endif
					{
						goto ReadReady;
					}
				}
#ifdef _WEB_EPOOL_SOCKET_
				if (IsReadEnd) break;
#else
				else
				{
					// Timeout of wait for read is detected
					DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to timeout: (RX: %d)\r\n",
						ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
					HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
					ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
					break;
				}
				continue;
#endif

					
ReadReady:				

 printf("Enter SSL_read: Sct: %u\r\n", ParReadWeb->HttpSocket);
				rs = SSL_read(ParReadWeb->SslPtr, RecieveCMD, TCP_RX_BLOCK_SIZE);
				SslError = SSL_get_error(ParReadWeb->SslPtr, rs); 
 printf("Leave SSL_read: rs: %d, err: %d, Sct: %u\r\n", rs, SslError, ParReadWeb->HttpSocket);				
				if (rs < 0)
				{				
					if (SslError == SSL_ERROR_WANT_WRITE)
					{
						Sleep(5);
						continue;
					}
					else
					{
						DebugLogPrint(NULL, "%s: SSL (%d) RX connection closed due to error: %d (RX: %d)\r\n",
									 ThrReadWebName, ParReadWeb->HttpSocket, SslError, 
									 ParReadWeb->HTTPReqLen);
						HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
		#ifdef _WEB_EPOOL_SOCKET_
					    SocketRemPool(ReadWebInfoPtr, ParReadWeb);
        #endif
						ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
						break;
					}
				}
				else if (rs == 0) // Check for no data was read from HTTPS socket.
				{				
					if (SslError == SSL_ERROR_NONE)
					{
					    DebugLogPrint(NULL, "%s: SSL (%d) RX connection closed due to timeout (RX: %d)\r\n",
						    ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
					    ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
					}
					else
					{
						DebugLogPrint(NULL, "%s: SSL (%d) RX connection closed due to error: %d (RX: %d)\r\n",
							ThrReadWebName, ParReadWeb->HttpSocket, SslError, ParReadWeb->HTTPReqLen);
						ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
					}
					HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
		#ifdef _WEB_EPOOL_SOCKET_
					SocketRemPool(ReadWebInfoPtr, ParReadWeb);
        #endif
					break;
				}
				
				RecieveCMD[rs] = 0;
				if ((rs + ParReadWeb->HTTPReqLen) > MAX_LOAD_HTML_PAGE_SIZE)
				{
					DebugLogPrint(NULL, "%s: SSL (%d) RX connection closed due large page size: %d (RX: %d)\r\n",
						ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
						ParReadWeb->HTTPReqLen + rs);
			 		HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
		#ifdef _WEB_EPOOL_SOCKET_
					SocketRemPool(ReadWebInfoPtr, ParReadWeb);
        #endif
					ParReadWeb->Status = URP_HTTP_REQ_DATA_LARGE;
					break;
				}
#ifdef _HTTP_READ_DEBUG_
				printf("Sock: %d load header block (len=%d)\n", ParReadWeb->HttpSocket, rs);
#endif
				ReadBlkBufPtr = GetBuffer(&ReadWebInfoPtr->ReadBlkPool);
				NewKadrData = (RECHTTPDATA*)ReadBlkBufPtr->DataPtr;
				NewKadrData->BlkPoolPtr = ReadBlkBufPtr;
				ReadWebBufPtr = GetBuffer(&ReadWebInfoPtr->ReadWebPool);
				NewKadrData->Data = (unsigned char*)ReadWebBufPtr->DataPtr;
				NewKadrData->WebPoolPtr = ReadWebBufPtr;

				NewKadrData->BlockLen = rs;
				memcpy(NewKadrData->Data, RecieveCMD, rs);
				AddStructList(&ListHTTPData, NewKadrData);
				ParReadWeb->HTTPReqLen += (unsigned int)rs;
				if (!HeaderLoadDone)
				{
					SrcPtr = RecieveCMD;
					CntrPtr = &ContrEnd[0];
					for (i=0;i < rs;i++)
					{
						memcpy(CntrPtr, (CntrPtr+1), 3);
						if (!*SrcPtr)
						{
							ErrReadHTTP = true;
							break;
						}
						*(CntrPtr+3) = *SrcPtr++;
						if (memcmp(ContrEnd, DoubleNextLine, 4) == 0)
						{
							HeaderLoadDone = true;
							break;
						}
					}

					if (ErrReadHTTP)
					{
						DebugLogPrint(NULL, "%s: SSL (%d) RX connection closed due unexpected zero char: %d (RX: %d)\r\n",
							ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
							ParReadWeb->HTTPReqLen + rs);
						HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
		#ifdef _WEB_EPOOL_SOCKET_
					    SocketRemPool(ReadWebInfoPtr, ParReadWeb);
        #endif
						ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;                            
						break;
					}
				}
				else
				{
					ContentLdSize += rs;
					if (ContentLength <= ContentLdSize)
					{
#ifdef _HTTP_READ_DEBUG_
						printf("%s: Sock: %d load last content block (len=%d)\n", 
							ThrReadWebName, ParReadWeb->HttpSocket, rs);
#endif              
						ParReadWeb->HttpReqPtr = (char*)AllocateMemory(ParReadWeb->HTTPReqLen+4);
						ParReadWeb->HTTPReqLen = 0;
						while(ListHTTPData.Count)
						{
							PointTask = (ObjListTask*)ListHTTPData.FistTask;
							NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
							memcpy( &ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen], NewKadrData->Data, NewKadrData->BlockLen );
							ParReadWeb->HTTPReqLen += (unsigned int)NewKadrData->BlockLen;
							if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
							else                         FreeMemory(NewKadrData->Data);
							FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
							RemStructList( &ListHTTPData, PointTask );
						}
						ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen] = 0;
						ParReadWeb->Status = URP_SUCCESS;
		#ifdef _WEB_EPOOL_SOCKET_
					    SocketRemPool(ReadWebInfoPtr, ParReadWeb);
        #endif
#ifdef _HTTP_READ_DEBUG_
						printf("%s: Sock: %d Content load done (len=%d)\n",
							ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
#endif
						break;
					}
#ifdef _HTTP_READ_DEBUG_
					else
					{
						printf("%s: Sock: %d load content block (len=%d)\n",
							ThrReadWebName, ParReadWeb->HttpSocket, rs);
					}
#endif
				}

				if ((HeaderLoadDone = true) && (!HeadrCheck))
				{
					ParReadWeb->HttpReqPtr = (char*)AllocateMemory(ParReadWeb->HTTPReqLen+4);
					ParReadWeb->HTTPReqLen = 0;
					while(ListHTTPData.Count)
					{
						PointTask = (ObjListTask*)ListHTTPData.FistTask;
						NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
						memcpy( &ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen], NewKadrData->Data, NewKadrData->BlockLen );
						ParReadWeb->HTTPReqLen += (unsigned int)NewKadrData->BlockLen;
						if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
						else                         FreeMemory(NewKadrData->Data);
						FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
						RemStructList( &ListHTTPData, PointTask );
					}
					ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen] = 0;
					HeadrCheck = true;
					ParReadWeb->Status = URP_SUCCESS;
		#ifdef _WEB_EPOOL_SOCKET_
					SocketRemPool(ReadWebInfoPtr, ParReadWeb);
        #endif
#ifdef _HTTP_READ_DEBUG_
					printf("%s: Sock: %d HTTP header read done (len=%d)\n",
						ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
#endif
					i = FindCmdRequest(ParReadWeb->HttpReqPtr, "POST");
					if (i == -1) break;
#ifdef _HTTP_READ_DEBUG_
					printf("%s: HTTP header contains POST request\n". ThrReadWebName);
#endif
					i = FindCmdRequest(ParReadWeb->HttpReqPtr, "Content-Length:");
					pars_read = sscanf(&ParReadWeb->HttpReqPtr[i], "%d", &ContentLength);
					if (!pars_read) break;
					j = FindCmdRequest(&ParReadWeb->HttpReqPtr[i], DoubleNextLine);
					if (j == -1) break;
					ParReadWeb->ShiftBeginContent = i + j;
					ContentLdSize = ParReadWeb->HTTPReqLen - ParReadWeb->ShiftBeginContent;
					ParReadWeb->ContentLen = ContentLength;
#ifdef _HTTP_READ_DEBUG_
					printf("%s: Sock: %d POST request load %d, %d\n", 
							ThrReadWebName, ParReadWeb->HttpSocket, ContentLength, ContentLdSize);
#endif
					if (ContentLength <= ContentLdSize) break;
					ReadBlkBufPtr = GetBuffer(&ReadWebInfoPtr->ReadBlkPool);
					NewKadrData = (RECHTTPDATA*)ReadBlkBufPtr->DataPtr;
					NewKadrData->BlkPoolPtr = ReadBlkBufPtr;
					NewKadrData->WebPoolPtr = NULL;
					NewKadrData->BlockLen = ParReadWeb->HTTPReqLen;
					NewKadrData->Data = (unsigned char*)ParReadWeb->HttpReqPtr;
					AddStructList(&ListHTTPData, NewKadrData);
					ParReadWeb->HttpReqPtr = NULL;
				}
			}
			else
			{
#ifdef _WEB_EPOOL_SOCKET_   
				if (ParReadWeb->isKeepAlive) PoolTimeOut = ParReadWeb->KeepAliveTime;
				else                         PoolTimeOut = TIME_OUT_READ_HTTP;
				nfds = epoll_wait(ReadWebInfoPtr->epollfd, ReadWebInfoPtr->Events, 
					MAX_WEB_SESSION_PER_THREAD, PoolTimeOut*1000);
				if (nfds == -1) 
				{
					DebugLogPrint(NULL, "%s: The epool_wait is failed %d\n", ThrReadWebName, errno);
					HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
                    SocketRemPool(ReadWebInfoPtr, ParReadWeb);
					ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
					break;            
				}

				if (nfds == 0)
				{
					/* Timeout of wait for read is detected */
					DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to timeout: (RX: %d)\r\n",
						ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
					HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
					SocketRemPool(ReadWebInfoPtr, ParReadWeb);
					ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
					break;
				}
				for (i = 0; i < nfds;i++)
				{
					if (ReadWebInfoPtr->Events[i].data.fd == ParReadWeb->HttpSocket)
#else           
				memcpy(&work_rset, &master_rset, sizeof(master_rset));    
				if (ParReadWeb->isKeepAlive) select_time.tv_sec = (long)ParReadWeb->KeepAliveTime;
				else                         select_time.tv_sec = TIME_OUT_READ_HTTP;
				select_time.tv_usec = 0;
				Select_result = select(maxfdp+1, &work_rset, NULL, NULL, &select_time);        
				if (Select_result < 0)
				{
					if ((errno == EAGAIN) || (errno == EINTR))
					{
						continue;
					}
					else
					{
						DebugLogPrint(NULL, "%s: The socket select operation data load is failed due to error: %d\r\n",
							 ThrReadWebName, errno);
						break;
					}
				}
				else if (Select_result > 0)
				{
					if (FD_ISSET(ParReadWeb->HttpSocket, &work_rset))
#endif
					{
						rs = recv(ParReadWeb->HttpSocket, (char*)RecieveCMD, TCP_RX_BLOCK_SIZE, 0 );
						if (rs < 0)
						{
							if ((errno == EAGAIN) || (errno == EINTR))
							{
								Sleep(1);
								continue;
							}
							else
							{
								DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to error: %d (RX: %d)\r\n",
									 ThrReadWebName, ParReadWeb->HttpSocket, errno, 
									 ParReadWeb->HTTPReqLen);
								HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
							#ifdef _WEB_EPOOL_SOCKET_
                                SocketRemPool(ReadWebInfoPtr, ParReadWeb);
								IsReadEnd = true;
                            #endif
								ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
								break;
							}
						}

						if ((rs == 0) || (EmptyRead == true)) // Check for no data was read from HTTP socket.
						{
							DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to timeout: %d (RX: %d)\r\n",
								ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
								ParReadWeb->HTTPReqLen);
							HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
					#ifdef _WEB_EPOOL_SOCKET_
                            SocketRemPool(ReadWebInfoPtr, ParReadWeb);
							IsReadEnd = true;
                    #endif
							ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
							break;
						}
						else
						{
							RecieveCMD[rs] = 0;
							if ((rs + ParReadWeb->HTTPReqLen) > MAX_LOAD_HTML_PAGE_SIZE)
							{
								DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due large page size: %d (RX: %d)\r\n",
									ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
									ParReadWeb->HTTPReqLen + rs);
			 					HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
							#ifdef _WEB_EPOOL_SOCKET_
                                SocketRemPool(ReadWebInfoPtr, ParReadWeb);
								IsReadEnd = true;
                            #endif
								ParReadWeb->Status = URP_HTTP_REQ_DATA_LARGE;
								break;
							}
	    #ifdef _HTTP_READ_DEBUG_
							printf("Sock: %d load header block (len=%d)\n", ParReadWeb->HttpSocket, rs);
	    #endif

							ReadBlkBufPtr = GetBuffer(&ReadWebInfoPtr->ReadBlkPool);
							NewKadrData = (RECHTTPDATA*)ReadBlkBufPtr->DataPtr;
							NewKadrData->BlkPoolPtr = ReadBlkBufPtr;
							ReadWebBufPtr = GetBuffer(&ReadWebInfoPtr->ReadWebPool);
							NewKadrData->Data = (unsigned char*)ReadWebBufPtr->DataPtr;
							NewKadrData->WebPoolPtr = ReadWebBufPtr;

							NewKadrData->BlockLen = rs;
							memcpy(NewKadrData->Data, RecieveCMD, rs);
							AddStructList(&ListHTTPData, NewKadrData);
							ParReadWeb->HTTPReqLen += (unsigned int)rs;
							if (!HeaderLoadDone)
							{
								SrcPtr = RecieveCMD;
								CntrPtr = &ContrEnd[0];
								for (i=0;i < rs;i++)
								{
									memcpy(CntrPtr, (CntrPtr+1), 3);
									if (!*SrcPtr)
									{
										ErrReadHTTP = true;
										break;
									}

									*(CntrPtr+3) = *SrcPtr++;
									if (memcmp(ContrEnd, DoubleNextLine, 4) == 0)
									{
										HeaderLoadDone = true;
										break;
									}
								}

								if (ErrReadHTTP)
								{
									DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due unexpected zero char: %d (RX: %d)\r\n",
										ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
										ParReadWeb->HTTPReqLen + rs);
									HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
							#ifdef _WEB_EPOOL_SOCKET_
                                    SocketRemPool(ReadWebInfoPtr, ParReadWeb);
									IsReadEnd = true;
                            #endif
									ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;                            
									break;
								}
							}
							else
							{
								ContentLdSize += rs;
								if (ContentLength <= ContentLdSize)
								{
	#ifdef _HTTP_READ_DEBUG_
									printf("%s: Sock: %d load last content block (len=%d)\n", 
										ThrReadWebName, ParReadWeb->HttpSocket, rs);
	#endif              
									ParReadWeb->HttpReqPtr = (char*)AllocateMemory(ParReadWeb->HTTPReqLen+4);
									ParReadWeb->HTTPReqLen = 0;
									while(ListHTTPData.Count)
									{
										PointTask = (ObjListTask*)ListHTTPData.FistTask;
										NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
										memcpy( &ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen], NewKadrData->Data, NewKadrData->BlockLen );
										ParReadWeb->HTTPReqLen += (unsigned int)NewKadrData->BlockLen;
										if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
										else                         FreeMemory(NewKadrData->Data);
										FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
										RemStructList( &ListHTTPData, PointTask );
									}
									ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen] = 0;
									ParReadWeb->Status = URP_SUCCESS;
							#ifdef _WEB_EPOOL_SOCKET_
                                    SocketRemPool(ReadWebInfoPtr, ParReadWeb);
									IsReadEnd = true;
                            #endif
	#ifdef _HTTP_READ_DEBUG_
									printf("%s: Sock: %d Content load done (len=%d)\n",
										ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
	#endif
									break;
								}
	#ifdef _HTTP_READ_DEBUG_
								else
								{
									printf("%s: Sock: %d load content block (len=%d)\n",
										ThrReadWebName, ParReadWeb->HttpSocket, rs);
								}
	#endif
							}
						}

						if ((HeaderLoadDone = true) && (!HeadrCheck))
						{
							ParReadWeb->HttpReqPtr = (char*)AllocateMemory(ParReadWeb->HTTPReqLen+4);
							ParReadWeb->HTTPReqLen = 0;
							while(ListHTTPData.Count)
							{
								PointTask = (ObjListTask*)ListHTTPData.FistTask;
								NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
								memcpy( &ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen], NewKadrData->Data, NewKadrData->BlockLen );
								ParReadWeb->HTTPReqLen += (unsigned int)NewKadrData->BlockLen;
								if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
								else                         FreeMemory(NewKadrData->Data);
								FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
								RemStructList( &ListHTTPData, PointTask );
							}
							ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen] = 0;
							HeadrCheck = true;
							ParReadWeb->Status = URP_SUCCESS;
	#ifdef _HTTP_READ_DEBUG_
							printf("%s: Sock: %d HTTP header read done (len=%d)\n",
								ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
	#endif
							i = FindCmdRequestLine(ParReadWeb->HttpReqPtr, "POST");
							if (i == -1)
							{
						#ifdef _WEB_EPOOL_SOCKET_
								SocketRemPool(ReadWebInfoPtr, ParReadWeb);
					            IsReadEnd = true;
                        #endif
								break;
							}
	#ifdef _HTTP_READ_DEBUG_
							printf("%s: HTTP header contains POST request\n". ThrReadWebName);
	#endif
							i = FindCmdRequest(ParReadWeb->HttpReqPtr, "Content-Length:");
							pars_read = sscanf(&ParReadWeb->HttpReqPtr[i], "%d", &ContentLength);
							if (!pars_read)
							{
						#ifdef _WEB_EPOOL_SOCKET_
								SocketRemPool(ReadWebInfoPtr, ParReadWeb);
								IsReadEnd = true;
                        #endif
								break;
							}
							j = FindCmdRequest(&ParReadWeb->HttpReqPtr[i], DoubleNextLine);
							if (j == -1)
							{
						#ifdef _WEB_EPOOL_SOCKET_
								SocketRemPool(ReadWebInfoPtr, ParReadWeb);
								IsReadEnd = true;
                        #endif
								break;
							}
							ParReadWeb->ShiftBeginContent = i + j;
							ContentLdSize = ParReadWeb->HTTPReqLen - ParReadWeb->ShiftBeginContent;
							ParReadWeb->ContentLen = ContentLength;
	#ifdef _HTTP_READ_DEBUG_
							printf("%s: Sock: %d POST request load %d, %d\n", 
								 ThrReadWebName, ParReadWeb->HttpSocket, ContentLength, ContentLdSize);
	#endif
							if (ContentLength <= ContentLdSize)
							{
						#ifdef _WEB_EPOOL_SOCKET_
								SocketRemPool(ReadWebInfoPtr, ParReadWeb);
								IsReadEnd = true;
                        #endif
								break;
							}
							ReadBlkBufPtr = GetBuffer(&ReadWebInfoPtr->ReadBlkPool);
							NewKadrData = (RECHTTPDATA*)ReadBlkBufPtr->DataPtr;
							NewKadrData->BlkPoolPtr = ReadBlkBufPtr;
							NewKadrData->WebPoolPtr = NULL;
							NewKadrData->BlockLen = ParReadWeb->HTTPReqLen;
							NewKadrData->Data = (unsigned char*)ParReadWeb->HttpReqPtr;
							AddStructList(&ListHTTPData, NewKadrData);
							ParReadWeb->HttpReqPtr = NULL;
						}
					}
				}
#ifdef _WEB_EPOOL_SOCKET_
				if (IsReadEnd) break;
#else
				else
				{
					/* Timeout of wait for read is detected*/
					DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to timeout: (RX: %d)\r\n",
						ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
					HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
					ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
					break;
				}
#endif
			}
		}
#ifdef _READER_PERF_MEASURE_
	    clock_gettime(CLOCK_REALTIME, &spec);
        EndTime = (unsigned long long int)spec.tv_sec * 1000000;
        EndTime += (unsigned long long int)(spec.tv_nsec / 1000);
        DeltaTime = (unsigned int)(EndTime - StartTime);
        printf("DT Load: %u\n", (unsigned int)DeltaTime);
        StartTime = (unsigned long long int)spec.tv_sec * 1000000;
        StartTime += (unsigned long long int)(spec.tv_nsec / 1000);
#endif
		if ((ParReadWeb->HttpReqPtr) && (ParReadWeb->Status == URP_SUCCESS)) 
		    HandleHttpReq(ReadWebInfoPtr, ParReadWeb, ParReadWeb->HttpReqPtr);

#ifdef _READER_PERF_MEASURE_
	    clock_gettime(CLOCK_REALTIME, &spec);
        EndTime = (unsigned long long int)spec.tv_sec * 1000000;
        EndTime += (unsigned long long int)(spec.tv_nsec / 1000);
        DeltaTime = (unsigned int)(EndTime - StartTime);
        printf("DT parser: %u\n", (unsigned int)DeltaTime);
#endif
	    if (ParReadWeb->Status != URP_SUCCESS)
		{
	         if (ParReadWeb->BoundInd)      FreeMemory(ParReadWeb->BoundInd);
		     if (ParReadWeb->PicFileBufPtr) FreeMemory(ParReadWeb->PicFileBufPtr);
             if (ParReadWeb->XmlFileBufPtr) FreeMemory(ParReadWeb->XmlFileBufPtr);
		     if (ParReadWeb->StrCmdHTTP)    FreeMemory(ParReadWeb->StrCmdHTTP);		
		     ParReadWeb->BoundInd = NULL;
		     ParReadWeb->PicFileBufPtr = NULL;
             ParReadWeb->XmlFileBufPtr = NULL;
		     ParReadWeb->StrCmdHTTP = NULL;		
		     if (ParReadWeb->Status != URP_NOT_SUPPORT_METHOD)
			 {
			    if (ParReadWeb->HttpReqPtr)    FreeMemory(ParReadWeb->HttpReqPtr);
			    ParReadWeb->HttpReqPtr = NULL;
			    ParReadWeb->HTTPReqLen = 0;
			 }
		}
		else
		{
			ValidHttpReqPostProcess(ParReadWeb);
		}
    #ifdef _SERVDEBUG_
	    DebugLogPrint(NULL, "%s: Read HTTP request is done (%d)\r\n", 
		    ThrReadWebName, ParReadWeb->HttpSocket);
    #endif
	    WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
        HttpPageLoadListClean(ReadWebInfoPtr, &ListHTTPData);
    }

    SendReaderThrCloseNotify(ReadWebInfoPtr);
	FreeMemory(RecieveCMD);
	CloseThreadTimer(ReadWebInfoPtr->NotifyPort, WEB_READ_HB_CHECK_TMR_ID);
	UserAgentMsgParserDestroy(&ReadWebInfoPtr->UserAgentParserMessage);
	HttpCmdMsgParserDestroy(&ReadWebInfoPtr->HttpCmdParserMessage);
	ReadWebInfoPtr->isThreadReady = false;
	WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
#ifdef _WEB_EPOOL_SOCKET_     
    close(ReadWebInfoPtr->epollfd);
#endif
	pthread_exit((void *)0);
}
#endif
//---------------------------------------------------------------------------
static void SendReaderThrCloseNotify(READER_WEB_INFO *ReadWebInfoPtr)
{
#ifdef WIN32
	DebugLogPrint(NULL, "Reader worker thread stopped (SR: %d)\n", ReadWebInfoPtr->isThreadStopReq);
#else
	DebugLogPrint(NULL, "Reader worker thread stopped (NP: %d, WSP: %d, SR: %d)\n", 
        ReadWebInfoPtr->NotifyPort, ReadWebInfoPtr->WebServPort, ReadWebInfoPtr->isThreadStopReq);
#endif        
	DestroyPool(&ReadWebInfoPtr->ReadWebPool);
	DestroyPool(&ReadWebInfoPtr->ReadBlkPool);        
}
//---------------------------------------------------------------------------
static void HttpPageLoadListClean(READER_WEB_INFO *ReadWebInfoPtr, ListItsTask *ListPtr)
{
	RECHTTPDATA		*NewKadrData;
    ObjListTask		*PointTask;

	PointTask = (ObjListTask*)GetFistObjectList(ListPtr);
	while(PointTask)
	{
		NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
	    if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
		else                         FreeMemory(NewKadrData->Data);
        FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
		RemStructList(ListPtr, PointTask);
		PointTask = (ObjListTask*)GetFistObjectList(ListPtr);
	}
}
//---------------------------------------------------------------------------
#ifdef _WEB_EPOOL_SOCKET_
static void SocketRemPool(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb)
{
    ReadWebInfoPtr->Ev.events = EPOLLIN;    
    ReadWebInfoPtr->Ev.data.fd = ParReadWeb->HttpSocket;
    if (epoll_ctl(ReadWebInfoPtr->epollfd, EPOLL_CTL_DEL, 
        ParReadWeb->HttpSocket, &ReadWebInfoPtr->Ev) == -1)
    {
        DebugLogPrint(NULL, "%s: Failed epool_ctl for delete WEB channel socket (Error: %d, Socket: %d)\n",
		    ThrReadWebName, errno, ParReadWeb->HttpSocket);                        
    }
}
#endif
//---------------------------------------------------------------------------
static void HandleHttpReq(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb, char *BufAnsw)
{
	int				i,j,k,l,m, pt1, pt2;
	char            *StrCnvPtr;
	char            *LnChkPtr = NULL;
	unsigned char   charVal1, charVal2;
	bool            isHtmlPageTx = false;
	unsigned int    HeaderLen = 0;
	HTTP_MSG_EXTRACT_DATA *HttpDataPtr;
	char            StrBuf[128];

	ParReadWeb->BoundInd = NULL;
	ParReadWeb->PicFileBufPtr = NULL;
	ParReadWeb->PicFileLen = 0;
    ParReadWeb->XmlFileBufPtr = NULL;
    ParReadWeb->XmlFileLen = 0;
	ParReadWeb->FileType = 0;
	ParReadWeb->NoPwdLocalNameShift = 0;
    ParReadWeb->isEncodingAccept = false;
	ParReadWeb->StrCmdHTTP = NULL;
	ParReadWeb->Status = URP_SUCCESS;
	ParReadWeb->BotType = BOT_NONE;
	ParReadWeb->DeviceType = SDT_DESCTOP;
    ParReadWeb->BrowserType = UBT_GENERAL;
	ParReadWeb->MobileType = NULL;
	*ParReadWeb->LocalFileName = 0;

	HttpDataPtr = &ReadWebInfoPtr->HttpParserData;
	memset(HttpDataPtr, 0, sizeof(HTTP_MSG_EXTRACT_DATA));
	HttpDataPtr->BrowserType = UBT_GENERAL;
	HttpDataPtr->DeviceType = SDT_DESCTOP;
	HttpDataPtr->ReaderInfoPtr = ReadWebInfoPtr;
	HttpDataPtr->ParReadWebPtr = ParReadWeb;
	HttpDataPtr->Status = URP_NOT_SUPPORT_METHOD;
	HttpDataPtr->LocalFileName = ParReadWeb->LocalFileName;

	if (!MpttParserMsgExtract(&ReadWebInfoPtr->HttpCmdParserMessage, BufAnsw) &&
		!HttpDataPtr->isParseDone)
	{
		ParReadWeb->Status = HttpDataPtr->Status;
		return;
	}

	if (!HttpDataPtr->ReqestType)
	{
		ParReadWeb->Status = URP_NOT_SUPPORT_METHOD;
		return;
	}

	ParReadWeb->isEncodingAccept = HttpDataPtr->isEncodingAccept;
	ParReadWeb->isKeepAlive =HttpDataPtr->isKeepAlive;
	ParReadWeb->ReqestType = HttpDataPtr->ReqestType;
	ParReadWeb->MozilaMainVer = HttpDataPtr->MozilaMainVer;
    ParReadWeb->MozilaSubVer = HttpDataPtr->MozilaSubVer;
	ParReadWeb->CookieSessionId = HttpDataPtr->CookieSessionId;
	ParReadWeb->DeviceType = HttpDataPtr->DeviceType;
	ParReadWeb->BrowserType = HttpDataPtr->BrowserType;
	ParReadWeb->BotType = HttpDataPtr->BotType;
	ParReadWeb->MobileType = HttpDataPtr->MobileType;
    ParReadWeb->StrCmdHTTP = HttpDataPtr->StrCmdHTTP;
	ParReadWeb->FileType = HttpDataPtr->FileType;
	ParReadWeb->NoPwdLocalNameShift = HttpDataPtr->NoPwdLocalNameShift;

	if ((ParReadWeb->ReqestType == HRT_GET) || (ParReadWeb->ReqestType == HTR_HEAD))
	{
		if (FindCmdRequest(ParReadWeb->StrCmdHTTP, ".well-known/acme-challenge") != -1)
		{
			ParReadWeb->FileType = FRT_TXT_DATA;
			ParReadWeb->BotType = BOT_GOOGLE;
		}

		if ((ParReadWeb->FileType > 0) &&
			(ParReadWeb->CookieSessionId > 0) && 
			(ParReadWeb->FileType == FRT_HTML_PAGE))
		{
			if (!HttpDataPtr->CmdSesKeyPtr)
			{
				/* Session ID is absent in command line */
				sprintf(StrBuf, "&%s=", KeyFormSessionId);
				strcat(ParReadWeb->StrCmdHTTP, StrBuf);
				strncat(ParReadWeb->StrCmdHTTP, HttpDataPtr->CookieSesKeyPtr, SESSION_ID_KEY_LEN);
				strcat(ParReadWeb->StrCmdHTTP, "&");
			}
			else
			{
				/* Session Id is present and shoud be replaced with key from cookie */
				memcpy(HttpDataPtr->CmdSesKeyPtr, HttpDataPtr->CookieSesKeyPtr, SESSION_ID_KEY_LEN);
			}
		}
	}
	else
	{
		l = FindCmdRequest(BufAnsw, "Content-Type: multipart/form-data");
		if (l != -1)
		{
			k = FindCmdRequest(&BufAnsw[l], "boundary=");
			if (k != -1)
			{
				j = FindCmdRequest( &BufAnsw[l+k], "\r\n");
				if (j != -1)
				{
					m = j;
					ParReadWeb->BoundInd = (char*)AllocateMemory((m+1)*sizeof(char));
					memcpy(ParReadWeb->BoundInd, &BufAnsw[l+k], m);
					ParReadWeb->BoundInd[m] = 0;
					i = l+k+j;
					j = FindCmdRequest(&BufAnsw[i], "upload_item_picture");
					if (j != -1)
					{
						k = -1;
						pt1 = FindCmdRequest( &BufAnsw[i+j], "Content-Type: image/");
						if (pt1 != -1)
						{
							pt2 = FindCmdRequest( &BufAnsw[i+j+pt1], "pjpeg");
                            if (pt2 != -1)
							{
								k = pt1 + pt2;
							}
							else
							{
                                pt2 = FindCmdRequest( &BufAnsw[i+j+pt1], "jpeg");
								if (pt2 != -1)
								{
									k = pt1 + pt2;
								}
							}
						}

						if (k != -1)
						{
							//Picture file in JPEG format request upload is received.
							m = FindCmdRequest( &BufAnsw[i+j+k], DoubleNextLine);
							if (m != -1)
							{
								m = i + j + k + m;
								for(j=0;j < (int)strlen(ParReadWeb->BoundInd);j++)
								{
									if (ParReadWeb->BoundInd[j] != '-')
									{
										break;
									}
								}
								i = strlen(ParReadWeb->BoundInd) - j;
								k = ParReadWeb->HTTPReqLen - i;
								for(;k > 0;k--)
								{
									if (memcmp(&BufAnsw[k], &ParReadWeb->BoundInd[j], i) == 0)
									{
										// Detected end of picture file.
										for (i=k-1;i > 0;i--)
										{
											if ((BufAnsw[i] != '-') && (BufAnsw[i] != '\r') && (BufAnsw[i] != '\n'))
											{
												break;
											}
										}
										ParReadWeb->PicFileLen = i - m + 1;
										ParReadWeb->PicFileBufPtr = (unsigned char*)AllocateMemory(ParReadWeb->PicFileLen*sizeof(unsigned char));
										memcpy(ParReadWeb->PicFileBufPtr, &BufAnsw[m], ParReadWeb->PicFileLen);

										l = FindCmdRequest(BufAnsw, DoubleNextLine);
			                            if (l != -1)
			                            {
				                            m = strlen(&BufAnsw[l]);
	                                        ParReadWeb->StrCmdHTTP = (char*)AllocateMemory(l+4);
											ParReadWeb->StrCmdHTTP[0] = 0;
					                        StrCnvPtr = &BufAnsw[l];
											k = 0;
											for(;;)
											{
												l = FindCmdRequest(&StrCnvPtr[k], "form-data; name=\"");
												if (l == -1) break;
												k += l;
												j = k;
												for(;j < m;j++)
												{
													if (StrCnvPtr[j] == '"')
													{
														StrCnvPtr[j] = 0;
														break;
													}
												}
												if (j >= m) break;
												strcat(ParReadWeb->StrCmdHTTP, &StrCnvPtr[k]);
												strcat(ParReadWeb->StrCmdHTTP, "=");
												k = j + 1;
												if (memcmp(&StrCnvPtr[k], DoubleNextLine, 4) != 0) 
												{
													l = FindCmdRequest(&StrCnvPtr[k], "filename=\"");
													if (l == -1) break;
													k += l;
													for(j=k;j < m;j++)
													{
														if (StrCnvPtr[j] == '"')
														{
															StrCnvPtr[j] = 0;
															break;
														}
													}
												}
												else
												{
													k += 4;
													j = FindCmdRequest(&StrCnvPtr[k], "\r\n");
													if (j == -1) break;
													StrCnvPtr[k+j-2] = 0;
												}
												strcat(ParReadWeb->StrCmdHTTP, &StrCnvPtr[k]);
												strcat(ParReadWeb->StrCmdHTTP, "&");
												k += j;
											}
										}
										else
										{
											ParReadWeb->Status = URP_BAD_2_REQ;
										}
										break;
									}
								}
								if (!ParReadWeb->PicFileLen)
								{
									ParReadWeb->Status = URP_BAD_3_REQ;
								}                                                                                                                                                                                                        
							}
							else
							{
								ParReadWeb->Status = URP_BAD_4_REQ;
							}
						}
						else
						{
							ParReadWeb->Status = URP_NOT_SUPP_CONT_TYPE;
						}
					}
                    else
                    {
						j = FindCmdRequest( &BufAnsw[i], "upload_xml_action");
						if (j != -1)
						{
							k = -1;
							pt1 = FindCmdRequest(&BufAnsw[i+j], "Content-Type: text/");
							if (pt1 != -1)
							{
								pt2 = FindCmdRequest( &BufAnsw[i+j+pt1], "xml");
                                if (pt2 != -1)
								{
									k = pt1 + pt2;
								}
							}
							if (k != -1)
							{                                    
								//Test file in XML format request upload is received.
								m = FindCmdRequest( &BufAnsw[i+j+k], DoubleNextLine);
								if (m != -1)
								{
									m = i + j + k + m;
									for(j=0;j < (int)strlen(ParReadWeb->BoundInd);j++)
									{
										if (ParReadWeb->BoundInd[j] != '-')
										{
											break;
										}
									}
									i = strlen(ParReadWeb->BoundInd) - j;
									k = ParReadWeb->HTTPReqLen - i;
									for(;k > 0;k--)
									{
										if (memcmp(&BufAnsw[k], &ParReadWeb->BoundInd[j], i) == 0)
										{
											// Detected end of picture file.
											for (i=k-1;i > 0;i--)
											{
												if ((BufAnsw[i] != '-') && (BufAnsw[i] != '\r') && (BufAnsw[i] != '\n'))
												{
													break;
												}
											}
											ParReadWeb->XmlFileLen = i - m + 1;
											ParReadWeb->XmlFileBufPtr = (unsigned char*)AllocateMemory(ParReadWeb->XmlFileLen*sizeof(unsigned char));
											memcpy(ParReadWeb->XmlFileBufPtr, &BufAnsw[m], ParReadWeb->XmlFileLen);
											l = FindCmdRequest(BufAnsw, DoubleNextLine);
			                                if (l != -1)
			                                {
				                                m = strlen(&BufAnsw[l]);
	                                            ParReadWeb->StrCmdHTTP = (char*)AllocateMemory(l+4);
												ParReadWeb->StrCmdHTTP[0] = 0;
					                            StrCnvPtr = &BufAnsw[l];
												k = 0;
												for(;;)
												{
													l = FindCmdRequest(&StrCnvPtr[k], "form-data; name=\"");
													if (l == -1) break;
													k += l;
													j = k;
													for(;j < m;j++)
													{
														if (StrCnvPtr[j] == '"')
														{
															StrCnvPtr[j] = 0;
															break;
														}
													}
													if (j >= m) break;
													strcat(ParReadWeb->StrCmdHTTP, &StrCnvPtr[k]);
													strcat(ParReadWeb->StrCmdHTTP, "=");
													k = j + 1;
													if (memcmp(&StrCnvPtr[k], DoubleNextLine, 4) != 0) 
													{
														l = FindCmdRequest(&StrCnvPtr[k], "filename=\"");
														if (l == -1) break;
														k += l;
														for(j=k;j < m;j++)
														{
															if (StrCnvPtr[j] == '"')
															{
																StrCnvPtr[j] = 0;
																break;
															}
														}
													}
													else
													{
														k += 4;
														j = FindCmdRequest(&StrCnvPtr[k], "\r\n");
														if (j == -1) break;
														StrCnvPtr[k+j-2] = 0;
													}
													strcat(ParReadWeb->StrCmdHTTP, &StrCnvPtr[k]);
													strcat(ParReadWeb->StrCmdHTTP, "&");
													k += j;
												}
											}
											else
											{
												ParReadWeb->Status = URP_BAD_2_REQ;
											}
											break;
										}
									}
									if (!ParReadWeb->XmlFileLen)
									{
										ParReadWeb->Status = URP_BAD_3_REQ;
									}                                                                                                                                                                                                        
								}
								else
								{
									ParReadWeb->Status = URP_BAD_4_REQ;
								}
							}
							else
							{
								ParReadWeb->Status = URP_NOT_SUPP_CONT_TYPE;
							}
						}                            
                    }
				}
			}
		}
		if (!ParReadWeb->StrCmdHTTP)
		{
			l = FindCmdRequest(BufAnsw, DoubleNextLine);
			if (l != -1)
			{
				m = strlen(&BufAnsw[l]);
				ParReadWeb->StrCmdHTTP = (char*)AllocateMemory((2*m)+1024);
				StrCnvPtr = &BufAnsw[l];
				for(j=0,k=0;k < m;k++)
				{
					if (*StrCnvPtr == '%')
					{
						if ((m-k) < 2)
						{
							ParReadWeb->Status = URP_BAD_5_REQ;
							break;
						}
						StrCnvPtr++;
						k++;
						charVal1 = AsciiCharToHexOctet[*StrCnvPtr];
						if (charVal1 & 0x80)
						{
							ParReadWeb->Status = URP_BAD_6_REQ;
							break;
						}
						StrCnvPtr++;
						k++;
						charVal2 = AsciiCharToHexOctet[*StrCnvPtr];
						if (charVal2 & 0x80)
						{
							ParReadWeb->Status = URP_BAD_7_REQ;
							break;
						}
						ParReadWeb->StrCmdHTTP[j++] = (charVal1 << 4) + charVal2;
					}
					else
					{
						ParReadWeb->StrCmdHTTP[j++] = *StrCnvPtr;
					}
					StrCnvPtr++;
				}

				if (ParReadWeb->Status == URP_SUCCESS)
				{
					ParReadWeb->StrCmdHTTP[j] = 0;
					if ((FindCmdRequest(ParReadWeb->StrCmdHTTP, KeySessionId) == -1) &&
						(ParReadWeb->CookieSessionId > 0))
					{
						/* Session ID is absent in command line */
						sprintf(StrBuf, "&%s=", KeyFormSessionId);
						strcat(ParReadWeb->StrCmdHTTP, StrBuf);
						strncat(ParReadWeb->StrCmdHTTP, HttpDataPtr->CookieSesKeyPtr, SESSION_ID_KEY_LEN);
						strcat(ParReadWeb->StrCmdHTTP, "&");
					}
				}
			}
			else
			{
				ParReadWeb->Status = URP_BAD_8_REQ;
			}
		}
	}
}
//------------------------------------------------------
void StopReaderThread(READER_WEB_INFO *ReadWebInfo)
{
	ReadWebChannel  *ChannelPtr = NULL;
    ReadWebMessage  *ReadWebMsgPtr = NULL;
    ObjPoolListTask *PointTask = NULL;
	READWEBSOCK     *ParReadWeb = NULL;

	ChannelPtr = &ReadWebInfo->ReadChannel;
	ReadWebInfo->isThreadStopReq = true;
    ReadWebMsgPtr = GetReadWebMsgPool(ChannelPtr, WRC_CLOSE_CHANNEL);
    if (!ReadWebMsgPtr) return;
	ReadWebMsgQueuePost(&ChannelPtr->ReadWebQueue, ReadWebMsgPtr);
	Sleep(50);

#ifdef WIN32
	WaitCloseProcess(ReadWebInfo->HTRREADDATAWEB);
	CloseHandle(ReadWebInfo->HTRREADDATAWEB);
#else
    pthread_join(ReadWebInfo->ReadWebReq_thr, NULL);
	pthread_attr_destroy(&ReadWebInfo->ThrAttr);
#endif
    while (ChannelPtr->ListReadWebMsg.Count)
    {
        PointTask = (ObjPoolListTask*)ChannelPtr->ListReadWebMsg.FistTask;
        ReadWebMsgPtr = (ReadWebMessage*)PointTask->UsedTask;
		if (ReadWebMsgPtr->MsgTag == WRC_WEB_READ_REQ)
		{
		    ParReadWeb = (READWEBSOCK*)ReadWebMsgPtr->DataPtr;
			ReaderInstClear(ParReadWeb);
			if (ParReadWeb->HttpSocket > 0)
			{
		        CloseHttpSocket(ParReadWeb->HttpSocket);
			    ParReadWeb->HttpSocket = -1;
			}
		}
        RemPoolStructList(&ChannelPtr->ListReadWebMsg, PointTask);
    }

#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->ReadWebMsgAccess)) 
        printf("ReadWebThreadClose - Fail to release memory mutex\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->ReadWebMsgAccess);
#endif
    DestroyPoolListStructs(&ChannelPtr->ListReadWebMsg);
    DestroyPool(&ChannelPtr->ReadWebMsgPool);
    ReadWebMsgQueueDestroy(&ChannelPtr->ReadWebQueue);
#ifdef WIN32
	CloseHandle(ChannelPtr->ReadWebMsgAccess);
#else
    pthread_mutex_destroy(&ChannelPtr->ReadWebMsgAccess);
#endif
#ifdef WIN32
	DebugLogPrint(NULL, "%s: ReadWeb (%u) close is completed\n",
		ThrReadWebName, ReadWebInfo->MsgNotifyId);
#else
	DebugLogPrint(NULL, "%s: ReadWeb (%u) close is completed\n",
		ThrReadWebName, ReadWebInfo->NotifyPort);
#endif
}
//------------------------------------------------------------------
static void ReadWebMsgQueueCreate(ReadWebMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Initializing WEB reader channel messages queue.\n", ThrReadWebName);
#ifdef WIN32
	MsgQueuePtr->mutex = CreateMutex(NULL, FALSE, NULL);
    if (MsgQueuePtr->mutex == NULL) 
    {
        printf("Create TLS worker msg queue mutex error: %d\r\n", GetLastError());
	    exit(EXIT_FAILURE);
    }
#else
    pthread_mutex_init(&(MsgQueuePtr->mutex), NULL);
    pthread_cond_init(&(MsgQueuePtr->cond_var), NULL);
#endif
    PoolListInit(&MsgQueuePtr->queue_list, INIT_WEB_READER_MSG_COUNT);
    MsgQueuePtr->destroy = false;
}
//---------------------------------------------------------------------------
static void ReadWebMsgQueueMarkDestroy(ReadWebMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Marking WEB reader message queue for destroy.\n", ThrReadWebName);
    MsgQueuePtr->destroy = true;
    ReadWebMsgQueuePost(MsgQueuePtr, NULL);
}
//---------------------------------------------------------------------------
static void ReadWebMsgQueuePost(ReadWebMsgQueue *MsgQueuePtr, void *p_message)
{
#ifdef WIN32
    unsigned int CurrUsage;

    if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	{
		printf("Fail to get mutex (ReadWebMsgQueuePost)\r\n");
        return;
	}
	CurrUsage = (unsigned int)MsgQueuePtr->queue_list.Count;
	AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (ReadWebMsgQueuePost)\r\n");
	if (!CurrUsage) WinThreadMsgSend(MsgQueuePtr->ThrReadWeb, TWR_MSG_NOTIFY, 0, 0);
#else
    pthread_mutex_lock(&(MsgQueuePtr->mutex));
    AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
    pthread_cond_signal(&(MsgQueuePtr->cond_var));
#endif
}
//---------------------------------------------------------------------------
static void ReadWebMsgQueueDestroy(ReadWebMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: De-initializing WEB reader messages queue.\n", ThrReadWebName);
#ifdef WIN32
	CloseHandle(MsgQueuePtr->mutex);
#else
    pthread_mutex_destroy(&(MsgQueuePtr->mutex));
    pthread_cond_destroy(&(MsgQueuePtr->cond_var));
#endif
    DestroyPoolListStructs(&MsgQueuePtr->queue_list);
}
//---------------------------------------------------------------------------
static bool GetReadWebMessageQueue(ReadWebMsgQueue *MsgQueuePtr, void **pp_message)
{
    ObjPoolListTask *ObjPtr = NULL;
    bool status = true;
#ifdef WIN32
    struct tagMSG SysMsg;
#endif

    *pp_message = NULL;
#ifdef WIN32
    if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	{
		printf("Fail to get mutex (1) (GetReadWebMessageQueue)\r\n");
        return false;
	}
#else
    pthread_mutex_lock(&(MsgQueuePtr->mutex));
#endif
    ObjPtr = (ObjPoolListTask*)GetFistPoolObjectList(&MsgQueuePtr->queue_list);
    if (ObjPtr)
    {
        *pp_message = ObjPtr->UsedTask;
        RemPoolStructList(&MsgQueuePtr->queue_list, ObjPtr);
    }
    else
    {
        for(;;)
        {
#ifdef WIN32
            if (!ReleaseMutex(MsgQueuePtr->mutex)) 
                printf("Fail to release mutex (GetReadWebMessageQueue)\r\n");
			GetMessage(&SysMsg, NULL, TWR_MSG_NOTIFY, TWR_MSG_NOTIFY);
#else
            pthread_cond_wait(&(MsgQueuePtr->cond_var), &(MsgQueuePtr->mutex));
#endif
#ifdef WIN32
            if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	        {
		        printf("Fail to get mutex (2) (GetReadWebMessageQueue)\r\n");
				status = false;
                break;
	        }
#endif
            ObjPtr = (ObjPoolListTask*)GetFistPoolObjectList(&MsgQueuePtr->queue_list);
            if (ObjPtr)
            {
                *pp_message = ObjPtr->UsedTask;
                RemPoolStructList(&MsgQueuePtr->queue_list, ObjPtr);
                break;
            }
        }
    }

    if (MsgQueuePtr->destroy)
    {
		DebugLogPrint(NULL, "%s: Wokeup WEB reader due to destroy flag.\n", ThrReadWebName);
        status = false;
    }
#ifdef WIN32
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (GetReadWebMessageQueue)\r\n");
#else
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
#endif
    return status;
}
//---------------------------------------------------------------------------
static ReadWebMessage* GetReadWebMsgPool(ReadWebChannel *ChannelPtr, unsigned char MsgTag)
{
    ReadWebMessage   *ReadWebMsgPtr = NULL;
    POOL_RECORD_STRUCT *ObjTaskPtr = NULL;

#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->ReadWebMsgAccess, INFINITE) == WAIT_FAILED) return NULL;
#else
    pthread_mutex_lock(&ChannelPtr->ReadWebMsgAccess);
#endif
    ObjTaskPtr = GetBuffer(&ChannelPtr->ReadWebMsgPool);
    if (!ObjTaskPtr)
    {
		DebugLogPrint(NULL, "%s: No buffers for WEB reader message delivery.\n", ThrReadWebName);
#ifdef WIN32
        if (!ReleaseMutex(ChannelPtr->ReadWebMsgAccess)) 
            printf("Fail to release mutex (GetReadWebMsgPool)\r\n");
#else
        pthread_mutex_unlock(&ChannelPtr->ReadWebMsgAccess);
#endif
        return NULL;
    }
    ReadWebMsgPtr = (ReadWebMessage*)ObjTaskPtr->DataPtr;
    ReadWebMsgPtr->MsgTag = MsgTag;
    ReadWebMsgPtr->BlkPoolPtr = ObjTaskPtr;
    ReadWebMsgPtr->ObjPtr = AddPoolStructListObj(&ChannelPtr->ListReadWebMsg, ReadWebMsgPtr);
    if (ChannelPtr->MaxSimultReadWebMsgs < (unsigned int)ChannelPtr->ListReadWebMsg.Count)
        ChannelPtr->MaxSimultReadWebMsgs = (unsigned int)ChannelPtr->ListReadWebMsg.Count;
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->ReadWebMsgAccess)) 
        printf("Fail to release mutex (GetReadWebMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->ReadWebMsgAccess);
#endif
    return ReadWebMsgPtr;
}
//---------------------------------------------------------------------------
static void FreeReadWebMsgPool(ReadWebChannel *ChannelPtr, ReadWebMessage *ReadWebMsgPtr)
{
#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->ReadWebMsgAccess, INFINITE) == WAIT_FAILED)
	{
        return;
	}
#else
    pthread_mutex_lock(&ChannelPtr->ReadWebMsgAccess);
#endif
    FreeBuffer(&ChannelPtr->ReadWebMsgPool, ReadWebMsgPtr->BlkPoolPtr);
    RemPoolStructList(&ChannelPtr->ListReadWebMsg, ReadWebMsgPtr->ObjPtr);
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->ReadWebMsgAccess)) 
        printf("Fail to release mutex (FreeReadWebMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->ReadWebMsgAccess);
#endif
}
//---------------------------------------------------------------------------
#ifdef WIN32
bool ReadWebThreadCreate(READER_WEB_INFO *ReadWebInfoPtr)
{
    ReadWebInfoPtr->HTRREADDATAWEB = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)THRReadWebUser,
               (LPVOID)(ReadWebInfoPtr), 0, (LPDWORD)&ReadWebInfoPtr->ThrReadWebID);
	if (ReadWebInfoPtr->HTRREADDATAWEB) return true;
	else
    {
	    printf("ReadWeb thread create with error!\n");
		return false;
	}
}
#else
bool ReadWebThreadCreate(READER_WEB_INFO *ReadWebInfoPtr)
{
    pthread_attr_t *attrPtr = &ReadWebInfoPtr->ThrAttr;
    struct sched_param	sched;
	bool Result = true;
    size_t StackSize = 1024*1024;
    ReadWebChannel *ChannelPtr = NULL;

	ChannelPtr = &ReadWebInfoPtr->ReadChannel;
	memset(ChannelPtr, 0, sizeof(ReadWebChannel));
    PoolListInit(&ChannelPtr->ListReadWebMsg, INIT_WEB_READER_BLK_COUNT);
    CreatePool(&ChannelPtr->ReadWebMsgPool, INIT_WEB_READER_BLK_COUNT, sizeof(ReadWebMessage));
    pthread_mutex_init(&ChannelPtr->ReadWebMsgAccess, NULL);
    ReadWebMsgQueueCreate(&ChannelPtr->ReadWebQueue);

    pthread_attr_init(attrPtr);
    (void)pthread_attr_setstacksize (attrPtr, StackSize);
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0)
    {
	    sched.sched_priority = 0;
	    pthread_attr_setschedparam(attrPtr, &sched);
    }		
    if (pthread_create(&ReadWebInfoPtr->ReadWebReq_thr, &ReadWebInfoPtr->ThrAttr, 
		&THRReadWebUser, ReadWebInfoPtr) != 0)
    {
	    printf("ReadWeb thread create with %d error!\n", errno);
        DestroyPoolListStructs(&ChannelPtr->ListReadWebMsg);
        DestroyPool(&ChannelPtr->ReadWebMsgPool);
        ReadWebMsgQueueDestroy(&ChannelPtr->ReadWebQueue);
        pthread_mutex_destroy(&ChannelPtr->ReadWebMsgAccess);
		Result = false;
    }
	return Result;
}
#endif
//---------------------------------------------------------------------------
static void OnWebReadHbTimerExp(unsigned int TimerId, void *DataPtr)
{
    ReadWebMessage *ReadWebMsgPtr = NULL;
    ReadWebChannel *ChannelPtr = NULL;

	ChannelPtr = (ReadWebChannel*)DataPtr;
	if (!ChannelPtr) return;
    ReadWebMsgPtr = GetReadWebMsgPool(ChannelPtr, WRC_HB_REQ);
    if (!ReadWebMsgPtr) return;         
	ReadWebMsgQueuePost(&ChannelPtr->ReadWebQueue, ReadWebMsgPtr);
}
//---------------------------------------------------------------------------
void WebDataReadReq(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ReaderPtr)
{
	register ReadWebChannel  *ChannelPtr = NULL;
    register ReadWebMessage  *ReadWebMsgPtr = NULL;

	ChannelPtr = &ReadWebInfoPtr->ReadChannel;
    ReadWebMsgPtr = GetReadWebMsgPool(ChannelPtr, WRC_WEB_READ_REQ);
    if (!ReadWebMsgPtr) return;         
	ReadWebMsgPtr->DataPtr = (void*)ReaderPtr;
	ReadWebMsgQueuePost(&ChannelPtr->ReadWebQueue, ReadWebMsgPtr);
}
//---------------------------------------------------------------------------
unsigned int GetMaxUsageReaderQueue(READER_WEB_INFO *ReadWebInfoPtr)
{
	return ReadWebInfoPtr->ReadChannel.MaxSimultReadWebMsgs;
}
//---------------------------------------------------------------------------
unsigned int GetUsageReaderQueue(READER_WEB_INFO *ReadWebInfoPtr)
{
	return ReadWebInfoPtr->ReadChannel.ListReadWebMsg.Count;
}
//---------------------------------------------------------------------------
int SendHttpPage(READWEBSOCK *ParReadWeb, unsigned char *HtmlPagePtr)
{
	int rs;

	if (!ParReadWeb || !HtmlPagePtr) return -1;
	if (ParReadWeb->SslPtr) rs = SSL_write(ParReadWeb->SslPtr, HtmlPagePtr, strlen((const char*)HtmlPagePtr));
	else                    rs = send(ParReadWeb->HttpSocket, (const char*)HtmlPagePtr, strlen((const char*)HtmlPagePtr), 0 );
	return rs;
}
//---------------------------------------------------------------------------
static void ValidHttpReqPostProcess(READWEBSOCK *ParReadWeb)
{
	int  i;
	char *CwdPtr = NULL;

	ParReadWeb->SessioIdOffset = NULL;
	ParReadWeb->isUserAuthReq = false;
    ParReadWeb->isCapchaFile = false;
    ParReadWeb->isFileRedirect = false;
	if ((ParReadWeb->FileType == FRT_HTML_PAGE) ||
        (ParReadWeb->FileType == FRT_HTR_PAGE))
	{
        i = FindCmdRequest(ParReadWeb->StrCmdHTTP, KeySessionId);
		if (i != -1) ParReadWeb->SessioIdOffset = &ParReadWeb->StrCmdHTTP[i];
		if (FindCmdRequest(ParReadWeb->LocalFileName, GenPageUserAuthReq) != -1)
			ParReadWeb->isUserAuthReq = true;
	}

	if (!ParReadWeb->isUserAuthReq)
	{
#ifdef WIN32
		sprintf(ParReadWeb->RequestInfoStr, "Data request: IP:%d.%d.%d.%d; WC: %d; Rt: %d; Ft: %d; Bt: %d; Brt: %d; Ea: %d; File=%s",
		    (int)(ParReadWeb->HttpClientIP&0x000000ff), 
		    (int)((ParReadWeb->HttpClientIP&0x0000ff00)>>8), 
		    (int)((ParReadWeb->HttpClientIP&0x00ff0000)>>16),
		    (int)((ParReadWeb->HttpClientIP&0xff000000)>>24),
		    ParReadWeb->WebChanId, ParReadWeb->ReqestType, ParReadWeb->FileType, 
		    ParReadWeb->BotType, ParReadWeb->BrowserType, ParReadWeb->isEncodingAccept,
			ParReadWeb->LocalFileName);
#else
		sprintf(ParReadWeb->RequestInfoStr, "Data request: IP:%d.%d.%d.%d; WC: %d; Rt: %d; Ft: %d; Bt: %d; Brt: %d; Ea: %d; File=%s",
    #ifdef _SUN_BUILD_
            (int)((ParReadWeb->HttpClientIP&0xff000000)>>24),
            (int)((ParReadWeb->HttpClientIP&0x00ff0000)>>16),                    
		    (int)((ParReadWeb->HttpClientIP&0x0000ff00)>>8),
            (int)(ParReadWeb->HttpClientIP&0x000000ff),
            ParReadWeb->WebChanId, ParReadWeb->ReqestType, ParReadWeb->FileType,
			ParReadWeb->BotType, ParReadWeb->BrowserType, ParReadWeb->isEncodingAccept,
			ParReadWeb->LocalFileName);
    #else
		    (int)(ParReadWeb->HttpClientIP&0x000000ff), 
		    (int)((ParReadWeb->HttpClientIP&0x0000ff00)>>8), 
		    (int)((ParReadWeb->HttpClientIP&0x00ff0000)>>16),
		    (int)((ParReadWeb->HttpClientIP&0xff000000)>>24),
            ParReadWeb->WebChanId, ParReadWeb->ReqestType, ParReadWeb->FileType,
            ParReadWeb->BotType, ParReadWeb->BrowserType, ParReadWeb->isEncodingAccept,
            ParReadWeb->LocalFileName);    
    #endif
#endif
	}
	else
	{
		*ParReadWeb->RequestInfoStr = 0;
	}
    
    *ParReadWeb->HtmlFileName = 0;
	i = strlen(ParReadWeb->LocalFileName);
	if (i > 0)
	{
		for(;i > 0;i--)
		{
#ifdef WIN32
			if (ParReadWeb->LocalFileName[i-1] == '\\')
#else
			if (ParReadWeb->LocalFileName[i-1] == '/')
#endif
			{
		#ifdef WIN32
				CwdPtr = _getcwd((char*)(&ParReadWeb->HtmlFileName[0]),512);
        #else
	            CwdPtr = getcwd((char*)(&ParReadWeb->HtmlFileName[0]),512);
        #endif
	            strcat(ParReadWeb->HtmlFileName, HtmlDataPath);
                strcat(ParReadWeb->HtmlFileName, &ParReadWeb->LocalFileName[i]);
				break;
			}
		}
	}
    
	if ((ParReadWeb->FileType == FRT_PNG_PIC) &&
		(FindCmdRequest(ParReadWeb->LocalFileName, CapchaFileIdName) != -1)) ParReadWeb->isCapchaFile = true;
    
	if (FindCmdRequest( ParReadWeb->LocalFileName, "..\\") != -1) ParReadWeb->isFileRedirect = true; // File path redirection  prevention
	else if (FindCmdRequest( ParReadWeb->LocalFileName, "../") != -1) ParReadWeb->isFileRedirect = true; 

}
//---------------------------------------------------------------------------
void ReaderInstClear(READWEBSOCK *ParReadWeb)
{
    if (ParReadWeb->BoundInd)      FreeMemory(ParReadWeb->BoundInd);
    if (ParReadWeb->PicFileBufPtr) FreeMemory(ParReadWeb->PicFileBufPtr);
    if (ParReadWeb->XmlFileBufPtr) FreeMemory(ParReadWeb->XmlFileBufPtr);
    if (ParReadWeb->StrCmdHTTP)    FreeMemory(ParReadWeb->StrCmdHTTP);
    if (ParReadWeb->HttpReqPtr)    FreeMemory(ParReadWeb->HttpReqPtr);
	ParReadWeb->HTTPReqLen = 0;
	ParReadWeb->HttpReqPtr = NULL;
	ParReadWeb->StrCmdHTTP = NULL;
	ParReadWeb->BoundInd = NULL;
	ParReadWeb->PicFileBufPtr = NULL;
    ParReadWeb->XmlFileBufPtr = NULL;
}
//---------------------------------------------------------------------------
