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

TRG	= SampleWebServer

WARN	= #-Wall -Wstrict-prototypes -Wmissing-prototypes
CC	= gcc $(WARN)
CFLAGS	= -O2 -fno-strength-reduce
LDFLAGS	= -D_LINUX_X86_
PFFLAGS = -D_REPORT_LOG_WEB_
LIBS	= -lc -lrt -lpthread -lnsl
LD	= $(CC)

RM	= rm -f
DEBUG	= -g
INCS	=	-I $(SERVER_VOB)/inc/ \
            -I $(SERVER_VOB)/../ServSysLib/inc/

SERVER_SRC =	$(SERVER_VOB)/src/

include platform/platform_${TARGET_OS}.mk

SRCS = \
$(SERVER_SRC)web/common/AdminActiveSessionList.c \
$(SERVER_SRC)web/common/AdminPasswdChangeHttpPage.c \
$(SERVER_SRC)web/common/AdminRegClientList.c \
$(SERVER_SRC)web/common/ExistUserAuth.c \
$(SERVER_SRC)web/common/ExistUserExit.c \
$(SERVER_SRC)web/common/HtmlConstData.c \
$(SERVER_SRC)web/common/LostPasswdHttpPage.c \
$(SERVER_SRC)web/common/MainWebPage.c \
$(SERVER_SRC)web/common/NewUserRegister.c \
$(SERVER_SRC)web/common/ServerConfigWebPage.c \
$(SERVER_SRC)web/common/UserContactManage.c \
$(SERVER_SRC)web/common/UserContactSet.c \
$(SERVER_SRC)web/common/UserRegisterHttpPage.c \
$(SERVER_SRC)web/common/CapchaRequest.c \
$(SERVER_SRC)web/system/GroupWebDataBase.c \
$(SERVER_SRC)web/system/MainSystemShowWebPage.c \
$(SERVER_SRC)database/SampleCustomConfigDataBase.c \
$(SERVER_SRC)WebServer.c \
$(SERVER_SRC)SiteMapFileGen.c \
$(SERVER_SRC)threads/ThrCernel.c \
$(SERVER_SRC)threads/web_server/ThrWebServer.c \
$(SERVER_SRC)threads/web_server/HandlerStopServerReq.c \
$(SERVER_SRC)threads/web_server/HandleStatsCheckTimerExp.c \

OBJS	=  $(SRCS:.c=.o)

.c.o: 
	$(CC) $(CFLAGS) $(LDFLAGS) $(PFFLAGS) $(DEBUG) -c $(INCS) $< -o $@

all: $(TRG)
$(TRG) : $(OBJS)
	$(LD) -o $@ $(OBJS) $(DEBUG) $(LDFLAGS) $(LIBS)

clean:
	$(RM) $(OBJS) $(TRG)
