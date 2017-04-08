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

#include "ThrSendWebUser.h"
#include "ThrConnWeb.h"
#include "SysMessages.h"
#include "TrTimeOutThread.h"
#include "ThrReportMen.h"
#include "WebServMsgApi.h"

char ThrSendWebName[] = "ThrSendWebUser";

static void SendThrCloseNotify(SENDER_WEB_INFO *SendWebInfoPtr);
static void SendWebMsgQueueCreate(SendWebMsgQueue *p_msg_queue);
static void SendWebMsgQueueDestroy(SendWebMsgQueue *p_msg_queue);
static void SendWebMsgQueuePost(SendWebMsgQueue *p_msg_queue, void* p_message);
static bool GetSendWebMessageQueue(SendWebMsgQueue *p_msg_queue, void **pp_message);
static void SendWebMsgQueueMarkDestroy(SendWebMsgQueue *p_msg_queue);
static SendWebMessage* GetSendWebMsgPool(SendWebChannel *ChannelPtr, unsigned char MsgTag);
static void FreeSendWebMsgPool(SendWebChannel *ChannelPtr, SendWebMessage *SysMgmtMsgPtr);
static void HandleWebSendRequest(SENDER_WEB_INFO *SendWebInfoPtr, SendWebMessage *SendWebMsgPtr);
//---------------------------------------------------------------------------
#ifdef WIN32
DWORD WINAPI THRSendWebUser(LPVOID Data)
#else
void* THRSendWebUser(void *Data)
#endif
{
    bool            NeedsClose = false;
	SENDER_WEB_INFO *SendWebInfoPtr = NULL;
	SendWebChannel  *ChannelPtr = NULL;
	SendWebMessage  *SendWebMsgPtr = NULL;

    SendWebInfoPtr = (SENDER_WEB_INFO*)Data;
	ChannelPtr = &SendWebInfoPtr->SendChannel;
	SendWebInfoPtr->ComprBufSize = START_COMPRES_MEM_BLOCK_SIZE;
    SendWebInfoPtr->ComprDataBufPtr = (char*)AllocateMemory((SendWebInfoPtr->ComprBufSize+512)*(sizeof(char)));
	if (!SendWebInfoPtr->ComprDataBufPtr)
	{
		SendThrCloseNotify(SendWebInfoPtr);
#ifdef WIN32
        ExitThread( 0 );
#else
        pthread_exit((void *)0);
#endif
	}

#ifdef WIN32
    CreateThreadTimerCBD(OnWebSendHbTimerExp, GetCurrentThreadId(), 30*TMR_ONE_SEC, WEB_SEND_HB_CHECK_TMR_ID, true, ChannelPtr);
	DebugLogPrint(NULL, "Sender worker thread startup\n");
#else
	CreateThreadTimerCBD(OnWebSendHbTimerExp, SendWebInfoPtr->NotifyPort, 30*TMR_ONE_SEC, WEB_SEND_HB_CHECK_TMR_ID, true, ChannelPtr);
	DebugLogPrint(NULL, "Sender worker thread startup (NP: %d, WSP: %d)\n", 
        SendWebInfoPtr->NotifyPort, SendWebInfoPtr->WebServPort);
#endif

    /* Read the data from the WEB sender messages queue. */
    while (GetSendWebMessageQueue(&ChannelPtr->SendWebQueue, (void**)&SendWebMsgPtr))
    {
        switch(SendWebMsgPtr->MsgTag)
        {
		    case WSC_CLOSE_CHANNEL:
				NeedsClose = true;
				break;

			case WSC_WEB_SEND_REQ:
				HandleWebSendRequest(SendWebInfoPtr, SendWebMsgPtr);
				break;

			case WSC_HB_REQ:
#ifndef WIN32
                WebServerMsgSent(WSU_HBACTIONTHR, SendWebInfoPtr, (void*)(tulong)(unsigned int)WHN_SENDER_IND);
#endif
				break;

            default:
                DebugLogPrint(NULL, "%s: Unexpected(0x%02X) message tag is received by WEB sender\n",
					ThrSendWebName, SendWebMsgPtr->MsgTag);
                break;
        }
        FreeSendWebMsgPool(ChannelPtr, SendWebMsgPtr);
		if (NeedsClose) break;
    }

	if (SendWebInfoPtr->ComprDataBufPtr) FreeMemory(SendWebInfoPtr->ComprDataBufPtr);
	SendWebInfoPtr->ComprDataBufPtr = NULL;
	SendThrCloseNotify(SendWebInfoPtr);
#ifdef WIN32
	CloseThreadTimer(GetCurrentThreadId(), WEB_SEND_HB_CHECK_TMR_ID);
	WSASetLastError( 0 );
    ExitThread( 0 );
#else
	CloseThreadTimer(SendWebInfoPtr->NotifyPort, WEB_SEND_HB_CHECK_TMR_ID);
	pthread_exit((void *)0);
#endif
}
//---------------------------------------------------------------------------
static void HandleWebSendRequest(SENDER_WEB_INFO *SendWebInfoPtr, SendWebMessage *SendWebMsgPtr)
{
	char           *HttpBodyPtr = NULL;
	unsigned int    PageStartTxPoint;
	unsigned int    TxBlockSize;
	int             BytesSent, SslError;
	int             CompLen;
	unsigned int    HttpBodyLen;
	int             TcpErrCode = 0;
	bool            MsgSentResult = false;
	unsigned char   isBinary = 0;
	SendWebChannel  *ChannelPtr = NULL;
    SENDWEBSOCK	    *ParSendWeb = NULL;
	char            LenInf[16];

	ParSendWeb = (SENDWEBSOCK*)SendWebMsgPtr->DataPtr;
	ParSendWeb->WorkPhase = 0;
	ChannelPtr = &SendWebInfoPtr->SendChannel;
	if (!ParSendWeb->HttpSocket) return;

    PageStartTxPoint = 0;
    ParSendWeb->SentBytes = 0;
    ParSendWeb->isDeliverySuccess = true;
#ifdef _LINUX_X86_
  #ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: The received request for data send start for Sct=%d\r\n", 
		ThrSendWebName, ParSendWeb->HttpSocket);
  #endif
#endif
	if (ParSendWeb->isBodyCompress)
	{
		ParSendWeb->WorkPhase = 1;
		if (ParSendWeb->HTTPRespLen > SendWebInfoPtr->ComprBufSize)
		{
			FreeMemory(SendWebInfoPtr->ComprDataBufPtr);
			SendWebInfoPtr->ComprDataBufPtr = (char*)AllocateMemory((ParSendWeb->HTTPRespLen+512)*(sizeof(char)));
			SendWebInfoPtr->ComprBufSize = ParSendWeb->HTTPRespLen;
		}

		if (SendWebInfoPtr->ComprDataBufPtr)
		{
#ifdef _LINUX_X86_
	#ifdef _SERVDEBUG_
		    DebugLogPrint(NULL, "%s: Start compress of data Sct=%d, (len=%d) \r\n", 
				ThrSendWebName, ParSendWeb->HttpSocket, ParSendWeb->HTTPRespLen);
    #endif
#endif
			if (ParSendWeb->FileType == FRT_JPG_PIC) isBinary = 1;
			ParSendWeb->WorkPhase = 2;
            CompLen = zip(&ParSendWeb->ZipInfo, ParSendWeb->FileName, isBinary, 
			    ParSendWeb->HTTPRespLen, (unsigned char*)ParSendWeb->HttpRespPtr,
				(unsigned char*)SendWebInfoPtr->ComprDataBufPtr);
			ParSendWeb->WorkPhase = 3;
            sprintf(LenInf, "%d", CompLen);
			memcpy(ParSendWeb->HeaderBeginSetLenPtr, LenInf, strlen(LenInf));
			HttpBodyPtr = SendWebInfoPtr->ComprDataBufPtr;
			HttpBodyLen = CompLen;
			if (!ParSendWeb->isHashData) FreeMemory(ParSendWeb->HttpRespPtr);
			ParSendWeb->HttpRespPtr = NULL;

#ifdef _LINUX_X86_
	#ifdef _SERVDEBUG_
		    DebugLogPrint(NULL, "%s: Compress is done, Start send data Sct=%d, (hlen=%d, len=%d) \r\n", 
				ThrSendWebName, ParSendWeb->HttpSocket,
				ParSendWeb->HeaderLen, HttpBodyLen);
    #endif
#endif
		}
		else
		{
#ifdef _LINUX_X86_
	#ifdef _SERVDEBUG_
		    DebugLogPrint(NULL, "%s: No compress buf, Start send data Sct=%d, (hlen=%d, len=%d) \r\n", 
				ThrSendWebName, ParSendWeb->HttpSocket,
				ParSendWeb->HeaderLen, HttpBodyLen);
    #endif
#endif
		    HttpBodyPtr = ParSendWeb->HttpRespPtr;
		    HttpBodyLen = ParSendWeb->HTTPRespLen;
		}
	}
	else
	{
#ifdef _LINUX_X86_
	#ifdef _SERVDEBUG_
		    DebugLogPrint(NULL, "%s: Start send data without compress Sct=%d, (hlen=%d, len=%d) \r\n", 
				ThrSendWebName, ParSendWeb->HttpSocket, 
				ParSendWeb->HeaderLen, ParSendWeb->HTTPRespLen);
    #endif
#endif
		HttpBodyPtr = ParSendWeb->HttpRespPtr;
		HttpBodyLen = ParSendWeb->HTTPRespLen;
	}

	ParSendWeb->WorkPhase = 4;
	if (ParSendWeb->HeaderLen > 0)
	{
	#ifdef _LINUX_X86_
		for(;;)
		{
			if (ParSendWeb->SslPtr)
			{	
				BytesSent = SSL_write(ParSendWeb->SslPtr,
					&ParSendWeb->HttpRespHeader[PageStartTxPoint], 
					(ParSendWeb->HeaderLen - PageStartTxPoint));
				SslError = SSL_get_error(ParSendWeb->SslPtr, BytesSent); 
			}
			else
			{
				BytesSent = send(ParSendWeb->HttpSocket,
					&ParSendWeb->HttpRespHeader[PageStartTxPoint], 
					(ParSendWeb->HeaderLen - PageStartTxPoint), 0 );
			}

			if (BytesSent < 0)
			{
				if (ParSendWeb->SslPtr)
				{
					SslError = SSL_get_error(ParSendWeb->SslPtr, BytesSent); 
					DebugLogPrint(NULL, "%s: SSL connection closed due to error: %d (%d TX: %d)\r\n",
						ThrSendWebName, SslError, ParSendWeb->HttpSocket, ParSendWeb->SentBytes);
					ParSendWeb->isDeliverySuccess = false;
					break;
				}
				else
				{
					if ((errno == EAGAIN) || (errno == EINTR))
					{
						Sleep(5);
						continue;
					}
					else
					{
						DebugLogPrint(NULL, "%s: TCP connection closed due to error: %d (%d TX: %d)\r\n",
							ThrSendWebName, errno, ParSendWeb->HttpSocket, ParSendWeb->SentBytes);
						ParSendWeb->isDeliverySuccess = false;
						break;
					}
				}
			}
			else
			{
				if (BytesSent < (int)(ParSendWeb->HeaderLen - PageStartTxPoint))
				{
					PageStartTxPoint += BytesSent;
					Sleep(5);
				}
				else
				{
					break;
				}
			}
		}
	#endif
	#ifdef WIN32
		for(;;)
		{
			if (ParSendWeb->SslPtr)
			{
				BytesSent = SSL_write(ParSendWeb->SslPtr,
					&ParSendWeb->HttpRespHeader[PageStartTxPoint], 
					(ParSendWeb->HeaderLen - PageStartTxPoint));
				if (BytesSent < 0) BytesSent = SOCKET_ERROR;
			}
			else
			{
				BytesSent = send(ParSendWeb->HttpSocket,
					&ParSendWeb->HttpRespHeader[PageStartTxPoint], 
					(ParSendWeb->HeaderLen - PageStartTxPoint), 0 );
			}

			if (BytesSent == SOCKET_ERROR)
			{
				if (ParSendWeb->SslPtr)
				{
					SslError = SSL_get_error(ParSendWeb->SslPtr, BytesSent); 
					DebugLogPrint(NULL, "%s: SSL connection closed due to error: %d (%d TX: %d)\r\n",
						ThrSendWebName, SslError, ParSendWeb->HttpSocket, ParSendWeb->SentBytes);
					ParSendWeb->isDeliverySuccess = false;
					break;
				}
				else
				{
					TcpErrCode = WSAGetLastError();
					if ((TcpErrCode == WSAEINPROGRESS) || (TcpErrCode == WSAENOBUFS))
					{
						Sleep(5);
						continue;
					}
					else
					{
						DebugLogPrint(NULL, "%s: TCP connection closed due to error: %d (%d TX: %d)\r\n",
							ThrSendWebName, TcpErrCode, ParSendWeb->HttpSocket, 
							ParSendWeb->SentBytes);
						ParSendWeb->isDeliverySuccess = false;
						break;
					}
				}
			}
			else
			{
				if (BytesSent < (int)(ParSendWeb->HeaderLen - PageStartTxPoint))
				{
					PageStartTxPoint += BytesSent;
					Sleep(5);
				}
				else
				{
					break;
				}
			}
		}
	#endif

	#ifdef _SERVDEBUG_
		DebugLogPrint(NULL, "%s: Header send is done Sct=%d\r\n", 
			ThrSendWebName, ParSendWeb->HttpSocket);
	#endif
	}

	ParSendWeb->WorkPhase = 5;
	if(!ParSendWeb->isHeaderReq && ParSendWeb->isDeliverySuccess)
	{
		PageStartTxPoint = 0;
		while(PageStartTxPoint < HttpBodyLen)
		{
			if ((HttpBodyLen-PageStartTxPoint) < TCP_TX_BLOCK_SIZE)
			{
				TxBlockSize = HttpBodyLen-PageStartTxPoint;
			}
			else
			{
				TxBlockSize = TCP_TX_BLOCK_SIZE;
			}
		#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Send data block is begin (Sct=%d, Ptr=%d, Bs=%d)\r\n", ThrSendWebName, 
				ParSendWeb->HttpSocket, PageStartTxPoint, TxBlockSize);
		#endif
			if (ParSendWeb->SslPtr)
			{
				BytesSent = SSL_write(ParSendWeb->SslPtr,
				    &HttpBodyPtr[PageStartTxPoint], TxBlockSize);
        #ifdef WIN32
			   if (BytesSent < 0) BytesSent = SOCKET_ERROR;
        #endif
			}
			else
			{
				BytesSent = send(ParSendWeb->HttpSocket,
					&HttpBodyPtr[PageStartTxPoint], TxBlockSize, 0 );
			}
	#ifdef _LINUX_X86_
		#ifdef _SERVDEBUG_
			DebugLogPrint(NULL, "%s: Send data block is done (Sct=%d, SentByts=%d, Ptr=%d, Bs=%d)\r\n", ThrSendWebName, 
				ParSendWeb->HttpSocket, BytesSent, PageStartTxPoint, TxBlockSize);
		#endif
			if (BytesSent < 0)
			{
				if (ParSendWeb->SslPtr)
				{
					SslError = SSL_get_error(ParSendWeb->SslPtr, BytesSent); 
					DebugLogPrint(NULL, "%s: SSL connection closed due to error: %d (%d TX: %d)\r\n",
						ThrSendWebName, SslError, ParSendWeb->HttpSocket, ParSendWeb->SentBytes);
					ParSendWeb->isDeliverySuccess = false;
					break;
				}
				else
				{
					if ((errno == EAGAIN) || (errno == EINTR))
					{
						Sleep(5);
						continue;
					}
					else
					{
						DebugLogPrint(NULL, "%s: TCP connection closed due to error: %d (%d TX: %d)\r\n",
							ThrSendWebName, errno, ParSendWeb->HttpSocket, ParSendWeb->SentBytes);
						ParSendWeb->isDeliverySuccess = false;
						break;
					}
				}
			}
	#endif
	#ifdef WIN32
			if (BytesSent == SOCKET_ERROR)
			{
				if (ParSendWeb->SslPtr)
				{
					SslError = SSL_get_error(ParSendWeb->SslPtr, BytesSent); 
					DebugLogPrint(NULL, "%s: SSL connection closed due to error: %d (%d TX: %d)\r\n",
						ThrSendWebName, SslError, ParSendWeb->HttpSocket, ParSendWeb->SentBytes);
					ParSendWeb->isDeliverySuccess = false;
					break;
				}
				else
				{
					TcpErrCode = WSAGetLastError();
					if ((TcpErrCode == WSAEINPROGRESS) || (TcpErrCode == WSAENOBUFS))
					{
						Sleep(5);
						continue;
					}
					else
					{
						DebugLogPrint(NULL, "%s: TCP connection closed due to error: %d (%d TX: %d)\r\n",
							ThrSendWebName, TcpErrCode, ParSendWeb->HttpSocket, 
							ParSendWeb->SentBytes);
						ParSendWeb->isDeliverySuccess = false;
						break;
					}
				}
			}
	#endif                
			else
			{
				ParSendWeb->SentBytes += BytesSent;
				if (BytesSent < (int)TxBlockSize)
				{
					PageStartTxPoint += BytesSent;
					Sleep(5);
				}
				else
				{
					PageStartTxPoint += TxBlockSize;
				}
			}
		}

		ParSendWeb->WorkPhase = 6;
		if (!ParSendWeb->isHashData && ParSendWeb->HttpRespPtr) 
			FreeMemory(ParSendWeb->HttpRespPtr);
		ParSendWeb->HttpRespPtr = NULL;
		ParSendWeb->WorkPhase = 0;
	}
#ifdef _SERVDEBUG_
    DebugLogPrint(NULL, "%s: Sent HTTP response is done (%d)\r\n", 
        ThrSendWebName, ParSendWeb->HttpSocket);
#endif
    WebServerMsgSent(SendWebInfoPtr->IDMessSendWeb, (void*)ParSendWeb, 0);
}
//---------------------------------------------------------------------------
static void SendThrCloseNotify(SENDER_WEB_INFO *SendWebInfoPtr)
{
	SendWebInfoPtr->isThreadReady = false;
	if (SendWebInfoPtr->isThreadStopReq)
    {
#ifdef WIN32
        DebugLogPrint(NULL, "Sender worker thread stopped\n");
#else
        DebugLogPrint(NULL, "Sender worker thread stopped (NP: %d, WSP: %d)\n", 
            SendWebInfoPtr->NotifyPort, SendWebInfoPtr->WebServPort);
#endif
        return;
    }
    DebugLogPrint(NULL, "%s: Sent HTTP response thread is closed due to issue\r\n", ThrSendWebName);
//    WebServerMsgSent(SendWebInfoPtr->IDMessSendWeb, ParSendWeb, 0);
	return;
}
//---------------------------------------------------------------------------
#ifdef WIN32
bool SendWebThreadCreate(SENDER_WEB_INFO *SendWebInfoPtr)
{
	SendWebInfoPtr->HTRSENDDATAWEB = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)THRSendWebUser,
		    (LPVOID)(SendWebInfoPtr), 0, (LPDWORD)&SendWebInfoPtr->ThrSendWebID);
	if (SendWebInfoPtr->HTRSENDDATAWEB)
	{
		SendWebInfoPtr->isThreadReady = true;
		return true;
	}
	else
	{
		DebugLogPrint(NULL, "SenderWorker: Fail to create HTTP send thread\r\n");
		return false;
	}
}
#else
bool SendWebThreadCreate(SENDER_WEB_INFO *SendWebInfoPtr)
{
    pthread_attr_t	*attrPtr = &SendWebInfoPtr->ThrAttr;
    struct sched_param	sched;
	bool Result = true;
    size_t StackSize = 2048*1024;
	SendWebChannel *ChannelPtr = NULL;
    
	ChannelPtr = &SendWebInfoPtr->SendChannel;
	memset(ChannelPtr, 0, sizeof(SendWebChannel));
    PoolListInit(&ChannelPtr->ListSendWebMsg, INIT_WEB_SENDER_BLK_COUNT);
    CreatePool(&ChannelPtr->SendWebMsgPool, INIT_WEB_SENDER_BLK_COUNT, sizeof(SendWebMessage));
    pthread_mutex_init(&ChannelPtr->SendWebMsgAccess, NULL);
    SendWebMsgQueueCreate(&ChannelPtr->SendWebQueue);

    pthread_attr_init(attrPtr);
    (void)pthread_attr_setstacksize (attrPtr, StackSize);    
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0)
    {
	    sched.sched_priority = 0;
	    pthread_attr_setschedparam(attrPtr, &sched);
    }		
    if (pthread_create(&SendWebInfoPtr->SendWebResp_thr, 
		&SendWebInfoPtr->ThrAttr, &THRSendWebUser, SendWebInfoPtr) != 0)
    {
	    printf("ReadWeb thread create with %d error!\n", errno);
	    pthread_attr_destroy(&SendWebInfoPtr->ThrAttr);
        DestroyPoolListStructs(&ChannelPtr->ListSendWebMsg);
        DestroyPool(&ChannelPtr->SendWebMsgPool);
		SendWebMsgQueueDestroy(&ChannelPtr->SendWebQueue);
        pthread_mutex_destroy(&ChannelPtr->SendWebMsgAccess);
		Result = false;
    }
	return Result;
}
#endif
//---------------------------------------------------------------------------
void StopSenderThread(SENDER_WEB_INFO *SendWebInfoPtr)
{
	SendWebChannel  *ChannelPtr = NULL;
    SendWebMessage  *SendWebMsgPtr = NULL;
    ObjPoolListTask *PointTask = NULL;
    SENDWEBSOCK	    *ParSendWeb = NULL;

	ChannelPtr = &SendWebInfoPtr->SendChannel;
	SendWebInfoPtr->isThreadStopReq = true;

    SendWebMsgPtr = GetSendWebMsgPool(ChannelPtr, WSC_CLOSE_CHANNEL);
    if (!SendWebMsgPtr) return;
	SendWebMsgQueuePost(&ChannelPtr->SendWebQueue, SendWebMsgPtr);
	Sleep(50);

#ifdef WIN32
	WaitCloseProcess(SendWebInfoPtr->HTRSENDDATAWEB);
	CloseHandle(SendWebInfoPtr->HTRSENDDATAWEB);
    if (WaitForSingleObject(ChannelPtr->SendWebMsgAccess, INFINITE) == WAIT_FAILED)
	{
		printf("SendWebThreadClose - Fail to get access mutex\r\n");
        return;
	}
#else
    pthread_join(SendWebInfoPtr->SendWebResp_thr, NULL);
	pthread_attr_destroy(&SendWebInfoPtr->ThrAttr);
	pthread_mutex_lock(&ChannelPtr->SendWebMsgAccess);
#endif
    while (ChannelPtr->ListSendWebMsg.Count)
    {
        PointTask = (ObjPoolListTask*)ChannelPtr->ListSendWebMsg.FistTask;
        SendWebMsgPtr = (SendWebMessage*)PointTask->UsedTask;
		if (SendWebMsgPtr->MsgTag == WSC_WEB_SEND_REQ)
		{
		    ParSendWeb = (SENDWEBSOCK*)SendWebMsgPtr->DataPtr;
		    CloseHttpSocket(ParSendWeb->HttpSocket);
			ParSendWeb->HttpSocket = -1;
		}
        RemPoolStructList(&ChannelPtr->ListSendWebMsg, PointTask);
    }

#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->SendWebMsgAccess)) 
        printf("SendWebThreadClose - Fail to release memory mutex\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->SendWebMsgAccess);
#endif
    DestroyPoolListStructs(&ChannelPtr->ListSendWebMsg);
    DestroyPool(&ChannelPtr->SendWebMsgPool);
    SendWebMsgQueueDestroy(&ChannelPtr->SendWebQueue);
#ifdef WIN32
	CloseHandle(ChannelPtr->SendWebMsgAccess);
#else
    pthread_mutex_destroy(&ChannelPtr->SendWebMsgAccess);
#endif
    DebugLogPrint(NULL, "%s: SendWeb close is completed\n", ThrSendWebName);
}
//---------------------------------------------------------------------------
static void SendWebMsgQueueCreate(SendWebMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Initializing WEB sender channel messages queue.\n", ThrSendWebName);
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
    PoolListInit(&MsgQueuePtr->queue_list, INIT_WEB_SENDER_MSG_COUNT);
    MsgQueuePtr->destroy = false;
}
//---------------------------------------------------------------------------
static void SendWebMsgQueueMarkDestroy(SendWebMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Marking WEB sender message queue for destroy.\n", ThrSendWebName);
    MsgQueuePtr->destroy = true;
    SendWebMsgQueuePost(MsgQueuePtr, NULL);
}
//---------------------------------------------------------------------------
static void SendWebMsgQueuePost(SendWebMsgQueue *MsgQueuePtr, void *p_message)
{
#ifdef WIN32
    unsigned int CurrUsage;

    if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	{
		printf("Fail to get mutex (SendWebMsgQueuePost)\r\n");
        return;
	}
	CurrUsage = (unsigned int)MsgQueuePtr->queue_list.Count;
	AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (SendWebMsgQueuePost)\r\n");
	if (!CurrUsage) WinThreadMsgSend(MsgQueuePtr->ThrSendWeb, TWS_MSG_NOTIFY, 0, 0);
#else
    pthread_mutex_lock(&(MsgQueuePtr->mutex));
    AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
    pthread_cond_signal(&(MsgQueuePtr->cond_var));
#endif
}
//---------------------------------------------------------------------------
static void SendWebMsgQueueDestroy(SendWebMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: De-initializing WEB sender messages queue.\n", ThrSendWebName);
#ifdef WIN32
	CloseHandle(MsgQueuePtr->mutex);
#else
    pthread_mutex_destroy(&(MsgQueuePtr->mutex));
    pthread_cond_destroy(&(MsgQueuePtr->cond_var));
#endif
    DestroyPoolListStructs(&MsgQueuePtr->queue_list);
}
//---------------------------------------------------------------------------
static bool GetSendWebMessageQueue(SendWebMsgQueue *MsgQueuePtr, void **pp_message)
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
		printf("Fail to get mutex (1) (GetSendWebMessageQueue)\r\n");
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
                printf("Fail to release mutex (GetSendWebMessageQueue)\r\n");
			GetMessage(&SysMsg, NULL, TWS_MSG_NOTIFY, TWS_MSG_NOTIFY);
#else
            pthread_cond_wait(&(MsgQueuePtr->cond_var), &(MsgQueuePtr->mutex));
#endif
#ifdef WIN32
            if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	        {
		        printf("Fail to get mutex (2) (GetSendWebMessageQueue)\r\n");
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
		DebugLogPrint(NULL, "%s: Wokeup WEB sender due to destroy flag.\n", ThrSendWebName);
        status = false;
    }
#ifdef WIN32
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (GetSendWebMessageQueue)\r\n");
#else
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
#endif
    return status;
}
//---------------------------------------------------------------------------
static SendWebMessage* GetSendWebMsgPool(SendWebChannel *ChannelPtr, unsigned char MsgTag)
{
    SendWebMessage   *SendWebMsgPtr = NULL;
    POOL_RECORD_STRUCT *ObjTaskPtr = NULL;

#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->SendWebMsgAccess, INFINITE) == WAIT_FAILED) return NULL;
#else
    pthread_mutex_lock(&ChannelPtr->SendWebMsgAccess);
#endif
    ObjTaskPtr = GetBuffer(&ChannelPtr->SendWebMsgPool);
    if (!ObjTaskPtr)
    {
		DebugLogPrint(NULL, "%s: No buffers for WEB sender message delivery.\n", ThrSendWebName);
#ifdef WIN32
        if (!ReleaseMutex(ChannelPtr->SendWebMsgAccess)) 
            printf("Fail to release mutex (GetSendWebMsgPool)\r\n");
#else
        pthread_mutex_unlock(&ChannelPtr->SendWebMsgAccess);
#endif
        return NULL;
    }
    SendWebMsgPtr = (SendWebMessage*)ObjTaskPtr->DataPtr;
    SendWebMsgPtr->MsgTag = MsgTag;
    SendWebMsgPtr->BlkPoolPtr = ObjTaskPtr;
    SendWebMsgPtr->ObjPtr = AddPoolStructListObj(&ChannelPtr->ListSendWebMsg, SendWebMsgPtr);
    if (ChannelPtr->MaxSimultSendWebMsgs < (unsigned int)ChannelPtr->ListSendWebMsg.Count)
        ChannelPtr->MaxSimultSendWebMsgs = (unsigned int)ChannelPtr->ListSendWebMsg.Count;
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->SendWebMsgAccess)) 
        printf("Fail to release mutex (GetSendWebMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->SendWebMsgAccess);
#endif
    return SendWebMsgPtr;
}
//---------------------------------------------------------------------------
static void FreeSendWebMsgPool(SendWebChannel *ChannelPtr, SendWebMessage *SendWebMsgPtr)
{
#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->SendWebMsgAccess, INFINITE) == WAIT_FAILED)
	{
        return;
	}
#else
    pthread_mutex_lock(&ChannelPtr->SendWebMsgAccess);
#endif
    FreeBuffer(&ChannelPtr->SendWebMsgPool, SendWebMsgPtr->BlkPoolPtr);
    RemPoolStructList(&ChannelPtr->ListSendWebMsg, SendWebMsgPtr->ObjPtr);
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->SendWebMsgAccess)) 
        printf("Fail to release mutex (FreeSendWebMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->SendWebMsgAccess);
#endif
}
//---------------------------------------------------------------------------
void OnWebSendHbTimerExp(unsigned int TimerId, void *DataPtr)
{
    SendWebMessage *SendWebMsgPtr = NULL;
    SendWebChannel *ChannelPtr = NULL;

	ChannelPtr = (SendWebChannel*)DataPtr;
	if (!ChannelPtr) return;
    SendWebMsgPtr = GetSendWebMsgPool(ChannelPtr, WSC_HB_REQ);
    if (!SendWebMsgPtr) return;         
	SendWebMsgQueuePost(&ChannelPtr->SendWebQueue, SendWebMsgPtr);
}
//---------------------------------------------------------------------------
void WebDataSentReq(SENDER_WEB_INFO *SendWebInfoPtr, SENDWEBSOCK *SendWebDataPtr)
{
	register SendWebChannel  *ChannelPtr = NULL;
    register SendWebMessage  *SendWebMsgPtr = NULL;

	ChannelPtr = &SendWebInfoPtr->SendChannel;
    SendWebMsgPtr = GetSendWebMsgPool(ChannelPtr, WSC_WEB_SEND_REQ);
    if (!SendWebMsgPtr) return; 
	SendWebMsgPtr->DataPtr = (void*)SendWebDataPtr;
	SendWebMsgQueuePost(&ChannelPtr->SendWebQueue, SendWebMsgPtr);
}
//---------------------------------------------------------------------------
unsigned int GetMaxUsageSenderQueue(SENDER_WEB_INFO *SendWebInfoPtr)
{
	return SendWebInfoPtr->SendChannel.MaxSimultSendWebMsgs;
}
//---------------------------------------------------------------------------
unsigned int GetUsageSenderQueue(SENDER_WEB_INFO *SendWebInfoPtr)
{
	return SendWebInfoPtr->SendChannel.ListSendWebMsg.Count;
}
//---------------------------------------------------------------------------
