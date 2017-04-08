# if ! defined( HtmlTemplateParserH )
#	define HtmlTemplateParserH	/* only include me once */

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

#ifndef BaseWebServerH
#include "BaseWebServer.h"
#endif

#ifndef WebServInfoH
#include "WebServInfo.h"
#endif

typedef void (*THtmlFunc)(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);

typedef struct  {
    char* 	    HtmlPageName;
    THtmlFunc   FuncOnHandle;
} DEF_HTML_HANDLER;

typedef struct  {
    unsigned        AllPages;
    DEF_HTML_HANDLER  *ArrPageHandler;
} HTML_HANDLER_ARRAY;

typedef struct  {
    unsigned             TotalGropus;
    HTML_HANDLER_ARRAY   *HtmlHandlerArr;
} GROUP_HTML_HANDLERS;

typedef struct {
    PARAMWEBSERV  *ParWebServ;
    READWEBSOCK   *ParReadWeb;
	USER_SESSION  *SessionPtr;
	unsigned char *ParBufPtr;
	char          *HtmlGenPtr;
} HTML_CMD_PAR;

void HandleSetTitleInfo(void *ParPtr);
void HandleSetBeginMainPage(void *ParPtr);
void HandleSetEndMainPage(void *ParPtr);
void HandleSetServerName(void *ParPtr);
void HandleSetSessionId(void *ParPtr);
void HandleSetSessionKey(void *ParPtr);
void HandleSetMetaData(void *ParPtr);
void HandleSetSingleSessionId(void *ParPtr);
void HandleSetFirstPageDsButtton(void *ParPtr);
void HandleSetFirstPageEnButtton(void *ParPtr);
void HandleSetPrevPageDsButtton(void *ParPtr);
void HandleSetPrevPageEnButtton(void *ParPtr);
void HandleSetNextPageDsButtton(void *ParPtr);
void HandleSetNextPageEnButtton(void *ParPtr);
void HandleSetLastPageDsButtton(void *ParPtr);
void HandleSetLastPageEnButtton(void *ParPtr);
void HandleSetReturnDsButtton(void *ParPtr);
void HandleSetReturnEnButtton(void *ParPtr);
void HandleSetOriginalTextPage(void *ParPtr);
void HandleSetSysName(void *ParPtr);
void HandleSetServerVersion(void *ParPtr);
void HandleSetMenuGrpBgColor(void *ParPtr);
void HandleSetGrpTitleBgColor(void *ParPtr);
void HandleSetGrpContBgColor(void *ParPtr);
void HandleSetMenuZoneBgColor(void *ParPtr);
void HandleSetGrpBorderColor(void *ParPtr);
void HandleSetWorkZoneBgColor(void *ParPtr);
void HandleSetImgBgSideColumn(void *ParPtr);
void HandleSetImgNavBar(void *ParPtr);
void HandleSetImgBgWorkZone(void *ParPtr);
void HandleSetImgBgSiteGround(void *ParPtr);
void HandleSetUserSessionTimeout(void *ParPtr);
void HandleSetBaseTextById(void *ParPtr);
void HandleSetNavButtonZone(void *ParPtr);
void HandleSetImgProductLogo(void *ParPtr);
void HandleSetSecureKey(void *DataPtr);
bool HandleHtmlTemplate(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb,
	USER_SESSION *SessionPtr, FILE_HASH_RECORD *FileInfoPtr);
bool HandleHtmlLineTemplate(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, 
	USER_SESSION *SessionPtr, unsigned char *HtmlTemplPtr, unsigned char *ParamBuf);
void SetCssTemlateBody(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb,
	USER_SESSION *SessionPtr, FILE_HASH_RECORD *FileInfoPtr, char *CssTemplateFileName);
void SetHtmlTemlateBody(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, 
	USER_SESSION *SessionPtr, char *TemplateFileName);
void HtmlCmdHashInit(HTML_PAGE_HASH_CHAR_HOP *HtmlPageHashPtr, GROUP_HTML_HANDLERS* HtmlHandlePtr, char *Name);

//---------------------------------------------------------------------------
#endif  /* if ! defined( HtmlTemplateParserH ) */
