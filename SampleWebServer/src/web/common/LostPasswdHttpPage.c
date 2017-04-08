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

extern char KeyNameId[];
extern char EmailId[];
extern char KeyFormSessionId[];
extern char *EndHtmlPageGenPtr;
extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK *ParReadHttpSocketPtr;
//---------------------------------------------------------------------------
void LostPasswdShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	char BufLine[64];

    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

    AddStrWebPage("<form action=\"");
	AddStrWebPage(GenPagePasswdSentMail);
	AddStrWebPage("\" method=\"post\">\r\n");

    SetHiddenStrParForm(NULL, KeyFormSessionId, SessionPtr->SesionIdKey);

    AddStrWebPage("<div class=\"componentheading\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_FORGOT_PASS_LINE_ID);
    AddStrWebPage("</div>\r\n");
    AddStrWebPage("<table cellpadding=\"1\" cellspacing=\"1\" border=\"0\" width=\"90%\" class=\"contentpane\">\r\n");
    AddStrWebPage("<tr><td colspan=\"2\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_PASSWD_REST_INSTR_1_LINE_ID);
	AddStrWebPage("<br>");
	SetRusTextBuf(NULL, SITE_RUS_PASSWD_REST_INSTR_2_LINE_ID);
    AddStrWebPage("</td></tr>\r\n");
    
    AddStrWebPage("<tr><td>");
	SetRusTextBuf(NULL, SITE_RUS_ORDER_USER_NAME_LINE_ID);
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)  AddStrWebPage("</td></tr><tr><td>\r\n");
    else                                                 AddStrWebPage("</td><td>\r\n");
    AddStrWebPage("<input type=\"text\" name=\"");
	AddStrWebPage( KeyNameId);
	AddStrWebPage("\" class=\"inputbox\" size=\"40\"");
	sprintf(BufLine, " maxlength=\"%d\" ></td>\r\n", MAX_LEN_USER_INFO_NAME);
	AddStrWebPage( BufLine);
	AddStrWebPage("</tr>\r\n");
    
    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
    
    AddStrWebPage("<tr><td>");
	SetRusTextBuf(NULL, SITE_RUS_PASSWD_REST_EMAIL_LINE_ID);		
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)  AddStrWebPage("</td></tr><tr><td>\r\n");
    else                                                 AddStrWebPage("</td><td>\r\n");
    AddStrWebPage("<input type=\"text\" name=\"");
	AddStrWebPage( EmailId);
	AddStrWebPage("\" class=\"inputbox\" size=\"40\"");
	sprintf(BufLine, " maxlength=\"%d\" ></td>\r\n", MAX_LEN_USER_INFO_EMAIL);
	AddStrWebPage( BufLine);
    AddStrWebPage("</tr>\r\n");
    
    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");

    /* Capcha request */
    AddStrWebPage("<tr align=\"left\"><td>\r\n");
    SetCapchaCodeRequestIntro();
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)  AddStrWebPage("</td></tr><tr><td>\r\n");
    else                                                 AddStrWebPage("</td><td>\r\n");
    SetCapchaCodeRequestBody(ParWebServPtr, SessionPtr);
    AddStrWebPage("</td></tr>\r\n");
    
	AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
	AddStrWebPage("<tr><td colspan=\"2\"><hr></td></tr>\r\n");
    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
    
	AddStrWebPage("<tr><td colspan=\"2\">\r\n");
    AddStrWebPage("<input type=\"submit\" class=\"button\" value=\"");
	SetRusTextBuf(NULL, SITE_RUS_PASSWD_REST_SENT_PASS_LINE_ID);
    AddStrWebPage("\" >\r\n");
    AddStrWebPage("</td></tr></table>\r\n");
    AddStrWebPage("</form>\r\n");
    
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
