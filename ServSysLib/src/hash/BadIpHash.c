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

#ifdef _LINUX_X86_
#include <dirent.h>
#endif
#include <sys/stat.h>
#include "CommonPlatform.h"
#include "BadIpHash.h"
#include "SysWebFunction.h"
#include "WebServInfo.h"

unsigned int BadIpHashEntityCount = 0;
unsigned int BadIpRecInHashCount = 0;
unsigned int NewIpAddrIndex = 1;

BAD_IP_HASH_OCTET_HOP BadIpHashHop;
ListItsTask  BadIpList;
//---------------------------------------------------------------------------
void BadIPListLoad()
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
	unsigned int ReadRes;
	unsigned int BadIpListCount = 0;
	int IPAddr1, IPAddr2, IPAddr3, IPAddr4;
	unsigned int IPAddrToList;
	unsigned int DbTextLine;
	unsigned char *FileData;
	BAD_IP_RECORD *NewBadIpRecPtr = NULL;
	char *CwdRet = NULL;
	char DbTextItem[MAX_LEN_BAD_IP_LINE+1];
	char StartPath[512];
	char BdFileName[1024];

    BadIpList.Count = 0;
	BadIpList.CurrTask = NULL;
	BadIpList.FistTask = NULL;

	InitBadIpHash(&BadIpHashHop);
#ifdef WIN32
	CwdRet = _getcwd((char*)(&StartPath[0]),512);
#else
	CwdRet = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, BadIpDbNamePath);
#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB brand (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB list (%s) is not file\n", BdFileName);
            fclose(FileHandler);
            return;
        }
#endif
#ifdef WIN32
	HFSndMess = CreateFile((LPCWSTR)BdFileName,0,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (HFSndMess == INVALID_HANDLE_VALUE)
	{
       printf("File DB brand (%s) dos not present\n", BdFileName);
	   return;
	}
	SizeFile = GetFileSize( HFSndMess, NULL );
	CloseHandle( HFSndMess );
	FileHandler = fopen(BdFileName,"rb");
#endif
	FileData = (unsigned char*)AllocateMemory( SizeFile+2 );
	ReadRes = fread((unsigned char*)FileData, 1, SizeFile, FileHandler);
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
            if (FindCmdRequest(DbTextItem, "comment") == -1)
            {
				if ( DbTextItem[0] !='\r' && DbTextItem[0] !='\n')
		        {
					LastRot = true;
			        DbTextItem[DbTextLine] = 0;
                    /* Extract bad ip address line */
					pars_read = sscanf(&DbTextItem[0], "%d.%d.%d.%d", &IPAddr1, &IPAddr2, &IPAddr3, &IPAddr4);
					if ((pars_read == 4) &&
						(IPAddr1 < 256) && (IPAddr2 < 256) && (IPAddr3 < 256) && (IPAddr4 < 256))
					{
                    #ifdef WIN32
                        IPAddrToList = IPAddr4;
                        IPAddrToList |= (unsigned int)(IPAddr3 & 0xff) << 8;
						IPAddrToList |= (unsigned int)(IPAddr2 & 0xff) << 16;
						IPAddrToList |= (unsigned int)(IPAddr1 & 0xff) << 24;
                    #endif                    
                    #ifdef _LINUX_X86_
                      #ifdef _SUN_BUILD_
                        IPAddrToList = IPAddr4;
                        IPAddrToList |= (unsigned int)(IPAddr3 & 0xff) << 8;
						IPAddrToList |= (unsigned int)(IPAddr2 & 0xff) << 16;
						IPAddrToList |= (unsigned int)(IPAddr1 & 0xff) << 24;
                      #else
                        IPAddrToList = IPAddr1;
                        IPAddrToList |= (unsigned int)(IPAddr2 & 0xff) << 8;
						IPAddrToList |= (unsigned int)(IPAddr3 & 0xff) << 16;
						IPAddrToList |= (unsigned int)(IPAddr4 & 0xff) << 24;   
                      #endif
                    #endif
						NewBadIpRecPtr = (BAD_IP_RECORD*)AllocateMemory(sizeof(BAD_IP_RECORD));
						NewBadIpRecPtr->IpAddr = IPAddrToList;
						NewBadIpRecPtr->TryAccessCount = 0;
						if (AddBadIpHash(&BadIpHashHop, NewBadIpRecPtr))
						{
						    NewBadIpRecPtr->ObjPtr = AddStructListObj(&BadIpList, NewBadIpRecPtr);
							NewBadIpRecPtr->IpAddrId = NewIpAddrIndex++;
							BadIpListCount++;
						}
						else
						{
							FreeMemory(NewBadIpRecPtr);
						}
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
			   if (DbTextLine < MAX_LEN_BAD_IP_LINE)
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
	printf("Bad IP list load is done (Bad IPs: %d)\n", BadIpListCount);
}
//---------------------------------------------------------------------------
bool isIpInBadList(unsigned int TestIpAddr)
{
	bool               Result = false;
	BAD_IP_HASH_RECORD *BadIpRecPtr = NULL;

    BadIpRecPtr = FindBadIpHash(&BadIpHashHop, TestIpAddr);
	if (BadIpRecPtr)
	{
		BadIpRecPtr->BadIp->TryAccessCount++;
		Result = true;
	}
	return Result;
}
//---------------------------------------------------------------------------
void BadIPListClear()
{
    ObjListTask	 *SelObjPtr = NULL;
    BAD_IP_RECORD *SelBadIpRecPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&BadIpList);
	while(SelObjPtr)
	{
	    SelBadIpRecPtr = (BAD_IP_RECORD*)SelObjPtr->UsedTask;
		FreeMemory(SelBadIpRecPtr);
		RemStructList(&BadIpList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&BadIpList);
	}
    CloseBadIpHash(&BadIpHashHop);
}
//---------------------------------------------------------------------------
void InitBadIpHash(BAD_IP_HASH_OCTET_HOP *NameHop)
{
    unsigned int i;
	
	for(i=0;i < MAX_OCTET_HASH_INDEX;i++) 
	   NameHop->HashOctetHop[i] = NULL;
	NameHop->ParentHashOctetPtr = NULL;
    NameHop->Record = NULL;
    NameHop->UsedOctetCount = 0;
    NameHop->ParentHashOctetIndex = 0;	
}
//---------------------------------------------------------------------------
bool AddBadIpHash(BAD_IP_HASH_OCTET_HOP *RootNameHop, BAD_IP_RECORD *BadIp)
{
    bool Result = true;
	unsigned long IpOctetMask = 0x0f;
    unsigned int OctetIndex, HopIndex;
	BAD_IP_HASH_OCTET_HOP *NewHashHopOctetPtr = NULL;
	BAD_IP_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	BAD_IP_HASH_RECORD   *NewRecordPtr = NULL;
	
	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 8;OctetIndex++)
	{
	   HopIndex = (unsigned int)((BadIp->IpAddr & IpOctetMask) >> (OctetIndex << 2));
	   if (!SelHashHopOctetPtr->HashOctetHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopOctetPtr = (BAD_IP_HASH_OCTET_HOP*)AllocateMemory(sizeof(BAD_IP_HASH_OCTET_HOP));
		   if (!NewHashHopOctetPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitBadIpHash(NewHashHopOctetPtr);
		   SelHashHopOctetPtr->HashOctetHop[HopIndex] = NewHashHopOctetPtr;
		   SelHashHopOctetPtr->UsedOctetCount++;
		   NewHashHopOctetPtr->ParentHashOctetPtr = (void*)SelHashHopOctetPtr;
		   NewHashHopOctetPtr->ParentHashOctetIndex = HopIndex;
		   SelHashHopOctetPtr = NewHashHopOctetPtr;
		   BadIpHashEntityCount++;
	   }
	   else
	   {
	       /* Existing char in hash for this hop is detected */
		   SelHashHopOctetPtr = (BAD_IP_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[HopIndex];
	   }
       IpOctetMask <<= 4;
	}
	if (Result)
	{
		if (SelHashHopOctetPtr->Record)
		{
			printf("In cashe of bad IPs %08x is already present\n", BadIp->IpAddrId);
            return false;
		}
	    /* Sesion Ip Addr hash fillout */
		NewRecordPtr = (BAD_IP_HASH_RECORD*)AllocateMemory(sizeof(BAD_IP_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopOctetPtr->Record = NewRecordPtr;
			NewRecordPtr->HashOctetHopPtr = SelHashHopOctetPtr;
			NewRecordPtr->BadIp = BadIp;
			BadIpRecInHashCount++;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
BAD_IP_HASH_RECORD* FindBadIpHash(BAD_IP_HASH_OCTET_HOP *RootNameHop,
										  unsigned int BadIp)
{
    bool Result = true;
	unsigned int IpOctetMask = 0x0f;
    unsigned int OctetIndex, HopIndex;
	BAD_IP_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	BAD_IP_HASH_RECORD   *SelRecordPtr = NULL;

	SelHashHopOctetPtr = RootNameHop;
    for (OctetIndex=0;OctetIndex < 8;OctetIndex++)
	{
	    HopIndex = (unsigned int)((BadIp & IpOctetMask) >> (OctetIndex << 2));
	    if (!SelHashHopOctetPtr->HashOctetHop[HopIndex])
	    {
	        Result = false;
            break;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopOctetPtr = (BAD_IP_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[HopIndex];
	    }
		IpOctetMask <<= 4;
	}
	if (!Result) return NULL;
	return SelHashHopOctetPtr->Record;
}
//---------------------------------------------------------------------------
bool RemBadIpHash(BAD_IP_HASH_OCTET_HOP *RootNameHop, unsigned int BadIp)
{
    BAD_IP_HASH_RECORD *BadIpRecPtr = NULL;
	BAD_IP_HASH_OCTET_HOP *ParentHashHopOctetPtr = NULL;
	BAD_IP_HASH_OCTET_HOP *SelHashHopOctetPtr = NULL;
	
    BadIpRecPtr = FindBadIpHash(RootNameHop, BadIp);
	if (!BadIpRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopOctetPtr = (BAD_IP_HASH_OCTET_HOP*)BadIpRecPtr->HashOctetHopPtr;
	if (BadIpRecInHashCount > 0) BadIpRecInHashCount--;
	FreeMemory(BadIpRecPtr);
	SelHashHopOctetPtr->Record = NULL;
	while(!SelHashHopOctetPtr || (SelHashHopOctetPtr != RootNameHop))
	{
	    if (!SelHashHopOctetPtr->UsedOctetCount)
	    {
	        ParentHashHopOctetPtr = (BAD_IP_HASH_OCTET_HOP*)SelHashHopOctetPtr->ParentHashOctetPtr;
		    ParentHashHopOctetPtr->HashOctetHop[SelHashHopOctetPtr->ParentHashOctetIndex] = NULL;
		    FreeMemory(SelHashHopOctetPtr);
			BadIpHashEntityCount--;
		    ParentHashHopOctetPtr->UsedOctetCount--;
			SelHashHopOctetPtr = ParentHashHopOctetPtr;
	    }
	    else
	    {
	        break;
	    }
	}
	return true;
}
//---------------------------------------------------------------------------
void CloseBadIpHash(BAD_IP_HASH_OCTET_HOP *RootNameHop)
{
	CloseBadIpHashHop(RootNameHop);
}
//---------------------------------------------------------------------------
void CloseBadIpHashHop(BAD_IP_HASH_OCTET_HOP *SelHashHopOctetPtr)
{
	unsigned int index;
	BAD_IP_HASH_OCTET_HOP *NextHashHopOctetPtr = NULL;

	if (SelHashHopOctetPtr->UsedOctetCount)
	{
		for(index=0;index < MAX_OCTET_HASH_INDEX;index++)
		{
			if (SelHashHopOctetPtr->HashOctetHop[index])
			{
                NextHashHopOctetPtr = (BAD_IP_HASH_OCTET_HOP*)SelHashHopOctetPtr->HashOctetHop[index];
				if (!NextHashHopOctetPtr->UsedOctetCount)
				{
					if (NextHashHopOctetPtr->Record)
                        FreeMemory(NextHashHopOctetPtr->Record);
					FreeMemory(NextHashHopOctetPtr);
					SelHashHopOctetPtr->HashOctetHop[index] = NULL;
					SelHashHopOctetPtr->UsedOctetCount--;
					if (!SelHashHopOctetPtr->UsedOctetCount) break;
				}
				else
				{
                    CloseBadIpHashHop(NextHashHopOctetPtr);
					if (!NextHashHopOctetPtr->UsedOctetCount)
					{
					    if (NextHashHopOctetPtr->Record)
                            FreeMemory(NextHashHopOctetPtr->Record);
					    FreeMemory(NextHashHopOctetPtr);
					    SelHashHopOctetPtr->HashOctetHop[index] = NULL;
					    SelHashHopOctetPtr->UsedOctetCount--;
					    if (!SelHashHopOctetPtr->UsedOctetCount) break;
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
BAD_IP_RECORD* BadIpDbAddItem()
{
	BAD_IP_RECORD *BadIpRecPtr = NULL;

	BadIpRecPtr = (BAD_IP_RECORD*)AllocateMemory(sizeof(BAD_IP_RECORD));
	if (!BadIpRecPtr) return NULL;
	BadIpRecPtr->IpAddr = 0x01010101;
	BadIpRecPtr->TryAccessCount = 0;
	if (AddBadIpHash(&BadIpHashHop, BadIpRecPtr))
	{
	    BadIpRecPtr->ObjPtr = AddStructListObj(&BadIpList, BadIpRecPtr);
		BadIpRecPtr->IpAddrId = NewIpAddrIndex++;
	}
	else
	{
		FreeMemory(BadIpRecPtr);
		BadIpRecPtr = NULL;
	}
	return BadIpRecPtr;

}
//---------------------------------------------------------------------------
void BadIpDbSave()
{
    ObjListTask	 *SelObjPtr = NULL;
    BAD_IP_RECORD *SelBadIpRecPtr = NULL;
	FILE   *FileHandler;
	char *CwdRet = NULL;
	char StartPath[512];
	char BdFileName[1024];
	char IpAddrBuf[32];
	char BadIpCfgLine[128];

#ifdef WIN32
	CwdRet = _getcwd((char*)(&StartPath[0]),512);
#else
	CwdRet = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, BadIpDbNamePath);
	FileHandler = fopen(BdFileName,"wb");
	if (!FileHandler) 
	{
        printf("Failed to open (%s) bad IP DB file for write\n", BdFileName);
	    return;
	}
	SelObjPtr = (ObjListTask*)GetFistObjectList(&BadIpList);
	while(SelObjPtr)
	{
	    SelBadIpRecPtr = (BAD_IP_RECORD*)SelObjPtr->UsedTask;
        SetIpAddrToString(IpAddrBuf, SelBadIpRecPtr->IpAddr);
		sprintf(BadIpCfgLine, "%s\r\n", IpAddrBuf);
		fwrite(BadIpCfgLine, strlen(BadIpCfgLine), 1, FileHandler);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&BadIpList);
	}
    fclose(FileHandler);
}
//---------------------------------------------------------------------------
BAD_IP_RECORD* GetBadIpRecById(unsigned int BadIpId)
{
    ObjListTask	 *SelObjPtr = NULL;
    BAD_IP_RECORD *SelBadIpRecPtr = NULL;
	BAD_IP_RECORD *FindBadIpRecPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&BadIpList);
	while(SelObjPtr)
	{
	    SelBadIpRecPtr = (BAD_IP_RECORD*)SelObjPtr->UsedTask;
		if (SelBadIpRecPtr->IpAddrId == BadIpId)
		{
			FindBadIpRecPtr = SelBadIpRecPtr;
			break;
		}
		SelObjPtr = (ObjListTask*)GetNextObjectList(&BadIpList);
	}
	return FindBadIpRecPtr;
}
//---------------------------------------------------------------------------
