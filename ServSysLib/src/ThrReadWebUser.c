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

#include <sys/syscall.h>

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
static void HttpPageLoadListClean(READER_WEB_INFO *ReadWebInfoPtr, PoolListItsTask *ListPtr);
static void SendReaderThrCloseNotify(READER_WEB_INFO *ReadWebInfoPtr);
static void ReadWebMsgQueueCreate(ReadWebMsgQueue *p_msg_queue);
static void ReadWebMsgQueueDestroy(ReadWebMsgQueue *p_msg_queue);
static void ReadWebMsgQueuePost(ReadWebMsgQueue *p_msg_queue, void* p_message);
static bool GetReadWebMessageQueue(ReadWebMsgQueue *p_msg_queue, void **pp_message);
static void ReadWebMsgQueueMarkDestroy(ReadWebMsgQueue *p_msg_queue);
static ReadWebMessage* GetReadWebMsgPool(ReadWebChannel *ChannelPtr, unsigned char MsgTag);
static void FreeReadWebMsgPool(ReadWebChannel *ChannelPtr, ReadWebMessage *ReadWebMsgPtr);
static void OnWebReadHbTimerExp(unsigned int TimerId, void *DataPtr);
static void OnHttpLdTimeoutCheckTimerExp(unsigned int TimerId, void *DataPtr);
static void ValidHttpReqPostProcess(READWEBSOCK *ParReadWeb);
#ifdef _WEB_EPOOL_SOCKET_
static void SocketRemPool(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb);
#endif
static void ReaderSessionInit(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb);
static void HandleInternReaderMsg(ReadWebChannel *ChannelPtr, READER_WEB_INFO *ReadWebInfoPtr);
static void HandleHttpBlockMsg(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb);
static void HttpReqLoadDone(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb);
static void HttpSessionTimeOutCheck(READER_WEB_INFO *ReadWebInfoPtr);
static void RxWorkerMsgNotify(READER_WEB_INFO *ReadWebInfoPtr);
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
	bool			ErrReadHTTP;
	int             TcpErrCode;
	ListItsTask		ListHTTPData;
    unsigned char	*RecieveCMD;
	unsigned int    ContentLength = 0;
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
	HttpCmdMsgParserInit(&ReadWebInfoPtr->HttpCmdParserMessage);
	HttpCmdContentMsgParserInit(&ReadWebInfoPtr->HttpCotentCmdParserMessage);
	UserAgentMsgParserInit(&ReadWebInfoPtr->UserAgentParserMessage);
    HttpFileLineMsgParserInit(&ReadWebInfoPtr->HttpFileLineParserMessage);

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
		ParReadWeb->HeadrCheck = false;
        ParReadWeb->HeaderLoadDone = false;
	    memset(&ContrEnd, 0, 5*sizeof(char));
        ErrReadHTTP = false;
		ContentLength = 0;
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
			    if (!ParReadWeb->HeaderLoadDone)
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
			                ParReadWeb->HeaderLoadDone = true;
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

		    if ((ParReadWeb->HeaderLoadDone = true) && (!ParReadWeb->HeadrCheck))
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
			    ParReadWeb->HeadrCheck = true;
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
	HttpCmdContentMsgParserDestroy(&ReadWebInfoPtr->HttpCotentCmdParserMessage);
	HttpFileLineMsgParserDestroy(&ReadWebInfoPtr->HttpFileLineParserMessage);
	ReadWebInfoPtr->isThreadReady = false;
	WebServerMsgSent(ParReadWeb->IDMessReadWeb, ParReadWeb, 0);    
	WSASetLastError(0);
    ExitThread(0);
}
#else
void* THRReadWebUser(void *arg)
{
	int				rs, i, j;
    READER_WEB_INFO	*ReadWebInfoPtr;
	READWEBSOCK     *ParReadWeb;
	int             TcpErrCode, SslError;
    struct sockaddr_in reader_addr;
    ReadWebChannel  *ChannelPtr = NULL;
    ObjPoolListTask *PointTask = NULL;	
  #ifdef _WEB_EPOOL_SOCKET_
    int             nfds, Status;
	unsigned int    PoolTimeOut;
  #else
    fd_set          master_rset, work_rset;
    struct timeval  select_time;
    int             maxfdp = 0;
    int		        Select_result;
	unsigned int    ActSessionCount;
  #endif

	ReadWebInfoPtr = (READER_WEB_INFO*)arg;
	ChannelPtr = &ReadWebInfoPtr->ReadChannel;
	ReadWebInfoPtr->RecieveCMD = (unsigned char*)AllocateMemory((TCP_RX_BLOCK_SIZE+4)*sizeof(unsigned char));
	if (!ReadWebInfoPtr->RecieveCMD)
	{
	    printf("Fail to memory allocate for RecieveCMD in read thread\n");
		ReadWebInfoPtr->isThreadReady = false;
	    pthread_exit((void *)0);
	}

  #ifdef _WEB_EPOOL_SOCKET_
    ReadWebInfoPtr->epollfd = epoll_create(MAX_WEB_SESSION_PER_THREAD);
    if (ReadWebInfoPtr->epollfd == -1) 
    {
        printf("\n%s Failed to epool create (errno %d)\n",
            SetTimeStampLine(), errno);
		FreeMemory(ReadWebInfoPtr->RecieveCMD);
		ReadWebInfoPtr->isThreadReady = false;
        pthread_exit((void *)0);               
    }
  #endif

	ReadWebInfoPtr->ReaderSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (ReadWebInfoPtr->ReaderSocket == -1)
	{
	    printf("Send response of user's data read done socket open error\n");
        FreeMemory(ReadWebInfoPtr->RecieveCMD);
		ReadWebInfoPtr->isThreadReady = false;
	    pthread_exit((void *)0);
	}

    /* Small size of the socket receive buffer */
    if (updateSocketSize(ReadWebInfoPtr->ReaderSocket, SO_RCVBUF, READER_NOTIFY_SOCKET_BUF_SIZE) != 0)
    {
        printf("Failed to update RX buffer size for read web socket (errno %d)\n", errno);
        close(ReadWebInfoPtr->ReaderSocket);
		FreeMemory(ReadWebInfoPtr->RecieveCMD);
	    ReadWebInfoPtr->isThreadReady = false;
        pthread_exit((void *)0);
    }

    /* Small size of the socket transmit buffer */
    if (updateSocketSize(ReadWebInfoPtr->ReaderSocket, SO_SNDBUF, READER_NOTIFY_SOCKET_BUF_SIZE) != 0)
    {
        printf("Failed to update TX buffer size for read web socket (errno %d)\n", errno);
        close(ReadWebInfoPtr->ReaderSocket);
		FreeMemory(ReadWebInfoPtr->RecieveCMD);
	    ReadWebInfoPtr->isThreadReady = false;
        pthread_exit((void *)0);
    }

    bzero(&reader_addr, sizeof(reader_addr));
    reader_addr.sin_family      = AF_INET;
    reader_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    reader_addr.sin_port        = htons(ReadWebInfoPtr->NotifyPort);

    if (bind(ReadWebInfoPtr->ReaderSocket, (struct sockaddr *) &reader_addr,
             sizeof(reader_addr)) < 0)
    {
        printf("\n%s Failed to bind content reader thread socket (errno %d)\n",
            SetTimeStampLine(), errno);
        close(ReadWebInfoPtr->ReaderSocket);
		FreeMemory(ReadWebInfoPtr->RecieveCMD);
	    ReadWebInfoPtr->isThreadReady = false;        
        pthread_exit((void *)0);
    }

  #ifdef _WEB_EPOOL_SOCKET_
	ReadWebInfoPtr->Ev.events = EPOLLIN;
	ReadWebInfoPtr->Ev.data.fd = ReadWebInfoPtr->ReaderSocket;
	if (epoll_ctl(ReadWebInfoPtr->epollfd, EPOLL_CTL_ADD, 
				ReadWebInfoPtr->ReaderSocket, &ReadWebInfoPtr->Ev) == -1)
	{
        printf("\n%s Failed to add reader notify socket to epool (errno %d)\n",
            SetTimeStampLine(), errno);
        close(ReadWebInfoPtr->ReaderSocket);
		FreeMemory(ReadWebInfoPtr->RecieveCMD);
	    ReadWebInfoPtr->isThreadReady = false;        
        pthread_exit((void *)0);
	}

	ClientSocketHashChanCreate(&ReadWebInfoPtr->SocketHash);
    if (AddClientInfoSocketHash(&ReadWebInfoPtr->SocketHash, ReadWebInfoPtr, ReadWebInfoPtr->ReaderSocket) != SUCCES_ADD_SOCKET)
	{
		DebugLogPrint(NULL, "%s: Failed to add %u WEB socket in reader pool\r\n",
			ThrReadWebName, ParReadWeb->HttpSocket);

        ReadWebInfoPtr->Ev.events = EPOLLIN;    
        ReadWebInfoPtr->Ev.data.fd = ReadWebInfoPtr->ReaderSocket;
        if (epoll_ctl(ReadWebInfoPtr->epollfd, EPOLL_CTL_DEL, 
            ParReadWeb->HttpSocket, &ReadWebInfoPtr->Ev) == -1)
        {
            DebugLogPrint(NULL, "%s: Failed epool_ctl for delete reader channel socket (Error: %d, Socket: %d)\n",
		        ThrReadWebName, errno, ReadWebInfoPtr->ReaderSocket);                        
        }
		
		ClientSocketHashChanClose(&ReadWebInfoPtr->SocketHash);
        close(ReadWebInfoPtr->ReaderSocket);
		FreeMemory(ReadWebInfoPtr->RecieveCMD);
	    ReadWebInfoPtr->isThreadReady = false;        
        pthread_exit((void *)0);
	}	
  #else
    FD_ZERO (&master_rset);
	FD_SET (ReadWebInfoPtr->ReaderSocket, &master_rset);
	maxfdp = ReadWebInfoPtr->ReaderSocket;
  #endif

    CreatePool(&ReadWebInfoPtr->ReadWebPool, START_READ_WEB_BLK, (TCP_RX_BLOCK_SIZE + 4));
    CreatePool(&ReadWebInfoPtr->ReadBlkPool, START_READ_WEB_BLK, sizeof(RECHTTPDATA));
	HttpCmdMsgParserInit(&ReadWebInfoPtr->HttpCmdParserMessage);
	HttpCmdContentMsgParserInit(&ReadWebInfoPtr->HttpCotentCmdParserMessage);
	UserAgentMsgParserInit(&ReadWebInfoPtr->UserAgentParserMessage);
	HttpFileLineMsgParserInit(&ReadWebInfoPtr->HttpFileLineParserMessage);
    PoolListInit(&ReadWebInfoPtr->ActiveHttpSessionList, INIT_WEB_SESSION_BLK_COUNT);
	ReadWebInfoPtr->CurrTimeTick = GetTickCount();

	CreateThreadTimerCBD(OnWebReadHbTimerExp, ReadWebInfoPtr->NotifyPort, 30*TMR_ONE_SEC, WEB_READ_HB_CHECK_TMR_ID, true, ReadWebInfoPtr);
	CreateThreadTimerCBD(OnHttpLdTimeoutCheckTimerExp, ReadWebInfoPtr->NotifyPort, TMR_ONE_SEC, HTTP_LD_TIMEOOT_CHECK_TMR_ID, true, ReadWebInfoPtr);

	DebugLogPrint(NULL, "Reader worker thread startup (NP: %d, WSP: %d)\n", 
        ReadWebInfoPtr->NotifyPort, ReadWebInfoPtr->WebServPort);
	printf("Reader worker thread (%u/%u) startup is completed\n", 
		(unsigned int)syscall(SYS_gettid), ReadWebInfoPtr->NotifyPort);
  #ifdef _WEB_EPOOL_SOCKET_   
	for(;;)
	{
		nfds = epoll_wait(ReadWebInfoPtr->epollfd, ReadWebInfoPtr->Events, 
			MAX_WEB_SESSION_PER_THREAD, 60*1000);
		if (nfds == -1) 
		{
			DebugLogPrint(NULL, "%s: The epool_wait is failed %d\n", ThrReadWebName, errno);
			ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
			break;            
		}
		
		if (nfds == 0)
		{
			DebugLogPrint(NULL, "%s: The epool read timeout is expired (NP: %d)\r\n",
				ThrReadWebName, ReadWebInfoPtr->NotifyPort);
			continue;
		}

		for (i = 0; i < nfds;i++)
		{
			ParReadWeb = (READWEBSOCK*)ClientInfoFindBySocket(&ReadWebInfoPtr->SocketHash, 
				ReadWebInfoPtr->Events[i].data.fd);
			if (ParReadWeb)
			{
				if ((void*)ParReadWeb == (void*)ReadWebInfoPtr)
				{
					/* Internal notify event is received - try to read all messages */
					HandleInternReaderMsg(ChannelPtr, ReadWebInfoPtr);					
					if (ReadWebInfoPtr->isThreadStopReq) break;
				}
				else
				{
				    HandleHttpBlockMsg(ReadWebInfoPtr, ParReadWeb);
				}
			}
            else
			{
			    DebugLogPrint(NULL, "%s: Could not find reader contects for %d socket\r\n",
				    ThrReadWebName, ReadWebInfoPtr->Events[i].data.fd);
			}
		}

		if (ReadWebInfoPtr->isThreadStopReq) break;
	}
  #else
	for(;;)
	{
	    ActSessionCount = ReadWebInfoPtr->ActiveHttpSessionList.Count;
		memcpy(&work_rset, &master_rset, sizeof(master_rset));    

		select_time.tv_sec = 60;
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
			if (FD_ISSET(ReadWebInfoPtr->ReaderSocket, &work_rset))
			{
				/* Internal notify event is received - try to read all messages */
				HandleInternReaderMsg(ChannelPtr, ReadWebInfoPtr);					
				if (ReadWebInfoPtr->isThreadStopReq) break;
			}
			
            /* Check for HTTP data block from active HTTP sessions */				
		    PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&ReadWebInfoPtr->ActiveHttpSessionList);
			while(PointTask)
			{
		        ParReadWeb = (READWEBSOCK*)PointTask->UsedTask;
				if (FD_ISSET(ParReadWeb->HttpSocket, &work_rset))
					HandleHttpBlockMsg(ReadWebInfoPtr, ParReadWeb);
				PointTask = (ObjPoolListTask*)GetNextPoolObjectList(&ReadWebInfoPtr->ActiveHttpSessionList);
			}
		}			
		else
		{
			DebugLogPrint(NULL, "%s: The select read timeout is expired (NP: %d)\r\n",
				ThrReadWebNam, ReadWebInfoPtr->NotifyPorte);
		}

		if (ActSessionCount != ReadWebInfoPtr->ActiveHttpSessionList.Count)
		{
            FD_ZERO (&master_rset);
		    FD_SET (ReadWebInfoPtr->ReaderSocket, &master_rset);
		    maxfdp = ReadWebInfoPtr->ReaderSocket;
		    PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&ReadWebInfoPtr->ActiveHttpSessionList);
			while(PointTask)
			{
		        ParReadWeb = (READWEBSOCK*)PointTask->UsedTask;		
                FD_SET (ParReadWeb->HttpSocket, &master_rset);
			    if (maxfdp < ParReadWeb->HttpSocket) maxfdp = ParReadWeb->HttpSocket;
			}
		}		
    }
  #endif

    SendReaderThrCloseNotify(ReadWebInfoPtr);
	CloseThreadTimer(ReadWebInfoPtr->NotifyPort, HTTP_LD_TIMEOOT_CHECK_TMR_ID);
	CloseThreadTimer(ReadWebInfoPtr->NotifyPort, WEB_READ_HB_CHECK_TMR_ID);
	UserAgentMsgParserDestroy(&ReadWebInfoPtr->UserAgentParserMessage);
	HttpCmdMsgParserDestroy(&ReadWebInfoPtr->HttpCmdParserMessage);
    HttpCmdContentMsgParserDestroy(&ReadWebInfoPtr->HttpCotentCmdParserMessage);
	HttpFileLineMsgParserDestroy(&ReadWebInfoPtr->HttpFileLineParserMessage);
	
  #ifdef _WEB_EPOOL_SOCKET_
    ReadWebInfoPtr->Ev.events = EPOLLIN;    
    ReadWebInfoPtr->Ev.data.fd = ReadWebInfoPtr->ReaderSocket;
    if (epoll_ctl(ReadWebInfoPtr->epollfd, EPOLL_CTL_DEL, 
        ParReadWeb->HttpSocket, &ReadWebInfoPtr->Ev) == -1)
    {
        DebugLogPrint(NULL, "%s: Failed epool_ctl for delete reader channel socket (Error: %d, Socket: %d)\n",
		    ThrReadWebName, errno, ReadWebInfoPtr->ReaderSocket);                        
    }
	
	if (!RemClientInfoSocketHash(&ReadWebInfoPtr->SocketHash, ReadWebInfoPtr->ReaderSocket))
	{
	    DebugLogPrint(NULL, "%s: Fail to remove reader socket form sockets reader hash (%d)\r\n", 
		    ThrReadWebName, ReadWebInfoPtr->ReaderSocket);		
	}
    close(ReadWebInfoPtr->epollfd);
	ClientSocketHashChanClose(&ReadWebInfoPtr->SocketHash);
  #endif
	
	DestroyPoolListStructs(&ReadWebInfoPtr->ActiveHttpSessionList);
	ReadWebInfoPtr->isThreadReady = false;
	WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
	FreeMemory(ReadWebInfoPtr->RecieveCMD);
	close(ReadWebInfoPtr->ReaderSocket);
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
static void HttpPageLoadListClean(READER_WEB_INFO *ReadWebInfoPtr, PoolListItsTask *ListPtr)
{
	RECHTTPDATA		*NewKadrData = NULL;
    ObjPoolListTask	*PointTask = NULL;

	PointTask = (ObjPoolListTask*)GetFistPoolObjectList(ListPtr);
	while(PointTask)
	{
		NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
	    if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
		else                         FreeMemory(NewKadrData->Data);
        FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
		RemPoolStructList(ListPtr, PointTask);
		PointTask = (ObjPoolListTask*)GetFistPoolObjectList(ListPtr);
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
	HTTP_MSG_EXTRACT_DATA HttpParserData;
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
	ParReadWeb->IfModifyedSince = 0;
	*ParReadWeb->LocalFileName = 0;

	HttpDataPtr = &HttpParserData;
	memset(HttpDataPtr, 0, sizeof(HTTP_MSG_EXTRACT_DATA));
	HttpDataPtr->BrowserType = UBT_GENERAL;
	HttpDataPtr->DeviceType = SDT_DESCTOP;
	HttpDataPtr->ReaderInfoPtr = ReadWebInfoPtr;
	HttpDataPtr->ParReadWebPtr = ParReadWeb;
	HttpDataPtr->Status = URP_NOT_SUPPORT_METHOD;
	HttpDataPtr->LocalFileName = ParReadWeb->LocalFileName;

	if (!ParReadWeb->isContentDelivery)
	{
	    if (!MpttParserMsgExtract(&ReadWebInfoPtr->HttpCmdParserMessage, &HttpParserData, BufAnsw) &&
		    !HttpDataPtr->isParseDone)
	    {
		    ParReadWeb->Status = HttpDataPtr->Status;
		    return;
	    }
	}
	else
	{
	    if (!MpttParserMsgExtract(&ReadWebInfoPtr->HttpCotentCmdParserMessage, &HttpParserData, BufAnsw) &&
		    !HttpDataPtr->isParseDone)
	    {
		    ParReadWeb->Status = HttpDataPtr->Status;
		    return;
	    }
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
	ParReadWeb->IfModifyedSince = HttpDataPtr->IfModifyedSince;
	ParReadWeb->NoPwdLocalNameShift = HttpDataPtr->NoPwdLocalNameShift;

	if ((ParReadWeb->ReqestType == HRT_GET) || (ParReadWeb->ReqestType == HTR_HEAD))
	{
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
	RxWorkerMsgNotify(ReadWebInfo);
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
	READER_WEB_INFO *ReadWebInfoPtr = NULL;
    ReadWebMessage *ReadWebMsgPtr = NULL;
    ReadWebChannel *ChannelPtr = NULL;

	ReadWebInfoPtr = (READER_WEB_INFO*)DataPtr;
	if (!ReadWebInfoPtr) return;
	ChannelPtr = &ReadWebInfoPtr->ReadChannel;
	if (!ChannelPtr) return;
    ReadWebMsgPtr = GetReadWebMsgPool(ChannelPtr, WRC_HB_REQ);
    if (!ReadWebMsgPtr) return;         
	ReadWebMsgQueuePost(&ChannelPtr->ReadWebQueue, ReadWebMsgPtr);
	RxWorkerMsgNotify(ReadWebInfoPtr);
}
//---------------------------------------------------------------------------
static void OnHttpLdTimeoutCheckTimerExp(unsigned int TimerId, void *DataPtr)
{
	READER_WEB_INFO *ReadWebInfoPtr = NULL;
    ReadWebMessage *ReadWebMsgPtr = NULL;
    ReadWebChannel *ChannelPtr = NULL;

	ReadWebInfoPtr = (READER_WEB_INFO*)DataPtr;
    if (!ReadWebInfoPtr) return;
	ChannelPtr = &ReadWebInfoPtr->ReadChannel;
	if (!ChannelPtr) return;
    ReadWebMsgPtr = GetReadWebMsgPool(ChannelPtr, WRC_SESS_TO_CHECK);
    if (!ReadWebMsgPtr) return;         
	ReadWebMsgQueuePost(&ChannelPtr->ReadWebQueue, ReadWebMsgPtr);
	RxWorkerMsgNotify(ReadWebInfoPtr);
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
    RxWorkerMsgNotify(ReadWebInfoPtr);
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
static void ReaderSessionInit(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb)
{
	int    rs, SslError;
#ifdef _READER_PERF_MEASURE_
    struct timespec spec;
#endif

#ifdef _READER_PERF_MEASURE_
    clock_gettime(CLOCK_REALTIME, &spec);
    ParReadWeb->StartTime = (unsigned long long int)spec.tv_sec * 1000000;
    ParReadWeb->StartTime += (unsigned long long int)(spec.tv_nsec / 1000);
#endif
	ParReadWeb->HeadrCheck = false;
    ParReadWeb->HeaderLoadDone = false;
	memset(&ParReadWeb->ContrEnd[0], 0, 5*sizeof(char));
	ParReadWeb->DelayCount = 0;

	HttpPageLoadListClean(ReadWebInfoPtr, &ParReadWeb->ListHTTPData);
	
	ParReadWeb->HTTPReqLen = 0;
	ParReadWeb->ContentLen = 0;
	ParReadWeb->ContentLdSize = 0;
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
				ParReadWeb->AcceptBioPtr = NULL;
				ParReadWeb->BioPtr = NULL;
				ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
				WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
				return;
			}
		}
			
		/*
		ParReadWeb->AcceptBioPtr = BIO_new_socket(ParReadWeb->HttpSocket, BIO_CLOSE);
		if (!ParReadWeb->AcceptBioPtr)
		{
			SslError = SSL_get_error(ParReadWeb->SslPtr, rs); 
			SSL_shutdown(ParReadWeb->SslPtr);
			SSL_free(ParReadWeb->SslPtr);
			ParReadWeb->SslPtr = NULL;
			ParReadWeb->BioPtr = NULL;
			DebugLogPrint(NULL, "%s: SSL (%d) fail to set BIO socket due to error: %d\r\n", 
				ThrReadWebName, ParReadWeb->HttpSocket, SslError);
			ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
			WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
			continue;
		}
		SSL_set_bio(ParReadWeb->SslPtr, ParReadWeb->AcceptBioPtr, ParReadWeb->AcceptBioPtr);
		*/
			
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
			return;
		}

		SSL_set_accept_state(ParReadWeb->SslPtr);
		if (SSL_accept(ParReadWeb->SslPtr) == -1)
		{
			/*
			SSL_shutdown(ParReadWeb->SslPtr);
			BIO_free_all(ParReadWeb->AcceptBioPtr); */
				
			SSL_free(ParReadWeb->SslPtr);
				
			ParReadWeb->AcceptBioPtr = NULL;
			ParReadWeb->BioPtr = NULL;
			ParReadWeb->SslPtr = NULL;
			DebugLogPrint(NULL, "%s: SSL (%d) fail to SSL accept\r\n",
				ThrReadWebName, ParReadWeb->HttpSocket);
			ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
			WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
			return;
		}
			
		/*
		ParReadWeb->BioPtr = BIO_pop(ParReadWeb->AcceptBioPtr);
		if (!ParReadWeb->BioPtr)
		{
			SSL_shutdown(ParReadWeb->SslPtr);
			BIO_free_all(ParReadWeb->AcceptBioPtr);				
			ParReadWeb->AcceptBioPtr = NULL;
			ParReadWeb->BioPtr = NULL;
			ParReadWeb->SslPtr = NULL;
			DebugLogPrint(NULL, "%s: SSL (%d) fail to BIO_pop\r\n",
				ThrReadWebName, ParReadWeb->HttpSocket);
			ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
			WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
			continue;
		}
		*/
		ParReadWeb->isSslAccept = true;
	}

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
		return;
	}

    if (AddClientInfoSocketHash(&ReadWebInfoPtr->SocketHash, ParReadWeb, ParReadWeb->HttpSocket) != SUCCES_ADD_SOCKET)
	{
		DebugLogPrint(NULL, "%s: Failed to add %u WEB socket in reader pool\r\n",
			ThrReadWebName, ParReadWeb->HttpSocket);
		ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
		SocketRemPool(ReadWebInfoPtr, ParReadWeb);
		WebServerMsgSent(ParReadWeb->IDMessReadWeb, (void*)ParReadWeb, 0);
		return;
	}
#endif
		
	if (ParReadWeb->isKeepAlive) ParReadWeb->RxTimeOut = (unsigned long)ParReadWeb->KeepAliveTime;
	else                         ParReadWeb->RxTimeOut = TIME_OUT_READ_HTTP;
	ParReadWeb->RxTimeOut *= 1000;
	
    ParReadWeb->LastDataRxTime = ReadWebInfoPtr->CurrTimeTick;
    ParReadWeb->ObjPtr = AddPoolStructListObj(&ReadWebInfoPtr->ActiveHttpSessionList, ParReadWeb);

#ifdef _HTTP_READ_DEBUG_
	DebugLogPrint(NULL, "%s: HTTP headr load start: Sct=%d\r\n",
		ThrReadWebName, ParReadWeb->HttpSocket);
#endif
}
//---------------------------------------------------------------------------
static void HandleInternReaderMsg(ReadWebChannel *ChannelPtr, READER_WEB_INFO *ReadWebInfoPtr)
{
	int             rs;
	ReadWebMessage	*ReadWebMsgPtr = NULL;
	READWEBSOCK		*ParReadWeb = NULL;

	rs = recv(ReadWebInfoPtr->ReaderSocket, (char*)ReadWebInfoPtr->RecieveCMD, TCP_RX_BLOCK_SIZE, 0 );
	if (rs < 0)
	{
		if ((errno == EAGAIN) || (errno == EINTR))
		{
			Sleep(1);
			return;
		}
		else
		{
			DebugLogPrint(NULL, "%s: UDP (%d) Reader RX connection closed due to error: %d\r\n",
				ThrReadWebName, ReadWebInfoPtr->ReaderSocket, errno);
			return;
		}
	}

	while (GetReadWebMessageQueue(&ChannelPtr->ReadWebQueue, (void**)&ReadWebMsgPtr))
	{
		if (!ReadWebMsgPtr) break;

		if (ReadWebMsgPtr->MsgTag == WRC_CLOSE_CHANNEL)
		{
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
			ReadWebInfoPtr->isThreadStopReq = true;
			break;
		}
		else if (ReadWebMsgPtr->MsgTag == WRC_HB_REQ)
		{
			WebServerMsgSent(WSU_HBACTIONTHR, ReadWebInfoPtr, (void*)(tulong)(unsigned int)WHN_READER_IND);
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
		}
		else if (ReadWebMsgPtr->MsgTag == WRC_WEB_READ_REQ)
		{
			ParReadWeb = (READWEBSOCK*)ReadWebMsgPtr->DataPtr;
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
			if (!ParReadWeb) continue;
			/* Add new HTTP request for load and processing */
            ReaderSessionInit(ReadWebInfoPtr, ParReadWeb);
		}
		else if (ReadWebMsgPtr->MsgTag == WRC_SESS_TO_CHECK)
		{
            HttpSessionTimeOutCheck(ReadWebInfoPtr);
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
		}						
		else
		{
			DebugLogPrint(NULL, "%s: Unexpected(0x%02X) message tag is received by WEB reader\n",
				ThrReadWebName, ReadWebMsgPtr->MsgTag);
			FreeReadWebMsgPool(ChannelPtr, ReadWebMsgPtr);
		}
	}
}
//---------------------------------------------------------------------------
static void HandleHttpBlockMsg(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb)
{
	int				rs, i, j, SslError;
    int             TcpErrCode;
	unsigned int    pars_read;
	unsigned int    ContentLength = 0;
	bool			ErrReadHTTP = false;
	POOL_RECORD_STRUCT *ReadWebBufPtr = NULL;
	POOL_RECORD_STRUCT *ReadBlkBufPtr = NULL;
	unsigned char   *SrcPtr = NULL;
	unsigned char   *CntrPtr = NULL;
    ObjPoolListTask	*PointTask = NULL;
	RECHTTPDATA		*NewKadrData = NULL;
	
	if (ParReadWeb->SslPtr)
	{
// printf("Enter SSL_read: Sct: %u\r\n", ParReadWeb->HttpSocket);
		rs = SSL_read(ParReadWeb->SslPtr, ReadWebInfoPtr->RecieveCMD, TCP_RX_BLOCK_SIZE);
		SslError = SSL_get_error(ParReadWeb->SslPtr, rs); 
// printf("Leave SSL_read: rs: %d, err: %d, Sct: %u\r\n", rs, SslError, ParReadWeb->HttpSocket);				
		if (rs < 0)
		{				
			if (SslError == SSL_ERROR_WANT_WRITE)
			{
				return;
			}
			else
			{
				DebugLogPrint(NULL, "%s: SSL (%d) RX connection closed due to error: %d (RX: %d)\r\n",
								ThrReadWebName, ParReadWeb->HttpSocket, SslError, 
								ParReadWeb->HTTPReqLen);
				ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
				HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
				return;
			}
		}
		else if (rs == 0) // Check for no data was read from HTTPS socket.
		{				
			if (SslError == SSL_ERROR_NONE)
			{
				ParReadWeb->DelayCount++;
				if (ParReadWeb->DelayCount > 5) /* Just hard coded */
				{
					DebugLogPrint(NULL, "%s: SSL (%d) RX connection closed due to timeout (RX: %d)\r\n",
						ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
					ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
					HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
				}
			}
			else
			{
				DebugLogPrint(NULL, "%s: SSL (%d) RX connection closed due to error: %d (RX: %d)\r\n",
					ThrReadWebName, ParReadWeb->HttpSocket, SslError, ParReadWeb->HTTPReqLen);
				ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
				HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
			}
			return;
		}
				
		ParReadWeb->DelayCount = 0;
	}
	else
	{
		/* Read data from HTTP socket */
		rs = recv(ParReadWeb->HttpSocket, (char*)ReadWebInfoPtr->RecieveCMD, TCP_RX_BLOCK_SIZE, 0 );
		if (rs < 0)
		{
			if ((errno == EAGAIN) || (errno == EINTR))
			{
				Sleep(1);
				return;
			}
			else
			{
				DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to error: %d (RX: %d)\r\n",
						ThrReadWebName, ParReadWeb->HttpSocket, errno, 
						ParReadWeb->HTTPReqLen);
				ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;
				HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
				return;
			}
		}

		if (rs == 0) // Check for no data was read from HTTP socket.
		{
			DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to timeout: %d (RX: %d)\r\n",
				ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
				ParReadWeb->HTTPReqLen);
			ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
			HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
			return;
		}
	}

	ReadWebInfoPtr->RecieveCMD[rs] = 0;
    ParReadWeb->LastDataRxTime = ReadWebInfoPtr->CurrTimeTick;

	if ((rs + ParReadWeb->HTTPReqLen) > MAX_LOAD_HTML_PAGE_SIZE)
	{
		DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due large page size: %d (RX: %d)\r\n",
			ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
			ParReadWeb->HTTPReqLen + rs);
		ParReadWeb->Status = URP_HTTP_REQ_DATA_LARGE;
		HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
		return;
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
	memcpy(NewKadrData->Data, ReadWebInfoPtr->RecieveCMD, rs);
	AddPoolStructList(&ParReadWeb->ListHTTPData, NewKadrData);
	ParReadWeb->HTTPReqLen += (unsigned int)rs;
	if (!ParReadWeb->HeaderLoadDone)
	{
		SrcPtr = ReadWebInfoPtr->RecieveCMD;
		CntrPtr = (unsigned char*)&ParReadWeb->ContrEnd[0];
		for (i=0;i < rs;i++)
		{
			memcpy(CntrPtr, (CntrPtr+1), 3);
			if (!*SrcPtr)
			{
				ErrReadHTTP = true;
				break;
			}

			*(CntrPtr+3) = *SrcPtr++;
			if (memcmp(ParReadWeb->ContrEnd, DoubleNextLine, 4) == 0)
			{
				ParReadWeb->HeaderLoadDone = true;
				break;
			}
		}

		if (ErrReadHTTP)
		{
			DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due unexpected zero char: %d (RX: %d)\r\n",
				ThrReadWebName, ParReadWeb->HttpSocket, TcpErrCode, 
				ParReadWeb->HTTPReqLen + rs);
			ParReadWeb->Status = URP_CONNECT_CLOSE_ERROER;                            
			HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
			return;
		}
	}
	else
	{
		ParReadWeb->ContentLdSize += rs;
		if (ContentLength <= ParReadWeb->ContentLdSize)
		{
#ifdef _HTTP_READ_DEBUG_
			printf("%s: Sock: %d load last content block (len=%d)\n", 
				ThrReadWebName, ParReadWeb->HttpSocket, rs);
#endif              
			ParReadWeb->HttpReqPtr = (char*)AllocateMemory(ParReadWeb->HTTPReqLen+4);
			ParReadWeb->HTTPReqLen = 0;
			while(ParReadWeb->ListHTTPData.Count)
			{
				PointTask = (ObjPoolListTask*)ParReadWeb->ListHTTPData.FistTask;
				NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
				memcpy( &ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen], NewKadrData->Data, NewKadrData->BlockLen );
				ParReadWeb->HTTPReqLen += (unsigned int)NewKadrData->BlockLen;
				if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
				else                         FreeMemory(NewKadrData->Data);
				FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
				RemPoolStructList(&ParReadWeb->ListHTTPData, PointTask);
			}
			ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen] = 0;
			ParReadWeb->Status = URP_SUCCESS;						
#ifdef _HTTP_READ_DEBUG_
			printf("%s: Sock: %d Content load done (len=%d)\n",
				ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
#endif
            HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
			return;
		}
#ifdef _HTTP_READ_DEBUG_
		else
		{
			printf("%s: Sock: %d load content block (len=%d)\n",
				ThrReadWebName, ParReadWeb->HttpSocket, rs);
		}
#endif
	}

	if ((ParReadWeb->HeaderLoadDone = true) && (!ParReadWeb->HeadrCheck))
	{
		ParReadWeb->HttpReqPtr = (char*)AllocateMemory(ParReadWeb->HTTPReqLen+4);
		ParReadWeb->HTTPReqLen = 0;
		while(ParReadWeb->ListHTTPData.Count)
		{
			PointTask = (ObjPoolListTask*)ParReadWeb->ListHTTPData.FistTask;
			NewKadrData = (RECHTTPDATA*)PointTask->UsedTask;
			memcpy( &ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen], NewKadrData->Data, NewKadrData->BlockLen );
			ParReadWeb->HTTPReqLen += (unsigned int)NewKadrData->BlockLen;
			if (NewKadrData->WebPoolPtr) FreeBuffer(&ReadWebInfoPtr->ReadWebPool, (POOL_RECORD_STRUCT*)NewKadrData->WebPoolPtr);
			else                         FreeMemory(NewKadrData->Data);
			FreeBuffer(&ReadWebInfoPtr->ReadBlkPool, (POOL_RECORD_STRUCT*)NewKadrData->BlkPoolPtr);
			RemPoolStructList( &ParReadWeb->ListHTTPData, PointTask );
		}
		ParReadWeb->HttpReqPtr[ParReadWeb->HTTPReqLen] = 0;
		ParReadWeb->HeadrCheck = true;
		ParReadWeb->Status = URP_SUCCESS;
#ifdef _HTTP_READ_DEBUG_
		printf("%s: Sock: %d HTTP header read done (len=%d)\n",
			ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
#endif
		i = FindCmdRequestLine(ParReadWeb->HttpReqPtr, "POST");
		if (i == -1)
		{
            HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
			return;
		}
#ifdef _HTTP_READ_DEBUG_
		printf("%s: HTTP header contains POST request\n". ThrReadWebName);
#endif
		i = FindCmdRequest(ParReadWeb->HttpReqPtr, "Content-Length:");
		pars_read = sscanf(&ParReadWeb->HttpReqPtr[i], "%d", &ContentLength);
		if (!pars_read)
		{
            HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
			return;
		}
					
		j = FindCmdRequest(&ParReadWeb->HttpReqPtr[i], DoubleNextLine);
		if (j == -1)
		{
            HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
			return;
		}
		ParReadWeb->ShiftBeginContent = i + j;
		ParReadWeb->ContentLdSize = ParReadWeb->HTTPReqLen - ParReadWeb->ShiftBeginContent;
		ParReadWeb->ContentLen = ContentLength;
#ifdef _HTTP_READ_DEBUG_
		printf("%s: Sock: %d POST request load %d, %d\n", 
				ThrReadWebName, ParReadWeb->HttpSocket, ContentLength, ParReadWeb->ContentLdSize);
#endif
		if (ContentLength <= ParReadWeb->ContentLdSize) 
		{
			HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
			return;
		}
		ReadBlkBufPtr = GetBuffer(&ReadWebInfoPtr->ReadBlkPool);
		NewKadrData = (RECHTTPDATA*)ReadBlkBufPtr->DataPtr;
		NewKadrData->BlkPoolPtr = ReadBlkBufPtr;
		NewKadrData->WebPoolPtr = NULL;
		NewKadrData->BlockLen = ParReadWeb->HTTPReqLen;
		NewKadrData->Data = (unsigned char*)ParReadWeb->HttpReqPtr;
		AddPoolStructList(&ParReadWeb->ListHTTPData, NewKadrData);
		ParReadWeb->HttpReqPtr = NULL;
	}
}
//---------------------------------------------------------------------------
static void HttpReqLoadDone(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ParReadWeb)
{
#ifdef _READER_PERF_MEASURE_
    unsigned int           DeltaTime;
    unsigned long long int EndTime;
    struct timespec        spec;
#endif

	HttpPageLoadListClean(ReadWebInfoPtr, &ParReadWeb->ListHTTPData);
#ifdef _WEB_EPOOL_SOCKET_
	SocketRemPool(ReadWebInfoPtr, ParReadWeb);
	if (!RemClientInfoSocketHash(&ReadWebInfoPtr->SocketHash, ParReadWeb->HttpSocket))
	{
	    DebugLogPrint(NULL, "%s: Fail to remove socket form sockets reader hash (%d)\r\n", 
		    ThrReadWebName, ParReadWeb->HttpSocket);		
	}
#endif
    RemPoolStructList(&ReadWebInfoPtr->ActiveHttpSessionList, ParReadWeb->ObjPtr);

#ifdef _READER_PERF_MEASURE_
	clock_gettime(CLOCK_REALTIME, &spec);
    EndTime = (unsigned long long int)spec.tv_sec * 1000000;
    EndTime += (unsigned long long int)(spec.tv_nsec / 1000);
    DeltaTime = (unsigned int)(EndTime - ParReadWeb->StartTime);
    printf("DT Load: %u\n", (unsigned int)DeltaTime);
    ParReadWeb->StartTime = (unsigned long long int)spec.tv_sec * 1000000;
    ParReadWeb->StartTime += (unsigned long long int)(spec.tv_nsec / 1000);
#endif
	if ((ParReadWeb->HttpReqPtr) && (ParReadWeb->Status == URP_SUCCESS)) 
		HandleHttpReq(ReadWebInfoPtr, ParReadWeb, ParReadWeb->HttpReqPtr);

#ifdef _READER_PERF_MEASURE_
	clock_gettime(CLOCK_REALTIME, &spec);
    EndTime = (unsigned long long int)spec.tv_sec * 1000000;
    EndTime += (unsigned long long int)(spec.tv_nsec / 1000);
    DeltaTime = (unsigned int)(EndTime - ParReadWeb->StartTime);
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
	WebServerMsgSent(ParReadWeb->IDMessReadWeb, ParReadWeb, 0);
}
//---------------------------------------------------------------------------
static void HttpSessionTimeOutCheck(READER_WEB_INFO *ReadWebInfoPtr)
{
	register unsigned long CurrTime;
	READWEBSOCK     *ParReadWeb = NULL;
    ObjPoolListTask *PointTask1 = NULL;	
	ObjPoolListTask *PointTask2 = NULL;

	CurrTime = GetTickCount();
    ReadWebInfoPtr->CurrTimeTick = CurrTime;
	PointTask1 = (ObjPoolListTask*)GetFistPoolObjectList(&ReadWebInfoPtr->ActiveHttpSessionList);
	while(PointTask1)
	{
		ParReadWeb = (READWEBSOCK*)PointTask1->UsedTask;
		PointTask2 = (ObjPoolListTask*)GetNextPoolObjectList(&ReadWebInfoPtr->ActiveHttpSessionList);
        if ((CurrTime - ParReadWeb->LastDataRxTime) > ParReadWeb->RxTimeOut)
		{
			// HTTP RX timeout is detected
			DebugLogPrint(NULL, "%s: TCP (%d) RX connection closed due to timeout: (RX: %d)\r\n",
				ThrReadWebName, ParReadWeb->HttpSocket, ParReadWeb->HTTPReqLen);
			ParReadWeb->Status = URP_HTTP_RX_TIMEOUT;
			HttpReqLoadDone(ReadWebInfoPtr, ParReadWeb);
			PointTask1 = PointTask2;
		}
		else
		{
			break;
		}
	}
}
//---------------------------------------------------------------------------
static void RxWorkerMsgNotify(READER_WEB_INFO *ReadWebInfoPtr)
{
	unsigned int       QueueSize;
    unsigned char      Msg[8];
    struct sockaddr_in DestAddr;
    int                sent_bytes;
    ReadWebChannel     *ChannelPtr = NULL;
    ReadWebMsgQueue    *MsgQueuePtr = NULL;

#ifdef WIN32
#else
	ChannelPtr = &ReadWebInfoPtr->ReadChannel;
	MsgQueuePtr = &ChannelPtr->ReadWebQueue;
	pthread_mutex_lock(&(MsgQueuePtr->mutex));
	QueueSize = MsgQueuePtr->queue_list.Count;
	pthread_mutex_unlock(&(MsgQueuePtr->mutex));
	if (QueueSize > 1) return;
	
	memset(Msg, 0, 4);
    DestAddr.sin_family      = AF_INET;

  #ifndef _SUN_BUILD_
    DestAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    DestAddr.sin_port        = htons(ReadWebInfoPtr->NotifyPort);
  #else
    DestAddr.sin_addr.s_addr = INADDR_LOOPBACK;
    DestAddr.sin_port        = ReadWebInfoPtr->NotifyPortm;    
  #endif
    sent_bytes = sendto(ReadWebInfoPtr->ReaderSocket, (const char*)(&Msg), 
		    2, 0, (struct sockaddr *) &DestAddr, sizeof(DestAddr));      
    if ( sent_bytes < 0 )
    {
        DebugLogPrint(NULL, "\n%s Failed to sent packet to thread port: %d (errno: %d)\r\n",
			SetTimeStampLine(), ReadWebInfoPtr->NotifyPort, errno);
    }
#endif
}
//---------------------------------------------------------------------------
