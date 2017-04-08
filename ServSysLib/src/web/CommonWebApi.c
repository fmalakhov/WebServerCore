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
#include "HtmlTemplateParser.h"
#include "FileNameMapBase.h"
#include "FileDataHash.h"
#include "ImageNameHash.h"
#include "TextListDataBase.h"
#include "BaseTextLineCode.h"
#include "BaseHtmlConstData.h"

extern READWEBSOCK *ParReadHttpSocketPtr;
extern PARAMWEBSERV *ParWebServPtr;
extern char ThrWebServName[];
extern unsigned char gLanguageType;
extern char KeyUserNameId[];
extern char KeyFormConfirmKey[];
extern char PasswordId[];
extern char SecKeyId[];
extern ListItsTask UserSessionList;
extern char ServerVersion[];

	char    KeySectionId[]   = "section_id=";
	char    KeyFormSectionId[] = "section_id";
	char    KeyItemId[]      = "item_id=";
	char    KeyFormItemId[]  = "item_id";
	char    KeyListSortId[]  = "list_sort=";
	char    KeyReversModeId[] = "reverse_mode=";
	char    KeyItemsPageId[]  = "items_page=";
	char    KeyStartItemId[]  = "start_item=";
	char    KeySessionId[]    = "session_id=";
	char    KeyUserId[]       = "user_id=";
	char    KeyOrderId[]      = "order_id=";
	char    KeyStatusId[]     = "status_id=";

	char    KeyFormUserId[]    = "user_id";
	char    KeyFormSessionId[] = "session_id";
	char    KeyFormOrderId[]   = "order_id";
	char    KeyFormStatusId[]  = "status_id";
	char    KeyFormMenuRefId[] = "menu_ref";

	char    NoPhotoFileName[]  = "no_photo.jpg";
    char    FormKeyLangId[]    = "set_lang_id";

static char DemoLogin[MAX_LEN_USER_INFO_USER_NAME+1];
static char DemoPasswd[MAX_LEN_STATS_NAME+1];
//---------------------------------------------------------------------------
void MainPageSetShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	unsigned int TextLen;
    char*        FText = NULL;
	char*        FStrt = NULL;
	USER_INFO*   NewUserPtr = NULL;
	unsigned int i, pars_read;

    SessionPtr->isMainPageReq = true;
    if ((SessionPtr->UserPtr) && 
		(ParWebServPtr->SessionManager.SysShowUserType > 0) &&
		(SessionPtr->UserPtr->UserType == ParWebServPtr->SessionManager.SysShowUserType))
    {
        MainSystemShowWebPage(BufAnsw, SessionPtr, HttpCmd);
    }
    else
    {
        if (ParWebServPtr->ServCustomCfg.DemoMode)
        {
            /* Check for demo login and password available */
	        for(;;)
	        {
                *DemoLogin = 0;
                *DemoPasswd = 0;
                FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
                if (!FText) break;
	            FStrt = FText;
                strcpy(FText, HttpCmd);
                
				if (!StringParParse(FText, (char*)&DemoLogin[0],
		            "demo_login", MAX_LEN_USER_INFO_USER_NAME)) break;
		        FText = FStrt;
		        strcpy(FText,HttpCmd);
				if (!StringParParse(FText, (char*)&DemoPasswd[0],
		            "demo_passwd", MAX_LEN_USER_INFO_PASSWD)) break;
		        FText = FStrt;
		        strcpy(FText,HttpCmd);
                break;                
            }
            if (FStrt) FreeMemory(FStrt);            
        }    
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
	    AddGeneralInfoMainPageShopWebPage(BufAnsw, SessionPtr);
	    AddEndPageShopWebPage(BufAnsw, SessionPtr);
    }
}
//---------------------------------------------------------------------------
void SetSessionIdCmdRef(USER_SESSION *SessionPtr)
{
    char CmdGenBuf[64];

	if ((SessionPtr->UserPtr) || 
	    ((!ParReadHttpSocketPtr->CookieSessionId) && 
		(ParReadHttpSocketPtr->BotType == BOT_NONE)))
	{
	    sprintf(CmdGenBuf, "?%s=%s;", KeyFormSessionId, SessionPtr->SesionIdKey);
	    AddStrWebPage(CmdGenBuf);
	}
	else
	{
		AddStrWebPage( "?");
	}
}
//---------------------------------------------------------------------------
void ShowServerInfoShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];
    if (gLanguageType == LGT_ENGLISH)
    {
	    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
            SessionPtr, "ServerInfoEng.html");
    }
    else
    {
	    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
            SessionPtr, "ServerInfoRus.html");    
    }
}
//---------------------------------------------------------------------------
void AddComMainMenuItem(char *HtmlPageName, unsigned int NameId,
	USER_SESSION *SessionPtr, unsigned int tag)
{
	char StrBuf[128];

	AddStrWebPage("<a class=\"main_menu\" ");
	sprintf(StrBuf, " id=\"menu_id_%d\" ", tag);
    AddStrWebPage(StrBuf);
	sprintf(StrBuf, "('menu_id_%d', 'menu_text_%d');\" ", tag, tag);
	AddStrWebPage("onMouseOver=\"on_sel_menu_mouseover");
	AddStrWebPage(StrBuf);
	AddStrWebPage("onMouseOut=\"on_sel_menu_mouseout");
	AddStrWebPage(StrBuf);
	AddStrWebPage("onClick=\"on_sel_menu_click");
	AddStrWebPage(StrBuf);
	AddStrWebPage("href=\"");
	SetServerHttpAddr(NULL);
	AddStrWebPage(HtmlPageName);
	if ((SessionPtr->UserPtr) || 
	    ((!ParReadHttpSocketPtr->CookieSessionId) && 
		(ParReadHttpSocketPtr->BotType == BOT_NONE)))
	{
	    sprintf(StrBuf, "?%s=%s", KeyFormSessionId, SessionPtr->SesionIdKey);
	    AddStrWebPage( StrBuf);
	}
	AddStrWebPage("\"><div class=\"section_name\" ");
	sprintf(StrBuf, " id=\"menu_text_%d\">", tag);
    AddStrWebPage(StrBuf);
    SetRusTextBuf(NULL, NameId);
	AddStrWebPage("</div></a>\r\n");
}
//---------------------------------------------------------------------------
void AddSecMainMenuItem(char *HtmlPageName, unsigned int NameId, USER_SESSION *SessionPtr, unsigned int tag)
{
	char StrBuf[256];

	AddStrWebPage("<a class=\"main_menu\" ");
	sprintf(StrBuf, " id=\"menu_id_%d\" ", tag);
    AddStrWebPage(StrBuf);
	sprintf(StrBuf, "('menu_id_%d', 'menu_text_%d');\" ", tag, tag);
	AddStrWebPage("onMouseOver=\"on_sel_menu_mouseover");
	AddStrWebPage(StrBuf);
	AddStrWebPage("onMouseOut=\"on_sel_menu_mouseout");
	AddStrWebPage(StrBuf);
	AddStrWebPage("onClick=\"on_sel_menu_click");
	AddStrWebPage(StrBuf);
	AddStrWebPage("href=\"");
	SetServerHttpAddr(NULL);
    AddStrWebPage(HtmlPageName);
	sprintf(StrBuf, "?%s%d?%s%s?%s=%d", KeyUserId, SessionPtr->UserPtr->UserId, 
			    KeySessionId, SessionPtr->SesionIdKey, SecKeyId, SessionPtr->SecureKey);
	AddStrWebPage(StrBuf);
	AddStrWebPage("\"><div class=\"section_name\" ");
	sprintf(StrBuf, " id=\"menu_text_%d\">", tag);
    AddStrWebPage(StrBuf);
    SetRusTextBuf(NULL, NameId);
	AddStrWebPage("</div></a>\r\n");
}
//---------------------------------------------------------------------------
void AddSecExtMainMenuItem(char *HtmlPageName, unsigned int NameId, unsigned int tag)
{
	char StrBuf[256];

	AddStrWebPage("<a class=\"main_menu\" ");
	sprintf(StrBuf, " id=\"menu_id_%d\" ", tag);
    AddStrWebPage(StrBuf);
	sprintf(StrBuf, "('menu_id_%d', 'menu_text_%d');\" ", tag, tag);
	AddStrWebPage("onMouseOver=\"on_sel_menu_mouseover");
	AddStrWebPage(StrBuf);
	AddStrWebPage("onMouseOut=\"on_sel_menu_mouseout");
	AddStrWebPage(StrBuf);
	AddStrWebPage("onClick=\"on_sel_menu_click");
	AddStrWebPage(StrBuf);
	AddStrWebPage("href=\"");    
    AddStrWebPage(HtmlPageName);
	AddStrWebPage("\"  target=\"_blank\"><div class=\"section_name\" ");
	sprintf(StrBuf, " id=\"menu_text_%d\">", tag);
    AddStrWebPage(StrBuf);
    SetRusTextBuf(NULL, NameId);
	AddStrWebPage("</div></a>\r\n");
}
//---------------------------------------------------------------------------
void AddRegUserShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	char StrBuf[64];
	
	if (SessionPtr->UserPtr || 
	   (ParReadHttpSocketPtr->BotType != BOT_NONE)) return; /* Do not register user if user already registered or BOT */
	EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

    AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">\r\n");        
    AddStrWebPage("var AuthSessionId=\"");
    AddStrWebPage(SessionPtr->SesionIdKey);
    AddStrWebPage("\";\r\n");
    sprintf(StrBuf, "var AuthSecKey=%d;\r\n", SessionPtr->SecureKey);
    AddStrWebPage(StrBuf);
    AddStrWebPage("var PublicEncKey=[");
    AddSessionKeyWebPage(SessionPtr->SessionEncodeKeyList);
    AddStrWebPage("];\r\n</script>\r\n");

	AddStrWebPage("<div class=\"user_auth_group\">\r\n");
	AddStrWebPage("<div class=\"user_auth_title\" id=\"section_5\" >");
	SetRusTextBuf(NULL, SITE_RUS_AUTH_LINE_ID);
	AddStrWebPage("</div>\r\n");
	AddStrWebPage("<div class=\"user_auth_container\" id=\"container_5\">\r\n");

	AddStrWebPage("<form action=\"");
	AddStrWebPage(GenPageUserAuthReq);
	AddStrWebPage("\" method=\"post\" name=\"UserAuth\" >\r\n");
    
	AddStrWebPage("<label class=\"user_auth_text\" for=\"mod_login_username\">");
	SetRusTextBuf(NULL, SITE_RUS_AUTH_USER_LINE_ID);
	AddStrWebPage("</label>\r\n");
	AddStrWebPage("<br>\r\n");
	AddStrWebPage("<input name=\"");
	AddStrWebPage(KeyUserNameId);
	AddStrWebPage("\" id=\"mod_login_username\" type=\"text\" ");
    if (ParWebServPtr->ServCustomCfg.DemoMode && (strlen(DemoLogin) > 0))
    {
        AddStrWebPage("value=\"");
        AddStrWebPage(DemoLogin);
        AddStrWebPage("\" ");
    }
    AddStrWebPage("class=\"user_auth_box\" size=\"10\" >\r\n");
	AddStrWebPage("<br>\r\n");
    if (ParReadHttpSocketPtr->WebChanId == PRIMARY_WEB_CHAN)
    {
        if (ParWebServPtr->ServCustomCfg.PrimPortKeyAccess) UserRegisterWithKey();
        else                                                UserRegisterNoKey();
    }
    else
    {
        if (ParWebServPtr->ServCustomCfg.SecondPortKeyAccess) UserRegisterWithKey();
        else                                                  UserRegisterNoKey();
    }
    AddStrWebPage("<div id=\"srp_data_id\"></div>");    
    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr, SessionPtr, "UserAuth.html");

    if (ParReadHttpSocketPtr->WebChanId == PRIMARY_WEB_CHAN)
    {
        if (ParWebServPtr->ServCustomCfg.PrimPortKeyAccess)
            AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">UserAuthKeyInit();</script>\r\n");
    }
    else
    {
        if (ParWebServPtr->ServCustomCfg.SecondPortKeyAccess)
            AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">UserAuthKeyInit();</script>\r\n"); 
    }    

    if (!ParWebServPtr->ServCustomCfg.DemoMode)
    {
	    AddStrWebPage("<br><div class=\"user_auth_util\"><a href=\"");
	    AddStrWebPage(GenPageLostPasswd);
        if ((SessionPtr->UserPtr) || 
	        ((!ParReadHttpSocketPtr->CookieSessionId) && 
		    (ParReadHttpSocketPtr->BotType == BOT_NONE)))
	    {
	        sprintf(StrBuf,"?%s=%s", 
		        KeyFormSessionId, SessionPtr->SesionIdKey);
	        AddStrWebPage(StrBuf);
	    }
	    AddStrWebPage("\" class=\"user_auth_link\" >");
	    SetRusTextBuf(NULL, SITE_RUS_FORGOT_PASS_LINE_ID);
	    AddStrWebPage("</a>\r\n");

        if ((ParReadHttpSocketPtr->WebChanId == PRIMARY_WEB_CHAN) || 
            ParWebServPtr->ServCustomCfg.SecondPortInfoEdit)
        {
	        SetRusTextBuf(NULL, SITE_RUS_NOT_REG_YET_LINE_ID);
	        AddStrWebPage("<a href=\"");
	        AddStrWebPage(GenPageUserRegister);
	        if ((SessionPtr->UserPtr) || 
	            ((!ParReadHttpSocketPtr->CookieSessionId) && 
		        (ParReadHttpSocketPtr->BotType == BOT_NONE)))
	        {
	            sprintf(StrBuf,"?%s=%s", 
		            KeyFormSessionId, SessionPtr->SesionIdKey);
	            AddStrWebPage(StrBuf);
	        }
	        AddStrWebPage("\" class=\"user_auth_link\" >");
	        SetRusTextBuf(NULL, SITE_RUS_REGISTRATION_LINE_ID);
	        AddStrWebPage("</a>");
        }
        AddStrWebPage("</div>\r\n");
    }

    if ((SessionPtr->UserPtr) || 
	    ((!ParReadHttpSocketPtr->CookieSessionId) && 
		(ParReadHttpSocketPtr->BotType == BOT_NONE)))
	{
		SetHiddenStrParForm(NULL, KeyFormSessionId, SessionPtr->SesionIdKey);
	}
	SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	AddStrWebPage("</form>\r\n");

	AddStrWebPage("<form action=\"");
	AddStrWebPage(GenPageUserAuthReq);
	AddStrWebPage("\" method=\"post\" name=\"UserRegister\" >\r\n");
    if ((SessionPtr->UserPtr) || 
	    ((!ParReadHttpSocketPtr->CookieSessionId) && 
		(ParReadHttpSocketPtr->BotType == BOT_NONE)))
	{
		SetHiddenStrParForm(NULL, KeyFormSessionId, SessionPtr->SesionIdKey);
	}
	SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
    SetHiddenStrParForm(NULL, "usr_auth", "");
    SetHiddenStrParForm(NULL, KeyFormConfirmKey, "");
    AddStrWebPage("</form>\r\n");
    
	AddStrWebPage("</div></div>\r\n");        
	AddStrWebPage("<div class=\"inter_block_spase\"></div>\r\n");
	return;
}
//---------------------------------------------------------------------------
void UserRegisterNoKey()
{
    AddStrWebPage("<div class=\"auth_passwd_zone_block\" id=\"auth_passwd_zone_id\">\r\n");        
    AddStrWebPage("<div class=\"auth_enter_passwd_data_block\">");
	AddStrWebPage("<label class=\"user_auth_text\" for=\"mod_login_password\">");
	SetRusTextBuf(NULL, SITE_RUS_AUTH_PASSWD_LINE_ID);
	AddStrWebPage("</label>\r\n");
	AddStrWebPage("<br>\r\n");        
	AddStrWebPage("<input type=\"password\" id=\"mod_login_password\" name=\"");
	AddStrWebPage(PasswordId);
	AddStrWebPage("\" class=\"user_auth_box\" size=\"10\" ");
    if (ParWebServPtr->ServCustomCfg.DemoMode && (strlen(DemoPasswd) > 0))
    {
        AddStrWebPage("value=\"");
        AddStrWebPage(DemoPasswd);
        AddStrWebPage("\" ");
    }
    AddStrWebPage("onKeyPress=\"OnSubmitEnter(event);\">\r\n");    
    AddStrWebPage("</div>\r\n");    
	AddStrWebPage("<div class=\"auth_start_bt_block\" style=\"background-image: url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtEnterEnKey));        
    AddStrWebPage("); width: 70px; height: 28px;\" onmouseover = \"this.style.backgroundImage = 'url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtEnterMoKey));        
    AddStrWebPage(")'\" onmouseout = \"this.style.backgroundImage = 'url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtEnterEnKey));        
    AddStrWebPage(")'\" onclick=\"AuthNoKeyStartSentReq();\"></div>\r\n");
    AddStrWebPage("</div>\r\n"); 
    AddStrWebPage("<div class=\"show_user_auth_progress_block\" id=\"user_auth_show_id\"></div>\r\n");
}
//---------------------------------------------------------------------------
void UserRegisterWithKey()
{
    /* Auth key request */        
    AddStrWebPage("<div class=\"auth_passwd_zone_block\" id=\"auth_passwd_zone_id\">\r\n");
    AddStrWebPage("<div class=\"auth_enter_passwd_data_block\">");
	AddStrWebPage("<label class=\"user_auth_text\" for=\"mod_login_password\">");
	SetRusTextBuf(NULL, SITE_RUS_AUTH_PASSWD_LINE_ID);
	AddStrWebPage("</label>\r\n");
	AddStrWebPage("<br>\r\n");        
	AddStrWebPage("<input type=\"password\" id=\"mod_login_password\" name=\"");
	AddStrWebPage(PasswordId);
	AddStrWebPage("\" class=\"user_auth_box\" ");
    if (ParWebServPtr->ServCustomCfg.DemoMode && (strlen(DemoPasswd) > 0))
    {
        AddStrWebPage("value=\"");
        AddStrWebPage(DemoPasswd);
        AddStrWebPage("\" ");
    }    
    AddStrWebPage("size=\"10\">\r\n");        
    AddStrWebPage("</div>\r\n");
	AddStrWebPage("<div class=\"auth_key_request_block\" id=\"confirm_key_req_bt_id\" style=\"background-image: url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtKeyEnKey));        
    AddStrWebPage("); width: 70px; height: 28px;\" onmouseover = \"this.style.backgroundImage = 'url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtKeyMoKey));        
    AddStrWebPage(")'\" onmouseout = \"this.style.backgroundImage = 'url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtKeyEnKey));        
    AddStrWebPage(")'\" onclick=\"AuthKeySentReq();\"></div>\r\n");
    AddStrWebPage("<div id=\"key_auth_block_id\"></div></div>\r\n");

    /* Auth key enter zone */
    AddStrWebPage("<div class=\"auth_exec_zone_block\" id=\"auth_exec_zone_id\">\r\n");
    AddStrWebPage("<div class=\"auth_enter_key_data_block\">");
	AddStrWebPage("<label class=\"user_auth_text\" for=\"auth_conf_key_data\">");
	SetRusTextBuf(NULL, SITE_RUS_AUTH_CONF_KEY_LINE_ID);
	AddStrWebPage("</label>\r\n");
	AddStrWebPage("<br>\r\n");                
    AddStrWebPage("<input type=\"text\" id=\"auth_conf_key_data\" name=\"");
	AddStrWebPage(KeyFormConfirmKey);
	AddStrWebPage("\" class=\"user_auth_box\" size=\"10\" value=\"\" maxlength=\"36\">");       
    AddStrWebPage("</div>\r\n");    
	AddStrWebPage("<div class=\"auth_start_bt_block\" style=\"background-image: url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtEnterEnKey));        
    AddStrWebPage("); width: 70px; height: 28px;\" onmouseover = \"this.style.backgroundImage = 'url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtEnterMoKey));        
    AddStrWebPage(")'\" onmouseout = \"this.style.backgroundImage = 'url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtEnterEnKey));        
    AddStrWebPage(")'\" onclick=\"AuthStartSentReq();\"></div>\r\n");
    AddStrWebPage("</div>\r\n");
    AddStrWebPage("<div class=\"show_user_auth_progress_block\" id=\"user_auth_show_id\"></div>\r\n");     
}
//---------------------------------------------------------------------------
void UserAuthEncodeDataReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{ 
	bool            isParseDone = false;
    char*           FText = NULL;
	char*           FStrt = NULL;
	int             i, pars_read;
	unsigned int    SecKeyForm;     
    char            StrLine[128];

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
        
		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;
        FText = ParseParForm(&FText[i]);
        if (!FText) break;
        if (!strlen(FText)) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);
        
		isParseDone = true;
        break;
    }
    FreeMemory(FStrt);             
    if (!isParseDone)
    {
        *BufAnsw = 0;
        EndHtmlPageGenPtr = BufAnsw;
        AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">");        
        AddStrWebPage("var InfoPrcRes=0;</script>\r\n");      
        ParReadHttpSocketPtr->isCompressRequired = true;
        return;
    }
    UserAuthEncodeGen(SessionPtr->UserAuthEncode);    
    *BufAnsw = 0;
    EndHtmlPageGenPtr = BufAnsw;
    AddStrWebPage("<script type=\"text/javascript\">");        
    AddStrWebPage("var InfoPrcRes=1;var PrvEncKey=[");
    sprintf(StrLine, "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
        SessionPtr->UserAuthEncode[0], SessionPtr->UserAuthEncode[1], SessionPtr->UserAuthEncode[2], 
        SessionPtr->UserAuthEncode[3], SessionPtr->UserAuthEncode[4]);
    AddStrWebPage(StrLine);        
    AddStrWebPage("];</script>\r\n");
    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr, SessionPtr, "AuthEncode.html");
    ParReadHttpSocketPtr->isCompressRequired = true;
}
//---------------------------------------------------------------------------
bool UserAuthDecode(USER_SESSION *SessionPtr, char *EncData, char *LoginPtr, char *PasswdPtr)
{
    return LoginDataDecrypt(SessionPtr->SessionEncodeKeyList, SessionPtr->UserAuthEncode, EncData, LoginPtr, PasswdPtr);
}
//---------------------------------------------------------------------------
void HandleDynAuthConfKeyReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{    
    bool            StartResult;
    unsigned int    BodyLen;    
    char            *MailBodyPtr = NULL;
	bool            isParseDone = false;    
    char*           FText = NULL;
	char*           FStrt = NULL;
	USER_INFO*      UserPtr = NULL;
	int             i, j, pars_read;
    unsigned long   CurrSysTime;
	unsigned int    SecKeyForm;    
    char            UserLogin[MAX_LEN_USER_INFO_USER_NAME+1];
    char            UserPasswd[MAX_LEN_USER_INFO_PASSWD+1];
	char            MailSubject[128];
    char            StrLine[128];

    CurrSysTime = GetTickCount();
	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);

		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;
        FText = ParseParForm(&FText[i]);
        if (!FText) break;
        if (!strlen(FText)) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

        i = FindCmdRequest(FText, "usr_auth");
		if (i == -1) break;
        FText = ParseParForm(&FText[i]);
        if (!FText) break;
        j = strlen(FText);
        if (!j || (j > (MAX_LEN_USER_INFO_USER_NAME + MAX_LEN_USER_INFO_PASSWD + 2))) break;
        if (!UserAuthDecode(SessionPtr, FText, UserLogin, UserPasswd)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

        if ((CurrSysTime - SessionPtr->ConfKeyGenTime) < INTER_CONF_KEY_GEN_INT) break;
		isParseDone = true;
        break;
    }
    FreeMemory(FStrt);             
    if (!isParseDone)
    {
        *BufAnsw = 0;
        EndHtmlPageGenPtr = BufAnsw;
        AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">\r\n");        
        AddStrWebPage("var KeyDeliveryConfirm=0;\r\n");
        AddStrWebPage("var ClientDevInfo=[0, 0, 0];\r\n</script>\r\n");
        AddStrWebPage("No access to confirm key gen\r\n");  
        ParReadHttpSocketPtr->isCompressRequired = true;
        return;
    }
    UserPtr = CheckLoginUserInfoDb(UserLogin, UserPasswd);
    if (UserPtr)
    {
        ConfirmKeyGen(ParReadHttpSocketPtr->DeviceType, SessionPtr->ConfirmKey);
        SessionPtr->ConfKeyGenTime = CurrSysTime;
        if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)    
        {
            if (!ParWebServPtr->ServCustomCfg.DemoMode)
            {
                strcpy(MailSubject, "Confirmation Key");
	            MailBodyPtr = (char*)AllocateMemory(512*sizeof(char));
	            *MailBodyPtr = 0;
                EndHtmlPageGenPtr = MailBodyPtr;

                AddStrWebPage("One-time confirmatin key: ");
                AddStrWebPage(SessionPtr->ConfirmKey);
                AddStrWebPage("\n--------------------------------------------\n");    
                AddStrWebPage("Please do not replay for this message.\nThis message was generated automaticaly\n\n");
                AddStrWebPage("Best regards,\nInfrastructure Monitoring System!\n");

                EndHtmlPageGenPtr = MailBodyPtr;

                BodyLen = strlen(MailBodyPtr);       
	            StartResult = SendMailSmtpServer(&ParWebServPtr->MailWorker,
		            UserPtr->Contact.Email, MailSubject, MailBodyPtr);
            }
        }
        else
        {
            if (!ParWebServPtr->ServCustomCfg.DemoMode)
            {
                StartResult = SendSmsToServer(&ParWebServPtr->SmsWorker, UserPtr->Contact.MobilePhone, 
			         false, (unsigned char*)SessionPtr->ConfirmKey, NULL, 0, 0);             
            }
        }    
        SessionPtr->isConfKeySent = true; 
    }

    *BufAnsw = 0;
    EndHtmlPageGenPtr = BufAnsw;

    AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">\r\n");        
    if (UserPtr)
    {
        AddStrWebPage("var KeyDeliveryConfirm=1;\r\n");
        if (ParWebServPtr->ServCustomCfg.DemoMode)
        {
            AddStrWebPage("var DemoConfirmKey=\"");
            AddStrWebPage(SessionPtr->ConfirmKey);
            AddStrWebPage("\";\r\n");
        }        
    }
    else AddStrWebPage("var KeyDeliveryConfirm=0;\r\n");    
    AddStrWebPage("var ClientDevInfo=[");

	if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
	{
		if (!ParReadHttpSocketPtr->MobileType)
		{
            AddStrWebPage("1, 320, 400");
		}
		else
		{
            sprintf(StrLine, "1, %d, %d", ParReadHttpSocketPtr->MobileType->AspectWidth,
                ParReadHttpSocketPtr->MobileType->AspectHeigh);
            AddStrWebPage(StrLine);
		}
	}
	else AddStrWebPage("0, 0, 0");
    AddStrWebPage("];\r\n</script>\r\n");
            
    AddStrWebPage("<div class=\"key_confirm_block\" id=\"key_confirm_div\">\r\n");
    
	AddStrWebPage("  <div class=\"key_confirm_title\">\r\n");
    AddStrWebPage("    <div class=\"key_confirm_title_text\">");
	SetRusTextBuf(NULL, SITE_RUS_KEY_NOTIFY_LINE_ID);
    AddStrWebPage("</div>\r\n");
    AddStrWebPage("  </div>\r\n");

	AddStrWebPage("  <div class=\"key_confirm_info\">\r\n");

    /* Form body */
	AddStrWebPage("    <center><b><span style=\"");
    if (UserPtr) 
    {
        AddStrWebPage("color: green;font-weight: bold;\">");
        if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP) SetRusTextBuf(NULL, SITE_RUS_KEY_DELIV_INFO_LINE_ID);
        else                                                 SetRusTextBuf(NULL, SITE_RUS_KEY_DELIV_SMS_INFO_LINE_ID);
    }
    else
    {
        AddStrWebPage("color: red;font-weight: bold;\">");
        SetRusTextBuf(NULL, SITE_RUS_NO_USER_KEY_DELIVERY_LINE_ID);
    }
    AddStrWebPage("</b></span></center>\r\n");

    AddStrWebPage("  </div>\r\n");
    AddStrWebPage("  <div class=\"key_confirm_manage\">\r\n");
    
	AddStrWebPage("    <div class=\"key_confirm_mg_bt_ok\">\r\n");
    AddStrWebPage("      <div style=\"background-image: url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtOkEnKey));
    AddStrWebPage("); width: 100px; height: 32px; \" ");
    AddStrWebPage("onclick=\"OnReadConfKeyNotify();\" ");
	AddStrWebPage("onmouseover = \"this.style.backgroundImage = 'url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtOkMoKey));
	AddStrWebPage(")'\" ");
	AddStrWebPage("onmouseout =  \"this.style.backgroundImage = 'url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtOkEnKey));
	AddStrWebPage(")'\">\r\n");
    AddStrWebPage("        <div style=\"background-image: url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(EmptyBlkKey));
	AddStrWebPage("); width: 100px; height: 32px;\"></div>\r\n");
    AddStrWebPage("      </div>\r\n");
	AddStrWebPage("    </div>\r\n");
    AddStrWebPage("  </div>\r\n");    
	AddStrWebPage("</div>\r\n");
    ParReadHttpSocketPtr->isCompressRequired = true;
}
//---------------------------------------------------------------------------
void SetRegUserExitBtTimeWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	if (SessionPtr->UserPtr)
	{
	    AddStrWebPage("<div class=\"user_exit_group\">\r\n");
	    AddStrWebPage("<div class=\"user_exit_title\" id=\"div_user_exit_title\" >");
	    SetRusTextBuf(NULL, SITE_RUS_SESSION_INFO_LINE_ID);
	    AddStrWebPage("</div>\r\n");
	    AddStrWebPage("<div class=\"user_exit_container\" id=\"div_user_exit_cont\">\r\n");

		SetRusTextBuf(NULL, SITE_RUS_AUTH_USER_LINE_ID);
		AddStrWebPage(": ");
		SetRusTextBufName(NULL, (unsigned char*)&SessionPtr->UserPtr->Name[0]);
	    AddStrWebPage("<form action=\"");
		AddStrWebPage(GenPageUserExitReq);
		AddStrWebPage("\" method=\"post\" name=\"logout\" >\r\n");
        AddStrWebPage("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"");
	    SetRusTextBuf(NULL, SITE_RUS_AUTH_EXIT_LINE_ID);
	    AddStrWebPage("\" >\r\n");

		if ((SessionPtr->UserPtr) || 
	        ((!ParReadHttpSocketPtr->CookieSessionId) && 
		    (ParReadHttpSocketPtr->BotType == BOT_NONE)))
		{
			SetHiddenStrParForm(NULL, KeyFormSessionId, SessionPtr->SesionIdKey);
		}
        SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
		SetHiddenStrParForm(NULL, KeyUserNameId, SessionPtr->UserPtr->UserName);

	    AddStrWebPage("</form>\r\n");
	    AddStrWebPage("</div></div>\r\n");
	    AddStrWebPage("<div class=\"inter_block_spase\"></div>\r\n");
	}
}
//---------------------------------------------------------------------------
void SetDateSessionTimeWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
    /* Place date and time till session close information */
	if (ParReadHttpSocketPtr->BotType != BOT_NONE) return;
	AddStrWebPage("<div class=\"UpTimeSessBox\" id=\"up_right_box\">\r\n");
	AddStrWebPage("<div class=\"UpDateInfZone\">\r\n");
    SetDateWebPage();
    AddStrWebPage("</div>\r\n");
    if (ParReadHttpSocketPtr->BotType == BOT_NONE)
	{
        AddStrWebPage("<div class=\"UpSessionInfZone\">\r\n");
		if ((SessionPtr->UserPtr) || 
		    (!SessionPtr->UserPtr && 
			 ParWebServPtr->ServCustomCfg.AnonymTimoutExpInfo))
		{
		    AddStrWebPage("<div id=\"timer\" align=\"center\"></div>\r\n");
		}
		AddStrWebPage("</div>\r\n");
        LanguageSelectBlockSet(SessionPtr);      
		SpaseResizeBlockSet(SessionPtr);
	}    
	AddStrWebPage("</div>\r\n");
}
//---------------------------------------------------------------------------
void SetActUsersInfoWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	/* Show information about number of active clients */
	if (ParReadHttpSocketPtr->BotType != BOT_NONE) return;
	AddStrWebPage("<div class=\"act_users_group\">\r\n");
	AddStrWebPage("<div class=\"act_users_title\" id=\"div_act_users_title\" >");
	SetRusTextBuf(NULL, SITE_RUS_WHO_ON_SITE_LINE_ID);
	AddStrWebPage("</div>\r\n");
	AddStrWebPage("<div class=\"act_users_container\" id=\"div_act_usrts_cont\">\r\n");
	SetNumActiveUsersPage();
	AddStrWebPage("</div></div>\r\n");
	AddStrWebPage("<div class=\"inter_block_spase\"></div>\r\n");
}
//---------------------------------------------------------------------------
void ServLangSelect(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	int i, ReadVal, pars_read;
	unsigned int TextLen;
    char*      FText = NULL;
	char*      FStrt = NULL;
	char       CmdBuf[128];

	for (;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
		if (ParReadHttpSocketPtr->BotType != BOT_NONE) break;
		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, FormKeyLangId);
		if (i == -1) break;
		FText = ParseParForm(&FText[i]);
        if (!FText) break;
		TextLen = strlen(FText);
		if (!TextLen) break;
		pars_read = sscanf(FText, "%d", &ReadVal);
		if (!pars_read) break;
        if ((ReadVal < MIN_LANG_INDEX) || (ReadVal > MAX_LANG_INDEX)) break;
		SessionPtr->LanguageType = (unsigned char)ReadVal;
        gLanguageType = (unsigned char)ReadVal;
		FText = FStrt;
		strcpy(FText,HttpCmd);
	    break;
	}
    if ((SessionPtr->UserPtr) && 
		(ParWebServPtr->SessionManager.SysShowUserType > 0) &&
		(SessionPtr->UserPtr->UserType == ParWebServPtr->SessionManager.SysShowUserType))
    {
        MainSystemShowWebPage(BufAnsw, SessionPtr, HttpCmd);
    }
    else
    {
        MainPageSetShopWebPage(BufAnsw, SessionPtr, HttpCmd);
    }
	if (FText) FreeMemory(FText);
}
//---------------------------------------------------------------------------
void LanguageSelectBlockSet(USER_SESSION *SessionPtr)
{
    char TextLine[128];
    
    AddStrWebPage("<div class=\"sel_language_block\" id=\"sel_lang_blk\">\r\n");
	AddStrWebPage("<form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageLangSelect);    
	AddStrWebPage("\" method=\"post\" name=\"LagChangeReqForm\">\r\n");

	SetHiddenStrParForm(NULL, KeyFormSessionId, SessionPtr->SesionIdKey);

    if (gLanguageType == LGT_ENGLISH)
    {
        SetHiddenIntParForm(NULL, FormKeyLangId, LGT_RUSSIAN);
        
        AddStrWebPage("<div class=\"show_eng_flag_image\" ");
        AddStrWebPage(" id=\"eng_lang_sel_image\"></div>\r\n");
        AddStrWebPage("<div class=\"show_rus_flag_image\" ");
	    strcpy(TextLine, "('rus_lang_sel_image', 'LagChangeReqForm');\" ");
	    AddStrWebPage("onMouseOver=\"on_sel_lang_mouseover");
	    AddStrWebPage(TextLine);
	    AddStrWebPage("onMouseOut=\"on_sel_lang_mouseout");
	    AddStrWebPage(TextLine);
	    AddStrWebPage("onClick=\"on_sel_lang_click");
	    AddStrWebPage(TextLine);
        AddStrWebPage(" id=\"rus_lang_sel_image\"></div>\r\n");    
    }
    else
    {
        SetHiddenIntParForm(NULL, FormKeyLangId, LGT_ENGLISH);
        
        AddStrWebPage("<div class=\"show_eng_flag_image\" ");
	    strcpy(TextLine, "('eng_lang_sel_image', 'LagChangeReqForm');\" ");
	    AddStrWebPage("onMouseOver=\"on_sel_lang_mouseover");
	    AddStrWebPage(TextLine);
	    AddStrWebPage("onMouseOut=\"on_sel_lang_mouseout");
	    AddStrWebPage(TextLine);
	    AddStrWebPage("onClick=\"on_sel_lang_click");
	    AddStrWebPage(TextLine);
        AddStrWebPage(" id=\"eng_lang_sel_image\"></div>\r\n");
        AddStrWebPage("<div class=\"show_rus_flag_image\" ");
        AddStrWebPage(" id=\"rus_lang_sel_image\"></div>\r\n");    
    }    
    AddStrWebPage("</form></div>\r\n");
}
//---------------------------------------------------------------------------
void SetRightLocBannerListWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	unsigned int InitCount = 0;
	bool         FirstBanLine = true;
	unsigned int BannerCount = 0;
	ObjListTask  *SelObjPtr = NULL;
	BANNER_INFO  *SelBannerPtr = NULL;

	if ((ParReadHttpSocketPtr->BotType == BOT_NONE) && 
		(ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP) &&
		((!SessionPtr->UserPtr) || (SessionPtr->UserPtr && SessionPtr->UserPtr->UserType != UAT_ADMIN)))
	{
        AddStrWebPage("<table border=\"0\" cellspacing=\"2\" cellpadding=\"2\">\r\n");
        AddStrWebPage("<tr>\r\n");

		BannerCount = 0;
		FirstBanLine = true;
	    SelObjPtr = (ObjListTask*)GetFistObjectList(&ParWebServPtr->BannerList);
	    while(SelObjPtr)
		{
	        SelBannerPtr = (BANNER_INFO*)SelObjPtr->UsedTask;
			if (SelBannerPtr->Location == BPL_PAGE_RIGHT_AREA)
			{
				AddStrWebPage("<td>\r\n");
                AddStrWebPage(SelBannerPtr->BodyPtr);
				AddStrWebPage("</td>\r\n");
				BannerCount++;
				if (FirstBanLine) InitCount++;
			}
			if (BannerCount == 2)
			{
				AddStrWebPage("</tr><tr>\r\n");
				BannerCount = 0;
				FirstBanLine = false;
			}
		    SelObjPtr = (ObjListTask*)GetNextObjectList(&ParWebServPtr->BannerList);
		}
        AddStrWebPage("</tr>\r\n");
		AddStrWebPage("</table>\r\n");
	}
}
//---------------------------------------------------------------------------
void SetDownLocBannerListWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	unsigned int InitCount = 0;
	unsigned int BannerPerLine;
	bool         FirstBanLine = true;
	unsigned int BannerCount = 0;
	ObjListTask  *SelObjPtr = NULL;
	BANNER_INFO  *SelBannerPtr = NULL;
	char         BufLine[64];

	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP) BannerPerLine = 6;
	else                                                 BannerPerLine = 3;
	if ((ParReadHttpSocketPtr->BotType == BOT_NONE) && ((!SessionPtr->UserPtr) ||
		(SessionPtr->UserPtr && SessionPtr->UserPtr->UserType != UAT_ADMIN)))
	{
        AddStrWebPage("<div align=\"center\">\r\n");
        AddStrWebPage("<table border=\"0\" cellspacing=\"2\" cellpadding=\"2\">\r\n");
        AddStrWebPage("<tr>\r\n");

		BannerCount = 0;
		FirstBanLine = true;
	    SelObjPtr = (ObjListTask*)GetFistObjectList(&ParWebServPtr->BannerList);
	    while(SelObjPtr)
		{
	        SelBannerPtr = (BANNER_INFO*)SelObjPtr->UsedTask;
			if (SelBannerPtr->Location == BPL_PAGE_DOWN_AREA)
			{
				AddStrWebPage("<td>\r\n");
                AddStrWebPage(SelBannerPtr->BodyPtr);
				AddStrWebPage("</td>\r\n");
				BannerCount++;
				if (FirstBanLine) InitCount++;
			    if (BannerCount == BannerPerLine)
				{
				    AddStrWebPage("</tr><tr>\r\n");
				    BannerCount = 0;
					FirstBanLine = false;
				}
			}
		    SelObjPtr = (ObjListTask*)GetNextObjectList(&ParWebServPtr->BannerList);
		}
        AddStrWebPage("</tr>\r\n");
        sprintf(BufLine, "<tr><td colspan=\"%d\">\r\n", InitCount);
		AddStrWebPage(BufLine);

	    AddStrWebPage("<div align=\"center\">&copy;&nbsp;2014-2017&nbsp;");
        AddSpaceSetStrWebPage(GetSystemOvnerCompany());
        AddStrWebPage(HtmlSpaceTag);
        AddStrWebPage(ParWebServPtr->ShopInfoCfg.Name);
	    AddStrWebPage("</div>\r\n");
	    AddStrWebPage("</td></tr></table></div>\r\n");
	}
    else
	{
	    AddStrWebPage("<div align=\"center\">&copy;&nbsp;2014-2017&nbsp;");
        AddSpaceSetStrWebPage(GetSystemOvnerCompany());
        AddStrWebPage(HtmlSpaceTag);
        AddStrWebPage(ParWebServPtr->ShopInfoCfg.Name);
	    AddStrWebPage("</div>\r\n");
	}
}
//---------------------------------------------------------------------------
void SpaseResizeBlockSet(USER_SESSION *SessionPtr)
{
    char TextLine[128];
    
    AddStrWebPage("<div class=\"spase_resize_block\" id=\"spase_resize_blk\">\r\n");
	AddStrWebPage("<form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageSpaseResize);
	AddStrWebPage("\" method=\"post\" name=\"SpaseResizeReqForm\">\r\n");

	SetHiddenStrParForm(NULL, KeyFormSessionId, SessionPtr->SesionIdKey);
    SetHiddenIntParForm(NULL, KeyFormScreenWidth, 0);
	SetHiddenIntParForm(NULL, KeyFormScreenHeight, 0);
        
    AddStrWebPage("<div class=\"show_spase_resize_image\" ");
	strcpy(TextLine, "('spase_resize_image');\" ");
	AddStrWebPage("onMouseOver=\"on_spase_resize_mouseover");
	AddStrWebPage(TextLine);
	AddStrWebPage("onMouseOut=\"on_spase_resize_mouseout");
	AddStrWebPage(TextLine);
	AddStrWebPage("onClick=\"on_spase_resize_click");
	AddStrWebPage(TextLine);
    AddStrWebPage(" id=\"spase_resize_image\"></div>\r\n");
		
    AddStrWebPage("</form></div>\r\n");
}
//---------------------------------------------------------------------------
void HandlePageSpaseResize(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	int i, ReadVal, pars_read;
	unsigned int TextLen;
    char*      FText = NULL;
	char*      FStrt = NULL;
	char       CmdBuf[128];

	for (;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
		if (!FText) break;
	    FStrt = FText;
        strcpy(FText, HttpCmd);

		if (ParReadHttpSocketPtr->BotType != BOT_NONE) break;
		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		// Screen width form field value extract
		i = FindCmdRequest(FText, KeyFormScreenWidth);
		if (i == -1) break;
		FText = ParseParForm(&FText[i]);
        if (!FText) break;
		TextLen = strlen(FText);
		if (!TextLen) break;
		pars_read = sscanf(FText, "%d", &ReadVal);
		if (!pars_read) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);
		SessionPtr->UserScreenWidth = (unsigned short)ReadVal;
		
		// Screen height form field value extract
		i = FindCmdRequest(FText, KeyFormScreenHeight);
		if (i == -1) break;
		FText = ParseParForm(&FText[i]);
        if (!FText) break;
		TextLen = strlen(FText);
		if (!TextLen) break;
		pars_read = sscanf(FText, "%d", &ReadVal);
		if (!pars_read) break;		
		FText = FStrt;
		strcpy(FText,HttpCmd);		
		
		SessionPtr->UserScreenHeight = (unsigned short)ReadVal;
		if (SessionPtr->UserScreenWidth < 1280)
		{
			SessionPtr->SpaseType = 1;
		}
		else if ((SessionPtr->UserScreenWidth > 1279) && 
		      (SessionPtr->UserScreenWidth < 1600))
		{
			SessionPtr->SpaseType = 2;
		}
		else if ((SessionPtr->UserScreenWidth > 1599) && 
		      (SessionPtr->UserScreenWidth < 1900))
		{
			SessionPtr->SpaseType = 3;
		}		
		else
		{
			SessionPtr->SpaseType = 4;
		}
	    break;
	}
	
    MainPageSetShopWebPage(BufAnsw, SessionPtr, HttpCmd);
	if (FText) FreeMemory(FText);
}
//---------------------------------------------------------------------------
void HandleDynConfKeyReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
    bool            StartResult;
    unsigned int    BodyLen;    
    char            *MailBodyPtr = NULL;
	char            MailSubject[128];
    char            StrLine[128];
            
    if (!SessionPtr->UserPtr) return;

    ConfirmKeyGen(ParReadHttpSocketPtr->DeviceType, SessionPtr->ConfirmKey);
    if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)    
    {
	    DebugLogPrint(NULL, "%s: Start create mail for single-time key delivery\r\n", 
            ThrWebServName);

        /* Check for WEB server is not in demo mode. */
        if (!ParWebServPtr->ServCustomCfg.DemoMode)
        {
            strcpy(MailSubject, "Confirmation Key");
	        MailBodyPtr = (char*)AllocateMemory(512*sizeof(char));
	        *MailBodyPtr = 0;
            EndHtmlPageGenPtr = MailBodyPtr;

            AddStrWebPage("One-time confirmatin key: ");
            AddStrWebPage(SessionPtr->ConfirmKey);
            AddStrWebPage("\n--------------------------------------------\n");    
            AddStrWebPage("Please do not replay for this message.\nThis message was generated automaticaly\n\n");
            AddStrWebPage("Best regards,\nInfrastructure Monitoring System!\n");

            EndHtmlPageGenPtr = MailBodyPtr;

            BodyLen = strlen(MailBodyPtr);       
	        StartResult = SendMailSmtpServer(&ParWebServPtr->MailWorker,
		        SessionPtr->UserPtr->Contact.Email, MailSubject, MailBodyPtr);
        }
    }
    else
    {
	    DebugLogPrint(NULL, "%s: Start create SMS for single-time key delivery\r\n", 
            ThrWebServName);
    
        if (!ParWebServPtr->ServCustomCfg.DemoMode)
        {
            StartResult = SendSmsToServer(&ParWebServPtr->SmsWorker, SessionPtr->UserPtr->Contact.MobilePhone, 
			     false, (unsigned char*)SessionPtr->ConfirmKey, NULL, 0, 0);             
        }
    }    
    SessionPtr->isConfKeySent = true;

    *BufAnsw = 0;
    EndHtmlPageGenPtr = BufAnsw;

    AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">\r\n");   
    if (ParWebServPtr->ServCustomCfg.DemoMode)
    {
        AddStrWebPage("var DemoConfirmKey=\"");
        AddStrWebPage(SessionPtr->ConfirmKey);
        AddStrWebPage("\";\r\n");
    }         
    AddStrWebPage("var ClientDevInfo=[");

	if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
	{
		if (!ParReadHttpSocketPtr->MobileType)
		{
            AddStrWebPage("1, 320, 400");            
		}
		else
		{
            sprintf(StrLine, "1, %d, %d", ParReadHttpSocketPtr->MobileType->AspectWidth,
                ParReadHttpSocketPtr->MobileType->AspectHeigh);
            AddStrWebPage(StrLine);
		}
	}
	else AddStrWebPage("0, 0, 0");
    AddStrWebPage("];\r\n</script>\r\n");
            
    AddStrWebPage("<div class=\"key_confirm_block\" id=\"key_confirm_div\">\r\n");
    
	AddStrWebPage("  <div class=\"key_confirm_title\">\r\n");
    AddStrWebPage("    <div class=\"key_confirm_title_text\">");
	SetRusTextBuf(NULL, SITE_RUS_KEY_NOTIFY_LINE_ID);
    AddStrWebPage("</div>\r\n");
    AddStrWebPage("  </div>\r\n");

	AddStrWebPage("  <div class=\"key_confirm_info\">\r\n");

    /* Form body */
    AddStrWebPage("    <center><span style=\"color: green;font-weight: bold;\">");
    if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP) SetRusTextBuf(NULL, SITE_RUS_KEY_DELIV_INFO_LINE_ID);
    else                                                 SetRusTextBuf(NULL, SITE_RUS_KEY_DELIV_SMS_INFO_LINE_ID);
    AddStrWebPage("</span></center>\r\n");

    AddStrWebPage("  </div>\r\n");
    AddStrWebPage("  <div class=\"key_confirm_manage\">\r\n");
    
	AddStrWebPage("    <div class=\"key_confirm_mg_bt_ok\">\r\n");
    AddStrWebPage("      <div style=\"background-image: url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtOkEnKey));
    AddStrWebPage("); width: 100px; height: 32px; \" ");
    AddStrWebPage("onclick=\"OnReadConfKeyNotify();\" ");
	AddStrWebPage("onmouseover = \"this.style.backgroundImage = 'url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtOkMoKey));
	AddStrWebPage(")'\" ");
	AddStrWebPage("onmouseout =  \"this.style.backgroundImage = 'url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtOkEnKey));
	AddStrWebPage(")'\">\r\n");
    AddStrWebPage("        <div style=\"background-image: url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(EmptyBlkKey));
	AddStrWebPage("); width: 100px; height: 32px;\"></div>\r\n");
    AddStrWebPage("      </div>\r\n");
	AddStrWebPage("    </div>\r\n");
    AddStrWebPage("  </div>\r\n");    
	AddStrWebPage("</div>\r\n");    
}
//---------------------------------------------------------------------------
void HandleDynStatusShowSessIdReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
    *BufAnsw = 0;
    EndHtmlPageGenPtr = BufAnsw;
	DebugLogPrint(NULL, "%s: Assign new key (%s) for trusted site (Socket: %d)\r\n",
	    ThrWebServName, SessionPtr->SesionIdKey, ParReadHttpSocketPtr->HttpSocket);
    AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">");
    AddStrWebPage("var CurrentSessionId=\"");
    AddStrWebPage(SessionPtr->SesionIdKey);
    AddStrWebPage("\";</script>\r\n");
    SystemVersionSet();
}
//---------------------------------------------------------------------------
void SystemVersionSet()
{
    AddStrWebPage("&copy;&nbsp;2014-2017&nbsp;");
    AddSpaceSetStrWebPage(GetSystemOvnerCompany());
    AddStrWebPage("&nbsp;Version:&nbsp;");
    AddStrWebPage(ServerVersion);
}
//---------------------------------------------------------------------------
void HandleDynServDateTimeReq(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	char           CmdGenBuf[164];
#ifdef WIN32
	SYSTEMTIME     SysTime;
#else
    struct timeb hires_cur_time;
	struct tm      *SysTime;
#endif

    *BufAnsw = 0;
    EndHtmlPageGenPtr = BufAnsw;
#ifdef WIN32
    GetSystemTime(&SysTime);
	sprintf(CmdGenBuf,"%02d.%02d.%04d&nbsp;%02d:%02d:%02d",
        SysTime.wDay, SysTime.wMonth, SysTime.wYear,
        SysTime.wHour, SysTime.wMinute, SysTime.wSecond);
#else 
    ftime(&hires_cur_time);
    SysTime = localtime(&hires_cur_time.time);
	sprintf(CmdGenBuf,"%02d.%02d.%04d&nbsp;%02d:%02d:%02d",
        SysTime->tm_mday, SysTime->tm_mon+1, SysTime->tm_year+1900,
        SysTime->tm_hour, SysTime->tm_min, SysTime->tm_sec);
#endif
	AddStrWebPage(CmdGenBuf);
    SessionPtr->isActive = true;
}
//---------------------------------------------------------------------------
