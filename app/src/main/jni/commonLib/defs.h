#ifndef	__DEFS_H
#define	__DEFS_H

#include "tndefs.h"
#ifdef __ANDROID__
#include "defs_android.h"
#endif

#ifndef _time_t
#define	_time_t	time_t		//TODO: 2038 year problem.
#endif

#ifndef __override
#define	__override	virtual
#endif

#ifdef	_Windows
#define	LPISFAR			// LPは32ビットである
#endif

#ifdef __WIN32__
#define	L_FILENAME	MAX_PATH		// ベース名＋'.'＋拡張子
#else
#define	L_FILENAME	MAX_PATH		// ベース名＋'.'＋拡張子
#endif

#ifndef MAXPATH
#define	MAXPATH	_MAX_PATH
#endif
#ifndef MAX_PATH
#define	MAX_PATH	MAXPATH
#endif

#ifdef _UNICODE
#define	_mt(x)	__mustr(x)
#define	CF_TTEXT	CF_UNICODETEXT
#else
#define	_mt(x)	(x)
#define	CF_TTEXT	CF_TEXT
#endif

// C++Builder6 対策
#if defined(__BORLANDC__) && (__BORLANDC__>=0x0560) && !defined(max)
//template <T> bool max(T x, T y) { return ( (x) > (y) ) ? (x) : (y); }
//#define	max(x,y)	( ( (x) > (y) ) ? (x) : (y) )
#endif

/*======================================*/
/*DOS用、Windowsポータビリティ確保define*/
/*======================================*/
#if defined(DOS) || defined(UNIX) || defined(STDLIB)

#define	VOID			void
#define	FAR
#define	NEAR
#ifndef PASCAL
#define	PASCAL
#endif
#ifndef CDECL
#define	CDECL
#endif

#if !defined(PDIC32) && !defined(SYSMAC_H) && !defined(__ANDROID__)
// Androidはdefs_android.hにて定義
typedef struct tagSIZE {
	LONG cx;
	LONG cy;
} SIZE;
#define	DEFINED_SIZE
#endif


typedef	int				HFILE;
#define HFILE_ERROR ((HFILE)-1)

//	標準ライブラリ far >> near 置換
#if defined(CYGWIN)
#define	lstrlen			strlen
#define	lstrcpy			strcpy
#define	lstrncpy		strncpy
#define	lstrcat			strcat
#define	lstrcmp			strcmp
#define	ljstrrchr		jstrrchr
#define	ljstrchr		jstrchr
#define	lmemcpy			memcpy
#define	lmemset			_fmemset
#endif

#ifdef UNIX
#define	lfstrcpy		strcpy
#define	lfstrlen		strlen
#define	lfstrcmp		strcmp
#else
#define	lfstrcpy		_fstrcpy
#define	lfstrlen		_fstrlen
#define	lfstrcmp		_fstrcmp
#endif

#else	// !(DOS || UNIX || STDLIB)

//	標準ライブラリ置換
#ifdef __WIN32__
#define	ljstrrchr		jstrrchr
#define	ljstrchr		jstrchr
#define	lstrncpy		strncpy
#define	lfstrcpy		_tcscpy
#define	lfstrlen		_tcslen
#define	lfstrcmp		_tcscmp
#define	lmemcpy			memcpy
#define	lmemset			memset
#else
#define	ljstrrchr		_fjstrrchr
#define	ljstrchr		_fjstrchr
#define	lstrncpy		_fstrncpy
#define	lfstrcpy		strcpy
#define	lfstrlen		strlen
#define	lfstrcmp		strcmp
#define	lmemcpy			_fmemcpy
#define	lmemset			_fmemset
#endif

#endif	// !(DOS || UNIX || STDLIB)

#ifdef _UNICODE
//#pragma	message( "UNICODE iskanji is not supported!!" );
#define	_tiskanji(x)	(x >= 0x2000)	// 2016.3.17 0x100→0x2000変更
#else
#define	_tiskanji		_ismbblead
#endif

// BC++で定義済み、MSCで未定義
#if defined(_MSC_VER) || defined(UNIX)
#define	_fstrcmp		strcmp
#define	_fmemcpy		memcpy
#define	_fmemset		memset
#endif

// PDIC専用追加 //////////////////////////

//#define min(a,b)    (((a) < (b)) ? (a) : (b))

#if !defined(_UNICODE) && defined(_Windows)
typedef	const TCHAR *PCSTR;
#endif

#if	defined(_Windows) || defined(UNIX)
inline tchar *GetNextPtr( const tchar *p )
{
	return NEXT_CHAR( p );
}
#ifdef _UNICODE
#define	NEXT_P( p )	(*(tchar**)&p)++
#else
#define	NEXT_P( p ) p = CharNext( p )
#endif
#else	// !_Windows
#define	NEXT_P( p )	if ( _ismbblead( *p ) ) p++; p++
inline tchar *GetNextPtr( const tchar *p )
{
	NEXT_P( p );
	return (tchar*)p;
}
#endif

#define	SETFLAGS( flags, flag, b )		if ( b ) flags |= (flag); else flags &= ~(flag);

// 戻り値に\0のpointerを返す
inline tchar *strcpy2( tchar *dest, const tchar *str )
{
	int len = _tcslen(str);
	memcpy( dest, str, (len+1)*sizeof(tchar) );
	return dest + len;
}

#ifdef _MSC_VER
inline tchar *_mbstok( tchar *buf, const tchar *str )
{
	return (tchar*)_tmbstok( (tuchar *)buf, (const tuchar*)str );
}
#if 0
inline int _mbsnicmp( const tchar *str1, const tchar *str2, unsigned len )
{
	return _tmbsnicmp( (const tuchar *)str1, (const tuchar*)str2, len );
}
#endif
int _mbsnicmp( const char *str1, const char *str2, unsigned len );
#endif

//////// Compatibility for WinCE ////////
#ifdef WINCE
#define	TPM_LEFTBUTTON			0
#endif

#include "tnstr.h"
#define	tnstrW			_tnstrW
#define	tnstrA			_tnstrA
#define	tnstrWx			_tnstrWx
#define	tnstrAx			_tnstrAx

// for VCL application
#if defined(_UNICODE)
#define	TString			_tnstrW
#define	TStringA		_tnstrA
#define	TStringW		_tnstrW
#else	// !defined(_UNICODE)
#define	TString			_tnstrA
#endif

// for TNChar to comprise with WideString
#define	c_ws	c_str

#define	def_cstr(name, str)	\
	const tchar *name = _t(str)

#ifdef __ANDROID__
#include <pthread.h>
#include <semaphore.h>
#define	USE_MUTEX		1	// for debug
#define	MUTEX_DEBUG		0	// lockするとerrorが発生する
#define	USE_MUTEX_ATTR	0	// adnroidではrecursiveが使用できない？
class TMutex {
protected:
	pthread_mutex_t _mutex;
#if USE_MUTEX_ATTR
	pthread_mutexattr_t attr;
#else
	pthread_mutex_t CounterMutex;
	int Counter;
	pthread_t LockThread;
#endif
public:
	TMutex()
	{
#if USE_MUTEX_ATTR
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_RECURSIVE_MUTEX_INITIALIZER);
		pthread_mutex_init(&_mutex, &attr);
#else
		pthread_mutex_init(&_mutex, NULL);
		pthread_mutex_init(&CounterMutex, NULL);
		Counter = 0;
		LockThread = 0;
#endif
	}
	~TMutex()
	{
		pthread_mutex_destroy(&_mutex);
#if USE_MUTEX_ATTR
		pthread_mutexattr_destroy(&attr);
#else
		pthread_mutex_destroy(&CounterMutex);
#endif
	}
#if MUTEX_DEBUG || !USE_MUTEX_ATTR
	void Lock();
#else
	inline void Lock()
	{
#if USE_MUTEX
		pthread_mutex_lock(&_mutex);
#endif
	}
#endif
#if USE_MUTEX_ATTR
	inline void Unlock()
	{
#if USE_MUTEX
		pthread_mutex_unlock(&_mutex);
#endif
	}
#else
	void Unlock();
#endif
	inline bool IsLocked()
	{
#if USE_MUTEX
		if (pthread_mutex_trylock(&_mutex)!=0){
			return true;
		}
		pthread_mutex_unlock(&_mutex);
#endif
		return false;
	}
	inline pthread_mutex_t *mutex() { return &_mutex; }
};


class TSem {
protected:
	sem_t sem;
public:
	TSem(int initial /*, int max*/)
	{
		sem_init(&sem, 0, initial);
	}
	~TSem()
	{
		sem_destroy(&sem);
	}
	inline void Lock()
	{
		sem_wait(&sem);
	}
	inline void Unlock()
	{
		sem_post(&sem);
	}
	inline void Signal()
	{
		sem_post(&sem);
	}
	bool Wait(int msec)
	{
		timespec to;
		to.tv_sec = time(NULL) + msec/1000;
		to.tv_nsec = (msec%1000) * 1000;
		return sem_timedwait(&sem, &to) == 0;
	}
};

//TODO: not debugged yet 2015.2.13
// inversed semaphore
// 0 : signal
// 1 : non-signal
class TInvSem {
	TMutex Mutex;
	pthread_cond_t cond_zero;
	int Count;
public:
	TInvSem()
	{
		Count = 0;
		pthread_cond_init( &cond_zero, NULL );
	}
	~TInvSem()
	{
		pthread_cond_destroy(&cond_zero);
	}
	void Lock()
	{
		Mutex.Lock();
		Count++;
		Mutex.Unlock();
	}
	void Unlock()
	{
		Mutex.Lock();
		if (--Count==0){
			pthread_cond_signal(&cond_zero);
		}
		Mutex.Unlock();
	}
	void Wait()
	{
		Mutex.Lock();
		while (Count!=0){
			pthread_cond_wait(&cond_zero, Mutex.mutex());
		}
		Mutex.Unlock();
	}
};

#else	// !__ANDROID__
class TMutex {
protected:
	CRITICAL_SECTION cs;
public:
	TMutex()
	{
		InitializeCriticalSection(&cs);
	}
	~TMutex()
	{
		DeleteCriticalSection(&cs);
	}
	void Lock()
	{
		EnterCriticalSection(&cs);
	}
	void Unlock()
	{
		LeaveCriticalSection(&cs);
	}
	bool IsLocked()
	{
		if (TryEnterCriticalSection(&cs)){
			LeaveCriticalSection(&cs);
			return true;
		}
		return false;
	}
};

class TSem {
protected:
	HANDLE hSem;
public:
	TSem(int initial, int max)
	{
		hSem = CreateSemaphore(NULL, initial, max, NULL);
	}
	~TSem()
	{
		CloseHandle(hSem);
	}
	inline void Lock()
	{
		WaitForSingleObject(hSem, INFINITE);
	}
	inline void Unlock()
	{
		ReleaseSemaphore(hSem, 1, NULL);
	}
	inline void Signal()
	{
		ReleaseSemaphore(hSem, 1, NULL);
	}
	bool Wait(int msec)
	{
		return WaitForSingleObject(hSem, msec)==WAIT_OBJECT_0;
	}
};

// inversed semaphore
// 0 : signal
// 1 : non-signal
class TInvSem {
	TMutex Mutex;
	HANDLE hEvent;
	int Count;
public:
	TInvSem()
	{
		Count = 0;
		hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	}
	~TInvSem()
	{
		CloseHandle(hEvent);
	}
	void Lock()
	{
		Mutex.Lock();
		Count++;
		ResetEvent(hEvent);
		Mutex.Unlock();
	}
	void Unlock()
	{
		Mutex.Lock();
		if (--Count==0){
			SetEvent(hEvent);
		}
		Mutex.Unlock();
	}
	void Wait()
	{
		WaitForSingleObject(hEvent, INFINITE);
	}
};

#endif	// !__ANDROID__

class TAutoLock {
protected:
	TMutex &Mutex;
public:
	TAutoLock(TMutex &mutex)
		:Mutex(mutex)
	{
		Mutex.Lock();
	}
	~TAutoLock()
	{
		Mutex.Unlock();
	}
};

// read operation -> no lock
// write operation -> lock
class TReadWriteLock {
protected:
	TMutex Mutex;
	TInvSem Sem;
public:
	void ReadLock()
	{
		Mutex.Lock();
		Sem.Lock();
		Mutex.Unlock();
	}
	void ReadUnlock()
	{
		Sem.Unlock();
	}
	void WriteLock()
	{
		Mutex.Lock();
		Sem.Wait();
	}
	void WriteUnlock()
	{
		Mutex.Unlock();
	}
};

template<class X>
class autolock {
protected:
	X &obj;
public:
	autolock(X &_obj)
		:obj(_obj)
	{
		obj.Lock();
	}
	~autolock()
	{
		obj.Unlock();
	}
};

#endif	// __DEFS_H

