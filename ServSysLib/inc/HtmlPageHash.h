# if ! defined( HtmlPageHashH )
#	define HtmlPageHashH	/* only include me once */

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

#define MAX_CHAR_HTML_PAGE_HASH_INDEX    56

typedef struct {
	void         *HtmlPageHandlerPtr;
	unsigned int UserTypeMask;
	void         *HashCharHopPtr;
} HTML_PAGE_HASH_RECORD;

typedef struct {
   unsigned int     UsedCharCount;
   unsigned int     ParentHashCharIndex;
   void             *ParentHashCharPtr;
   HTML_PAGE_HASH_RECORD *Record;
   void             *HashCharHop[MAX_CHAR_HTML_PAGE_HASH_INDEX+1];
} HTML_PAGE_HASH_CHAR_HOP;

void InitHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *NameHop);
void CloseHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *RootNameHop);
bool AddHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *RootNameHop, 
					 char *HtmlPageName, unsigned int UserTypeMask, void *HtmlHandlerPtr);
HTML_PAGE_HASH_RECORD* FindHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *RootNameHop,
										char *HtmlPageName, unsigned int UserTypeMask);
bool RemHtmlPageHash(HTML_PAGE_HASH_CHAR_HOP *RootNameHop, char *HtmlPageName);
void CloseHtmlPageHashHop(HTML_PAGE_HASH_CHAR_HOP *SelHashHopCharPtr);
//---------------------------------------------------------------------------
#endif  /* if ! defined( FileDataHashH ) */
