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
#include <sys/syscall.h>
#include <sched.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#include "ThrConnWeb.h"
#include "ThrReportMen.h"
#include "ThrReadWebUser.h"
#include "WebServMsgApi.h"
#include "IpAccessControl.h"

#define HTTP_RX_BUF_SIZE 64000
#define HTTP_TX_BUF_SIZE 64000

#define TIME_OUT_CONN_CLOSE 15000
#define MAX_SERV_SOCK_LISTEN   10

char ThrConnWebName[] = "ThrConnWeb";

static void HandleHttpConnect(CONNWEBSOCK *ParConnWeb, unsigned long ClientIp, SOCKET HttpSocket);
//---------------------------------------------------------------------------
#ifdef WIN32
DWORD WINAPI THRConnWebUser(LPVOID Data)
#else
void* THRConnWebUser(void *Data)
#endif
{
    CONNWEBSOCK		*ParConnWeb;
    struct sockaddr AdrRemSocet;
    struct sockaddr AdrLocalSocet;
	unsigned long   ClientIpAddr;
    int				LineLocal,rs,i;
    int             ReuseIpAddr = 1;
    char			ac[80];
    struct in_addr	addr;
    struct hostent	*phe;
    bool            isCheckPass = false;
	bool			DetectLocalIP = false;
	bool            CloseTaskReq = false;
    bool            iOptVal = false;
    int             iOptLen = sizeof(bool);
	int             iResult = 0;
    int             NoDelay = 1;
	DWORD           dwTime = TIME_OUT_CONN_CLOSE;
	const SSL_METHOD *MethodPtr = NULL;
#ifdef WIN32 
    SOCKET	        TelnetSocket;
#else
    int             policy = 0;
    int	            TelnetSocket;
    /* struct sched_param is used to store the scheduling priority */
    struct sched_param params;
	pthread_t       this_thread;
	struct timeval  timeout;
#endif

    ParConnWeb = (CONNWEBSOCK*)Data;
#ifdef WIN32
    printf("User's connect thread startup\n");
#else
    printf("User's connect thread startup (WSP: %d)\n", ParConnWeb->WebServIpPort);
#endif  
    //Initialisation socket at TCP protocol.
#ifdef WIN32    
    if ( gethostname(ac,sizeof(ac))== SOCKET_ERROR) goto ErrorIniTask; //No network support.
    phe = gethostbyname(ac);
    if (phe == 0) goto ErrorIniTask;
    ParConnWeb->WebServerSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if (ParConnWeb->WebServerSocket == SOCKET_ERROR) {
ErrorIniTask:
       WinThreadMsgSend(ParConnWeb->ThrAnswStart, ParConnWeb->IDAnswStart, 0, (LPARAM)WSAGetLastError() );
       WSASetLastError( 0 );
       WSACleanup();
	   ParConnWeb->LocalIPServer = 0;
       ExitThread(0);
       return 0;
      }
#else
    if (!ParConnWeb->LocalIPServer)
    {
        if ( gethostname(ac,sizeof(ac)) == -1)
        {
            printf("Conn WEB gert host name error (Error:%d)\n", errno);
            sem_post(&ParConnWeb->semx);
	        ParConnWeb->LocalIPServer = 0;
            pthread_exit((void *)0);        
        }
        phe = gethostbyname(ac);
        if (phe == NULL)
        {
            printf("Conn WEB gert host by name error (Error:%d)\n", errno);
            sem_post(&ParConnWeb->semx);
	        ParConnWeb->LocalIPServer = 0;
            pthread_exit((void *)0);    
        }
    }
    
    ParConnWeb->WebServerSocket = socket( AF_INET, SOCK_STREAM, 0 );
    if (ParConnWeb->WebServerSocket < 0) 
    {
        printf("Conn WEB serv. descriptor open error (Error:%d)\n", errno);
        sem_post(&ParConnWeb->semx);
        close(ParConnWeb->WebServerSocket);
		ParConnWeb->LocalIPServer = 0;
        pthread_exit((void *)0);
    }

    if (setsockopt(ParConnWeb->WebServerSocket, SOL_SOCKET, SO_REUSEADDR, &ReuseIpAddr, sizeof(int)) == -1)
    {
        printf("Faile to set SO_REUSEADDR for server socket with error (Error:%d)\n", errno);
        sem_post(&ParConnWeb->semx);
        close(ParConnWeb->WebServerSocket);
		ParConnWeb->LocalIPServer = 0;
        pthread_exit((void *)0);
    }

    /* Attempt to set thread real-time priority to the SCHED_OTHER policy */
	this_thread = pthread_self();
	params.sched_priority = sched_get_priority_max(SCHED_FIFO);
	rs = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
    if (rs != 0)
	{
		printf("Conn WEB serv.: unsuccessful in setting thread realtime prio (%u)\n", errno);
	    params.sched_priority = sched_get_priority_max(SCHED_OTHER);
        rs = pthread_setschedparam(this_thread, SCHED_OTHER, &params);
        if (rs != 0)
	    {
            printf("Conn WEB serv.: unsuccessful in setting thread realtime prio\n");
            sem_post(&ParConnWeb->semx);
            close(ParConnWeb->WebServerSocket);
		    ParConnWeb->LocalIPServer = 0;
            pthread_exit((void *)0);
		}
	}

    rs = pthread_getschedparam(this_thread, &policy, &params);
    if (rs != 0)
	{
        printf("Conn WEB serv.: couldn't retrieve real-time scheduling paramers\n");
        sem_post(&ParConnWeb->semx);
        close(ParConnWeb->WebServerSocket);
		ParConnWeb->LocalIPServer = 0;
        pthread_exit((void *)0);
    }

    // Check the correct policy was applied
	switch(policy)
	{
		case SCHED_FIFO:
		    printf("Conn WEB serv.: SCHED_FIFO OK\n");
		    break;

		case SCHED_OTHER:
		    printf("Conn WEB serv.: SCHED_OTHER OK\n");
		    break;
			
		default:
		    printf("Conn WEB serv.: Scheduling is NOT SCHED_OTHER!\n");
		    break;
	}

#endif      
      
    printf("User's connect thread startup - Find local IP\n");
    ParConnWeb->LocalIPServer = 0;
    AdrLocalSocet.sa_family = AF_INET;
    *((u_short*)(AdrLocalSocet.sa_data)) = ntohs( ParConnWeb->IPPortUserWEB ); //PORT_FOR Telnet connection.
    LineLocal = sizeof(struct sockaddr);
    if (!ParConnWeb->IPLocalAddress)
    {
        for (i=0,DetectLocalIP=false;phe->h_addr_list[i] !=0 ;++i)
        {
	        memcpy(&addr,phe->h_addr_list[i],sizeof(struct in_addr));
            ParConnWeb->LocalIPServer = inet_addr(inet_ntoa(addr));
            printf("Check connect  - Addr: 0x%08x Port: %d\n",
                (unsigned int)ParConnWeb->LocalIPServer, ParConnWeb->IPPortUserWEB);            
            *((unsigned long*)(AdrLocalSocet.sa_data+sizeof(u_short))) = ParConnWeb->LocalIPServer;
       	    if (bind(ParConnWeb->WebServerSocket, &AdrLocalSocet, sizeof(struct sockaddr) ) == 0 )
	        {
	            DetectLocalIP=true;
                printf("Local IP is detected 0x%08x\n", (unsigned int)ParConnWeb->LocalIPServer);
		        break;
	        }
        }

	    if (!DetectLocalIP)
        {
#ifdef WIN32
            WinThreadMsgSend(ParConnWeb->ThrAnswStart, ParConnWeb->IDAnswStart, 0, (LPARAM)WSAGetLastError() );
            closesocket(ParConnWeb->WebServerSocket);
            WSASetLastError( 0 );
            WSACleanup();
			ParConnWeb->LocalIPServer = 0;
            ExitThread(0);
            return 0;
#else
            printf("User's connect thread startup is failed - no local IP detection\n");
            sem_post(&ParConnWeb->semx);
            close(ParConnWeb->WebServerSocket);
			ParConnWeb->LocalIPServer = 0;
            pthread_exit((void *)0);
#endif
        }
        strcpy(ac,inet_ntoa(addr));
    }
    else
    {
        ParConnWeb->LocalIPServer = ParConnWeb->IPLocalAddress;
        *((unsigned long*)(AdrLocalSocet.sa_data+sizeof(u_short))) = ParConnWeb->LocalIPServer;
        printf("Local IP is set from configuration - 0x%08x\n", (unsigned int)ParConnWeb->LocalIPServer);
    }
    
    if (updateSocketSize(ParConnWeb->WebServerSocket, SO_SNDBUF, HTTP_TX_BUF_SIZE) != 0)
    {
#ifdef WIN32
       WinThreadMsgSend(ParConnWeb->ThrAnswStart, ParConnWeb->IDAnswStart, 0, (LPARAM)WSAGetLastError() );
       closesocket(ParConnWeb->WebServerSocket);
       WSASetLastError( 0 );
       WSACleanup();
	   ParConnWeb->LocalIPServer = 0;
       ExitThread(0);
       return 0;
#else
       printf("Conn WEB serv. socket TX buf size update is failed (Error:%d)\n", errno);
	   sem_post(&ParConnWeb->semx);
	   close(ParConnWeb->WebServerSocket);
	   pthread_exit((void *)0);
#endif
	}

    if (updateSocketSize(ParConnWeb->WebServerSocket, SO_RCVBUF, HTTP_RX_BUF_SIZE) != 0)
	{
#ifdef WIN32
       WinThreadMsgSend(ParConnWeb->ThrAnswStart, ParConnWeb->IDAnswStart, 0, (LPARAM)WSAGetLastError() );
       closesocket(ParConnWeb->WebServerSocket);
       WSASetLastError( 0 );
       WSACleanup();
	   ParConnWeb->LocalIPServer = 0;
       ExitThread(0);
       return 0;
#else
       printf("Conn WEB serv. socket RX buf size update is failed (Error:%d)\n", errno);
	   sem_post(&ParConnWeb->semx);
	   close(ParConnWeb->WebServerSocket);
	   ParConnWeb->LocalIPServer = 0;
	   pthread_exit((void *)0);
#endif
	}

	ParConnWeb->CtxPtr = NULL;
	if (ParConnWeb->isHttpSecure)
	{
		isCheckPass = false;
		for(;;)
		{			
			OpenSSL_add_all_algorithms();			
	        ERR_load_BIO_strings();
	        ERR_load_crypto_strings();
            SSL_load_error_strings();	
			
	        if(SSL_library_init() < 0)
	        {
		        printf("SSL lib init fail\n");
		        break;
	        }
			
            //OpenSSL_add_ssl_algorithms();
			MethodPtr = SSLv23_server_method(); /* create new server-method instance */
			if (!MethodPtr)
			{
               printf("Fail to get SSLV3 server method (Port: %u)\n", ParConnWeb->IPPortUserWEB);
               break;
			}

            ParConnWeb->CtxPtr = SSL_CTX_new(MethodPtr); /* create new context from method */
            if (!ParConnWeb->CtxPtr)
            {
               printf("Fail to create SSL context (Port: %u)\n", ParConnWeb->IPPortUserWEB);
               break;
            }

			SSL_CTX_set_options(ParConnWeb->CtxPtr, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
			SSL_CTX_set_timeout(ParConnWeb->CtxPtr, ParConnWeb->TimeOut);
			SSL_CTX_set_mode(ParConnWeb->CtxPtr, SSL_MODE_AUTO_RETRY);			
			SSL_CTX_set_session_cache_mode(ParConnWeb->CtxPtr, SSL_SESS_CACHE_NO_AUTO_CLEAR | SSL_SESS_CACHE_SERVER);
			//SSL_CTX_set_session_cache_mode(ParConnWeb->CtxPtr, SSL_SESS_CACHE_SERVER);
            //SSL_CTX_set_session_cache_mode(ParConnWeb->CtxPtr, SSL_SESS_CACHE_OFF);

			/* set the local certificate from CertFile */
			if (SSL_CTX_use_certificate_file(ParConnWeb->CtxPtr, ParConnWeb->SertificateFile, SSL_FILETYPE_PEM) <= 0)
			{
				printf("Fail to set the local certificate for SSL server (Port: %u)\n", ParConnWeb->IPPortUserWEB);
				break;
			}

			/* set the private key from KeyFile (may be the same as CertFile) */
			if (SSL_CTX_use_PrivateKey_file(ParConnWeb->CtxPtr, ParConnWeb->ServerKeyFile, SSL_FILETYPE_PEM) <= 0)
			{
				printf("Fail to set the private key for SSL server (Port: %u)\n", ParConnWeb->IPPortUserWEB);
				break;
			}

			/* verify private key */
			if (!SSL_CTX_check_private_key(ParConnWeb->CtxPtr))
			{
				printf("Private key does not match the public certificate (Port: %u)\n", ParConnWeb->IPPortUserWEB);
				break;
			}

			printf("SSL channel configuration is finished (Port: %u)\n", ParConnWeb->IPPortUserWEB);
			isCheckPass = true;
			break;
		}

		if (!isCheckPass)
		{
			printf("Server run without HTTP secure (Port: %u)\n", ParConnWeb->IPPortUserWEB);
			if (ParConnWeb->CtxPtr) SSL_CTX_free(ParConnWeb->CtxPtr);
            ParConnWeb->isHttpSecure = false;
			ParConnWeb->CtxPtr = NULL;
		}
	}

#ifdef WIN32
	isCheckPass = false;
    while ( 1 )
    {
		if (!DetectLocalIP)
		{
            iResult = bind( ParConnWeb->WebServerSocket, &AdrLocalSocet, sizeof(struct sockaddr) );
            if (iResult == SOCKET_ERROR)
			{
                printf("bind is failed for web connect socket with error: %u\n", WSAGetLastError());
                break;
			}
		}
        iResult = getsockopt(ParConnWeb->WebServerSocket, SOL_SOCKET, SO_KEEPALIVE, (char *) &iOptVal, &iOptLen);
        if (iResult == SOCKET_ERROR) 
        {
            printf("getsockopt for SO_KEEPALIVE failed for web connect socket with error: %u\n", WSAGetLastError());
            break;
        }
		if (ParConnWeb->isKeepAlive) dwTime = (DWORD)(ParConnWeb->TimeOut*1000);
		else                         dwTime = TIME_OUT_CONN_CLOSE;
        iResult = setsockopt(ParConnWeb->WebServerSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTime, sizeof(dwTime));
        if (iResult == SOCKET_ERROR)
        {
            printf("getsockopt for SO_RCVTIMEO failed for web connect socket with error: %u\n", WSAGetLastError());
            break;
        }
        rs = listen(ParConnWeb->WebServerSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR)
        {
            printf("listen is failed for web connect socket with error: %u\n", WSAGetLastError());
            break;
        }
        isCheckPass = true;
		break;
   }

   if (isCheckPass)
   {
	   ParConnWeb->isStartDone = true;
	   WinThreadMsgSend(ParConnWeb->ThrAnswStart, ParConnWeb->IDAnswStart, 
		   (WPARAM)1, (WPARAM)ParConnWeb->LocalIPServer);
       for(;;)
       {
		   if (ParConnWeb->ConnWebCloseReq) break;
           TelnetSocket = accept(ParConnWeb->WebServerSocket,&AdrRemSocet, &LineLocal);
           if (ParConnWeb->ConnWebCloseReq)
           {
               closesocket(TelnetSocket);
               break;
           }
           if (TelnetSocket == SOCKET_ERROR)
           {
	           if ((TelnetSocket == WSAEINPROGRESS) || (TelnetSocket == WSAENOBUFS))
			   {
	               Sleep(5);
                   continue;
			   }
	           else
			   {
	               printf("Failed to ACCEPT HTTP socket (Error: %d)\r\n", WSAGetLastError());
                   break;
			   }
           }
           else
           {
                #ifdef _SERVDEBUG_
                printf("Received connect request from user's host (%d)\r\n", TelnetSocket);
                #endif

				ClientIpAddr = *((unsigned long*)(AdrRemSocet.sa_data+sizeof(u_short)));
				if (ParConnWeb->isDdosProtect)
				{
					if (!CheckIpAccess((unsigned int)ClientIpAddr))
					{
						closesocket(TelnetSocket);
						continue;
					}
				}

                if (updateSocketSize(TelnetSocket, SO_SNDBUF, HTTP_TX_BUF_SIZE) != 0)
				{
                    printf("Receive HTTP data. socket TX buf size update is failed (Error:%d)\n", WSAGetLastError());
                    closesocket(TelnetSocket);
					continue;
				}
                if (updateSocketSize(TelnetSocket, SO_RCVBUF, HTTP_RX_BUF_SIZE) != 0)
				{
                    printf("Send HTTP data. socket RX buf size update is failed (Error:%d)\n", WSAGetLastError());
                    closesocket(TelnetSocket);
					continue;
				}

			    HandleHttpConnect(ParConnWeb, 
				    *((unsigned long*)(AdrRemSocet.sa_data+sizeof(u_short))),
	                TelnetSocket);
		   }
	   }
    }
    else
	{
		WinThreadMsgSend( ParConnWeb->ThrAnswStart, ParConnWeb->IDAnswStart, 
			(WPARAM)0, (WPARAM)ParConnWeb->LocalIPServer );
	}
    closesocket(ParConnWeb->WebServerSocket);
	if (ParConnWeb->CtxPtr) SSL_CTX_free(ParConnWeb->CtxPtr); /* Release SSL context */
    WSASetLastError( 0 );
    WinThreadMsgSend(ParConnWeb->ThrAnswStart, ParConnWeb->IDMessCloseCWeb, 0, 0 );
	ParConnWeb->LocalIPServer = 0;
    ExitThread(0);
#endif

#ifdef _LINUX_X86_
	isCheckPass = false;
    for(;;)
    {
        printf("User's connect thread startup - server socket configuration\n");
        if (!DetectLocalIP)
        {
            rs = bind( ParConnWeb->WebServerSocket, &AdrLocalSocet, sizeof(struct sockaddr) );
            if (rs < 0)
	        {
                printf("bind is failed for web connect socket with error: %d\n", errno);
                break;
            }
        }
        iResult = listen(ParConnWeb->WebServerSocket, MAX_SERV_SOCK_LISTEN);
        if (iResult < 0)
        {
            printf("listen is failed for web connect socket with error: %d\n", errno);
            break;
        }        
        isCheckPass = true;
        ParConnWeb->isStartDone = true;
        break;
    }
    sem_post(&ParConnWeb->semx);
    if (isCheckPass)
    {
        printf("User's connect (%u/%u) thread startup is completed\n",
			(unsigned int)syscall(SYS_gettid), ParConnWeb->WebChanId);
        for(;;)
        {
            TelnetSocket = accept(ParConnWeb->WebServerSocket, &AdrRemSocet, &LineLocal);
			if (ParConnWeb->ConnWebCloseReq)
			{
                if (TelnetSocket > 0)
				{
					iResult = shutdown(TelnetSocket, SHUT_RDWR);
					close(TelnetSocket);
				}
				break;
			}

            if (TelnetSocket < 0)
	        {
	            if ((errno == EAGAIN) || (errno == EINTR) || (errno == EMFILE))
	            {
	                Sleep(5);
                    continue;
	            }
	            else
	            {
                    DebugLogPrint(NULL, "%s: Failed to accept of HTTP request socket (Error: %d)\r\n",
			            ThrConnWebName, errno);
	                break;
	            }
	        }
	        else
	        {
				if (TelnetSocket > 0)
				{
			#ifdef _SERVDEBUG_
                    DebugLogPrint(NULL, "%s: Received connect request from user's host (%d)\r\n",
			            ThrConnWebName, TelnetSocket);
            #endif
					ClientIpAddr = *((unsigned long*)(AdrRemSocet.sa_data+sizeof(u_short)));
					if (ParConnWeb->isDdosProtect)
					{
						if (!CheckIpAccess((unsigned int)ClientIpAddr))
						{
							close(TelnetSocket);
							continue;
						}
					}

                    if (updateSocketSize(TelnetSocket, SO_SNDBUF, HTTP_TX_BUF_SIZE) != 0)
					{
                        DebugLogPrint(NULL, "%s: Receive HTTP data. socket TX buf size update is failed (Error:%d)\n", 
						    ThrConnWebName, errno);
						iResult = shutdown(TelnetSocket, SHUT_RDWR);
	                    close(TelnetSocket);
					    continue;
					}
                    if (updateSocketSize(TelnetSocket, SO_RCVBUF, HTTP_RX_BUF_SIZE) != 0)
					{
						DebugLogPrint(NULL, "%s: Send HTTP data. socket RX buf size update is failed (Error:%d)\n", 
							ThrConnWebName, errno);
						iResult = shutdown(TelnetSocket, SHUT_RDWR);
	                    close(TelnetSocket);
					    continue;
					}                
                    if (setsockopt(TelnetSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&NoDelay, sizeof(NoDelay)) == -1)
                    {
						DebugLogPrint(NULL, "%s: set TCP_NODELAY is failed (Error:%d)\n", ThrConnWebName, errno);
						iResult = shutdown(TelnetSocket, SHUT_RDWR);
	                    close(TelnetSocket);
					    continue;                    
                    }
					
					timeout.tv_sec = 1;
                    timeout.tv_usec = 0;				
		            if (setsockopt(TelnetSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout))  < 0)
                    {
						DebugLogPrint(NULL, "%s: set SO_RCVTIMEO is failed (Error:%d)\n", ThrConnWebName, errno);
						iResult = shutdown(TelnetSocket, SHUT_RDWR);
	                    close(TelnetSocket);
					    continue;                    
                    }
					
			        HandleHttpConnect(ParConnWeb, ClientIpAddr, TelnetSocket);
				}
	        }
        }
    }
	DebugLogPrint(NULL, "%s: Thread http user's connection is closed\r\n", ThrConnWebName);
    WebServerMsgSent(ParConnWeb->IDMessCloseCWeb, 0, 0 );
    if (ParConnWeb->WebServerSocket != -1) close(ParConnWeb->WebServerSocket);
    ParConnWeb->WebServerSocket = -1;
	if (ParConnWeb->CtxPtr) SSL_CTX_free(ParConnWeb->CtxPtr); /* Release SSL context */
	ParConnWeb->LocalIPServer = 0;
    pthread_exit((void *)0);
#endif
}
//---------------------------------------------------------------------------
static void HandleHttpConnect(CONNWEBSOCK *ParConnWeb, unsigned long ClientIp,
	SOCKET HttpSocket)
{
	READER_WEB_INFO        *ReadWebInfoPtr = NULL;
	READWEBSOCK            *ParReadWeb = NULL;
	POOL_RECORD_STRUCT     *ReadWebBufPtr = NULL;
	WEB_READER_POOL_ACCESS *ReaderPoolPtr = NULL;
	READER_WORKER_INFO     *ReaderWorkerPtr = NULL;
	ListItsTask            *ListTasks = NULL;
    ObjListTask		       *PointTask = NULL;

	ReaderWorkerPtr = ParConnWeb->ReaderWorkerPtr;
	ReaderPoolPtr = &ReaderWorkerPtr->WebReaderPool;
#ifdef WIN32
	if (WaitForSingleObject(ReaderPoolPtr->ReadPoolMutex, INFINITE) == WAIT_FAILED)
	{
	    printf("Fail to set read pool mutex (MpttCallClose)\r\n");
	}
#else
    sem_wait(&ReaderPoolPtr->ReadPoolMutex);
#endif

	if (ReaderPoolPtr->ReadWebPool.m_NumUsedRecords < MAX_OPEN_READ_POOL)
	{
		/* Get Reader record fror HTTP request load */
	    ReadWebBufPtr = GetBuffer(&ReaderPoolPtr->ReadWebPool);
		if (ReadWebBufPtr)
		{
	        ParReadWeb = (READWEBSOCK*)ReadWebBufPtr->DataPtr;
            ParReadWeb->ReadBufPoolPtr = (void*)ReadWebBufPtr;

		    /* Get reader instance for HTTP request processing */
            ReadWebInfoPtr = GetReaderInstanceBase(ReaderWorkerPtr);
			if (!ReadWebInfoPtr)
			{
				FreeBuffer(&ReaderPoolPtr->ReadWebPool, ReadWebBufPtr);
				ParReadWeb = NULL;
			}
		}
	}
	else
	{
		DebugLogPrint(NULL, "ReaderWorker: Max limit (%d) of open read pools is reached\r\n", MAX_OPEN_READ_POOL);
	}

#ifdef WIN32
    if (!ReleaseMutex(ReaderPoolPtr->ReadPoolMutex)) 
        printf("Fail to release mutex (Web read pool)\r\n");
#else
    sem_post(&ReaderPoolPtr->ReadPoolMutex);
#endif

	if (!ParReadWeb)
	{
		CloseHttpSocket(HttpSocket);
		return;
	}

	ParReadWeb->StartReqHandleTick = GetTickCount();
	ParReadWeb->IDMessReadWeb = WSU_USERDATA;
	ParReadWeb->HttpClientIP = ClientIp;
	ParReadWeb->HttpSocket = (SOCKET)HttpSocket;
    ParReadWeb->WebChanId = ParConnWeb->WebChanId;
	ParReadWeb->HTTPReqLen = 0;
	ParReadWeb->HttpReqPtr = NULL;
	ParReadWeb->StrCmdHTTP = NULL;
	ParReadWeb->BoundInd = NULL;
	ParReadWeb->PicFileBufPtr = NULL;
    ParReadWeb->XmlFileBufPtr = NULL;
	ParReadWeb->isKeepAlive = false;
    ParReadWeb->isEncodingAccept = false;
	ParReadWeb->MozilaMainVer = 0;
	ParReadWeb->MozilaSubVer = 0;
	ParReadWeb->KeepAliveFlag = ParConnWeb->isKeepAlive;
	ParReadWeb->KeepAliveTime = ParConnWeb->TimeOut;
	ParReadWeb->ServRootDir = ParConnWeb->ServRootDir;
	ParReadWeb->MobDevListPtr = ParConnWeb->MobDevListPtr;
	ParReadWeb->isContentDelivery = ParConnWeb->isContentDelivery;
	ParReadWeb->MobileType = NULL;
	ParReadWeb->SslPtr = NULL;
	ParReadWeb->BioPtr = NULL;
	ParReadWeb->AcceptBioPtr = NULL;
	ParReadWeb->isSslAccept = false;
	ParReadWeb->CtxPtr = ParConnWeb->CtxPtr;
	
    if (!ParReadWeb->isInitDone)
	{
		PoolListInit(&ParReadWeb->ListHTTPData, 4);
		ParReadWeb->isInitDone = true;
	}
	
	ParConnWeb->UserConnectCount++;
	WebServerMsgSent(ParConnWeb->IDMessConnWeb, (void*)ParReadWeb, 0);
	WebDataReadReq(ReadWebInfoPtr, ParReadWeb);
}
//---------------------------------------------------------------------------
bool HandleNextReqKeepAlive(CONNWEBSOCK *ParConnWeb, READWEBSOCK *ParReadWeb)
{
	READER_WEB_INFO    *ReadWebInfoPtr = NULL;

	ReaderInstClear(ParReadWeb);
	ParReadWeb->isKeepAlive = false;
    ParReadWeb->isEncodingAccept = false;   
	ParReadWeb->StartReqHandleTick = GetTickCount();
	ReadWebInfoPtr = GetReaderInstance(ParConnWeb->ReaderWorkerPtr);
	ParConnWeb->UserConnectCount++;
	WebServerMsgSent(ParConnWeb->IDMessConnWeb, (void*)ParReadWeb, 0);
	WebDataReadReq(ReadWebInfoPtr, ParReadWeb);
	return true;
}
//---------------------------------------------------------------------------
void SetKeepAliveTimeout(CONNWEBSOCK *ParConnWeb, unsigned int TimeOut)
{
	ParConnWeb->TimeOut = TimeOut;
    if (ParConnWeb->CtxPtr) SSL_CTX_set_timeout(ParConnWeb->CtxPtr, TimeOut);
}
//---------------------------------------------------------------------------
#ifdef WIN32
void ConnWebThreadCreate(CONNWEBSOCK *ConnWebInfoPtr)
{
	struct tagMSG	WebMsg;

    ConnWebInfoPtr->HtrConnWeb = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)THRConnWebUser,
                        (LPVOID)ConnWebInfoPtr, 0, (LPDWORD)&ConnWebInfoPtr->ThrConnWebId);
    GetMessage(&WebMsg, NULL, ConnWebInfoPtr->ThrAnswStart, ConnWebInfoPtr->ThrAnswStart);
}
#else
void ConnWebThreadCreate(CONNWEBSOCK *ConnWebInfoPtr)
{
    pthread_attr_t	attr, *attrPtr = &attr;
    struct sched_param	sched;
    size_t StackSize = 1024*1024;	

    sem_init(&ConnWebInfoPtr->semx, 0, 0);
    pthread_attr_init(attrPtr);
    (void)pthread_attr_setstacksize (attrPtr, StackSize);
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0)
    {
	    sched.sched_priority = 0;
	    pthread_attr_setschedparam(attrPtr, &sched);
    }	
    printf("\nConnect web user thread sturtup\n");	
    if (pthread_create(&ConnWebInfoPtr->ConnWeb_thr, &attr, &THRConnWebUser, 
	    ConnWebInfoPtr) != 0)
    {
	    printf("Report thread create with %d error!\n", errno);
    }
    else
	{
		sem_wait(&ConnWebInfoPtr->semx);
	}
}
#endif
//---------------------------------------------------------------------------
#ifdef WIN32
void ConnWebClose(CONNWEBSOCK *ConnWebInfoPtr)
{
	ConnWebInfoPtr->ConnWebCloseReq = true;
    if (ConnWebInfoPtr->WebServerSocket > 0)
        closesocket(ConnWebInfoPtr->WebServerSocket);
    ConnWebInfoPtr->WebServerSocket = -1;
    if (ConnWebInfoPtr->ThrConnWebId)
        WaitCloseProcess(ConnWebInfoPtr->HtrConnWeb);
    CloseHandle(ConnWebInfoPtr->HtrConnWeb);
    ConnWebInfoPtr->HtrConnWeb = 0;
    ConnWebInfoPtr->ThrConnWebId = 0;
    return;
}
#else
void ConnWebClose(CONNWEBSOCK *ConnWebInfoPtr)
{
	ConnWebInfoPtr->ConnWebCloseReq = true;
    if (ConnWebInfoPtr->WebServerSocket > 0)
        close(ConnWebInfoPtr->WebServerSocket);
    ConnWebInfoPtr->WebServerSocket = -1;
    pthread_join(ConnWebInfoPtr->ConnWeb_thr, NULL);
    sem_destroy(&ConnWebInfoPtr->semx);
    return;
}
#endif
//---------------------------------------------------------------------------

