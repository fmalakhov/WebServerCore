# if ! defined( TestListDataBaseH )
#	define TestListDataBaseH /* only include me once */

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

#ifndef vistypesH
#include "vistypes.h"
#endif

#ifndef SysLibToolH
#include "SysLibTool.h"
#endif

#ifndef SysWebFunctionH
#include "SysWebFunction.h"
#endif

#define MAX_RUS_LINES_LIST 1024

#define UPPER_RUS_A_CHAR  0xc0
#define LOWER_RUS_A_CHAR  0xe0
#define LOWER_RUS_YA_CHAR 0xff

typedef struct {
    void          *ParWebServ;
	unsigned char *ParBufPtr;
	char          *HtmlGenPtr;
} TEXT_CMD_PAR;

void SetRusTextBuf(char *BufAnsw, unsigned int RusTextIndex);
void SetRusTextBufName(char *BufAnsw, unsigned char *RusAnsiPtr);
int SetRusTextBufLen(char *BufAnsw, unsigned int RusTextIndex, unsigned int MaxLen);
void SetOriginalRusTextBuf(char *BufAnsw, unsigned int RusTextIndex);
void SetProtectRusTextBufName(char *BufAnsw, unsigned char *RusAnsiPtr);

//---------------------------------------------------------------------------
#endif  /* if ! defined( TestListDataBaseH ) */
