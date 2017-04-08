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
#include "ThrWebServer.h"
#include "ThrCernel.h"
#include "SysWebFunction.h"
#include "HttpPageGen.h"
#include "ImageNameHash.h"

extern char ServerHttpAddr[];
extern char KeySessionId[];
extern char KeyUserNameId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char KeyUserId[];
extern char KeyOrderId[];
extern char KeyStatusId[];
extern ListItsTask  UserInfoList;
extern ListItsTask  UserSessionList;
extern ListItsTask  GroupList;
extern unsigned int UserPreffixArray[];
extern char *EndHtmlPageGenPtr;
extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK *ParReadHttpSocketPtr;
extern USER_DB_INFO SampleUserDbIfo;

extern char KeySectionId[];
extern char KeyItemId[];
extern char KeyFormStatusId[];
extern char KeyFormUserId[];
extern char KeyFormSessionId[];
extern char KeyFormOrderId[];
extern char FormKeySessionId[];
extern char FormKeyUserId[];

char KeyDelUserId[] = "del_usr_id=";
char KeyChgUserTd[] = "chg_usr_id=";
char KeyChgUserType[] = "chg_usr_type=";
char KeyGrpSetId[] = "uah_group_id";
char FormKeyChgUserId[] = "chg_usr_id";

void RegClientsListPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void GroupAccessListEditPageGen(char *BufAnsw, USER_SESSION *SessionPtr, 
    char *HttpCmd, USER_INFO *ChgUserPtr);
//---------------------------------------------------------------------------
void RegClientsList(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	int          i, pars_read;
	unsigned int SecKeyForm, UserId;
	ObjListTask  *SelObjPtr = NULL;

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;
        FText = ParseParForm( &FText[i] );
        if (!FText) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyUserId);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &UserId);
		if (!pars_read) break;
		if (UserId != SessionPtr->UserPtr->UserId) break;

		FText = FStrt;
		strcpy(FText,HttpCmd);
		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
		RegClientsListPageGen(BufAnsw, SessionPtr, HttpCmd);
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void DelUserRegShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	int          i, pars_read;
	unsigned int SecKeyForm, UserId, DelUserId;
	ObjListTask  *SelObjPtr = NULL;
	ObjListTask  *SelSessionObjPtr = NULL;
	USER_INFO    *UserPtr = NULL;
	USER_SESSION *SelSessionPtr = NULL;

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;
        FText = ParseParForm( &FText[i] );
        if (!FText) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyUserId);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &UserId);
		if (!pars_read) break;
		if (UserId != SessionPtr->UserPtr->UserId) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyDelUserId);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &DelUserId);
		if (!pars_read) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);
	    SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	    while(SelObjPtr)
		{
	        UserPtr = (USER_INFO*)SelObjPtr->UsedTask;
		    if ((UserPtr->UserId == DelUserId) && (UserPtr->UserType != UAT_ADMIN))
			{
		        RemStructList(&UserInfoList, SelObjPtr);
	            SelSessionObjPtr = (ObjListTask*)GetFistObjectList(&UserSessionList);
	            while(SelSessionObjPtr)
				{
                    SelSessionPtr = (USER_SESSION*)SelSessionObjPtr->UsedTask;
					if (SelSessionPtr->UserPtr == UserPtr)
					{
						/* Change active user's session to anonym session */
						// TBD
                        SelSessionPtr->UserPtr = NULL;
						break;
					}
                    SelSessionObjPtr = (ObjListTask*)GetNextObjectList(&UserSessionList);
				}
                FreeMemory(UserPtr);
				UserInfoDBSave(&SampleUserDbIfo);
				break;
			}
			SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
		}
		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
		RegClientsListPageGen(BufAnsw, SessionPtr, HttpCmd);
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void ChgUserHostAccessGrpWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	int          i, pars_read;
	unsigned int SecKeyForm, UserId, ChgUserId;
    USER_INFO    *ChgUserPtr = NULL;

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;
        FText = ParseParForm( &FText[i] );
        if (!FText) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyUserId);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &UserId);
		if (!pars_read) break;
		if (UserId != SessionPtr->UserPtr->UserId) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyChgUserTd);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &ChgUserId);        
		if (!pars_read) break;
        ChgUserPtr = GetUserInfoById(ChgUserId);
        if (!ChgUserPtr) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);
		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
		GroupAccessListEditPageGen(BufAnsw, SessionPtr, HttpCmd, ChgUserPtr);
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void ChgUserGroupSetWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    bool         isStepPass = true;
    char*        FText = NULL;
	char*        FStrt = NULL;
	int          i, j, pars_read, TextLen;
	unsigned int SecKeyForm, UserId, ChgUserId, GroupId;
    unsigned long long int GroupMask;
    USER_INFO    *ChgUserPtr = NULL;

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;
        FText = ParseParForm( &FText[i] );
        if (!FText) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyUserId);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &UserId);
		if (!pars_read) break;
		if (UserId != SessionPtr->UserPtr->UserId) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, FormKeyChgUserId);
		if (i == -1) break;
		FText = ParseParForm(&FText[i]);
        if (!FText) break;
		TextLen = strlen(FText);
		if (!TextLen) break;
		pars_read = sscanf(FText, "%d", &ChgUserId);        
		if (!pars_read) break;
        ChgUserPtr = GetUserInfoById(ChgUserId);
        if (!ChgUserPtr) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);
           
        GroupMask = 0;
        j = 0;
        for(;;)
        {     
		    i = FindCmdRequest(&FText[j], KeyGrpSetId);
		    if (i == -1) break;
            isStepPass = false;
		    FText = ParseParForm(&FText[j+i]);
            if (!FText) break;
		    TextLen = strlen(FText);
		    if (!TextLen) break;
		    pars_read = sscanf(FText, "%d", &GroupId);
		    if (!pars_read) break;
            if ((GroupId < 0) || (GroupId > MAX_USER_GRP_ID)) break;
            GroupMask |= ((unsigned long long int)1 << GroupId);
            isStepPass = true;
            j += i;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
        }
        if (!isStepPass) break;
        ChgUserPtr->GroupMask = GroupMask;
        UserInfoDBSave(&SampleUserDbIfo);
		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
        RegClientsListPageGen(BufAnsw, SessionPtr, HttpCmd);
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void GroupAccessListEditPageGen(char *BufAnsw, USER_SESSION *SessionPtr, 
    char *HttpCmd, USER_INFO *ChgUserPtr)
{
    unsigned int    SelPerLine = 0;
    unsigned int    MaxSelLine = 4;
    unsigned int    Index = 1;
	ObjListTask     *SelObjPtr = NULL;
	GROUP_INFO_TYPE *GroupPtr = NULL;
	char            CmdGenBuf[512];
    
    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE) MaxSelLine = 2;

	// Form header
    AddStrWebPage("<h3><center><strong>\r\n");
    SetRusTextBuf(NULL, SITE_RUS_EDIT_GRP_USER_INFO_LINE_ID);
	if (ChgUserPtr->Contact.UserTitleId > SITE_RUS_ORDER_REQ_TYPE_1_LINE_ID)
	{
		SetRusTextBuf(NULL, UserPreffixArray[ChgUserPtr->Contact.UserTitleId]);
		AddStrWebPage(" ");
	}
	sprintf(CmdGenBuf, "%s %s %s</strong>\r\n", 
		ChgUserPtr->Contact.FirstName, ChgUserPtr->Contact.LastName,
		ChgUserPtr->Contact.MiddleName);
	AddStrWebPage(CmdGenBuf);    
    
    AddStrWebPage("</center></h3><br>\r\n");

	AddStrWebPage("<table width=\"80%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr><td>\r\n");

	AddStrWebPage("<fieldset>\r\n<legend><span class=\"sectiontableheader\">\r\n");
    SetRusTextBuf(NULL, SITE_RUS_EDIT_USER_HOST_ACC_GRP_LINE_ID);
    AddStrWebPage("</span></legend>\r\n");

    AddStrWebPage("<form action=\"");
	AddStrWebPage(GenPageChgGrpSet);
	AddStrWebPage("\" name=\"GroupSetForm\" method=\"post\">\r\n");

	SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);
    SetHiddenIntParForm(NULL, FormKeyChgUserId, ChgUserPtr->UserId);
    
    AddStrWebPage("<table cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr align=\"left\">\r\n");
    
    /* Select common group */
    AddStrWebPage("<td width=\"120\"><input type=\"checkbox\" ");
    sprintf(CmdGenBuf, "id=\"cb_grp_id_%d\" name=\"%s\" value=\"%d\" ", 
        Index, KeyGrpSetId, 0);
    AddStrWebPage(CmdGenBuf);
    if (ChgUserPtr->GroupMask & 0x01) AddStrWebPage(" checked ");
	AddStrWebPage("class=\"inputbox\" >&nbsp;\r\n");
    SetRusTextBuf(NULL, SITE_RUS_SET_COMMON_GROUP_LINE_ID);
    AddStrWebPage("</td>\r\n");        
    SelPerLine++;
    Index++;
     
    /* Select user's group */  
	SelObjPtr = (ObjListTask*)GetFistObjectList(&GroupList);
	while(SelObjPtr)
	{
        GroupPtr = (GROUP_INFO_TYPE*)SelObjPtr->UsedTask;        
        AddStrWebPage("<td width=\"120\"><input type=\"checkbox\" ");
        sprintf(CmdGenBuf, "id=\"cb_grp_id_%d\" name=\"%s\" value=\"%d\" ", 
            Index, KeyGrpSetId, GroupPtr->GroupId);
        AddStrWebPage(CmdGenBuf);
        if (ChgUserPtr->GroupMask & (0x01 << GroupPtr->GroupId)) AddStrWebPage(" checked ");
	    AddStrWebPage("class=\"inputbox\" >&nbsp;\r\n");
        AddStrWebPage(&GroupPtr->GroupName[0]);
        AddStrWebPage("</td>\r\n");        
        SelPerLine++;
        Index++;
        if (SelPerLine == MaxSelLine)
        {
            SelPerLine = 0;
            AddStrWebPage("</tr><tr>\r\n");
        }        
        SelObjPtr = (ObjListTask*)GetNextObjectList(&GroupList);
    }
    AddStrWebPage("</tr></table><br></td></tr>\r\n");    

    AddStrWebPage("<tr><td>\r\n"); 
	AddStrWebPage("<table cellspacing=\"4\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr><td>\r\n");
    
	// Key for group record save
    AddStrWebPage("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"");
    SetRusTextBuf(NULL, SITE_RUS_SHOP_DB_ITEM_SAVE_LINE_ID);
	AddStrWebPage("\" >\r\n");    
    
    AddStrWebPage("</td><td>\r\n");
    
    /* Return button */
    AddStrWebPage("<div><a href=\"");
	SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageMain);
	SetSessionIdCmdRef(SessionPtr);	
	AddStrWebPage("\" ");
	SetReturnNavEnButton();
    AddStrWebPage("</div>\r\n");
    
    AddStrWebPage("</td></tr></table>\r\n");            
    AddStrWebPage("</form></fieldset><br>\r\n");
	AddStrWebPage("</td></tr></table>\r\n");
    
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
void ChgUserTypeShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	int          i, pars_read;
	unsigned int SecKeyForm, UserId, ChgUserId, ChgUserType;
	ObjListTask  *SelObjPtr = NULL;
	USER_INFO    *UserPtr = NULL;

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;
        FText = ParseParForm( &FText[i] );
        if (!FText) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyUserId);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &UserId);
		if (!pars_read) break;
		if (UserId != SessionPtr->UserPtr->UserId) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyChgUserTd);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &ChgUserId);
		if (!pars_read) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyChgUserType);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &ChgUserType);
		if (!pars_read || (ChgUserType < MIN_UAT_ID) || (ChgUserType > MAX_UAT_ID)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

	    SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	    while(SelObjPtr)
		{
	        UserPtr = (USER_INFO*)SelObjPtr->UsedTask;
		    if (UserPtr->UserId == ChgUserId)
			{
				UserPtr->UserType = ChgUserType;
                if (isPrimaryGroupAccessUser(UserPtr))                
                    UserPtr->GroupMask = GetAllGroupAccessMask();              
				UserInfoDBSave(&SampleUserDbIfo);
				break;
			}
			SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
		}
		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
		RegClientsListPageGen(BufAnsw, SessionPtr, HttpCmd);
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void RegClientsListPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	ObjListTask  *SelObjPtr = NULL;
	char         CmdGenBuf[512];
	USER_INFO    *UserPtr = NULL;
	int          index = 1;
	unsigned int UsrType;

    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	// Form header
    AddStrWebPage("<h3><center>\r\n");
    SetRusTextBuf(NULL, SITE_RUS_SHOP_DB_REG_CLIENTS_LINE_ID);
    AddStrWebPage("</center></h3><br>\r\n");
	if (!UserInfoList.Count)
	{
		SetRusTextBuf(NULL, SITE_RUS_USERS_LIST_EMPTY_LINE_ID);
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
		return;
	}
    AddStrWebPage("<div class=\"reg_user_info_text_area\">\r\n");
    AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr align=\"left\" class=\"sectiontableheader\">\r\n");
	AddStrWebPage("<th>Nn</th>\r\n");
    AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_USER_TYPE_LINE_ID);
	AddStrWebPage("</th>\r\n");
    AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_USERS_LIST_CLIENT_LINE_ID);
	AddStrWebPage("</th>\r\n");
    AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_USERS_REGISTER_LINE_ID);
	AddStrWebPage("</th>\r\n");
    AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_USERS_LAST_VISIT_LINE_ID);
	AddStrWebPage("</th>\r\n");
    
    if (ParWebServPtr->ServCustomCfg.ReqUserCityField)
    {
        AddStrWebPage("<th>");
	    SetRusTextBuf(NULL, SITE_RUS_ORDER_CITY_LINE_ID);
	    AddStrWebPage("</th>\r\n");
    }
    
    AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_ORDER_MOB_PHONE_LINE_ID);
	AddStrWebPage("</th>\r\n");
    AddStrWebPage("<th>E-mail</th>\r\n");
	AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_SERV_DB_OPER_LINE_ID);
	AddStrWebPage("</th></tr>\r\n");


	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	while(SelObjPtr)
	{
	    UserPtr = (USER_INFO*)SelObjPtr->UsedTask;
		if (UserPtr->UserId != 1)
		{
            AddStrWebPage("<tr valign=\"top\" class=\"sectiontableentry1\">\r\n");

	        // Show number of order in backet
		    sprintf(CmdGenBuf, "<td>%d</td>\r\n", index);
		    AddStrWebPage(CmdGenBuf);
			
			/* Show client access role */
			AddStrWebPage("<td>\r\n");
            AddStrWebPage("<div><form action=\"");
            SetServerHttpAddr(NULL);
	        AddStrWebPage(GenPageChgUserType);
	        AddStrWebPage("\" method=\"get\" ");
			sprintf(CmdGenBuf, "name=\"user_type_chg_%d\">\r\n", index);
			AddStrWebPage(CmdGenBuf);
            AddStrWebPage("<select class=\"inputbox\" ");
			sprintf(CmdGenBuf, "name=\"user_type_chg_%d\" ", index);
			AddStrWebPage(CmdGenBuf);
			AddStrWebPage("style=\"width:100px;\" onchange=\"document.location.href=\'");
	        SetServerHttpAddr(NULL);
	        AddStrWebPage(GenPageChgUserType);
	        AddStrWebPage( ";chg_usr_type=' + this.options[selectedIndex].value");
	        sprintf(CmdGenBuf, " + '&amp;%s=%d'", SecKeyId, SessionPtr->SecureKey);
	        AddStrWebPage(CmdGenBuf);
	        sprintf(CmdGenBuf, " + '&amp;%s=%s'", KeyFormSessionId, SessionPtr->SesionIdKey);
	        AddStrWebPage(CmdGenBuf);
	        sprintf(CmdGenBuf, " + '&amp;%s=%d'", KeyFormUserId, SessionPtr->UserPtr->UserId);
	        AddStrWebPage(CmdGenBuf);
	        sprintf(CmdGenBuf, " + '&amp;%s%d'", KeyChgUserTd, UserPtr->UserId);
	        AddStrWebPage(CmdGenBuf);
	        AddStrWebPage(";\">\r\n");

            for (UsrType=MIN_UAT_ID;UsrType <= MAX_UAT_ID;UsrType++)
			{
                if (UserPtr->UserType == UsrType)
				{
			        sprintf(CmdGenBuf, "<option value=\"%d\" selected=\"selected\">", UsrType);
				}
		        else
				{
			        sprintf(CmdGenBuf, "<option value=\"%d\">", UsrType);
				}
		        AddStrWebPage(CmdGenBuf);
			    switch(UsrType)
				{
			        case UAT_ADMIN:
				        SetRusTextBuf(NULL, SITE_RUS_ROLE_SITE_ADMIN_LINE_ID);
					    break;

			        case UAT_SYSSHOW:
				        SetRusTextBuf(NULL, SITE_RUS_ROLE_STATUS_SHOW_LINE_ID);
					    break;

				    default:
					    SetRusTextBuf(NULL, SITE_RUS_AUTH_USER_LINE_ID);
					    break;
				}
		        AddStrWebPage("</option>\r\n");
			}
            AddStrWebPage("</select>\r\n");

	        SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	        SetHiddenIntParForm(NULL, KeyFormSessionId, SessionPtr->SessionId);
	        SetHiddenIntParForm(NULL, KeyFormUserId, SessionPtr->UserPtr->UserId);

	        AddStrWebPage("</form></div>\r\n");
            AddStrWebPage("</td>\r\n");

	        // Show client full name
            AddStrWebPage("<td><strong>\r\n");
		    if (UserPtr->Contact.UserTitleId > SITE_RUS_ORDER_REQ_TYPE_1_LINE_ID)
		    {
		        SetRusTextBuf(NULL, UserPreffixArray[UserPtr->Contact.UserTitleId]);
			    AddStrWebPage(" ");
		    }
		    sprintf(CmdGenBuf, "%s %s %s</strong></td>\r\n", 
			    UserPtr->Contact.FirstName, UserPtr->Contact.LastName,
			    UserPtr->Contact.MiddleName);
	        AddStrWebPage(CmdGenBuf);

            /* Show user's register date */        
	        sprintf(CmdGenBuf, "<td>%02d.%02d.%04d&nbsp;%02d:%02d:%02d</td>\r\n", 
    #ifdef _VCL60ENV_        
		        UserPtr->RegisterTime.wDay, UserPtr->RegisterTime.wMonth, UserPtr->RegisterTime.wYear,            
                UserPtr->RegisterTime.wHour, UserPtr->RegisterTime.wMinute, UserPtr->RegisterTime.wSecond);
    #endif
    #ifdef _LINUX_X86_             
	            UserPtr->RegisterTime.tm_mday, UserPtr->RegisterTime.tm_mon+1, UserPtr->RegisterTime.tm_year+1900,
                UserPtr->RegisterTime.tm_hour, UserPtr->RegisterTime.tm_min, UserPtr->RegisterTime.tm_sec);
    #endif
	        AddStrWebPage(CmdGenBuf);

            /* Show user's last visit date */        
	        sprintf(CmdGenBuf, "<td>%02d.%02d.%04d&nbsp;%02d:%02d:%02d</td>\r\n", 
    #ifdef _VCL60ENV_        
		        UserPtr->LastVisitTime.wDay, UserPtr->LastVisitTime.wMonth, UserPtr->LastVisitTime.wYear,            
                UserPtr->LastVisitTime.wHour, UserPtr->LastVisitTime.wMinute, UserPtr->LastVisitTime.wSecond);
    #endif
    #ifdef _LINUX_X86_             
	            UserPtr->LastVisitTime.tm_mday, UserPtr->LastVisitTime.tm_mon+1, UserPtr->LastVisitTime.tm_year+1900,
                UserPtr->LastVisitTime.tm_hour, UserPtr->LastVisitTime.tm_min, UserPtr->LastVisitTime.tm_sec);
    #endif
	        AddStrWebPage(CmdGenBuf);
            
            if (ParWebServPtr->ServCustomCfg.ReqUserCityField)
            {
		        // Show client leave city
		        sprintf(CmdGenBuf, "<td><strong>%s</strong></td>\r\n", (const char*)&UserPtr->Contact.City);
		        AddStrWebPage(CmdGenBuf);
            }

		    // Show client mobile phone
		    sprintf(CmdGenBuf,"<td>%s</td>\r\n", (const char*)&UserPtr->Contact.MobilePhone);
		    AddStrWebPage(CmdGenBuf);

		    // Show client E-Mail
		    sprintf(CmdGenBuf, "<td>%s</td>\r\n", (const char*)&UserPtr->Contact.Email);
		    AddStrWebPage(CmdGenBuf);

			AddStrWebPage("<td align=\"center\" valign=\"top\">\r\n");
			if (UserPtr->UserType == UAT_ADMIN)
			{
                AddStrWebPage("N//A");
			}
			else
			{
                /* Show button for selecterd user remove request */
                AddStrWebPage("<table cellspacing=\"4\" cellpadding=\"0\" border=\"0\">\r\n");
                
                AddStrWebPage("<tr><td><a href=\"");
	            SetServerHttpAddr(NULL);
	            AddStrWebPage(GenPageDelUser);
		        sprintf(CmdGenBuf, "?%s%d&%s%s&%s=%d&%s%d", 
			        KeyUserId, SessionPtr->UserPtr->UserId, 
			        KeySessionId, SessionPtr->SesionIdKey,
				    SecKeyId, SessionPtr->SecureKey, 
				    KeyDelUserId, UserPtr->UserId);
			    AddStrWebPage(CmdGenBuf);
                AddStrWebPage("\" title=\"");
			    SetRusTextBuf(NULL, SITE_RUS_USER_DEL_REQ_LINE_ID);
			    AddStrWebPage("\">\r\n");
                AddStrWebPage("<div style=\"background-image: url(");
	            SetServerHttpAddr(NULL);
                AddStrWebPage(GetImageNameByKey(BtRemItemEnKey));
                AddStrWebPage("); width: 21px; height: 17px; background-size: contain; background-repeat: no-repeat; opacity: 1.0;\"\r\n");
                AddStrWebPage("onmouseover = \"this.style.backgroundImage = 'url(");
	            SetServerHttpAddr(NULL);
                AddStrWebPage(GetImageNameByKey(BtRemItemMoKey));
                AddStrWebPage(")'\"\r\n");
                AddStrWebPage("onmouseout = \"this.style.backgroundImage = 'url(");
	            SetServerHttpAddr(NULL);
                AddStrWebPage(GetImageNameByKey(BtRemItemEnKey));
                AddStrWebPage(")'\">\r\n");
                AddStrWebPage("<div style=\"background-image: url(");
	            SetServerHttpAddr(NULL);
                AddStrWebPage(GetImageNameByKey(EmptyBlkKey));
                AddStrWebPage("); width: 21px; height: 17px; background-size: contain; background-repeat: no-repeat; opacity: 1.0;\"></div>\r\n");
                AddStrWebPage("</div></a></td>\r\n");
                
                AddStrWebPage("<td><a href=\"");
	            SetServerHttpAddr(NULL);
	            AddStrWebPage(GenPageChgGrpUser);
		        sprintf(CmdGenBuf, "?%s%d&%s%s&%s=%d&%s%d", 
			        KeyUserId, SessionPtr->UserPtr->UserId, 
			        KeySessionId, SessionPtr->SesionIdKey,
				    SecKeyId, SessionPtr->SecureKey, 
				    KeyChgUserTd, UserPtr->UserId);
			    AddStrWebPage(CmdGenBuf);
                AddStrWebPage("\" title=\"");
			    SetRusTextBuf(NULL, SITE_RUS_USER_GRP_SET_REQ_LINE_ID);
			    AddStrWebPage("\">\r\n");
                AddStrWebPage("<div style=\"background-image: url(");
	            SetServerHttpAddr(NULL);
                AddStrWebPage(GetImageNameByKey(BtUserGrpEnKey));
                AddStrWebPage("); width: 17px; height: 17px; background-size: contain; background-repeat: no-repeat; opacity: 1.0;\"\r\n");
                AddStrWebPage("onmouseover = \"this.style.backgroundImage = 'url(");
	            SetServerHttpAddr(NULL);
                AddStrWebPage(GetImageNameByKey(BtUserGrpMoKey));
                AddStrWebPage(")'\"\r\n");
                AddStrWebPage("onmouseout = \"this.style.backgroundImage = 'url(");
	            SetServerHttpAddr(NULL);
                AddStrWebPage(GetImageNameByKey(BtUserGrpEnKey));
                AddStrWebPage(")'\">\r\n");
                AddStrWebPage("<div style=\"background-image: url(");
	            SetServerHttpAddr(NULL);
                AddStrWebPage(GetImageNameByKey(EmptyBlkKey));
                AddStrWebPage("); width: 17px; height: 17px; background-size: contain; background-repeat: no-repeat; opacity: 1.0;\"></div>\r\n");
                AddStrWebPage("</div></a></td>\r\n");                
                
                AddStrWebPage("</tr></table>\r\n");
			}
            AddStrWebPage("</td></tr>\r\n");            
		    index++;
		}
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
	}
    AddStrWebPage("</table>\r\n</div>\r\n");
    
	/* Button for return to main page */
    AddStrWebPage("<br><hr><br>\r\n");
    AddStrWebPage("<div><a href=\"");
	SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageMain);
	SetSessionIdCmdRef(SessionPtr);	
	AddStrWebPage("\" ");
	SetReturnNavEnButton();
    AddStrWebPage("</div>\r\n");
    
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
