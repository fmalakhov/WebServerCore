###############################################################################
#Copyright (c) 2012-2017 MFBS, Fedor Malakhov
#
#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.
###############################################################################
SRCS = \
$(SERVER_VOB)/src/CpuUtilization.c \
$(SERVER_VOB)/src/MemoryPool.c \
$(SERVER_VOB)/src/PoolList.c \
$(SERVER_VOB)/src/SysLibTool.c \
$(SERVER_VOB)/src/BackupFilesRollback.c \
$(SERVER_VOB)/src/MemoryUtilization.c \
$(SERVER_VOB)/src/crc32.c \
$(SERVER_VOB)/src/EncodeKey.c \
$(SERVER_VOB)/src/MsgRxStack.c \
$(SERVER_VOB)/src/WavToneLoad.c \
$(SERVER_VOB)/src/ThrReportMen.c \
$(SERVER_VOB)/src/TrTimeOutThread.c \
$(SERVER_VOB)/src/ThrConnWeb.c \
$(SERVER_VOB)/src/ThrReadWebUser.c \
$(SERVER_VOB)/src/ThrSendWebUser.c \
$(SERVER_VOB)/src/ThrSmsClient.c \
$(SERVER_VOB)/src/ThrSmtpClient.c \
$(SERVER_VOB)/src/SysWebFunction.c \
$(SERVER_VOB)/src/deflate.c \
$(SERVER_VOB)/src/gzip.c \
$(SERVER_VOB)/src/trees.c \
$(SERVER_VOB)/src/IpAccessControl.c \
$(SERVER_VOB)/src/OpenSslLock.c \
$(SERVER_VOB)/src/HttpParser.c \
$(SERVER_VOB)/src/BaseWebServer.c \
$(SERVER_VOB)/src/hash/BadIpHash.c \
$(SERVER_VOB)/src/hash/FileDataHash.c \
$(SERVER_VOB)/src/hash/HtmlPageHash.c \
$(SERVER_VOB)/src/hash/SessionIpHash.c \
$(SERVER_VOB)/src/hash/BotNameHash.c \
$(SERVER_VOB)/src/hash/FileNameMapHash.c \
$(SERVER_VOB)/src/hash/ImageNameHash.c \
$(SERVER_VOB)/src/hash/SessionKeyHash.c \
$(SERVER_VOB)/src/hash/MpttParserHash.c \
$(SERVER_VOB)/src/hash/HtmlMacrosHash.c \
$(SERVER_VOB)/src/hash/MobileDeviceHash.c \
$(SERVER_VOB)/src/hash/ForumTagHash.c \
$(SERVER_VOB)/src/hash/ClientSocketHash.c \
$(SERVER_VOB)/src/database/BotDataBase.c \
$(SERVER_VOB)/src/database/SiteMapStaticUrlDataBase.c \
$(SERVER_VOB)/src/database/TextListDataBase.c \
$(SERVER_VOB)/src/database/MobileDeviceDataBase.c \
$(SERVER_VOB)/src/database/FileNameMapBase.c \
$(SERVER_VOB)/src/database/StatsDataBase.c \
$(SERVER_VOB)/src/database/ForumDataBase.c \
$(SERVER_VOB)/src/database/BackRequestDataBase.c \
$(SERVER_VOB)/src/database/UserInfoDataBase.c \
$(SERVER_VOB)/src/database/GroupDataBase.c \
$(SERVER_VOB)/src/database/CustomConfigDataBase.c \
$(SERVER_VOB)/src/stats/ClientRegisterHandle.c \
$(SERVER_VOB)/src/stats/ClientDeregisterHandle.c \
$(SERVER_VOB)/src/stats/ClientIdentHash.c \
$(SERVER_VOB)/src/stats/StatIncHandle.c \
$(SERVER_VOB)/src/stats/StatRegisterHandle.c \
$(SERVER_VOB)/src/stats/StatDeregisterHandle.c \
$(SERVER_VOB)/src/stats/StatMaxHandle.c \
$(SERVER_VOB)/src/stats/StatsCodeHash.c \
$(SERVER_VOB)/src/worker/ReaderWorker.c \
$(SERVER_VOB)/src/worker/SenderWorker.c \
$(SERVER_VOB)/src/worker/MailWorker.c \
$(SERVER_VOB)/src/worker/SmsWorker.c \
$(SERVER_VOB)/src/threads/ThrWebMgmt.c \
$(SERVER_VOB)/src/threads/web_server/ServerCpuUtilLog.c \
$(SERVER_VOB)/src/threads/web_server/ServerStats.c \
$(SERVER_VOB)/src/threads/web_server/HandleReadBadWebReq.c \
$(SERVER_VOB)/src/threads/web_server/Capcha.c \
$(SERVER_VOB)/src/threads/web_server/SetServerHttpAddr.c \
$(SERVER_VOB)/src/threads/web_server/WebServMsgApi.c \
$(SERVER_VOB)/src/threads/web_server/RemConnMsgApi.c \
$(SERVER_VOB)/src/threads/web_server/Session.c \
$(SERVER_VOB)/src/threads/web_server/HtmlTemplateParser.c \
$(SERVER_VOB)/src/threads/web_server/HttpResponseCreate.c \
$(SERVER_VOB)/src/threads/web_server/HandleReaderDataInd.c \
$(SERVER_VOB)/src/threads/web_server/HandleSendWorkerDoneInd.c \
$(SERVER_VOB)/src/web/ItemsListNavigation.c \
$(SERVER_VOB)/src/web/CommonWebApi.c \
$(SERVER_VOB)/src/web/BaseHtmlConstData.c \
$(SERVER_VOB)/src/web/CsvFileAccessValidate.c \
$(SERVER_VOB)/src/web/PdfFileAccessValidate.c \
$(SERVER_VOB)/src/web/PasswdSentMail.c \
$(SERVER_VOB)/src/web/AdminServStatsWebPage.c \
$(SERVER_VOB)/src/web/AdminBadIpDataBase.c \
$(SERVER_VOB)/src/web/AdminLogManager.c \
$(SERVER_VOB)/src/web/ContactsHttpPage.c \

RM	= rm -f
INCS    = -I $(SERVER_VOB)/inc/
include $(SERVER_VOB)/platform/platform_${TARGET_OS}.mk

OBJS	= $(SRCS:.c=.o)

all:  ${PROG} 

.c.o:
	$(CC) $(CFLAGS) $(DEBUG) -c $(INCS) $< -o $@

${PROG}: ${OBJS}
	${AR} $@ ${OBJS}; ${RANLIB} $@

clean:
	$(RM) $(OBJS) $(PROG)
