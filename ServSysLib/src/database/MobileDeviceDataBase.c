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
#include "MobileDeviceDataBase.h"
#include "MobileDeviceHash.h"
#include "WebServInfo.h"

unsigned int NewMobDevIndex = 1;
unsigned int MinMobDevKeyLen = 0;

ListItsTask  MobileDeviceList;
MOBILE_DEV_MAP_HASH_CHAR_HOP MobileDeviceHash;

extern char MobileBeviceDbNamePath[];

#ifdef _VCL60ENV_
extern HANDLE gFileMutex;
#endif

/* List of accepted commands for mobile devices database.*/
	char	*TablMobDeviceParsDb[] = {
        "DbMobDevCreate",       //The creating of new mobile device record ;
		"comment",			    //The identificator of not visibled comment for this line;
	};

	char CmdDbMobDevId[]         = "DeviceId";
    char CmdDbMobDevDetect[]     = "DeviceDetect";
	char CmdDbMobDevName[]       = "DeviceName";
	char CmdDbMobDevScrWidth[]   = "DeviceScrWidth";
	char CmdDbMobDevScrHeight[]  = "DeviceScrHeight";
	char CmdDbMobDevAspWidth[]   = "DeviceAspWidth";
	char CmdDbMobDevAspHeight[]  = "DeviceAspHeight";
    char CmdDbMobDevType[]       = "DeviceType";

//---------------------------------------------------------------------------
void MobileDeviceDBLoad()
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
	char DbTextItem[MAX_LEN_MBDV_BASE_LINE+1];
	char StartPath[512];
	char BdFileName[1024];

    MobileDeviceList.Count = 0;
	MobileDeviceList.CurrTask = NULL;
	MobileDeviceList.FistTask = NULL;
    InitMobileDeviceHash(&MobileDeviceHash);
#ifdef WIN32
	RetCodeCwd = _getcwd((char*)(&StartPath[0]),512);
#else
	RetCodeCwd = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, MobileBeviceDbNamePath);
#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB mobile devices list (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB mobile devices list (%s) is not file\n", BdFileName);
            fclose(FileHandler);
            return;
        }
#endif
#ifdef WIN32
	HFSndMess = CreateFile((LPCWSTR)BdFileName, 0,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
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
	                switch ( FindCmdArray( DbTextItem, TablMobDeviceParsDb, 2 ) )
                    {
                        case  0:
						    CmdAddNewMobileDeviceDb((unsigned char*)&DbTextItem[0]);
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
			   if (DbTextLine < MAX_LEN_MBDV_BASE_LINE)
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
	printf("Mobile devices DB load is done (Devices in base: %d)\n",
		(unsigned int)MobileDeviceList.Count);
}
//---------------------------------------------------------------------------
void CmdAddNewMobileDeviceDb(unsigned char *CmdLinePtr)
{
	unsigned int  i, KeyLen;
	unsigned int  DataLen;
	int           pars_read, ReadValue;
	bool          isCompleate = false;
    char          *FText;
    char          *StrPar;
	char		  *FStrt = NULL;
    MOBILE_DEV_TYPE *NewDevicePtr = NULL;

    FStrt = 0;
	for(;;)
	{
	    NewDevicePtr = (MOBILE_DEV_TYPE*)AllocateMemory(sizeof(MOBILE_DEV_TYPE));
		if (!NewDevicePtr) break;
		NewDevicePtr->DevDetectLine = NULL;
        NewDevicePtr->DevName = NULL;
        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory( strlen(StrPar)+4 );
	    FStrt = FText;
        strcpy(FText,StrPar);

        /* Parse mobile device id field */
        i = FindCmdRequest(FText, CmdDbMobDevId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
		if (!strlen(FText)) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewDevicePtr->MobDevId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

		/* Parse mobile device detection field */
        i = FindCmdRequest(FText, CmdDbMobDevDetect);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		DataLen = strlen(FText);
		if (!DataLen || DataLen > MAX_LEN_MD_DETECT) break;
        NewDevicePtr->DevDetectLine = (char*)AllocateMemory((DataLen+1)*sizeof(char));
		if (!NewDevicePtr->DevDetectLine) break;
		strcpy(NewDevicePtr->DevDetectLine, (const char*)FText);
		FText = FStrt;
		strcpy(FText,StrPar);

		/* Parse mobile device name field */
        i = FindCmdRequest(FText, CmdDbMobDevName);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		DataLen = strlen(FText);
		if (!DataLen || DataLen > MAX_LEN_MD_NAME) break;
        NewDevicePtr->DevName = (char*)AllocateMemory((DataLen+1)*sizeof(char));
		if (!NewDevicePtr->DevName) break;
		strcpy(NewDevicePtr->DevName, (const char*)FText);
		FText = FStrt;
		strcpy(FText,StrPar);

        /* Parse mobile screen width field */
        i = FindCmdRequest(FText, CmdDbMobDevScrWidth);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		if (!strlen(FText)) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read || ReadValue < 320) break;
		NewDevicePtr->ScreenWidth = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse mobile screen height field */
        i = FindCmdRequest(FText, CmdDbMobDevScrHeight);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		if (!strlen(FText)) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read || ReadValue < 200) break;
		NewDevicePtr->ScreenHeigh = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse aspect screen width field */
        i = FindCmdRequest(FText, CmdDbMobDevAspWidth);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		if (!strlen(FText)) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read || ReadValue < 320) break;
		NewDevicePtr->AspectWidth = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse aspect screen heigh field */
        i = FindCmdRequest(FText, CmdDbMobDevAspHeight);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		if (!strlen(FText)) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read || ReadValue < 100) break;
		NewDevicePtr->AspectHeigh = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse mobile device type field */
        i = FindCmdRequest(FText, CmdDbMobDevType);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		if (!strlen(FText)) break;
		if (!strcmp(FText, "PHONE"))
		{
            NewDevicePtr->DeviceSize = MDT_PHONE;
		}
		else if (!strcmp(FText, "phone"))
		{
            NewDevicePtr->DeviceSize = MDT_PHONE;
		}
		else if (!strcmp(FText, "PAD"))
		{
            NewDevicePtr->DeviceSize = MDT_PAD;
		}
		else if (!strcmp(FText, "pad"))
		{
            NewDevicePtr->DeviceSize = MDT_PAD;
		}
		else
		{
			break;
		}

        KeyLen = strlen(NewDevicePtr->DevDetectLine);
        if (!MinMobDevKeyLen)
        {
            MinMobDevKeyLen = KeyLen;
        }
        else
        {
            if (MinMobDevKeyLen > KeyLen)
            {
                MinMobDevKeyLen = KeyLen;
            }
        }        
		AddStructList(&MobileDeviceList, NewDevicePtr);
        if (!AddMobileDeviceHash(&MobileDeviceHash, NewDevicePtr->DevDetectLine, NewDevicePtr))
        {
            printf("Fail to add %s mobile device to hash\n", NewDevicePtr->DevDetectLine);
        }        
		if (NewMobDevIndex <= NewDevicePtr->MobDevId) 
			NewMobDevIndex = NewDevicePtr->MobDevId + 1;
	    isCompleate = true;
		break;
	}
	if (!isCompleate && NewDevicePtr)
	{
		if (NewDevicePtr->DevDetectLine) FreeMemory(NewDevicePtr->DevDetectLine);
		if (NewDevicePtr->DevName)       FreeMemory(NewDevicePtr->DevName);
		FreeMemory(NewDevicePtr);
	}
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void MobileDeviceDBClear()
{
    ObjListTask	    *SelObjPtr = NULL;
	MOBILE_DEV_TYPE *SelMobileDevRecPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&MobileDeviceList);
	while(SelObjPtr)
	{
	    SelMobileDevRecPtr = (MOBILE_DEV_TYPE*)SelObjPtr->UsedTask;
		if (SelMobileDevRecPtr->DevDetectLine) FreeMemory(SelMobileDevRecPtr->DevDetectLine);
		if (SelMobileDevRecPtr->DevName) FreeMemory(SelMobileDevRecPtr->DevName);
		FreeMemory(SelMobileDevRecPtr);
		RemStructList(&MobileDeviceList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&MobileDeviceList);
	}
    CloseMobileDeviceHashHop(&MobileDeviceHash);
}
//---------------------------------------------------------------------------
MOBILE_DEV_TYPE* FindMobileDevice(unsigned char *StrPtr, unsigned int StrLen)
{
    MOBILE_DEV_TYPE*           MobDevPtr = NULL;
    MOBILE_DEV_MAP_HASH_RECORD *MDRecPtr = NULL;
   
    if (!MinMobDevKeyLen || (StrLen < MinMobDevKeyLen)) return MobDevPtr;
    while(StrLen >= MinMobDevKeyLen)
    {
        MDRecPtr = FindMobileDeviceHash(&MobileDeviceHash, StrPtr);
        if (MDRecPtr && MDRecPtr->MobileDevicePtr)
        {
            MobDevPtr = MDRecPtr->MobileDevicePtr;
            break;
        }
        StrPtr++;
        StrLen--;
    }
    return MobDevPtr;
}
//---------------------------------------------------------------------------
MOBILE_DEV_TYPE* GetMobileDevice(char *Name)
{
	MOBILE_DEV_TYPE *MobTypePtr = NULL;
	MOBILE_DEV_TYPE *CheckMobDevPtr = NULL;
    ObjListTask     *CurrTaskPtr = NULL;

	if (!MobileDeviceList.Count) return NULL;
	CurrTaskPtr = (ObjListTask*)MobileDeviceList.FistTask;
    while(CurrTaskPtr)
	{
		CheckMobDevPtr = (MOBILE_DEV_TYPE*)CurrTaskPtr->UsedTask;
	    if (!strcmp(CheckMobDevPtr->DevDetectLine, Name))
		{
            MobTypePtr = CheckMobDevPtr;
			break;
		}
        if (MobileDeviceList.FistTask == (ObjListTask*)(CurrTaskPtr->NextTask)) break;
        CurrTaskPtr = (ObjListTask*)(CurrTaskPtr->NextTask);
	}
	return MobTypePtr;
}
//------------------------------------------------------
