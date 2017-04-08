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

#include "MsgRxStack.h"
//---------------------------------------------------------------------------
unsigned int MsgRxStackBlock(MSG_RX_STACK *RxStackPtr, unsigned char *RxBlockPtr, unsigned int BlockSize)
{
	unsigned int	i, j, k;
    unsigned int    BlockShift = 0;
    unsigned int    BlockLen = 0;    
    unsigned char   *SrcBuf = NULL;
    unsigned char   *DestBuf = NULL;

    BlockShift = 0;              
	if (!RxStackPtr->PrefixLoadDone)
	{
		for (i=0;i < BlockSize;i++)
		{
		    memcpy(&RxStackPtr->ContrEnd[0], &RxStackPtr->ContrEnd[1], PREFFIX_MARKER_LEN);
			RxStackPtr->ContrEnd[7] = RxBlockPtr[i];
			if (strncmp(RxStackPtr->ContrEnd, RxStackPtr->PrefixLine, PREFFIX_MARKER_LEN) == 0)
			{
			    RxStackPtr->PrefixLoadDone = true;
                i++;
                if (i < BlockSize)
                {
                    k = i;
                    for(;i < BlockSize;i++) 
                    {
		                memcpy(&RxStackPtr->ContrSuffix[0], &RxStackPtr->ContrSuffix[1], SUFFIX_MARKER_LEN);
			            RxStackPtr->ContrSuffix[7] = RxBlockPtr[i];
			            if (strncmp(RxStackPtr->ContrSuffix, RxStackPtr->SuffixLine, SUFFIX_MARKER_LEN) == 0)
						{
                            RxStackPtr->SuffixLoadDone = true; 
                            i++;
                            while(i < BlockSize)                                           
                            {
                                if (RxBlockPtr[i] != RxStackPtr->SuffixLine[0]) break;
                                i++;
                            }
                            break;
                        }
                    }
                    if (!RxStackPtr->SuffixLoadDone)
                    {
                        if ((RxStackPtr->OamReqLen+(BlockSize - k)) < MAX_OAM_SERV_CMD_LEN)
                        {
                            memcpy(&RxStackPtr->OamReqBuf[RxStackPtr->OamReqLen], &RxBlockPtr[k], (BlockSize - k));
                            RxStackPtr->OamReqLen += (BlockSize - k);
                        }
                        else
                        {
                            MsgRxStackReset(RxStackPtr);
                        }                    
                    }
                    else
                    {
                        BlockShift = k;                                        
                    }
                }
				break;
			}
		}
	}
	else
	{
        for(i=0;i < BlockSize;i++) 
        {
		    memcpy(&RxStackPtr->ContrSuffix[0], &RxStackPtr->ContrSuffix[1], SUFFIX_MARKER_LEN);
			RxStackPtr->ContrSuffix[7] = RxBlockPtr[i];
			if (strncmp(RxStackPtr->ContrSuffix, RxStackPtr->SuffixLine, SUFFIX_MARKER_LEN) == 0)
			{
                RxStackPtr->SuffixLoadDone = true;
                i++;
                while(i < BlockSize)                                           
                {
                    if (RxBlockPtr[i] != RxStackPtr->SuffixLine[0]) break;
                    i++;
                }                
                break;
            }
        }

        if (!RxStackPtr->SuffixLoadDone)
        {
            if ((RxStackPtr->OamReqLen+BlockSize) < MAX_OAM_SERV_CMD_LEN)
            {
                memcpy(&RxStackPtr->OamReqBuf[RxStackPtr->OamReqLen], RxBlockPtr, BlockSize);
                RxStackPtr->OamReqLen += BlockSize;
            }
            else
            {
                MsgRxStackReset(RxStackPtr);
            }
        }     
    }

    if (RxStackPtr->PrefixLoadDone && RxStackPtr->SuffixLoadDone)
    {            
        if (i > (BlockShift + SUFFIX_MARKER_LEN))
        {
            BlockLen = i - (BlockShift + SUFFIX_MARKER_LEN);
            memcpy(&RxStackPtr->OamReqBuf[RxStackPtr->OamReqLen], &RxBlockPtr[BlockShift], BlockLen);                            
            RxStackPtr->OamReqLen += BlockLen;
        }
        else
        {
            k = i;
            j = 0;
            while((k > 0) || (j < SUFFIX_MARKER_LEN))
            {
                if (RxBlockPtr[k-1] != RxStackPtr->SuffixLine[0]) break;
                k--;
                j++;
            }
            if (k > 0)
            {
                memcpy(&RxStackPtr->OamReqBuf[RxStackPtr->OamReqLen], &RxBlockPtr[BlockShift], k);
                RxStackPtr->OamReqLen += k;
            }
            RxStackPtr->OamReqLen -= (SUFFIX_MARKER_LEN - j);
        }     
 
        /* Check for prefix chars in begin of message body */  
        BlockShift = 0;
        while(RxStackPtr->OamReqBuf[BlockShift] == RxStackPtr->PrefixLine[0])
        {
            BlockShift++;
            RxStackPtr->OamReqLen--;
        }
        if ((BlockShift) > 0 && (RxStackPtr->OamReqLen > 0))
        {
            SrcBuf = RxStackPtr->OamReqBuf + BlockShift;
            DestBuf = RxStackPtr->OamReqBuf;
            for(i=0;i < RxStackPtr->OamReqLen;i++)
            {
                *DestBuf++ = *SrcBuf;
                SrcBuf++;
            }
        }        
        if (RxStackPtr->OnMsgReceive) (RxStackPtr->OnMsgReceive)(RxStackPtr->DataPtr, RxStackPtr->OamReqBuf, RxStackPtr->OamReqLen);
        MsgRxStackReset(RxStackPtr);
    }
    return (unsigned int)i;
}
//---------------------------------------------------------------------------
void MsgRxStackInit(MSG_RX_STACK *RxStackPtr)
{
    unsigned int i;

    RxStackPtr->DataPtr = NULL;
    RxStackPtr->OnMsgReceive = NULL;
    for(i=0;i < PREFFIX_MARKER_LEN;i++) RxStackPtr->PrefixLine[i] = '+';  
    for(i=0;i < SUFFIX_MARKER_LEN;i++)  RxStackPtr->SuffixLine[i] = '~';
    MsgRxStackReset(RxStackPtr);
}
//---------------------------------------------------------------------------
void MsgRxStackReceive(MSG_RX_STACK *RxStackPtr, unsigned char *MsgBufPtr, unsigned int HandleBlockLen)
{
    unsigned int    ShiftHandleData;
    unsigned int    ProcLen;

    /* Data request was received */
    ShiftHandleData = 0;
    for(;;)
    {   
        if (!HandleBlockLen) break;
        ProcLen = MsgRxStackBlock(RxStackPtr, &MsgBufPtr[ShiftHandleData], HandleBlockLen);
		if (!ProcLen) break;
        if (ProcLen >= HandleBlockLen) break;
        ShiftHandleData += ProcLen;
        HandleBlockLen = HandleBlockLen - ProcLen;                            
    }
}
//---------------------------------------------------------------------------
void MsgRxStackReset(MSG_RX_STACK *RxStackPtr)
{
    RxStackPtr->OamReqLen = 0;
    *RxStackPtr->OamReqBuf = 0;
    RxStackPtr->PrefixLoadDone = false;
    RxStackPtr->SuffixLoadDone = false;
    memset(&RxStackPtr->ContrEnd, 0, PREFFIX_MARKER_LEN*sizeof(char));
    memset(&RxStackPtr->ContrSuffix, 0, SUFFIX_MARKER_LEN*sizeof(char));
}
//---------------------------------------------------------------------------
