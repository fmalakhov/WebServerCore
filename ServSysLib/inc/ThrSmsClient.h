# if ! defined( ThrSmsClientH )
#	define ThrSmsClientH	/* only include me once */

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

#define MAX_SMS_PHONE_LIST           10
#define SMS_CONFIRM_KEY_LEN          6

#define MAX_LEN_SMS_SERVER_PORT      5
#define MAX_LEN_SMS_SERVER_TIMEOUT   3

#define MAX_SMS_RX_MSG_LENGTH        4096
#define MAX_RETRY_SMS_DELIVERY_WAIT  20

#define MAX_LEN_SMS_SERVER_NAME      128
#define MAX_LEN_SMS_SERVER_ACCESS_ID 64
#define MAX_LEN_SMS_BODY             140
#define MAX_LEN_SMS_PHONE_NUM        16
#define MAX_LEN_SMS_SRC_LEN          11 
#define MAX_LEN_PROXY_ADDR           512

#define SMSDL_ADD_TO_DELIVERY          100
#define SMSDL_WRONG_AP_ID              200
#define SMSDL_NO_MANY_TO_SENT          201
#define SMSDL_WRONG_MAIL_RECEIVER      202
#define SMSDL_NAME_NOT_AGREED          204
#define SMSDL_DAY_LIMIT_REACHED        206
#define SMSDL_NO_SENT_TO_RECEIVER      207

#define SMSST_MSG_NOT_FOUND            -1
#define SMSST_SMS_IN_QUEUE             100
#define SMSST_SMS_DELIV_OPER           101
#define SMSST_SMS_SENT                 102
#define SMSST_SMS_DELIVERED            103
#define SMSST_SMS_NOT_DELIV_LIFE_TIME  104
#define SMSST_SMS_DELETE_BY_OPERATOR   105
#define SMSST_SMS_NO_DELIV_PHONE_ISSU  106
#define SMSST_SMS_NO_DELIV_UNKNOWN     107
#define SMSST_SMS_NOT_DELIV_REJECT     108
#define SMSST_WRONG_APP_ID             200
#define SMSST_SERVICE_UNAVAILABE       220
#define SMSST_USER_NOT_REGISTER        302

typedef struct {
	bool         isRusLangInfo;
	char         PhoneNum[MAX_LEN_SMS_PHONE_NUM+1];
	ObjListTask  *ObjPtr;
} SENTSMSNUM;

typedef struct {
    bool           isUsedProxy; /* Flag of proxy server usage in LAN */
	unsigned int   SmsIpPort;  /* IP port of SMS server */
	unsigned int   SmsTimeout; /* Timeout for data transmit to SNS server */
    unsigned int   ProxyServPort; /* Proxy server IP port */
	char           SmsSrcName[MAX_LEN_SMS_SRC_LEN+1];
    char           SmsServerName[MAX_LEN_SMS_SERVER_NAME+1];
	char           AccessId[MAX_LEN_SMS_SERVER_ACCESS_ID+1];
    char           ProxyServAddr[MAX_LEN_PROXY_ADDR+1]; /* Proxy server address */    
	ListItsTask    SmsDestNumList;
} PARSMSCLIENT;

typedef struct {
	int            Status;
	PARSMSCLIENT   *SmsCfgPtr;
	unsigned int   CriticalAlarmsCount;
    unsigned int   MajorAlarmsCount;
    char           SmsText[MAX_LEN_SMS_BODY+1];
    char           ConfirmKey[SMS_CONFIRM_KEY_LEN+1];
	char           *UrlPtr;
	SENTSMSNUM     PhoneNum;
	unsigned       IDMessSendSmsRes;
    struct sockaddr_in st_Sockaddr;
	
#ifdef WIN32
    HANDLE	       HTRSENDSMS;
	DWORD          ParentThrID;
    DWORD          ThrSendSmsID;
	DWORD          ThrReportMgrId;
#else
    unsigned int   NotifyPort; /* IP port for notification about SMS delivery status */
    unsigned int   ReportPort; /* IP port of logging thread */
    TimerThrInfo   SendSmsInfo;
	ThrMsgChanInfo ReqThrChan;
	pthread_t	   SmsClient_thr;     /* SMS client thread */
    fd_set         master_rset, work_rset;
    int            maxfdp;
#endif
} PARSENDSMS;

#ifdef WIN32
DWORD WINAPI ThrSendSmsClient( LPVOID );
#else
void* ThrSendSmsClient(void *arg);
bool SendSmsThreadCreate(PARSENDSMS *ParSendSmsPtr);
#endif

int OpenSmsConnect(PARSENDSMS *ParSmsSend);
void SentSmsThrStatus(PARSENDSMS *ParSmsSendPtr);
bool SendDataSmsServer(PARSENDSMS *ParSmsSend, char *SendBufPtr,  int Socket);
bool SendBufSmsServer(PARSENDSMS *ParSmsSend, unsigned char *SendBufPtr,  int Socket, int BufLen);
int ReceiveDataSmsServer(PARSENDSMS *ParSmsSend, char *RecvBufPtr,  int Socket);
bool SetSmsGateHostByName(PARSENDSMS *ParSmsSend);

//---------------------------------------------------------------------------
#endif  /* if ! defined( ThrSmsClientH ) */
