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

#include "HttpParser.h"
#include "SessionKeyHash.h"
#include "MobileDeviceDataBase.h"
#include "BotDataBase.h"
#include "ThrReadWebUser.h"

extern char CookieMarker[];
extern char SessionCookieId[];
extern char DoubleNextLine[];
extern char BeginHttpReqMarker[];
extern char KeySessionId[];
extern char GenPageMain[];

static char* GetReqLineParse(HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr, char *HttpParserPtr);

static char* BrowserMsTypeSet(void *DataPtr, char *HttpParserPtr);
static char* BrowserCromeTypeSet(void *DataPtr, char *HttpParserPtr);
static char* BrowserFirefoxTypeSet(void *DataPtr, char *HttpParserPtr);
static char* MobileDeviceTypeSet(void *DataPtr, char *HttpParserPtr);
static char* GetRequestTypeSet(void *DataPtr, char *HttpParserPtr);
static char* HeadRequestTypeSet(void *DataPtr, char *HttpParserPtr);
static char* PostRequestTypeSet(void *DataPtr, char *HttpParserPtr);
static char* AccEncTypeSet(void *DataPtr, char *HttpParserPtr);
static char* ConnectTypeSet(void *DataPtr, char *HttpParserPtr);
static char* MozillaVerSet(void *DataPtr, char *HttpParserPtr);
static char* CookieSessionIdSet(void *DataPtr, char *HttpParserPtr);
static char* EndHttpCmdParse(void *DataPtr, char *HttpParserPtr);
static char* UserAgentInfoParse(void *DataPtr, char *HttpParserPtr);
static char* LineParseSkip(void *DataPtr, char *HttpParserPtr);
static char* EndHttpLineSet(char *HttpParserPtr);

static char* HtmpFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* GifFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* JpgFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* PngFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* IcoFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* CssFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* JsFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* TxtFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* XmlFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* CsvFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* HtrFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* PdfFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* WavFileTypeSet(void *DataPtr, char *HttpParserPtr);
static char* NotSupportMethod(void *DataPtr, char *HttpParserPtr);
static char* IfModifSinceParse(void *DataPtr, char *HttpParserPtr);
static char* EndSpaseHttpLineSet(char *HttpParserPtr);
static char* EndDataBlockHttpLineSet(char *HttpParserPtr);

static FIELD_EXTRACT_INFO DayWeekInfo[] = {
	{1, "Sun, ", 5},
	{2, "Mon, ", 5},
	{3, "Tue, ", 5},
	{4, "Wed, ", 5},
	{5, "Thu, ", 5},
	{6, "Fri, ", 5},
	{7, "Sat, ", 5}
};

static FIELD_EXTRACT_INFO MonthNameInfo[] = {
	{1,  "Jan", 3},
	{2,  "Feb", 3},
	{3,  "Mar", 3},
	{4,  "Apr", 3},
	{5,  "May", 3},
	{6,  "Jun", 3},
	{7,  "Jul", 3},
	{8,  "Aug", 3},
	{9,  "Sep", 3},
	{10, "Oct", 3},
	{11, "Now", 3},
	{12, "Dec", 3}
};

static char *TablDefExtFiles[] = {".html",".gif",".jpg", ".png", 
                                  ".ico", ".css", ".js", ".txt", 
						          ".xml", ".csv", ".htr", ".pdf",
                                  ".wav"};

MPTT_PARSER_DATA UserAgentFiledList[] = {
	"MSIE",    BrowserMsTypeSet,
	"Chrome",  BrowserCromeTypeSet,
	"Firefox", BrowserFirefoxTypeSet,
	"Android", MobileDeviceTypeSet,
	"Phone",   MobileDeviceTypeSet,
	"SymbOS",  MobileDeviceTypeSet
};

MPTT_PARSER_DATA HttpCmdFiledList[] = {
	"GET",                 GetRequestTypeSet,
	"HEAD",                HeadRequestTypeSet,
	"POST",                PostRequestTypeSet,
	"Accept-Encoding:",    AccEncTypeSet,
	"Connection:",         ConnectTypeSet,
	"Mozilla/",            MozillaVerSet,
	CookieMarker,          CookieSessionIdSet,
	"User-Agent: ",        UserAgentInfoParse,
	"If-Modified-Since:",  IfModifSinceParse,
	DoubleNextLine,        EndHttpCmdParse
};

MPTT_PARSER_DATA HttpCmdContextFiledList[] = {
	"GET",                 GetRequestTypeSet,
	"HEAD",                HeadRequestTypeSet,
	"POST",                NotSupportMethod,
	"Accept-Encoding:",    AccEncTypeSet,
	"Connection:",         ConnectTypeSet,
	"Mozilla/",            LineParseSkip,
	CookieMarker,          LineParseSkip,
	"User-Agent: ",        LineParseSkip,
	"If-Modified-Since:",  IfModifSinceParse,
	DoubleNextLine,        EndHttpCmdParse
};

static MPTT_PARSER_DATA HttpFileLineParseList[] = {
	".html",	HtmpFileTypeSet,
	"gif",		GifFileTypeSet,
	".jpg",		JpgFileTypeSet,
	".png",		PngFileTypeSet,
	".ico",		IcoFileTypeSet,
	".css",		CssFileTypeSet,
	".js",		JsFileTypeSet,
	".txt",		TxtFileTypeSet,
	".xml",		XmlFileTypeSet,
	".csv",		CsvFileTypeSet,
	".htr",		HtrFileTypeSet,
	".pdf",		PdfFileTypeSet,
    ".wav",		WavFileTypeSet
};

//---------------------------------------------------------------------------
void UserAgentMsgParserInit(MPTT_PARSER_MESSAGE *UserAgentParserMessage)
{
	UserAgentParserMessage->ListSize = sizeof(UserAgentFiledList)/sizeof(MPTT_PARSER_DATA);
	UserAgentParserMessage->MpttParserList = UserAgentFiledList;
    CreateMpttParserHash(UserAgentParserMessage);
}
//---------------------------------------------------------------------------
void UserAgentMsgParserDestroy(MPTT_PARSER_MESSAGE *UserAgentParserMessage)
{
	CloseMpttParserHash(UserAgentParserMessage);
}
//---------------------------------------------------------------------------
void HttpCmdMsgParserInit(MPTT_PARSER_MESSAGE *HttpCmdParserMessage)
{
	HttpCmdParserMessage->ListSize = sizeof(HttpCmdFiledList)/sizeof(MPTT_PARSER_DATA);
	HttpCmdParserMessage->MpttParserList = HttpCmdFiledList;
    CreateMpttParserHash(HttpCmdParserMessage);
}
//---------------------------------------------------------------------------
void HttpCmdMsgParserDestroy(MPTT_PARSER_MESSAGE *HttpCmdParserMessage)
{
	CloseMpttParserHash(HttpCmdParserMessage);
}
//---------------------------------------------------------------------------
void HttpCmdContentMsgParserInit(MPTT_PARSER_MESSAGE *HttpCmdParserMessage)
{
	HttpCmdParserMessage->ListSize = sizeof(HttpCmdContextFiledList)/sizeof(MPTT_PARSER_DATA);
	HttpCmdParserMessage->MpttParserList = HttpCmdContextFiledList;
    CreateMpttParserHash(HttpCmdParserMessage);
}
//---------------------------------------------------------------------------
void HttpCmdContentMsgParserDestroy(MPTT_PARSER_MESSAGE *HttpCmdParserMessage)
{
	CloseMpttParserHash(HttpCmdParserMessage);
}
//---------------------------------------------------------------------------
void HttpFileLineMsgParserInit(MPTT_PARSER_MESSAGE *HttpCmdParserMessage)
{
	HttpCmdParserMessage->ListSize = sizeof(HttpFileLineParseList)/sizeof(MPTT_PARSER_DATA);
	HttpCmdParserMessage->MpttParserList = HttpFileLineParseList;
    CreateMpttParserHash(HttpCmdParserMessage);
}
//---------------------------------------------------------------------------
void HttpFileLineMsgParserDestroy(MPTT_PARSER_MESSAGE *HttpCmdParserMessage)
{
	CloseMpttParserHash(HttpCmdParserMessage);
}
//---------------------------------------------------------------------------
static char* BrowserMsTypeSet(void *DataPtr, char *HttpParserPtr)
{
	HTTP_MSG_EXTRACT_DATA *UserAgentDataPtr;

	UserAgentDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	UserAgentDataPtr->BrowserType = UBT_MSIE;
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* BrowserCromeTypeSet(void *DataPtr, char *HttpParserPtr)
{
	HTTP_MSG_EXTRACT_DATA *UserAgentDataPtr;

	UserAgentDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	UserAgentDataPtr->BrowserType = UBT_CROME;
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* BrowserFirefoxTypeSet(void *DataPtr, char *HttpParserPtr)
{
	HTTP_MSG_EXTRACT_DATA *UserAgentDataPtr;

	UserAgentDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
    UserAgentDataPtr->BrowserType = UBT_FIREFOX;
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* MobileDeviceTypeSet(void *DataPtr, char *HttpParserPtr)
{
	HTTP_MSG_EXTRACT_DATA *UserAgentDataPtr;

	UserAgentDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
    UserAgentDataPtr->DeviceType = SDT_MOBILE;
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* NotSupportMethod(void *DataPtr, char *HttpParserPtr)
{
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	*HttpCmdDataPtr->LocalFileName = 0;
	HttpCmdDataPtr->Status = URP_NOT_SUPPORT_METHOD;
	return NULL;
}
//---------------------------------------------------------------------------
static char* GetReqLineParse(HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr, char *HttpParserPtr)
{
	register int  i, j, k;
	char          *SrcPtr;
	READWEBSOCK   *ParReadWeb;

	ParReadWeb = (READWEBSOCK*)HttpCmdDataPtr->ParReadWebPtr;
	i = strlen(HttpParserPtr);
	while ((i > 0) && *HttpParserPtr == ' ')
	{
		HttpParserPtr++;
		i--;
	}

	i = FindCmdRequestLine(HttpParserPtr, BeginHttpReqMarker);
	if (i != -1)
	{
		j = i - 4;

		/* Clean file name lengrh get */
		i = 0;
		SrcPtr = HttpParserPtr;
	    while (i < j)
	    {
		    if (*SrcPtr == ' ' ) break;
	        if (*SrcPtr == '?' ) break;
			SrcPtr++;
	        i++;
	    }

		/* File name length check */
		if (i > (MAX_LEN_HTTP_REQ_FILE_NAME-1))
		{
			*HttpCmdDataPtr->LocalFileName = 0;
			HttpCmdDataPtr->Status = URP_FILE_NAME_TOO_LONG;
			return NULL;
		}

		HttpCmdDataPtr->StrCmdHTTP = (char*)AllocateMemory(j + 256);
		CmdStrConvert(HttpCmdDataPtr->StrCmdHTTP, HttpParserPtr, j);

		(bool)MpttParserLineMsgExtract(&((READER_WEB_INFO*)HttpCmdDataPtr->ReaderInfoPtr)->HttpFileLineParserMessage, 
	        HttpCmdDataPtr, HttpCmdDataPtr->StrCmdHTTP);

		j = FindCmdRequestLine(HttpCmdDataPtr->StrCmdHTTP, KeySessionId);
		if ((j != -1) && strlen(&HttpCmdDataPtr->StrCmdHTTP[j]) >= SESSION_ID_KEY_LEN)
		{
			// Session key is present in command line
			HttpCmdDataPtr->CmdSesKeyPtr = &HttpCmdDataPtr->StrCmdHTTP[j];
		}

		/*
		for (j=0;j < 13;j++)
		{
			if  (FindCmdRequest(HttpCmdDataPtr->StrCmdHTTP, TablDefExtFiles[j]) != -1)
			{
				HttpCmdDataPtr->FileType = j + 1;
				break;
			}
		}
		*/
		if (FindCmdRequest(HttpCmdDataPtr->StrCmdHTTP, ".well-known/acme-challenge") != -1)
		{
			HttpCmdDataPtr->FileType = FRT_TXT_DATA;
			HttpCmdDataPtr->BotType = BOT_GOOGLE;
		}

		if (HttpCmdDataPtr->FileType > 0)
		{
			/* Set full path and name of file */
			strcpy(HttpCmdDataPtr->LocalFileName, ParReadWeb->ServRootDir);
			if (!ParReadWeb->isContentDelivery)
			{
			#ifdef WIN32
				strcat(HttpCmdDataPtr->LocalFileName, "\\WebData");
				if (HttpCmdDataPtr->FileType == FRT_ICO_PIC) strcat(HttpCmdDataPtr->LocalFileName, "\\images");
			#else
				strcat(HttpCmdDataPtr->LocalFileName, "/WebData");
				if (HttpCmdDataPtr->FileType == FRT_ICO_PIC) strcat(HttpCmdDataPtr->LocalFileName, "/images");
			#endif
			}

			HttpCmdDataPtr->NoPwdLocalNameShift = strlen(HttpCmdDataPtr->LocalFileName);
		#ifdef WIN32
			/* Needsd to change directory separators from Windows style */
			for (k=HttpCmdDataPtr->NoPwdLocalNameShift, j=0;j < i;j++)
			{
				if (HttpParserPtr[j] == '/') HttpCmdDataPtr->LocalFileName[k] = '\\';
				else                         HttpCmdDataPtr->LocalFileName[k] = HttpParserPtr[j];
				k++;
			}
			HttpCmdDataPtr->LocalFileName[k] = 0;
        #else
			memcpy(&HttpCmdDataPtr->LocalFileName[HttpCmdDataPtr->NoPwdLocalNameShift], HttpParserPtr, i);
			HttpCmdDataPtr->LocalFileName[HttpCmdDataPtr->NoPwdLocalNameShift + i] = 0;
        #endif
			HttpCmdDataPtr->NoPwdLocalNameShift++;

			/* File directory redirection check */
			if  (FindCmdRequest(HttpCmdDataPtr->LocalFileName, "./") != -1)
			{
				HttpCmdDataPtr->Status = URP_INV_FILE_NAME;
				return NULL;
			}
		}
		else
		{
			if (ParReadWeb->isContentDelivery)
			{
				HttpCmdDataPtr->NoPwdLocalNameShift = 0;
			    memcpy(HttpCmdDataPtr->LocalFileName, HttpParserPtr, i);
			    HttpCmdDataPtr->LocalFileName[i] = 0;
				HttpCmdDataPtr->Status = URP_INV_FILE_NAME;
				return NULL;
			}
			else
			{
			    strcpy(HttpCmdDataPtr->LocalFileName, ParReadWeb->ServRootDir);
		#ifdef WIN32
			    strcat(HttpCmdDataPtr->LocalFileName, "\\WebData\\");
		#else
			    strcat(HttpCmdDataPtr->LocalFileName, "/WebData/");
		#endif
			    HttpCmdDataPtr->NoPwdLocalNameShift = strlen(HttpCmdDataPtr->LocalFileName);
			    strcat(HttpCmdDataPtr->LocalFileName, GenPageMain);
			    HttpCmdDataPtr->FileType = FRT_HTML_PAGE;
			}
		}
		HttpParserPtr += i;
	}
	else
	{
		HttpCmdDataPtr->Status = URP_BAD_1_REQ;
		HttpParserPtr = NULL;
	}
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* GetRequestTypeSet(void *DataPtr, char *HttpParserPtr)
{
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	HttpCmdDataPtr->ReqestType = HRT_GET;
	return GetReqLineParse(HttpCmdDataPtr, HttpParserPtr);
}
//---------------------------------------------------------------------------
static char* HeadRequestTypeSet(void *DataPtr, char *HttpParserPtr)
{
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	HttpCmdDataPtr->ReqestType = HTR_HEAD;
	return GetReqLineParse(HttpCmdDataPtr, HttpParserPtr);
}
//---------------------------------------------------------------------------
static char* PostRequestTypeSet(void *DataPtr, char *HttpParserPtr)
{
	register int  i, j;
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	HttpCmdDataPtr->ReqestType = HTR_POST;

	i = strlen(HttpParserPtr);
	while ((i > 0) && *HttpParserPtr == ' ')
	{
		HttpParserPtr++;
		i--;
	}

	i = FindCmdRequestLine(HttpParserPtr, BeginHttpReqMarker);
	if (i != -1)
	{
		j = i - 4;
	    if (j > (MAX_LEN_HTTP_REQ_FILE_NAME-1))
		{
			HttpCmdDataPtr->Status = URP_FILE_NAME_TOO_LONG;
			HttpParserPtr = NULL;
		}
		else
		{
			CmdStrConvert(HttpCmdDataPtr->LocalFileName,  HttpParserPtr, j);
			HttpCmdDataPtr->NoPwdLocalNameShift = 1;
			for (j=0;j < 13;j++)
			{
				if  (FindCmdRequest(HttpCmdDataPtr->LocalFileName, TablDefExtFiles[j]) != -1)
				{
					HttpCmdDataPtr->FileType = j + 1;
					break;
				}
			}

            if (!HttpCmdDataPtr->FileType)
		    {
			    HttpCmdDataPtr->Status = URP_NOT_SUPP_FILE_TYPE;
				HttpParserPtr = NULL;
		    }
			else
			{
			    HttpParserPtr += i;
			}
		}
	}
	else
	{
		HttpCmdDataPtr->Status = URP_BAD_9_REQ;
		HttpParserPtr = NULL;
	}

	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* AccEncTypeSet(void *DataPtr, char *HttpParserPtr)
{
	register int          i;
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	i = FindCmdRequestLine(HttpParserPtr, "gzip");
	if (i != -1)
	{
		HttpCmdDataPtr->isEncodingAccept = true;
		HttpParserPtr += (unsigned int)i;
	}
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* ConnectTypeSet(void *DataPtr, char *HttpParserPtr)
{
    register int          i;
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	i = FindCmdRequestLine(HttpParserPtr, "Keep-Alive");
	if (i != -1)
	{
		HttpCmdDataPtr->isKeepAlive = true;
		HttpParserPtr += (unsigned int)i;
	}
	else
	{
		i = FindCmdRequestLine(HttpParserPtr, "keep-alive");
		if (i != -1)
		{
			HttpCmdDataPtr->isKeepAlive = true;
			HttpParserPtr += (unsigned int)i;
		}
	}
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* MozillaVerSet(void *DataPtr, char *HttpParserPtr)
{
	int                   pars_read, Par1Value, Par2Value;
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	pars_read = sscanf(HttpParserPtr, "%d.%d", &Par1Value, &Par2Value);
	if (pars_read > 1)
	{
        HttpCmdDataPtr->MozilaMainVer = (unsigned char)Par1Value;
        HttpCmdDataPtr->MozilaSubVer = (unsigned char)Par2Value;
	}
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* CookieSessionIdSet(void *DataPtr, char *HttpParserPtr)
{
	register int          i;
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	i = FindCmdRequestLine(HttpParserPtr, SessionCookieId);
    if (i != -1)
	{
		HttpCmdDataPtr->CookieSesKeyPtr = &HttpParserPtr[i];
		/* Cookie with session ID is detected */
		if (strlen(HttpCmdDataPtr->CookieSesKeyPtr) >= SESSION_ID_KEY_LEN)
            HttpCmdDataPtr->CookieSessionId = FindSessionByKey(HttpCmdDataPtr->CookieSesKeyPtr);
		HttpCmdDataPtr += (unsigned int)i;
	}
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
static char* EndHttpCmdParse(void *DataPtr, char *HttpParserPtr)
{
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	HttpCmdDataPtr->isParseDone = true;
	return NULL;
}
//---------------------------------------------------------------------------
static char* UserAgentInfoParse(void *DataPtr, char *HttpParserPtr)
{
	register char         CheckChar;
	register int          stln = 0;
	register char         *LnChkPtr = NULL;
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;
	READER_WEB_INFO       *ReaderInfoPtr;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	if (!HttpCmdDataPtr->ReaderInfoPtr) return NULL;
	ReaderInfoPtr = (READER_WEB_INFO*)HttpCmdDataPtr->ReaderInfoPtr;
	LnChkPtr = HttpParserPtr; 
	for(;;)
	{
		CheckChar = *LnChkPtr;
		if((CheckChar == 0) || (CheckChar == '\r') || (CheckChar == '\n')) break;
        LnChkPtr++;
        stln++;
	}

	if (!MpttParserLineMsgExtract(&ReaderInfoPtr->UserAgentParserMessage, DataPtr, HttpParserPtr)) return NULL;
	if (HttpCmdDataPtr->DeviceType == SDT_MOBILE)
	{
		/* Search mobile device in DB */
		HttpCmdDataPtr->MobileType = FindMobileDevice((unsigned char*)HttpParserPtr, stln);
	}

	/* Search robot (bot) type detection */
    HttpCmdDataPtr->BotType = FindBotInfoStrLine((unsigned char*)HttpParserPtr, stln);

    // For like bot testing  
/*
	// Test bot is detected 
	//HttpCmdDataPtr->BotType = BOT_YANDEX;
    HttpCmdDataPtr->DeviceType = SDT_MOBILE;
    HttpCmdDataPtr->BrowserType = UBT_GENERAL;
	//HttpCmdDataPtr->MobileType = NULL;
	HttpCmdDataPtr->MobileType = GetMobileDevice("HTC_Desire_X"); //320
	//HttpCmdDataPtr->MobileType = GetMobileDevice("HTC_One_X"); // 360
	//HttpCmdDataPtr->MobileType = GetMobileDevice("GT-N7000"); // 400
	//HttpCmdDataPtr->MobileType = GetMobileDevice("SM-T211"); // Tab
*/
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
char* EndHttpLineSet(char *HttpParserPtr)
{
	register unsigned char Value;

	for(;;)
	{
		Value = *HttpParserPtr;
		switch(Value)
		{
		    case 0x00:
			    return NULL;

			case 0x0A:
				return HttpParserPtr++;

			default:
				HttpParserPtr++;
				break;
		}
	}
}
//---------------------------------------------------------------------------
static char* LineParseSkip(void *DataPtr, char *HttpParserPtr)
{
	return EndHttpLineSet(HttpParserPtr);
}
//---------------------------------------------------------------------------
static char* HtmpFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_HTML_PAGE;
	return NULL;
}
//---------------------------------------------------------------------------
static char* GifFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_GIF_PIC;
	return NULL;
}
//---------------------------------------------------------------------------
static char* JpgFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_JPG_PIC;
	return NULL;
}
//---------------------------------------------------------------------------
static char* PngFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_PNG_PIC;
	return NULL;
}
//---------------------------------------------------------------------------
static char* IcoFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_ICO_PIC;
	return NULL;
}
//---------------------------------------------------------------------------
static char* CssFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_CSS_SCRIPT;
	return NULL;
}
//---------------------------------------------------------------------------
static char* JsFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_JS_SCRIPT;
	return NULL;
}
//---------------------------------------------------------------------------
static char* TxtFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_TXT_DATA;
	return NULL;
}
//---------------------------------------------------------------------------
static char* XmlFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_XML_DATA;
	return NULL;
}
//---------------------------------------------------------------------------
static char* CsvFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_CSV_DATA;
	return NULL;
}
//---------------------------------------------------------------------------
static char* HtrFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_HTR_PAGE;
	return NULL;
}
//---------------------------------------------------------------------------
static char* PdfFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_PDF_DOC;
	return NULL;
}
//---------------------------------------------------------------------------
static char* WavFileTypeSet(void *DataPtr, char *HttpParserPtr)
{
	((HTTP_MSG_EXTRACT_DATA*)DataPtr)->FileType = FRT_WAV_SOUND;
	return NULL;
}
//---------------------------------------------------------------------------
static char* IfModifSinceParse(void *DataPtr, char *HttpParserPtr)
{
	int           i;
	unsigned int  len, pars_read, Day = 0;
	unsigned int  Year, Hour, Minute, Second;
	unsigned char DayWeek = 0;
	unsigned char Month = 0;
	HTTP_MSG_EXTRACT_DATA *HttpCmdDataPtr;
	FIELD_EXTRACT_INFO *FldExtractPtr;
	struct tm     tm;
    time_t        epoch;

	HttpCmdDataPtr = (HTTP_MSG_EXTRACT_DATA*)DataPtr;
	HttpParserPtr = EndSpaseHttpLineSet(HttpParserPtr);
	if (!HttpParserPtr) return NULL;

    i = FindCmdRequestLine(HttpParserPtr, "GMT");
	if (i == -1) return EndHttpLineSet(HttpParserPtr);

	len = strlen(HttpParserPtr);
	for(i=0;i < sizeof(DayWeekInfo)/sizeof(FIELD_EXTRACT_INFO);i++)
	{
		FldExtractPtr = &DayWeekInfo[i];
		if ((len >= FldExtractPtr->FieldLen) && (memcmp(HttpParserPtr, FldExtractPtr->FiledNamePtr, FldExtractPtr->FieldLen) == 0))
		{
			DayWeek = FldExtractPtr->Index;
			HttpParserPtr += FldExtractPtr->FieldLen;
			break;
		}
	}
	if (!DayWeek) return NULL;
	HttpParserPtr = EndSpaseHttpLineSet(HttpParserPtr);
	if (!HttpParserPtr) return NULL;

	pars_read = sscanf(HttpParserPtr, "%u", &Day);
	if (!pars_read) return NULL;
	HttpParserPtr = EndDataBlockHttpLineSet(HttpParserPtr);
	if (!HttpParserPtr) return NULL;

	len = strlen(HttpParserPtr);
	for(i=0;i < sizeof(MonthNameInfo)/sizeof(FIELD_EXTRACT_INFO);i++)
	{
		FldExtractPtr = &MonthNameInfo[i];
		if ((len >= FldExtractPtr->FieldLen) && (memcmp(HttpParserPtr, FldExtractPtr->FiledNamePtr, FldExtractPtr->FieldLen) == 0))
		{
			Month = FldExtractPtr->Index - 1;
			HttpParserPtr += FldExtractPtr->FieldLen;
			break;
		}
	}

	if (!Month) return NULL;
	HttpParserPtr = EndSpaseHttpLineSet(HttpParserPtr);
	if (!HttpParserPtr) return NULL;

	pars_read = sscanf(HttpParserPtr, "%u %u:%u:%u", &Year, &Hour, &Minute, &Second);
	if ((pars_read < 4) || (Year < 1900)) return NULL;
	HttpParserPtr += 8;

	tm.tm_mon  = Month;
	tm.tm_mday = Day;
	tm.tm_year = Year - 1900;
	tm.tm_hour = Hour;
	tm.tm_min  = Minute;
	tm.tm_sec  = Second;

	HttpCmdDataPtr->IfModifyedSince = mktime(&tm);
	return EndHttpLineSet(HttpParserPtr);
}
//---------------------------------------------------------------------------
static char* EndSpaseHttpLineSet(char *HttpParserPtr)
{
	register unsigned char Value;

	for(;;)
	{
		Value = *HttpParserPtr;
		switch(Value)
		{
			case 0x00:
				return NULL;

		    case ' ':
			    HttpParserPtr++;
				break;

			default:
				return HttpParserPtr;
		}
	}
}
//---------------------------------------------------------------------------
static char* EndDataBlockHttpLineSet(char *HttpParserPtr)
{
	register unsigned char Value;

	for(;;)
	{
		Value = *HttpParserPtr;
		if ((Value == ' ') || (Value == ',') || (Value == 0x0a))
		{
			HttpParserPtr++;
			break;
		}
		else if (Value == 0x0)
		{
			HttpParserPtr = NULL;
			break;
		}
		HttpParserPtr++;
	}
	return HttpParserPtr;
}
//---------------------------------------------------------------------------
