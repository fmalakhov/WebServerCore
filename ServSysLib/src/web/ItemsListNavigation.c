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
#include "ImageNameHash.h"
#include "TextListDataBase.h"
#include "BaseHtmlConstData.h"

extern char KeyUserNameId[];
extern char SecKeyId[];
extern char KeySectionId[];
extern char KeySessionId[];
extern char KeyItemId[];
extern char KeyListSortId[];
extern char KeyReversModeId[];
extern char KeyItemsPageId[];
extern char KeyStartItemId[];
extern char KeyFormBrandId[];

extern unsigned int ItemsPerPageTable[];
extern char *EndHtmlPageGenPtr;
extern unsigned int NumPageShowLevel;
extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK *ParReadHttpSocketPtr;

extern unsigned int TypeSortArray[];
extern unsigned int ItemsPerPageTable[];

unsigned int ItemsPerPageTable[] = {10, 25, 50, 75, 100, 150, 200, 500, 1000, 2000};
unsigned int NumPageShowLevel = sizeof(ItemsPerPageTable)/sizeof(unsigned int);
//---------------------------------------------------------------------------
void SetNavPageItemListShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr,
	unsigned int ItemsPageId, unsigned int StartItemId, unsigned int ItemsInList,
	unsigned int SectionId, char *HtmlPageName)
{
	unsigned int NewStartItemId;
	unsigned int PageIndex = 0;
	unsigned int EndPageIndex = 0;
	unsigned int AllPages = 0;
	char BufStrLine[64];

	// First shop items page selection
	if (BufAnsw) EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	AddStrWebPage("<div align=\"center\"><table cellspacing=\"1\" cellpadding=\"1\" border=\"0\">\r\n");
    AddStrWebPage("<tr><td align=\"center\" valign=\"center\">\r\n");
	if (StartItemId < ItemsPerPageTable[ItemsPageId])
	{
		SetFirstPageNavDsButton();
	}
	else
	{
        AddStrWebPage("<a href=\"");
        SetServerHttpAddr(NULL);
		AddStrWebPage(HtmlPageName);
        SetSessionIdCmdRef(SessionPtr);
        if (SectionId > 0)
		{
	        AddStrWebPage(KeySectionId);
	        sprintf(BufStrLine, "%d;", SectionId);
	        AddStrWebPage(BufStrLine);
		}
	    NewStartItemId = 0;
	    AddStrWebPage(KeyStartItemId);
	    sprintf(BufStrLine, "%d\" ", NewStartItemId);
	    AddStrWebPage(BufStrLine);
		SetFirstPageNavEnButton();
	}

    AddStrWebPage("</td><td align=\"center\" valign=\"center\">\r\n");

	// Previous shop items page selection
	if (StartItemId < ItemsPerPageTable[ItemsPageId])
	{
		SetPrevPageNavDsButton();
	}
	else
	{
        AddStrWebPage("<a href=\"");
        SetServerHttpAddr(NULL);
		AddStrWebPage(HtmlPageName);
        SetSessionIdCmdRef(SessionPtr);
        if (SectionId > 0)
		{
	        AddStrWebPage(KeySectionId);
	        sprintf(BufStrLine, "%d;", SectionId);
	        AddStrWebPage(BufStrLine);
		}
		NewStartItemId = StartItemId - ItemsPerPageTable[ItemsPageId];
	    AddStrWebPage( KeyStartItemId);
	    sprintf(BufStrLine, "%d\" ", NewStartItemId);
	    AddStrWebPage( BufStrLine);
		SetPrevPageNavEnButton();
	}

	AllPages = ItemsInList/ItemsPerPageTable[ItemsPageId];
	if ((AllPages*ItemsPerPageTable[ItemsPageId]) < ItemsInList) AllPages++;

	if ((ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP) &&
		(ParReadHttpSocketPtr->BotType == BOT_NONE))
	{
    AddStrWebPage("</td><td>\r\n");
	AddStrWebPage("<table cellspacing=\"0\" cellpadding=\"0\" border=\"1\"><tr>\r\n");
	if (AllPages < 10)
	{
	    for (PageIndex=0;PageIndex < AllPages;PageIndex++)
		{
			AddStrWebPage("<td width=\"25\" align=\"center\" valign=\"center\"");
		    if ((PageIndex*ItemsPerPageTable[ItemsPageId]) == StartItemId)
			{
				AddStrWebPage("bgcolor=\"#577475\" >\r\n");
			    sprintf(BufStrLine, "<span class=\"pagenav\">%d</span>\r\n", PageIndex+1);
			    AddStrWebPage( BufStrLine);
			}
		    else
			{
				AddStrWebPage("bgcolor=\"#314142\" >\r\n");
	            AddStrWebPage("<a href=\"");
                SetServerHttpAddr(NULL);
			    AddStrWebPage(HtmlPageName);
                SetSessionIdCmdRef(SessionPtr);
                if (SectionId > 0)
				{
	                AddStrWebPage(KeySectionId);
	                sprintf(BufStrLine, "%d;", SectionId);
	                AddStrWebPage(BufStrLine);
				}
		        NewStartItemId = PageIndex*ItemsPerPageTable[ItemsPageId];
	            AddStrWebPage(KeyStartItemId);
	            sprintf(BufStrLine, "%d", NewStartItemId);
	            AddStrWebPage(BufStrLine);
			    sprintf(BufStrLine, "\" class=\"pagenav\"><strong>%d</strong></a>\r\n", PageIndex+1);
			    AddStrWebPage(BufStrLine);
			}
			strcat(BufAnsw,"</td>\r\n");
		}
	}
	else
	{
        PageIndex = StartItemId/ItemsPerPageTable[ItemsPageId];
		if (PageIndex < 4)
		{
			PageIndex = 0;
			EndPageIndex = 9;
		}
		else
		{
			if ((PageIndex + 4) < AllPages)
			{
                PageIndex -= 4;
			    EndPageIndex = PageIndex + 9;
			}
			else
			{
                PageIndex = AllPages - 9;
			    EndPageIndex = AllPages;
            }
        }
	    for (;PageIndex < EndPageIndex;PageIndex++)
		{
			AddStrWebPage("<td width=\"25\" align=\"center\" valign=\"center\"");
		    if ((PageIndex*ItemsPerPageTable[ItemsPageId]) == StartItemId)
			{
				AddStrWebPage("bgcolor=\"#577475\" >\r\n");
			    sprintf(BufStrLine, "<span class=\"pagenav\">%d</span>\r\n", PageIndex+1);
			    AddStrWebPage( BufStrLine);
			}
		    else
			{
				AddStrWebPage("bgcolor=\"#314142\" >\r\n");
	            AddStrWebPage("<a href=\"");
                SetServerHttpAddr(NULL);
			    AddStrWebPage(HtmlPageName);
                SetSessionIdCmdRef(SessionPtr);
                if (SectionId > 0)
				{
	                AddStrWebPage(KeySectionId);
	                sprintf(BufStrLine, "%d;", SectionId);
	                AddStrWebPage(BufStrLine);
				}
		        NewStartItemId = PageIndex*ItemsPerPageTable[ItemsPageId];
	            AddStrWebPage(KeyStartItemId);
	            sprintf(BufStrLine, "%d", NewStartItemId);
	            AddStrWebPage(BufStrLine);
			    sprintf(BufStrLine, "\" class=\"pagenav\"><strong>%d</strong></a>\r\n", PageIndex+1);
			    AddStrWebPage(BufStrLine);
			}
			AddStrWebPage("</td>\r\n");
		}
	}
    AddStrWebPage("</tr></table>\r\n");
	}

	if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
	{
        AddStrWebPage("</td></tr><tr>\r\n");
	}
    else
	{
	    AddStrWebPage("</td>\r\n");
	}

    AddStrWebPage("<td align=\"center\" valign=\"center\">\r\n");

	// Next shop items page selection
	if ((StartItemId + ItemsPerPageTable[ItemsPageId]) > ItemsInList)
	{
		SetNextPageNavDsButton();
	}
	else
	{
        AddStrWebPage("<a href=\"");
        SetServerHttpAddr(NULL);
		AddStrWebPage(HtmlPageName);
        SetSessionIdCmdRef(SessionPtr);
        if (SectionId > 0)
		{
	        AddStrWebPage(KeySectionId);
	        sprintf(BufStrLine, "%d;", SectionId);
	        AddStrWebPage(BufStrLine);
		}
	    if ((StartItemId+ItemsPerPageTable[ItemsPageId]) < ItemsInList)
	    {
		    NewStartItemId = StartItemId+ItemsPerPageTable[ItemsPageId];
	    }
	    else
	    {
		    NewStartItemId = StartItemId;
	    }
	    AddStrWebPage( KeyStartItemId);
	    sprintf(BufStrLine, "%d\" ", NewStartItemId);
	    AddStrWebPage( BufStrLine);
		SetNextPageNavEnButton();
	}

    AddStrWebPage("</td><td align=\"center\" valign=\"center\">\r\n");

	// Last shop items page selection
	if ((StartItemId + ItemsPerPageTable[ItemsPageId]) >= ItemsInList)
	{
		SetLastPageNavDsButton();
	}
	else
	{
	    AddStrWebPage("<a href=\"");
        SetServerHttpAddr(NULL);
		AddStrWebPage(HtmlPageName);
		SetSessionIdCmdRef(SessionPtr);
        if (SectionId > 0)
		{
	        AddStrWebPage(KeySectionId);
	        sprintf(BufStrLine, "%d;", SectionId);
	        AddStrWebPage(BufStrLine);
		}
	    NewStartItemId = (ItemsInList/ItemsPerPageTable[ItemsPageId]) * ItemsPerPageTable[ItemsPageId];
	    if (NewStartItemId == ItemsInList)
	    {
		    NewStartItemId = ItemsInList - ItemsPerPageTable[ItemsPageId];
	    }
	    AddStrWebPage( KeyStartItemId);
	    sprintf(BufStrLine, "%d\" ", NewStartItemId);
	    AddStrWebPage( BufStrLine);
        SetLastPageNavEnButton();
	}
	AddStrWebPage("</td></tr></table></div>\r\n");
}
//---------------------------------------------------------------------------
void AddNumItemsPerFilterPageSelectToHtml(char *BufAnsw, unsigned int SessionId,
										  unsigned int ItemsPageId, unsigned int SectionId,
										  char *HtmlPageName)
{
	unsigned int Index;
	USER_SESSION *SessionPtr = NULL;
	char BufStrLine[128];

	SessionPtr = GetSessionBySessionId(&ParWebServPtr->SessionManager, SessionId);
	if (!SessionPtr) return;

	if (BufAnsw) EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];
    AddStrWebPage("<select name=\"items_page\" class=\"inputbox\" size=\"1\" onchange=\"document.location.href=\'");
	SetServerHttpAddr(NULL);
	AddStrWebPage(HtmlPageName);
    SetSessionIdCmdRef(SessionPtr);
    if (SectionId > 0)
	{
	    AddStrWebPage(KeySectionId);
	    sprintf(BufStrLine, "%d", SectionId);
	    AddStrWebPage(BufStrLine);
	}
	AddStrWebPage(";items_page=' + this.options[selectedIndex].value + '&amp;limitstart=0';\">\r\n");
	for (Index=0;Index < (int)NumPageShowLevel;Index++)
	{
		if (Index == ItemsPageId)
		{
			sprintf(BufStrLine, "<option value=\"%d\" selected=\"selected\">%d</option>\r\n", 
				Index, ItemsPerPageTable[Index]);
		}
		else
		{
			sprintf(BufStrLine, "<option value=\"%d\">%d</option>\r\n", 
				Index, ItemsPerPageTable[Index]);
		}
		AddStrWebPage(BufStrLine);
	}
    AddStrWebPage( "</select>\r\n");
}
//---------------------------------------------------------------------------
void SetFirstPageNavDsButton()
{
	AddStrWebPage("<img src=\"");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavFirstPgDsKey));
	AddStrWebPage("\" title=\"");
    SetRusTextBuf(NULL, SITE_RUS_FIRST_PAGE_LINE_ID);
	AddStrWebPage("\">\r\n");
}
//---------------------------------------------------------------------------
void SetFirstPageNavEnButton()
{
    AddStrWebPage("class=\"pagenav\" title=\"");
	SetRusTextBuf(NULL, SITE_RUS_FIRST_PAGE_LINE_ID);
    AddStrWebPage("\"><div  style=\"background-image: url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavFirstPgEnKey));
	AddStrWebPage("); width: 90px; height: 25px; \"\r\n");
	AddStrWebPage("onmouseover = \"this.style.backgroundImage = 'url(");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GetImageNameByKey(BtNavFirstPgMOKey));
	AddStrWebPage(")'\"\r\n");
    AddStrWebPage("onmouseout = \"this.style.backgroundImage = 'url(");
    SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavFirstPgEnKey));
	AddStrWebPage(")'\"\r\n");
	AddStrWebPage("onmousedown = \"this.style.backgroundImage = 'url(");
    SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavFirstPgMSKey));
    AddStrWebPage(")'\"\r\n");
	AddStrWebPage("onmouseup = \"this.style.backgroundImage = 'url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavFirstPgMOKey));
	AddStrWebPage(")'\"></div>");
	AddStrWebPage("</a>\r\n");
}
//---------------------------------------------------------------------------
void SetPrevPageNavDsButton()
{
	AddStrWebPage("<img src=\"");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavPrevPgDsKey));
    AddStrWebPage("\" title=\"");
    SetRusTextBuf(NULL, SITE_RUS_PREV_PAGE_LINE_ID);
    AddStrWebPage("\">\r\n");
}
//---------------------------------------------------------------------------
void SetPrevPageNavEnButton()
{
    AddStrWebPage("class=\"pagenav\" title=\"");
	SetRusTextBuf(NULL, SITE_RUS_PREV_PAGE_LINE_ID);
	AddStrWebPage("\"><div style=\"background-image: url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavPrevPgEnKey));
	AddStrWebPage("); width: 100px; height: 25px; \"\r\n");
	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
	    AddStrWebPage("onmouseover = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
	    AddStrWebPage(GetImageNameByKey(BtNavPrevPgMOKey));
	    AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmouseout = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavPrevPgEnKey));
	    AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmousedown = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavPrevPgMSKey));
        AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmouseup = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavPrevPgMOKey));
	    AddStrWebPage(")'\"></div>");
	}
	else
	{
        AddStrWebPage("\"></div>");
	}
	AddStrWebPage("</a>\r\n");
}
//---------------------------------------------------------------------------
void SetNextPageNavDsButton()
{
	AddStrWebPage("<img src=\"");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavNextPgDsKey));
	AddStrWebPage("\" title=\"");
    SetRusTextBuf(NULL, SITE_RUS_NEXT_PAGE_LINE_ID);
	AddStrWebPage("\">\r\n");
}
//---------------------------------------------------------------------------
void SetNextPageNavEnButton()
{
    AddStrWebPage("class=\"pagenav\" title=\"");
	SetRusTextBuf(NULL, SITE_RUS_NEXT_PAGE_LINE_ID);
	AddStrWebPage("\"><div style=\"background-image: url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavNextPgEnKey));
	AddStrWebPage("); width: 100px; height: 25px; \"\r\n");
	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
	    AddStrWebPage("onmouseover = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
	    AddStrWebPage(GetImageNameByKey(BtNavNextPgMOKey));
	    AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmouseout = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavNextPgEnKey));
	    AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmousedown = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavNextPgMSKey));
        AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmouseup = \"this.style.backgroundImage = 'url(");
        SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavNextPgMOKey));
	    AddStrWebPage(")'\"></div>");
	}
	else
	{
        AddStrWebPage("\"></div>");
	}
	AddStrWebPage("</a>\r\n");
}
//---------------------------------------------------------------------------
void SetLastPageNavDsButton()
{
	AddStrWebPage("<img src=\"");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavLastPgDsKey));
	AddStrWebPage("\" title=\"");
    SetRusTextBuf(NULL, SITE_RUS_LAST_PAGE_LINE_ID);
	AddStrWebPage("\">\r\n");
}
//---------------------------------------------------------------------------
void SetLastPageNavEnButton()
{
    AddStrWebPage("class=\"pagenav\" title=\"");
	SetRusTextBuf(NULL, SITE_RUS_LAST_PAGE_LINE_ID);
    AddStrWebPage("\"><div style=\"background-image: url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavLastPgEnKey));
	AddStrWebPage("); width: 100px; height: 25px; \"\r\n");
	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
	    AddStrWebPage("onmouseover = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
	    AddStrWebPage(GetImageNameByKey(BtNavLastPgMOKey));
	    AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmouseout = \"this.style.backgroundImage = 'url(");
        SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavLastPgEnKey));
	    AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmousedown = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavLastPgMSKey));
        AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmouseup = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavLastPgMOKey));
	    AddStrWebPage(")'\"></div>");
	}
	else
	{
        AddStrWebPage("\"></div>");
	}
	AddStrWebPage("</a>\r\n");
}
//---------------------------------------------------------------------------
void SetReturnNavDsButton()
{
	AddStrWebPage("<img src=\"");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavRetPgDsKey));
	AddStrWebPage("\" title=\"");
    SetRusTextBuf(NULL, SITE_RUS_RETURN_PREV_LEVEL_INF_LINE_ID);
	AddStrWebPage("\">\r\n");
}
//---------------------------------------------------------------------------
void SetReturnNavEnButton()
{
    AddStrWebPage("class=\"pagenav\" title=\"");
	SetRusTextBuf(NULL, SITE_RUS_RETURN_PREV_LEVEL_INF_LINE_ID);
    AddStrWebPage("\"><div style=\"background-image: url(");
	SetServerHttpAddr(NULL);
    AddStrWebPage(GetImageNameByKey(BtNavRetPgEnKey));
	AddStrWebPage("); width: 100px; height: 25px; \"\r\n");
	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
	    AddStrWebPage("onmouseover = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
	    AddStrWebPage(GetImageNameByKey(BtNavRetPgMOKey));
	    AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmouseout = \"this.style.backgroundImage = 'url(");
        SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavRetPgEnKey));
	    AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmousedown = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavRetPgMSKey));
        AddStrWebPage(")'\"\r\n");
	    AddStrWebPage("onmouseup = \"this.style.backgroundImage = 'url(");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(GetImageNameByKey(BtNavRetPgMOKey));
	    AddStrWebPage(")'\"></div>");
	}
	else
	{
        AddStrWebPage("\"></div>");
	}
	AddStrWebPage("</a>\r\n");
}
//---------------------------------------------------------------------------
