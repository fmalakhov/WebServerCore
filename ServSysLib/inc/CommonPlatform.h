# if ! defined( CommonPlatformH )
#	define CommonPlatformH	/* only include me once */

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

#ifdef WIN32
  #include <windows.h>
  #include <windowsx.h>
  #include <winsock.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <tchar.h>
  #include <direct.h>
  #include <sys/stat.h>
  #include <direct.h>
#endif

#ifdef _LINUX_X86_
  /* System Header Files */
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/wait.h>
  #include <sys/ipc.h>
  #include <sys/shm.h>
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <sys/time.h>
  #include <sys/timeb.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <semaphore.h>
  #include <pthread.h>
  #include <netdb.h>
  #include <errno.h>
  #include <string.h>
  #include <time.h>
  #include <unistd.h>
  #include <sched.h>
  #include <signal.h>

  #include <stdio.h>
  #include <locale.h>
  #include <stdarg.h>
  #include <stdlib.h>
#endif
//---------------------------------------------------------------------------
#endif  /* if ! defined( CommonPlatformH ) */
