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

#include "ThrSmsClient.h"
#include "TrTimeOutThread.h"
#include "ThrReportMen.h"
#include "SysWebFunction.h"
#include "TextListDataBase.h"
#include "WebServInfo.h"
#include "WebServMsgApi.h"

char ThrSendSmsName[]     = "ThrSendSmsClient";
char HttpSmsStart[]       = "POST http://";
char HttpSmsReq2[]        = "/sms/send?api_id=";
char HttpEndReq[]         = "HTTP/1.0\r\n\r\n";
char HttpSmsSentStatus[]  = "/sms/status?api_id=";
char HttpProxyConType[]   = "HTTP/1.0\r\nProxy-Connection: Keep-Alive\r\n";
//---------------------------------------------------------------------------
#ifdef WIN32
DWORD WINAPI ThrSendSmsClient(LPVOID Data)
#else
void* ThrSendSmsClient(void *Data)
#endif
{
    PARSENDSMS	    *ParSmsSend;
    int             skt_Sms;
    int             SmsWaitDeliveryCount = 0;
	int             iResult = 0;
	int             TcpErrCode = 0;
	int             UnicodeLen = 0;
	unsigned int    SentBodyBytes = 0;
	char            *BodySendPtr = NULL;
    char            *TextConvSmsPtr = NULL;
    char            *SrcSmsBodyPtr = NULL;
	char            StrLine[64];
    char            recv_Buf[MAX_SMS_RX_MSG_LENGTH];
    char            send_Buf[MAX_SMS_RX_MSG_LENGTH];

    bool            SmsSentDone = false;
    bool            SmsEndDelivery = false;
    bool            SmsStatusDone = false;
    int             i, j,  pars_read, TextLen, ParValue;
    int             MsgId1, MsgId2;

#ifdef _LINUX_X86_
	int	            sock_webserv;
#endif

    ParSmsSend = (PARSENDSMS*)Data;
	ParSmsSend->Status = 0;
#ifdef _LINUX_X86_
	sock_webserv = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_webserv == -1)
	{
		ParSmsSend->Status = 1;
		SentSmsThrStatus(ParSmsSend);
	    pthread_exit((void *)0);
	}
	ParSmsSend->ReqThrChan.DestThrPortNum = htons(ParSmsSend->NotifyPort);
    ParSmsSend->ReqThrChan.DestThrAddr = htonl(INADDR_LOOPBACK);
	ParSmsSend->ReqThrChan.ThrSockfd = sock_webserv;  
#endif

    DebugLogPrint(NULL, "%s: Start handle SMS for %s number (Proxy: %d, Addr: %s, Port: %d)\r\n", 
		ThrSendSmsName, ParSmsSend->PhoneNum.PhoneNum,
        ParSmsSend->SmsCfgPtr->isUsedProxy, ParSmsSend->SmsCfgPtr->ProxyServAddr,
        ParSmsSend->SmsCfgPtr->ProxyServPort);

    if (!SetSmsGateHostByName(ParSmsSend))
    {    
		SentSmsThrStatus(ParSmsSend);
#ifdef WIN32
        ExitThread(0);
		return 0;
#else
        pthread_exit((void *)0);
#endif
    }

	skt_Sms = OpenSmsConnect(ParSmsSend);
    if (!skt_Sms)
    {
#ifdef WIN32
        ExitThread(0);
		return 0;
#else
		close(sock_webserv);
        pthread_exit((void *)0);
#endif
    }
	for (;;)
	{
        strcpy(send_Buf, HttpSmsStart);
        strcat(send_Buf, ParSmsSend->SmsCfgPtr->SmsServerName);
		strcat(send_Buf, HttpSmsReq2);
		strcat(send_Buf, ParSmsSend->SmsCfgPtr->AccessId);
		if (strlen(ParSmsSend->SmsCfgPtr->SmsSrcName) > 0)
		{
            strcat(send_Buf, "&from=");
			strcat(send_Buf, ParSmsSend->SmsCfgPtr->SmsSrcName);
		}
		strcat(send_Buf, "&to=");
		strcat(send_Buf, ParSmsSend->PhoneNum.PhoneNum);
		strcat(send_Buf, "&text=");
		if (!strlen(ParSmsSend->SmsCfgPtr->SmsSrcName))
		{
		    strcat(send_Buf, ParSmsSend->UrlPtr);
		    strcat(send_Buf, ":+");
		}
        if (ParSmsSend->ConfirmKey && (strlen(ParSmsSend->ConfirmKey) > 0))
        {
            sprintf(StrLine, "One+time+confirmation+key:+%s+for+enter ", ParSmsSend->ConfirmKey);
            strcat(send_Buf, StrLine);
        }
        else if (ParSmsSend->SmsText && (strlen(ParSmsSend->SmsText) > 0))
        {
            /* change spaces to pluses for SMS text delivery */
            TextConvSmsPtr = &send_Buf[strlen(send_Buf)];
            SrcSmsBodyPtr = ParSmsSend->SmsText;
            for(i=0;i < (int)strlen(ParSmsSend->SmsText);i++)
            {
                if (*SrcSmsBodyPtr == ' ') *TextConvSmsPtr++ = '+';
                else                       *TextConvSmsPtr++ = *SrcSmsBodyPtr;
                SrcSmsBodyPtr++;
            }
            *TextConvSmsPtr++ = ' ';
            *TextConvSmsPtr = 0;
        }
        else
        {
		    if (ParSmsSend->CriticalAlarmsCount > 0)
		    {
		        sprintf(StrLine, "%d+CRITICAL+", ParSmsSend->CriticalAlarmsCount);
		        if (ParSmsSend->CriticalAlarmsCount > 1)
			    {
			        if (ParSmsSend->PhoneNum.isRusLangInfo)
				    {
				        SetOriginalRusTextBuf(send_Buf, SITE_RUS_SMS_ALM_NOTIFY_TXT_2_LINE_ID);
				        strcat(send_Buf, StrLine);
				        SetOriginalRusTextBuf(send_Buf, SITE_RUS_SMS_ALM_NOTIFY_TXT_4_LINE_ID);
				    }
			        else
				    {
				        strcat(send_Buf, StrLine);
			            strcat(send_Buf, "alarms+are+raised");
				    }
			    }
		        else
			    {
			        if (ParSmsSend->PhoneNum.isRusLangInfo)
				    {
				        SetOriginalRusTextBuf(send_Buf, SITE_RUS_SMS_ALM_NOTIFY_TXT_1_LINE_ID);
				        strcat(send_Buf, StrLine);
				        SetOriginalRusTextBuf(send_Buf, SITE_RUS_SMS_ALM_NOTIFY_TXT_3_LINE_ID);
				    }
			        else
				    {
				        strcat(send_Buf, StrLine);
		                strcat(send_Buf, "alarm+is+raised");
				    }
			    }
			    if (!ParSmsSend->MajorAlarmsCount) strcat(send_Buf, " ");
                else                               strcat(send_Buf, ",+");
		    }

		    if (ParSmsSend->MajorAlarmsCount > 0)
		    {
		        sprintf(StrLine, "%d+MAJOR+", ParSmsSend->MajorAlarmsCount);
		        if (ParSmsSend->MajorAlarmsCount > 1)
			    {
			        if (ParSmsSend->PhoneNum.isRusLangInfo)
				    {
				        SetOriginalRusTextBuf(send_Buf, SITE_RUS_SMS_ALM_NOTIFY_TXT_2_LINE_ID);
				        strcat(send_Buf, StrLine);
				        SetOriginalRusTextBuf(send_Buf, SITE_RUS_SMS_ALM_NOTIFY_TXT_4_LINE_ID);
				    }
			        else
				    {
				        strcat(send_Buf, StrLine);
			            strcat(send_Buf, "alarms+are+raised");
				    }
			    }
		        else
			    {
			        if (ParSmsSend->PhoneNum.isRusLangInfo)
				    {
				        SetOriginalRusTextBuf(send_Buf, SITE_RUS_SMS_ALM_NOTIFY_TXT_1_LINE_ID);
				        strcat(send_Buf, StrLine);
				        SetOriginalRusTextBuf(send_Buf, SITE_RUS_SMS_ALM_NOTIFY_TXT_3_LINE_ID);
				    }
			        else
				    {
				        strcat(send_Buf, StrLine);
		                strcat(send_Buf, "alarm+is+raised");
				    }
			    }
			    strcat(send_Buf, " ");
		    }
        }

        if (ParSmsSend->SmsCfgPtr->isUsedProxy)
        {
            strcat(send_Buf, HttpProxyConType);
            strcat(send_Buf, "Host: ");
            strcat(send_Buf, ParSmsSend->SmsCfgPtr->SmsServerName);
            strcat(send_Buf, "\r\n\r\n");
        }
        else
        {
		    strcat(send_Buf, HttpEndReq);        
        }

        if(!SendDataSmsServer(ParSmsSend, send_Buf,  skt_Sms))
		{
		    ParSmsSend->Status = 8;
		    break;
		}

	    ParSmsSend->Status = ReceiveDataSmsServer(ParSmsSend, recv_Buf,  skt_Sms);
	    if (ParSmsSend->Status > 0) break;
        for(;;)
		{
			ParSmsSend->Status = 40;
            i = FindCmdRequest(recv_Buf, "HTTP/1.1 ");
            if (i == -1) break;
			ParSmsSend->Status = 41;
	        TextLen = strlen(&recv_Buf[i]);
	        if (!TextLen) break;
			ParSmsSend->Status = 42;
	        pars_read = sscanf(&recv_Buf[i], "%d", &ParValue);
	        if (!pars_read) break;
			ParSmsSend->Status = 43;
            if (ParValue != 200)
            {
                DebugLogStrBufPrint(NULL, recv_Buf, "%s: Wrong response code (%d) SMS for %s number Resp=", 
		            ThrSendSmsName, ParValue, ParSmsSend->PhoneNum.PhoneNum);            
                break;
            }
			ParSmsSend->Status = 44;
            j = FindCmdRequest(&recv_Buf[i], "\r\n\r\n");
            if (j == -1) break;
            i += j;
			ParSmsSend->Status = 45;
            if(strlen(&recv_Buf[i]) < 5) break; 
            /* Read code of sms delivery to phone */
			ParSmsSend->Status = 46;
            pars_read = sscanf(&recv_Buf[i], "%d", &ParValue);
            if (!pars_read) break;
            if (ParValue != SMSDL_ADD_TO_DELIVERY)
			{
				ParSmsSend->Status = ParValue;
				break;
			}
			ParSmsSend->Status = 47;
            j = FindCmdRequest(&recv_Buf[i], "\n");
            if (j == -1) break;
            i += j;
			ParSmsSend->Status = 48;
            if(strlen(&recv_Buf[i]) < 3) break; 
            pars_read = sscanf(&recv_Buf[i], "%d-%d", &MsgId1, &MsgId2);
			ParSmsSend->Status = 49;
            if (pars_read < 2) break;
            ParSmsSend->Status = 0;
            SmsSentDone = true;
            break;
		}
		break;
	}

#ifdef WIN32
    closesocket(skt_Sms);
#else
	close(skt_Sms);
#endif
    if ((ParSmsSend->Status > 0) || (!SmsSentDone))
    {
        DebugLogPrint(NULL, "%s: Sent SMS for %s is failed with %d status\r\n", 
		    ThrSendSmsName, ParSmsSend->PhoneNum.PhoneNum, ParSmsSend->Status);    
		SentSmsThrStatus(ParSmsSend);
#ifdef _LINUX_X86_
		close(sock_webserv);
        pthread_exit((void *)0);
#else
        ExitThread(0);
		return 0;
#endif
    }

	for (;;)
	{
	  #ifdef _LINUX_X86_
		sleep(15);
      #endif
	  #ifdef WIN32
		Sleep(15000);
      #endif
        skt_Sms = OpenSmsConnect(ParSmsSend);
        if (!skt_Sms) break;

        strcpy(send_Buf, HttpSmsStart);
        strcat(send_Buf, ParSmsSend->SmsCfgPtr->SmsServerName);
		strcat(send_Buf, HttpSmsSentStatus);
		strcat(send_Buf, ParSmsSend->SmsCfgPtr->AccessId);
        sprintf(StrLine, "&id=%d-%d ", MsgId1, MsgId2);
        strcat(send_Buf, StrLine);
        strcat(send_Buf, HttpEndReq);

        if(!SendDataSmsServer(ParSmsSend, send_Buf,  skt_Sms))
		{
		    ParSmsSend->Status = 8;
		    break;
		}

	    ParSmsSend->Status = ReceiveDataSmsServer(ParSmsSend, recv_Buf,  skt_Sms);
	    if (ParSmsSend->Status > 0) break;

        SmsStatusDone = false;
        for(;;)
		{
            i = FindCmdRequest(recv_Buf, "HTTP/1.1 ");
			ParSmsSend->Status = 50;
            if (i == -1) break;
	        TextLen = strlen(&recv_Buf[i]);
			ParSmsSend->Status = 51;
	        if (!TextLen) break;
	        pars_read = sscanf(&recv_Buf[i], "%d", &ParValue);
			ParSmsSend->Status = 52;
	        if (!pars_read) break;
			ParSmsSend->Status = 53;
            if (ParValue != 200) break;
            j = FindCmdRequest(&recv_Buf[i], "\r\n\r\n");
			ParSmsSend->Status = 54;
            if (j == -1) break;
            i += j;
			ParSmsSend->Status = 55;
            if(strlen(&recv_Buf[i]) < 3) break; 
            
            /* Read code of sms delivery to first phone */
            pars_read = sscanf(&recv_Buf[i], "%d", &ParValue);
            ParSmsSend->Status = 56;
            if (!pars_read) break;

            DebugLogPrint(NULL, "%s: Received (%s) SMS %d status code\r\n", 
                ThrSendSmsName, ParSmsSend->PhoneNum.PhoneNum, ParValue);
           
            if (ParValue == SMSST_SMS_DELIVERED)
			{
                SmsEndDelivery = true;
                SmsStatusDone = true;
			    ParSmsSend->Status = 0;
			}
            else if ((ParValue == SMSST_SMS_IN_QUEUE) ||
                     (ParValue == SMSST_SMS_DELIV_OPER) ||
                     (ParValue == SMSST_SMS_SENT))
			{
                SmsStatusDone = true; 
				ParSmsSend->Status = 0;
			}
			else
			{
                ParSmsSend->Status = ParValue;
			}
            break;
		}

#ifdef _LINUX_X86_
	    if (skt_Sms > 0) close(skt_Sms);        
        if ((ParSmsSend->Status > 0) || (!SmsStatusDone))	
		{
		    close(sock_webserv);
            DebugLogPrint(NULL, "%s: Sent SMS for %s is done 2 with %d status\r\n", 
		        ThrSendSmsName, ParSmsSend->PhoneNum.PhoneNum, ParSmsSend->Status);
	        SentSmsThrStatus(ParSmsSend);            
            pthread_exit((void *)0);
        }
#endif
#ifdef WIN32
        if (skt_Sms > 0) closesocket(skt_Sms);
        if ((ParSmsSend->Status > 0) || (!SmsStatusDone))	
		{
            DebugLogPrint(NULL, "%s: Sent SMS for %s is done 2 with %d status\r\n", 
		        ThrSendSmsName, ParSmsSend->PhoneNum.PhoneNum, ParSmsSend->Status);
	        SentSmsThrStatus(ParSmsSend);            
            ExitThread(0);
		    return 0;
		}
#endif
        if (SmsEndDelivery) break;
        SmsWaitDeliveryCount++;
        if (SmsWaitDeliveryCount > MAX_RETRY_SMS_DELIVERY_WAIT)
        {
            ParSmsSend->Status = ParValue;
            break;
        }
    }
    DebugLogPrint(NULL, "%s: Sent SMS for %s is done 1 with %d status\r\n", 
		ThrSendSmsName, ParSmsSend->PhoneNum.PhoneNum, ParSmsSend->Status);
	SentSmsThrStatus(ParSmsSend);
#ifdef WIN32
    ExitThread(0);
	return 0;
#else
    close(sock_webserv);
	pthread_exit((void *)0);
#endif
}
//---------------------------------------------------------------------------
void SentSmsThrStatus(PARSENDSMS *ParSmsSendPtr)
{
	WebServerMsgSent(ParSmsSendPtr->IDMessSendSmsRes, (void*)ParSmsSendPtr, 0);
}
//---------------------------------------------------------------------------
#ifdef WIN32
bool SendBufSmsServer(PARSENDSMS *ParSmsSend, unsigned char *SendBufPtr,  int Socket, int BufLen)
{
	int SentLen, DelieredLen;
	int TextBufLen;
	int TcpErrCode = 0;
	bool Status = true;

	DelieredLen = 0;
	TextBufLen = BufLen;
	for(;;)
	{
        SentLen = send(Socket, (const char*)&SendBufPtr[DelieredLen], (TextBufLen - DelieredLen), 0);
	    if (SentLen == SOCKET_ERROR)
	    {
		    TcpErrCode = WSAGetLastError();
			if ((TcpErrCode == WSAEINPROGRESS) || (TcpErrCode == WSAENOBUFS))
			{
				Sleep(5);
                                continue;
			}
			else
			{
                DebugLogPrint(NULL, "%s: SMS TCP connection closed due to error: %d (%d TX: %d)\r\n",
					ThrSendSmsName, TcpErrCode, Socket, DelieredLen);
				Status = false;
				break;
			}
		}
		if (SentLen == TextBufLen) break;
		DelieredLen += SentLen;
	}
	return Status;
}
#endif

#ifdef _LINUX_X86_
bool SendBufSmsServer(PARSENDSMS *ParSmsSend, unsigned char *SendBufPtr,  int Socket, int BufLen)
{
	int SentLen, DelieredLen;
	int TextBufLen;
	int TcpErrCode = 0;
	bool Status = true;

	DelieredLen = 0;
	TextBufLen = BufLen;
	for(;;)
	{
        SentLen = send(Socket, (const char*)&SendBufPtr[DelieredLen], (TextBufLen - DelieredLen), 0);
	    if (SentLen < 0)
	    {
			if ((errno ==  EINTR) || (errno ==  EAGAIN))
			{
				Sleep(5);
                                continue;
			}
			else
			{
		        DebugLogPrint(NULL, "%s: SMS TCP connection closed due to error: %d (%d TX: %d)\r\n",
					ThrSendSmsName, errno, Socket, DelieredLen);
				Status = false;
				break;
			}
		}
		if (SentLen == TextBufLen) break;
		DelieredLen += SentLen;
	}
	return Status;
}
#endif

//---------------------------------------------------------------------------
bool SendDataSmsServer(PARSENDSMS *ParSmsSend, char *SendBufPtr,  int Socket)
{
	return SendBufSmsServer(ParSmsSend, (unsigned char*)SendBufPtr,  Socket, strlen(SendBufPtr));
}
//---------------------------------------------------------------------------
#ifdef WIN32
int ReceiveDataSmsServer(PARSENDSMS *ParSmsSend, char *RecvBufPtr,  int Socket)
{
	int TcpErrCode = 0;
	int Status = 0;
    int r_len;
	unsigned int RxLen = 0;
	char RxBuf[MAX_SMS_RX_MSG_LENGTH+1];

	for(;;)
	{
        r_len = recv(Socket, RxBuf, MAX_SMS_RX_MSG_LENGTH, 0);
        if (r_len == SOCKET_ERROR)
	    {
            TcpErrCode = WSAGetLastError();
		    if ((TcpErrCode == WSAEINPROGRESS) || (TcpErrCode == WSAENOBUFS))
		    {
		        Sleep(1);
			    continue;
		    }
		    else if (TcpErrCode == WSAETIMEDOUT)
		    {
                DebugLogPrint(NULL, "%s: SMS TCP (%d) RX connection closed due to timeout: %d (RX: %d)\r\n",
					ThrSendSmsName, Socket, TcpErrCode, r_len);
				Status = 6;
				break;
		    }
		    else
		    {
                DebugLogPrint(NULL, "%s: SMS TCP (%d) RX connection closed due to error: %d (RX: %d)\r\n",
					ThrSendSmsName, Socket, TcpErrCode, r_len);
			    Status = 7;
				break;
		    }
		}
		else if (r_len == 0)
		{
            RecvBufPtr[RxLen] = 0;
			break;
		}
		else
		{
			if ((RxLen + r_len) < MAX_SMS_RX_MSG_LENGTH)
			{
                memcpy(&RecvBufPtr[RxLen], RxBuf, r_len);
			    RxLen += (unsigned int)r_len;
			}
			else
			{
				Status = 9;
				break;
			}
		}
	}
	return Status;
}
#endif

#ifdef _LINUX_X86_
int ReceiveDataSmsServer(PARSENDSMS *ParSmsSend, char *RecvBufPtr,  int Socket)
{
	int TcpErrCode = 0;
	int Status = 0;
    int r_len;
    struct timeval select_time;
    int	Select_result;
	unsigned int RxLen = 0;
	char RxBuf[MAX_SMS_RX_MSG_LENGTH+1];

	for(;;)
	{
        memcpy(&ParSmsSend->work_rset, &ParSmsSend->master_rset, sizeof(ParSmsSend->master_rset));
    
        select_time.tv_sec  = ParSmsSend->SmsCfgPtr->SmsTimeout;
        select_time.tv_usec = 0;

        Select_result = select(ParSmsSend->maxfdp+1, &ParSmsSend->work_rset, NULL, NULL, &select_time);
        if (Select_result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                printf("The socket select operation is failed: router is terminated\n");
                break;
            }
        }
        else if (Select_result > 0)
        {
            if (FD_ISSET(Socket, &ParSmsSend->work_rset))
            {
                r_len = recv(Socket, RxBuf, MAX_SMS_RX_MSG_LENGTH, 0);
                if (r_len < 0)
				{
		            if ((errno ==  EINTR) || (errno ==  EAGAIN))
					{
		                Sleep(1);
			            continue;
					}
		            else
					{
		                DebugLogPrint(NULL, "%s: SMS TCP (%d) RX connection closed due to error: %d (RX: %d)\r\n",
					        ThrSendSmsName, Socket, errno, r_len);
			            Status = 7;
			            break;
					}
				}
		        else if (r_len == 0)
				{
                    RecvBufPtr[RxLen] = 0;
			        break;
				}
		        else
				{
			        if ((RxLen + r_len) < MAX_SMS_RX_MSG_LENGTH)
					{
                        memcpy(&RecvBufPtr[RxLen], RxBuf, r_len);
			            RxLen += (unsigned int)r_len;
					}
			        else
					{
				        Status = 9;
				        break;
					}
				}
			}
		}
		else
		{
            DebugLogPrint(NULL, "%s: SMS TCP (%d) RX connection closed due to timeout: %d (RX: %d)\r\n",
				ThrSendSmsName, Socket, TcpErrCode, r_len);
			Status = 6;
			break;
		}
	}
	return Status;
}
#endif
//---------------------------------------------------------------------------
int OpenSmsConnect(PARSENDSMS *ParSmsSend)
{
    int             skt_Sms = 0;
	int             iResult = 0;
    DWORD           dwTime;

    skt_Sms = socket(AF_INET,SOCK_STREAM,0);
    if (skt_Sms < 0)
    {
		DebugLogPrint(NULL, "%s: Fail to open SMS client socket\r\n", ThrSendSmsName);
		ParSmsSend->Status = 1;
        return 0;
    }

	for(;;)
	{
	    dwTime = (DWORD)(ParSmsSend->SmsCfgPtr->SmsTimeout * 1000);
            
#ifdef WIN32          
	    iResult = setsockopt(skt_Sms, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTime, sizeof(dwTime));

        if (iResult == SOCKET_ERROR)
	    {
            DebugLogPrint(NULL, "%s: SMS client getsockopt for SO_RCVTIMEO failed with error: %u\n", 
				ThrSendSmsName, WSAGetLastError());
			ParSmsSend->Status = 2;
			break;
		}
#endif        
        iResult = connect(skt_Sms, (struct sockaddr*) &ParSmsSend->st_Sockaddr, sizeof(struct sockaddr_in));
#ifdef WIN32        
        if (iResult == SOCKET_ERROR)
	    {
            DebugLogPrint(NULL, "%s: SMS client connect is failed with error: %u\n", 
				ThrSendSmsName, WSAGetLastError());
		    ParSmsSend->Status = 5;
            break;
        }
#endif        
#ifdef _LINUX_X86_
        if (iResult < 0)
	    {
		    DebugLogPrint(NULL, "%s: SMS client connect is failed with error: %d\n", 
				ThrSendSmsName, errno);
		    ParSmsSend->Status = 5;
            break;
        }

	    FD_ZERO (&ParSmsSend->master_rset);
        ParSmsSend->maxfdp = 0;

        FD_SET (skt_Sms, &ParSmsSend->master_rset);
        if (skt_Sms > ParSmsSend->maxfdp) ParSmsSend->maxfdp = skt_Sms;
#endif

		break;
	}
	if ((skt_Sms > 0) && (ParSmsSend->Status > 0)) 
	{
		SentSmsThrStatus(ParSmsSend);    
#ifdef _LINUX_X86_
		close(skt_Sms);
#endif
#ifdef WIN32
		closesocket(skt_Sms);
#endif
		skt_Sms = 0;
	}
	return skt_Sms;
}
//---------------------------------------------------------------------------
bool SetSmsGateHostByName(PARSENDSMS *ParSmsSend)
{
    struct hostent	*phe = NULL;
	struct in_addr	addr;

    if (ParSmsSend->SmsCfgPtr->isUsedProxy) phe = gethostbyname(ParSmsSend->SmsCfgPtr->ProxyServAddr);
	else                                    phe = gethostbyname(ParSmsSend->SmsCfgPtr->SmsServerName);
    
    if (phe == NULL)
	{
        DebugLogPrint(NULL, "%s: Fail to get SMS server host by name\r\n", ThrSendSmsName);
		ParSmsSend->Status = 3;
		return false;
	}

    ParSmsSend->st_Sockaddr.sin_family = AF_INET;
    if (ParSmsSend->SmsCfgPtr->isUsedProxy) ParSmsSend->st_Sockaddr.sin_port = htons((unsigned short)ParSmsSend->SmsCfgPtr->ProxyServPort);
    else                                    ParSmsSend->st_Sockaddr.sin_port = htons((unsigned short)ParSmsSend->SmsCfgPtr->SmsIpPort);

    /* Get the IP address and initialize the structure */
	if (!phe->h_addr_list[0])
	{
        DebugLogPrint(NULL, "%s: Fail to find local address for SMS server\r\n", ThrSendSmsName);
		ParSmsSend->Status = 4;
		return false;
	}    

    memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr));
	ParSmsSend->st_Sockaddr.sin_addr.s_addr = inet_addr(inet_ntoa(addr));
    return true;
}
//---------------------------------------------------------------------------
#ifdef _LINUX_X86_
bool SendSmsThreadCreate(PARSENDSMS *ParSendSmsPtr)
{
    pthread_attr_t	attr, *attrPtr = &attr;
    struct sched_param	sched;
	bool Result = true;
    size_t StackSize = 1024*1024;
	
    pthread_attr_init(attrPtr);
    (void)pthread_attr_setstacksize (attrPtr, StackSize);
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0)
    {
	    sched.sched_priority = 0;
	    pthread_attr_setschedparam(attrPtr, &sched);
    }		
    if (pthread_create(&ParSendSmsPtr->SmsClient_thr, &attr,
		&ThrSendSmsClient, ParSendSmsPtr) != 0)
    {
	    printf("SMS cilent thread create with %d error!\n", errno);
		Result = false;
    }
	return Result;
}
#endif
//---------------------------------------------------------------------------
