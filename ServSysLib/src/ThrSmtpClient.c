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

#include "ThrSmtpClient.h"
#include "TrTimeOutThread.h"
#include "ThrReportMen.h"
#include "WebServMsgApi.h"

static char EncodeSymbol[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char MailSet[] = "Content-type: text/plain; charset=windows-1251\r\n";
char ThrSendMailClient[] = "ThrSendMailClient";

static void Encode64Block(unsigned char *ConvBuf, unsigned char *EncodeBuf, int len);
//---------------------------------------------------------------------------
#ifdef WIN32
DWORD WINAPI THRSendMailClient(LPVOID Data)
#else
void* THRSendMailClient(void *Data)
#endif
{
    PARSENDMAIL	    *ParMailSend;
    int             skt_Smtp;
	int             iResult = 0;
	int             TcpErrCode = 0;
	int             UnicodeLen = 0;
	unsigned int    SentBodyBytes = 0;
	unsigned int    TxBlockSize;
	unsigned int    SentBodyLen;
	unsigned int    PageStartTxPoint;
	char            *BodySendPtr = NULL;
    char            *SslLine = NULL;
    struct sockaddr_in st_Sockaddr; 
    struct hostent	*phe;
	struct in_addr	addr;
    DWORD           dwTime;
    char            recv_Buf[MAX_SMTP_RX_MSG_LENGTH+1];
    char            send_Buf[MAX_SMTP_RX_MSG_LENGTH+1];
#ifdef _LINUX_X86_
	int	            sock_webserv;
#endif

    ParMailSend = (PARSENDMAIL*)Data;
	ParMailSend->Status = 0;
	ParMailSend->Phase = 0;

	DebugLogPrint(NULL, "%s: SMTP start sent mail via %s server %u port to %s user\n", 
		ThrSendMailClient, ParMailSend->ClientCfgPtr->SmtpServerName,
        ParMailSend->ClientCfgPtr->SmtpIpPort, (const char*)&ParMailSend->MailTo);
    
#ifdef _LINUX_X86_
	sock_webserv = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_webserv == -1)
	{
		DebugLogPrint(NULL, "%s: WEB server thread connect socket create is failed with error: %d\n", 
			ThrSendMailClient, errno);
		ParMailSend->Status = 1;
		SentSmtpThrStatus(ParMailSend);
	    pthread_exit((void *)0);
	}
	ParMailSend->ReqThrChan.DestThrPortNum = htons(ParMailSend->NotifyPort);
    ParMailSend->ReqThrChan.DestThrAddr = htonl(INADDR_LOOPBACK);
	ParMailSend->ReqThrChan.ThrSockfd = sock_webserv;
#endif

    /* For initial stat only  - if unix mail is tunned */            
    if (ParMailSend->ClientCfgPtr->MailClientUse)
    {
        sprintf(send_Buf, "echo \"%s\" | mail -s \"%s\" %s %s\n",
           ParMailSend->MailBody, ParMailSend->MailSubject, ParMailSend->MailFrom, ParMailSend->MailTo);
        iResult = system(send_Buf);
    
		ParMailSend->Phase = 31;
		ParMailSend->Status = 0;

        DebugLogPrint(NULL, "%s: SMTP Sent E-mail is done with %d status and %d phase\r\n",
		    ThrSendMailClient, ParMailSend->Status, ParMailSend->Phase);
	    SentSmtpThrStatus(ParMailSend);

    #ifdef WIN32
        ExitThread(0);
	    return 0;
    #else
        close(sock_webserv);
	    pthread_exit((void *)0);
    #endif
    }

    skt_Smtp = socket(AF_INET,SOCK_STREAM,0);
    if (skt_Smtp < 0)
    {
		DebugLogPrint(NULL," %s: Fail to open SNMP client socket\r\n", ThrSendMailClient);
		ParMailSend->Status = 1;    
		SentSmtpThrStatus(ParMailSend);        
#ifdef WIN32
        ExitThread(0);
		return 0;
#else
		close(sock_webserv);
        pthread_exit((void *)0);
#endif
    }

#ifndef _SUN_BUILD_
    ParMailSend->cert = NULL;
    ParMailSend->certname = NULL;
    ParMailSend->method = NULL;
    ParMailSend->ctx = NULL;
    ParMailSend->ssl = NULL;
#endif

	for(;;)
	{
	    dwTime = (DWORD)(ParMailSend->ClientCfgPtr->SmtpTimeout * 1000);

#ifdef WIN32          
	    iResult = setsockopt(skt_Smtp, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTime, sizeof(dwTime));

        if (iResult == SOCKET_ERROR)
	    {
            DebugLogPrint(NULL, "%s: SMTP client getsockopt for SO_RCVTIMEO failed with error: %u\n", 
				ThrSendMailClient, WSAGetLastError());
			ParMailSend->Status = 2;
			break;
		}

	    phe = gethostbyname(ParMailSend->ClientCfgPtr->SmtpServerName);
        if (phe == 0)
	    {
            DebugLogPrint(NULL, "%s: Fail to get SMTP server host by name\r\n", ThrSendMailClient);
		    ParMailSend->Status = 3;
			break;
	    }

        st_Sockaddr.sin_family = AF_INET;
	    st_Sockaddr.sin_port = htons((unsigned short)ParMailSend->ClientCfgPtr->SmtpIpPort);

        /* Get the IP address and initialize the structure */
	    if (!phe->h_addr_list[0])
	    {
            DebugLogPrint(NULL, "%s: Fail to find local address for SMTP server\r\n", ThrSendMailClient);
			ParMailSend->Status = 4;
			break;
	    }    
#endif

#ifdef _LINUX_X86_          
	    phe = gethostbyname(ParMailSend->ClientCfgPtr->SmtpServerName);
        if (phe == 0)
	    {
		    DebugLogPrint(NULL, "%s: Fail to get SMTP server host by name\r\n", ThrSendMailClient);
		    ParMailSend->Status = 3;
			break;
	    }

        st_Sockaddr.sin_family = AF_INET;
	    st_Sockaddr.sin_port = htons((unsigned short)ParMailSend->ClientCfgPtr->SmtpIpPort);

        /* Get the IP address and initialize the structure */
	    if (!phe->h_addr_list[0])
	    {
		    DebugLogPrint(NULL, "%s: Fail to find local address for SMTP server\r\n", ThrSendMailClient);
			ParMailSend->Status = 4;
			break;
	    }    
#endif

        memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr));
	    st_Sockaddr.sin_addr.s_addr = inet_addr(inet_ntoa(addr));
        iResult = connect(skt_Smtp, (struct sockaddr*) &st_Sockaddr, sizeof(st_Sockaddr));
#ifdef WIN32        
        if (iResult == SOCKET_ERROR)
	    {
            DebugLogPrint(NULL, "%s: SMTP client connect is failed with error: %u\n", 
				ThrSendMailClient, WSAGetLastError());
		    ParMailSend->Status = 5;
            break;
        }
#else
        if (iResult < 0)
	    {
		    DebugLogPrint(NULL, "%s: SMTP client connect is failed with error: %d\n", 
				ThrSendMailClient, errno);
		    ParMailSend->Status = 5;
            break;
        }
#endif
        if (ParMailSend->ClientCfgPtr->SmtpEncodeType != SMTP_ENCODE_NONE)
        {
#ifndef _SUN_BUILD_
            DebugLogPrint(NULL, "%s: SMTP mail should be sent over SSL or TLS\r\n",
		        ThrSendMailClient);            

			/* Set SSLv2 client hello, also announce SSLv3 and TLSv1 */
            if (ParMailSend->ClientCfgPtr->SmtpEncodeType == SMTP_ENCODE_TLS)
            {
                ParMailSend->method = TLSv1_method();
            }
            else
            {
                ParMailSend->method = SSLv23_client_method();
                //ParMailSend->method = SSLv3_client_method();  /* Create new client-method instance */
            }

            /* Try to create a new SSL context */
#ifdef _TPT_DEBIAN6_AMD64_
			if ( (ParMailSend->ctx = SSL_CTX_new((SSL_METHOD*)ParMailSend->method)) == NULL)
#else
            if ( (ParMailSend->ctx = SSL_CTX_new(ParMailSend->method)) == NULL)
#endif
            {
		        ParMailSend->Status = 5;            
                DebugLogPrint(NULL, "%s: SMTP (St: %u, Phase: %u) Unable to create a new SSL context structure.\r\n",
		            ThrSendMailClient, ParMailSend->Status, ParMailSend->Phase);            
                break;            
            }

            /* Disabling SSLv2 will leave v3 and TSLv1 for negotiation */
            if (ParMailSend->ClientCfgPtr->SmtpEncodeType == SMTP_ENCODE_SSL)
                SSL_CTX_set_options(ParMailSend->ctx, SSL_OP_NO_SSLv2);

            /* Create new SSL connection state object */
            ParMailSend->ssl = SSL_new(ParMailSend->ctx);
            
            /* Attach the SSL session to the socket descriptor */
            SSL_set_fd(ParMailSend->ssl, skt_Smtp);

            /* Try to SSL-connect here, returns 1 for success */
            if ( SSL_connect(ParMailSend->ssl) != 1)
            {
		        ParMailSend->Status = 5;            
                DebugLogPrint(NULL, "%s: SMTP (St: %u, Phase: %u) Could not build a SSL session.\r\n",
		            ThrSendMailClient, ParMailSend->Status, ParMailSend->Phase);            
                break;                        
            }
            else
            {
                DebugLogPrint(NULL, "%s: SMTP (St: %u, Phase: %u) Successfully enabled session with %s encryption\r\n",
		            ThrSendMailClient, ParMailSend->Status, ParMailSend->Phase, SSL_get_cipher(ParMailSend->ssl));            
            }

            /* Get the remote certificate into the X509 structure */
            ParMailSend->cert = SSL_get_peer_certificate(ParMailSend->ssl);
            if (!ParMailSend->cert)
            {
		        ParMailSend->Status = 5;            
                DebugLogPrint(NULL, "%s: SMTP (St: %u, Phase: %u) Could not get a certificate.\r\n",
		            ThrSendMailClient, ParMailSend->Status, ParMailSend->Phase);            
                break;                        
            }
            else
            {
                DebugLogPrint(NULL, "%s: SMTP (St: %u, Phase: %u) Retrieved the server's certificate.\r\n",
		            ThrSendMailClient, ParMailSend->Status, ParMailSend->Phase);            
            }

            /* extract various certificate information */
            SslLine = X509_NAME_oneline(X509_get_subject_name(ParMailSend->cert), 0, 0);
            if (SslLine)
            {
                /* display the cert subject here */
                DebugLogStrBufPrint(NULL, SslLine, "%s: SMTP Certificate subject: ",  ThrSendMailClient);
                free(SslLine);
            }
            else
            {
                DebugLogPrint(NULL, "%s: Fail to get SMTP Certificate subject\r\n",  ThrSendMailClient);
            }
            
            SslLine = X509_NAME_oneline(X509_get_issuer_name(ParMailSend->cert), 0, 0);
            if (SslLine)
            {
                /* display the cert issuer here */
                DebugLogStrBufPrint(NULL, SslLine, "%s: SMTP Certificate issuer: ",  ThrSendMailClient);
                free(SslLine);
            }
            else
            {
                DebugLogPrint(NULL, "%s: Fail to get SMTP Certificate issuer\r\n",  ThrSendMailClient);
            }
            
            X509_free(ParMailSend->cert);
            ParMailSend->cert = NULL;
            ParMailSend->SecureConnect = true;
#endif
        }
        else
        {
#ifdef _LINUX_X86_
	        FD_ZERO (&ParMailSend->master_rset);
            ParMailSend->maxfdp = 0;

            FD_SET (skt_Smtp, &ParMailSend->master_rset);
            if (skt_Smtp > ParMailSend->maxfdp) ParMailSend->maxfdp = skt_Smtp;
            ParMailSend->SecureConnect = false;
#endif
        }

		ParMailSend->Phase = 1;
	    ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	    if (ParMailSend->Status > 0) break;
		ParMailSend->Phase = 2;
		ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_SERVICE_REDY);
		if (ParMailSend->Status > 0) break;

        //Say Hello to the domain
		ParMailSend->Phase = 3;
		sprintf(send_Buf,"EHLO %s\r\n", ParMailSend->ClientCfgPtr->LocalIpAddr);
	    if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		ParMailSend->Phase = 4;
	    ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	    if (ParMailSend->Status > 0) break;

		ParMailSend->Phase = 5;
		ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_REQ_ACTION_CMPLT);
		if (ParMailSend->Status > 0) break;

        if (strlen(ParMailSend->ClientCfgPtr->MailLogin) > 0)
        {
		    ParMailSend->Phase = 6;
		    strcpy(send_Buf,"AUTH LOGIN\r\n");
	        if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		    /* Login request receive */
		    ParMailSend->Phase = 7;
	        ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	        if (ParMailSend->Status > 0) break;
		    ParMailSend->Phase = 8;
		    ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_TEXT_HAS_BASE64);
		    if (ParMailSend->Status > 0) break;

	        // Sent user's login
		    ParMailSend->Phase = 9;
		    StrBase64Encode(send_Buf, ParMailSend->ClientCfgPtr->MailLogin);
		    strcat(send_Buf,"\r\n");
	        if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		    /* Passwd requesr receive */
		    ParMailSend->Phase = 10;
	        ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	        if (ParMailSend->Status > 0) break;
		    ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_TEXT_HAS_BASE64);
		    if (ParMailSend->Status > 0) break;

	        /* Sent user's password */
		    ParMailSend->Phase = 11;
		    StrBase64Encode(send_Buf, ParMailSend->ClientCfgPtr->MailPasswd);
		    strcat(send_Buf,"\r\n");
	        if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		    ParMailSend->Phase = 12;
	        ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	        if (ParMailSend->Status > 0) break;
		    ParMailSend->Phase = 13;
		    ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_AUTH_SUCCESS);
		    if (ParMailSend->Status > 0) break;
        }

        /* Send from address */
        ParMailSend->Phase = 14;
	    sprintf(send_Buf, "MAIL FROM: %s\r\n", (const char*)&ParMailSend->MailFrom);
	    if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		ParMailSend->Phase = 15;
	    ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	    if (ParMailSend->Status > 0) break;

		ParMailSend->Phase = 16;
		ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_REQ_ACTION_CMPLT);
		if (ParMailSend->Status > 0) break;

        /* Send RCPT address */
		ParMailSend->Phase = 17;
	    sprintf(send_Buf,"RCPT TO:<%s>\r\n", (const char*)&ParMailSend->MailTo);
	    if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		ParMailSend->Phase = 18;
	    ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	    if (ParMailSend->Status > 0) break;

		ParMailSend->Phase = 19;
		ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_REQ_ACTION_CMPLT);
		if (ParMailSend->Status > 0) break;

        /* Send DATA */
		ParMailSend->Phase = 20;
        strcpy(send_Buf,"DATA\r\n");
	    if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		ParMailSend->Phase = 21;
	    ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	    if (ParMailSend->Status > 0) break;

		ParMailSend->Phase = 22;
		ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_START_EMAIL_INPUT);
		if (ParMailSend->Status > 0) break;

		/* Set mail charset mode */
		ParMailSend->Phase = 23;
		if (!SendDataSmtpServer(ParMailSend, MailSet,  skt_Smtp)) break;

        /* Send mail subject */
		ParMailSend->Phase = 24;
	    sprintf(send_Buf,"To: %s\r\nSubject: %s\r\n\r\n", 
		    ParMailSend->MailTo, ParMailSend->MailSubject);
	    if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		ParMailSend->Phase = 25;
	    PageStartTxPoint = 0;
	    BodySendPtr = ParMailSend->MailBody;	
	    SentBodyLen = strlen(BodySendPtr);
	    while(PageStartTxPoint < SentBodyLen)
	    {
		    if ((SentBodyLen-PageStartTxPoint) < MAX_SMTP_TX_MSG_LENGTH)
		    {
			    TxBlockSize = SentBodyLen-PageStartTxPoint;
		    }
		    else
		    {
			    TxBlockSize = MAX_SMTP_TX_MSG_LENGTH;
		    }
            if (ParMailSend->SecureConnect)
            {
			    if (!SslSendBufSmtpServer(ParMailSend, 
				        (unsigned char*)&BodySendPtr[PageStartTxPoint], TxBlockSize)) break;            
            }
            else
            {
			    if (!SendBufSmtpServer(ParMailSend, 
				        (unsigned char*)&BodySendPtr[PageStartTxPoint], 
				        skt_Smtp, TxBlockSize)) break;
            }
			SentBodyBytes += (unsigned int)TxBlockSize;
			PageStartTxPoint += TxBlockSize;
		    Sleep(1);
	    }
	    if (ParMailSend->Status > 0) break;

	    /* Send end mail body identifier */
		ParMailSend->Phase = 26;
	    strcpy(send_Buf,"\r\n.\r\n");
	    if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		ParMailSend->Phase = 27;
	    ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	    if (ParMailSend->Status > 0) break;

		ParMailSend->Phase = 28;
		ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_REQ_ACTION_CMPLT);
		if (ParMailSend->Status > 0) break;

        /* Send QUIT SMTP command */
		ParMailSend->Phase = 29;
        strcpy(send_Buf,"QUIT\n");
	    if (!SendDataSmtpServer(ParMailSend, send_Buf,  skt_Smtp)) break;

		ParMailSend->Phase = 30;
	    ParMailSend->Status = ReceiveDataSmtpServer(ParMailSend, recv_Buf,  skt_Smtp);
	    if (ParMailSend->Status > 0) break;

		ParMailSend->Phase = 31;
		ParMailSend->Status = MailServRetcodeCheck(ParMailSend, recv_Buf, SMTP_SERVICE_CLOSE);
	    break;
    }
    
    if (ParMailSend->ClientCfgPtr->SmtpEncodeType != SMTP_ENCODE_NONE)
    {
#ifndef _SUN_BUILD_
        if (ParMailSend->ssl)
        {
            DebugLogPrint(NULL, "%s: SMTP (St: %u, Phase: %u) Clsoed SSL/TLS connection with server.\r\n",
		        ThrSendMailClient, ParMailSend->Status, ParMailSend->Phase);                    
            SSL_free(ParMailSend->ssl);
        }
        if (ParMailSend->cert) X509_free(ParMailSend->cert);
        if (ParMailSend->ctx) SSL_CTX_free(ParMailSend->ctx);
        ParMailSend->ssl = NULL;
        ParMailSend->cert = NULL;
        ParMailSend->ctx = NULL;
#endif
    }
        
#ifdef WIN32
    closesocket(skt_Smtp);
#else
	close(skt_Smtp);
#endif

    DebugLogPrint(NULL, "%s: SMTP Sent E-mail is done with %d status and %d phase\r\n",
		ThrSendMailClient, ParMailSend->Status, ParMailSend->Phase);
	SentSmtpThrStatus(ParMailSend);

#ifdef WIN32
    ExitThread(0);
#else
    close(sock_webserv);
	pthread_exit((void *)0);
#endif
}
//---------------------------------------------------------------------------
void SentSmtpThrStatus(PARSENDMAIL *ParMailSendPtr)
{
	WebServerMsgSent(ParMailSendPtr->IDMessSendMailRes, (void*)ParMailSendPtr, 0);
}
//---------------------------------------------------------------------------
#ifdef WIN32
bool SendBufSmtpServer(PARSENDMAIL *ParMailSend, unsigned char *SendBufPtr,  int Socket, int BufLen)
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
                DebugLogPrint(NULL, "%s: SMTP [Ph:%d] TCP connection closed due to error: %d (%d TX: %d)\r\n",
					ThrSendMailClient, ParMailSend->Phase, TcpErrCode, Socket, DelieredLen);
				ParMailSend->Status = 8;
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
bool SendBufSmtpServer(PARSENDMAIL *ParMailSend, unsigned char *SendBufPtr,  int Socket, int BufLen)
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
		        DebugLogPrint(NULL, "%s: SMTP [Ph:%d] TCP connection closed due to error: %d (%d TX: %d)\r\n",
					ThrSendMailClient, ParMailSend->Phase, errno, Socket, DelieredLen);
				Status = false;
				ParMailSend->Status = 8;
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
bool SslSendBufSmtpServer(PARSENDMAIL *ParMailSend, unsigned char *SendBufPtr,  int BufLen)
{
	int SentLen, DelieredLen;
	int TextBufLen;
	int SslErrCode = 0;
	bool Status = true;

#ifndef _SUN_BUILD_
	DelieredLen = 0;
	TextBufLen = BufLen;
	for(;;)
	{
        SentLen = SSL_write(ParMailSend->ssl, (const char*)&SendBufPtr[DelieredLen], (TextBufLen - DelieredLen));
	    if (SentLen > 0)
        {
            /* Good block of data delivery over SSL */
        }
        else
	    {
		    DebugLogPrint(NULL, "%s: SMTP [Ph:%d] SSL connection closed due to error: %d (TX: %d)\r\n",
				ThrSendMailClient, ParMailSend->Phase, SSL_get_error(ParMailSend->ssl, SentLen), DelieredLen);
			Status = false;
			ParMailSend->Status = 8;
			break;
		}
		if (SentLen == TextBufLen) break;
		DelieredLen += SentLen;
	}
#endif
	return Status;
}
//---------------------------------------------------------------------------
bool SendDataSmtpServer(PARSENDMAIL *ParMailSend, char *SendBufPtr,  int Socket)
{
    if (ParMailSend->SecureConnect)
    {
        return SslSendBufSmtpServer(ParMailSend, (unsigned char*)SendBufPtr, strlen(SendBufPtr));
    }
    else
    {
	    return SendBufSmtpServer(ParMailSend, (unsigned char*)SendBufPtr,  Socket, strlen(SendBufPtr));
    }
}
//---------------------------------------------------------------------------
#ifdef WIN32
int TcpReceiveDataSmtpServer(PARSENDMAIL *ParMailSend, char *RecvBufPtr,  int Socket)
{
	int TcpErrCode = 0;
	int Status = 0;
    int r_len;

	for(;;)
	{
        r_len = recv(Socket, RecvBufPtr, MAX_SMTP_RX_MSG_LENGTH, 0);
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
                DebugLogPrint(NULL,	"%s: SMTP [Ph:%d] TCP (%d) RX connection closed due to timeout: %d (RX: %d)\r\n",
					ThrSendMailClient, ParMailSend->Phase, Socket, TcpErrCode, r_len);
				Status = 6;
				break;
		    }
		    else
		    {
                DebugLogPrint(NULL, "%s: SMTP [Ph:%d] TCP (%d) RX connection closed due to error: %d (RX: %d)\r\n",
					ThrSendMailClient, ParMailSend->Phase, Socket, TcpErrCode, r_len);
			    Status = 7;
				break;
		    }
		}
		else
		{
	        if (!strlen(RecvBufPtr))
			{
			    Status = 9;
		        DebugLogPrint(NULL, "%s: SMTP [Ph:%d] TCP (%d) Empty buffer is received (RX: %d)\r\n",
					ThrSendMailClient, ParMailSend->Phase, Socket, r_len);
			}
			break;
		}
	}
	if (!Status) RecvBufPtr[r_len] = 0;
	return Status;
}
#endif

#ifdef _LINUX_X86_
int TcpReceiveDataSmtpServer(PARSENDMAIL *ParMailSend, char *RecvBufPtr,  int Socket)
{
	int TcpErrCode = 0;
	int Status = 0;
    int r_len;
    struct timeval select_time;
    int	Select_result;

	for(;;)
	{
        memcpy(&ParMailSend->work_rset, &ParMailSend->master_rset, sizeof(ParMailSend->master_rset));
    
        select_time.tv_sec  = ParMailSend->ClientCfgPtr->SmtpTimeout;
        select_time.tv_usec = 0;

        Select_result = select(ParMailSend->maxfdp+1, &ParMailSend->work_rset, NULL, NULL, &select_time);
        if (Select_result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
		        DebugLogPrint(NULL, "%s: SMTP [Ph:%d] TCP (%d) socket select operation is failed: router is terminated\r\n",
					ThrSendMailClient, ParMailSend->Phase, Socket);
				Status = 20;
                break;
            }
        }
        else if (Select_result > 0)
        {
            if (FD_ISSET(Socket, &ParMailSend->work_rset))
            {
                r_len = recv(Socket, RecvBufPtr, MAX_SMTP_RX_MSG_LENGTH, 0);
                if (r_len < 0)
				{
		            if ((errno ==  EINTR) || (errno ==  EAGAIN))
					{
		                Sleep(1);
			            continue;
					}
		            else
					{
		                DebugLogPrint(NULL, "%s: SMTP [Ph:%d] TCP (%d) RX connection closed due to error: %d (RX: %d)\r\n",
					        ThrSendMailClient, ParMailSend->Phase, Socket, errno, r_len);
			            Status = 7;
			            break;
					}
				}
		        else
				{
	                if (!strlen(RecvBufPtr))
					{
						Status = 9;
		                DebugLogPrint(NULL, "%s: SMTP [Ph:%d] TCP (%d) Empty buffer is received (RX: %d)\r\n",
					        ThrSendMailClient, ParMailSend->Phase, Socket, r_len);
					}
			        break;
				}
			}
		}
		else
		{
            DebugLogPrint(NULL, "%s: SMTP [Ph:%d] TCP (%d) RX connection closed due to timeout: %d (RX: %d)\r\n",
				ThrSendMailClient, ParMailSend->Phase, Socket, TcpErrCode, r_len);
			Status = 6;
			break;
		}
	}
	if (!Status) RecvBufPtr[r_len] = 0;
	return Status;
}
#endif
//---------------------------------------------------------------------------
int SslReceiveDataSmtpServer(PARSENDMAIL *ParMailSend, char *RecvBufPtr)
{
	int TcpErrCode = 0;
	int Status = 0;
    int r_len, SslError;

    *RecvBufPtr = 0;
#ifndef _SUN_BUILD_
	for(;;)
	{
        r_len = SSL_read(ParMailSend->ssl, RecvBufPtr, MAX_SMTP_RX_MSG_LENGTH);
        if (r_len > 0)
        {
	        if (!strlen(RecvBufPtr))
			{
				Status = 9;
		        DebugLogPrint(NULL, "%s: SMTP [Ph:%d] SSL Empty buffer is received (RX: %d)\r\n",
					ThrSendMailClient, ParMailSend->Phase, r_len);
			}
			break;
        }
        else
        {
            SslError = SSL_get_error(ParMailSend->ssl, r_len); 
            if ((SslError == SSL_ERROR_WANT_READ) || (SslError == SSL_ERROR_WANT_WRITE))
            {
                Sleep(10);
                continue;
            }
            else
            {
		        DebugLogPrint(NULL, "%s: SMTP [Ph:%d] SSL RX connection closed due to error: %u (RX: %d)\r\n",
					ThrSendMailClient, ParMailSend->Phase, SslError, r_len);
			    Status = 7;
			    break;
            }
        }
	}
	if (!Status) RecvBufPtr[r_len] = 0;
#endif
	return Status;
}
//---------------------------------------------------------------------------
int ReceiveDataSmtpServer(PARSENDMAIL *ParMailSend, char *RecvBufPtr, int Socket)
{
    if (ParMailSend->SecureConnect)
    {
        return SslReceiveDataSmtpServer(ParMailSend, RecvBufPtr);
    }
    else
    {
	    return TcpReceiveDataSmtpServer(ParMailSend, RecvBufPtr,  Socket);
    }
}
//---------------------------------------------------------------------------
int MailServRetcodeCheck(PARSENDMAIL *ParMailSend, char *RecvBufPtr, int CheckCode)
{
    int Status = 0;
	int pars_read;
	int RetCode;

	for(;;)
	{
	    pars_read = sscanf(RecvBufPtr, "%d", &RetCode);
	    if (!pars_read)
	    {
			DebugLogPrint(NULL, "%s: SMTP [Ph:%d] TCP Failed to read return code from input buffer\r\n",
				ThrSendMailClient, ParMailSend->Phase);
            DebugLogStrBufPrint(NULL, RecvBufPtr, "%s: SMTP server response: ",  ThrSendMailClient);
		    Status = 10;
            break;
	    }
	    if (RetCode != CheckCode)
		{
			DebugLogPrint(NULL, "%s: SMTP [%d] TCP Ret code check is failed (%d, %d)\r\n",
				ThrSendMailClient, ParMailSend->Phase, RetCode, CheckCode);
            DebugLogStrBufPrint(NULL, RecvBufPtr, "%s: SMTP server response: ",  ThrSendMailClient);
			Status = 11;
		}
		break;
	}
	return Status;
}
//---------------------------------------------------------------------------
void StrBase64Encode(char *EncodeBuf, char *SrcText)
{
    bool          isLastBlock = false;
    int           EncPos = 0;
    int           SrcPos = 0;
    int           i, len;
    char          CVal;
    unsigned char ConvBuf[BASE_64_ENC_LEN];
    
    *EncodeBuf = 0;
    if (!SrcText || !strlen(SrcText)) return;
    while(!isLastBlock)
    {
        len = 0;
        memset(ConvBuf, 0, BASE_64_ENC_LEN);
        for(i = 0;i < BASE_64_ENC_LEN;i++) 
        {
            CVal = SrcText[SrcPos++];
            if(CVal != 0)
            {
                ConvBuf[i] = CVal;
                len++;
            }
            else
            {
                isLastBlock = true;
                break;
            }
        }
        
        if(len > 0)
        {
            Encode64Block(ConvBuf, &EncodeBuf[EncPos], len);
            EncPos += 4;
        }
   }
   EncodeBuf[EncPos] = 0;
}
//---------------------------------------------------------------------------
int AnsiToUnicodeConvert(char *AnsiBufPtr, unsigned char *UniBufPtr)
{
	unsigned int TextLen, UniLen = 0;
	unsigned int Index, AnsiInd;
	unsigned char AnsiChar; 
	char *RusAnsiPtr = NULL;
	char *BufPtr = NULL;

	    TextLen = strlen((const char*)AnsiBufPtr);
		AnsiInd = 0;
		UniLen = 2;
	    *UniBufPtr++ = 0xFE;
	    *UniBufPtr++ = 0xFF;
		for (Index=0;Index < TextLen;Index++)
		{
			AnsiChar = (unsigned char)AnsiBufPtr[Index];
			if (AnsiChar < 0xC0)
			{
				*UniBufPtr++ = 0x00;
			    *UniBufPtr++ = AnsiChar;
				UniLen += 2;
			}
			else
			{
				/* Convert to cirilic unicode */
			    *UniBufPtr++ = 0x04;
			    *UniBufPtr++ = 0x10 + (AnsiChar - 0xC0);
				UniLen += 2;
			}
		}
	return UniLen;
}
//---------------------------------------------------------------------------
static void Encode64Block(unsigned char *ConvBuf, unsigned char *EncodeBuf, int len)
{
    int Index;

    switch (len)
    {
        case 1:
            Index = (int)((ConvBuf[0] & 0xFC) >> 2);
            *EncodeBuf++ = EncodeSymbol[Index];
            Index = (int)(((ConvBuf[0] & 0x03) << 4) | ((ConvBuf[1] & 0xF0) >> 4));
            *EncodeBuf++ = EncodeSymbol[Index];
            *EncodeBuf++ = '=';
            *EncodeBuf++ = '=';
            break;

        case 2:
            Index = (int)((ConvBuf[0] & 0xFC) >> 2);
            *EncodeBuf++ = EncodeSymbol[Index];
            Index = (int)(((ConvBuf[0] & 0x03) << 4) | ((ConvBuf[1] & 0xF0) >> 4));
            *EncodeBuf++ = EncodeSymbol[Index];
            Index = (int)(((ConvBuf[1] & 0x0F) << 2) | ((ConvBuf[2] & 0xC0) >> 6));
            *EncodeBuf++ = EncodeSymbol[Index];    
            *EncodeBuf++ = '=';                
            break;
        
        default:
            Index = (int)((ConvBuf[0] & 0xFC) >> 2);
            *EncodeBuf++ = EncodeSymbol[Index];
            Index = (int)(((ConvBuf[0] & 0x03) << 4) | ((ConvBuf[1] & 0xF0) >> 4));
            *EncodeBuf++ = EncodeSymbol[Index];
            Index = (int)(((ConvBuf[1] & 0x0F) << 2) | ((ConvBuf[2] & 0xC0) >> 6));
            *EncodeBuf++ = EncodeSymbol[Index];    
            Index = (int)(ConvBuf[2] & 0x3F);
            *EncodeBuf++ = EncodeSymbol[Index];
            break;
    }
}
//---------------------------------------------------------------------------
#ifdef _LINUX_X86_
bool SendMailThreadCreate(PARSENDMAIL *ParSendMailPtr)
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
    if (pthread_create(&ParSendMailPtr->SmtpClient_thr, &attr,
		&THRSendMailClient, ParSendMailPtr) != 0)
    {
	    printf("SMTP cilent thread create with %d error!\n", errno);
		Result = false;
    }
	return Result;
}
#endif
//---------------------------------------------------------------------------

