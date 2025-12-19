#ifndef	__TNWINX_H
#define	__TNWINX_H

#include "tndefs.h"

void InitTNWinLib( HINSTANCE hInstance );
void FinishTNWinLib();
	
// 初期化関数

#define	LPSTRC	(LPSTR)(const char *)

void MoveCenter( RECT &r );

#include "getstr.h"
#include "tnstr.h"

// LoadString()の便利版
// 返り値がNULLのときはエラー
// 文字列は256文字まで('\0'を含めて）
// 同時に呼んで良い回数は４回まで
// InitTNWinLib()を呼ぶこと！！

extern HINSTANCE hTNInstance;

tnstr GetWindowTextStr( HWND hwnd );

void _cdecl EnableItemWindows( HWND hwnd, BOOL f, ... );
void _cdecl _EnableItemWindows( HWND hwnd, BOOL f, ... );
void EnableItemWindows( HWND hwnd, BOOL f, int *ids );
void _cdecl ShowItemWindows( HWND hwnd, BOOL f, ... );
void _cdecl _ShowItemWindows( HWND hwnd, BOOL f, ... );
void ShowItemWindows( HWND hwnd, BOOL f, int *ids );

#ifndef EXCLUDE_DBW
void DBW( const char *format, ... );
void D8( const void *data, int len );
void D16( const void *data, int len );
void D32( const void *data, int len );
void DBW( const wchar_t *format, ... );
#else
inline void DBW( const char *, ... ){}
inline void D8( const void *, int  ){}
inline void D16( const void *, int ){}
inline void D32( const void *, int ){}
inline void DFIN( const char * ){}
inline void DFOUT( const char * ){}
void DBW( const wchar_t *format, ... ){}
#endif

void dbwcls();

#define	dbw	DBW

#ifdef __ANDROID__
void DBe( const char *format, ... );
void DBw( const char *format, ... );
void DBi( const char *format, ... );
#else
#define	DBe	DBW
#define	DBw	DBW
#define	DBi	DBW
#endif

void alog_filename(const char *filename, char *pathbuf);
void alog_init(const char *filename);
void alog_exit();
void alog(const char *format, ...);
void elog(const char *format, ...);		// alog+DBW

bool tlog_init(const char *filename, int queSize, bool append);
void tlog_filename(const char *filename, char *pathbuf);
void tlog_exit();
void tlog(const char *format, ...);
void tlogs(const char *s, int len=-1);
void tlog_flush();

#ifdef	__cplusplus
extern "C" {
#endif
void    FAR _cdecl DEBUGOUTPUT(UINT flags, LPCSTR lpsz, ...);
#ifdef	__cplusplus
};
#endif



#ifdef WIN32
inline BOOL IsValidInstance( HINSTANCE hInst )
{
	return hInst != NULL;
}
#else
inline BOOL IsValidInstance( HINSTANCE hInst )
{
	return hInst >= HINSTANCE_ERROR;
}
#endif

#ifndef WINCE
inline HWND GetWindow_HWNDPARENT( HWND hwnd )
{
#if defined(WIN32)
	return (HWND)::GetWindowLong( hwnd, GWL_HWNDPARENT );
#else
	return (HWND)::GetWindowWord( hwnd, GWW_HWNDPARENT );
#endif
}

#if defined(WIN32)
// Win32ではうまくない？->やめる
#else
inline HINSTANCE GetWindow_HINSTANCE( HWND hwnd )
{
	return (HINSTANCE)::GetWindowWord( hwnd, GWW_HINSTANCE );
}
#endif
#endif	// !WINCE

inline UINT GetWindow_ID( HWND hwnd )
{
#if defined(WIN32)
	return GetWindowLong( hwnd, GWL_ID );
#else
	return GetWindowWord( hwnd, GWW_ID );
#endif
}

#ifndef WINCE
inline HBRUSH GetClass_HBRBACKGROUND( HWND hwnd )
{
#if defined(WIN32)
	return (HBRUSH)::GetClassLong( hwnd, GCL_HBRBACKGROUND );
#else
	return (HBRUSH)::GetClassWord( hwnd, GCW_HBRBACKGROUND );
#endif
}

inline void SetClass_HBRBACKGROUND( HWND hwnd, HBRUSH hbr )
{
#if defined(WIN32)
	::SetClassLong( hwnd, GCL_HBRBACKGROUND, (LONG)hbr );
#else
	::SetClassWord( hwnd, GCW_HBRBACKGROUND, (WORD)hbr );
#endif
}
#endif	// !WINCE


void IMEOpen( HWND hwnd, BOOL f );
BOOL IsIMEOpened( HWND hwnd );

HWND GetPreviousInstanceWindow( HINSTANCE hPrev );	// 既に起動しているインスタンスのウィンドウを返す

void Hourglass( BOOL bOn );
void ResetHourglass( );

#if defined(WIN32) && !defined(WINCE)
inline void MoveTo( HDC hdc, int x, int y )
{
	::MoveToEx( hdc, x, y, NULL );
}
#endif

// in TNWIN6.CPP
bool GetTemporaryFileName( const tchar *prefix, tnstr &filepath );	// 作業用ファイルの作成

// in TNWIN7.CPP
void LoadInitWindow( HWND hwnd, RECT &rcProf, UINT &showProf );
void SaveInitWindow( HWND hwnd, RECT &rcProf, UINT &showProf );

#if defined(WINCE) && defined(SUPPORT_LANG)
typedef int (WINAPI *FNWideCharToMultiByte)( UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL );
typedef int (WINAPI *FNMultiByteToWideChar)( UINT, DWORD, LPCSTR, int, LPWSTR, int );
extern FNWideCharToMultiByte _WideCharToMultiByte;
extern FNMultiByteToWideChar _MultiByteToWideChar;
#else
#define	_WideCharToMultiByte	WideCharToMultiByte
#define	_MultiByteToWideChar	MultiByteToWideChar
#endif

#endif	// __TNWINX_H

