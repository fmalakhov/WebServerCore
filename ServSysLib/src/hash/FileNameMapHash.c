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
#include "FileNameMapHash.h"

extern unsigned char AsciiFileCharToIndex[];
//---------------------------------------------------------------------------
void InitFileNameMapHash(FILE_NAME_MAP_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(FILE_NAME_MAP_HASH_CHAR_HOP));
}
//---------------------------------------------------------------------------
bool AddFileNameMapHash(FILE_NAME_MAP_HASH_CHAR_HOP *RootNameHop,
    char *FileNameKey, char *FileNameMap)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	char *NameOnlyPtr = NULL;
	FILE_NAME_MAP_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	FILE_NAME_MAP_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	FILE_NAME_MAP_HASH_RECORD   *NewRecordPtr = NULL;

	NameLen = strlen(FileNameKey);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	   HopIndex = AsciiFileCharToIndex[(unsigned char)FileNameKey[CharIndex]];
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopCharPtr = (FILE_NAME_MAP_HASH_CHAR_HOP*)AllocateMemory(sizeof(FILE_NAME_MAP_HASH_CHAR_HOP));
		   if (!NewHashHopCharPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitFileNameMapHash(NewHashHopCharPtr);
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
			printf("In cashe of image names key (%s) already present\n", FileNameKey);
            return false;
		}
	    /* File hash fillout */
		NewRecordPtr = (FILE_NAME_MAP_HASH_RECORD*)AllocateMemory(sizeof(FILE_NAME_MAP_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
			strncpy((char*)NewRecordPtr->FileNameMap, FileNameMap, MAX_LEN_FILE_NAME_MAP);
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
FILE_NAME_MAP_HASH_RECORD* FindFileNameMapHash(FILE_NAME_MAP_HASH_CHAR_HOP *RootNameHop, char *FileNameKey)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	FILE_NAME_MAP_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	FILE_NAME_MAP_HASH_RECORD   *SelRecordPtr = NULL;

	NameLen = strlen(FileNameKey);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	    HopIndex = AsciiFileCharToIndex[(unsigned char)FileNameKey[CharIndex]];
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
bool RemFileNameMapHash(FILE_NAME_MAP_HASH_CHAR_HOP *RootNameHop, char *FileNameKey)
{
    FILE_NAME_MAP_HASH_RECORD *FileNameMapRecPtr = NULL;
	FILE_NAME_MAP_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	FILE_NAME_MAP_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    FileNameMapRecPtr = FindFileNameMapHash(RootNameHop, FileNameKey);
	if (!FileNameMapRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (FILE_NAME_MAP_HASH_CHAR_HOP*)FileNameMapRecPtr->HashCharHopPtr;
	FreeMemory(FileNameMapRecPtr);
	SelHashHopCharPtr->Record = NULL;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (FILE_NAME_MAP_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
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
void CloseFileNameMapHashHop(FILE_NAME_MAP_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	FILE_NAME_MAP_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MAX_FNM_CHAR_HASH_INDEX;index++)
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
                    CloseFileNameMapHashHop(NextHashHopCharPtr);
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
