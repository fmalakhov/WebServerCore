# if ! defined( HtmlMacrosHashH )
#	define HtmlMacrosHashH	/* only include me once */

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

#ifndef WebServInfoH
#include "WebServInfo.h"
#endif

#ifndef SysWebFunctionH
#include "SysWebFunction.h"
#endif

#define MAX_CHAR_HTML_MACROS_HASH_INDEX    36

typedef struct {
	void            *HtmlMacrosHandlerPtr;
	void            *HashCharHopPtr;
} HTML_MACROS_HASH_RECORD;

typedef struct {
   unsigned int     UsedCharCount;
   unsigned int     ParentHashCharIndex;
   void             *ParentHashCharPtr;
   HTML_MACROS_HASH_RECORD *Record;
   void             *HashCharHop[MAX_CHAR_HTML_MACROS_HASH_INDEX+1];
} HTML_MACROS_HASH_CHAR_HOP;

void InitHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *NameHop);
void CloseHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *RootNameHop);
bool AddHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *RootNameHop, 
					 char *HtmlPageName, void *HtmlMacrosHandlerPtr);
HTML_MACROS_HASH_RECORD* FindHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *RootNameHop,
										char *HtmlPageName);
bool RemHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *RootNameHop, char *HtmlPageName);
void CloseHtmlMacrosHashHop(HTML_MACROS_HASH_CHAR_HOP *SelHashHopCharPtr);
void InitHtmlMacrosCmdHash(HTML_MACROS_HASH_CHAR_HOP *NameHop, 
						   CMD_INFO* CmdListPtr, unsigned int NumRecList);
void InitHtmlBodyCmdHash(CMD_INFO* CmdListPtr, unsigned int NumRecList);
void InitHtmlLineCmdHash(CMD_INFO* CmdListPtr, unsigned int NumRecList);
void CloseHtmlBodyCmdHash();
void CloseHtmlLineCmdHash();
CMD_INFO* FindHtmlMacrosCmdHash(HTML_MACROS_HASH_CHAR_HOP *NameHop, char *HtmlMacrosName);
CMD_INFO* FindHtmlBodyCmdHash(char *HtmlMacrosName);
CMD_INFO* FindHtmlLineCmdHash(char *HtmlMacrosName);
//---------------------------------------------------------------------------
#endif  /* if ! defined( HtmlMacrosHashH ) */
