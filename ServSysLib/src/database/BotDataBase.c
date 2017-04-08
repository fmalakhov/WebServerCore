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
#include "vistypes.h"
#include "SysLibTool.h"
#include "SysWebFunction.h"
#include "BotDataBase.h"
#include "BotNameHash.h"

unsigned int NewBotInfoIndex = 1;
unsigned int MinBotKeyLen = 0;

ListItsTask  BotInfoList;
BOT_NAME_MAP_HASH_CHAR_HOP BotInfoHash;

extern char BotInfoDbNamePath[];

#ifdef WIN32
extern HANDLE gFileMutex;
#endif

/* List of accepted commands for mobile devices database.*/
	char	*TablBotInfoParsDb[] = {
        "DbBotInfoCreate",       //The creating of new BOT info. record;
		"comment",			     //The identificator of not visibled comment for this line;
	};

    char CmdDbBotId[]            = "BotId";
	char CmdDbBotName[]          = "BotName";
    char CmdDbBotIdentKey[]      = "BotIdentKey";
    char CmdDbBotIndex[]         = "BotIndex";

//---------------------------------------------------------------------------
void BotInfoDBLoad()
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
	char DbTextItem[MAX_LEN_BTDT_BASE_LINE+1];
	char StartPath[512];
	char BdFileName[1024];

    BotInfoList.Count = 0;
	BotInfoList.CurrTask = NULL;
	BotInfoList.FistTask = NULL;
    InitBotNameHash(&BotInfoHash);
#ifdef WIN32
	RetCodeCwd = _getcwd((char*)(&StartPath[0]),512);
#else
	RetCodeCwd = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, BotInfoDbNamePath);
#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB BOT list (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB BOT list (%s) is not file\n", BdFileName);
            fclose(FileHandler);
            return;
        }
#endif
#ifdef WIN32
	HFSndMess = CreateFile((LPCWSTR)BdFileName,0,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
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
				if ((DbTextItem[0] !='\r') && (DbTextItem[0] !='\n'))
		        {
					LastRot = true;
			        DbTextItem[DbTextLine] = 0;
	                switch ( FindCmdArray( DbTextItem, TablBotInfoParsDb, 2 ) )
                    {
                        case  0:
						    CmdAddNewBotDb((unsigned char*)&DbTextItem[0]);
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
			   if (DbTextLine < MAX_LEN_BTDT_BASE_LINE)
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
	printf("Bot info. DB load is done (Records base: %d)\n",
		(unsigned int)BotInfoList.Count);
}
//---------------------------------------------------------------------------
void CmdAddNewBotDb(unsigned char *CmdLinePtr)
{
	unsigned int  i, KeyLen;
	unsigned int  DataLen;
	int           pars_read, ReadValue;
	bool          isCompleate = false;
    char          *FText;
    char          *StrPar;
	char		  *FStrt = NULL;
    BOT_INFO_TYPE *NewBotPtr = NULL;

	for(;;)
	{
	    NewBotPtr = (BOT_INFO_TYPE*)AllocateMemory(sizeof(BOT_INFO_TYPE));
		if (!NewBotPtr) break;
        
        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory( strlen(StrPar)+4 );
	    FStrt = FText;
        strcpy(FText,StrPar);

        /* Parse BOT id field */
        i = FindCmdRequest(FText, CmdDbBotId);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		if (!strlen(FText)) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewBotPtr->BotId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

		/* Parse BOT index field */
        i = FindCmdRequest(FText, CmdDbBotIndex);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		if (!strlen(FText)) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewBotPtr->BotIndex = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

		/* Parse BOT name field */
        i = FindCmdRequest(FText, CmdDbBotName);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		DataLen = strlen(FText);
		if (!DataLen || DataLen > MAX_LEN_BOT_NAME) break;
		strcpy(NewBotPtr->BotName, (const char*)FText);
		FText = FStrt;
		strcpy(FText,StrPar);

		/* Parse BOT key field */
        i = FindCmdRequest(FText, CmdDbBotIdentKey);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		DataLen = strlen(FText);
		if (!DataLen || DataLen > MAX_LEN_BOT_KEY) break;
		strcpy(NewBotPtr->BotKey, (const char*)FText);
		FText = FStrt;
		strcpy(FText,StrPar);
        KeyLen = strlen(NewBotPtr->BotKey);
        if (!MinBotKeyLen)
        {
            MinBotKeyLen = KeyLen;
        }
        else
        {
            if (MinBotKeyLen > KeyLen)
            {
                MinBotKeyLen = KeyLen;
            }
        }
		AddStructList(&BotInfoList, NewBotPtr);
        if (!AddBotNameHash(&BotInfoHash, NewBotPtr->BotKey, NewBotPtr))
        {
            printf("Fail to add %s bot to hash\n", NewBotPtr->BotName);
        }
		if (NewBotInfoIndex <= NewBotPtr->BotId) 
			NewBotInfoIndex = NewBotPtr->BotId + 1;
	    isCompleate = true;
		break;
	}
	if (!isCompleate && NewBotPtr) FreeMemory(NewBotPtr);
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void BotDBClear()
{
    ObjListTask	    *SelObjPtr = NULL;
	BOT_INFO_TYPE   *SelBotRecPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&BotInfoList);
	while(SelObjPtr)
	{
	    SelBotRecPtr = (BOT_INFO_TYPE*)SelObjPtr->UsedTask;
		FreeMemory(SelBotRecPtr);
		RemStructList(&BotInfoList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&BotInfoList);
	}
    CloseBotNameHashHop(&BotInfoHash);
}
//---------------------------------------------------------------------------
unsigned char FindBotInfoStrLine(unsigned char *StrPtr, unsigned int StrLen)
{
    unsigned char            BotIndex = BOT_NONE;
    BOT_NAME_MAP_HASH_RECORD *BotRecPtr = NULL;
   
    if (!MinBotKeyLen || (StrLen < MinBotKeyLen)) return BotIndex;
    while(StrLen >= MinBotKeyLen)
    {
        BotRecPtr = FindBotNameHash(&BotInfoHash, StrPtr);
        if (BotRecPtr && BotRecPtr->BotInfoPtr)
        {
            BotIndex = (unsigned char)BotRecPtr->BotInfoPtr->BotIndex;
            break;
        }
        StrPtr++;
        StrLen--;
    }
    return BotIndex;
}
//---------------------------------------------------------------------------
