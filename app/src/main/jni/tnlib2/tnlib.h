#ifndef	__TNLIB_H
#define	__TNLIB_H

// 基本的なクラスライブラリ用

#if defined(NOWINDOWS) || defined(UNIX)
#undef _Windows
#endif

#if defined(_Windows)
#ifndef STRICT
	#define	STRICT
#endif
#if !defined(WIN32)
  #define	WIN32
#endif
#define	WIN32_LEAN_AND_MEAN
#define	CBUILDER
//#pragma	pack(push,1)
#endif

#ifndef __BC_H
#include	"bc.h"
#endif

#ifndef __TNDEFS_H
#include	"tndefs.h"
#endif

#ifdef _Windows
#define	OEMRESOURCE
#endif

#if defined(_Windows) && !defined(__WINS_H)
#include	"wins.h"
#endif

#ifndef __TNCOM_H
#include "tncom.h"
#endif

#if !defined(__TNWINLIB_H) && !defined(ANSI) && defined(_Windows)
#include	"tnwinlib.h"
#endif

#ifdef UNIX
#include "getstr.h"
#endif

//#include "mutex.h"

tchar *newstr( const tchar *str );
tchar *newstr( const tchar *str, int len );

#include	"fexist.h"
//#include	"str.h"

inline tchar *nstrcpy( tchar *buf, const tchar *src )
{
	return buf + _tcslen( _tcscpy( buf, src ) ) + 1;

}
#ifdef CBUILDER
//#pragma pack(pop)
#endif

#if 0
inline void* operator new(size_t size){
	void *ret = malloc(size);
	if (ret)
		DBW("new: %08X %08X %08X", ret, size, *(((unsigned*)&ret)+2));
	return ret;
}

inline void operator delete(void* pv) {
	if (pv)
		DBW("del: %08X %08X", pv, *(((unsigned*)&pv)+2));
	free(pv);
}
#endif

#endif	// __TNLIB_H

