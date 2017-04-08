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

#include "OpenSslLock.h"

static bool isLockOpen = false;
#ifdef WIN32
static HANDLE *lock_cs;
#else
static pthread_mutex_t *lock_cs;
static long *lock_count;
#endif

static unsigned long pthreads_thread_id(void );
static void pthreads_locking_callback(int mode, int type, char *file, int line);
//---------------------------------------------------------------------------
void OpenSslLockCreate()
{
	unsigned int i;
#ifdef WIN32
	lock_cs = (HANDLE*)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(HANDLE));
	for (i=0; i < (unsigned int)CRYPTO_num_locks(); i++)
		lock_cs[i]=CreateMutex(NULL,FALSE,NULL);
	CRYPTO_set_locking_callback((void(*)(int, int, char*, int))pthreads_locking_callback);
#else
    /* Lock create */
	lock_cs = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
	lock_count = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(long));
	for (i=0; i < (unsigned int)CRYPTO_num_locks(); i++)
	{
		lock_count[i]=0;
		pthread_mutex_init(&(lock_cs[i]), NULL);
	}
	CRYPTO_set_id_callback((unsigned long (*)())pthreads_thread_id);
	CRYPTO_set_locking_callback((void (*)())pthreads_locking_callback);
#endif
	isLockOpen = true;
}
//---------------------------------------------------------------------------
void OpenSslLockClose()
{
	unsigned int i;

	if (!isLockOpen) return;
	CRYPTO_set_locking_callback(NULL);
#ifdef WIN32
	for (i=0; i < (unsigned int)CRYPTO_num_locks(); i++)
		CloseHandle(lock_cs[i]);
	OPENSSL_free(lock_cs);
#else
	for (i=0; i < (unsigned int)CRYPTO_num_locks(); i++)
		pthread_mutex_destroy(&(lock_cs[i]));
	OPENSSL_free(lock_cs);
	OPENSSL_free(lock_count);
#endif
	isLockOpen = false;
}
//---------------------------------------------------------------------------
static void pthreads_locking_callback(int mode, int type, char *file, int line)
{
#ifdef WIN32
	if (mode & CRYPTO_LOCK) WaitForSingleObject(lock_cs[type], INFINITE);
	else                    ReleaseMutex(lock_cs[type]);
#else
	if (mode & CRYPTO_LOCK)
	{
		pthread_mutex_lock(&(lock_cs[type]));
		lock_count[type]++;
	}
	else
	{
		pthread_mutex_unlock(&(lock_cs[type]));
	}
#endif
}
//---------------------------------------------------------------------------
static unsigned long pthreads_thread_id(void)
{
#ifdef _LINUX_X86_
	unsigned long ret;

	ret = (unsigned long)pthread_self();
	return(ret);
#endif
}
//---------------------------------------------------------------------------
