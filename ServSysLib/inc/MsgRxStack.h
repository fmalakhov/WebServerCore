# if ! defined( MsgRxStackH )
#	define MsgRxStackH	/* only include me once */

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

#define PREFFIX_MARKER_LEN         8
#define SUFFIX_MARKER_LEN          8
#define MAX_OAM_SERV_CMD_LEN       8192

typedef void (*TOnMsgRxHandle)(void *DataPtr,  unsigned char *BufPtr, unsigned int Len);

typedef struct {
    /* RX info */
    TOnMsgRxHandle OnMsgReceive;
    void           *DataPtr;
    bool		   PrefixLoadDone;
    bool           SuffixLoadDone;
    unsigned int   OamReqLen; 
    char           PrefixLine[PREFFIX_MARKER_LEN];
    char           SuffixLine[SUFFIX_MARKER_LEN];
    char           ContrEnd[PREFFIX_MARKER_LEN+2];
    char           ContrSuffix[SUFFIX_MARKER_LEN+2]; 
    unsigned char  OamReqBuf[MAX_OAM_SERV_CMD_LEN+1]; 
} MSG_RX_STACK;

void MsgRxStackInit(MSG_RX_STACK *RxStackPtr);
void MsgRxStackReceive(MSG_RX_STACK *RxStackPtr, unsigned char *MsgBufPtr, unsigned int HandleBlockLen);
unsigned int MsgRxStackBlock(MSG_RX_STACK *RxStackPtr, unsigned char *RxBlockPtr, unsigned int BlockSize);
void MsgRxStackReset(MSG_RX_STACK *RxStackPtr);

//---------------------------------------------------------------------------
#endif  /* if ! defined( MsgRxStackH ) */
