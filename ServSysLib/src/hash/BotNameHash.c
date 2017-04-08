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

#include "CommonPlatform.h"
#include "SysLibTool.h"
#include "BotNameHash.h"

unsigned char BotIdentCharToIndex[] ={
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,  47,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   3,   4,   5,   6,
   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,   0,   0,   0,   0,
   19, 20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
   35, 36,  37,  38,  39,  40,  41,  42,  43,  44,  45,   0,   0,   0,   0,  46,
   0,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
   35, 36,  37,  38,  39,  40,  41,  42,  43,  44,  45,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };

//---------------------------------------------------------------------------
void InitBotNameHash(BOT_NAME_MAP_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(BOT_NAME_MAP_HASH_CHAR_HOP));
}
//---------------------------------------------------------------------------
bool AddBotNameHash(BOT_NAME_MAP_HASH_CHAR_HOP *RootNameHop,
    char *BotNameKey, BOT_INFO_TYPE *BotInfoPtr)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	char *NameOnlyPtr = NULL;
	BOT_NAME_MAP_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	BOT_NAME_MAP_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	BOT_NAME_MAP_HASH_RECORD   *NewRecordPtr = NULL;

	NameLen = strlen(BotNameKey);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	   HopIndex = BotIdentCharToIndex[(unsigned char)BotNameKey[CharIndex]];
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopCharPtr = (BOT_NAME_MAP_HASH_CHAR_HOP*)AllocateMemory(sizeof(BOT_NAME_MAP_HASH_CHAR_HOP));
		   if (!NewHashHopCharPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitBotNameHash(NewHashHopCharPtr);
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
			printf("In cashe of BOTs key (%s) already present\n", BotNameKey);
            return false;
		}
	    /* File hash fillout */
		NewRecordPtr = (BOT_NAME_MAP_HASH_RECORD*)AllocateMemory(sizeof(BOT_NAME_MAP_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
            NewRecordPtr->BotInfoPtr = BotInfoPtr;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
BOT_NAME_MAP_HASH_RECORD* FindBotNameHash(BOT_NAME_MAP_HASH_CHAR_HOP *RootNameHop, char *BotNameKey)
{
    unsigned int CharIndex, HopIndex, NameLen;
	BOT_NAME_MAP_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
    BOT_NAME_MAP_HASH_CHAR_HOP *LastSelHashHopCharPtr = NULL;
	BOT_NAME_MAP_HASH_RECORD   *SelRecordPtr = NULL;

    if (!BotNameKey || !RootNameHop) return NULL;
	NameLen = strlen(BotNameKey);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	    HopIndex = BotIdentCharToIndex[(unsigned char)BotNameKey[CharIndex]];
	    if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	    {
            break;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
            LastSelHashHopCharPtr = SelHashHopCharPtr;
		    SelHashHopCharPtr = SelHashHopCharPtr->HashCharHop[HopIndex];
	    }
	}
    if (SelHashHopCharPtr && SelHashHopCharPtr->Record)
    {
        return SelHashHopCharPtr->Record;
    }
    else
    {
	    if (!LastSelHashHopCharPtr) return NULL;
        if (!LastSelHashHopCharPtr->Record) return NULL;
	    return LastSelHashHopCharPtr->Record;
    }
}
//---------------------------------------------------------------------------
bool RemBotNameHash(BOT_NAME_MAP_HASH_CHAR_HOP *RootNameHop, char *BotNameKey)
{
    BOT_NAME_MAP_HASH_RECORD *BotNameRecPtr = NULL;
	BOT_NAME_MAP_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	BOT_NAME_MAP_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    BotNameRecPtr = FindBotNameHash(RootNameHop, BotNameKey);
	if (!BotNameRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (BOT_NAME_MAP_HASH_CHAR_HOP*)BotNameRecPtr->HashCharHopPtr;
	FreeMemory(BotNameRecPtr);
	SelHashHopCharPtr->Record = NULL;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (BOT_NAME_MAP_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
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
void CloseBotNameHashHop(BOT_NAME_MAP_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	BOT_NAME_MAP_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MAX_CHAR_HASH_INDEX;index++)
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
                    CloseBotNameHashHop(NextHashHopCharPtr);
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
