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

#include "CommonPlatform.h"
#include "ThrWebServer.h"
#include "ThrCernel.h"
#include "SysWebFunction.h"
#include "ThrConnWeb.h"
#include "SysMessages.h"
#include "TrTimeOutThread.h"
#include "HttpPageGen.h"
#include "ServerPorts.h"

extern char KeyFormSessionId[];
extern char SecKeyId[];
extern char ThrWebServName[];
extern PARAMWEBSERV *ParWebServPtr;
//---------------------------------------------------------------------------
void SetCapchaCodeRequestIntro()
{
	AddStrWebPage("<span style=\"color:black;font-weight:bold\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_CAPCHA_INTRO_LINE_ID);
	AddStrWebPage("&nbsp</span><span style=\"color:red;font-weight:bold\">*</span>\r\n");
}
//---------------------------------------------------------------------------
void SetCapchaCodeRequestBody(PARAMWEBSERV *ParWebServPtr, USER_SESSION *SessionPtr)
{
	char StrBuf[64];
    
    SessionPtr->CapchaCode = SetExpectSessionCapchaCode(ParWebServPtr);
    
    AddStrWebPage("<style type=\"text/css\">\r\n");
    AddStrWebPage("<!--\r\n");
    AddStrWebPage(".register_capcha_image_block {\r\n");
    AddStrWebPage("background-image: url(../images/user_check_capcha.png");

	sprintf(StrBuf,"?%s=%s", KeyFormSessionId, SessionPtr->SesionIdKey);
	AddStrWebPage(StrBuf);

    AddStrWebPage(");\r\nwidth: 106px;\r\n");
    AddStrWebPage("height: 45px;\r\n");
    AddStrWebPage("background-size: contain;\r\n");
    AddStrWebPage("background-repeat: no-repeat;\r\n");
    AddStrWebPage("position: relative;\r\n");
    AddStrWebPage("left: 0px;\r\n");
    AddStrWebPage("top:  0px;\r\n");
    AddStrWebPage("border-style: solid;\r\n");
    AddStrWebPage("border-color: #000000;\r\n");
    AddStrWebPage("border-width: 1px;\r\n");
    AddStrWebPage("border-radius: 4px;\r\n");
    AddStrWebPage("-moz-border-radius: 4px;\r\n");
    AddStrWebPage("-webkit-border-radius: 4px;\r\n");
    AddStrWebPage("-khtml-border-radius: 4px;\r\n}\r\n");
	AddStrWebPage("-->\r\n");
    AddStrWebPage("</style>\r\n");
   
	AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr align=\"left\">\r\n");
    AddStrWebPage("<td width=\"100\"><input type=\"text\" id=\"message_capcha_id\" autocomplete=\"off\" name=\"");
	AddStrWebPage(KeyFormCapchaCode);	
	AddStrWebPage("\" size=\"7\" value=\"\" maxlength=\"4\"></td>\r\n");
	AddStrWebPage("<td width=\"110\"><div class=\"register_capcha_image_block\" id=\"capcha_image_id\"></div>\r\n");
    AddStrWebPage("</td><td></td></tr></table>\r\n");    
}
//---------------------------------------------------------------------------
