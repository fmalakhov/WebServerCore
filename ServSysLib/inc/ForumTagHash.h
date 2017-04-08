# if ! defined( ForumTagHashH )
#	define ForumTagHashH	/* only include me once */

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

#define SMILE_FUNCT_TAG      1
#define OPERATION_FUNCT_TAG  2
#define MAX_CHAR_FORUM_TAG_HASH_INDEX    47
#define START_RUS_CHAR       0xc0

typedef struct {
	bool          isTextOnly;
	unsigned char TagType;
	unsigned int  TagLen;
	char          *PostTagPtr;
	void          *TagRecordPtr;
	void          *HashCharHopPtr;
} FORUM_TAG_HASH_RECORD;

typedef struct {
   unsigned int     UsedCharCount;
   unsigned int     ParentHashCharIndex;
   void             *ParentHashCharPtr;
   FORUM_TAG_HASH_RECORD *Record;
   void             *HashCharHop[MAX_CHAR_FORUM_TAG_HASH_INDEX+1];
} FORUM_TAG_HASH_CHAR_HOP;

typedef void (*TForumTagHanler)(FORUM_TAG_HASH_RECORD *FindInfoPtr);

typedef struct {
	unsigned char *SmileCodePtr;
	unsigned int  SmileTextId;
	unsigned char *SmileNamePtr;
	TForumTagHanler TagFunction;
} SMILE_INFO;

typedef struct {
	unsigned char *OperationStartMarkerPtr;
	unsigned char *OperationEndMarkerPtr;
	unsigned int  OperationTextId;
	unsigned char *OperationNamePtr;
	TForumTagHanler TagFunction;
} OPERATION_INFO;

void LoadForumTagHash();
void CloseForumTagHash();
void UserMessageHtmlConvert(char *UserMsgPtr, bool isTextOnly);
void ConvertTextForumView(char *UserMsgPtr, unsigned int MsgLen, bool isTextOnly);
void InitForumTagHash(FORUM_TAG_HASH_CHAR_HOP *NameHop);
bool AddForumTagHash(FORUM_TAG_HASH_CHAR_HOP *RootNameHop, char *ForumTag, void *TagRecordPtr, unsigned char TagType);
FORUM_TAG_HASH_RECORD* FindForumTagHash(FORUM_TAG_HASH_CHAR_HOP *RootNameHop, char *SearchLine);
bool RemForumTagHash(FORUM_TAG_HASH_CHAR_HOP *RootNameHop, char *ForumTag);
void CloseForumTagHashHop(FORUM_TAG_HASH_CHAR_HOP *SelHashHopCharPtr);
//---------------------------------------------------------------------------
#endif  /* if ! defined( ForumTagHashH ) */
