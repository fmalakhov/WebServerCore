# if ! defined( SysMessagesH )
#	define SysMessagesH /* only include me once */

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

#define BAZE_SYS_MESS          2000
#define TM_TERMINATETHREAD     BAZE_SYS_MESS
#define TM_RESIVCADRNET        BAZE_SYS_MESS+1
#define TM_ONTIMEOUT           BAZE_SYS_MESS+2
#define TM_SETTIMEOUT          BAZE_SYS_MESS+3
#define TM_RESETCOUNTTIMER     BAZE_SYS_MESS+4
#define TM_RESETTIMEOUT        BAZE_SYS_MESS+5
#define	TM_GETTIMERINFO		   BAZE_SYS_MESS+6
#define TM_RESETGRPCOUNTTIMEMR BAZE_SYS_MESS+7 // Request counter for group of timers.
#define	TM_STOPSYSTIMER		   BAZE_SYS_MESS+8
//---------------------------------------------------------------------------
#endif  /* if ! defined( SysMessagesH ) */
