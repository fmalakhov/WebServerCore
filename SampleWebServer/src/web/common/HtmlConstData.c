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

#include "CommonPlatform.h"
#include "vistypes.h"
#include "ThrCernel.h"
#include "SysWebFunction.h"
#include "HttpPageGen.h"
#include "ImageNameHash.h"

char GenPageAboutServer[] = "AboutServer.html";
char GenPageMyContacts[] = "MyContacts.html";
char GenPageUserRegRequest[] = "UserRegRequest.html";
char GenPageUserContactInfoSet[] = "UserContactInfoSet.html";
char GenPageRegClientsList[] = "RegClientsList.html";
char GenPageServerConfig[] = "ServerConfig.html";
char GenPagePasswdSentMail[] = "PasswdSentMail.html";
char GenPageServStats[] = "ServerStats.html";
char GenPageActSessList[] = "ActiveSessionsList.html";
char GenPageAdmPswdChg[] = "AdminPasswordChange.html";
char GenPageDelUser[] = "DelUserRegister.html";
char GenPageChgUserType[] = "ChgUserRegType.html";
char GenPageCghGroupDbManage[] = "ChgViewGrpDBMgt.html";
char GenPageGroupDBManage[] = "GroupDBManage.html";
char GenPageChgGrpUser[] = "ChgHostActGropUser.html";
char GenPageChgGrpSet[] = "ChgHostActGropSet.html";
char GenPageDynEncodeDataReq[] = "ServEcpDataReq.htr";
char GenPageDynAuthConfKeyReq[] = "AuthConfKeyReq.htr";

/* User's defined HTML page names */
char GenPageSampleOverview[] = "SampleOverview.html";

char NoHostInfo[] = "<script language=\"javascript\" type=\"text/javascript\">var HostInfoData=[];</script>\r\n";

char LogsGrpName[]      = "SampleWebServer";

char CsvTablePathBaseName[]     = "/LogFiles/log_";

char AlarmFilterPathBaseName[] = "alarm_filter_";
char PowerFilterPathBaseName[] = "power_filter_";

char BtExitPageKey[]      = "BtExitPage";
char BtNavFirstPgDsKey[]  = "BtNavFirstPgDs";
char BtNavFirstPgEnKey[]  = "BtNavFirstPgEn";
char BtNavFirstPgMOKey[]  = "BtNavFirstPgMO";
char BtNavFirstPgMSKey[]  = "BtNavFirstPgMS";
char BtNavPrevPgDsKey[]   = "BtNavPrevPgDs";
char BtNavPrevPgEnKey[]   = "BtNavPrevPgEn";
char BtNavPrevPgMOKey[]   = "BtNavPrevPgMO";
char BtNavPrevPgMSKey[]   = "BtNavPrevPgMS";
char BtNavNextPgDsKey[]   = "BtNavNextPgDs";
char BtNavNextPgEnKey[]   = "BtNavNextPgEn";
char BtNavNextPgMOKey[]   = "BtNavNextPgMO";
char BtNavNextPgMSKey[]   = "BtNavNextPgMS";
char BtNavLastPgDsKey[]   = "BtNavLastPgDs";
char BtNavLastPgEnKey[]   = "BtNavLastPgEn";
char BtNavLastPgMOKey[]   = "BtNavLastPgMO";
char BtNavLastPgMSKey[]   = "BtNavLastPgMS";
char WorkZoneBgImageKey[] = "WorkZoneBgImage";
char BtNavRetPgDsKey[]    = "BtNavRetPgDs";
char BtNavRetPgEnKey[]    = "BtNavRetPgEn";
char BtNavRetPgMOKey[]    = "BtNavRetPgMO";
char BtNavRetPgMSKey[]    = "BtNavRetPgMS";
char BtBacktInfoDsKey[]   = "BtBacktInfoDs";
char BtBacktInfoEnKey[]   = "BtBacktInfoEn";
char BtBacktInfoMoKey[]   = "BtBacktInfoMo";
char BtRemItemEnKey[]     = "BtRemItemEn";
char BtRemItemMoKey[]     = "BtRemItemMo";
char BtUserGrpEnKey[]     = "BtUserGrpEn";
char BtUserGrpMoKey[]     = "BtUserGrpMo";

char EmptyBlkKey[]        = "EmptyBlk";
char BtInfSendServEnKey[] = "BtInfSendServEn";
char BtInfSendServMoKey[] = "BtInfSendServMo";
char BtFormCancelEnKey[]  = "BtFormCancelEn";
char BtFormCancelMoKey[]  = "BtFormCancelMo";
char BtFormCloseEnKey[]   = "BtFormCloseEn";
char BtFormCloseMoKey[]   = "BtFormCloseMo";
char BtExportExcelEnKey[] = "BtExportExcelEn";
char BtExportExcelMoKey[] = "BtExportExcelMo";
char BtExportExcelMsKey[] = "BtExportExcelMs";
char BtStopTestEnKey[]    = "BtStopTestEn";
char BtStopTestMoKey[]    = "BtStopTestMo";
char BtLogViewEnKey[]     = "BtLogViewEn";
char BtLogViewMoKey[]     = "BtLogViewMo";
char BtStatsViewEnKey[]   = "BtStatsViewEn";
char BtStatsViewMoKey[]   = "BtStatsViewMo";
char BtGetKeyEnKey[]      = "BtGetKeyEn";
char BtGetKeyMoKey[]      = "BtGetKeyMo";
char BtActionExecEnKey[]  = "BtActionExecEn";
char BtActionExecMoKey[]  = "BtActionExecMo";
char BtEnterEnKey[]       = "BtEnterEn";
char BtEnterMoKey[]       = "BtEnterMo";
char BtKeyEnKey[]         = "BtKeyEn";
char BtKeyMoKey[]         = "BtKeyMo";
char BtOkEnKey[]          = "BtOkEn";
char BtOkMoKey[]          = "BtOkMo";

IMAGE_NAME ServImageArray[] = {
	/* Background image for work zone of site pages */
	WorkZoneBgImageKey,"images/WorkZoneBgImage0320201403.png",    "images/WorkZoneBgImage0320201403.png",

	BtExitPageKey,     "images/BtEngExitPage0125201401.png",         "images/BtExitPage0125201401.png",

    /* Navigation button to first page */
	BtNavFirstPgDsKey, "images/BtNavEngFirstPgDs0126201401.png",     "images/BtNavFirstPgDs0126201401.png",
    BtNavFirstPgEnKey, "images/BtNavEngFirstPgEn0126201401.png",     "images/BtNavFirstPgEn0126201401.png",
	BtNavFirstPgMOKey, "images/BtNavEngFirstPgMO0126201401.png",     "images/BtNavFirstPgMO0126201401.png",
	BtNavFirstPgMSKey, "images/BtNavEngFirstPgMS0126201401.png",     "images/BtNavFirstPgMS0126201401.png",

	/* Navigation button to previous page */
	BtNavPrevPgDsKey,  "images/BtNavEngPrevPgDs0126201401.png",      "images/BtNavPrevPgDs0126201401.png",
	BtNavPrevPgEnKey,  "images/BtNavEngPrevPgEn0126201401.png",      "images/BtNavPrevPgEn0126201401.png",
	BtNavPrevPgMOKey,  "images/BtNavEngPrevPgMO0126201401.png",      "images/BtNavPrevPgMO0126201401.png",
	BtNavPrevPgMSKey,  "images/BtNavEngPrevPgMS0126201401.png",      "images/BtNavPrevPgMS0126201401.png",

	/* Navigation button to next page */
	BtNavNextPgDsKey,  "images/BtNavEngNextPgDs0126201401.png",      "images/BtNavNextPgDs0126201401.png",
	BtNavNextPgEnKey,  "images/BtNavEngNextPgEn0126201401.png",      "images/BtNavNextPgEn0126201401.png",
	BtNavNextPgMOKey,  "images/BtNavEngNextPgMO0126201401.png",      "images/BtNavNextPgMO0126201401.png",
	BtNavNextPgMSKey,  "images/BtNavEngNextPgMS0126201401.png",      "images/BtNavNextPgMS0126201401.png",

    /* Navigation button to last page */
    BtNavLastPgDsKey,  "images/BtNavEngLastPgDs0126201401.png",      "images/BtNavLastPgDs0126201401.png",
	BtNavLastPgEnKey,  "images/BtNavEngLastPgEn0126201401.png",      "images/BtNavLastPgEn0126201401.png",
	BtNavLastPgMOKey,  "images/BtNavEngLastPgMO0126201401.png",      "images/BtNavLastPgMO0126201401.png",
	BtNavLastPgMSKey,  "images/BtNavEngLastPgMS0126201401.png",      "images/BtNavLastPgMS0126201401.png",

	/* Navigation button for return from page */
    BtNavRetPgDsKey,   "images/BtNavEngReturnDs0128201401.png",      "images/BtNavReturnDs0128201401.png",
	BtNavRetPgEnKey,   "images/BtNavEngReturnEn0128201401.png",      "images/BtNavReturnEn0128201401.png",
	BtNavRetPgMOKey,   "images/BtNavEngReturnMO0128201401.png",      "images/BtNavReturnMO0128201401.png",
	BtNavRetPgMSKey,   "images/BtNavEngReturnMS0128201401.png",      "images/BtNavReturnMS0128201401.png",

	/* Object remove button */
	BtRemItemEnKey,    "images/BtRemEn0201201401.png",            "images/BtRemEn0201201401.png",
	BtRemItemMoKey,    "images/BtRemMo0201201401.png",            "images/BtRemMo0201201401.png",
	EmptyBlkKey,       "images/EmptyBlk0221201401.gif",           "images/EmptyBlk0221201401.gif",

	/* Group access change button */
	BtUserGrpEnKey,    "images/BtUserGrpEn0416201501.png",        "images/BtUserGrpEn0416201501.png",
	BtUserGrpMoKey,    "images/BtUserGrpMo0416201501.png",        "images/BtUserGrpMo0416201501.png",

	/* Button for information sent to server */
	BtInfSendServEnKey, "images/BtEngSendReqEn0328201401.png",       "images/BtSendReqEn0328201401.png",
	BtInfSendServMoKey, "images/BtEngSendReqMO0328201401.png",       "images/BtSendReqMO0328201401.png",

	/* Buttom for form cancel */
	BtFormCancelEnKey,  "images/BtEngCancelEn0328201401.png",        "images/BtCancelEn0328201401.png",
	BtFormCancelMoKey,  "images/BtEngCancelMO0328201401.png",        "images/BtCancelMO0328201401.png",

	/* Button for form close */
    BtFormCloseEnKey,  "images/BtExitEn0328201401.png",           "images/BtExitEn0328201401.png",
	BtFormCloseMoKey,  "images/BtExitMO0328201401.png",           "images/BtExitMO0328201401.png",

	/* Button for export to excel load file */
	BtExportExcelEnKey,"images/BtEngExportExcelEn0418201401.png",   "images/BtExportExcelEn0418201401.png", 
	BtExportExcelMoKey,"images/BtEngExportExcelMo0418201402.png",   "images/BtExportExcelMo0418201402.png",
	BtExportExcelMsKey,"images/BtEngExportExcelMs0418201402.png",   "images/BtExportExcelMs0418201402.png",
    
    /* Button for stop test execution */
    BtStopTestEnKey,   "images/BtEngStopTestEn1117201401.png",      "images/BtRusStopTestEn1117201401.png",
    BtStopTestMoKey,   "images/BtEngStopTestMo1117201401.png",      "images/BtRusStopTestMo1117201401.png",
    
    /* Button for logs view */
    BtLogViewEnKey,   "images/BtEngViewLogEn.png",      "images/BtRusViewLogEn.png",
    BtLogViewMoKey,   "images/BtEngViewLogMo.png",      "images/BtRusViewLogMo.png",    
    
    /* Button for stats view */
    BtStatsViewEnKey, "images/BtEngViewStatsEn.png",    "images/BtRusViewStatsEn.png",
    BtStatsViewMoKey, "images/BtEngViewStatsMo.png",    "images/BtRusViewStatsMo.png",
    
    /* Button for confirmation key request */
    BtGetKeyEnKey,    "images/BtEngGetKeyEn.png",       "images/BtRusGetKeyEn.png",
    BtGetKeyMoKey,    "images/BtEngGetKeyMo.png",       "images/BtRusGetKeyMo.png",
    
    /* Button for action execution start */
    BtActionExecEnKey, "images/BtEngActionExecEn.png",  "images/BtRusActionExecEn.png",
    BtActionExecMoKey, "images/BtEngActionExecMo.png",  "images/BtRusActionExecMo.png",
    
    /* Button for nuers enter to the system */
    BtEnterEnKey,      "images/BtEngEnterEn.png",       "images/BtRusEnterEn.png",
    BtEnterMoKey,      "images/BtEngEnterMo.png",       "images/BtRusEnterMo.png",
    
    /* Button for access key get */
    BtKeyEnKey,        "images/BtEngKeyEn.png",         "images/BtRusKeyEn.png",
    BtKeyMoKey,        "images/BtEngKeyMo.png",         "images/BtRusKeyMo.png",
    
    /* Button for confirmation */
    BtOkEnKey,         "images/BtOkEn.png",             "images/BtOkEn.png",
    BtOkMoKey,         "images/BtOkMo.png",             "images/BtOkMo.png"    
};

unsigned int ServImageArrayLen = sizeof(ServImageArray)/sizeof(IMAGE_NAME);

//---------------------------------------------------------------------------
