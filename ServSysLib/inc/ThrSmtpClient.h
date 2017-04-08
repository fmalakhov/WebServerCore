# if ! defined( ThrSmtpClientH )
#	define ThrSmtpClientH	/* only include me once */

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

#ifndef CommonPlatformH
#include "CommonPlatform.h"
#endif

#ifndef vistypesH
#include "vistypes.h"
#endif

#ifndef SysLibToolH
#include "SysLibTool.h"
#endif

#ifndef _SUN_BUILD_
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#endif

#define MAX_LEN_SERVER_NAME     128
#define MAX_LEN_SERVER_PORT     5
#define MAX_LEN_SERVER_TIMEOUT  3
#define MAX_LEN_SERVER_EMAIL    100
#define MAX_LEN_SERVER_LOGIN    64
#define MAX_LEN_SERVER_PASSWD   40

#define SMTP_REQ_ACTION_CMPLT   250 /* Requested mail action okay, completed */
#define SMTP_SERVICE_REDY       220 /* Service ready */
#define SMTP_SERVICE_CLOSE      221 /* Service closing transmission channel */
#define SMTP_TEXT_HAS_BASE64    334 /* Text part containing the [BASE64] encoded string */
#define SMTP_AUTH_SUCCESS       235 /* Authentication Succeeded */
#define SMTP_START_EMAIL_INPUT  354 /* Start mail input; end with <CRLF>.<CRLF> */

#define MAX_SMTP_RX_MSG_LENGTH  1024
#define MAX_SMTP_TX_MSG_LENGTH  1024

#define BASE_64_ENC_LEN 3

#define SMTP_ENCODE_NONE 1
#define SMTP_ENCODE_SSL  2
#define SMTP_ENCODE_TLS  3
#define MIN_SMTP_ENCODE_TYPE  SMTP_ENCODE_NONE
#define MAX_SMTP_ENCODE_TYPE  SMTP_ENCODE_TLS

typedef struct {
    bool         MailClientUse; /* Flag of external mail client usage */
    unsigned char SmtpEncodeType; /* Type of SMTP encode */
	unsigned int SmtpIpPort;  /* IP port of SMTP server */
	unsigned int SmtpTimeout; /* Timeout for data transmit to SMTP server */
	unsigned int MailSendInt; /* Interval between mails in seconds. */
	char         LocalIpAddr[128];
    char         SmtpServerName[MAX_LEN_SERVER_NAME+1];
	char         MailFrom[MAX_LEN_SERVER_EMAIL+1];
	char         MailLogin[MAX_LEN_SERVER_LOGIN+1];
	char         MailPasswd[MAX_LEN_SERVER_PASSWD+1];
} PARMAILCLIENT;

typedef struct {
    bool          SecureConnect; 
	int           Status;
	int           Phase;
	PARMAILCLIENT *ClientCfgPtr;
	char          *MailBody;
	char          MailFrom[128];
	char          MailTo[128];
	char          MailSubject[128];
	unsigned      IDMessSendMailRes;
	
#ifdef WIN32
    HANDLE	      HTRSENDMAILSMTP;
	DWORD         ParentThrID;
    DWORD         ThrSendMailSmtpID;
	DWORD         ThrReportMgrId;
#else
    unsigned int   NotifyPort;  /* IP port of notification about mail sent result */
    unsigned int   ReportPort;  /* IP port of logging thread */
	TimerThrInfo   SendMailInfo;
	ThrMsgChanInfo ReqThrChan;
	pthread_t	   SmtpClient_thr;     /* SMTP client thread */
    fd_set         master_rset, work_rset;
    int            maxfdp;
#endif

    /* SSL related information */
#ifndef _SUN_BUILD_
    X509             *cert;
    X509_NAME        *certname;
  #ifdef _TPT_DEBIAN6_AMD64_
    SSL_METHOD       *method;
  #else
    const SSL_METHOD *method;
  #endif
    SSL_CTX          *ctx;
    SSL              *ssl;
#endif
} PARSENDMAIL;

#ifdef WIN32
DWORD WINAPI THRSendMailClient( LPVOID );
#else
void* THRSendMailClient(void *arg);
bool SendMailThreadCreate(PARSENDMAIL *ParSendMailPtr);
#endif

int MailServRetcodeCheck(PARSENDMAIL *ParMailSend, char *RecvBufPtr, int CheckCode);
void SentSmtpThrStatus(PARSENDMAIL *ParMailSendPtr);
bool SendDataSmtpServer(PARSENDMAIL *ParMailSend, char *SendBufPtr,  int Socket);
bool SendBufSmtpServer(PARSENDMAIL *ParMailSend, unsigned char *SendBufPtr,  int Socket, int BufLen);
int TcpReceiveDataSmtpServer(PARSENDMAIL *ParMailSend, char *RecvBufPtr,  int Socket);
void StrBase64Encode(char *EncodeBuf, char *SrcText);
int AnsiToUnicodeConvert(char *AnsiBufPtr, unsigned char *UniBufPtr);
bool SslSendBufSmtpServer(PARSENDMAIL *ParMailSend, unsigned char *SendBufPtr,  int BufLen);
int SslReceiveDataSmtpServer(PARSENDMAIL *ParMailSend, char *RecvBufPtr);
int ReceiveDataSmtpServer(PARSENDMAIL *ParMailSend, char *RecvBufPtr,  int Socket);

//---------------------------------------------------------------------------
#endif  /* if ! defined( ThrSmtpClientH ) */
