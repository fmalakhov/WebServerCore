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

#include "BaseWebServer.h"
#include "ThrWebServer.h"
#include "HttpPageGen.h"
#include "HtmlTemplateParser.h"

extern char KeyNameId[];
extern char KeyUserNameId[];
extern char EmailId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char KeyFormSessionId[];
extern char *EndHtmlPageGenPtr;
extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK *ParReadHttpSocketPtr;
//---------------------------------------------------------------------------
void UserRegisterShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	char StrBuf[64];

	AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
		SessionPtr, "UserRegisterFieldsCheck.html");
    
    AddStrWebPage("<form action=\"UserRegRequest.html\" method=\"post\" name=\"UserRegisterForm\">\r\n");
	AddStrWebPage("<div class=\"componentheading\">");
    SetRusTextBuf(NULL, SITE_RUS_REGISTRATION_LINE_ID);
	AddStrWebPage("</div>\r\n");
    
	AddStrWebPage("<table cellpadding=\"1\" cellspacing=\"1\" border=\"0\" width=\"90%\" align=\"center\" class=\"contentpane\">\r\n");
	AddStrWebPage("<tr><td colspan=\"2\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_USER_REG_9_LINE_ID);
	AddStrWebPage("</td></tr>\r\n");
	AddStrWebPage("<tr><td width=\"30%\">\r\n");
    SetRusTextBuf(NULL, SITE_RUS_USER_REG_10_LINE_ID);
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)  AddStrWebPage("</td></tr><tr><td>\r\n");
    else                                                 AddStrWebPage("</td><td>\r\n");
	AddStrWebPage("<input type=\"text\" name=\"");
	AddStrWebPage(KeyNameId);
	AddStrWebPage("\" size=\"40\" value=\"\" class=\"inputbox\" maxlength=\"");
	sprintf(StrBuf, "%d\" >\r\n", MAX_LEN_USER_INFO_NAME);
	AddStrWebPage(StrBuf);
	AddStrWebPage("</td></tr>\r\n");
    
    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
    
	AddStrWebPage("<tr><td>\r\n");
    SetRusTextBuf(NULL, SITE_RUS_USER_REG_11_LINE_ID);
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)  AddStrWebPage("</td></tr><tr><td>\r\n");
    else                                                 AddStrWebPage("</td><td>\r\n");
	AddStrWebPage("<input type=\"text\" name=\"");
	AddStrWebPage(KeyUserNameId);
	AddStrWebPage("\" size=\"40\" value=\"\" class=\"inputbox\" maxlength=\"");
	sprintf(StrBuf, "%d\" >\r\n", MAX_LEN_USER_INFO_USER_NAME);
	AddStrWebPage(StrBuf);
	AddStrWebPage("</td></tr>\r\n");
    
    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
    
	AddStrWebPage("<tr><td>E-mail: *r\n");
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)  AddStrWebPage("</td></tr><tr><td>\r\n");
    else                                                 AddStrWebPage("</td><td>\r\n");
	AddStrWebPage("<input type=\"text\" name=\"");
	AddStrWebPage(EmailId);
	AddStrWebPage("\" size=\"40\" value=\"\" class=\"inputbox\" maxlength=\"");
	sprintf(StrBuf, "%d\" >\r\n", MAX_LEN_USER_INFO_EMAIL);
	AddStrWebPage(StrBuf);
	AddStrWebPage("</td></tr>\r\n");
    
    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
    
	AddStrWebPage("<tr><td>\r\n");
    SetRusTextBuf(NULL, SITE_RUS_USER_REG_12_LINE_ID);
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)  AddStrWebPage("</td></tr><tr><td>\r\n");
    else                                                 AddStrWebPage("</td><td>\r\n");
	AddStrWebPage("<input class=\"inputbox\" type=\"password\" name=\"");
	AddStrWebPage(PasswordId);
	AddStrWebPage("\" size=\"40\" value=\"\" maxlength=\"");
	sprintf(StrBuf, "%d\" >\r\n", MAX_LEN_USER_INFO_PASSWD);
	AddStrWebPage(StrBuf);
	AddStrWebPage("</td></tr>\r\n");
    
    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
    
	AddStrWebPage("<tr><td>\r\n");
    SetRusTextBuf(NULL, SITE_RUS_USER_REG_13_LINE_ID);
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)  AddStrWebPage("</td></tr><tr><td>\r\n");
    else                                                 AddStrWebPage("</td><td>\r\n");
	AddStrWebPage("<input class=\"inputbox\" type=\"password\" name=\"password2\" size=\"40\" value=\"\" maxlength=\"");
	sprintf(StrBuf, "%d\" >\r\n", MAX_LEN_USER_INFO_PASSWD);
	AddStrWebPage(StrBuf);
	AddStrWebPage("</td></tr>\r\n");

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
	AddStrWebPage("<input type=\"button\" value=\"");
	SetRusTextBuf(NULL, SITE_RUS_SENT_REQ_LINE_ID);
    AddStrWebPage("\" class=\"button\" onclick=\"UserRegisterSubmit()\">\r\n");
    AddStrWebPage("</td></tr>\r\n");    
	AddStrWebPage("</table>\r\n");

    // Add session id for HTML page handle.
	SetHiddenStrParForm(NULL, KeyFormSessionId, SessionPtr->SesionIdKey);
    SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	AddStrWebPage("</form>\r\n");

	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
