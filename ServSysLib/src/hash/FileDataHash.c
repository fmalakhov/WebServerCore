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
#include "FileDataHash.h"
#include "SysWebFunction.h"
#include "gzip.h"
#include "ThrReportMen.h"

unsigned int NextEtag = 1000000;
unsigned int HashEntityCount = 0;
unsigned int RecInHashCount = 0;
unsigned int UpdateRecInHashCount = 0;

FILE_HASH_CHAR_HOP FileHashHop;

static char *TablZipReadyExtFiles[] = {".html", ".css", ".js", ".txt", ".xml", ".csv"};


unsigned char AsciiFileCharToIndex[] ={
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   3,  68,  69,
   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,   0,   0,   0,   0,   0,   0,
   0,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,   0,  70,   0,   0,  40,
   0,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,
  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };
//---------------------------------------------------------------------------
void InitFileHash(FILE_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(FILE_HASH_CHAR_HOP));	
}
//---------------------------------------------------------------------------
bool AddFileHash(FILE_HASH_CHAR_HOP *RootNameHop, char *FileName, char *HashKey)
{
    bool Result = true;
    unsigned int       i, CharIndex, HopIndex, NameLen;
	unsigned char      *ComprDataBufPtr = NULL;
	FILE_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	FILE_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	FILE_HASH_RECORD   *NewRecordPtr = NULL;
	FILE			   *HandelFilePtr;	
	int				   BlockRead;
	GZIP_TASK_INFO     ZipInfo;
#ifdef WIN32
	HANDLE		       HDIRFILE;
	WIN32_FIND_DATA    Data_File;
#else
    DIR                *DirPtr;
    struct dirent      *DirEntPtr;
    struct stat        st;
#endif

	NameLen = strlen(HashKey);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	   HopIndex = AsciiFileCharToIndex[(unsigned char)HashKey[CharIndex]];
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopCharPtr = (FILE_HASH_CHAR_HOP*)AllocateMemory(sizeof(FILE_HASH_CHAR_HOP));
		   if (!NewHashHopCharPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitFileHash(NewHashHopCharPtr);
		   SelHashHopCharPtr->HashCharHop[HopIndex] = NewHashHopCharPtr;
		   SelHashHopCharPtr->UsedCharCount++;
		   NewHashHopCharPtr->ParentHashCharPtr = (void*)SelHashHopCharPtr;
		   NewHashHopCharPtr->ParentHashCharIndex = HopIndex;
		   SelHashHopCharPtr = NewHashHopCharPtr;
		   HashEntityCount++;
	   }
	   else
	   {
	       /* Existing char in hash for this hop is detected */
		   SelHashHopCharPtr = (FILE_HASH_CHAR_HOP*)SelHashHopCharPtr->HashCharHop[HopIndex];
		   if (!SelHashHopCharPtr)
		   {
			   Result = false;
			   break;
		   }
	   }
	}

	if (Result)
	{
		if (SelHashHopCharPtr->Record)
		{
			printf("In cashe of web files file (%s) already present\n", FileName);
            return false;
		}
	    /* File hash fillout */
		NewRecordPtr = (FILE_HASH_RECORD*)AllocateMemory(sizeof(FILE_HASH_RECORD));
		memset(NewRecordPtr, 0, sizeof(FILE_HASH_RECORD));
		if (!NewRecordPtr)
		{	
			printf("Fail to memory allocation for file hash record\n");
		    Result = false;
		}
		else
		{
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
#ifdef WIN32        
	        HDIRFILE = FindFirstFile((LPCWSTR)FileName, &Data_File);
	        if (HDIRFILE != INVALID_HANDLE_VALUE)
            {
                NewRecordPtr->FileLen = ((unsigned long)Data_File.nFileSizeHigh)<<16;
		        NewRecordPtr->FileLen += (unsigned long)Data_File.nFileSizeLow;
                NewRecordPtr->FileBodyBuf = (unsigned char*)AllocateMemory((NewRecordPtr->FileLen+1)*sizeof(unsigned char));
				if (NewRecordPtr->FileBodyBuf)
				{
				    *NewRecordPtr->FileBodyBuf = 0;
				    HandelFilePtr = fopen(FileName, "rb");
				    BlockRead = fread((unsigned char*)&NewRecordPtr->FileBodyBuf[0], 1, 
					    NewRecordPtr->FileLen, HandelFilePtr);
	                fclose(HandelFilePtr);
                    NewRecordPtr->FileBodyBuf[NewRecordPtr->FileLen] = 0;
				    NewRecordPtr->FileEtag = NextEtag++;
					memcpy(&NewRecordPtr->LastUpdate, &Data_File.ftLastWriteTime, sizeof(FILETIME));
					*NewRecordPtr->LastUpdateStr = 0;
					/* Check file extention for gzip compression */
		            for (i=0;i < (sizeof(TablZipReadyExtFiles)/sizeof(char*));i++)
		            {
			            if  (FindCmdRequest(FileName, TablZipReadyExtFiles[i]) != -1)
			            {
				            /* File is gzip complined - should be compressed */
							ComprDataBufPtr = (unsigned char*)AllocateMemory((NewRecordPtr->FileLen+256)*sizeof(unsigned char));
							if (ComprDataBufPtr)
							{
							    NewRecordPtr->ZipFileLen = zip(&ZipInfo, FileName, false, (unsigned int)NewRecordPtr->FileLen, 
								    (unsigned char*)&NewRecordPtr->FileBodyBuf[0], ComprDataBufPtr);
								NewRecordPtr->ZipFileBodyBuf = (unsigned char*)AllocateMemory((NewRecordPtr->ZipFileLen+1)*sizeof(unsigned char));
								if (NewRecordPtr->ZipFileBodyBuf)
								{
								    memcpy(NewRecordPtr->ZipFileBodyBuf, ComprDataBufPtr, NewRecordPtr->ZipFileLen);
								}
								else
								{
								    printf("Fail to memory allocation for compressed file body\n");
								    FreeMemory(NewRecordPtr->FileBodyBuf);
				                    FreeMemory(NewRecordPtr);
				                    SelHashHopCharPtr->Record = NULL;
				                    Result = false;	
								}
								FreeMemory(ComprDataBufPtr);
							}
							else
							{
								printf("Fail to memory allocation for file body compress result\n");
								FreeMemory(NewRecordPtr->FileBodyBuf);
				                FreeMemory(NewRecordPtr);
				                SelHashHopCharPtr->Record = NULL;
				                Result = false;	
							}
				            break;
			            }
		            }
				}
				else
				{
				    /* Memory allocation for file body cashe is failed */
				    FreeMemory(NewRecordPtr);
				    SelHashHopCharPtr->Record = NULL;
				    Result = false;					
				}
				FindClose(HDIRFILE);
			}
			else
			{			
			    /* Requested file does not present in system */
				FreeMemory(NewRecordPtr);
				NewRecordPtr = NULL;
				SelHashHopCharPtr->Record = NULL;
				Result = false;
			}
#else   
            if (stat(FileName, &st) != -1)
			{
                NewRecordPtr->FileLen = (unsigned long)st.st_size;
                NewRecordPtr->FileBodyBuf = (unsigned char*)AllocateMemory((NewRecordPtr->FileLen+1)*sizeof(unsigned char));
			    if (NewRecordPtr->FileBodyBuf)
			    {
				    *NewRecordPtr->FileBodyBuf = 0;
				    HandelFilePtr = fopen(FileName, "rb");
				    BlockRead = fread((unsigned char*)&NewRecordPtr->FileBodyBuf[0], 1, 
					    NewRecordPtr->FileLen, HandelFilePtr);
					if (BlockRead)
					{
	                fclose(HandelFilePtr);
					NewRecordPtr->FileBodyBuf[NewRecordPtr->FileLen] = 0;
				    NewRecordPtr->FileEtag = NextEtag++;
					NewRecordPtr->LastUpdate = st.st_mtime;
					SetLastModifyedDate(NewRecordPtr->LastUpdateStr, NewRecordPtr->LastUpdate);

					/* Check file extention for gzip compression */
		            for (i=0;i < (sizeof(TablZipReadyExtFiles)/sizeof(char*));i++)
		            {
			            if (FindCmdRequest(FileName, TablZipReadyExtFiles[i]) != -1)
			            {
				            /* File is gzip complined - should be compressed */
							ComprDataBufPtr = (unsigned char*)AllocateMemory((NewRecordPtr->FileLen+256)*sizeof(unsigned char));
							if (ComprDataBufPtr)
							{
							    NewRecordPtr->ZipFileLen = zip(&ZipInfo, FileName, false, (unsigned int)NewRecordPtr->FileLen, 
								    (unsigned char*)&NewRecordPtr->FileBodyBuf[0], ComprDataBufPtr);
								NewRecordPtr->ZipFileBodyBuf = (unsigned char*)AllocateMemory((NewRecordPtr->ZipFileLen+1)*sizeof(unsigned char));
								if (NewRecordPtr->ZipFileLen)
								{
								    memcpy(NewRecordPtr->ZipFileBodyBuf, ComprDataBufPtr, NewRecordPtr->ZipFileLen);
								}
								else
								{
								    printf("Fail to memory allocation for compressed file body\n");
								    FreeMemory(NewRecordPtr->FileBodyBuf);
				                    FreeMemory(NewRecordPtr);
				                    SelHashHopCharPtr->Record = NULL;
				                    Result = false;	
								}
								FreeMemory(ComprDataBufPtr);
							}
							else
							{
								printf("Fail to memory allocation for file body compress result\n");
								FreeMemory(NewRecordPtr->FileBodyBuf);
				                FreeMemory(NewRecordPtr);
				                SelHashHopCharPtr->Record = NULL;
				                Result = false;	
							}
				            break;
			            }
		            }
					}
					else
					{
						printf("Fail to read %s file\n", FileName);
						FreeMemory(NewRecordPtr->FileBodyBuf);
				        FreeMemory(NewRecordPtr);
				        SelHashHopCharPtr->Record = NULL;
				        Result = false;	
					}
			    }
			    else
			    {
				    /* Memory allocation for file body cashe is failed */
				    if (NewRecordPtr) FreeMemory(NewRecordPtr);
					NewRecordPtr = NULL;
				    SelHashHopCharPtr->Record = NULL;
				    Result = false;					
			    }
            }			
			else
			{
			    /* Requested file does not present in system */
				if (NewRecordPtr) FreeMemory(NewRecordPtr);
				NewRecordPtr = NULL;
				SelHashHopCharPtr->Record = NULL;
				Result = false;			
			}
#endif
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
FILE_HASH_RECORD* FindFileHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType, char *FileName)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	char *NameOnlyPtr = NULL;
	FILE_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	FILE_HASH_RECORD   *SelRecordPtr = NULL;

	if (DirType == BASE_HASH_LIST)
	{
		/* Name file only extract */
		NameLen = strlen(FileName);
		for (CharIndex=NameLen;CharIndex > 0;CharIndex--)
		{
		  #ifdef _LINUX_X86_
			if (FileName[CharIndex-1] == '/')
		  #endif
		  #ifdef WIN32
			if (FileName[CharIndex-1] == '\\')
		  #endif
			{
				NameOnlyPtr = &FileName[CharIndex];
				NameLen = strlen(NameOnlyPtr);
				break;
			}
		}
		if (!NameOnlyPtr) return NULL;
	}
	else
	{
	    NameOnlyPtr = FileName;
		NameLen = strlen(FileName);
    }

	SelHashHopCharPtr = RootNameHop;
	switch(DirType)
	{
		case PRIM_CONTENT_LIST:
			HopIndex = AsciiFileCharToIndex[(unsigned char)'P'];
			break;

		case SECOND_CONTENT_LIST:
			HopIndex = AsciiFileCharToIndex[(unsigned char)'S'];
			break;

		default:
			HopIndex = AsciiFileCharToIndex[(unsigned char)'B'];
			break;
	}
	if (!SelHashHopCharPtr->HashCharHop[HopIndex]) return NULL;
	SelHashHopCharPtr = (FILE_HASH_CHAR_HOP*)SelHashHopCharPtr->HashCharHop[HopIndex];

    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	    HopIndex = AsciiFileCharToIndex[(unsigned char)NameOnlyPtr[CharIndex]];
	    if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	    {
	        Result = false;
            break;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopCharPtr = SelHashHopCharPtr->HashCharHop[HopIndex];
	    }
	}
	if (!Result) return NULL;
	return SelHashHopCharPtr->Record;
}
//---------------------------------------------------------------------------
bool RemFileHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType, char *FileName)
{
    FILE_HASH_RECORD *FileRecPtr = NULL;
	FILE_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	FILE_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    FileRecPtr = FindFileHash(RootNameHop, DirType, FileName);
	if (!FileRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (FILE_HASH_CHAR_HOP*)FileRecPtr->HashCharHopPtr;
	if (FileRecPtr->ZipFileBodyBuf) FreeMemory(FileRecPtr->ZipFileBodyBuf);
    if (FileRecPtr->FileBodyBuf) FreeMemory(FileRecPtr->FileBodyBuf);
	FreeMemory(FileRecPtr);
	SelHashHopCharPtr->Record = NULL;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (FILE_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
		    ParentHashHopCharPtr->HashCharHop[SelHashHopCharPtr->ParentHashCharIndex] = NULL;
		    FreeMemory(SelHashHopCharPtr);
			HashEntityCount--;
		    ParentHashHopCharPtr->UsedCharCount--;
			SelHashHopCharPtr = ParentHashHopCharPtr;
	    }
	    else
	    {
	        break;
	    }
	}
	return true;
}
//---------------------------------------------------------------------------
void CloseFileHash(FILE_HASH_CHAR_HOP *RootNameHop)
{
	CloseFileHashHop(RootNameHop);
}
//---------------------------------------------------------------------------
void CloseFileHashHop(FILE_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	FILE_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;
    FILE_HASH_RECORD *FileRecPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MAX_FDH_CHAR_HASH_INDEX;index++)
		{
			if (SelHashHopCharPtr->HashCharHop[index])
			{
                NextHashHopCharPtr = (FILE_HASH_CHAR_HOP*)SelHashHopCharPtr->HashCharHop[index];
				if (!NextHashHopCharPtr->UsedCharCount)
				{
					if (NextHashHopCharPtr->Record)
					{
						FileRecPtr = NextHashHopCharPtr->Record;
	                    if (FileRecPtr->ZipFileBodyBuf) FreeMemory(FileRecPtr->ZipFileBodyBuf);
                        if (FileRecPtr->FileBodyBuf) FreeMemory(FileRecPtr->FileBodyBuf);
                        FreeMemory(FileRecPtr);
					}
					FreeMemory(NextHashHopCharPtr);
					SelHashHopCharPtr->HashCharHop[index] = NULL;
					SelHashHopCharPtr->UsedCharCount--;
					if (!SelHashHopCharPtr->UsedCharCount) break;
				}
				else
				{
                    CloseFileHashHop(NextHashHopCharPtr);
					if (!NextHashHopCharPtr->UsedCharCount)
					{
					    if (NextHashHopCharPtr->Record)
					    {
						    FileRecPtr = NextHashHopCharPtr->Record;
	                        if (FileRecPtr->ZipFileBodyBuf) FreeMemory(FileRecPtr->ZipFileBodyBuf);
                            if (FileRecPtr->FileBodyBuf) FreeMemory(FileRecPtr->FileBodyBuf);
                            FreeMemory(FileRecPtr);
					    }
					    FreeMemory(NextHashHopCharPtr);
					    SelHashHopCharPtr->HashCharHop[index] = NULL;
					    SelHashHopCharPtr->UsedCharCount--;
					    if (!SelHashHopCharPtr->UsedCharCount) break;
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void AddFilesDirHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType, 
	char *BaseDirPtr, char *DirNamePtr)
{
	char		  *PathSearth = NULL;
    char          *FileNamePtr = NULL;
	char          *HashKey = NULL;
	char          *PathHift = NULL;
#ifdef WIN32
	unsigned int  FileSize;
	HANDLE		  HDIRFILE;
	WIN32_FIND_DATA Data_File;
#endif
#ifdef _LINUX_X86_
    DIR           *DirPtr;
    struct dirent *DirEntPtr;
    struct stat   st;
#endif

	PathSearth = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathSearth) return;
	HashKey = (char*)AllocateMemory(MAX_LEN_PATH_NAME*sizeof(char));
	if (!HashKey)
	{
		FreeMemory(PathSearth);
		return;
	}
	strcpy(PathSearth, BaseDirPtr);
	strcat(PathSearth, DirNamePtr);
#ifdef WIN32     
    FileNamePtr = &PathSearth[strlen(PathSearth)];      
	strcpy(FileNamePtr, "\\*.*");	
	HDIRFILE = FindFirstFile((LPCWSTR)PathSearth, &Data_File);
	if (HDIRFILE != INVALID_HANDLE_VALUE)
    {
        while ( 1 )
	    {
            if ( !(Data_File.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) )
		    {
                FileSize = ((unsigned int)Data_File.nFileSizeHigh)<<16;
		        FileSize += (unsigned int)Data_File.nFileSizeLow;
				if ((FileSize < MAX_ADD_CASHE_FILE_LEN) &&
                    (FindCmdRequest(Data_File.cFileName, ".backup") == -1) &&
					(FindCmdRequest(Data_File.cFileName, "Thumbs.db") == -1))
				{
					strcpy(FileNamePtr, "\\");
					switch(DirType)
					{
						case PRIM_CONTENT_LIST:
							strcpy(HashKey, "P");
							PathHift = DirNamePtr;
							if (*PathHift == '\\')
							{
							    PathHift++;
								if (strlen(PathHift) > 0)
								{
							        strcat(HashKey, PathHift);
							        strcat(HashKey, "\\");
								}
							}
							break;

						case SECOND_CONTENT_LIST:
							strcpy(HashKey, "S");
							PathHift = DirNamePtr;
							if (*PathHift == '\\')
							{
								PathHift++;
								if (strlen(PathHift) > 0)
								{
							        strcat(HashKey, PathHift);
							        strcat(HashKey, "\\");
								}
							}
							break;

						default:
							strcpy(HashKey, "B");
							break;
					}
		            strcat(FileNamePtr, (char*)&Data_File.cFileName);
					strcat(HashKey, (char*)&Data_File.cFileName);
				    if (AddFileHash(RootNameHop, PathSearth, HashKey))
					{
					    RecInHashCount++;
					}
				}
		    }
		    if (!FindNextFile(HDIRFILE,&Data_File) &&
		        (GetLastError() == ERROR_NO_MORE_FILES)) break;
	    }
        FindClose(HDIRFILE);
    }
#else    
    DirPtr = opendir(PathSearth);
	if (DirPtr)
    {
        FileNamePtr = &PathSearth[strlen(PathSearth)];
        while ((DirEntPtr = readdir(DirPtr)) != NULL)
	    {
			strcpy(FileNamePtr, "/");
		    strcat(FileNamePtr, (char*)DirEntPtr->d_name);
            stat(PathSearth, &st);            
            if (((st.st_mode & S_IFMT) != S_IFDIR) &&
                (strcmp(DirEntPtr->d_name, ".") != 0) &&
                (strcmp(DirEntPtr->d_name, "..") != 0) &&
				((unsigned int)st.st_size < MAX_ADD_CASHE_FILE_LEN) &&
                (FindCmdRequest(DirEntPtr->d_name, ".backup") == -1) &&
				(FindCmdRequest(DirEntPtr->d_name, "Thumbs.db") == -1))
		    {
				switch(DirType)
				{
					case PRIM_CONTENT_LIST:
						strcpy(HashKey, "P");
						PathHift = DirNamePtr;
						if (*PathHift == '/')
						{
						    PathHift++;
							if (strlen(PathHift) > 0)
							{
						        strcat(HashKey, PathHift);
						        strcat(HashKey, "/");
							}
						}
						break;

					case SECOND_CONTENT_LIST:
						strcpy(HashKey, "S");
						PathHift = DirNamePtr;
						if (*PathHift == '/')
						{
						    PathHift++;
							if (strlen(PathHift) > 0)
							{
						        strcat(HashKey, PathHift);
						        strcat(HashKey, "/");
							}
						}
						break;

					default:
						strcpy(HashKey, "B");
						break;
				}
				strcat(HashKey, (char*)DirEntPtr->d_name);
                if (AddFileHash(RootNameHop, PathSearth, HashKey))
				{
				    RecInHashCount++;
				}
		    }
	    }
        closedir(DirPtr);
    }
#endif
	FreeMemory(HashKey);
	FreeMemory(PathSearth);
	return;
}
//---------------------------------------------------------------------------
void AddSingleFileHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType, 
	char *BaseDirPtr, char *DirNamePtr)
{
	char		  *PathSearth = NULL;
    char          *FileNamePtr = NULL;
	char          *HashKey = NULL;

	PathSearth = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathSearth) return;
	HashKey = (char*)AllocateMemory(MAX_LEN_PATH_NAME*sizeof(char));
	if (!HashKey)
	{
		FreeMemory(PathSearth);
		return;
	}
	strcpy(PathSearth, BaseDirPtr);
	strcat(PathSearth, DirNamePtr);
#ifdef WIN32 
	switch(DirType)
	{
		case PRIM_CONTENT_LIST:
			strcpy(HashKey, "P");
			break;

		case SECOND_CONTENT_LIST:
			strcpy(HashKey, "S");
			break;

		default:
			strcpy(HashKey, "B");
			break;
	}
#else    
	switch(DirType)
	{
		case PRIM_CONTENT_LIST:
			strcpy(HashKey, "P");
			break;

		case SECOND_CONTENT_LIST:
			strcpy(HashKey, "S");
			break;

		default:
			strcpy(HashKey, "B");
			break;
	}
#endif
	if (*DirNamePtr == '/') strcat(HashKey, &DirNamePtr[1]);
	else                    strcat(HashKey, DirNamePtr);
    if (AddFileHash(RootNameHop, PathSearth, HashKey))
	{
		RecInHashCount++;
	}
	FreeMemory(HashKey);
	FreeMemory(PathSearth);
	return;
}
//---------------------------------------------------------------------------
void UpdateFilesDirHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType,
	char *BaseDirPtr, char *DirNamePtr)
{
	char		     *PathSearth = NULL;
    char             *FileNamePtr = NULL;
	FILE_HASH_RECORD *FileRecPtr = NULL;
	FILE			 *HandelFilePtr;
	int				 i, BlockRead;
	unsigned char    *ComprDataBufPtr = NULL;
	char             *HashKey = NULL;
	char             *PathHift = NULL;
	GZIP_TASK_INFO   ZipInfo;
#ifdef WIN32
	unsigned int     FileSize;
	HANDLE		     HDIRFILE;
	WIN32_FIND_DATA  Data_File;
#endif
#ifdef _LINUX_X86_
    DIR              *DirPtr;
    struct dirent    *DirEntPtr;
    struct stat      st;
#endif

	PathSearth = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathSearth) return;
	HashKey = (char*)AllocateMemory(MAX_LEN_PATH_NAME*sizeof(char));
	if (!HashKey)
	{
		FreeMemory(PathSearth);
		return;
	}
	strcpy(PathSearth, DirNamePtr);
#ifdef WIN32     
    FileNamePtr = &PathSearth[strlen(PathSearth)];      
	strcpy(FileNamePtr, "\\*.*");	
	HDIRFILE = FindFirstFile((LPCWSTR)PathSearth, &Data_File);
	if (HDIRFILE != INVALID_HANDLE_VALUE)
    {
        while ( 1 )
	    {
            if ( !(Data_File.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) )
		    {
                FileSize = ((unsigned int)Data_File.nFileSizeHigh)<<16;
		        FileSize += (unsigned int)Data_File.nFileSizeLow;
				if ((FileSize < MAX_ADD_CASHE_FILE_LEN) &&
                    (FindCmdRequest(Data_File.cFileName, ".backup") == -1) &&
					(FindCmdRequest(Data_File.cFileName, "Thumbs.db") == -1))
				{
					strcpy(FileNamePtr, "\\");
					switch(DirType)
					{
						case PRIM_CONTENT_LIST:
							strcpy(HashKey, "P");
						    PathHift = DirNamePtr;
						    if (*PathHift == '/')
						    {
						        PathHift++;
							    if (strlen(PathHift) > 0)
							    {
						            strcat(HashKey, PathHift);
						            strcat(HashKey, "/");
							    }
						    }
							break;

						case SECOND_CONTENT_LIST:
							strcpy(HashKey, "S");
						    PathHift = DirNamePtr;
						    if (*PathHift == '/')
						    {
						        PathHift++;
							    if (strlen(PathHift) > 0)
							    {
						            strcat(HashKey, PathHift);
						            strcat(HashKey, "/");
							    }
						    }
							break;

						default:
							strcpy(HashKey, "B");
							break;
					}
		            strcat(FileNamePtr, (char*)&Data_File.cFileName);
					strcat(HashKey, (char*)&Data_File.cFileName);
                    FileRecPtr = FindFileHash(RootNameHop, DirType, PathSearth);
					if (!FileRecPtr)
					{
				        if (AddFileHash(RootNameHop, PathSearth))
						{
					        RecInHashCount++;
						}
					}
					else
					{
						if (CompareFileTime(&FileRecPtr->LastUpdate, &Data_File.ftLastWriteTime) != 0)
						{
							/* File was changed and needs to be re-load */
				            FreeMemory(FileRecPtr->FileBodyBuf);
                            FileRecPtr->FileLen = ((unsigned long)Data_File.nFileSizeHigh)<<16;
		                    FileRecPtr->FileLen += (unsigned long)Data_File.nFileSizeLow;
                            FileRecPtr->FileBodyBuf = (unsigned char*)AllocateMemory((FileRecPtr->FileLen+1)*sizeof(unsigned char));
				            if (FileRecPtr->FileBodyBuf)
							{
				                FileRecPtr->FileBodyBuf[0] = 0;
				                HandelFilePtr = fopen(PathSearth, "rb");
				                BlockRead = fread((unsigned char*)&FileRecPtr->FileBodyBuf[0], 1, 
					                FileRecPtr->FileLen, HandelFilePtr);
	                            fclose(HandelFilePtr);
                                FileRecPtr->FileBodyBuf[FileRecPtr->FileLen] = 0;
					            memcpy(&FileRecPtr->LastUpdate, &Data_File.ftLastWriteTime, sizeof(FILETIME));
								*FileRecPtr->LastUpdateStr = 0;
							}
				            else
							{
				                /* Memory allocation for file body cashe is failed */
								printf("Fail to allocate memory for file hash (%s)\n", PathSearth);
                            }
						}
					}
				}
		    }
		    if (!FindNextFile(HDIRFILE,&Data_File) &&
		        (GetLastError() == ERROR_NO_MORE_FILES)) break;
	    }
        FindClose(HDIRFILE);
    }
#endif
#ifdef _LINUX_X86_     
    DirPtr = opendir(PathSearth);
	if (DirPtr)
    {
        FileNamePtr = &PathSearth[strlen(PathSearth)];
        while ((DirEntPtr = readdir(DirPtr)) != NULL)
	    {
			strcpy(FileNamePtr, "/");
		    strcat(FileNamePtr, (char*)DirEntPtr->d_name);
            stat(PathSearth, &st);            
            if (((st.st_mode & S_IFMT) != S_IFDIR) &&
                (strcmp(DirEntPtr->d_name, ".") != 0) &&
                (strcmp(DirEntPtr->d_name, "..") != 0) &&
				((unsigned int)st.st_size < MAX_ADD_CASHE_FILE_LEN) &&
                (FindCmdRequest(DirEntPtr->d_name, ".backup") == -1) &&
				(FindCmdRequest(DirEntPtr->d_name, "Thumbs.db") == -1))
		    {
				switch(DirType)
				{
					case PRIM_CONTENT_LIST:
						strcpy(HashKey, "P");
						PathHift = DirNamePtr;
						if (*PathHift == '/')
						{
						    PathHift++;
							if (strlen(PathHift) > 0)
							{
						        strcat(HashKey, PathHift);
						        strcat(HashKey, "/");
							}
						}
						break;

					case SECOND_CONTENT_LIST:
						strcpy(HashKey, "S");
						PathHift = DirNamePtr;
						if (*PathHift == '/')
						{
						    PathHift++;
							if (strlen(PathHift) > 0)
							{
						        strcat(HashKey, PathHift);
						        strcat(HashKey, "/");
							}
						}
						break;

					default:
						strcpy(HashKey, "B");
						break;
				}
				strcat(HashKey, (char*)DirEntPtr->d_name);
                FileRecPtr = FindFileHash(RootNameHop, DirType, PathSearth);
				if (!FileRecPtr)
				{
				    if (AddFileHash(RootNameHop, PathSearth, HashKey))
					{
					    RecInHashCount++;
					}
				}
				else
				{
					if (st.st_mtime != FileRecPtr->LastUpdate)
					{
						/* File was changed and needs to be re-load */
						if (FileRecPtr->ZipFileBodyBuf) FreeMemory(FileRecPtr->ZipFileBodyBuf);
				        if (FileRecPtr->FileBodyBuf) FreeMemory(FileRecPtr->FileBodyBuf);
                        FileRecPtr->FileLen = (unsigned long)st.st_size;
                        FileRecPtr->FileBodyBuf = (unsigned char*)AllocateMemory((FileRecPtr->FileLen+1)*sizeof(unsigned char));
				        if (FileRecPtr->FileBodyBuf)
						{
				            *FileRecPtr->FileBodyBuf = 0;
				            HandelFilePtr = fopen(PathSearth, "rb");
				            BlockRead = fread((unsigned char*)&FileRecPtr->FileBodyBuf[0], 1, 
					            FileRecPtr->FileLen, HandelFilePtr);
	                        fclose(HandelFilePtr);
                            FileRecPtr->FileBodyBuf[FileRecPtr->FileLen] = 0;
					        FileRecPtr->LastUpdate = st.st_mtime;
							SetLastModifyedDate(FileRecPtr->LastUpdateStr, FileRecPtr->LastUpdate);

					        /* Check file extention for gzip compression */
		                    for (i=0;i < (sizeof(TablZipReadyExtFiles)/sizeof(char*));i++)
		                    {
			                    if  (FindCmdRequest(FileNamePtr, TablZipReadyExtFiles[i]) != -1)
			                    {
				                    /* File is gzip complined - should be compressed */
							        ComprDataBufPtr = (unsigned char*)AllocateMemory((FileRecPtr->FileLen+256)*sizeof(unsigned char));
							        if (ComprDataBufPtr)
							        {
							            FileRecPtr->ZipFileLen = zip(&ZipInfo, FileNamePtr, false, (unsigned int)FileRecPtr->FileLen, 
								            (unsigned char*)&FileRecPtr->FileBodyBuf[0], ComprDataBufPtr);
								        FileRecPtr->ZipFileBodyBuf = (unsigned char*)AllocateMemory((FileRecPtr->ZipFileLen+1)*sizeof(unsigned char));
								        if (FileRecPtr->ZipFileLen)
								        {
								            memcpy(FileRecPtr->ZipFileBodyBuf, ComprDataBufPtr, FileRecPtr->ZipFileLen);
								        }
								        else
								        {
								            printf("Fail to memory allocation for compressed file body\n");
				                            RemFileHash(RootNameHop, DirType, PathSearth);
								        }
								        FreeMemory(ComprDataBufPtr);
							        }
							        else
							        {
								        printf("Fail to memory allocation for file body compress result\n");
								        RemFileHash(RootNameHop, DirType, PathSearth);
							        }
			                    }
		                    }
							UpdateRecInHashCount++;
						}
				        else
						{
				            /* Memory allocation for file body cashe is failed */
							printf("Fail to allocate memory for file hash (%s)\n", PathSearth);
                        }
                    }
				}
		    }
	    }
        closedir(DirPtr);
    }
#endif
	FreeMemory(PathSearth);
	FreeMemory(HashKey);
	return;
}
//---------------------------------------------------------------------------
void ContentFilesHashInit(char *ServStartPath, SERVER_CUSTOM_CONFIG *ServCustCfgPtr,
	char *HashDirArray[], unsigned int ArraySize)
{
	unsigned int    index;
	char		    *PathSearth = NULL;
	char		    *BaseDirPtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	HASH_DIR_INFO   *SelDirPtr = NULL;
	char            *RetCodeCwd = NULL;

	InitFileHash(&FileHashHop);

	PathSearth = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathSearth) return;

#ifdef WIN32
	RetCodeCwd = _getcwd(PathSearth ,512);
#else
	RetCodeCwd = getcwd((char*)PathSearth ,512);
#endif

	/* Add to hash default directories */
	for(index=0;index < ArraySize;index++)
	    AddFilesDirHash(&FileHashHop, BASE_HASH_LIST, PathSearth, HashDirArray[index]);

	/* Add to hash customer defined directories */
	SelObjPtr = (ObjListTask*)GetFistObjectList(&ServCustCfgPtr->FileHashDirList);
	while(SelObjPtr)
	{
	    SelDirPtr = (HASH_DIR_INFO*)SelObjPtr->UsedTask;
		switch(SelDirPtr->DirType)
		{
			case PRIM_CONTENT_LIST:
				BaseDirPtr = ServCustCfgPtr->PrimContentRootDir;
				break;

			case SECOND_CONTENT_LIST:
				BaseDirPtr = ServCustCfgPtr->SecondContentRootDir;
				break;

			default:
				BaseDirPtr = PathSearth;
				break;
		}

		AddFilesDirHash(&FileHashHop, SelDirPtr->DirType, BaseDirPtr, SelDirPtr->HashDir);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&ServCustCfgPtr->FileHashDirList);
	}

	printf("Content files load to hash is done (Files: %d, HashCount: %d)\n", 
		RecInHashCount, HashEntityCount);
	FreeMemory(PathSearth);
}
//---------------------------------------------------------------------------
void FileDataHashUpdate(char *ServStartPath, SERVER_CUSTOM_CONFIG *ServCustCfgPtr,
	char *HashDirArray[], unsigned int ArraySize)
{
	unsigned int    index;
	char		    *PathSearth = NULL;
	char		    *BaseDirPtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	HASH_DIR_INFO   *SelDirPtr = NULL;
	char            *RetCodeCwd = NULL;

	PathSearth = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathSearth) return;

#ifdef WIN32
	RetCodeCwd = _getcwd(PathSearth ,512);
#else
	RetCodeCwd = getcwd((char*)PathSearth ,512);
#endif

	/* Add to hash default directories */
	for(index=0;index < ArraySize;index++)
	    UpdateFilesDirHash(&FileHashHop, BASE_HASH_LIST, PathSearth, HashDirArray[index]);

	/* Add to hash customer defined directories */
	SelObjPtr = (ObjListTask*)GetFistObjectList(&ServCustCfgPtr->FileHashDirList);
	while(SelObjPtr)
	{
	    SelDirPtr = (HASH_DIR_INFO*)SelObjPtr->UsedTask;
		switch(SelDirPtr->DirType)
		{
			case PRIM_CONTENT_LIST:
				BaseDirPtr = ServCustCfgPtr->PrimContentRootDir;
				break;

			case SECOND_CONTENT_LIST:
				BaseDirPtr = ServCustCfgPtr->SecondContentRootDir;
				break;

			default:
				BaseDirPtr = PathSearth;
				break;
		}

		UpdateFilesDirHash(&FileHashHop, SelDirPtr->DirType, BaseDirPtr, SelDirPtr->HashDir);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&ServCustCfgPtr->FileHashDirList);
	}

	DebugLogPrint(NULL, "Content files update in hash is done (Updated: %d, Files: %d, HashCount: %d)\r\n",
	    UpdateRecInHashCount, RecInHashCount, HashEntityCount);
	FreeMemory(PathSearth);
}
//---------------------------------------------------------------------------
void ContentFilesHashClose(SERVER_CUSTOM_CONFIG *ServCustCfgPtr)
{
	HASH_DIR_INFO   *SelDirPtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&ServCustCfgPtr->FileHashDirList);
	while(SelObjPtr)
	{
	    SelDirPtr = (HASH_DIR_INFO*)SelObjPtr->UsedTask;
		FreeMemory(SelDirPtr);
		RemStructList(&ServCustCfgPtr->FileHashDirList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&ServCustCfgPtr->FileHashDirList);
	}
}
//---------------------------------------------------------------------------
