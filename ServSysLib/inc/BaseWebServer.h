# if ! defined( BaseWebServerH )
#	define BaseWebServerH	/* only include me once */

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

#ifndef WebServMsgApiH
#include "WebServMsgApi.h"
#endif

#ifndef RemConnMsgApiH
#include "RemConnMsgApi.h"
#endif

#ifndef ThrSmtpClientH
#include "ThrSmtpClient.h"
#endif

#ifndef ThrSmsClientH
#include "ThrSmsClient.h"
#endif

#ifndef ThrConnWebH
#include "ThrConnWeb.h"
#endif

#ifndef FileDataHashH
#include "FileDataHash.h"
#endif

#ifndef GZipH
#include "gzip.h"
#endif

#ifndef MemoryPoolH
#include "MemoryPool.h"
#endif

#ifndef SessionKeyHashH
#include "SessionKeyHash.h"
#endif

#ifndef SessionIpHashH
#include "SessionIpHash.h"
#endif

#ifndef CpuUtilizationH
#include "CpuUtilization.h"
#endif

#ifndef  MobileDeviceDataBaseH
#include "MobileDeviceDataBase.h"
#endif

#ifndef BotDataBaseH
#include "BotDataBase.h"
#endif

#ifndef WavToneLoadH
#include "WavToneLoad.h"
#endif

#ifndef GroupDataBaseH
#include "GroupDataBase.h"
#endif

#ifndef WebServInfoH
#include "WebServInfo.h"
#endif

#ifndef SiteMapStaticUrlDataBaseH
#include "SiteMapStaticUrlDataBase.h"
#endif

#ifndef  StatsDataBaseH
#include "StatsDataBase.h"
#endif

#ifndef  UserInfoDataBaseH
#include "UserInfoDataBase.h"
#endif

#ifndef BackRequestDataBaseH
#include "BackRequestDataBase.h"
#endif

#ifndef  CustomConfigDataBaseH
#include "CustomConfigDataBase.h"
#endif

#ifndef ThrReadWebUserH
#include "ThrReadWebUser.h"
#endif

#ifndef ThrSendWebUserH
#include "ThrSendWebUser.h"
#endif

#ifndef ThrSmsClientH
#include "ThrSmsClient.h"
#endif

#ifndef ThrSmtpClientH
#include "ThrSmtpClient.h"
#endif

#ifndef ReaderWorkerH
#include "ReaderWorker.h"
#endif

#ifndef SenderWorkerH
#include "SenderWorker.h"
#endif

#ifndef MailWorkerH
#include "MailWorker.h"
#endif

#ifndef SmsWorkerH
#include "SmsWorker.h"
#endif

#ifndef OpenSslLockH
#include "OpenSslLock.h"
#endif

#ifndef SessionH
#include "Session.h"
#endif

#ifndef BaseTextLineCodeH
#include "BaseTextLineCode.h"
#endif

#define WSU_RES_START_NET_CONN  1000
#ifdef WIN32
#define WEB_SERV_MSG_ID         (GetCurrentThreadId())
#else
#define WEB_SERV_MSG_ID         (ParWebServ->ServCustomCfg.WebServMsgPort)
#endif

typedef int (*TOnUserCsvFileAccessCheck)(char *FText);
typedef unsigned int (*TOnGetHtmlUserTypeMask)(unsigned int UserType);

typedef struct {
	bool		    ControlIPHost;
	bool            StopServReq;
    bool            OamServConnect;  /* Falg of connection to LM server */
    bool            EthStreamUp;     /* Flag of Eth stream activity */
    bool            AudioServConnect;/* Falg of connection to Audio server */
	bool            LogTypeListLoadDone;
	unsigned char   LastDayOfWeek;
	unsigned	    IDAnswStart;
    unsigned int    RemConPort;
	unsigned	    PrimLocalIPAddrServ;   /* The primary local IP addres for WWW server initialization; */
    unsigned	    SecondLocalIPAddrServ; /* The secondary local IP addres for WWW server initialization; */
	unsigned	    ForwardLocalIPAddrServ; /* The forward local IP addres for WWW server initialization; */
	ListItsTask	    ListJobReq;
	ListItsTask     BannerList;
	ListItsTask     LogTypeList;
    ListItsTask     ClientTypeList;
    ListItsTask     PlatformList;
	ListItsTask     SchemaList;
	unsigned int	Count_ID;
	unsigned int    SummHtmlPageGenTime; /* Summary time in ms for all HTML pages generation */
	unsigned int    HtmlPageGenCount;    /* Numbr of generated HTML pages */
	unsigned int    SummHtmlReqLoadTime; /* Summary time in ms for all HTML requests load */
	unsigned int    HtmlReqLoadCount;    /* Numbr of loaded HTML requests */
	GENPARSERVER    GeneralCfg;
	PARSHOPINFO     ShopInfoCfg;
	SERVER_CUSTOM_CONFIG ServCustomCfg;
    POOL_RECORD_BASE RemConnPool;
    READER_WORKER_INFO ReaderWorker;
    SENDER_WORKER_INFO SenderWorker;
    MAIL_WORKER_INFO   MailWorker;
    SMS_WORKER_INFO    SmsWorker;
	SESSION_MANAGER    SessionManager;
	unsigned int    UserBackReqCount;
	unsigned int    StatsIncCnt;
	unsigned int    DataReadyCount;
	unsigned int    SentRespUser;
	unsigned int    SentRespDoneCount;
    bool            ActivServer;
    CONNWEBSOCK     PrimConnWeb;
    CONNWEBSOCK     SecondConnWeb;                
	CONNWEBSOCK     ForwardConnWeb;
#ifdef WIN32
    DWORD		    ThrAnswStart;
    DWORD           ThrCernelID;
	DWORD           ThrReportMgrId;
#else
	unsigned int    NextSendNotifyPort;
	bool            isStartDone;
	TimerThrInfo    WebServInfo;
	ThrMsgChanInfo  ReqThrChan;
    ThrMsgChanInfo  ReportThrChan;
    in_port_t       ReqMsgPort;
    in_addr_t       ReqMsgAddr;
#endif
	WAV_SAMPLE_BLOCK VselpEot;
	WAV_SAMPLE_BLOCK AmrWbEot;
	TOnUserCsvFileAccessCheck OnUserCsvFileAccessCheck;
	TOnGetHtmlUserTypeMask OnGetHtmlUserTypeMask;
	unsigned int    CapchaTableSize;
	unsigned int    CapchaTable[MAX_CAPCHA_SIZE+1];
	char        LogTimeStampStr[64];
	char        ReqTimeStr[128];
	char        ExpDateCookieStr[128];
	char        UserTitle[512];
	char        *UserMetaData;
	char		LocalAddrIP[24];
	char        PrimLocalHostName[MAX_LEN_URL_SERVER+1];
    char        SecondLocalHostName[MAX_LEN_URL_SERVER+1];
	char        ForwardLocalHostName[MAX_LEN_URL_SERVER+1];
    char		StartPath[512];
	char        ServerStartTime[128];
	char        BrowserCacheExpTime[128];
    char        SysStatusData[4096];
} PARAMWEBSERV;

void StatsDataInit();
void StatsDataSave();
void HandleReadBadWebReq(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb);
void LoadCapcha(PARAMWEBSERV *ParWebServ);
unsigned int SetExpectSessionCapchaCode(PARAMWEBSERV *ParWebServ);
bool UserCapchaCodeValidate(unsigned int ExpectedCapchaCode, unsigned int EnterCapchaCode);
void SetServerHttpAddr(char *BufAnsw);
char* DataFileHttpSentHeaderGen(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, char *BufPtr, unsigned int SizeFile, 
    unsigned char FileType, unsigned int ETagId, bool isKeepAlive, bool isCompressBody);
bool SendHttpHashFileHost(PARAMWEBSERV *ParWebServ, READWEBSOCK *ReadWeb, FILE_HASH_RECORD *HashFilePtr);
void SetAddrConnectChan(PARAMWEBSERV *ParWebServ);
void ConnChanInfoInit(PARAMWEBSERV *ParWebServ);
bool WebConnChanOpen(PARAMWEBSERV *ParWebServ);
void WebConnChanClose(PARAMWEBSERV *ParWebServ);
bool MainWebReqHandler(PARAMWEBSERV *ParWebServ, unsigned int SysShowHostIP);
void HandlerWebServerReq(PARAMWEBSERV *WebServParPtr, InterThreadInfoMsg* WebServMsgPtr);
bool HandlerStopServerReq(PARAMWEBSERV *ParWebServ, InterThreadInfoMsg* WebServMsgPtr);
bool SendHttpTextDataFileHost(PARAMWEBSERV *ParWebServ, READWEBSOCK *ReadWeb,
    char *HttpRespPtr, unsigned int DataLen, FILE_HASH_RECORD *HashFilePtr, bool isZipBody, char *FileName);
void SendFileNotModifyedTime(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, time_t LastModifyed, unsigned int ETagId);
void SendFileNotModifyedStr(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, char *LastModifyed, unsigned int ETagId);
bool SendHttpHtmlPageHost(PARAMWEBSERV *ParWebServ, READWEBSOCK *ReadWeb, USER_SESSION *SessionPtr,
						  char *HttpRespPtr, unsigned int DataLen, bool isZipBody, char *FileName);
bool CsvFileAccessValidate(PARAMWEBSERV *ParWebServPtr, char *BufAnsw, char *HttpCmd);
bool PdfFileAccessValidate(PARAMWEBSERV *ParWebServPtr, char *BufAnsw, char *HttpCmd);
void HandleReaderDataInd(PARAMWEBSERV *ParWebServ, InterThreadInfoMsg* WebServMsgPtr, unsigned int SysShowHostIP);
void AdminBadIpDataBase(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void AdminLogManager(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void HandlePageSpaseResize(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void HandleDynConfKeyReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void ShowContactsShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
unsigned GetLocalIPServer(char *SrcCmd , char *Param, char *LocalHostName);
void HandleeTimeStampSetTimerExp(PARAMWEBSERV *WebServInfoPtr);
unsigned int GetRealTimeMarker();
void HandleStatsCheckTimerExp(PARAMWEBSERV *ParWebServ);

// ItemsListNavigation
void SetNavPageItemListShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr,
	unsigned int ItemsPageId, unsigned int StartItemId, unsigned int ItemsInList,
	unsigned int SectionId, char *HtmlPageName);
void AddNumItemsPerFilterPageSelectToHtml(char *BufAnsw, unsigned int SessionId,
	unsigned int ItemsPageId, unsigned int SectionId, char *HtmlPageName);
void SetFirstPageNavDsButton();
void SetFirstPageNavEnButton();
void SetPrevPageNavDsButton();
void SetPrevPageNavEnButton();
void SetNextPageNavDsButton();
void SetNextPageNavEnButton();
void SetLastPageNavDsButton();
void SetLastPageNavEnButton();
void SetReturnNavDsButton();
void SetReturnNavEnButton();

// Common WEB sunctions
void SetSessionIdCmdRef(USER_SESSION *SessionPtr);
void ShowServerInfoShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void AddComMainMenuItem(char *HtmlPageName, unsigned int NameId, USER_SESSION *SessionPtr, unsigned int tag);
void AddSecMainMenuItem(char *HtmlPageName, unsigned int NameId, USER_SESSION *SessionPtr, unsigned int tag);
void AddSecExtMainMenuItem(char *HtmlPageName, unsigned int NameId, unsigned int tag);
void AddRegUserShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void UserRegisterNoKey();
void UserRegisterWithKey();
void UserAuthEncodeDataReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
bool UserAuthDecode(USER_SESSION *SessionPtr, char *EncData, char *LoginPtr, char *PasswdPtr);
void HandleDynAuthConfKeyReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void SetRegUserExitBtTimeWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void SetDateSessionTimeWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void SetActUsersInfoWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void ServLangSelect(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void LanguageSelectBlockSet(USER_SESSION *SessionPtr);
void SetNumActiveUsersPage();
void SpaseResizeBlockSet(USER_SESSION *SessionPtr);
void MainSystemShowWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void MainPageSetShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void AddGeneralInfoMainPageShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void AddBeginPageShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void AddEndPageShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void HandleDynStatusShowSessIdReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void HandleSendWorkerDoneInd(PARAMWEBSERV *ParWebServ, InterThreadInfoMsg* WebServMsgPtr);
void CreateResponse(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, SOCKET UserSocket, USER_SESSION *SessionPtr, 
	char *BufAnsw, char *LocalAdress, unsigned long CurrRemAddr, int LocalPortServ, HashDirTypeT DirType);
void CreateContentResponse(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, SOCKET UserSocket,
	char *BufAnsw, char *LocalAdress, unsigned long CurrRemAddr, int LocalPortServ, HashDirTypeT DirType);
void SetRightLocBannerListWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void SetDownLocBannerListWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void SpaseResizeBlockSet(USER_SESSION *SessionPtr);
void SystemVersionSet();
void HandleDynServDateTimeReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
char* GetSystemOvnerCompany();

#ifdef WIN32
DWORD WINAPI THRWebServer(LPVOID);
#else
void* THRWebServer(void *arg);
void WebServThreadCreate(PARAMWEBSERV *WebServInfoPtr, unsigned int TimerExpMsgId);
void WebServThreadClose(PARAMWEBSERV *WebServInfoPtr);
#endif
//---------------------------------------------------------------------------
#endif  /* if ! defined( BaseWebServerH ) */
