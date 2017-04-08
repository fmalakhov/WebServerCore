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
#include "MobileDeviceHash.h"

extern unsigned char BotIdentCharToIndex[];
//---------------------------------------------------------------------------
void InitMobileDeviceHash(MOBILE_DEV_MAP_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(MOBILE_DEV_MAP_HASH_CHAR_HOP));
}
//---------------------------------------------------------------------------
bool AddMobileDeviceHash(MOBILE_DEV_MAP_HASH_CHAR_HOP *RootNameHop,
    char *MobileDeviceKey, MOBILE_DEV_TYPE *MobileDevPtr)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	char *NameOnlyPtr = NULL;
	MOBILE_DEV_MAP_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	MOBILE_DEV_MAP_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	MOBILE_DEV_MAP_HASH_RECORD   *NewRecordPtr = NULL;

	NameLen = strlen(MobileDeviceKey);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	   HopIndex = BotIdentCharToIndex[(unsigned char)MobileDeviceKey[CharIndex]];
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopCharPtr = (MOBILE_DEV_MAP_HASH_CHAR_HOP*)AllocateMemory(sizeof(MOBILE_DEV_MAP_HASH_CHAR_HOP));
		   if (!NewHashHopCharPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitMobileDeviceHash(NewHashHopCharPtr);
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
			printf("In cashe of BOTs key (%s) already present\n", MobileDeviceKey);
            return false;
		}
	    /* File hash fillout */
		NewRecordPtr = (MOBILE_DEV_MAP_HASH_RECORD*)AllocateMemory(sizeof(MOBILE_DEV_MAP_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
            NewRecordPtr->MobileDevicePtr = MobileDevPtr;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
MOBILE_DEV_MAP_HASH_RECORD* FindMobileDeviceHash(MOBILE_DEV_MAP_HASH_CHAR_HOP *RootNameHop, char *MobileDeviceKey)
{
    unsigned int CharIndex, HopIndex, NameLen;
	MOBILE_DEV_MAP_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
    MOBILE_DEV_MAP_HASH_CHAR_HOP *LastSelHashHopCharPtr = NULL;
	MOBILE_DEV_MAP_HASH_RECORD   *SelRecordPtr = NULL;

    if (!MobileDeviceKey || !RootNameHop) return NULL;
	NameLen = strlen(MobileDeviceKey);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	    HopIndex = BotIdentCharToIndex[(unsigned char)MobileDeviceKey[CharIndex]];
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
bool RemMobileDeviceHash(MOBILE_DEV_MAP_HASH_CHAR_HOP *RootNameHop, char *MobileDeviceKey)
{
    MOBILE_DEV_MAP_HASH_RECORD *MobileDeviceRecPtr = NULL;
	MOBILE_DEV_MAP_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	MOBILE_DEV_MAP_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    MobileDeviceRecPtr = FindMobileDeviceHash(RootNameHop, MobileDeviceKey);
	if (!MobileDeviceRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (MOBILE_DEV_MAP_HASH_CHAR_HOP*)MobileDeviceRecPtr->HashCharHopPtr;
	FreeMemory(MobileDeviceRecPtr);
	SelHashHopCharPtr->Record = NULL;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (MOBILE_DEV_MAP_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
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
void CloseMobileDeviceHashHop(MOBILE_DEV_MAP_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	MOBILE_DEV_MAP_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MAX_MOBDEV_CHAR_HASH_INDEX;index++)
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
                    CloseMobileDeviceHashHop(NextHashHopCharPtr);
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
