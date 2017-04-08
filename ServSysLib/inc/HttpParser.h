# if ! defined( HttpParserH )
#	define HttpParserH	/* only include me once */

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

#ifndef MpttParserHashH
#include "MpttParserHash.h"
#endif

#ifndef WebServInfoH
#include "WebServInfo.h"
#endif

#ifndef MobileDeviceDataBaseH
#include "MobileDeviceDataBase.h"
#endif

typedef struct {
	unsigned char  Index;
	char		   *FiledNamePtr;
	unsigned int   FieldLen;
} FIELD_EXTRACT_INFO;

typedef struct {
	bool           isParseDone;
	bool           isEncodingAccept;
	bool           isKeepAlive;
	unsigned char  ReqestType;
	unsigned char  FileType;
	unsigned char  MozilaMainVer;
    unsigned char  MozilaSubVer;
	unsigned char  DeviceType;
    unsigned char  BrowserType;
	unsigned char  BotType;
	unsigned char  Status;
	unsigned int   CookieSessionId;
	unsigned int   NoPwdLocalNameShift;
	time_t         IfModifyedSince;
	char           *CookieSesKeyPtr;
	char           *StrCmdHTTP;
	char           *CmdSesKeyPtr;
	char           *LocalFileName;
	MOBILE_DEV_TYPE *MobileType;
	void           *ReaderInfoPtr;
	void           *ParReadWebPtr;
} HTTP_MSG_EXTRACT_DATA;

void HttpCmdMsgParserInit(MPTT_PARSER_MESSAGE *HttpCmdParserMessage);
void HttpCmdMsgParserDestroy(MPTT_PARSER_MESSAGE *HttpCmdParserMessage);
void UserAgentMsgParserInit(MPTT_PARSER_MESSAGE *UserAgentParserMessage);
void UserAgentMsgParserDestroy(MPTT_PARSER_MESSAGE *UserAgentParserMessage);
void HttpCmdContentMsgParserInit(MPTT_PARSER_MESSAGE *HttpCmdParserMessage);
void HttpCmdContentMsgParserDestroy(MPTT_PARSER_MESSAGE *HttpCmdParserMessage);
void HttpFileLineMsgParserInit(MPTT_PARSER_MESSAGE *HttpCmdParserMessage);
void HttpFileLineMsgParserDestroy(MPTT_PARSER_MESSAGE *HttpCmdParserMessage);
//---------------------------------------------------------------------------
#endif  /* if ! defined( HttpParserH ) */
