#!/bin/sh -e
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

if [ -z ${SERVER_VOB+x} ]; then echo "SERVER_VOB var is unset"; exit 0; else echo "SERVER_VOB var is set to '$SERVER_VOB'"; fi

echo "Target platform selection"
echo ""
echo "[1] Ubuntu for X86 64 bit"
echo "[2] Debian6 for AMD 64 bit"
echo "[3] RedHut for X86 64 bit"
echo "[4] CentOS for X86 64 bit"
echo "[5] Quit from install sheld"
echo ""
echo "Please select option? "

read answer
finish="-1"
while [ "$finish" = '-1' ]
do
  finish="1"
  if [ "$answer" = '' ];
  then
    answer=""
  else
    case $answer in
      1 ) answer="OK";TARGET_PLATFORM=ubuntu_x86_64;;
      2 ) answer="Ok";TARGET_PLATFORM=debian_amd64;;
      3 ) answer="OK";TARGET_PLATFORM=redhut_x86_64;;
	  4 ) answer="OK";TARGET_PLATFORM=centos_x86_64;;
      5 ) exit 0;;
      *) finish="-1";
         echo -n 'Invalid response -- please reenter:';
         read answer;;
     esac
  fi
  done
 
export TARGET_PLATFORM
TARGET_OS=serv_$TARGET_PLATFORM
export TARGET_OS

echo "Needs to build SampleWebServer_$TARGET_PLATFORM"
cd $SERVER_VOB
make clean > /dev/null
make -j24 > /dev/null
cp SampleWebServer SampleWebServer_$TARGET_PLATFORM
chmod 664 SampleWebServer_$TARGET_PLATFORM
make clean > /dev/null

echo "Create Sample initial structure"
cd $SERVER_VOB/install
mkdir sample
cd sample
mkdir web_server

echo "Create web server initial structure"
cd $SERVER_VOB/install/sample/web_server
mkdir LogFiles
mkdir ssl
cp -r $SERVER_VOB/CapchaImage .
cp -r $SERVER_VOB/html_template .
cp -r $SERVER_VOB/database .
cp -r $SERVER_VOB/WebData .
cp $SERVER_VOB/SampleWebServer_$TARGET_PLATFORM .
cp $SERVER_VOB/SampleWebServer.cfg .
chmod 664 SampleWebServer.cfg
chmod 755 SampleWebServer_$TARGET_PLATFORM
chmod 444 CapchaImage/*
chmod 664 database/*
chmod 444 html_template/*
cd WebData
chmod 755 images
chmod 755 scripts
chmod 444 images/*
chmod 444 scripts/*
chmod 444 robots.txt

echo "Create installation package"
cd $SERVER_VOB/install
tar -cvf sample_$TARGET_PLATFORM.tar sample > /dev/null
gzip sample_$TARGET_PLATFORM.tar > /dev/null
rm -rf sample

echo "Installation package preparation is done"

exit 0
