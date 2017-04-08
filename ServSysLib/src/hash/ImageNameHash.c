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
#include "ImageNameHash.h"
#include "SysWebFunction.h"
#include "WebServInfo.h"

IMAGE_NAME_HASH_CHAR_HOP ImageNameHashHop;

extern unsigned char AsciiFileCharToIndex[];
extern IMAGE_NAME ServImageArray[];
extern unsigned int ServImageArrayLen;
extern unsigned char gLanguageType;
//---------------------------------------------------------------------------
void InitImageNameHash(IMAGE_NAME_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(IMAGE_NAME_HASH_CHAR_HOP));
}
//---------------------------------------------------------------------------
void LoadImageNameHash()
{
	unsigned int i;

	InitImageNameHash(&ImageNameHashHop);
	for (i=0;i < ServImageArrayLen;i++)
	{
        AddImageNameHash(&ImageNameHashHop, (char*)ServImageArray[i].Key, 
			(char*)ServImageArray[i].EngName, (char*)ServImageArray[i].RusName);
	}
}
//---------------------------------------------------------------------------
bool AddImageNameHash(IMAGE_NAME_HASH_CHAR_HOP *RootNameHop, char *ImageKey,
    char *ImageName, char *RusImageName)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	char *NameOnlyPtr = NULL;
	IMAGE_NAME_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	IMAGE_NAME_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	IMAGE_NAME_HASH_RECORD   *NewRecordPtr = NULL;

	NameLen = strlen(ImageKey);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	   HopIndex = AsciiFileCharToIndex[(unsigned char)ImageKey[CharIndex]];
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopCharPtr = (IMAGE_NAME_HASH_CHAR_HOP*)AllocateMemory(sizeof(IMAGE_NAME_HASH_CHAR_HOP));
		   if (!NewHashHopCharPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitImageNameHash(NewHashHopCharPtr);
		   SelHashHopCharPtr->HashCharHop[HopIndex] = NewHashHopCharPtr;
		   SelHashHopCharPtr->UsedCharCount++;
		   NewHashHopCharPtr->ParentHashCharPtr = (void*)SelHashHopCharPtr;
		   NewHashHopCharPtr->ParentHashCharIndex = HopIndex;
		   SelHashHopCharPtr = NewHashHopCharPtr;
	   }
	   else
	   {
	       /* Existing char in hash for this hop is detected */
		   SelHashHopCharPtr = SelHashHopCharPtr->HashCharHop[HopIndex];
	   }
	}
	if (Result)
	{
		if (SelHashHopCharPtr->Record)
		{
			printf("In cashe of image names key (%s) already present\n", ImageKey);
            return false;
		}
	    /* File hash fillout */
		NewRecordPtr = (IMAGE_NAME_HASH_RECORD*)AllocateMemory(sizeof(IMAGE_NAME_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
			strncpy((char*)NewRecordPtr->ImageName, ImageName, MAX_LEN_IMAGE_NAME);
            strncpy((char*)NewRecordPtr->RusImageName, RusImageName, MAX_LEN_IMAGE_NAME);

        }
	}
	return Result;
}
//---------------------------------------------------------------------------
char* GetImageNameByKey(char *ImageKey)
{
    IMAGE_NAME_HASH_RECORD *ImageNamePtr = NULL;
    char                   *SelNamePtr = NULL;

    ImageNamePtr = FindImageNameHash(&ImageNameHashHop, ImageKey);
	if (ImageNamePtr)
    {
        switch(gLanguageType)
        {
            case LGT_ENGLISH:
                SelNamePtr = ImageNamePtr->ImageName;
                break;
                
            default: /* For Russian language */
                SelNamePtr = ImageNamePtr->RusImageName;
                break;            
        }
    }
    return SelNamePtr;
}
//---------------------------------------------------------------------------
IMAGE_NAME_HASH_RECORD* FindImageNameHash(IMAGE_NAME_HASH_CHAR_HOP *RootNameHop, char *ImageKey)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	IMAGE_NAME_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	IMAGE_NAME_HASH_RECORD   *SelRecordPtr = NULL;

	NameLen = strlen(ImageKey);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	    HopIndex = AsciiFileCharToIndex[(unsigned char)ImageKey[CharIndex]];
	    if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	    {
	        Result = false;
            break;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopCharPtr = (IMAGE_NAME_HASH_CHAR_HOP*)SelHashHopCharPtr->HashCharHop[HopIndex];
	    }
	}
	if (!Result) return NULL;
	return SelHashHopCharPtr->Record;
}
//---------------------------------------------------------------------------
bool RemImageNameHash(IMAGE_NAME_HASH_CHAR_HOP *RootNameHop, char *ImageKey)
{
    IMAGE_NAME_HASH_RECORD *ImageNameRecPtr = NULL;
	IMAGE_NAME_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	IMAGE_NAME_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    ImageNameRecPtr = FindImageNameHash(RootNameHop, ImageKey);
	if (!ImageNameRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (IMAGE_NAME_HASH_CHAR_HOP*)ImageNameRecPtr->HashCharHopPtr;
	FreeMemory(ImageNameRecPtr);
	SelHashHopCharPtr->Record = NULL;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (IMAGE_NAME_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
		    ParentHashHopCharPtr->HashCharHop[SelHashHopCharPtr->ParentHashCharIndex] = NULL;
		    FreeMemory(SelHashHopCharPtr);
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
void CloseImageNameHash()
{
	CloseImageNameHashHop(&ImageNameHashHop);
}
//---------------------------------------------------------------------------
void CloseImageNameHashHop(IMAGE_NAME_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	IMAGE_NAME_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MAX_INH_CHAR_HASH_INDEX;index++)
		{
			if (SelHashHopCharPtr->HashCharHop[index])
			{
                NextHashHopCharPtr = SelHashHopCharPtr->HashCharHop[index];
				if (!NextHashHopCharPtr->UsedCharCount)
				{
					if (NextHashHopCharPtr->Record)
                        FreeMemory(NextHashHopCharPtr->Record);
					FreeMemory(NextHashHopCharPtr);
					SelHashHopCharPtr->HashCharHop[index] = NULL;
					SelHashHopCharPtr->UsedCharCount--;
					if (!SelHashHopCharPtr->UsedCharCount) break;
				}
				else
				{
                    CloseImageNameHashHop(NextHashHopCharPtr);
					if (!NextHashHopCharPtr->UsedCharCount)
					{
					    if (NextHashHopCharPtr->Record)
                            FreeMemory(NextHashHopCharPtr->Record);
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
