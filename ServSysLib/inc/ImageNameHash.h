# if ! defined( ImageNameHashH )
#	define ImageNameHashH	/* only include me once */

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

#define MAX_INH_CHAR_HASH_INDEX    70
#define MAX_LEN_IMAGE_NAME         512

typedef struct {
	unsigned char *Key;
	unsigned char *EngName;
    unsigned char *RusName;
} IMAGE_NAME;

typedef struct {
    unsigned char   ImageName[MAX_LEN_IMAGE_NAME+1];
    unsigned char   RusImageName[MAX_LEN_IMAGE_NAME+1];
	void            *HashCharHopPtr;
} IMAGE_NAME_HASH_RECORD;

typedef struct {
   unsigned int     UsedCharCount;
   unsigned int     ParentHashCharIndex;
   void             *ParentHashCharPtr;
   IMAGE_NAME_HASH_RECORD *Record;
   void             *HashCharHop[MAX_INH_CHAR_HASH_INDEX+1];
} IMAGE_NAME_HASH_CHAR_HOP;

void LoadImageNameHash();
void CloseImageNameHash();
char* GetImageNameByKey(char *ImageKey);
void InitImageNameHash(IMAGE_NAME_HASH_CHAR_HOP *NameHop);
bool AddImageNameHash(IMAGE_NAME_HASH_CHAR_HOP *RootNameHop, char *ImageKey, char *ImageName, char *RusImageName);
IMAGE_NAME_HASH_RECORD* FindImageNameHash(IMAGE_NAME_HASH_CHAR_HOP *RootNameHop,char *ImageKey);
bool RemImageNameHash(IMAGE_NAME_HASH_CHAR_HOP *RootNameHop, char *ImageKey);
void CloseImageNameHashHop(IMAGE_NAME_HASH_CHAR_HOP *SelHashHopCharPtr);
//---------------------------------------------------------------------------
#endif  /* if ! defined( ImageNameHashH ) */
