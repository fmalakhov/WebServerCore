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
#include "HtmlTemplateParser.h"
#include "BaseHtmlConstData.h"
#include "HtmlMacrosHash.h"
#include "TextListDataBase.h"

extern char KeyFormSessionId[];
extern char ServerVersion[];
extern char ThrWebServName[];
extern PARAMWEBSERV *ParWebServPtr;
extern char *MemWebPageGenPtr;
extern char *EndHtmlPageGenPtr;
extern FILE_HASH_CHAR_HOP FileHashHop;
extern char AnsiToHtmlRusConver[];
extern char SecKeyId[];
//---------------------------------------------------------------------------
void HandleSetUserSessionTimeout(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
	unsigned int Delay;
	char LineBuf[32];

    ParPtr = (HTML_CMD_PAR*)DataPtr;
	Delay = GetSessionTimeout(&ParPtr->ParWebServ->SessionManager, ParPtr->SessionPtr);
	sprintf(LineBuf, "%d", Delay*1000);
	AddStrWebPage(LineBuf);
}
//---------------------------------------------------------------------------
void HandleSetBeginMainPage(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;

    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddBeginPageShopWebPage(ParPtr->HtmlGenPtr, ParPtr->SessionPtr);
	EndHtmlPageGenPtr = &ParPtr->HtmlGenPtr[strlen(ParPtr->HtmlGenPtr)];
}
//---------------------------------------------------------------------------
void HandleSetEndMainPage(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
 
    ParPtr = (HTML_CMD_PAR*)DataPtr;
	AddEndPageShopWebPage(ParPtr->HtmlGenPtr, ParPtr->SessionPtr);
	EndHtmlPageGenPtr = &ParPtr->HtmlGenPtr[strlen(ParPtr->HtmlGenPtr)];
}
//---------------------------------------------------------------------------
void HandleSetServerName(void *DataPtr)
{
	SetServerHttpAddr(NULL);
}
//---------------------------------------------------------------------------
void HandleSetSessionId(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;

    ParPtr = (HTML_CMD_PAR*)DataPtr;    
	SetSessionIdCmdRef(ParPtr->SessionPtr);
}
//---------------------------------------------------------------------------
void HandleSetSingleSessionId(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    char CmdGenBuf[64];

    ParPtr = (HTML_CMD_PAR*)DataPtr;
    if ((!ParPtr->SessionPtr->UserPtr) && 
		(ParPtr->ParReadWeb->CookieSessionId > 0) && 
		(ParPtr->ParReadWeb->BotType == BOT_NONE))
	{
		/* Do nothing */
	}
	else
	{
	    sprintf(CmdGenBuf, "?%s=%s;", KeyFormSessionId, ParPtr->SessionPtr->SesionIdKey);
	    AddStrWebPage(CmdGenBuf);
	}
}
//---------------------------------------------------------------------------
void HandleSetSecureKey(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    char CmdGenBuf[64];

    ParPtr = (HTML_CMD_PAR*)DataPtr;
	sprintf(CmdGenBuf, "?%s=%u;", SecKeyId, ParPtr->SessionPtr->SecureKey);
	AddStrWebPage(CmdGenBuf);
}
//---------------------------------------------------------------------------
void HandleSetSessionKey(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;

    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParPtr->SessionPtr->SesionIdKey);
}
//---------------------------------------------------------------------------
void HandleSetTitleInfo(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
	if (strlen((const char*)ParPtr->ParBufPtr) > 0)
		strcat(ParPtr->ParWebServ->UserTitle, (const char*)ParPtr->ParBufPtr);
}
//---------------------------------------------------------------------------
void HandleSetMetaData(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    unsigned int  len;
	unsigned char ParamBuf[1201];

    ParPtr = (HTML_CMD_PAR*)DataPtr;
	len = (unsigned int)strlen((const char*)ParPtr->ParBufPtr);
	if ((len > 0) && (len < 1200))
	{
		strcpy((char*)ParamBuf, (const char*)ParPtr->ParBufPtr);
        if (HandleHtmlLineTemplate(ParPtr->ParWebServ, ParPtr->ParReadWeb, 
						ParPtr->SessionPtr, ParamBuf, ParPtr->ParBufPtr))
		{
            strcat(ParPtr->ParWebServ->UserMetaData, (const char*)ParPtr->ParBufPtr);
		}
	}
}
//---------------------------------------------------------------------------
void HandleSetFirstPageDsButtton(void *DataPtr)
{
	SetFirstPageNavDsButton();
}
//---------------------------------------------------------------------------
void HandleSetFirstPageEnButtton(void *DataPtr)
{
	SetFirstPageNavEnButton();
}
//---------------------------------------------------------------------------
void HandleSetPrevPageDsButtton(void *DataPtr)
{
	SetPrevPageNavDsButton();
}
//---------------------------------------------------------------------------
void HandleSetPrevPageEnButtton(void *DataPtr)
{
	SetPrevPageNavEnButton();
}
//---------------------------------------------------------------------------
void HandleSetNextPageDsButtton(void *DataPtr)
{
	SetNextPageNavDsButton();
}
//---------------------------------------------------------------------------
void HandleSetNextPageEnButtton(void *DataPtr)
{
	SetNextPageNavEnButton();
}
//---------------------------------------------------------------------------
void HandleSetLastPageDsButtton(void *DataPtr)
{
	SetLastPageNavDsButton();
}
//---------------------------------------------------------------------------
void HandleSetLastPageEnButtton(void *DataPtr)
{
	 SetLastPageNavEnButton();
}
//---------------------------------------------------------------------------
void HandleSetReturnDsButtton(void *DataPtr)
{
	SetReturnNavDsButton();
}
//---------------------------------------------------------------------------
void HandleSetReturnEnButtton(void *DataPtr)
{
	 SetReturnNavEnButton();
}
//---------------------------------------------------------------------------
void HandleSetOriginalTextPage(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
	if (strlen(ParPtr->ParBufPtr) > 0)
		AddStrWebPage(ParPtr->ParBufPtr);
}
//---------------------------------------------------------------------------
void HandleSetSysName(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
	AddStrWebPage(ParPtr->ParWebServ->ShopInfoCfg.Name);
}
//---------------------------------------------------------------------------
void HandleSetBaseTextById(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    int pars_read, TextID;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
	if (!strlen(ParPtr->ParBufPtr)) return;
    pars_read = sscanf(ParPtr->ParBufPtr, "%d", &TextID);
    if (!pars_read) return;
    SetOriginalRusTextBuf(NULL, TextID);
}
//---------------------------------------------------------------------------
void HandleSetServerVersion(void *DataPtr)
{
	AddStrWebPage(ServerVersion);
}
//---------------------------------------------------------------------------
void HandleSetMenuGrpBgColor(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.MenuGrpBgColor);
}
//---------------------------------------------------------------------------
void HandleSetGrpTitleBgColor(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.GrpTitleBgColor);
}
//---------------------------------------------------------------------------
void HandleSetGrpContBgColor(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.GrpContBgColor);
}
//---------------------------------------------------------------------------
void HandleSetMenuZoneBgColor(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.MenuZoneBgColor);
}
//---------------------------------------------------------------------------
void HandleSetGrpBorderColor(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.GrpBorderColor);
}
//---------------------------------------------------------------------------
void HandleSetWorkZoneBgColor(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.WorkZoneBgColor);
}
//---------------------------------------------------------------------------
void HandleSetImgBgSideColumn(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgSideColumn);
}
//---------------------------------------------------------------------------
void HandleSetImgNavBar(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgNavBar);
}
//---------------------------------------------------------------------------
void HandleSetImgBgWorkZone(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgWorkZone);
}
//---------------------------------------------------------------------------
void HandleSetImgBgSiteGround(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgSiteGround);
}
//---------------------------------------------------------------------------
void HandleSetNavButtonZone(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    if (ParPtr->ParReadWeb)
    {
        if (ParPtr->ParReadWeb->DeviceType == SDT_MOBILE)   AddStrWebPage("</td></tr><tr>");
        else                                                AddStrWebPage("</td>");
    }
}
//---------------------------------------------------------------------------
void HandleSetImgProductLogo(void *DataPtr)
{
    HTML_CMD_PAR *ParPtr;
    
    ParPtr = (HTML_CMD_PAR*)DataPtr;
    AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgProductLogo);
}
//---------------------------------------------------------------------------
bool HandleHtmlTemplate(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, 
						USER_SESSION *SessionPtr, FILE_HASH_RECORD *FileInfoPtr)
{
	bool          isCmdParseDone = false;
	bool          HtmlParseResult = true;
	bool          isHtmlParseDone = false;
	bool          isCommentLine = false;
	unsigned char ParseZone = 0;
    unsigned char *HtmlTemplPtr = NULL;
	unsigned char *CmdNamePtr = NULL;
	unsigned char *ParBufPtr = NULL;
	HTML_CMD_PAR  HtmlCmdParPtr;
	CMD_INFO      *CmdInfPtr = NULL;
	unsigned int  LineCnt = 1;
	unsigned int  CharsLineCnt = 0;
	unsigned char SrcSymb;
	unsigned int  CharId;
	unsigned char CmdNameBuf[64];
	unsigned char ParamBuf[1024];

    HtmlTemplPtr = FileInfoPtr->FileBodyBuf;
    HtmlCmdParPtr.ParWebServ = ParWebServ;
    HtmlCmdParPtr.ParReadWeb = ParReadWeb;
	HtmlCmdParPtr.SessionPtr = SessionPtr;
    for(;;)
	{
		switch(*HtmlTemplPtr)
		{
		    case '$':
				if (isCommentLine)
				{
					HtmlTemplPtr++;
					break;
				}

				if ((*(HtmlTemplPtr+1) == '(') || (*(HtmlTemplPtr+1) == '.'))
				{
                    /* Do not convert AJAX templates */                    
			        SrcSymb = (unsigned char)*HtmlTemplPtr;
				    CharsLineCnt++;
				    if (!isCommentLine && (CharsLineCnt > 0))
				    {
                        *EndHtmlPageGenPtr++ = *HtmlTemplPtr;
				    }
				    HtmlTemplPtr++;
                    break;
                }
                                
	            CmdNamePtr = &CmdNameBuf[0];
	            ParBufPtr = &ParamBuf[0];
                ParamBuf[0] = 0;
	            CmdNameBuf[0] = 0;
				HtmlTemplPtr++;
				isCmdParseDone = false;
				for(;;)
				{
					switch (*HtmlTemplPtr)
					{
					    case '(':
							if (ParseZone == 0)
							{
                                ParseZone = 1; /* Begin input parameter zone is detected */
                                HtmlTemplPtr++;
							}
							else
							{
							    isCmdParseDone = true;
                                HtmlParseResult = false;
	                            DebugLogPrint(NULL, "%s: %s:%d Error 1 HTML template line processing.\r\n", 
			                        ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
							}
							break;

						case ')':
                            if (ParseZone == 1)
							{
                                ParseZone = 2; /* End input parameter zone is detected */
							    HtmlTemplPtr++;
							}
                            else
							{
							    isCmdParseDone = true;
                                HtmlParseResult = false;
	                            DebugLogPrint(NULL,  "%s: %s:%d Error 2 HTML template line processing.\r\n", 
			                        ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
							}
							break;

						case ';':
							if (ParseZone == 1)
							{
								if ((unsigned long)(ParBufPtr - &ParamBuf[0]) < 1023)
                                       *ParBufPtr++ = *HtmlTemplPtr++;
								else
								{
							        isCmdParseDone = true;
									isHtmlParseDone = true;
                                    HtmlParseResult = false;
	                                DebugLogPrint(NULL, "%s: %s:%d Error 6 HTML template line processing.\r\n", 
			                            ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
								}
								break;
							}
                            *ParBufPtr = 0;
							*CmdNamePtr = 0;
							*EndHtmlPageGenPtr = 0;
                            HtmlTemplPtr++;
							HtmlCmdParPtr.ParBufPtr  = &ParamBuf[0];
							HtmlCmdParPtr.HtmlGenPtr = MemWebPageGenPtr;
							CmdInfPtr = FindHtmlBodyCmdHash(CmdNameBuf);
							if (CmdInfPtr)
							{
                                (CmdInfPtr->HtmlFunction)(&HtmlCmdParPtr);
							}
							else
							{
	                            DebugLogPrint(NULL, "%s: %s:%d Error 3 - unknown command name (%s) was detected.\r\n", 
			                        ThrWebServName, ParReadWeb->LocalFileName, LineCnt, CmdNameBuf);
								HtmlParseResult = false;
							}
							isCmdParseDone = true;
							ParseZone = 0;
							break;

						case 0:
							isCmdParseDone = true;
							isHtmlParseDone = true;
                            HtmlParseResult = false;
	                        DebugLogPrint(NULL, "%s: %s:%d Error 4 HTML template line processing.\r\n", 
			                    ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
							break;

						case '\r':
						case '\n':
							isCmdParseDone = true;
                            HtmlParseResult = false;
							break;

						default:
							switch(ParseZone)
							{
							    case 0:
									if ((unsigned long)(CmdNamePtr - &CmdNameBuf[0]) < 63)
							            *CmdNamePtr++ = *HtmlTemplPtr++;
								    else
									{
							            isCmdParseDone = true;
										isHtmlParseDone = true;
                                        HtmlParseResult = false;
	                                    DebugLogPrint(NULL, "%s: %s:%d Error 5 HTML template line processing.\r\n", 
			                                ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
									}
								    break;

								case 1:
									if ((unsigned long)(ParBufPtr - &ParamBuf[0]) < 1023)
                                        *ParBufPtr++ = *HtmlTemplPtr++;
									else
									{
							            isCmdParseDone = true;
										isHtmlParseDone = true;
                                        HtmlParseResult = false;
	                                    DebugLogPrint(NULL, "%s: %s:%d Error 6 HTML template line processing.\r\n", 
			                                ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
									}
									break;

								default:
									if (*HtmlTemplPtr != ' ')
									{
							            isCmdParseDone = true;
										isHtmlParseDone = true;
                                        HtmlParseResult = false;
	                                    DebugLogPrint(NULL, "%s: %s:%d Error 7 HTML template line processing.\r\n", 
			                                ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
									}
									break;
							}
							break;
					}
					if (isCmdParseDone) break;
				}
                break;

			case 0:
				isHtmlParseDone = true;
				break;

			default:
			    SrcSymb = (unsigned char)*HtmlTemplPtr;
				/* Check for first symbol in string - if # than this line is comment */
				if ((SrcSymb == '#') && (!CharsLineCnt)) isCommentLine = true;
				if ((SrcSymb != '\r') && (SrcSymb !='\n')) CharsLineCnt++;
				if (!isCommentLine)
				{
		            if (SrcSymb >= (unsigned char)UPPER_RUS_A_CHAR)
					{
				        CharId = ((unsigned int)(SrcSymb) - UPPER_RUS_A_CHAR) << 3;
				        memcpy(EndHtmlPageGenPtr, &AnsiToHtmlRusConver[CharId], 7);
			            EndHtmlPageGenPtr += 7;
					}
		            else
					{
					    if (CharsLineCnt > 0)
						{
			                *EndHtmlPageGenPtr++ = *HtmlTemplPtr;
						}
					}
				}
				if (SrcSymb == '\n')
				{
					LineCnt++;
					CharsLineCnt = 0;
					isCommentLine = false;
				}
				HtmlTemplPtr++;
				break;
		}
		if (isHtmlParseDone) break;
	}
	*EndHtmlPageGenPtr = 0;
	return HtmlParseResult;
}
//---------------------------------------------------------------------------
bool HandleHtmlLineTemplate(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, 
						USER_SESSION *SessionPtr, unsigned char *HtmlTemplPtr,
						unsigned char *ParamBuf)
{
	bool          isCmdParseDone = false;
	bool          HtmlParseResult = true;
	bool          isHtmlParseDone = false;
	bool          isCommentLine = false;
	unsigned char ParseZone = 0;
	unsigned char *CmdNamePtr = NULL;
	unsigned char *ParBufPtr = NULL;
	CMD_INFO      *CmdInfPtr = NULL;
	HTML_CMD_PAR  HtmlCmdParPtr;
	unsigned int  LineCnt = 1;
	unsigned int  CharsLineCnt = 0;
	char          *HtmlBufPtr;
	unsigned char SrcSymb;
	unsigned char CmdNameBuf[64];

	ParBufPtr = ParamBuf;
    *ParamBuf = 0;
    HtmlCmdParPtr.ParWebServ = ParWebServ;
    HtmlCmdParPtr.ParReadWeb = ParReadWeb;
	HtmlCmdParPtr.SessionPtr = SessionPtr;
    for(;;)
	{
		switch(*HtmlTemplPtr)
		{
		    case '$':
				if (isCommentLine)
				{
					HtmlTemplPtr++;
					break;
				}
                
	            CmdNamePtr = &CmdNameBuf[0];
	            CmdNameBuf[0] = 0;
				HtmlTemplPtr++;
				isCmdParseDone = false;
				for(;;)
				{
					switch (*HtmlTemplPtr)
					{
						case ';':
							*CmdNamePtr = 0;
                            HtmlTemplPtr++;
							HtmlCmdParPtr.ParBufPtr  = ParBufPtr;
							HtmlCmdParPtr.HtmlGenPtr = MemWebPageGenPtr;
							CmdInfPtr = FindHtmlLineCmdHash(CmdNameBuf);
							if (CmdInfPtr)
							{
	                            HtmlBufPtr = EndHtmlPageGenPtr;
                                EndHtmlPageGenPtr = ParBufPtr;
                                (CmdInfPtr->HtmlFunction)(&HtmlCmdParPtr);
                                EndHtmlPageGenPtr = HtmlBufPtr;
								ParBufPtr = &ParamBuf[strlen(ParamBuf)];
							}
							else
							{
	                            DebugLogPrint(NULL, "%s: %s:%d Error 3 - unknown command name (%s) was detected.\r\n", 
			                        ThrWebServName, ParReadWeb->LocalFileName, LineCnt, CmdNameBuf);
								HtmlParseResult = false;
							}
							isCmdParseDone = true;
							ParseZone = 0;
							break;

						case 0:
							isCmdParseDone = true;
							isHtmlParseDone = true;
                            HtmlParseResult = false;
	                        DebugLogPrint(NULL, "%s: %s:%d Error 4 HTML template line processing.\r\n", 
			                    ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
							break;

						case '\r':
						case '\n':
							isCmdParseDone = true;
                            HtmlParseResult = false;
							break;

						default:
							switch(ParseZone)
							{
							    case 0:
									if ((unsigned long)(CmdNamePtr - &CmdNameBuf[0]) < 63)
							            *CmdNamePtr++ = *HtmlTemplPtr++;
								    else
									{
							            isCmdParseDone = true;
										isHtmlParseDone = true;
                                        HtmlParseResult = false;
	                                    DebugLogPrint(NULL, "%s: %s:%d Error 5 HTML template line processing.\r\n", 
			                                ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
									}
								    break;

								default:
									if (*HtmlTemplPtr != ' ')
									{
							            isCmdParseDone = true;
										isHtmlParseDone = true;
                                        HtmlParseResult = false;
	                                    DebugLogPrint(NULL, "%s: %s:%d Error 7 HTML template line processing.\r\n", 
			                                ThrWebServName, ParReadWeb->LocalFileName, LineCnt);
									}
									break;
							}
							break;
					}
					if (isCmdParseDone) break;
				}
                break;

			case 0:
				isHtmlParseDone = true;
				break;

			default:
			    SrcSymb = (unsigned char)*HtmlTemplPtr;
				/* Check for first symbol in string - if # than this line is comment */
				if ((SrcSymb == '#') && (!CharsLineCnt)) isCommentLine = true;
				if ((SrcSymb != '\r') && (SrcSymb !='\n')) CharsLineCnt++;
				if ((!isCommentLine) && (CharsLineCnt > 0))
				{
			        *ParBufPtr++ = *HtmlTemplPtr;
				}
				if (SrcSymb == '\n')
				{
					LineCnt++;
					CharsLineCnt = 0;
					isCommentLine = false;
				}
				HtmlTemplPtr++;
				break;
		}
		if (isHtmlParseDone) break;
	}
    *ParBufPtr++ = '\r';
	*ParBufPtr++ = '\n';
	*ParBufPtr = 0;
	return HtmlParseResult;
}
//---------------------------------------------------------------------------
void SetHtmlTemlateBody(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, 
						USER_SESSION *SessionPtr, char *TemplateFileName)
{
	FILE_HASH_RECORD *FileInfoPtr = NULL;
	char             *CwdPtr = NULL;
	char             HtmlFileName[1024];

    CwdPtr = getcwd((char*)(&HtmlFileName[0]),512);
	strcat(HtmlFileName, HtmlDataPath);
	strcat(HtmlFileName, HtmlTemplateDir);
    strcat(HtmlFileName, TemplateFileName);
    FileInfoPtr = FindFileHash(&FileHashHop, BASE_HASH_LIST, HtmlFileName);
    if (FileInfoPtr)
	{
		if (!HandleHtmlTemplate(ParWebServ, ParReadWeb, SessionPtr, FileInfoPtr))
		{
	        DebugLogPrint(NULL, "%s: Faild parse of HTML temlate %s file.\r\n", 
			    ThrWebServName, TemplateFileName);
		}
	}
	else
	{
	    DebugLogPrint(NULL, "%s: The HTML temlate %s file does not present in hash.\r\n", 
		    ThrWebServName, TemplateFileName);
	}
}
//---------------------------------------------------------------------------
void SetCssTemlateBody(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, 
						USER_SESSION *SessionPtr, FILE_HASH_RECORD *FileInfoPtr,
						char *CssTemplateFileName)
{
    if (FileInfoPtr)
	{
		if (!HandleHtmlTemplate(ParWebServ, ParReadWeb, SessionPtr, FileInfoPtr))
		{
	        DebugLogPrint(NULL, "%s: Faild parse of CSS temlate %s file.\r\n", 
			    ThrWebServName, CssTemplateFileName);
		}
	}
	else
	{
	    DebugLogPrint(NULL, "%s: The CSS temlate %s file does not present in hash.\r\n", 
		    ThrWebServName, CssTemplateFileName);
	}
}
//---------------------------------------------------------------------------
