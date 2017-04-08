# if ! defined( SiteMapStaticUrlDataBaseH )
#	define SiteMapStaticUrlDataBaseH	/* only include me once */

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

#define MAX_LEN_SMINF_URL      512
#define MAX_LEN_SMINF_LCDATE   19
#define MAX_LEN_SMINF_PRIORITY 4

typedef struct {
	char *Url[MAX_LEN_SMINF_URL+1];
	char LastChangeDate[MAX_LEN_SMINF_LCDATE+1];
	char Priority[MAX_LEN_SMINF_PRIORITY+1];
} SITEMAP_URL_INFO;

void SiteMapStUrlDBLoad();
void SiteMapStUrlDBClear();

//---------------------------------------------------------------------------
#endif  /* if ! defined( SiteMapStaticUrlDataBaseH ) */
