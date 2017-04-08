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
#include "HtmlPageHash.h"
#include "SysWebFunction.h"

unsigned int HtmlPgHashEntityCnt = 0;
unsigned int HtmlPgRecInHashCnt = 0;

//char HtmlPageExt[] = ".html";

static unsigned char AsciiHtmlPgCharToIndex[] ={
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   2,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,
  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,   0,   0,   0,   0,  29,
   0,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };
//---------------------------------------------------------------------------
void InitHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(HTML_PAGE_HASH_CHAR_HOP));	
}
//---------------------------------------------------------------------------
bool AddHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *RootNameHop, 
					 char *HtmlPageName, unsigned int UserTypeMask,
					 void *HtmlHandlerPtr)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	HTML_PAGE_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	HTML_PAGE_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	HTML_PAGE_HASH_RECORD   *NewRecordPtr = NULL;

	NameLen = strlen(HtmlPageName);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	   HopIndex = AsciiHtmlPgCharToIndex[(unsigned char)HtmlPageName[CharIndex]];
	   if (!HopIndex)
	   {
		   if ((HtmlPageName[CharIndex] < '0') || (HtmlPageName[CharIndex] > '9'))
		       printf("Add HTML to hash is failed for (%s)\n", HtmlPageName);
		   break;
	   }
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopCharPtr = (HTML_PAGE_HASH_CHAR_HOP*)AllocateMemory(sizeof(HTML_PAGE_HASH_CHAR_HOP));
		   if (!NewHashHopCharPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitHtmlPageHash(NewHashHopCharPtr);
		   SelHashHopCharPtr->HashCharHop[HopIndex] = NewHashHopCharPtr;
		   SelHashHopCharPtr->UsedCharCount++;
		   NewHashHopCharPtr->ParentHashCharPtr = (void*)SelHashHopCharPtr;
		   NewHashHopCharPtr->ParentHashCharIndex = HopIndex;
		   SelHashHopCharPtr = NewHashHopCharPtr;
		   HtmlPgHashEntityCnt++;
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
			NewRecordPtr = SelHashHopCharPtr->Record;
			if (!(NewRecordPtr->UserTypeMask & UserTypeMask))
			{
                NewRecordPtr->UserTypeMask |= UserTypeMask;
			}
			else
			{
			    printf("In cashe of html pages (%s) already present for %d mask\n", 
					HtmlPageName, UserTypeMask);
			    Result = false;
			}
            return Result;
		}
	    /* File hash fillout */
		NewRecordPtr = (HTML_PAGE_HASH_RECORD*)AllocateMemory(sizeof(HTML_PAGE_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
            NewRecordPtr->UserTypeMask = UserTypeMask;
			NewRecordPtr->HtmlPageHandlerPtr = HtmlHandlerPtr;
			HtmlPgRecInHashCnt++;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
HTML_PAGE_HASH_RECORD* FindHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *RootNameHop,
										char *HtmlPageName, unsigned int UserTypeMask)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	HTML_PAGE_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	HTML_PAGE_HASH_RECORD   *SelRecordPtr = NULL;

	NameLen = strlen(HtmlPageName);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	    HopIndex = AsciiHtmlPgCharToIndex[(unsigned char)HtmlPageName[CharIndex]];
		if (!HopIndex)
		{
			if (!SelHashHopCharPtr) Result = false;
			break;
		}
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
	if (!Result || !SelHashHopCharPtr->Record) return NULL;
	if (!(SelHashHopCharPtr->Record->UserTypeMask & UserTypeMask)) return NULL;
	return SelHashHopCharPtr->Record;
}
//---------------------------------------------------------------------------
bool RemHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *RootNameHop, char *HtmlPageName)
{
    HTML_PAGE_HASH_RECORD *HtmlPageRecPtr = NULL;
	HTML_PAGE_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	HTML_PAGE_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    HtmlPageRecPtr = FindHtmlPageHash(RootNameHop, HtmlPageName, 0xffffffff);
	if (!HtmlPageRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (HTML_PAGE_HASH_CHAR_HOP*)HtmlPageRecPtr->HashCharHopPtr;
	FreeMemory(HtmlPageRecPtr);
	SelHashHopCharPtr->Record = NULL;
	HtmlPgRecInHashCnt--;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (HTML_PAGE_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
		    ParentHashHopCharPtr->HashCharHop[SelHashHopCharPtr->ParentHashCharIndex] = NULL;
		    FreeMemory(SelHashHopCharPtr);
			HtmlPgHashEntityCnt--;
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
void CloseHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *RootNameHop)
{
	CloseHtmlPageHashHop(RootNameHop);
}
//---------------------------------------------------------------------------
void CloseHtmlPageHashHop(HTML_PAGE_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	HTML_PAGE_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MAX_CHAR_HTML_PAGE_HASH_INDEX;index++)
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
                    CloseHtmlPageHashHop(NextHashHopCharPtr);
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
