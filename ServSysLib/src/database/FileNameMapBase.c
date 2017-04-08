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
#include "FileNameMapBase.h"
#include "FileNameMapHash.h"

FILE_NAME_MAP_HASH_CHAR_HOP FileNameMapHashHop;
FILE_NAME_MAP_HASH_CHAR_HOP ReversFileNameMapHashHop;

static unsigned int FileMapRecCount = 0;

extern char FileNameMapDbNamePath[];

#ifdef WIN32
extern HANDLE gFileMutex;
#endif

#define MAX_LEN_FNM_BASE_LINE 512

/* List of accepted commands for mobile devices database.*/
	char	*TablFileNameMapParsDb[] = {
        "DbFileNameMapCreate",  //The creating of file name map record;
		"comment",			    //The identificator of not visibled comment for this line;
	};

	char CmdDbExtFileName[]         = "ExtFileName";
    char CmdDbLocFileName[]         = "LocFileName";

static void CmdAddFileNameMapDb(unsigned char *CmdLinePtr);
//---------------------------------------------------------------------------
void FileNameMapDBLoad()
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
	char DbTextItem[MAX_LEN_FNM_BASE_LINE+1];
	char StartPath[512];
	char BdFileName[1024];

    InitFileNameMapHash(&FileNameMapHashHop);
    InitFileNameMapHash(&ReversFileNameMapHashHop);
#ifdef WIN32
	RetCodeCwd = _getcwd((char*)(&StartPath[0]),512);
#else
	RetCodeCwd = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, FileNameMapDbNamePath);
#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB file names map list (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB file names map list (%s) is not file\n", BdFileName);
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
	while (PointFind < SizeFile)
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
	                switch ( FindCmdArray( DbTextItem, TablFileNameMapParsDb, 2 ) )
                    {
                        case  0:
						    CmdAddFileNameMapDb((unsigned char*)&DbTextItem[0]);
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
			   if (DbTextLine < MAX_LEN_FNM_BASE_LINE)
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
	printf("File name mapping DB load is done (Records in hash: %d)\n", FileMapRecCount);
}
//---------------------------------------------------------------------------
static void CmdAddFileNameMapDb(unsigned char *CmdLinePtr)
{
	unsigned int  i;
	unsigned int  DataLen;
    char          *FText;
    char          *StrPar;
	char		  *FStrt = NULL;
    char          FileNameKey[MAX_LEN_FILE_NAME_MAP+1];
    char          FileNameMap[MAX_LEN_FILE_NAME_MAP+1];

    FStrt = 0;
	for(;;)
	{
        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory( strlen(StrPar)+4 );
	    FStrt = FText;
        strcpy(FText,StrPar);

		/* Parse external file name field */
        i = FindCmdRequest(FText, CmdDbExtFileName);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		DataLen = strlen(FText);
		if (!DataLen || DataLen > MAX_LEN_FILE_NAME_MAP) break;
		strcpy(FileNameKey, (const char*)FText);
		FText = FStrt;
		strcpy(FText,StrPar);

		/* Parse internal file name field */
        i = FindCmdRequest(FText, CmdDbLocFileName);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		DataLen = strlen(FText);
		if (!DataLen || DataLen > MAX_LEN_FILE_NAME_MAP) break;
		strcpy(FileNameMap, (const char*)FText);
		FText = FStrt;
		strcpy(FText,StrPar);
        
        if (!AddFileNameMapHash(&FileNameMapHashHop, FileNameKey, FileNameMap))
        {
            printf("Fail to add Key: %s; Map: %s to file name map\n", FileNameKey, FileNameMap);
        }
        if (!AddFileNameMapHash(&ReversFileNameMapHashHop, FileNameMap, FileNameKey))
        {
            printf("Fail to add Key: %s; Map: %s to file name revers map\n", FileNameMap, FileNameKey);
        }        
        FileMapRecCount++;
		break;
	}
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
char* GetLocalFileNameByExtName(char *FileNameKey)
{
    FILE_NAME_MAP_HASH_RECORD *RecordPtr = NULL;
    unsigned int              CharIndex, NameLen, SubDirCnt;
	char                      *NameOnlyPtr = NULL;
    
    SubDirCnt = 0;
	NameLen = strlen(FileNameKey);
	for (CharIndex=NameLen;CharIndex > 0;CharIndex--)
	{
	  #ifdef _LINUX_X86_
	    if (FileNameKey[CharIndex-1] == '/')
	  #endif
	  #ifdef _VCL60ENV_
	    if (FileNameKey[CharIndex-1] == '\\')
	  #endif
	    {
            SubDirCnt++;
            if (SubDirCnt > 1)
            {
	            NameOnlyPtr = &FileNameKey[CharIndex];
	            break;
            }
	    }
	}
	if (!NameOnlyPtr) return NULL;    
    RecordPtr = FindFileNameMapHash(&FileNameMapHashHop, NameOnlyPtr);
    if (!RecordPtr) return NULL;
    else            return (char*)RecordPtr->FileNameMap;
}
//---------------------------------------------------------------------------
char* GetExtFileNameByLocalName(char *FileNameKey)
{
    FILE_NAME_MAP_HASH_RECORD *RecordPtr = NULL;
    
    RecordPtr = FindFileNameMapHash(&ReversFileNameMapHashHop, FileNameKey);
    if (!RecordPtr) return FileNameKey;
    else            return (char*)RecordPtr->FileNameMap;
}
//---------------------------------------------------------------------------
void FileNameMapDBClear()
{
    CloseFileNameMapHashHop(&FileNameMapHashHop);
    CloseFileNameMapHashHop(&ReversFileNameMapHashHop);
}
//---------------------------------------------------------------------------
