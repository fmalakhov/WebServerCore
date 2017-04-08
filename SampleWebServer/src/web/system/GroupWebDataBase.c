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
#include "FileDataHash.h"
#include "Interface.h"
#include "HtmlTemplateParser.h"

extern char ServerHttpAddr[];
extern char KeySessionId[];
extern char KeyUserNameId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char KeyUserId[];
extern char ThrWebServName[];
extern char KeyItemsPageId[];
extern char KeyStartItemId[];

extern READWEBSOCK  *ParReadHttpSocketPtr;
extern PARAMWEBSERV *ParWebServPtr;
extern ListItsTask  GroupList;
extern ListItsTask  HostList;
extern char *EndHtmlPageGenPtr;
extern unsigned int ItemsPerPageTable[];
extern int NumPageShowLevel;
extern USER_DB_INFO SampleUserDbIfo;

extern char KeySectionId[];
extern char KeyItemId[];
extern char KeyFormUserId[];
extern char FormKeySessionId[];
extern char FormKeyUserId[];

char KeyFormGroupId[]          = "group_id";
char KeyFormGroupName[]        = "group_name";
char KeyFormGroupDbOper[]      = "db_operation";

void GroupBasePageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void SetGroupTextFieldLen();
void SetGroupSelectFieldLen();
//---------------------------------------------------------------------------
void GroupDataBase(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
	unsigned int TextLen;
    char*        FText = NULL;
	char*        FStrt = NULL;
	USER_INFO*   NewUserPtr = NULL;
    unsigned long long int GroupChgMask;
	unsigned int i, pars_read, SecKeyForm, UserId, 
		         ParValue, GroupId;
	ObjListTask    *SelObjPtr = NULL;    
    GROUP_INFO_TYPE *ChgGroupPtr = NULL;
	char           *TextConvertBufPtr = NULL;
	GROUP_INFO_TYPE EditGroup;
    unsigned int   DbOperId = 0;
	unsigned int   RandIndex, DateIndex;
	unsigned char  *PictureDirPtr = NULL;
	char           *PrevPicPtr = NULL;
	unsigned int   SecKeyLen = 0;
	char           *ParNamePtr = NULL;
	char           *BodyBufPtr = NULL;
	SAMPLE_SPECIFIC_USER_SESSION *SampleSessionPtr = NULL;
#ifdef _LINUX_X86_
	ThrMsgChanInfo ReqThrChan;
#endif
	char           ParName[64];
	char           SectionKey[128];

	SampleSessionPtr = (SAMPLE_SPECIFIC_USER_SESSION*)SessionPtr->UserSessionInfoPtr;
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
	
		i = FindCmdRequest(FText, "db_operation=");
		if (i != -1)
		{
		    pars_read = sscanf(&HttpCmd[i], "%d", &DbOperId);
		    if (!pars_read) break;
			if ((DbOperId < DB_GROUP_MIN_REQ) || (DbOperId > DB_GROUP_MAX_REQ)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);

            /* Check for WEB server in demo mode. If yes, than do nothing */
            if (ParWebServPtr->ServCustomCfg.DemoMode)
            {
                switch(DbOperId)
		        {
				    case DB_GROUP_SEL_EDIT_GROUP_REQ:
		                i = FindCmdRequest(FText, KeyFormGroupId);
					    if (i == -1) break;
					    FText = ParseParForm( &FText[i] );
                        if (!FText) break;
					    TextLen = strlen(FText);
					    if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &GroupId);
		                if (!pars_read) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);
					    SampleSessionPtr->SelGroupId = GroupId;
					    isParseDone = true;
					    break;

                    default:
                        SampleSessionPtr->SelGroupId = 0;
                        isParseDone = true;
                        break;
                }
                break;
            }

            switch(DbOperId)
		    {
				case DB_GROUP_SEL_EDIT_GROUP_REQ:
		            i = FindCmdRequest(FText, KeyFormGroupId);
					if (i == -1) break;
					FText = ParseParForm( &FText[i] );
                    if (!FText) break;
					TextLen = strlen(FText);
					if (!TextLen) break;
		            pars_read = sscanf(FText, "%d", &GroupId);
		            if (!pars_read) break;
		            FText = FStrt;
		            strcpy(FText,HttpCmd);
					SampleSessionPtr->SelGroupId = GroupId;
					isParseDone = true;
					break;

				case DB_GROUP_SAVE_GROUP_REQ:
					ChgGroupPtr = GetGroupByGroupId(SampleSessionPtr->SelGroupId);
					if (!ChgGroupPtr) break;
					memcpy(&EditGroup, ChgGroupPtr, sizeof(GROUP_INFO_TYPE));

				    if (!StringParParse(FText, (char*)&EditGroup.GroupName[0],
		                KeyFormGroupName, MAX_LEN_GROUP_BASE_NAME)) break;
		            FText = FStrt;
		            strcpy(FText,HttpCmd);

					memcpy(ChgGroupPtr, &EditGroup, sizeof(GROUP_INFO_TYPE));
					GroupDbSave();
					SampleSessionPtr->SelGroupId = 0;
					isParseDone = true;
					break;

				case DB_GROUP_BD_RELOAD_REQ:
					GroupDBClear();
					GroupDBLoad();
					SampleSessionPtr->SelGroupId = 0;
					isParseDone = true;
					break;

				case DB_GROUP_ADD_GROUP_REQ:
					ChgGroupPtr = GroupDbAddItem();
					if (ChgGroupPtr)
					{
						GroupDbSave();
                        AddPrimaryUserGroupMask(&SampleUserDbIfo, (0x01 << (unsigned long long int)ChgGroupPtr->GroupId));
						SampleSessionPtr->SelGroupId = ChgGroupPtr->GroupId;
					    isParseDone = true;
					}
					break;
                
                case DB_GROUP_REM_GROUP_REQ:
					ChgGroupPtr = GetGroupByGroupId(SampleSessionPtr->SelGroupId);
					if (!ChgGroupPtr) break;
                    GroupChgMask = (0x01 << (unsigned long long int)ChgGroupPtr->GroupId);
                    RemUserGroupMask(&SampleUserDbIfo, GroupChgMask);                                       
                    GroupDbRemItem(ChgGroupPtr);                                               
                    SampleSessionPtr->SelGroupId = 0;
                    isParseDone = true;
                    break;
                                        
				default:
					break;
		    }
		}
		else
		{
			SampleSessionPtr->SelGroupId = 0;
		    isParseDone = true;
		}
		break;
	}
    if (isParseDone)
	{
		GroupBasePageGen(BufAnsw, SessionPtr, HttpCmd);
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
void GroupBasePageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
    unsigned int    i, CurrRecord = 0;
    unsigned int    GroupRecPage;
	ObjListTask     *SelObjPtr = NULL;
    ObjListTask     *SelTypeObjPtr = NULL;
	GROUP_INFO_TYPE  *GroupPtr = NULL;
	GROUP_INFO_TYPE  *EditGroupPtr = NULL;  
	SAMPLE_SPECIFIC_USER_SESSION *SampleSessionPtr = NULL;
	int             index = 1;
	int             EndRecordPage;    
	char            StrBuf[256];
    
	SampleSessionPtr = (SAMPLE_SPECIFIC_USER_SESSION*)SessionPtr->UserSessionInfoPtr;
    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	// Script for item for edit selection support
	SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
        SessionPtr, "SelButtonScript.html");

    AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">//<![CDATA[\r\n");
    AddStrWebPage("function on_submit_group_record() {\r\n");
    AddStrWebPage("var form = document.GroupEditForm;\r\n");

	/* Check stats name filed */
	AddStrWebPage("var isvalid = true;\r\n");
    AddStrWebPage("if (form.group_name.value == \"\") {\r\n");
	AddStrWebPage("document.getElementById('group_name_div').style.color = \"red\";\r\n");
	AddStrWebPage("isvalid = false;}\r\n");
	AddStrWebPage("else {document.getElementById('group_name_div').style.color = \"white\";}\r\n");

	AddStrWebPage("if(!isvalid) {\r\n");
    AddStrWebPage("alert( \"");
    SetOriginalRusTextBuf(NULL, SITE_RUS_ORDER_5_LINE_ID);
    AddStrWebPage("\");\r\n");
	AddStrWebPage("return false;\r\n");
    AddStrWebPage("} else {\r\n");
	AddStrWebPage("return true;}\r\n");
    AddStrWebPage("}\r\n");
    AddStrWebPage("</script>\r\n");

	// Form header
    AddStrWebPage("<h3><center>\r\n");
    SetRusTextBuf(NULL, SITE_RUS_GROUP_BASE_LINE_ID);
    AddStrWebPage("</center></h3><br >\r\n");

    AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr align=\"center\">\r\n");

    AddStrWebPage("<td></td>\r\n");

	// Database reload request button
    AddStrWebPage("<td><form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageGroupDBManage);
	AddStrWebPage("\" method=\"post\" name=\"BaseGroupReloadForm\">\r\n");

	SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);
	SetHiddenIntParForm(NULL, KeyFormGroupDbOper, DB_GROUP_BD_RELOAD_REQ);

	AddStrWebPage("<input type=\"submit\" value=\"");
    SetRusTextBuf(NULL, SITE_RUS_SHOP_DB_RELOAD_LINE_ID);
    AddStrWebPage("\" class=\"button\" onclick=\"\" >\r\n");
	AddStrWebPage("</form></td>\r\n");

	/* New stats record create request button */
    AddStrWebPage("<td><form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageGroupDBManage);
	AddStrWebPage("\" method=\"post\" name=\"BaseGroupCreateForm\">\r\n");

	SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);
	SetHiddenIntParForm(NULL, KeyFormGroupDbOper, DB_GROUP_ADD_GROUP_REQ);

	AddStrWebPage("<input type=\"submit\" value=\"");
    SetRusTextBuf(NULL, SITE_RUS_GROUP_DB_NEW_GROUP_LINE_ID);
    AddStrWebPage("\" class=\"button\" onclick=\"\" >\r\n");
	AddStrWebPage("</form></td>\r\n");
	AddStrWebPage("</tr></table>\r\n");

	if (!GroupList.Count)
	{
		AddStrWebPage("<center><h2>\r\n");
		SetRusTextBuf(NULL, SITE_RUS_GROUP_DB_EMPTY_SEC_LINE_ID);
		AddStrWebPage("</h2></center>\r\n");
        
	    AddStrWebPage("<br style=\"clear:both;\"></div><hr><br>\r\n");

	    /* Button for return to main page */
        AddStrWebPage("<div><a href=\"");
	    SetServerHttpAddr(NULL);
	    AddStrWebPage(GenPageMain);
	    SetSessionIdCmdRef(SessionPtr);	
	    AddStrWebPage("\" ");
	    SetReturnNavEnButton();
        AddStrWebPage("</div>\r\n");    
        
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
		return;
	}
    
    AddStrWebPage("<br style=\"clear:both;\"></div><hr><br>\r\n");

    AddStrWebPage("<div style=\"text-align:center;\">");
	SetNavPageItemListShopWebPage(BufAnsw, SessionPtr, 
		SampleSessionPtr->GroupPerPage, SampleSessionPtr->StartGroupRec, 
		(unsigned int)GroupList.Count, 0, GenPageCghGroupDbManage);	
	AddStrWebPage("</div>\r\n<br>\r\n");

    AddStrWebPage("<div class=\"stats_base_info_text_area\">\r\n");
    AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr align=\"left\" class=\"sectiontableheader\">\r\n");
	AddStrWebPage("<th width=\"32\">Nn</th>\r\n");
    AddStrWebPage("<th width=\"100\">");
	SetRusTextBuf(NULL, SITE_RUS_GROUP_DFN_INDEX_LINE_ID);
	AddStrWebPage("</th>\r\n");                
    AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_GROUP_DB_GROUP_NAME_LINE_ID);
	AddStrWebPage("</th>\r\n");
    AddStrWebPage("<th width=\"64\">");
	SetRusTextBuf(NULL, SITE_RUS_SERV_DB_OPER_LINE_ID);
	AddStrWebPage("</th></tr>\r\n");

	index = SampleSessionPtr->StartGroupRec+1;
    GroupRecPage = ItemsPerPageTable[SampleSessionPtr->GroupPerPage];
	SelObjPtr = (ObjListTask*)GetFistObjectList(&GroupList);
	while(SelObjPtr && GroupRecPage)
	{
        if (CurrRecord < SampleSessionPtr->StartGroupRec)
        {
            CurrRecord++;
            SelObjPtr = (ObjListTask*)GetNextObjectList(&GroupList);
            continue;
        }
	    GroupPtr = (GROUP_INFO_TYPE*)SelObjPtr->UsedTask;

        AddStrWebPage("<tr valign=\"top\" class=\"sectiontableentry1\">\r\n");

	    /* Show record num */
		sprintf(StrBuf, "<td>%d</td>\r\n", index);
		AddStrWebPage(StrBuf);

	    /* Show group id */
		sprintf(StrBuf, "<td>%d</td>\r\n", GroupPtr->GroupId);
		AddStrWebPage(StrBuf);
        
	    /* Show name of group */
		sprintf(StrBuf, "<td>%s</td>\r\n", &GroupPtr->GroupName[0]);
		AddStrWebPage(StrBuf);

	    /* Show button for record edit */
		AddStrWebPage("<td align=\"center\" valign=\"top\"><a href=\"");
		AddStrWebPage(GenPageGroupDBManage);
	    sprintf(StrBuf, "?%s=%s;", FormKeySessionId, SessionPtr->SesionIdKey);
	    AddStrWebPage(StrBuf);
	    sprintf(StrBuf, "%s=%d;", SecKeyId, SessionPtr->SecureKey);
	    AddStrWebPage(StrBuf);
		sprintf(StrBuf, "%s=%d;", KeyFormGroupDbOper, DB_GROUP_SEL_EDIT_GROUP_REQ);
		AddStrWebPage(StrBuf);        
	    sprintf(StrBuf, "%s=%d;", FormKeyUserId, SessionPtr->UserPtr->UserId);
	    AddStrWebPage(StrBuf);
		sprintf(StrBuf, "%s=%d", KeyFormGroupId, GroupPtr->GroupId);
	    AddStrWebPage(StrBuf);
		sprintf(StrBuf, "\" onMouseOver=\"hiLite('BtSelItemEdit%d','BtSelItemEditS','1');return true\" ", index);
		AddStrWebPage(StrBuf);
		if (GroupPtr->GroupId == SampleSessionPtr->SelGroupId)
		{
			sprintf(StrBuf, "\" onMouseOut=\"hiLite('BtSelItemEdit%d','BtSelItemEditA','0');return true\" ", index);
			EditGroupPtr = GroupPtr;
		}
		else
		{
		    sprintf(StrBuf, "\" onMouseOut=\"hiLite('BtSelItemEdit%d','BtSelItemEditP','0');return true\" ", index);
		}
		AddStrWebPage(StrBuf);
		sprintf(StrBuf, "\" onClick=\"hiLite('BtSelItemEdit%d','BtSelItemEditD','1');return true\">\r\n", index);
		AddStrWebPage(StrBuf);
		sprintf(StrBuf, "<img name = \"BtSelItemEdit%d\" ", index);
		AddStrWebPage(StrBuf);
		if (GroupPtr->GroupId == SampleSessionPtr->SelGroupId)
		{
	        AddStrWebPage("src=\"images/BtSelItemEditAct.png\"");
		}
		else
		{
		    AddStrWebPage("src=\"images/BtSelItemEditPas.png\"");
		}
		AddStrWebPage(" border=\"0\" width=\"20\" height=\"20\" ALT=\"Select group for edit.\"></a></td>\r\n");
        AddStrWebPage("</tr>\r\n");
		index++;
        GroupRecPage--;
		SelObjPtr = (ObjListTask*)GetNextObjectList(&GroupList);
	}
    AddStrWebPage("</table></div>\r\n");

	AddStrWebPage("<br style=\"clear:both;\"></div>\r\n");
	SetNavPageItemListShopWebPage(BufAnsw, SessionPtr, 
		SampleSessionPtr->GroupPerPage, SampleSessionPtr->StartGroupRec, 
		(unsigned int)GroupList.Count, 0, GenPageCghGroupDbManage);	

    AddStrWebPage("<br><div align=\"center\">\r\n");
	AddStrWebPage("<form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage("\" method=\"post\">");
	SetRusTextBuf(NULL, SITE_RUS_SHOW_ITEMS_LINE_ID);
    AddStrWebPage("#&nbsp;&nbsp;\r\n");
	AddNumItemsPerFilterPageSelectToHtml(NULL,
		SessionPtr->SessionId, SampleSessionPtr->GroupPerPage, 0, GenPageCghGroupDbManage);

    SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);

	AddStrWebPage("<noscript><input class=\"button\" type=\"submit\" value=\"");
	SetRusTextBuf(NULL, SITE_RUS_SENT_REQ_LINE_ID);
	AddStrWebPage("\" ></noscript></form></div>\r\n");

	AddStrWebPage("<div align=\"center\">\r\n");        
	SetRusTextBuf(NULL, SITE_RUS_RESULTS_LINE_ID);
	if (SampleSessionPtr->StartGroupRec + ItemsPerPageTable[SampleSessionPtr->GroupPerPage] < 
		(unsigned int)GroupList.Count)
	{
        EndRecordPage = SampleSessionPtr->StartGroupRec + ItemsPerPageTable[SampleSessionPtr->GroupPerPage];
	}
	else
	{
		EndRecordPage = (unsigned int)GroupList.Count;
	}
	sprintf(StrBuf," %d - %d (%d)</div>\r\n", (SampleSessionPtr->StartGroupRec+1), 
		EndRecordPage, (unsigned int)GroupList.Count);
    AddStrWebPage(StrBuf);
    AddStrWebPage("<br>\r\n");

	if ((SampleSessionPtr->SelGroupId > 0) && EditGroupPtr)
	{
	    AddStrWebPage("<fieldset>\r\n<legend><span class=\"sectiontableheader\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_GROUP_DB_GROUP_EDIT_LINE_ID);
        AddStrWebPage("</span></legend>\r\n");

        AddStrWebPage("<form action=\"");
		AddStrWebPage(GenPageGroupDBManage);
		AddStrWebPage("\" name=\"GroupEditForm\" method=\"post\">\r\n");

		AddStrWebPage("<table width=\"95%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
        AddStrWebPage("<tr><td>\r\n");
        
        AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");

		/* Group name field edit */
		AddStrWebPage("<tr align=\"left\">\r\n"); 
		AddStrWebPage("<td><div class=\"group_base_field_descr\" id=\"group_name_div\" >\r\n");
        AddStrWebPage("<label for=\"gropu_name_info\">");
        SetRusTextBuf(NULL, SITE_RUS_GROUP_DB_GROUP_NAME_LINE_ID);
        AddStrWebPage(":</label></div>\r\n");
        AddStrWebPage("<input type=\"text\" id=\"group_name_info\" name=\"");
	    AddStrWebPage(KeyFormGroupName);
        AddStrWebPage("\" class=\"inputbox\" ");
        SetGroupTextFieldLen();
		sprintf(StrBuf, "value=\"%s\" maxlength=\"%d\">\r\n", 
            EditGroupPtr->GroupName, MAX_LEN_GROUP_BASE_NAME);
	    AddStrWebPage(StrBuf);	
		AddStrWebPage("</td></tr>\r\n");

		AddStrWebPage("<tr align=\"center\"><td>\r\n");

		AddStrWebPage("<table cellspacing=\"4\" cellpadding=\"2\" border=\"0\">\r\n");
        AddStrWebPage("<tr><td>\r\n");

	    SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	    SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	    SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);
	    SetHiddenIntParForm(NULL, KeyFormGroupDbOper, DB_GROUP_SAVE_GROUP_REQ);

		// Key for group record save
        AddStrWebPage("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"");
        SetRusTextBuf(NULL, SITE_RUS_SHOP_DB_ITEM_SAVE_LINE_ID);
		AddStrWebPage("\" onclick=\"return(on_submit_group_record());\" >\r\n");
		AddStrWebPage("</form>\r\n");

        AddStrWebPage("</td><td>\r\n");

	    /* Selected stats record delete request button */
        AddStrWebPage("<form action=\"");
        SetServerHttpAddr(NULL);
	    AddStrWebPage(GenPageGroupDBManage);
	    AddStrWebPage("\" method=\"post\" name=\"BaseGroupDeleteForm\">\r\n");

	    SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	    SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	    SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);
	    SetHiddenIntParForm(NULL, KeyFormGroupDbOper, DB_GROUP_REM_GROUP_REQ);

	    AddStrWebPage("<input type=\"submit\" value=\"");
        SetRusTextBuf(NULL, SITE_RUS_GROUP_DB_GROUP_DELETE_LINE_ID);
        AddStrWebPage("\" class=\"button\" onclick=\"\" >\r\n");
	    AddStrWebPage("</form>\r\n");

        AddStrWebPage("</td></tr></table>\r\n");
		AddStrWebPage("</td></tr></table>\r\n");
		AddStrWebPage("</td></tr>\r\n");
        AddStrWebPage("</table>\r\n");      
              
		AddStrWebPage("</fieldset><br>\r\n");
	}
    else
    {
	    AddStrWebPage("<br style=\"clear:both;\"><hr><br>\r\n");
    }
    
	/* Button for return to main page */
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
void ChgViewGroupDataBase(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool isItemDispAttr = false;
	int i;
	unsigned int GroupPageId;
	unsigned int StartGroupId;
	SAMPLE_SPECIFIC_USER_SESSION *SampleSessionPtr = NULL;

	SampleSessionPtr = (SAMPLE_SPECIFIC_USER_SESSION*)SessionPtr->UserSessionInfoPtr;     
	for (;;)
	{        
        i = FindCmdRequest(HttpCmd, KeyItemsPageId);
        if (i != -1)
		{
	        if (!sscanf(&HttpCmd[i], "%d", &GroupPageId)) break;
			if (GroupPageId > NumPageShowLevel) break;
			SampleSessionPtr->GroupPerPage = GroupPageId;
            SampleSessionPtr->StartGroupRec = 0;
		}
                
        i = FindCmdRequest(HttpCmd, KeyStartItemId);
		if (i != -1)
		{
	        if (!sscanf(&HttpCmd[i], "%d", &StartGroupId)) break;
			if (StartGroupId > (unsigned int)GroupList.Count)
                 SampleSessionPtr->StartGroupRec = 0;
			else SampleSessionPtr->StartGroupRec = StartGroupId;
		}
		if (SampleSessionPtr->StartGroupRec >= (unsigned int)GroupList.Count)
		{
            SampleSessionPtr->StartGroupRec = 0;
		}
		isItemDispAttr = true;
		break;
	}

	if (isItemDispAttr)
	{
        GroupBasePageGen(BufAnsw, SessionPtr, HttpCmd);
	}
	else
	{
		AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_SECTION_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
}
//---------------------------------------------------------------------------
void SetGroupTextFieldLen()
{
    AddStrWebPage("style=\"width:");
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
    {
        if (ParReadHttpSocketPtr->MobileType)
        {
            switch(ParReadHttpSocketPtr->MobileType->AspectWidth)
            {                    
                case 360:
	                AddStrWebPage("164px");          
                    break;

                case 400:
                 	AddStrWebPage("204px");          
                    break;

                default:
	                AddStrWebPage("124px");
                    break;
            }
        }
        else
        {
            AddStrWebPage("124px");
        }
    }
    else
    {
        AddStrWebPage("200px");
    }
    AddStrWebPage(";\" ");
}
//---------------------------------------------------------------------------
void SetGroupSelectFieldLen()
{
    AddStrWebPage("style=\"width:");
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
    {
        if (ParReadHttpSocketPtr->MobileType)
        {
            switch(ParReadHttpSocketPtr->MobileType->AspectWidth)
            {                    
                case 360:
	                AddStrWebPage("168px");          
                    break;

                case 400:
                 	AddStrWebPage("208px");          
                    break;

                default:
	                AddStrWebPage("128px");
                    break;
            }
        }
        else
        {
            AddStrWebPage("128px");
        }
    }
    else
    {
        AddStrWebPage("204px");
    }
    AddStrWebPage(";\" ");
}
//---------------------------------------------------------------------------
