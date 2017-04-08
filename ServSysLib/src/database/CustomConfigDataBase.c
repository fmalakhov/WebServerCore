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
#include "CustomConfigDataBase.h"
#include "ServerPorts.h"

static void HandleComment(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleUserAuthRegNonUserSale(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetManNamePrefix(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetLastNameField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqLastNameField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetMiddleNameField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqMiddleNameField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetUserCityField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqUserCityField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetUserZipField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqUserZipField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetUserCountryField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqUserCountryField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetUserFaxField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqUserFaxField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqUserEmailFiled(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetCompanyName(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetUserMobPhoneField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqUserMobPhoneField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetUserAddr1Field(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqUserAddr1Field(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetUserAddr2Field(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqUserAddr2Field(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleTableBgColor1(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleTableBgColor2(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleOnObjMouseOverColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleOnObjClickColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleDownCentrBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleServConfigBgColor1(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleServConfigBgColor2(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleWorkZoneBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleMenuGrpBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleGrpTitleBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleGrpContBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleMenuZoneBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleGrpBorderColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgBgSiteGround(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgProductLogo(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgBgSideColumn(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgNavBar(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgBgWorkZone(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgBgDownCentr(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgBgHeaderDesc(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgBgHeaderM320(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgBgHeaderM360(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgBgHeaderM400(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleImgShopView(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleAutoOfferGen(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleHourStartSmsNotifyDelay(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleHourStopSmsNotifyDelay(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleAnonymTimoutExpInfo(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleUsePaidDelivery(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleAdminSessionTimeout(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleUserSessionTimeout(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleAnonymSessionTimeout(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSetUserLandPhoneField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleReqUserLandPhoneField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecWebPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondWebAccIPPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondExtServIPPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondLocalIPAddrServ(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondLocalHostName(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimPortKeyAccess(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondPortKeyAccess(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondPortInfoEdit(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
#ifdef _LINUX_X86_
static void HandleSudoSet(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
#endif
static void HandlePrimWebAccIPPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimExtServIPPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimLocalIPAddrServ(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimLocalHostName(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleStartSessionLanguage(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleDemoModeSet(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimPortHttpSecure(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondPortHttpSecure(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondPortSertFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondPortKeyFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimPortSertFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimPortKeyFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimPortDdosProtect(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondPortDdosProtect(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleDDosDetectTreshold(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleDDosIpLockTime(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleDDosIpFreeTime(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleForwardIpPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleForwardUrl(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleForwardPortDdosProtect(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleForwardPortHttpSecure(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleForwardPortSertFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleForwardPortKeyFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleForwardLocalIPAddrServ(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleForwardLocalHostName(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimContentDelivery(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondContentDelivery(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandlePrimContentRootDir(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleSecondContentRootDir(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleAddDirHash(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleNumReaderWorkers(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleBaseReaderPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);
static void HandleFileRequestLogging(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr);

extern char CustomConfigNamePath[];

#ifdef WIN32
extern HANDLE gFileMutex;
#endif

#define MAX_LEN_CONFIG_BASE_LINE 640

static bool CmdLineParse(char *BufRequest, DEF_CMD_HANDLER *TablArrCMD, 
    unsigned int AllTestCMD, SERVER_CUSTOM_CONFIG *ServCfgPtr, char *CmdBufPtr);
static void ExtCmdLineParse(char *BufRequest,
	WEB_SERVER_CUST_CFG_INFO *CustCfgInfoPtr, char *CmdBufPtr);

/* List of accepted commands for custom configuration base.*/
static DEF_CMD_HANDLER TablCustomCfgCmdDb[] = {
		/* Flags */
        "UserAuthRegNonUserSale", HandleUserAuthRegNonUserSale, /* Flag of user's request of user auth if order was rased by anonym user */
	    "SetManNamePrefix",       HandleSetManNamePrefix,       /* Flag of name prefix like Ser, .. set priore name */
	    "SetLastNameField",       HandleSetLastNameField,       /* Flag of enable for fill last name of customer */
	    "ReqLastNameField",       HandleReqLastNameField,       /* Flag of required fillout of last name field by customer */
	    "SetMiddleNameField",     HandleSetMiddleNameField,     /* Flag of enable for fill middle name of customer */
	    "ReqMiddleNameField",     HandleReqMiddleNameField,     /* Flag of required fillout of middle name field by customer */
	    "SetUserCityField",       HandleSetUserCityField,       /* Flag of enable for fill user's city name */
	    "ReqUserCityField",       HandleReqUserCityField,       /* Flag of required fillout of city name */
	    "SetUserZipField",        HandleSetUserZipField,        /* Flag of enable for fill user's zip code */
	    "ReqUserZipField",        HandleReqUserZipField,        /* Flag of required fillout of zip code */
	    "SetUserCountryField",    HandleSetUserCountryField,    /* Flag of enable for fill user's country */
	    "ReqUserCountryField",    HandleReqUserCountryField,    /* Flag of required fillout of user's country  */
	    "SetUserFaxField",        HandleSetUserFaxField,        /* Flag of enable for fill user's fax */
	    "ReqUserFaxField",        HandleReqUserFaxField,        /* Flag of required fillout of user's fax  */
	    "ReqUserEmailFiled",      HandleReqUserEmailFiled,      /* Flag of required fillout of user's e-mail  */
	    "SetCompanyName",         HandleSetCompanyName,         /* Flag of enable for fill user's company name */
	    "SetUserMobPhoneField",   HandleSetUserMobPhoneField,   /* Flag of enable for fill user's mobile phone num. */
	    "ReqUserMobPhoneField",   HandleReqUserMobPhoneField,   /* Flag of required fillout of mobile phone num. */
	    "SetUserAddr1Field",      HandleSetUserAddr1Field,      /* Flag of enable for fill user's part of address 1. */        
	    "ReqUserAddr1Field",      HandleReqUserAddr1Field,      /* Flag of required fillout of user's part of address 1. */
	    "SetUserAddr2Field",      HandleSetUserAddr2Field,      /* Flag of enable for fill user's part of address 2. */
	    "ReqUserAddr2Field",      HandleReqUserAddr2Field,      /* Flag of required fillout of user's part of address 2. */
		"AutoOfferGen",           HandleAutoOfferGen,           /* Flag of enable automatical generation of additional offers for item based on sales base */
		"AnonymTimoutExpInfo",    HandleAnonymTimoutExpInfo,    /* Falg of anonym user notification about session timeout expiration */
		"UsePaidDelivery",        HandleUsePaidDelivery,        /* Flag of enable of paid delivery usage */
	    "SetUserLandPhoneField",  HandleSetUserLandPhoneField,  /* Flag of enable for fill user's land phone num. */
	    "ReqUserLandPhoneField",  HandleReqUserLandPhoneField,  /* Flag of required fillout of land phone num. */

		/* Colors */
	    "TableBgColor1",          HandleTableBgColor1,
	    "TableBgColor2",          HandleTableBgColor2,
	    "OnObjMouseOverColor",    HandleOnObjMouseOverColor,
	    "OnObjClickColor",        HandleOnObjClickColor,
	    "DownCentrBgColor",       HandleDownCentrBgColor,
	    "ServConfigBgColor1",     HandleServConfigBgColor1,
	    "ServConfigBgColor2",     HandleServConfigBgColor2,
	    "WorkZoneBgColor",        HandleWorkZoneBgColor,
	    "MenuGrpBgColor",         HandleMenuGrpBgColor,
	    "GrpTitleBgColor",        HandleGrpTitleBgColor,
	    "GrpContBgColor",         HandleGrpContBgColor,
	    "MenuZoneBgColor",        HandleMenuZoneBgColor,
	    "GrpBorderColor",         HandleGrpBorderColor,
        
		/* Images */
	    "ImgBgSiteGround",        HandleImgBgSiteGround,
	    "ImgBgSideColumn",        HandleImgBgSideColumn,
	    "ImgNavBar",              HandleImgNavBar,
	    "ImgBgWorkZone",          HandleImgBgWorkZone,
	    "ImgBgDownCentr",         HandleImgBgDownCentr,
		"ImgBgHeaderDesc",        HandleImgBgHeaderDesc,
        "ImgBgHeaderM320",        HandleImgBgHeaderM320,
        "ImgBgHeaderM360",        HandleImgBgHeaderM360,
        "ImgBgHeaderM400",        HandleImgBgHeaderM400,
		"ImgShopView",            HandleImgShopView,
        "ImgProductLogo",         HandleImgProductLogo,        

		/* Others */
        "StartSessionLanguage",      HandleStartSessionLanguage,      /* The used language upon session startup */
		"HourStartSmsNotifyDelay",   HandleHourStartSmsNotifyDelay,   /* Hour of begin delay SMS notification sent */
		"HourStopSmsNotifyDelay",    HandleHourStopSmsNotifyDelay,    /* Hour of stop delaty and enable SMS notify sent */
        "AdminSessionTimeout",       HandleAdminSessionTimeout,       /* Time out for admin's session */
        "UserSessionTimeout",        HandleUserSessionTimeout,        /* Time out for user's session */
        "AnonymSessionTimeout",      HandleAnonymSessionTimeout,      /* Time out for anonym's session */

		"NumReaderWorkers",		  HandleNumReaderWorkers,       /* Number of reader workers */

		"comment",			      HandleComment,                /* The identificator of not visibled comment for this line; */
		"FileRequestLogging",	  HandleFileRequestLogging,		/* Flag of logging of files requests */
		"BaseReaderPort",		  HandleBaseReaderPort,

#ifdef _LINUX_X86_
        "SudoSet",                HandleSudoSet,               /* Flag of sudo usage for system operations */
#endif
        "DemoMode",               HandleDemoModeSet,           /* Flag of demo mode set */
        
		"DDosDetectTreshold",     HandleDDosDetectTreshold,    /* The treshold of number of packets from same IP address per second */
		"DDosIpLockTime",         HandleDDosIpLockTime,        /* Time of access lock from IP address in seconds */
		"DDosIpFreeTime",         HandleDDosIpFreeTime,        /* Time of IP record remove in seconds */

		"ForwardIpPort",          HandleForwardIpPort,         /* Ip port for requests forward */
		"ForwardUrl",             HandleForwardUrl,            /* URL for forward */
		"ForwardPortDdosProtect", HandleForwardPortDdosProtect,/* Flag of forward channel protection from DDOS */
		"ForwardPortHttpSecure",  HandleForwardPortHttpSecure, /* Flag of forward channel secure HTTP over SSL */
		"ForwardPortSertFile",    HandleForwardPortSertFile,   /* The path and name for forward channel sertificate file */
		"ForwardPortKeyFile",     HandleForwardPortKeyFile,    /* The path and name for forward channel server key file */
		"ForwardLocalIPAddrServ", HandleForwardLocalIPAddrServ,/* The local IP addres of forward channel */ 
		"ForwardLocalHostName",   HandleForwardLocalHostName,  /* The local host name of forward channel */

        /* Secondary WEB channel */
		"SecondContentDelivery",  HandleSecondContentDelivery, /* Falg of content delivery server's channel mode set */ 
        "SecWebPort",             HandleSecWebPort,            /* Flag of secondary WEB port configuration */
        "PrimPortKeyAccess",      HandlePrimPortKeyAccess,     /* Flag of access to primary channel via KEY */
		"PrimPortHttpSecure",     HandlePrimPortHttpSecure,    /* Flag of primary channel secure HTTP over SSL */
		"PrimPortDdosProtect",    HandlePrimPortDdosProtect,   /* Flag of primary channel protection from DDOS */
		"SecondPortHttpSecure",   HandleSecondPortHttpSecure,  /* Flag of secondary channel secure HTTP over SSL */
        "SecondPortKeyAccess",    HandleSecondPortKeyAccess,   /* Flag of access to secondary channel via KEY */
        "SecondPortInfoEdit",     HandleSecondPortInfoEdit,    /* Flag of personal data access from secondary channel */
		"SecondPortDdosProtect",  HandleSecondPortDdosProtect, /* Flag of secondary channel protection from DDOS */
        "SecondWebAccIPPort",     HandleSecondWebAccIPPort,    /* The WWW server IP access port */
        "SecondExtServIPPort",    HandleSecondExtServIPPort,   /* The WWW server external IP access port */
        "SecondLocalIPAddrServ",  HandleSecondLocalIPAddrServ, /* The local IP addres of secondary channel */
        "SecondLocalHostName",    HandleSecondLocalHostName,   /* The local host name of secondary channel */
		"SecondPortSertFile",     HandleSecondPortSertFile,    /* The path and name for secondary channel sertificate file */
		"SecondPortKeyFile",      HandleSecondPortKeyFile,     /* The path and name for secondary channel server key file */
		"SecondContentRootDir",   HandleSecondContentRootDir,  /* The path to root dir of primary channel context */
        
		"PrimContentDelivery",    HandlePrimContentDelivery,   /* Falg of content delivery server's channel mode set */
        "PrimWebAccIPPort",       HandlePrimWebAccIPPort,      /* The WWW server IP access port */
        "PrimExtServIPPort",      HandlePrimExtServIPPort,     /* The WWW server external IP access port */
        "PrimLocalIPAddrServ",    HandlePrimLocalIPAddrServ,   /* The local IP addres of primary channel */
        "PrimLocalHostName",      HandlePrimLocalHostName,     /* The local host name of primary channel */        
		"PrimPortSertFile",       HandlePrimPortSertFile,      /* The path and name for primary channel sertificate file */
		"PrimPortKeyFile",        HandlePrimPortKeyFile,       /* The path and name for primary channel server key file */
		"PrimContentRootDir",     HandlePrimContentRootDir,    /* The path to root dir of secondary channel context */

		"AddDirHash",             HandleAddDirHash
	};

#define CUSTOM_CFG_LEN sizeof(TablCustomCfgCmdDb)/sizeof(DEF_CMD_HANDLER)
//---------------------------------------------------------------------------
void CustomConfigInit(WEB_SERVER_CUST_CFG_INFO *CustCfgInfoPtr)
{
	SERVER_CUSTOM_CONFIG *CustCfgPtr = NULL;

	CustCfgPtr = CustCfgInfoPtr->BaseServCustCfg;
	CustCfgPtr->UserAuthRegNonUserSale = false;
	CustCfgPtr->SetManNamePrefix = false;
	CustCfgPtr->SetLastNameField = false;
	CustCfgPtr->ReqLastNameField = false;
	CustCfgPtr->SetMiddleNameField = false;
	CustCfgPtr->ReqMiddleNameField = false;
	CustCfgPtr->SetUserCityField = false;
	CustCfgPtr->ReqUserCityField = false;
	CustCfgPtr->SetUserZipField = false;
	CustCfgPtr->ReqUserZipField = false;
	CustCfgPtr->SetUserCountryField = false;
	CustCfgPtr->ReqUserCountryField = false;
    CustCfgPtr->SetUserFaxField = false;
	CustCfgPtr->ReqUserFaxField = false;
	CustCfgPtr->ReqUserEmailFiled = false;
	CustCfgPtr->SetCompanyName = false;
	CustCfgPtr->SetUserMobPhoneField = false;
	CustCfgPtr->ReqUserMobPhoneField = false;
    CustCfgPtr->SetUserAddr1Field = false;
	CustCfgPtr->ReqUserAddr1Field = false;
    CustCfgPtr->SetUserAddr2Field = false;
    CustCfgPtr->ReqUserAddr2Field = false;
	CustCfgPtr->AutoOfferGen = true;
	CustCfgPtr->AnonymTimoutExpInfo = false;
	CustCfgPtr->UsePaidDelivery = true;
    CustCfgPtr->SetUserLandPhoneField = false;
    CustCfgPtr->ReqUserLandPhoneField = false;

	CustCfgPtr->FileRequestLogging = false;

    CustCfgPtr->SessionStartLanguage = LGT_ENGLISH;
	CustCfgPtr->HourStartSmsNotifyDelay = 22;
	CustCfgPtr->HourStopSmsNotifyDelay = 9;
	CustCfgPtr->AdminSessionTimeout = 7200;
	CustCfgPtr->UserSessionTimeout = 1200;
	CustCfgPtr->AnonymSessionTimeout = 600;

	CustCfgPtr->NumReaderWorkers = 4;

    strcpy(CustCfgPtr->TableBgColor,       "#49639c");
    strcpy(CustCfgPtr->TableBgColor2,      "#395054");
    strcpy(CustCfgPtr->OnObjMouseOverColor,"#4f709d");
    strcpy(CustCfgPtr->OnObjClickColor,    "#FF0505");
    strcpy(CustCfgPtr->DownCentrBgColor,   "#ffffff");
    strcpy(CustCfgPtr->ServConfigBgColor1, "#293738");
    strcpy(CustCfgPtr->ServConfigBgColor2, "#42555A");

    /* CSS template colors */
    strcpy(CustCfgPtr->WorkZoneBgColor, "#688cec");
    strcpy(CustCfgPtr->MenuGrpBgColor,  "#49639c");
    strcpy(CustCfgPtr->GrpTitleBgColor, "#4f709d");
    strcpy(CustCfgPtr->GrpContBgColor,  "#b7c6db");
    strcpy(CustCfgPtr->MenuZoneBgColor, "#6923ac");
    strcpy(CustCfgPtr->GrpBorderColor,  "#92a9c9");

    /* Images */
    strcpy(CustCfgPtr->ImgBgSiteGround, "../images/groundLink.gif");
    strcpy(CustCfgPtr->ImgProductLogo,  "../images/sample_logo.png");
    strcpy(CustCfgPtr->ImgBgSideColumn, "../images/BgImgSideColumn0926201402.png");
    strcpy(CustCfgPtr->ImgNavBar,       "../images/navbar0603201402.png");
    strcpy(CustCfgPtr->ImgBgWorkZone,   "../images/WorkZoneBgImage0320201403.png");
    strcpy(CustCfgPtr->ImgBgDownCentr,  "../images/BgImageDwnCentr0320201401.png");

	strcpy(CustCfgPtr->ImgBgHeaderDesc, "images/Header_desc_0704201701.png");
	strcpy(CustCfgPtr->ImgBgHeaderM320, "images/Header_320_0704201701.png");
	strcpy(CustCfgPtr->ImgBgHeaderM360, "images/Header_360_0704201701.png");
	strcpy(CustCfgPtr->ImgBgHeaderM400, "images/Header_400_0704201701.png");
/*
	strcpy(CustCfgPtr->ImgBgHeaderDesc, "images/Header_desc_0407201601.jpg");
	strcpy(CustCfgPtr->ImgBgHeaderM320, "images/Header_320_0926201401.jpg");
	strcpy(CustCfgPtr->ImgBgHeaderM360, "images/Header_360_0926201401.jpg");
	strcpy(CustCfgPtr->ImgBgHeaderM400, "images/Header_400_0926201401.jpg");
*/
	strcpy(CustCfgPtr->ImgShopView, "images/spb_view.jpg");

    /* Server ports */
    CustCfgPtr->WebServMsgPort = WEBSERV_MSG_RX_PORT;
    CustCfgPtr->BaseSenderPort = BASE_SEND_NOTIFY_PORT;
    CustCfgPtr->BaseReaderPort = BASE_READ_NOTIFY_PORT;
    CustCfgPtr->BaseRemConnPort = BASE_REM_CONN_PORT;

#ifdef _LINUX_X86_
    CustCfgPtr->SudoSet = false;
#endif
    CustCfgPtr->DemoMode = false;
   
	/* DDos prevention */
	CustCfgPtr->DDosDetectTreshold = 500;
	CustCfgPtr->DDosIpLockTime = 3600;
	CustCfgPtr->DDosIpFreeTime = 3600;

	/* Port forward */
	CustCfgPtr->ForwardPortDdosProtect = false;
	CustCfgPtr->ForwardPortHttpSecure = false;
	CustCfgPtr->ForwardIpPort = 0;
	memset(CustCfgPtr->ForwardUrl, 0, MAX_LEN_URL_SERVER);
	memset(CustCfgPtr->ForwardPortSertFile, 0, MAX_LEN_PATH_NAME);
	memset(CustCfgPtr->ForwardPortKeyFile, 0, MAX_LEN_PATH_NAME);
    memset(CustCfgPtr->ForwardLocalIPAddrServ, 0, MAX_LEN_IP_ADDR);
    memset(CustCfgPtr->ForwardLocalHostName, 0, MAX_LEN_URL_SERVER);

    /* Web access */
    CustCfgPtr->SecWebPort = false;
	CustCfgPtr->PrimContentDelivery = false;
	CustCfgPtr->SecondContentDelivery = false;
    CustCfgPtr->PrimPortKeyAccess = false;
	CustCfgPtr->PrimPortHttpSecure = false;
	CustCfgPtr->PrimPortDdosProtect = false;
	CustCfgPtr->SecondPortHttpSecure = false;
    CustCfgPtr->SecondPortKeyAccess = true;
    CustCfgPtr->SecondPortInfoEdit = false;
	CustCfgPtr->SecondPortDdosProtect = false;
    CustCfgPtr->SecondWebAccIPPort = DEF_HTTP_IP_PORT;
    CustCfgPtr->SecondExtServIPPort = DEF_HTTP_IP_PORT;
    memset(CustCfgPtr->SecondLocalIPAddrServ, 0, MAX_LEN_IP_ADDR);
    memset(CustCfgPtr->SecondLocalHostName, 0, MAX_LEN_URL_SERVER);
    CustCfgPtr->PrimWebAccIPPort = DEF_HTTP_IP_PORT;
    CustCfgPtr->PrimExtServIPPort = DEF_HTTP_IP_PORT;
    memset(CustCfgPtr->PrimLocalIPAddrServ, 0, MAX_LEN_IP_ADDR);
    memset(CustCfgPtr->PrimLocalHostName, 0, MAX_LEN_URL_SERVER);
	memset(CustCfgPtr->PrimPortSertFile, 0, MAX_LEN_PATH_NAME);
	memset(CustCfgPtr->PrimPortKeyFile, 0, MAX_LEN_PATH_NAME);
	memset(CustCfgPtr->SecondPortSertFile, 0, MAX_LEN_PATH_NAME);
	memset(CustCfgPtr->SecondPortKeyFile, 0, MAX_LEN_PATH_NAME);
	memset(CustCfgPtr->PrimContentRootDir, 0, MAX_LEN_PATH_NAME);
	memset(CustCfgPtr->SecondContentRootDir, 0, MAX_LEN_PATH_NAME);

	CustCfgPtr->FileHashDirList.Count = 0;
    CustCfgPtr->FileHashDirList.CurrTask = NULL;
	CustCfgPtr->FileHashDirList.FistTask = NULL;
}
//---------------------------------------------------------------------------
void CustomConfigLoad(WEB_SERVER_CUST_CFG_INFO *CustCfgInfoPtr)
{
    struct stat   st;
    int           pos, c;    
	FILE          *FileHandler;
	char          *RetCodeCwd = NULL;
	char          CmdBuf[MAX_LEN_CONFIG_BASE_LINE + 1];
	char          DbTextItem[MAX_LEN_CONFIG_BASE_LINE+1];
	char          StartPath[512];
	char          BdFileName[1024];

#ifdef WIN32
	RetCodeCwd = _getcwd((char*)(&StartPath[0]),512);
#else
	RetCodeCwd = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, CustomConfigNamePath);
    stat(BdFileName, &st);
    if ((st.st_mode & S_IFMT) == S_IFMT)
    {
        printf("File WEB Server config (%s) is not file\n", BdFileName);
        return;
    }

	FileHandler = fopen(BdFileName,"r");        
    if(FileHandler) 
    {
        do 
        { 
            /* read all lines in config file */
            pos = 0;
            do
			{ 
                /* read one line */
                c = fgetc(FileHandler);
                if((c != EOF) && (pos < MAX_LEN_CONFIG_BASE_LINE))
                    DbTextItem[pos++] = (char)c;
			} while((c != EOF) && (c != '\n'));
            DbTextItem[pos] = 0;
			if (*DbTextItem !='\r' && *DbTextItem !='\n' && *DbTextItem !='#')
		    {
				if (!CmdLineParse(DbTextItem, TablCustomCfgCmdDb,
					CUSTOM_CFG_LEN, CustCfgInfoPtr->BaseServCustCfg, &CmdBuf[0]))
				{
					/* System WEB serv command not found */
					ExtCmdLineParse(DbTextItem, CustCfgInfoPtr, &CmdBuf[0]);
				}
		    }
		} while(c != EOF); 
        fclose(FileHandler);
    }
	else
	{
        printf("File WEB Server config.(%s) dos not present (Default settings are set)\n", BdFileName);
	}
}
//---------------------------------------------------------------------------
static bool CmdLineParse(char *BufRequest, DEF_CMD_HANDLER *TablArrCMD, 
    unsigned int AllTestCMD, SERVER_CUSTOM_CONFIG *ServCfgPtr, char *CmdBufPtr)
{
	bool			CmpPar;
	unsigned int	i,j,m,n;
	unsigned int	*LenCmds;
	char            *FText;

	if ( !AllTestCMD || !BufRequest || !TablArrCMD ) return false;
	LenCmds = (unsigned int*)AllocateMemory(sizeof(unsigned int)*AllTestCMD);
	for (i=0;i < AllTestCMD;i++) LenCmds[i] = strlen(TablArrCMD[i].CommandName);
	m = n = strlen(BufRequest);
	for (i=0;i < m;i++)
	{
		for (CmpPar=false, j=0;j < AllTestCMD;j++)
		{
			if (LenCmds[j] <= n)
			{
				if (strncmp(&BufRequest[i], TablArrCMD[j].CommandName, LenCmds[j]) == 0)
			    {
					strcpy(CmdBufPtr, &BufRequest[i+LenCmds[j]]);
					FText = ParseParFunction(CmdBufPtr);
                    if (FText) (TablArrCMD[j].FuncOnHandle)(ServCfgPtr, FText);
					FreeMemory(LenCmds);
					return true;
				}
			    CmpPar = true;
			}
		}
		if (!CmpPar) break;
		n--;
	}
	FreeMemory(LenCmds);
	return false;
}
//---------------------------------------------------------------------------
static void ExtCmdLineParse(char *BufRequest, 
	WEB_SERVER_CUST_CFG_INFO *CustCfgInfoPtr, char *CmdBufPtr)
{
	bool			CmpPar;
	unsigned int	i,j,m,n;
	unsigned int	*LenCmds;
	char            *FText;

	if ( !CustCfgInfoPtr->AllCustCmd || !BufRequest || !CustCfgInfoPtr->TablCustCmd ) return;
	LenCmds = (unsigned int*)AllocateMemory(sizeof(unsigned int)*CustCfgInfoPtr->AllCustCmd);
	for (i=0;i < CustCfgInfoPtr->AllCustCmd;i++) LenCmds[i] = strlen(CustCfgInfoPtr->TablCustCmd[i].CommandName);
	m = n = strlen(BufRequest);
	for (i=0;i < m;i++)
	{
		for (CmpPar=false, j=0;j < CustCfgInfoPtr->AllCustCmd;j++)
		{
			if (LenCmds[j] <= n)
			{
				if (strncmp(&BufRequest[i], CustCfgInfoPtr->TablCustCmd[j].CommandName, LenCmds[j]) == 0)
			    {
					strcpy(CmdBufPtr, &BufRequest[i+LenCmds[j]]);
					FText = ParseParFunction(CmdBufPtr);
                    if (FText) (CustCfgInfoPtr->TablCustCmd[j].FuncOnHandle)(CustCfgInfoPtr->ExtCustCfgPtr, FText);
					FreeMemory(LenCmds);
					return;
				}
			    CmpPar = true;
			}
		}
		if (!CmpPar) break;
		n--;
	}
	FreeMemory(LenCmds);
	return;
}
//---------------------------------------------------------------------------
void HandleComment(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
}
//---------------------------------------------------------------------------
static void HandleUserAuthRegNonUserSale(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->UserAuthRegNonUserSale = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetManNamePrefix(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetManNamePrefix = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetLastNameField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetLastNameField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqLastNameField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqLastNameField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetMiddleNameField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetMiddleNameField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqMiddleNameField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqMiddleNameField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetUserCityField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetUserCityField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqUserCityField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqUserCityField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetUserZipField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetUserZipField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqUserZipField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqUserZipField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetUserCountryField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetUserCountryField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqUserCountryField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqUserCountryField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetUserFaxField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetUserFaxField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqUserFaxField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqUserFaxField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqUserEmailFiled(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqUserEmailFiled = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetCompanyName(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetCompanyName = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetUserMobPhoneField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetUserMobPhoneField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqUserMobPhoneField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqUserMobPhoneField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetUserLandPhoneField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetUserLandPhoneField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqUserLandPhoneField(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqUserLandPhoneField = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetUserAddr1Field(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetUserAddr1Field = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqUserAddr1Field(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqUserAddr1Field = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSetUserAddr2Field(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SetUserAddr2Field = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleReqUserAddr2Field(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ReqUserAddr2Field = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleTableBgColor1(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->TableBgColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleTableBgColor2(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->TableBgColor2[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleOnObjMouseOverColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->OnObjMouseOverColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleOnObjClickColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->OnObjClickColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleDownCentrBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->DownCentrBgColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleServConfigBgColor1(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->ServConfigBgColor1[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleServConfigBgColor2(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->ServConfigBgColor2[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleWorkZoneBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->WorkZoneBgColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleMenuGrpBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->MenuGrpBgColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleGrpTitleBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->GrpTitleBgColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleGrpContBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->GrpContBgColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleMenuZoneBgColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->MenuZoneBgColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleGrpBorderColor(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_COLOR_STR) return;
	strncpy((char*)&CustCfgPtr->GrpBorderColor[0],
        (const char*)CmdLinePtr, MAX_LEN_COLOR_STR);
}
//---------------------------------------------------------------------------
static void HandleImgBgSiteGround(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgBgSiteGround[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgProductLogo(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgProductLogo[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgBgSideColumn(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgBgSideColumn[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgNavBar(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgNavBar[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgBgWorkZone(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgBgWorkZone[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgBgDownCentr(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgBgDownCentr[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgBgHeaderDesc(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgBgHeaderDesc[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgBgHeaderM320(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgBgHeaderM320[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgBgHeaderM360(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgBgHeaderM360[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgBgHeaderM400(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgBgHeaderM400[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleImgShopView(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PICTURE_PATH_STR) return;
	strncpy((char*)&CustCfgPtr->ImgShopView[0],
        (const char*)CmdLinePtr, MAX_LEN_PICTURE_PATH_STR);
}
//---------------------------------------------------------------------------
static void HandleAutoOfferGen(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->AutoOfferGen = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleHourStartSmsNotifyDelay(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 23)) break;
		CustCfgPtr->HourStartSmsNotifyDelay = ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleHourStopSmsNotifyDelay(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 23)) break;
		CustCfgPtr->HourStopSmsNotifyDelay = ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleAnonymTimoutExpInfo(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->AnonymTimoutExpInfo = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleUsePaidDelivery(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->UsePaidDelivery = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleAdminSessionTimeout(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 60) || (ReadValue > 65000)) break;
		CustCfgPtr->AdminSessionTimeout = (unsigned int)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleUserSessionTimeout(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 60) || (ReadValue > 65000)) break;
		CustCfgPtr->UserSessionTimeout = (unsigned int)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleAnonymSessionTimeout(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 60) || (ReadValue > 65000)) break;
		CustCfgPtr->AnonymSessionTimeout = (unsigned int)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecWebPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SecWebPort = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecondWebAccIPPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < DEF_HTTP_IP_PORT) || (ReadValue > 60000)) break;
		CustCfgPtr->SecondWebAccIPPort = (unsigned short)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecondExtServIPPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 1000) || (ReadValue > 60000)) break;
		CustCfgPtr->SecondExtServIPPort = (unsigned short)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecondLocalIPAddrServ(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_IP_ADDR) return;
	strncpy((char*)&CustCfgPtr->SecondLocalIPAddrServ[0],
        (const char*)CmdLinePtr, MAX_LEN_IP_ADDR);
}
//---------------------------------------------------------------------------
static void HandleSecondLocalHostName(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_URL_SERVER) return;
	strncpy((char*)&CustCfgPtr->SecondLocalHostName[0],
        (const char*)CmdLinePtr, MAX_LEN_URL_SERVER);
}
//---------------------------------------------------------------------------
static void HandlePrimPortKeyAccess(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->PrimPortKeyAccess = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecondPortKeyAccess(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SecondPortKeyAccess = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecondPortInfoEdit(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SecondPortInfoEdit = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
#ifdef _LINUX_X86_
static void HandleSudoSet(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SudoSet = (bool)ReadValue;
		break;
	}
}
#endif
//---------------------------------------------------------------------------
static void HandlePrimWebAccIPPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < DEF_HTTP_IP_PORT) || (ReadValue > 60000)) break;
		CustCfgPtr->PrimWebAccIPPort = (unsigned short)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandlePrimExtServIPPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < DEF_HTTP_IP_PORT) || (ReadValue > 60000)) break;
		CustCfgPtr->PrimExtServIPPort = (unsigned short)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandlePrimLocalIPAddrServ(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_IP_ADDR) return;
	strncpy((char*)&CustCfgPtr->PrimLocalIPAddrServ[0],
        (const char*)CmdLinePtr, MAX_LEN_IP_ADDR);
}
//---------------------------------------------------------------------------
static void HandlePrimLocalHostName(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_URL_SERVER) return;
	strncpy((char*)&CustCfgPtr->PrimLocalHostName[0],
        (const char*)CmdLinePtr, MAX_LEN_URL_SERVER);
}
//---------------------------------------------------------------------------
static void HandleStartSessionLanguage(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < MIN_LANG_INDEX) || (ReadValue > MAX_LANG_INDEX)) break;
		CustCfgPtr->SessionStartLanguage = (unsigned char)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleDemoModeSet(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->DemoMode = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandlePrimPortHttpSecure(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->PrimPortHttpSecure = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecondPortHttpSecure(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SecondPortHttpSecure = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecondPortSertFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PATH_NAME) return;
	strncpy((char*)&CustCfgPtr->SecondPortSertFile[0],
        (const char*)CmdLinePtr, MAX_LEN_PATH_NAME);
}
//---------------------------------------------------------------------------
static void HandleSecondPortKeyFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PATH_NAME) return;
	strncpy((char*)&CustCfgPtr->SecondPortKeyFile[0],
        (const char*)CmdLinePtr, MAX_LEN_PATH_NAME);
}
//---------------------------------------------------------------------------
static void HandlePrimPortSertFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PATH_NAME) return;
	strncpy((char*)&CustCfgPtr->PrimPortSertFile[0],
        (const char*)CmdLinePtr, MAX_LEN_PATH_NAME);
}
//---------------------------------------------------------------------------
static void HandlePrimPortKeyFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PATH_NAME) return;
	strncpy((char*)&CustCfgPtr->PrimPortKeyFile[0],
        (const char*)CmdLinePtr, MAX_LEN_PATH_NAME);
}
//---------------------------------------------------------------------------
static void HandlePrimPortDdosProtect(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->PrimPortDdosProtect = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecondPortDdosProtect(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SecondPortDdosProtect = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleDDosDetectTreshold(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 50) || (ReadValue > 10000)) break;
		CustCfgPtr->DDosDetectTreshold = (unsigned char)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleDDosIpLockTime(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 60) || (ReadValue > 60000)) break;
		CustCfgPtr->DDosIpLockTime = (unsigned int)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleDDosIpFreeTime(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 60) || (ReadValue > 60000)) break;
		CustCfgPtr->DDosIpFreeTime = (unsigned int)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleForwardIpPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 50000)) break;
		CustCfgPtr->ForwardIpPort = (unsigned short)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleForwardUrl(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_URL_SERVER) return;
	strncpy((char*)&CustCfgPtr->ForwardUrl[0],
        (const char*)CmdLinePtr, MAX_LEN_URL_SERVER);
}
//---------------------------------------------------------------------------
static void HandleForwardPortDdosProtect(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ForwardPortDdosProtect = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleForwardPortHttpSecure(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->ForwardPortHttpSecure = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleForwardPortSertFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PATH_NAME) return;
	strncpy((char*)&CustCfgPtr->ForwardPortSertFile[0],
        (const char*)CmdLinePtr, MAX_LEN_PATH_NAME);
}
//---------------------------------------------------------------------------
static void HandleForwardPortKeyFile(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PATH_NAME) return;
	strncpy((char*)&CustCfgPtr->ForwardPortKeyFile[0],
        (const char*)CmdLinePtr, MAX_LEN_PATH_NAME);
}
//---------------------------------------------------------------------------
static void HandleForwardLocalIPAddrServ(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_IP_ADDR) return;
	strncpy((char*)&CustCfgPtr->ForwardLocalIPAddrServ[0],
        (const char*)CmdLinePtr, MAX_LEN_IP_ADDR);
}
//---------------------------------------------------------------------------
static void HandleForwardLocalHostName(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_URL_SERVER) return;
	strncpy((char*)&CustCfgPtr->ForwardLocalHostName[0],
        (const char*)CmdLinePtr, MAX_LEN_URL_SERVER);
}
//---------------------------------------------------------------------------
static void HandlePrimContentDelivery(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->PrimContentDelivery = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleSecondContentDelivery(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->SecondContentDelivery = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandlePrimContentRootDir(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PATH_NAME) return;
	strncpy((char*)&CustCfgPtr->PrimContentRootDir[0],
        (const char*)CmdLinePtr, MAX_LEN_PATH_NAME);
}
//---------------------------------------------------------------------------
static void HandleSecondContentRootDir(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
    if (strlen(CmdLinePtr) > MAX_LEN_PATH_NAME) return;
	strncpy((char*)&CustCfgPtr->SecondContentRootDir[0],
        (const char*)CmdLinePtr, MAX_LEN_PATH_NAME);
}
//---------------------------------------------------------------------------
static void HandleAddDirHash(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	bool Result = false;
	int  pars_read, ReadValue;
	HASH_DIR_INFO   *HashDirPtr = NULL;

	if (strlen(CmdLinePtr) > (MAX_LEN_PATH_NAME+4)) return;
	for(;;)
	{
		HashDirPtr = (HASH_DIR_INFO*)AllocateMemory(sizeof(HASH_DIR_INFO));
		if (!HashDirPtr) break;
		pars_read = sscanf(CmdLinePtr, "%u, %s", &ReadValue, &HashDirPtr->HashDir[0]);
	    if (pars_read < 2) break;
		if ((ReadValue < BASE_HASH_LIST) || (ReadValue > SECOND_CONTENT_LIST)) break;
		HashDirPtr->DirType = (HashDirTypeT)ReadValue;
		AddStructList(&CustCfgPtr->FileHashDirList, HashDirPtr);
		Result = true;
		break;
	}
	if (!Result && HashDirPtr) FreeMemory(HashDirPtr);
}
//---------------------------------------------------------------------------
static void HandleNumReaderWorkers(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 2) || (ReadValue > 64)) break;
		CustCfgPtr->NumReaderWorkers = (unsigned char)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleBaseReaderPort(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 1000) || (ReadValue > 60000)) break;
		CustCfgPtr->BaseReaderPort = (unsigned int)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
static void HandleFileRequestLogging(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdLinePtr)
{
	int  pars_read, ReadValue;

	for(;;)
	{
	    pars_read = sscanf(CmdLinePtr, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		CustCfgPtr->FileRequestLogging = (bool)ReadValue;
		break;
	}
}
//---------------------------------------------------------------------------
