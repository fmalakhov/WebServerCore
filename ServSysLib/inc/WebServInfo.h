#ifndef WebServInfoH
#define WebServInfoH /* only include me once */

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

/* Common user access types */
#define UAT_ADMIN   1
#define UAT_GUEST   2

#define BPL_HIDE_AREA_PGBEG 1
#define BPL_PAGE_DOWN_AREA  2
#define BPL_PAGE_RIGHT_AREA 3
#define MIN_BPL_TYPE_ID     BPL_HIDE_AREA_PGBEG
#define MAX_BPL_TYPE_ID     BPL_PAGE_RIGHT_AREA
#define MAX_RUS_LINES_LIST 1024

#define UPPER_RUS_A_CHAR  0xc0
#define LOWER_RUS_A_CHAR  0xe0
#define LOWER_RUS_YA_CHAR 0xff

#define MAX_DWLD_HTTP_FILE_LEN  4000000   /* Maximum size of file for download to client in bytes */
#define INTER_CONF_KEY_GEN_INT  60000

extern char BadIpDbNamePath[];
extern char HtmlSpaceTag[];

#define RAND_GEN_MAX 32767
#define RAND_GEN_MASK 32768

#define MAX_REGISTER_USERS          100000 /* Maximum number of registered user's */

#define USER_AUTH_ENCODE_PUBLIC_KEY_LEN  128
#define USER_AUTH_ENCODE_PRIVATE_KEY_LEN 4

#define MAX_ZIP_CODE_ID             999999
#define MAX_LEN_BANNER_NAME         32

#define MAX_LEN_USER_INFO_NAME      50
#define MAX_LEN_USER_INFO_USER_NAME 25
#define MAX_LEN_USER_INFO_EMAIL     100
#define MAX_LEN_USER_INFO_PASSWD    40
#define MAX_LEN_COMPANY_NAME        100
#define MAX_LEN_ADDR_1_NAME         128
#define MAX_LEN_ADDR_2_NAME         128
#define MAX_LEN_CITY_NAME           64
#define MAX_LEN_ZIP_CODE            6
#define MAX_LEN_PHONE_NUM           24
#define MAX_LEN_URL_SERVER          128
#define MAX_LEN_REGION_NAME         64
#define MAX_LOCATION_LEN            16
#define CONFIRM_KEY_LEN             64

#define CREATE_SIMULT_WEB_SENT_THR   32
#define START_SIMULT_WEB_READ_HDR    512
#define START_SIMULT_WEB_SENT_HDR    512

#define START_COMPRES_MEM_BLOCK_SIZE 100000
#define MAX_LEN_HTTP_REQ_FILE_NAME   512
#define MAX_LOAD_HTML_PAGE_SIZE      4096*1024
#define START_READ_WEB_BLK           60
#define TCP_TX_BLOCK_SIZE            8000
#define TCP_RX_BLOCK_SIZE            8000

#define RAND_GEN_MAX                 32767
#define RAND_GEN_MASK                32768
#define SECURE_RANGE_MIN             1000000
#define SECURE_RANGE_MAX             9999999

#define BASE_MAIN_WEB_MESS  WM_USER+150
#define WSU_CONNECTUSER     BASE_MAIN_WEB_MESS
#define WSU_STATEWEBSERV    BASE_MAIN_WEB_MESS+1
#define WSU_CLOSECONNWEB    BASE_MAIN_WEB_MESS+2
#define WSU_FINISHJOBTASK   BASE_MAIN_WEB_MESS+3
#define WSU_USERDATA        BASE_MAIN_WEB_MESS+4
#define WSU_RESPSENDCMPLT   BASE_MAIN_WEB_MESS+5
#define WSU_RESPMAILSENT    BASE_MAIN_WEB_MESS+6
#define WSU_RESPSMSSENT     BASE_MAIN_WEB_MESS+7
#define WSU_HBACTIONTHR     BASE_MAIN_WEB_MESS+8
#define WSU_HBURCTHR        BASE_MAIN_WEB_MESS+9
#define WSU_USRREMCONNIND   BASE_MAIN_WEB_MESS+10
#define WSU_CLOSEWEBSERVER  BASE_MAIN_WEB_MESS+11

#define URP_SUCCESS              0 /* User's HTTP request load and parse is completed */
#define URP_INV_FILE_NAME        1 /* User's HTTP request has invalid file name in request */
#define URP_NOT_SUPP_CONT_TYPE   2 /* Not supported content type */
#define URP_CONNECT_CLOSE_ERROER 3 /* Client Http load connect closed due to error */
#define URP_HTTP_REQ_DATA_LARGE  4 /* Too large http request is received from user's host */
#define URP_HTTP_RX_TIMEOUT      5 /* The tiomeout is detected till end of HTTP request is downloaded */
#define URP_BAD_1_REQ            6 /* Bad case 1 */
#define URP_BAD_2_REQ            7 /* Bad case 2 */
#define URP_BAD_3_REQ            8 /* Bad case 3 */
#define URP_BAD_4_REQ            9 /* Bad case 4 */
#define URP_BAD_5_REQ            10 /* Bad case 5 */
#define URP_BAD_6_REQ            11 /* Bad case 6 */
#define URP_BAD_7_REQ            12 /* Bad case 7 */
#define URP_BAD_8_REQ            13 /* Bad case 8 */
#define URP_BAD_9_REQ            14 /* Bad case 9 */
#define URP_NOT_SUPPORT_METHOD   15 /* HTTP request contains not supported method */
#define URP_FILE_NAME_TOO_LONG   16 /* HTTP request contains too long file name */
#define URP_NOT_SUPP_FILE_TYPE   17 /* HTTP request contains not supported file type */
#define URP_BAD_REQ              40 /* All other bad cases */

#define SDT_DESCTOP  1
#define SDT_MOBILE   2

/* Browser type constants */
#define UBT_GENERAL          1
#define UBT_MSIE             2
#define UBT_CROME            3
#define UBT_FIREFOX          4

/* HTTP request type constants */
#define HTR_UNKNOWN 0
#define HRT_GET     1
#define HTR_POST    2
#define HTR_HEAD    3

/* File request types */
#define FRT_HTML_PAGE        1
#define FRT_GIF_PIC          2
#define FRT_JPG_PIC          3
#define FRT_PNG_PIC          4
#define FRT_ICO_PIC          5
#define FRT_CSS_SCRIPT       6
#define FRT_JS_SCRIPT        7
#define FRT_TXT_DATA         8
#define FRT_XML_DATA         9
#define FRT_CSV_DATA         10
#define FRT_HTR_PAGE         11
#define FRT_PDF_DOC          12
#define FRT_WAV_SOUND        13
#define FRT_CAPCHA           20

#define LGT_ENGLISH 1
#define LGT_RUSSIAN 2
#define MIN_LANG_INDEX LGT_ENGLISH
#define MAX_LANG_INDEX LGT_RUSSIAN

#define MDT_PHONE    1
#define MDT_PAD      2

#define MAX_CAPCHA_SIZE        10000

/* Web server system message tags */
#define SITE_RUS_SMS_ALM_NOTIFY_TXT_1_LINE_ID   546
#define SITE_RUS_SMS_ALM_NOTIFY_TXT_2_LINE_ID   547
#define SITE_RUS_SMS_ALM_NOTIFY_TXT_3_LINE_ID   548
#define SITE_RUS_SMS_ALM_NOTIFY_TXT_4_LINE_ID   549

/* Forum related defines */
#define MPS_WAIT_APPROVAL  1
#define MPS_LOCAL_PUBLIST  2
#define MPS_GLOBAL_PUBLISH 3
#define MIN_MPS_VALUE MPS_WAIT_APPROVAL
#define MAX_MPS_VALUE MPS_GLOBAL_PUBLISH

#define MAX_LEN_FORUM_NAME  128
#define MAX_LEN_TOPIC_NAME  128
#define MAX_LEN_TOPIC_DESCR 128
#define MAX_LEN_USER_NAME   32
#define MAX_FORUM_MESSAGE_LEN 4096
#define MAX_TOPICS_FORUM    10000
#define MAX_MESSAGES_FORUM  100000
#define MAX_TOPIC_SEE_ARR_SIZE ((MAX_TOPICS_FORUM >> 3) + 1)

typedef struct {
	unsigned int  PublishStatus;
	unsigned int  ForumId;
	unsigned int  TopicId;
    unsigned int  MessageId;
	unsigned int  UserId;
#ifdef WIN32         
	SYSTEMTIME    SubmitDate;
#else
    struct tm     SubmitDate;
#endif
	char          *Message;
	ObjListTask   *UserListObjPtr;
	ObjListTask   *BaseObjPtr;
	ObjListTask   *NewMsgObjPtr;
	char          UserName[MAX_LEN_USER_NAME+1];
} FORUM_MESSAGE_INFO;

typedef struct {
	bool          isAnonymModify;
	bool          isBotVisible;
	bool          isClosed;
	unsigned int  ForumId;
	unsigned int  TopicId;
	unsigned int  ParentTopicId;
	unsigned int  ModeratorId;
	unsigned int  MessageCount;
	unsigned int  ChaildTopicCount;
	unsigned int  ViewCount;
	char          TopicName[MAX_LEN_TOPIC_NAME+1];
	char          TopicDescr[MAX_LEN_TOPIC_DESCR+1];
	ListItsTask   ChaildTopicList;
	ListItsTask   MessageList;
	FORUM_MESSAGE_INFO *LastMessagePtr;
} FORUN_TOPIC_INFO;

typedef struct {
    unsigned int  ForumId;
    char          ForumName[MAX_LEN_FORUM_NAME+1];
	ListItsTask   RootTopicList;
} FORUM_INFO;

/**** User's feedback related *****/
#define MAX_FEEDBACK_TEXT_LEN       1024

typedef struct {
	unsigned int FeedbackId;
	unsigned int ItemId;
	unsigned int UserId;
	unsigned char ItemRating;
#ifdef WIN32         
	SYSTEMTIME   SubmitDate;
#else       
    struct tm    SubmitDate;
#endif
	ObjListTask* ObjPtr;
	char         FeedbackDescr[MAX_FEEDBACK_TEXT_LEN+1];
} USER_FEEDBACK;


typedef struct {
	bool          isActive;
    unsigned char Location;
	char          Name[MAX_LEN_BANNER_NAME+1];
    char          *BodyPtr;
	ObjListTask   *ObjPtr;
} BANNER_INFO;

typedef struct {
	unsigned int MaxOpenSessions;
	unsigned int MaxSesionPerIP;
	bool         HtmlPageComprssEnable;
	unsigned int MinHtmlSizeCompress;
	bool         KeepAliveEnable;
	unsigned int KeepAliveTimeout;
} GENPARSERVER;

typedef struct {
	unsigned int ZipCode;
	char         Name[MAX_LEN_COMPANY_NAME+1];
	char         URL[MAX_LEN_URL_SERVER+1];
    char         Region[MAX_LEN_REGION_NAME+1];
	char         City[MAX_LEN_CITY_NAME+1];
	char         Address[MAX_LEN_ADDR_1_NAME+1];
	char         LandPhone[MAX_LEN_PHONE_NUM+1];
	char         MobilePhone1[MAX_LEN_PHONE_NUM+1];
	char         MobilePhone2[MAX_LEN_PHONE_NUM+1];
	char         FaxPhone[MAX_LEN_PHONE_NUM+1];
	char         LocLatitude[MAX_LOCATION_LEN+1];
	char         LocLongitude[MAX_LOCATION_LEN+1];
} PARSHOPINFO;

typedef struct {
	unsigned int  CountryId;
	unsigned int  ZipCode;
	unsigned char UserTitleId;
	char CompanyName[MAX_LEN_COMPANY_NAME+1];
	char FirstName[MAX_LEN_USER_INFO_NAME+1];
	char MiddleName[MAX_LEN_USER_INFO_NAME+1];
	char LastName[MAX_LEN_USER_INFO_NAME+1];
	char Address1[MAX_LEN_ADDR_1_NAME+1];
	char Address2[MAX_LEN_ADDR_2_NAME+1];
	char City[MAX_LEN_CITY_NAME+1];
	char LandPhone[MAX_LEN_PHONE_NUM];
	char MobilePhone[MAX_LEN_PHONE_NUM];
	char FaxPhone[MAX_LEN_PHONE_NUM];
	char Email[MAX_LEN_USER_INFO_EMAIL];
} CONTACT_INFO;

#define CONTACT_INFO_PACK_SIZE 2 + 3*UINT_PACK_SIZE + (MAX_LEN_COMPANY_NAME+1) + 3*(MAX_LEN_USER_INFO_NAME+1) +\
    2*(MAX_LEN_ADDR_1_NAME+1) + (MAX_LEN_CITY_NAME+1) + 3*(MAX_LEN_PHONE_NUM+1) + MAX_LEN_USER_INFO_EMAIL

typedef struct {
	unsigned int SumultReadSessions; /* Max. number of request read threads at the same time */
	unsigned int SimultSendSessions; /* Max. number of response sent threads at the same time */
	unsigned int UserSessions;       /* Number of opened user sessions */
	unsigned int BotSessions;        /* Number of opened bot sessions */
	unsigned int SimultOpenSessions; /* Max. number of open sessions at the same time */
	unsigned int SuccMailDelivery;   /* Number of successfully delivered mails */
	unsigned int FailMailDelivery;   /* Number of fail delivered mails */
	unsigned int SuccSmsDelivery;    /* Number of successfully delivered SMSs */
	unsigned int FailSmsDelivery;    /* Number of fail delivered SMSs */
	unsigned int SendHtmlPages;      /* Number of sent HTML pages */
	unsigned int SendFiles;          /* Number of sent files */
	unsigned int AvgPageGenTime;     /* Average time of HTML page generation */
	unsigned int MaxPageGenTime;     /* Maximum time of HTML page generation */
	unsigned int AvgReqLoadTime;     /* Average time of HTML request load */
	unsigned int MaxReqLoadTime;     /* Maximum time of HTML request load */
/* Per 15 min stats */
	unsigned int IntUserSessions;    /* Number of open user's sessions per stats interval */
	unsigned int IntGoodUserReq;     /* Number of good user's requersts per stats interval */
	unsigned int IntBotSessions;     /* Number of open bot's sessions per stats interval */
	unsigned int IntGoodBotReq;      /* Number of good bot's requersts per stats interval */
	unsigned int IntBadUserReq;      /* Number of failed by different reasons user's requests per stats interval */
	unsigned int IntSendHtmlPages;   /* Number of sent HTML pages per stats interval */
	unsigned int IntSendFiles;       /* Number of sent files per stats interval */
} STATS_INFO;

typedef void (*THtmlCmdHanler)(void *HtmlCmdParPtr);

typedef struct {
	unsigned char *HtmlCmdName;
	THtmlCmdHanler HtmlFunction;
} CMD_INFO;

void HandleCpuUtilMeasureTimerExp();

#endif
