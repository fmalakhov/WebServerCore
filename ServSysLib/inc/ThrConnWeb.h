# if ! defined( ThrConnWebH )
#	define ThrConnWebH	/* only include me once */

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

#ifndef ReaderWorkerH
#include "ReaderWorker.h"
#endif

#ifndef _SUN_BUILD_
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#endif

#define PRIMARY_WEB_CHAN   1
#define SECONDARY_WEB_CHAN 2
#define FORWARD_WEB_CHAN   3

typedef struct {
#ifdef WIN32
        DWORD	       ThrAnswStart;
		DWORD          ThrReportMgrId;
		DWORD		   ThrConnWebId;
	    unsigned       IDAnswStart;
	    HANDLE         ReadPoolMutex;
        HANDLE		   HtrConnWeb;
		SOCKET         WebServerSocket;
#else
        int            WebServerSocket;   
		sem_t          semx;
		pthread_t	   ConnWeb_thr;     /* Web server connection thread */
#endif
        bool           isStartDone;
		bool           isHttpSecure;     /* Flag of HTTP over SSL */
		bool           isDdosProtect;    /* Flag of server protection from DDos */
		bool           isForfardChan;    /* Flag of channel work in forward mode */
		bool           isContentDelivery; /* Flag of content delivery mode */
		char           *ServerKeyFile;   /* Path and name of server's private key file */
		char           *SertificateFile; /* Path and name of server's sertificate file */
        unsigned char  WebChanId;
        u_short        IPPortUserWEB;
	    unsigned int   IPLocalAddress;
        unsigned int   WebServIpPort;
        unsigned       IDMessConnWeb;
        unsigned       IDMessCloseCWeb;
		unsigned long  LocalIPServer;
		unsigned int   TimeOut;
		unsigned int   UserConnectCount;
		bool           isKeepAlive;
		bool           ConnWebCloseReq;
		SSL_CTX        *CtxPtr;
		char           *ServRootDir;
		ListItsTask    *MobDevListPtr;
		READER_WORKER_INFO *ReaderWorkerPtr;
} CONNWEBSOCK;

#ifdef WIN32
DWORD WINAPI THRConnWebUser( LPVOID );
#else
void* THRConnWebUser(void *arg);
#endif
void ConnWebThreadCreate(CONNWEBSOCK *ConnWebInfoPtr);
void ConnWebClose(CONNWEBSOCK *ConnWebInfoPtr);
bool HandleNextReqKeepAlive(CONNWEBSOCK *ParConnWeb, READWEBSOCK *ParReadWeb);
void SetKeepAliveTimeout(CONNWEBSOCK *ParConnWeb, unsigned int TimeOut);
//---------------------------------------------------------------------------
#endif  /* if ! defined( ThrConnWebH ) */
