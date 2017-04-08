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
#include <dirent.h>
#include <sys/syscall.h>

#include "CommonPlatform.h"
#include "ThrWebServer.h"
#include "ThrCernel.h"
#include "SysWebFunction.h"
#include "ThrConnWeb.h"
#include "SysMessages.h"
#include "TrTimeOutThread.h"
#include "HttpPageGen.h"
#include "ServerPorts.h"
#include "SessionIpHash.h"
#include "BadIpHash.h"
#include "ImageNameHash.h"
#include "HtmlPageHash.h"
#include "HtmlMacrosHash.h"
#include "ThrWebMgmt.h"
#include "HtmlTemplateParser.h"
#include "FileNameMapBase.h"

static void HandleSetSysCompany(void *ParPtr);
static void HandleSetWebOwnerMail(void *ParPtr);

ListItsTask  UserSessionList;
ListItsTask  UserInfoList;

extern ListItsTask  MobileDeviceList;
extern char ServerVersion[];
extern char *EndHtmlPageGenPtr;
extern SAMPLE_SERVER_CUSTOM_CONFIG SampleCustomCfg;
extern WebServChannel  gWebServMsgChannel;

bool gIsSiteMapGenNeeds = false;
bool gIsUserDbSaveNeeds = false;
unsigned int NewOrderId = 1;
char *MemWebPageGenPtr = NULL;

STATS_INFO   ServerStats;
USER_DB_INFO SampleUserDbIfo;

READWEBSOCK *ParReadHttpSocketPtr = NULL;
PARAMWEBSERV *ParWebServPtr = NULL;

/* Flag of not found file from web page handler */
bool gFileNotFoundFlag = false;

/* Type of used lahguage during HTML page generation */
unsigned char gLanguageType = LGT_ENGLISH;

extern char *TablDefNameDey[];
extern char KeySessionId[];
extern char KeyFormSectionId[];
extern char KeyFormItemId[];
extern char KeyFormBrandId[];
extern char KeyFormSessionId[];
extern char SecKeyId[];

extern unsigned int gCloseCernelFlag;

extern FILE_HASH_CHAR_HOP FileHashHop;
extern SESSION_IP_HASH_OCTET_HOP SessionIpHashHop;
extern unsigned int HashEntityCount;
extern unsigned int RecInHashCount;
extern unsigned int SessIpHashEntityCount;
extern unsigned int SessIpRecInHashCount;
extern char AnsiToHtmlRusConver[];

unsigned char gMemoryUtil = 0; /* Current memory utilization on host */
char WebServerName[33];

#ifdef _SERVER_PERF_MEASURE_
unsigned int AvearageSummTime = 0;
unsigned int MaxTime = 0;
unsigned int SummCount = 0;
#endif

CMD_INFO HtmlCmdList[] = {
    "SETBEGINMAINPAGE", HandleSetBeginMainPage,
    "SETENDMAINPAGE",   HandleSetEndMainPage,
    "SERVERNAME",       HandleSetServerName,
	"SETSYSNAME",       HandleSetSysName,    
    "SETSYSCOMPANY",    HandleSetSysCompany,
	"SETSINGSESSIONID", HandleSetSingleSessionId,
    "SETSESSIONKEY",    HandleSetSessionKey,
    "SETSESSIONID",     HandleSetSessionId,
	"SETTITLEINFO",     HandleSetTitleInfo,
	"SETMETADATA",      HandleSetMetaData,
	"SETORIGTEXTPAGE",  HandleSetOriginalTextPage,
	"SETFIRSTPAGEDSBT", HandleSetFirstPageDsButtton,
	"SETFIRSTPAGEENBT", HandleSetFirstPageEnButtton,
	"SETPREVPAGEDSBT",  HandleSetPrevPageDsButtton,
    "SETPREVPAGEENBT",  HandleSetPrevPageEnButtton,
    "SETNEXTPAGEDSBT",  HandleSetNextPageDsButtton,
    "SETNEXTPAGEENBT",  HandleSetNextPageEnButtton,
    "SETLASTPAGEDSBT",  HandleSetLastPageDsButtton,
    "SETLASTPAGEENBT",  HandleSetLastPageEnButtton,
	"SETRETPGDSBT",     HandleSetReturnDsButtton,
	"SETRETPGENBT",     HandleSetReturnEnButtton,
	"SETSERVERVERSION", HandleSetServerVersion,
	"SETMENUGRPBGCOLOR",  HandleSetMenuGrpBgColor,
	"SETGRPTITLEBGCOLOR", HandleSetGrpTitleBgColor,
	"SETGRPCONTBGCOLOR",  HandleSetGrpContBgColor,
	"SETMENUZONEBGCOLOR", HandleSetMenuZoneBgColor,
	"SETGRPBORDERBGCOLOR",HandleSetGrpBorderColor,
	"SETWORKZONEBGCOLOR", HandleSetWorkZoneBgColor,
	"SETIMGBGSIDECOLUMN", HandleSetImgBgSideColumn,
	"SETIMGNAVBAR",       HandleSetImgNavBar,
    "SETIMGBGWORKZONE",   HandleSetImgBgWorkZone,
	"SETIMGBGSITEGROUND", HandleSetImgBgSiteGround,
    "SETIMGPRODUCTLOGO",  HandleSetImgProductLogo,
	"SETUSERSESSTIMEOUT", HandleSetUserSessionTimeout,
    "SETBASETEXTBYID",    HandleSetBaseTextById,
    "SETNAVBUTTONZONE",   HandleSetNavButtonZone,
    "SETWEBOWNERMAIL",    HandleSetWebOwnerMail
};

#define HtmlCmdListLen  sizeof(HtmlCmdList)/sizeof(CMD_INFO)


CMD_INFO HtmlCmdInLineList[] = {
    "SERVERNAME",       HandleSetServerName,
    "SETSYSNAME",       HandleSetSysName,
	"SETSINGSESSIONID", HandleSetSingleSessionId,
    "SETSESSIONID",     HandleSetSessionId,
	"SETTITLEINFO",     HandleSetTitleInfo,
	"SETSERVERVERSION", HandleSetServerVersion,
    "SETBASETEXTBYID",  HandleSetBaseTextById
};

#define HtmlCmdInLineListLen  sizeof(HtmlCmdInLineList)/sizeof(CMD_INFO)

#define HTTP_PAGE_CREATE_BUF_SIZE 16384000
//---------------------------------------------------------------------------
DEF_HTML_HANDLER HttpAnonymGetReqList[] = {
GenPageMain,                MainPageSetShopWebPage,
GenPageAboutServer,         ShowServerInfoShopWebPage,
GenPageMyContacts,          UserContactManage,
GenPageContacts,            ShowContactsShopWebPage,
GenPageUserRegister,        UserRegisterShopWebPage,
GenPageLostPasswd,          LostPasswdShopWebPage,
GenPageUserContactInfoSet,  UserContactSet
};

DEF_HTML_HANDLER HttpClientGetReqList[] = {
GenPageMain,                MainPageSetShopWebPage,
GenPageAboutServer,         ShowServerInfoShopWebPage,
GenPageMyContacts,          UserContactManage,
GenPageContacts,            ShowContactsShopWebPage,
GenPageUserContactInfoSet,  UserContactSet
};

DEF_HTML_HANDLER HttpAdmGetReqList[] = {
GenPageMain,                MainPageSetShopWebPage,
GenPageAboutServer,         ShowServerInfoShopWebPage,
GenPageMyContacts,          UserContactManage,
GenPageContacts,            ShowContactsShopWebPage,
GenPageUserRegister,        UserRegisterShopWebPage,
GenPageLostPasswd,          LostPasswdShopWebPage,
GenPageUserContactInfoSet,  UserContactSet,
GenPageRegClientsList,      RegClientsList,
GenPageServerConfig,        ServerConfigWebPage, /* GET & POST */
GenPageServStats,           AdminServStatsWebPage,
GenPageActSessList,         ActiveSessionsList,
GenPageAdmPswdChg,          AdminPasswdChange,
GenPageBadIpDBManage,       AdminBadIpDataBase,
GenPageDelUser,             DelUserRegShopWebPage,
GenPageChgUserType,         ChgUserTypeShopWebPage,
GenPageDynConfKeyReq,       HandleDynConfKeyReq,
GenPageGroupDBManage,       GroupDataBase,    /* GET & POST */
GenPageCghGroupDbManage,    ChgViewGroupDataBase,
GenPageChgGrpUser,          ChgUserHostAccessGrpWebPage
};

DEF_HTML_HANDLER HttpStatusShowGetReqList[] = {
GenPageMain,                MainSystemShowWebPage,
GenPageDynServDateTime,     HandleDynServDateTimeReq
};

HTML_HANDLER_ARRAY HttpGetReqArray[] = {
	sizeof(HttpAnonymGetReqList)/sizeof(DEF_HTML_HANDLER), &HttpAnonymGetReqList[0],
	sizeof(HttpClientGetReqList)/sizeof(DEF_HTML_HANDLER), &HttpClientGetReqList[0],
    sizeof(HttpAdmGetReqList)/sizeof(DEF_HTML_HANDLER), &HttpAdmGetReqList[0],
    sizeof(HttpStatusShowGetReqList)/sizeof(DEF_HTML_HANDLER), &HttpStatusShowGetReqList[0] };

GROUP_HTML_HANDLERS GrupHttpGetArray = {sizeof(HttpGetReqArray)/sizeof(HTML_HANDLER_ARRAY), &HttpGetReqArray[0]};


DEF_HTML_HANDLER HttpAnonimPostReqList[] = {
GenPageUserRegRequest,      NewUserRegister,
GenPageUserAuthReq,         ExistUserAuth, /* GET & POST */
GenPageUserExitReq,         ExistUserExit,
GenPagePasswdSentMail,      PasswdSentMail,
GenPageUserContactInfoSet,  UserContactSet,
GenPageLangSelect,          ServLangSelect,
GenPageSpaseResize,         HandlePageSpaseResize,
GenPageDynEncodeDataReq,    UserAuthEncodeDataReq,
GenPageDynAuthConfKeyReq,   HandleDynAuthConfKeyReq
};

DEF_HTML_HANDLER HttpClientPostReqList[] = {
GenPageUserExitReq,         ExistUserExit,
GenPageUserContactInfoSet,  UserContactSet,
GenPageLangSelect,          ServLangSelect,
GenPageSpaseResize,         HandlePageSpaseResize
};

DEF_HTML_HANDLER HttpAdmPostReqList[] = {
GenPageUserRegRequest,      NewUserRegister,
GenPageUserAuthReq,         ExistUserAuth,
GenPageUserExitReq,         ExistUserExit,
GenPagePasswdSentMail,      PasswdSentMail,
GenPageUserContactInfoSet,  UserContactSet,
GenPageAdmPswdChg,          AdminPasswdChange,   /* GET & POST */
GenPageServerConfig,        ServerConfigWebPage,
GenPageBadIpDBManage,       AdminBadIpDataBase,    /* GET & POST */
GenPageLangSelect,          ServLangSelect,
GenPageSpaseResize,         HandlePageSpaseResize,
GenPageGroupDBManage,       GroupDataBase,    /* GET & POST */
GenPageChgGrpSet,           ChgUserGroupSetWebPage
};

DEF_HTML_HANDLER HttpStatusShowPostReqList[] = {
GenPageUserExitReq,         ExistUserExit,
GenPageLangSelect,          ServLangSelect,
GenPageDynStatusShowSessIdReq, HandleDynStatusShowSessIdReq
};

HTML_HANDLER_ARRAY HttpPostReqArray[] = {
	sizeof(HttpAnonimPostReqList)/sizeof(DEF_HTML_HANDLER), &HttpAnonimPostReqList[0],
	sizeof(HttpClientPostReqList)/sizeof(DEF_HTML_HANDLER), &HttpClientPostReqList[0],
    sizeof(HttpAdmPostReqList)/sizeof(DEF_HTML_HANDLER), &HttpAdmPostReqList[0],
    sizeof(HttpStatusShowPostReqList)/sizeof(DEF_HTML_HANDLER), &HttpStatusShowPostReqList[0] };

GROUP_HTML_HANDLERS GrupHttpPostArray = {sizeof(HttpPostReqArray)/sizeof(HTML_HANDLER_ARRAY), &HttpPostReqArray[0]};

HTML_PAGE_HASH_CHAR_HOP GetHtmlPageHashHop;
HTML_PAGE_HASH_CHAR_HOP PostHtmlPageHashHop;

char ThrWebServName[]  = "ThrWebServ";
char SystemName[]      = "Sample";
char OwnerCompany[]    = "An company";
char WebOvnerMail[]    = "an.user@mail.com";

unsigned int LastPageGenTime[4];
unsigned int LastReqLoadTime[4];

char *HashDirArray[] = {
    "/WebData/images",
    "/WebData/scripts",
	"/WebData/html_data",
	"/WebData",
	"/html_template",
	"/CapchaImage"
};

const unsigned int HashDirArraySize = sizeof(HashDirArray)/sizeof(char*);

unsigned char UserBufEncKey[] = {
0x61, 0xC0, 0x6F, 0x75, 0x57, 0xF9, 0x4C, 0xEA, 0x2F, 0xCB, 0xBC, 0xAD, 0xF4, 0xFD, 0xE5, 0x40,
0x7A, 0xC4, 0x52, 0xFE, 0x1D, 0xEE, 0xE1, 0x8B, 0x70, 0x5C, 0x28, 0x65, 0x35, 0x99, 0xCF, 0x9C,
0x60, 0x34, 0x0B, 0xB1, 0x37, 0x5E, 0xA5, 0x5C, 0x23, 0x5B, 0x03, 0x11, 0x5E, 0xEF, 0x58, 0xD2,
0xAD, 0xB4, 0xCB, 0xC0, 0x9D, 0xB2, 0x52, 0x17, 0x08, 0x84, 0x72, 0x47, 0x27, 0x3B, 0xDA, 0x81,
0x76, 0xEF, 0x38, 0xA7, 0x47, 0xE7, 0xFA, 0x61, 0x38, 0x07, 0x78, 0x91, 0xEC, 0xDA, 0x6D, 0x90,
0x89, 0x3E, 0x5A, 0x2C, 0xEA, 0xB6, 0x3D, 0xFD, 0x34, 0xA9, 0x3A, 0x52, 0xEA, 0x1E, 0xDD, 0x5A,
0x04, 0x1C, 0xF8, 0x45, 0xFD, 0xFC, 0xAC, 0x3C, 0xFD, 0x2F, 0xD7, 0xE0, 0x03, 0x3A, 0x7A, 0x92,
0x73, 0xDE, 0xB8, 0x67, 0x8F, 0xFF, 0x5A, 0xBD, 0xAE, 0x9F, 0x09, 0x93, 0xB7, 0xED, 0xF7, 0xB5,
0x03, 0xE9, 0xF1, 0x06, 0xEF, 0xA7, 0x3C, 0xE3, 0xCC, 0x0D, 0xCD, 0xD6, 0x4E, 0x41, 0x62, 0xBB,
0x19, 0x25, 0x18, 0xAE, 0x1A, 0x7D, 0x62, 0xC3, 0x12, 0x75, 0x5C, 0xC3, 0x58, 0x4D, 0x7F, 0x65,
0x3D, 0x76, 0x62, 0x22, 0x17, 0x98, 0x0F, 0xDA, 0xAC, 0xD2, 0xAA, 0xF4, 0x1D, 0x16, 0xA9, 0x3D,
0x35, 0xCB, 0xE5, 0x56, 0x4E, 0x41, 0x13, 0x5A, 0xBD, 0x69, 0x24, 0x1F, 0xBC, 0x9D, 0x7B, 0xF3,
0x0D, 0xE7, 0x1C, 0x1A, 0x79, 0x25, 0xFE, 0x2F, 0xFE, 0xA2, 0x1D, 0x15, 0xB3, 0xCC, 0x48, 0xEE,
0x9E, 0x34, 0x3E, 0xE6, 0x7F, 0x4B, 0x3B, 0x32, 0xBA, 0x59, 0x48, 0x71, 0xFC, 0xCD, 0x6A, 0x03,
0xAA, 0x80, 0x27, 0x2D, 0xAC, 0x1C, 0x53, 0xA4, 0xB8, 0x76, 0xB3, 0x75, 0x3D, 0x02, 0x5A, 0xD5,
0x30, 0x92, 0xB5, 0xA9, 0xE4, 0xF6, 0xE2, 0x98, 0x49, 0x34, 0x13, 0x4F, 0xFB, 0x84, 0x48, 0xAF,
0x0E, 0x76, 0xD2, 0xB4, 0x8C, 0x2F, 0x52, 0x4E, 0x9C, 0x0C, 0xBA, 0xD3, 0x08, 0x1E, 0xAE, 0x42,
0xAA, 0x5D, 0xF5, 0x88, 0x4A, 0xD1, 0x2B, 0x9D, 0x0F, 0x44, 0xE3, 0x00, 0xC2, 0x35, 0xA9, 0xCB,
0xA5, 0x82, 0x79, 0x2B, 0xAB, 0xD2, 0x70, 0x41, 0xD8, 0x34, 0x1A, 0xEA, 0x4C, 0xC2, 0x36, 0xF0,
0x26, 0x25, 0x83, 0x6A, 0xFD, 0xB4, 0x01, 0x02, 0xF2, 0xEA, 0x0D, 0xBF, 0x26, 0xBC, 0x80, 0xC5,
0x38, 0x03, 0xF7, 0xEA, 0xDF, 0x61, 0x35, 0xB1, 0x9F, 0x56, 0xA5, 0xE5, 0x12, 0xD5, 0xDF, 0x32,
0xF1, 0x58, 0xA6, 0xF4, 0x16, 0xAE, 0xF0, 0x03, 0x92, 0x07, 0xB8, 0xB2, 0xBA, 0x42, 0x7E, 0xFC,
0x4F, 0x6F, 0xE0, 0x25, 0xD6, 0x1C, 0xDC, 0x6F, 0x6C, 0x78, 0x4A, 0x78, 0x57, 0x23, 0xB5, 0x4E,
0x82, 0x55, 0x3C, 0x92, 0xFD, 0x37, 0x9F, 0x96, 0x34, 0x51, 0x42, 0xE8, 0x9D, 0xBA, 0xEF, 0xE3,
0x23, 0xC9, 0x0E, 0xF3, 0xEF, 0xE4, 0x58, 0x55, 0x66, 0xAC, 0xD4, 0xB4, 0xD6, 0x83, 0xFC, 0x52,
0xDE, 0x33, 0xEE, 0xD2, 0x60, 0x84, 0x62, 0x9E, 0xDF, 0xAE, 0x81, 0x73, 0x63, 0x76, 0x5C, 0x8C,
0x39, 0x64, 0x86, 0x2F, 0x42, 0xD8, 0x7A, 0xA3, 0x8F, 0x48, 0x51, 0x5F, 0xD5, 0x57, 0xAB, 0xAA,
0x80, 0x93, 0x86, 0xEA, 0x11, 0xE2, 0x83, 0xF7, 0x8A, 0x0A, 0x60, 0xF7, 0x7A, 0xC6, 0x7A, 0xBD,
0x24, 0xFA, 0xE2, 0x60, 0xDC, 0x67, 0x0D, 0x65, 0xA9, 0x64, 0xBA, 0x85, 0xB2, 0x6F, 0x29, 0x3C,
0xF9, 0xA9, 0x21, 0x10, 0x95, 0xAA, 0x01, 0x19, 0xBE, 0x6B, 0x17, 0x32, 0x2B, 0x8B, 0xE9, 0x49,
0x8F, 0xD2, 0xB0, 0x65, 0x33, 0xC7, 0xC1, 0xE2, 0x22, 0x85, 0x61, 0xDE, 0xEB, 0x90, 0x14, 0xEA,
0x43, 0x3F, 0x04, 0xDE, 0xE3, 0x0C, 0xFE, 0x9B, 0x71, 0x0F, 0xD7, 0xA3, 0x90, 0xC7, 0xF2, 0x19
};

unsigned int EncKeyLen = sizeof(UserBufEncKey)/sizeof(unsigned char);

unsigned char UserInfoEncodeKey[] = {
0xFE, 0x5F, 0xF0, 0xEA, 0xC8, 0x66, 0xD3, 0x75, 0xB0, 0x54, 0x23, 0x32, 0x6B, 0x62, 0x7A, 0xDF,
0xE5, 0x5B, 0xCD, 0x61, 0x82, 0x71, 0x7E, 0x14, 0xEF, 0xC3, 0xB7, 0xFA, 0xAA, 0x06, 0x50, 0x03,
0xFF, 0xAB, 0x94, 0x2E, 0xA8, 0xC1, 0x3A, 0xC3, 0xBC, 0xC4, 0x9C, 0x8E, 0xC1, 0x70, 0xC7, 0x4D,
0x32, 0x2B, 0x54, 0x5F, 0x02, 0x2D, 0xCD, 0x88, 0x97, 0x1B, 0xED, 0xD8, 0xB8, 0xA4, 0x45, 0x1E,
0xE9, 0x70, 0xA7, 0x38, 0xD8, 0x78, 0x65, 0xFE, 0xA7, 0x98, 0xE7, 0x0E, 0x73, 0x45, 0xF2, 0x0F,
0x16, 0xA1, 0xC5, 0xB3, 0x75, 0x29, 0xA2, 0x62, 0xAB, 0x36, 0xA5, 0xCD, 0x75, 0x81, 0x42, 0xC5,
0x9B, 0x83, 0x67, 0xDA, 0x62, 0x63, 0x33, 0xA3, 0x62, 0xB0, 0x48, 0x7F, 0x9C, 0xA5, 0xE5, 0x0D,
0xEC, 0x41, 0x27, 0xF8, 0x10, 0x60, 0xC5, 0x22, 0x31, 0x00, 0x96, 0x0C, 0x28, 0x72, 0x68, 0x2A,
0x9C, 0x76, 0x6E, 0x99, 0x70, 0x38, 0xA3, 0x7C, 0x53, 0x92, 0x52, 0x49, 0xD1, 0xDE, 0xFD, 0x24,
0x86, 0xBA, 0x87, 0x31, 0x85, 0xE2, 0xFD, 0x5C, 0x8D, 0xEA, 0xC3, 0x5C, 0xC7, 0xD2, 0xE0, 0xFA,
0xA2, 0xE9, 0xFD, 0xBD, 0x88, 0x07, 0x90, 0x45, 0x33, 0x4D, 0x35, 0x6B, 0x82, 0x89, 0x36, 0xA2,
0xAA, 0x54, 0x7A, 0xC9, 0xD1, 0xDE, 0x8C, 0xC5, 0x22, 0xF6, 0xBB, 0x80, 0x23, 0x02, 0xE4, 0x6C,
0x92, 0x78, 0x83, 0x85, 0xE6, 0xBA, 0x61, 0xB0, 0x61, 0x3D, 0x82, 0x8A, 0x2C, 0x53, 0xD7, 0x71,
0x01, 0xAB, 0xA1, 0x79, 0xE0, 0xD4, 0xA4, 0xAD, 0x25, 0xC6, 0xD7, 0xEE, 0x63, 0x52, 0xF5, 0x9C,
0x35, 0x1F, 0xB8, 0xB2, 0x33, 0x83, 0xCC, 0x3B, 0x27, 0xE9, 0x2C, 0xEA, 0xA2, 0x9D, 0xC5, 0x4A,
0xAF, 0x0D, 0x2A, 0x36, 0x7B, 0x69, 0x7D, 0x07, 0xD6, 0xAB, 0x8C, 0xD0, 0x64, 0x1B, 0xD7, 0x30,
0x91, 0xE9, 0x4D, 0x2B, 0x13, 0xB0, 0xCD, 0xD1, 0x03, 0x93, 0x25, 0x4C, 0x97, 0x81, 0x31, 0xDD,
0x35, 0xC2, 0x6A, 0x17, 0xD5, 0x4E, 0xB4, 0x02, 0x90, 0xDB, 0x7C, 0x9F, 0x5D, 0xAA, 0x36, 0x54,
0x3A, 0x1D, 0xE6, 0xB4, 0x34, 0x4D, 0xEF, 0xDE, 0x47, 0xAB, 0x85, 0x75, 0xD3, 0x5D, 0xA9, 0x6F,
0xB9, 0xBA, 0x1C, 0xF5, 0x62, 0x2B, 0x9E, 0x9D, 0x6D, 0x75, 0x92, 0x20, 0xB9, 0x23, 0x1F, 0x5A,
0xA7, 0x9C, 0x68, 0x75, 0x40, 0xFE, 0xAA, 0x2E, 0x00, 0xC9, 0x3A, 0x7A, 0x8D, 0x4A, 0x40, 0xAD,
0x6E, 0xC7, 0x39, 0x6B, 0x89, 0x31, 0x6F, 0x9C, 0x0D, 0x98, 0x27, 0x2D, 0x25, 0xDD, 0xE1, 0x63,
0xD0, 0xF0, 0x7F, 0xBA, 0x49, 0x83, 0x43, 0xF0, 0xF3, 0xE7, 0xD5, 0xE7, 0xC8, 0xBC, 0x2A, 0xD1,
0x1D, 0xCA, 0xA3, 0x0D, 0x62, 0xA8, 0x00, 0x09, 0xAB, 0xCE, 0xDD, 0x77, 0x02, 0x25, 0x70, 0x7C,
0xBC, 0x56, 0x91, 0x6C, 0x70, 0x7B, 0xC7, 0xCA, 0xF9, 0x33, 0x4B, 0x2B, 0x49, 0x1C, 0x63, 0xCD,
0x41, 0xAC, 0x71, 0x4D, 0xFF, 0x1B, 0xFD, 0x01, 0x40, 0x31, 0x1E, 0xEC, 0xFC, 0xE9, 0xC3, 0x13,
0xA6, 0xFB, 0x19, 0xB0, 0xDD, 0x47, 0xE5, 0x3C, 0x10, 0xD7, 0xCE, 0xC0, 0x4A, 0xC8, 0x34, 0x35,
0x1F, 0x0C, 0x19, 0x75, 0x8E, 0x7D, 0x1C, 0x68, 0x15, 0x95, 0xFF, 0x68, 0xE5, 0x59, 0xE5, 0x22,
0xBB, 0x65, 0x7D, 0xFF, 0x43, 0xF8, 0x92, 0xFA, 0x36, 0xFB, 0x25, 0x1A, 0x2D, 0xF0, 0xB6, 0xA3,
0x66, 0x36, 0xBE, 0x8F, 0x0A, 0x35, 0x9E, 0x86, 0x21, 0xF4, 0x88, 0xAD, 0xB4, 0x14, 0x76, 0xD6,
0x10, 0x4D, 0x2F, 0xFA, 0xAC, 0x58, 0x5E, 0x7D, 0xBD, 0x1A, 0xFE, 0x41, 0x74, 0x0F, 0x8B, 0x75,
0xDC, 0xA0, 0x9B, 0x41, 0x7C, 0x93, 0x61, 0x04, 0xEE, 0x90, 0x48, 0x3C, 0x0F, 0x58, 0x6D, 0x86
};
	
unsigned int UserInfoEncodeKeyLen = sizeof(UserInfoEncodeKey)/sizeof(unsigned char);

/*---------------------------------- MACROS -----------------------------------*/
/*--------------------------- FUNCTION DEFINITION -----------------------------*/
void ServerBasesClose(PARAMWEBSERV *ParWebServ);
static void HandleOnAddPageSend(void *DataPtr);
static void HandleOnMailSendStatus(bool Status);
static void HandleOnSmsSendStatus(bool Status);

static void OnCreateExtDbRecCB(void *ExtUserInfoPtr);
static unsigned int OnLoadVerExtDbRecCB(unsigned char LoadVersion);
static unsigned char* OnReadExtDbRecCB(void *ExtUserInfoPtr, unsigned char LoadVersion, unsigned char *DataPtr);
static unsigned char* OnSaveExtDbRecCB(void *ExtUserInfoPtr, unsigned char *DataPtr);
static void OnUserSessionOpenCB(void *UserSessionPtr);
static void OnUserSessionCloseCB(USER_SESSION *UserSessionPtr, void *DataPtr);
static unsigned int OnGetSessionTimeoutCB(unsigned int UserType, void *DataPtr);
static void OnSessionTimerStartCB(unsigned int TimerId, unsigned int Delay, void *DataPtr);
static void OnSessionTimerResetCB(unsigned int TimerId, void *DataPtr);
static void OnSessionGrpTimersRestartCB(unsigned int *TimerRestartReqArrayPtr, void *DataPtr);
static unsigned int OnGetHtmlUserTypeMask(unsigned int UserType);
/*-------------------------- LOCAL VARIABLES ----------------------------------*/
/*------------------------------- CODE ----------------------------------------*/
void* THRWebServer(void *Data)
{
    PARAMWEBSERV	*ParWebServ;
    struct timeb    hires_cur_time;
    struct tm       *cur_time;
    WebServMessage  *WebServMsgPtr = NULL;
	char       *CwdRet = NULL;
    char       LogBuf[64];

	ParWebServ = (PARAMWEBSERV*)Data;
	ParWebServ->StatsIncCnt = 0;
	ParWebServ->StopServReq = false;
	ParWebServ->DataReadyCount = 0;
	ParWebServ->SentRespUser = 0;
	ParWebServ->Count_ID = 1;
	ParWebServ->SummHtmlPageGenTime = 0;
	ParWebServ->HtmlPageGenCount = 0;
	ParWebServ->SummHtmlReqLoadTime = 0;
	ParWebServ->HtmlReqLoadCount = 0;
	ParWebServ->ListJobReq.Count = 0;
	ParWebServ->SentRespDoneCount = 0;
    ParWebServ->LogTypeListLoadDone = false;
    ParWebServ->OamServConnect = false;
    strcpy(ParWebServ->SysStatusData, NoHostInfo);
    ParWebServ->ReaderWorker.isActive = false;
    ParWebServ->SenderWorker.isActive = false;
    ParWebServ->MailWorker.isActive = false;
    ParWebServ->SmsWorker.isActive = false;
	ParWebServ->OnGetHtmlUserTypeMask = OnGetHtmlUserTypeMask;

	ParWebServ->BannerList.Count = 0;
    ParWebServ->BannerList.CurrTask = NULL;
	ParWebServ->BannerList.FistTask = NULL;

	memset(&ParWebServ->ShopInfoCfg, 0, sizeof(PARSHOPINFO));
	ParWebServ->ShopInfoCfg.ZipCode = 190000;

	strncpy(ParWebServ->ShopInfoCfg.Name,        SystemName,          MAX_LEN_COMPANY_NAME);
    strncpy(ParWebServ->ShopInfoCfg.URL,         "myserver.ru",       MAX_LEN_URL_SERVER);
    strncpy(ParWebServ->ShopInfoCfg.City,        "St. Petersburg",    MAX_LEN_CITY_NAME);
	strncpy(ParWebServ->ShopInfoCfg.Address,     "Nevsky, 1",         MAX_LEN_ADDR_1_NAME);
	strncpy(ParWebServ->ShopInfoCfg.LandPhone,   "+7(812)7777777",    MAX_LEN_PHONE_NUM);
	strncpy(ParWebServ->ShopInfoCfg.MobilePhone1,"+7(921)3333333",    MAX_LEN_PHONE_NUM);
	strncpy(ParWebServ->ShopInfoCfg.MobilePhone2,"+7(921)4444444",    MAX_LEN_PHONE_NUM);
	strncpy(ParWebServ->ShopInfoCfg.LocLongitude,"30.361855",         MAX_LOCATION_LEN);
	strncpy(ParWebServ->ShopInfoCfg.LocLatitude, "59.931055",         MAX_LOCATION_LEN);

	ParWebServ->GeneralCfg.MaxOpenSessions = 100;
    ParWebServ->GeneralCfg.MaxSesionPerIP = 10;
    ParWebServ->GeneralCfg.HtmlPageComprssEnable = true;
    ParWebServ->GeneralCfg.MinHtmlSizeCompress = 1024;
    ParWebServ->GeneralCfg.KeepAliveEnable = false;
    ParWebServ->GeneralCfg.KeepAliveTimeout = 5;
	ParWebServ->UserMetaData = NULL;
	ParWebServ->UserMetaData = (char*)AllocateMemory(16384*sizeof(char));
    memset(ParWebServ->UserMetaData, 0, 8192*sizeof(char));

    memset(&LastPageGenTime, 0, 4*sizeof(unsigned int));
	memset(&LastReqLoadTime, 0, 4*sizeof(unsigned int));

	InitSessionKey();

	StatsDataInit();
    CwdRet = getcwd((char*)(&ParWebServ->StartPath[0]),512);
    ParWebServ->ActivServer = true;
	SetStartDateFileHeadRequest(&ParWebServ->ServerStartTime[0]);
    SetExpDateFileHeadRequest(&ParWebServ->BrowserCacheExpTime[0]);

    SetAddrConnectChan(ParWebServ);
	
	/* Load Capcha indexes */
	LoadCapcha(ParWebServ);

	/* Init session IP hash table */
	InitSessionIpHash(&SessionIpHashHop);
	
	/* Init session manager */
	SessionManagerInit(&ParWebServ->SessionManager, sizeof(SAMPLE_SERVER_CUSTOM_CONFIG));
	ParWebServ->SessionManager.OnUserSessionOpenCB = OnUserSessionOpenCB;
	ParWebServ->SessionManager.OnUserSessionCloseCB = OnUserSessionCloseCB;
	ParWebServ->SessionManager.OnGetSessionTimeoutCB = OnGetSessionTimeoutCB;
	ParWebServ->SessionManager.OnSessionTimerStartCB = OnSessionTimerStartCB;
	ParWebServ->SessionManager.OnSessionTimerResetCB = OnSessionTimerResetCB;
	ParWebServ->SessionManager.OnSessionGrpTimersRestartCB = OnSessionGrpTimersRestartCB;
	ParWebServ->SessionManager.ServCustomCfgPtr = &ParWebServ->ServCustomCfg;
	ParWebServ->SessionManager.StartPath = ParWebServ->StartPath;
	ParWebServ->SessionManager.DataPtr = ParWebServ;
	ParWebServ->SessionManager.SysShowUserType = UAT_SYSSHOW;
	ParWebServ->SessionManager.SysShowHostIP = SampleCustomCfg.SysShowHostIP;
	ParWebServ->SessionManager.SelConfigEditGrpId = GEC_GENERAL;

	/* Init of html GET/POST commands hash */
    HtmlCmdHashInit(&GetHtmlPageHashHop, &GrupHttpGetArray, "GET");
    HtmlCmdHashInit(&PostHtmlPageHashHop, &GrupHttpPostArray, "POST");

	/* Init hash of HTML template commands */
	InitHtmlBodyCmdHash(HtmlCmdList, HtmlCmdListLen);

	/* Init hash of HTML line commands */
	InitHtmlLineCmdHash(HtmlCmdInLineList, HtmlCmdInLineListLen);
	
	/* Content files hash init */
    ContentFilesHashInit(ParWebServ->StartPath, &ParWebServ->ServCustomCfg,
	    HashDirArray, HashDirArraySize);
	
    MobileDeviceDBLoad();
    BotInfoDBLoad();

    /* These function calls initialize openssl for correct work. */
    OpenSSL_add_all_algorithms(); /* Load cryptos, et.al. */
    ERR_load_crypto_strings();
    SSL_load_error_strings();     /* Bring in and register error messages */
    OpenSslLockCreate();

    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
	ParWebServ->LastDayOfWeek = (unsigned char)(cur_time->tm_wday);

    ParWebServ->ReqMsgPort = 0;
    ParWebServ->ReqMsgAddr = 0;
    ConnChanInfoInit(ParWebServ);

    /* Sms worker init */    
    ParWebServ->SmsWorker.WebServMsgPort = ParWebServ->ServCustomCfg.WebServMsgPort;     
    ParWebServ->SmsWorker.URL = ParWebServ->ShopInfoCfg.URL;
    ParWebServ->SmsWorker.OnSmsSendStatus = HandleOnSmsSendStatus;
    SmsWorkerInit(&ParWebServ->SmsWorker);
    
    /* Mail worker init */    
    ParWebServ->MailWorker.WebServMsgPort = ParWebServ->ServCustomCfg.WebServMsgPort;     
    ParWebServ->MailWorker.MailSendDelayTimerId = INTER_MAIL_DELAY_TMR_ID;
    ParWebServ->MailWorker.OnMailSendStatus = HandleOnMailSendStatus;
    MailWorkerInit(&ParWebServ->MailWorker, ParWebServ->LocalAddrIP, WebOvnerMail);
    
    /* Sender worker init */
    ParWebServ->SenderWorker.WebServMsgPort = ParWebServ->ServCustomCfg.WebServMsgPort; 
    ParWebServ->SenderWorker.NextSendNotifyPort = ParWebServ->ServCustomCfg.BaseSenderPort; 
    ParWebServ->SenderWorker.KeepAliveEnable = ParWebServ->GeneralCfg.KeepAliveEnable;
    ParWebServ->SenderWorker.DataPtr = (void*)ParWebServ;
    ParWebServ->SenderWorker.OnAddPageSendCB = HandleOnAddPageSend;
	SenderWorkerInit(&ParWebServ->SenderWorker);

    /* Reader worker init */
	ParWebServ->ReaderWorker.NumActiveReaders = ParWebServ->ServCustomCfg.NumReaderWorkers;
    ParWebServ->ReaderWorker.WebServMsgPort = ParWebServ->ServCustomCfg.WebServMsgPort;
    ParWebServ->ReaderWorker.KeepAliveEnable = ParWebServ->GeneralCfg.KeepAliveEnable;
    ParWebServ->ReaderWorker.BaseReaderPort = ParWebServ->ServCustomCfg.BaseReaderPort;
    ReaderWorkerInit(&ParWebServ->ReaderWorker);
    
    ServerConfigLoad(ParWebServ);
    strcpy(ParWebServ->ShopInfoCfg.Name, SystemName);
    ParWebServ->SenderWorker.KeepAliveEnable = ParWebServ->GeneralCfg.KeepAliveEnable;
    ParWebServ->ReaderWorker.KeepAliveEnable = ParWebServ->GeneralCfg.KeepAliveEnable;
    ParWebServ->SecondConnWeb.isKeepAlive = ParWebServ->GeneralCfg.KeepAliveEnable;
    ParWebServ->SecondConnWeb.TimeOut = ParWebServ->GeneralCfg.KeepAliveTimeout;
    ParWebServ->PrimConnWeb.isKeepAlive = ParWebServ->GeneralCfg.KeepAliveEnable;
    ParWebServ->PrimConnWeb.TimeOut = ParWebServ->GeneralCfg.KeepAliveTimeout;
    
    UserSessionList.Count = 0;
    UserSessionList.CurrTask = NULL;
    UserSessionList.FistTask = NULL;
	
	SiteMapStUrlDBLoad();
    RusTextListLoad(ParWebServ);
    EngTextListLoad(ParWebServ);
    GroupDBLoad();    
    
    SampleUserDbIfo.AdmUserType = UAT_ADMIN;
    SampleUserDbIfo.DbVersion = LATEST_USER_DB_VERSION;
    SampleUserDbIfo.UserRecordSize = sizeof(SAMPLE_USER_INFO);
    SampleUserDbIfo.UserRecordPackSize = 3;
    SampleUserDbIfo.OnCreateExtDbRecCB = OnCreateExtDbRecCB;
    SampleUserDbIfo.OnLoadVerExtDbRecCB = OnLoadVerExtDbRecCB;
    SampleUserDbIfo.OnReadExtDbRecCB = OnReadExtDbRecCB;
    SampleUserDbIfo.OnSaveExtDbRecCB = OnSaveExtDbRecCB;
    UserInfoDBLoad(&SampleUserDbIfo);

    BadIPListLoad(); 
    LoadImageNameHash();
    FileNameMapDBLoad();
    WebMgmtsInit(ParWebServ, GetRealTimeMarker);

	if (!WebConnChanOpen(ParWebServ))
	{
        ServerBasesClose(ParWebServ);
        WebMgmtsClose();
        ParWebServ->isStartDone = false;
        sem_post(&ParWebServ->WebServInfo.semx);
	    pthread_exit((void *)0);
	}
		
    CreateThreadTimerCB(OnWebServerTimerExp, WEB_SERV_MSG_ID, 15*TMR_ONE_SEC, USER_SESSION_ACTIVITY_TMR_ID, true);
    CreateThreadTimerCB(OnWebServerTimerExp, WEB_SERV_MSG_ID, TMR_ONE_MIN, STATS_CHECK_TMR_ID, true);
    CreateThreadTimerCB(OnWebServerTimerExp, WEB_SERV_MSG_ID, 200, SYS_TIMESTAMP_TMR_ID, true);

    MemWebPageGenPtr = (char*)AllocateMemory(HTTP_PAGE_CREATE_BUF_SIZE*sizeof(char));
    SiteMapFileGen(ParWebServ);
    HandleeTimeStampSetTimerExp(ParWebServ);

	if (!MainWebReqHandler(ParWebServ, SampleCustomCfg.SysShowHostIP))
	{
        ServerBasesClose(ParWebServ);
        WebMgmtsClose();
        ParWebServ->isStartDone = false;
        sem_post(&ParWebServ->WebServInfo.semx);
        pthread_exit((void *)0);           
	}
	
    CloseThreadTimer(WEB_SERV_MSG_ID, STATS_CHECK_TMR_ID);
    CloseThreadTimer(WEB_SERV_MSG_ID, USER_SESSION_ACTIVITY_TMR_ID);
    CloseThreadTimer(WEB_SERV_MSG_ID, SYS_TIMESTAMP_TMR_ID);

    ServerBasesClose(ParWebServ);
    WebMgmtsClose();
    OpenSslLockClose();
    
    DebugLogPrint(NULL, "%s: *************** %s Server was successfully shutdown *****************\r\n", 
		ThrWebServName, ParWebServ->ShopInfoCfg.Name);

    EventLogPrint(NULL, "%s: *************** %s Server was successfully shutdown *****************\r\n", 
		ThrWebServName, ParWebServ->ShopInfoCfg.Name);

	Sleep(100);
	gCloseCernelFlag = 1;
    pthread_exit((void *)0);
}
//---------------------------------------------------------------------------
void ServerBasesClose(PARAMWEBSERV *ParWebServ)
{
    BotDBClear();
    GroupDBClear();
    FileNameMapDBClear();
    CloseImageNameHash();
	BadIPListClear();
	UserInfoDBClose();
	CloseFileHash(&FileHashHop);
	CloseSessionIpHash(&SessionIpHashHop);
	MobileDeviceDBClear();
	SiteMapStUrlDBClear();
	CloseHtmlPageHash(&GetHtmlPageHashHop);
    CloseHtmlPageHash(&PostHtmlPageHashHop);
	CloseHtmlBodyCmdHash();
    CloseHtmlLineCmdHash();
    SenderWorkerClose(&ParWebServ->SenderWorker);
	Sleep(20);
    ReaderWorkerClose(&ParWebServ->ReaderWorker);
    CloseSessionKey();
	ContentFilesHashClose(&ParWebServ->ServCustomCfg);
	SessionManagerClose(&ParWebServ->SessionManager);
	if (MemWebPageGenPtr) FreeMemory(MemWebPageGenPtr);
    MemWebPageGenPtr = NULL;  
}
//---------------------------------------------------------------------------
void HandlerWebServerReq(PARAMWEBSERV *ParWebServ, InterThreadInfoMsg* WebServMsgPtr)
{
	unsigned long	UseIPAddrClient;
    
    switch (WebServMsgPtr->MsgTag)
    {
		case WSU_CONNECTUSER:
			HandleNewUserConnect(ParWebServ, (READWEBSOCK*)WebServMsgPtr->WParam);
			break;

		case WSU_USERDATA:
            HandleReaderDataInd(ParWebServ, WebServMsgPtr, SampleCustomCfg.SysShowHostIP);
            break;

		case WSU_RESPSENDCMPLT:
            HandleSendWorkerDoneInd(ParWebServ, WebServMsgPtr);
			break;

        case WSU_STATEWEBSERV:
		    UseIPAddrClient = (unsigned long)WebServMsgPtr->LParam;
            break;
            
		case TM_ONTIMEOUT:
			switch((unsigned int)(tulong)WebServMsgPtr->WParam)
			{                    
			    case INTER_MAIL_DELAY_TMR_ID:
					HandleTimeoutNextMailSent(ParWebServ);
				    break;

				case USER_SESSION_ACTIVITY_TMR_ID:
					HandleSessionActivTmrExp(&ParWebServ->SessionManager);
					break;

				case STATS_CHECK_TMR_ID:
                    HandleStatsCheckTimerExp(ParWebServ);
                    ReaderInstQueueUsageReport(&ParWebServ->ReaderWorker);
                    SenderInstQueueUsageReport(&ParWebServ->SenderWorker);
					break;

                case  SYS_TIMESTAMP_TMR_ID:
                    HandleeTimeStampSetTimerExp(ParWebServ);
                    break;
                       
				default:
					HandleSessionTimerExp(&ParWebServ->SessionManager, (unsigned int)(tulong)WebServMsgPtr->WParam);
					break;
			}
			break;

		case WSU_RESPSMSSENT:
            HandleSmsThrDone(&ParWebServ->SmsWorker, (PARSENDSMS*)WebServMsgPtr->WParam);
			break;

		case WSU_RESPMAILSENT:
			HandleMailThrDone(&ParWebServ->MailWorker, (PARSENDMAIL*)WebServMsgPtr->WParam);
			break;

        case WSU_CLOSECONNWEB:
        case WSU_CLOSEWEBSERVER:
			  /* Stop web server thread req */
			  ParWebServ->StopServReq = true;
			  break;

		case WSU_HBACTIONTHR:
            switch((unsigned int)(tulong)WebServMsgPtr->LParam)
            {
                case WHN_READER_IND:
			        DebugLogPrint(NULL, "%s: Received HB message from reader thread (Port: %u)\r\n", 
				        ThrWebServName, ((READER_WEB_INFO*)WebServMsgPtr->WParam)->NotifyPort);                
                    break;
                    
                default: //WHN_SENDER_IND
			        DebugLogPrint(NULL, "%s: Received HB message from sender thread (Port: %u)\r\n", 
				        ThrWebServName, ((SENDER_WEB_INFO*)WebServMsgPtr->WParam)->NotifyPort);
                    break;
            }
			break;

        default:
			DebugLogPrint(NULL, "%s: received unexpected message %d\r\n", 
				ThrWebServName, WebServMsgPtr->MsgTag);
            break;
    }
}
//---------------------------------------------------------------------------
void HandleLineSetSysName(void *ParPtr)
{
    strcpy(((TEXT_CMD_PAR*)ParPtr)->ParBufPtr, SystemName);
}
//---------------------------------------------------------------------------
void HandleLineSetServerVersion(void *ParPtr)
{
    strcpy(((TEXT_CMD_PAR*)ParPtr)->ParBufPtr, ServerVersion);
}
//---------------------------------------------------------------------------
static void HandleOnAddPageSend(void *DataPtr)
{
    PARAMWEBSERV *ParWebServPtr = NULL;
    
    ParWebServPtr = (PARAMWEBSERV*)DataPtr;
	if (ParWebServPtr->SenderWorker.HttpSendThrList.Count > ServerStats.SimultSendSessions)
		ServerStats.SimultSendSessions = ParWebServPtr->SenderWorker.HttpSendThrList.Count;
	ParWebServPtr->SentRespUser++;
}
//---------------------------------------------------------------------------
static void HandleOnMailSendStatus(bool Status)
{
	if (Status) ServerStats.SuccMailDelivery++;
	else        ServerStats.FailMailDelivery++;
}
//---------------------------------------------------------------------------
static void HandleOnSmsSendStatus(bool Status)
{
	if (Status) ServerStats.SuccSmsDelivery++;
	else        ServerStats.FailSmsDelivery++;
}
//---------------------------------------------------------------------------
bool isPrimaryGroupAccessUser(USER_INFO *UserPtr)
{
    if ((UserPtr->UserType == UAT_ADMIN) || 
        (UserPtr->UserType == UAT_SYSSHOW))
    {
        return true;
    }
    else
    {
        return false;
    }
}
//---------------------------------------------------------------------------
static void OnCreateExtDbRecCB(void *ExtUserInfoPtr)
{
    SAMPLE_USER_INFO *SampleUserInfPtr = NULL;
    
    SampleUserInfPtr = (SAMPLE_USER_INFO*)ExtUserInfoPtr;
    SampleUserInfPtr->EventMailNotify = false;
}
//---------------------------------------------------------------------------
static unsigned int OnLoadVerExtDbRecCB(unsigned char LoadVersion)
{
    return 3;
}
//---------------------------------------------------------------------------
static unsigned char* OnReadExtDbRecCB(void *ExtUserInfoPtr, unsigned char LoadVersion, unsigned char *DataPtr)
{
    SAMPLE_USER_INFO *SampleUserInfPtr = NULL;
    
    SampleUserInfPtr = (SAMPLE_USER_INFO*)ExtUserInfoPtr;
    SampleUserInfPtr->EventMailNotify = (bool)*DataPtr++;  
    return DataPtr;
}
//---------------------------------------------------------------------------
static unsigned char* OnSaveExtDbRecCB(void *ExtUserInfoPtr, unsigned char *DataPtr)
{
    SAMPLE_USER_INFO *SampleUserInfPtr = NULL;
    
    SampleUserInfPtr = (SAMPLE_USER_INFO*)ExtUserInfoPtr;
    *DataPtr++ = (unsigned char)SampleUserInfPtr->EventMailNotify; 
    return DataPtr;
}
//---------------------------------------------------------------------------
void HandleNewUserConnect(PARAMWEBSERV *ParWebServPtr, READWEBSOCK *ParReadWeb)
{
    unsigned int ActiveReaders;
      
#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "%s: Begin HandlerWebServerReq function (%d) (WSU_CONNECTUSER)\r\n", 
		ThrWebServName, ((READWEBSOCK*)WebServMsgPtr->WParam)->HttpSocket);
#endif
    HttpLoadAddList(&ParWebServPtr->ReaderWorker, ParReadWeb);
    ActiveReaders = GetNumberActiveReaders(&ParWebServPtr->ReaderWorker);
    if (ActiveReaders > ServerStats.SumultReadSessions)
        ServerStats.SumultReadSessions = ActiveReaders;
#ifdef _SERVDEBUG_
    DebugLogPrint(NULL,"%s: Start load HTTP request (%d)\r\n", 
			ThrWebServName, ParReadWeb->HttpSocket);
#endif
}
//---------------------------------------------------------------------------
static void OnUserSessionOpenCB(void *UserSessionPtr)
{
	SAMPLE_SPECIFIC_USER_SESSION *SessionPtr;
	
	SessionPtr = (SAMPLE_SPECIFIC_USER_SESSION*)UserSessionPtr;
	SessionPtr->UserSelGrpId = SHOW_ALL_AVAIL_GRP;
	SessionPtr->SelGroupId = 0;
	SessionPtr->StartGroupRec = 0;
	SessionPtr->GroupPerPage = 1;
}
//---------------------------------------------------------------------------
static void OnUserSessionCloseCB(USER_SESSION *SessionPtr, void *DataPtr)
{
	SAMPLE_SPECIFIC_USER_SESSION *SampleSessionPtr;
	PARAMWEBSERV *ParWebServ = NULL;

	ParWebServ = (PARAMWEBSERV*)DataPtr;
	SampleSessionPtr = (SAMPLE_SPECIFIC_USER_SESSION*)SessionPtr->UserSessionInfoPtr;
}
//---------------------------------------------------------------------------
static unsigned int OnGetSessionTimeoutCB(unsigned int UserType, void *DataPtr)
{
	unsigned int Delay;
	PARAMWEBSERV *ParWebServ = NULL;

	ParWebServ = (PARAMWEBSERV*)DataPtr;		
	switch(UserType)
	{
        case UAT_SYSSHOW:
            Delay = ParWebServ->ServCustomCfg.UserSessionTimeout;
            break;

		default:
			Delay = ParWebServ->ServCustomCfg.UserSessionTimeout;
			break;
	}
	return Delay;
}
//---------------------------------------------------------------------------
static void OnSessionTimerResetCB(unsigned int TimerId, void *DataPtr)
{
	PARAMWEBSERV *ParWebServ = NULL;

	ParWebServ = (PARAMWEBSERV*)DataPtr;
	CloseThreadTimer(WEB_SERV_MSG_ID, (unsigned short)TimerId);
}
//---------------------------------------------------------------------------
static void OnSessionTimerStartCB(unsigned int TimerId, unsigned int Delay, void *DataPtr)
{
	PARAMWEBSERV *ParWebServ = NULL;

	ParWebServ = (PARAMWEBSERV*)DataPtr;
	CreateThreadTimerCB(OnWebServerTimerExp, WEB_SERV_MSG_ID, Delay*TMR_ONE_SEC, TimerId, false);
}
//---------------------------------------------------------------------------
static void OnSessionGrpTimersRestartCB(unsigned int *TimerRestartReqArrayPtr, void *DataPtr)
{
	PARAMWEBSERV *ParWebServ = NULL;

    ParWebServ = (PARAMWEBSERV*)DataPtr;
    RestartGrpThreadTimer(WEB_SERV_MSG_ID, TimerRestartReqArrayPtr);
}
//---------------------------------------------------------------------------
static unsigned int OnGetHtmlUserTypeMask(unsigned int UserType)
{
	unsigned int HtmlUserTypeMask;

	switch(UserType)
	{
		case UAT_ADMIN:
			HtmlUserTypeMask = 0x04;
			break;

        case UAT_SYSSHOW:
            HtmlUserTypeMask = 0x08;
            break;

		default:
			HtmlUserTypeMask = 0x02;
			break;
	}
	return HtmlUserTypeMask;
}
//---------------------------------------------------------------------------
static void HandleSetSysCompany(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;

    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddSpaceSetStrWebPage(OwnerCompany);
}
//---------------------------------------------------------------------------
static void HandleSetWebOwnerMail(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
	AddStrWebPage(WebOvnerMail);
}
//---------------------------------------------------------------------------
char* GetSystemOvnerCompany()
{
	return OwnerCompany;
}
//---------------------------------------------------------------------------
