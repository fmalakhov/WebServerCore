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

#include "MpttParserHash.h"

static void InitMpttParserHash(MPTT_PARSER_HASH_CHAR_HOP *NameHop);
static bool AddMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage, MPTT_PARSER_DATA *MpttDataPtr);
static MPTT_PARSER_HASH_RECORD* FindMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage, char *MsgBody, unsigned int MsgLen);
static bool RemMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage, char *MsgName);
static void CloseMpttParserHashHop(MPTT_PARSER_MESSAGE *MpttParserMessage,
    MPTT_PARSER_HASH_CHAR_HOP *SelHashHopCharPtr);
//---------------------------------------------------------------------------
static void InitMpttParserHash(MPTT_PARSER_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(MPTT_PARSER_HASH_CHAR_HOP));	
}
//---------------------------------------------------------------------------
static bool AddMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage, MPTT_PARSER_DATA *MpttDataPtr)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	MPTT_PARSER_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	MPTT_PARSER_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	MPTT_PARSER_HASH_RECORD   *NewRecordPtr = NULL;

	NameLen = strlen((const char*)MpttDataPtr->FieldMarker);
	SelHashHopCharPtr = &MpttParserMessage->RootNameHop;
	for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	   HopIndex = MpttParserMessage->AsciMpttParserCharToIndex[(unsigned char)MpttDataPtr->FieldMarker[CharIndex]];
	   if (!HopIndex)
	   {
	       Result = false;
	       break;
	   }
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopCharPtr = (MPTT_PARSER_HASH_CHAR_HOP*)AllocateMemory(sizeof(MPTT_PARSER_HASH_CHAR_HOP));
		   if (!NewHashHopCharPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitMpttParserHash(NewHashHopCharPtr);
		   SelHashHopCharPtr->HashCharHop[HopIndex] = NewHashHopCharPtr;
		   SelHashHopCharPtr->UsedCharCount++;
		   NewHashHopCharPtr->ParentHashCharPtr = (void*)SelHashHopCharPtr;
		   NewHashHopCharPtr->ParentHashCharIndex = HopIndex;
		   SelHashHopCharPtr = NewHashHopCharPtr;
	   }
	   else
	   {
	       /* Existing char in hash for this hop is detected */
		   SelHashHopCharPtr = (MPTT_PARSER_HASH_CHAR_HOP*)SelHashHopCharPtr->HashCharHop[HopIndex];
	   }
	}
	if (Result)
	{
		if (SelHashHopCharPtr->Record) return false;
	    /* File hash fillout */
		NewRecordPtr = (MPTT_PARSER_HASH_RECORD*)AllocateMemory(sizeof(MPTT_PARSER_HASH_RECORD));
		if (!NewRecordPtr) Result = false;
		else
		{
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
			NewRecordPtr->FieldParser = MpttDataPtr;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
static MPTT_PARSER_HASH_RECORD* FindMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage, char *MsgBody, unsigned int MsgLen)
{
    register unsigned int CharIndex, HopIndex;
	register MPTT_PARSER_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;

	SelHashHopCharPtr = &MpttParserMessage->RootNameHop;
    for (CharIndex=0;CharIndex < MsgLen;CharIndex++)
	{
	    HopIndex = MpttParserMessage->AsciMpttParserCharToIndex[(unsigned char)MsgBody[CharIndex]];
		if (!HopIndex) break;
	    if (!SelHashHopCharPtr->HashCharHop[HopIndex]) break;
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopCharPtr = (MPTT_PARSER_HASH_CHAR_HOP*)SelHashHopCharPtr->HashCharHop[HopIndex];
	    }
	}
	if (!SelHashHopCharPtr->Record) return NULL;
	SelHashHopCharPtr->Record->Offset = CharIndex;
	return SelHashHopCharPtr->Record;
}
//---------------------------------------------------------------------------
static bool RemMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage, char *MsgName)
{
    MPTT_PARSER_HASH_RECORD *GroupNameRecPtr = NULL;
	MPTT_PARSER_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	MPTT_PARSER_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    GroupNameRecPtr = FindMpttParserHash(MpttParserMessage, MsgName, strlen(MsgName));
	if (!GroupNameRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (MPTT_PARSER_HASH_CHAR_HOP*)GroupNameRecPtr->HashCharHopPtr;
	FreeMemory(GroupNameRecPtr);
	SelHashHopCharPtr->Record = NULL;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != &MpttParserMessage->RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (MPTT_PARSER_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
			if (ParentHashHopCharPtr)
			{
		        ParentHashHopCharPtr->HashCharHop[SelHashHopCharPtr->ParentHashCharIndex] = NULL;
		        FreeMemory(SelHashHopCharPtr);
		        ParentHashHopCharPtr->UsedCharCount--;
			    SelHashHopCharPtr = ParentHashHopCharPtr;
		    }
			else
			{
				FreeMemory(SelHashHopCharPtr);
				break;
			}
	    }
	    else
	    {
	        break;
	    }
	}
	return true;
}
//---------------------------------------------------------------------------
void CloseMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage)
{
	CloseMpttParserHashHop(MpttParserMessage, &MpttParserMessage->RootNameHop);
}
//---------------------------------------------------------------------------
static void CloseMpttParserHashHop(MPTT_PARSER_MESSAGE *MpttParserMessage,
    MPTT_PARSER_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	MPTT_PARSER_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MpttParserMessage->MaxMpttParserIndex;index++)
		{
			if (SelHashHopCharPtr->HashCharHop[index])
			{
                NextHashHopCharPtr = (MPTT_PARSER_HASH_CHAR_HOP*)SelHashHopCharPtr->HashCharHop[index];
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
                    CloseMpttParserHashHop(MpttParserMessage, NextHashHopCharPtr);
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
void CreateMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage)
{
	unsigned Index = 1;
	unsigned int i, j;
	
	memset(MpttParserMessage->AsciMpttParserCharToIndex, 0, MAX_CHAR_MPTT_PARSER_HASH_INDEX);
	for (i=0;i < MpttParserMessage->ListSize;i++)
	{
		for (j=0;j < strlen(MpttParserMessage->MpttParserList[i].FieldMarker);j++)
			MpttParserMessage->AsciMpttParserCharToIndex[(unsigned char)MpttParserMessage->MpttParserList[i].FieldMarker[j]] = 1;
	}
	
	MpttParserMessage->MaxMpttParserIndex = 0;
	for(i=0;i < MAX_CHAR_MPTT_PARSER_HASH_INDEX;i++)
	{
		if (MpttParserMessage->AsciMpttParserCharToIndex[i])
		{
			MpttParserMessage->AsciMpttParserCharToIndex[i] = Index++;
			MpttParserMessage->MaxMpttParserIndex++;
	    }
	}
	
    InitMpttParserHash(&MpttParserMessage->RootNameHop);
	
	for (i=0;i < MpttParserMessage->ListSize;i++)
	{
        if (!AddMpttParserHash(MpttParserMessage, &MpttParserMessage->MpttParserList[i]))
		{
			printf("Fail to add %s MPTT field marker to MPTT parser hash\n", 
		        MpttParserMessage->MpttParserList[i].FieldMarker);
	    }
	}
}
//---------------------------------------------------------------------------
bool MpttParserMsgExtract(MPTT_PARSER_MESSAGE *MpttParserMessage, void *DataPtr, char *MpttParserPtr)
{
	unsigned int            MpttBodyLen;
    char                    *CheckPtr;
	char                    *MarkerEndPtr = NULL;
	MPTT_PARSER_HASH_RECORD *MsgHashPtr = NULL; 
	
	if (!MpttParserPtr || !MpttParserMessage) return false;
    CheckPtr = MpttParserPtr;
	MpttBodyLen = strlen(MpttParserPtr);
	/* Search trow MPTT message body */	
	while(CheckPtr && (*((unsigned char*)CheckPtr) > 0))
	{
	    MsgHashPtr = FindMpttParserHash(MpttParserMessage, CheckPtr,
			(MpttBodyLen - (unsigned int)(CheckPtr - MpttParserPtr)));
		if (MsgHashPtr)
		{
			MarkerEndPtr = CheckPtr + MsgHashPtr->Offset;
			if (MsgHashPtr->FieldParser && MsgHashPtr->FieldParser->FieldHandler)  
				        CheckPtr = (MsgHashPtr->FieldParser->FieldHandler)(DataPtr, (char*)MarkerEndPtr);
			else        CheckPtr = MarkerEndPtr;
		}
		else
		{
		    CheckPtr++;
	    }
	}
	if (CheckPtr) return true;
	else          return false;
}
//---------------------------------------------------------------------------
bool MpttParserLineMsgExtract(MPTT_PARSER_MESSAGE *MpttParserMessage, void *DataPtr, char *MpttParserPtr)
{
	unsigned int            MpttBodyLen;
    char                    *CheckPtr;
	char                    *MarkerEndPtr = NULL;
	MPTT_PARSER_HASH_RECORD *MsgHashPtr = NULL; 
	
	if (!MpttParserPtr || !MpttParserMessage) return false;
    CheckPtr = MpttParserPtr;
	MpttBodyLen = strlen(MpttParserPtr);
	/* Search trow message line */	
	while(CheckPtr && (*((unsigned char*)CheckPtr) > 0))
	{
		if ((*CheckPtr == '\r') || (*CheckPtr == '\n')) break;
	    MsgHashPtr = FindMpttParserHash(MpttParserMessage, CheckPtr,
			(MpttBodyLen - (unsigned int)(CheckPtr - MpttParserPtr)));
		if (MsgHashPtr)
		{
			MarkerEndPtr = CheckPtr + MsgHashPtr->Offset;
			if (MsgHashPtr->FieldParser && MsgHashPtr->FieldParser->FieldHandler)  
				        CheckPtr = (MsgHashPtr->FieldParser->FieldHandler)((void*)DataPtr, (char*)MarkerEndPtr);
			else        CheckPtr = MarkerEndPtr;
		}
		else
		{
		    CheckPtr++;
	    }
	}
	if (CheckPtr) return true;
	else          return false;
}
//---------------------------------------------------------------------------
