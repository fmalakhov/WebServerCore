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
#include "SysMessages.h"
#include "HttpPageGen.h"

extern ListItsTask  SiteMapStUrlList;
extern ListItsTask  UserInfoList;
extern PARAMWEBSERV *ParWebServPtr;
extern char *EndHtmlPageGenPtr;
extern FILE_HASH_CHAR_HOP FileHashHop;
extern bool gIsSiteMapGenNeeds;
extern char *MemWebPageGenPtr;
extern char BaseWebDataPath2[];

#ifdef _VCL60ENV_
extern HANDLE gFileMutex;
#endif
//---------------------------------------------------------------------------
void SiteMapFileGen(PARAMWEBSERV *ParWebServ)
{
	FILE             *FileSiteMapPtr = NULL;
	char             *PathFilePtr = NULL;
	char             *CwdRet = NULL;
	FILE_HASH_RECORD *FileRecPtr = NULL;
    ObjListTask	     *SelObjPtr = NULL;
    ObjListTask	     *SelItemObjPtr = NULL;
	SITEMAP_URL_INFO *SelSiteMapStUrlRecPtr = NULL;
	char             StrBuf[256];

	PathFilePtr = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathFilePtr) return;

	CwdRet = getcwd(PathFilePtr, 512);
	strcat(PathFilePtr, BaseWebDataPath2);
	strcat(PathFilePtr, SiteMapNamePath);
	FileSiteMapPtr = fopen(PathFilePtr,"wb");
	if (!FileSiteMapPtr) 
	{
		FreeMemory(PathFilePtr);
		return;
	}

	ParWebServPtr = ParWebServ;
	EndHtmlPageGenPtr = MemWebPageGenPtr;
    *EndHtmlPageGenPtr = 0;

    AddStrWebPage("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    AddStrWebPage("<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n");

	SelObjPtr = (ObjListTask*)GetFistObjectList(&SiteMapStUrlList);
	while(SelObjPtr)
	{
	    SelSiteMapStUrlRecPtr = (SITEMAP_URL_INFO*)SelObjPtr->UsedTask;
        AddStrWebPage("  <url>\n");
        AddStrWebPage("    <loc>");
        SetServerHttpAddr(NULL);
	    AddStrWebPage((char*)&SelSiteMapStUrlRecPtr->Url[0]);
		AddStrWebPage("</loc>\n");
        AddStrWebPage("    <lastmod>");
		AddStrWebPage((char*)&SelSiteMapStUrlRecPtr->LastChangeDate[0]);
		AddStrWebPage("+00:00</lastmod>\n");
        AddStrWebPage("    <changefreq>always</changefreq>\n");
        AddStrWebPage("    <priority>");
		AddStrWebPage((char*)&SelSiteMapStUrlRecPtr->Priority[0]);
		AddStrWebPage("</priority>\n");
        AddStrWebPage("  </url>\n");
        SelObjPtr = (ObjListTask*)GetNextObjectList(&SiteMapStUrlList);
	}

    AddStrWebPage("</urlset>\n");

	fwrite(MemWebPageGenPtr, strlen(MemWebPageGenPtr), 1, FileSiteMapPtr);
	fclose(FileSiteMapPtr);

	CwdRet = getcwd(PathFilePtr, 512);
	strcat(PathFilePtr, BaseWebDataPath2);
	
	/* Update site_map.xml in files hash */
	FileRecPtr = FindFileHash(&FileHashHop, BASE_HASH_LIST, PathFilePtr);		
	if (FileRecPtr) RemFileHash(&FileHashHop, BASE_HASH_LIST, PathFilePtr);
	AddSingleFileHash(&FileHashHop, BASE_HASH_LIST, PathFilePtr, SiteMapNamePath);

	gIsSiteMapGenNeeds = false;
	FreeMemory(PathFilePtr);
}
//---------------------------------------------------------------------------
