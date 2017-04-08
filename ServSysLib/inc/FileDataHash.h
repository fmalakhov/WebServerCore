# if ! defined( FileDataHashH )
#	define FileDataHashH	/* only include me once */

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


#ifndef CommonPlatformH
#include "CommonPlatform.h"
#endif

#ifdef _LINUX_X86_
#include <time.h>
#endif

#ifndef vistypesH
#include "vistypes.h"
#endif

#ifndef SysLibToolH
#include "SysLibTool.h"
#endif

#ifndef CustomConfigDataBaseH
#include "CustomConfigDataBase.h"
#endif

#define MAX_FDH_CHAR_HASH_INDEX  70
#define MAX_ADD_CASHE_FILE_LEN   4000000
#define MAX_LEN_LAST_UPDATE_STR	 64

typedef struct {
#ifdef WIN32
	FILETIME        LastUpdate;
#else
	time_t          LastUpdate;
#endif
	unsigned int    ZipFileLen;
    unsigned long   FileLen;
    unsigned int    FileEtag;
    unsigned char   *FileBodyBuf;
	unsigned char   *ZipFileBodyBuf;
	void            *HashCharHopPtr;
	char            LastUpdateStr[MAX_LEN_LAST_UPDATE_STR+1];
} FILE_HASH_RECORD;

typedef struct {
   unsigned int     UsedCharCount;
   unsigned int     ParentHashCharIndex;
   void             *ParentHashCharPtr;
   FILE_HASH_RECORD *Record;
   void             *HashCharHop[MAX_FDH_CHAR_HASH_INDEX+1];
} FILE_HASH_CHAR_HOP;

void InitFileHash(FILE_HASH_CHAR_HOP *NameHop);
void CloseFileHash(FILE_HASH_CHAR_HOP *RootNameHop);
bool AddFileHash(FILE_HASH_CHAR_HOP *RootNameHop,  char *FileName, char *HashKey);
FILE_HASH_RECORD* FindFileHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType, char *FileName);
bool RemFileHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType, char *FileName);
void AddFilesDirHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType, 
	char *BaseDirPtr, char *DirNamePtr);
void CloseFileHashHop(FILE_HASH_CHAR_HOP *SelHashHopCharPtr);
void UpdateFilesDirHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType, char *BaseDirPtr, char *DirNamePtr);
void ContentFilesHashInit(char *ServStartPath, SERVER_CUSTOM_CONFIG *ServCustCfgPtr,
	char *HashDirArray[], unsigned int ArraySize);
void FileDataHashUpdate(char *ServStartPath, SERVER_CUSTOM_CONFIG *ServCustCfgPtr,
	char *HashDirArray[], unsigned int ArraySize);
void AddSingleFileHash(FILE_HASH_CHAR_HOP *RootNameHop, HashDirTypeT DirType, 
	char *BaseDirPtr, char *DirNamePtr);
void ContentFilesHashClose(SERVER_CUSTOM_CONFIG *ServCustCfgPtr);
//---------------------------------------------------------------------------
#endif  /* if ! defined( FileDataHashH ) */
