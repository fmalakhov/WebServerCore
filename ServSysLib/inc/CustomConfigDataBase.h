# if ! defined( CustomConfigDataBaseH )
#	define CustomConfigDataBaseH	/* only include me once */

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

#ifndef SysWebFunctionH
#include "SysWebFunction.h"
#endif

#ifndef WebServInfoH
#include "WebServInfo.h"
#endif

#define MAX_LEN_COLOR_STR          7
#define MAX_LEN_PICTURE_PATH_STR   255
#define MAX_LEN_IP_ADDR            16
#define MAX_LEN_PATH_NAME          512

typedef struct {
	/* User's form for order submit */
	bool          UserAuthRegNonUserSale; /* Flag of user's request of user auth if order was rased by anonym user */
	bool          SetManNamePrefix;       /* Flag of name prefix like Ser, .. set priore name */
	bool          SetLastNameField;       /* Flag of enable for fill last name of customer */
	bool          ReqLastNameField;       /* Flag of required fillout of last name field by customer */
	bool          SetMiddleNameField;     /* Flag of enable for fill middle name of customer */
	bool          ReqMiddleNameField;     /* Flag of required fillout of middle name field by customer */
	bool          SetUserCityField;       /* Flag of enable for fill user's city name */
	bool          ReqUserCityField;       /* Flag of required fillout of city name */
	bool          SetUserZipField;        /* Flag of enable for fill user's zip code */
	bool          ReqUserZipField;        /* Flag of required fillout of zip code */
	bool          SetUserCountryField;    /* Flag of enable for fill user's country */
	bool          ReqUserCountryField;    /* Flag of required fillout of user's country  */
	bool          SetUserFaxField;        /* Flag of enable for fill user's fax */
    bool          ReqUserFaxField;        /* Flag of required fillout of user's fax  */
	bool          ReqUserEmailFiled;      /* Flag of required fillout of user's e-mail  */
	bool          SetCompanyName;         /* Flag of enable for fill user's company name */
	bool          SetUserMobPhoneField;   /* Flag of enable for fill user's mobile phone num. */
	bool          ReqUserMobPhoneField;   /* Flag of required fillout of mobile phone num. */
	bool          SetUserAddr1Field;      /* Flag of enable for fill user's part of address 1. */
    bool          ReqUserAddr1Field;      /* Flag of required fillout of user's part of address 1. */
	bool          SetUserAddr2Field;      /* Flag of enable for fill user's part of address 2. */
	bool          ReqUserAddr2Field;      /* Flag of required fillout of user's part of address 2. */
	bool          AutoOfferGen;           /* Flag of enable automatical generation of additional offers for item based on sales base */
	bool          AnonymTimoutExpInfo;    /* Falg of anonym user notification about session timeout expiration */
	bool          UsePaidDelivery;        /* Flag of enable of paid delivery usage */
	bool          UseFreeDeliveryPrice;   /* Flag of enable free delivery if order price more set treshold */
    bool          SetUserLandPhoneField;  /* Flag of enable for fill user's land phone num. */
	bool          ReqUserLandPhoneField;  /* Flag of required fillout of land phone num. */

    char          TableBgColor[MAX_LEN_COLOR_STR+1];
    char          TableBgColor2[MAX_LEN_COLOR_STR+1];
    char          OnObjMouseOverColor[MAX_LEN_COLOR_STR+1];
    char          OnObjClickColor[MAX_LEN_COLOR_STR+1];
    char          DownCentrBgColor[MAX_LEN_COLOR_STR+1];
    char          ServConfigBgColor1[MAX_LEN_COLOR_STR+1];
    char          ServConfigBgColor2[MAX_LEN_COLOR_STR+1];

    /* CSS template colors */
    char          WorkZoneBgColor[MAX_LEN_COLOR_STR+1];
    char          MenuGrpBgColor[MAX_LEN_COLOR_STR+1];
    char          GrpTitleBgColor[MAX_LEN_COLOR_STR+1];
    char          GrpContBgColor[MAX_LEN_COLOR_STR+1];
    char          MenuZoneBgColor[MAX_LEN_COLOR_STR+1];
    char          GrpBorderColor[MAX_LEN_COLOR_STR+1];

	/* Other */
    unsigned char SessionStartLanguage;    /* The used language upon session startup */
	unsigned int  HourStartSmsNotifyDelay; /* Hour of begin delay SMS notification sent */
	unsigned int  HourStopSmsNotifyDelay;  /* Hour of stop delaty and enable SMS notify sent */
	unsigned int  AdminSessionTimeout;     /* Time out for admin's session */
	unsigned int  UserSessionTimeout;      /* Time out for user's session */
	unsigned int  AnonymSessionTimeout;    /* Time out for anonym's session */
    
    /* Images */
    char          ImgBgSiteGround[MAX_LEN_PICTURE_PATH_STR+1];
    char          ImgBgSideColumn[MAX_LEN_PICTURE_PATH_STR+1];
    char          ImgNavBar[MAX_LEN_PICTURE_PATH_STR+1];
    char          ImgBgWorkZone[MAX_LEN_PICTURE_PATH_STR+1];
    char          ImgBgDownCentr[MAX_LEN_PICTURE_PATH_STR+1];
	char          ImgBgHeaderDesc[MAX_LEN_PICTURE_PATH_STR+1];
	char          ImgBgHeaderM320[MAX_LEN_PICTURE_PATH_STR+1];
	char          ImgBgHeaderM360[MAX_LEN_PICTURE_PATH_STR+1];
	char          ImgBgHeaderM400[MAX_LEN_PICTURE_PATH_STR+1];
	char          ImgShopView[MAX_LEN_PICTURE_PATH_STR+1];
    char          ImgProductLogo[MAX_LEN_PICTURE_PATH_STR+1];

	/* Logging */
	bool          FileRequestLogging; /* Flag of logging of files requests */
	/* Workers */
	unsigned char NumReaderWorkers; /* Number of configured reader worker threads */

    /* Server internal ports */
    unsigned int  WebServMsgPort;  /* Web server rx port */
    unsigned int  BaseSenderPort;  /* Base port for sender threads */
    unsigned int  BaseReaderPort;  /* Base port for reader threads */
    unsigned int  BaseRemConnPort; /* Base port for remote connection threads */

	/* DDos prevention */
	unsigned int   DDosDetectTreshold;  /* The treshold of number of packets from same IP address per second */
	unsigned int   DDosIpLockTime;      /* Time of access lock from IP address in seconds */
	unsigned int   DDosIpFreeTime;      /* Time of IP record remove in seconds */

	/* Prot URL forward channel */
	bool           ForwardPortDdosProtect; /* Flag of forward channel protection from DDOS */
	bool           ForwardPortHttpSecure;  /* Flag of secure HTTP over SSL */
	unsigned short ForwardIpPort;          /* Ip port for requests forward */
	char           ForwardUrl[MAX_LEN_URL_SERVER+1]; /* URL for forward */
    char           ForwardLocalIPAddrServ[MAX_LEN_IP_ADDR+1];   /* The local IP addres of forward channel */
    char           ForwardLocalHostName[MAX_LEN_URL_SERVER+1];  /* The local host name of forward channel */
	char           ForwardPortKeyFile[MAX_LEN_PATH_NAME+1];   /* The path and name for server key file */
	char           ForwardPortSertFile[MAX_LEN_PATH_NAME+1];  /* The path and name for sertificate file */

    /* Primary WEB channel */
	bool           PrimContentDelivery; /* Falg of content delivery server's channel mode set */
    bool           PrimPortKeyAccess;   /* Flag of access to primary channel via KEY */
	bool           PrimPortHttpSecure;  /* Flag of secure HTTP over SSL */
	bool           PrimPortDdosProtect; /* Flag of primary channel protection from DDOS */
    unsigned short PrimWebAccIPPort;    /* The WWW server IP access port */
    unsigned short PrimExtServIPPort;   /* The WWW server external IP access port */
    char           PrimLocalIPAddrServ[MAX_LEN_IP_ADDR+1];   /* The local IP addres of secondary channel */
    char           PrimLocalHostName[MAX_LEN_URL_SERVER+1];  /* The local host name of secondary channel */
	char           PrimPortSertFile[MAX_LEN_PATH_NAME+1];    /* The path and name for sertificate file */
	char           PrimPortKeyFile[MAX_LEN_PATH_NAME+1];     /* The path and name for server key file */
	char           PrimContentRootDir[MAX_LEN_PATH_NAME+1];  /* The path to root dir of primary channel context */

    /* Secondary WEB channel */
	bool           SecondContentDelivery; /* Falg of content delivery server's channel mode set */
    bool           SecWebPort;          /* Flag of secondary WEB port configuration */
    bool           SecondPortKeyAccess; /* Flag of access to secondary channel via KEY */
    bool           SecondPortInfoEdit;  /* Flag of personal data access from secondary channel */
	bool           SecondPortHttpSecure;/* Flag of secure HTTP over SSL */
	bool           SecondPortDdosProtect; /* Flag of secondary channel protection from DDOS */
    unsigned short SecondWebAccIPPort;  /* The WWW server IP access port */
    unsigned short SecondExtServIPPort; /* The WWW server external IP access port */
    char           SecondLocalIPAddrServ[MAX_LEN_IP_ADDR+1];   /* The local IP addres of secondary channel */
    char           SecondLocalHostName[MAX_LEN_URL_SERVER+1];  /* The local host name of secondary channel */
	char           SecondPortSertFile[MAX_LEN_PATH_NAME+1];    /* The path and name for sertificate file */
	char           SecondPortKeyFile[MAX_LEN_PATH_NAME+1];     /* The path and name for server key file */
	char           SecondContentRootDir[MAX_LEN_PATH_NAME+1];  /* The path to root dir of secondary channel context */
    
#ifdef _LINUX_X86_
    bool          SudoSet;
#endif
    bool          DemoMode; /* Flag of WEB server operation in demonstration mode */
	ListItsTask   FileHashDirList; /* List of directories to be hashed */
} SERVER_CUSTOM_CONFIG;

typedef enum {BASE_HASH_LIST=0x01, PRIM_CONTENT_LIST, SECOND_CONTENT_LIST} HashDirTypeT;

typedef struct {
	HashDirTypeT DirType;
	char         HashDir[MAX_LEN_PATH_NAME+5];
} HASH_DIR_INFO;

typedef void (*TCmdFunc)(SERVER_CUSTOM_CONFIG *CustCfgPtr, char *CmdPtr);
typedef void (*TExtCmdFunc)(void *CustCfgPtr, char *CmdPtr);

typedef struct  {
    char* 	    CommandName;
    TCmdFunc    FuncOnHandle;
} DEF_CMD_HANDLER;

typedef struct  {
    char* 	    CommandName;
    TExtCmdFunc FuncOnHandle;
} DEF_EXT_CMD_HANDLER;

typedef struct {
	unsigned int         AllCustCmd;
	DEF_EXT_CMD_HANDLER  *TablCustCmd;
	void                 *ExtCustCfgPtr;
	SERVER_CUSTOM_CONFIG *BaseServCustCfg;
} WEB_SERVER_CUST_CFG_INFO;

void CustomConfigInit(WEB_SERVER_CUST_CFG_INFO *CustCfgInfoPtr);
void CustomConfigLoad(WEB_SERVER_CUST_CFG_INFO *CustCfgInfoPtr);

//---------------------------------------------------------------------------
#endif  /* if ! defined( CustomConfigDataBaseH ) */
