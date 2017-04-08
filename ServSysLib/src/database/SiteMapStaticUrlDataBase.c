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
#include "SiteMapStaticUrlDataBase.h"

ListItsTask  SiteMapStUrlList;

extern char SiteMapStUrlDbNamePath[];

#ifdef WIN32
extern HANDLE gFileMutex;
#endif

#define MAX_LEN_SMURL_BASE_LINE 700

/* List of accepted commands for mobile devices database.*/
	char	*TablSiteMapStUrlParsDb[] = {
        "DbStUrlCreate",        //The creating of new site map static url record;
		"comment",			    //The identificator of not visibled comment for this line;
	};

    char CmdDbSiteMapStUrlInf[]          = "SMUrlInf";
	char CmdDbSiteMapStUrlLastChgDate[]  = "SMUrlDate";
	char CmdDbSiteMapStUrlPriority[]     = "SMUrlPriority";

static void CmdAddNewSiteMapStUrlDb(unsigned char *CmdLinePtr);
//---------------------------------------------------------------------------
void SiteMapStUrlDBLoad()
{
	DWORD PointFind;
	bool     LastRot;
#ifdef WIN32
	DWORD  SizeFile;
	HANDLE HFSndMess;
#else
	unsigned long SizeFile;
        struct stat st;
#endif
	FILE   *FileHandler;
	int pars_read = 0;
	unsigned int read_blk;
	unsigned int DbTextLine;
	unsigned char *FileData;
	char          *RetCodeCwd = NULL;
	char DbTextItem[MAX_LEN_SMURL_BASE_LINE+1];
	char StartPath[512];
	char BdFileName[1024];

    SiteMapStUrlList.Count = 0;
	SiteMapStUrlList.CurrTask = NULL;
	SiteMapStUrlList.FistTask = NULL;
#ifdef WIN32
	RetCodeCwd = _getcwd((char*)(&StartPath[0]),512);
#else
	RetCodeCwd = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, SiteMapStUrlDbNamePath);
#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB static URLs for site map list (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB static URLs for site map list (%s) is not file\n", BdFileName);
            fclose(FileHandler);
            return;
        }
#endif
#ifdef WIN32
	HFSndMess = CreateFile((LPCWSTR)BdFileName, 0, FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (HFSndMess == INVALID_HANDLE_VALUE)
	{
	   return;
	}
	SizeFile = GetFileSize( HFSndMess, NULL );
	CloseHandle( HFSndMess );
        FileHandler = fopen(BdFileName,"rb");
#endif
	FileData = (unsigned char*)AllocateMemory( SizeFile+2 );
	read_blk = fread((unsigned char*)FileData, 1, SizeFile, FileHandler);
	LastRot = false;
    PointFind = 0;
	DbTextLine = 0;
	while ( PointFind < SizeFile )
    {
		if (FileData[PointFind] == 0x00) break;
		if ( FileData[PointFind] =='\r' || FileData[PointFind] =='\n')
        {
			if ( LastRot ) goto NoMove;
            if ( FileData[PointFind] =='\r' ) LastRot = true;
            DbTextItem[DbTextLine] = 0;
            if ((DbTextItem[0] !='#') && (FindCmdRequest(DbTextItem, "comment") == -1))
            {
				if ( DbTextItem[0] !='\r' && DbTextItem[0] !='\n')
		        {
					LastRot = true;
			        DbTextItem[DbTextLine] = 0;
	                switch ( FindCmdArray( DbTextItem, TablSiteMapStUrlParsDb, 2 ) )
                    {
                        case  0:
						    CmdAddNewSiteMapStUrlDb((unsigned char*)&DbTextItem[0]);
							break;

		                default:
							break;
	                }					
					DbTextLine = 0;
			        PointFind++;
		        }
		    }
		    else
		    {
				DbTextLine = 0;
			    FileData[DbTextLine] = 0;
		    }
		    PointFind++;
	   }
       else
       {
   NoMove:
           if ((FileData[PointFind] !='\r') &&
			   (FileData[PointFind] !='\n') &&
			   (FileData[PointFind] != 0))
           {
			   if (DbTextLine < MAX_LEN_SMURL_BASE_LINE)
			   {
			       DbTextItem[DbTextLine] = FileData[PointFind];
                   DbTextLine++;
			   }
           }
           PointFind++;
		   LastRot = false;
        }
    }
	fclose(FileHandler);
	FreeMemory(FileData);
	printf("Static URLs for site map DB load is done (URLs in base: %d)\n",
		(unsigned int)SiteMapStUrlList.Count);
}
//---------------------------------------------------------------------------
static void CmdAddNewSiteMapStUrlDb(unsigned char *CmdLinePtr)
{
	unsigned int  i;
	unsigned int  DataLen;
	bool          isCompleate = false;
    char          *FText;
    char          *StrPar;
	char		  *FStrt = NULL;
    SITEMAP_URL_INFO *NewSTUrlPtr = NULL;

	for(;;)
	{
	    NewSTUrlPtr = (SITEMAP_URL_INFO*)AllocateMemory(sizeof(SITEMAP_URL_INFO));
		if (!NewSTUrlPtr) break;
		memset(NewSTUrlPtr, 0, sizeof(SITEMAP_URL_INFO));
        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory( strlen(StrPar)+4 );
		if (!FText) break;
	    FStrt = FText;
        strcpy(FText,StrPar);

        /* Parse site map URL field */
        i = FindCmdRequest(FText, CmdDbSiteMapStUrlInf);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		DataLen = strlen(FText);
		if (!DataLen || DataLen > MAX_LEN_SMINF_URL) break;
		strcpy((char*)&NewSTUrlPtr->Url[0], (const char*)FText);
		FText = FStrt;
		strcpy(FText,StrPar);

        /* Parse site map last change date field */
        i = FindCmdRequest(FText, CmdDbSiteMapStUrlLastChgDate);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		DataLen = strlen(FText);
		if (DataLen != MAX_LEN_SMINF_LCDATE) break;
		strcpy((char*)&NewSTUrlPtr->LastChangeDate[0], (const char*)FText);
		FText = FStrt;
		strcpy(FText,StrPar);

        /* Parse site map priority field */
        i = FindCmdRequest(FText, CmdDbSiteMapStUrlPriority);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		DataLen = strlen(FText);
		if (DataLen != MAX_LEN_SMINF_PRIORITY) break;
		strcpy((char*)&NewSTUrlPtr->Priority[0], (const char*)FText);
		FText = FStrt;
		strcpy(FText,StrPar);

		AddStructList(&SiteMapStUrlList, NewSTUrlPtr);
	    isCompleate = true;
		break;
	}
	if (!isCompleate && NewSTUrlPtr) FreeMemory(NewSTUrlPtr);
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void SiteMapStUrlDBClear()
{
    ObjListTask	     *SelObjPtr = NULL;
	SITEMAP_URL_INFO *SelSiteMapStUrlRecPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&SiteMapStUrlList);
	while(SelObjPtr)
	{
	    SelSiteMapStUrlRecPtr = (SITEMAP_URL_INFO*)SelObjPtr->UsedTask;
        FreeMemory(SelSiteMapStUrlRecPtr);
		RemStructList(&SiteMapStUrlList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&SiteMapStUrlList);
	}
}
//---------------------------------------------------------------------------
