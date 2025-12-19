#ifndef	__TNDEFS_H
#define	__TNDEFS_H

#if defined(_WIN64) || defined(__arm64__) || defined(__x86_64__) || defined(__aarch64__)
#define	__64
#endif

#ifdef __ANDROID__
#define	UNIX
#ifdef __BORLANDC__
#else	// !__BORLANDC__ = AndroidStudio, and so on...
#include <inttypes.h>
typedef int64_t __int64;
#define	_UNICODE
#define	UNICODE
#endif
#endif

#ifdef UNIX	// __ANDROID__

#if __cplusplus <= 201402L
typedef unsigned char byte;
#endif
typedef	unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char boolean;

#ifdef __64
typedef uint64_t uint_ptr;
#else
typedef unsigned uint_ptr;
#endif

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define	wchar_size	4
typedef wchar_t	utf32_t;
typedef unsigned short	utf16_t;

#include "utf.h"

#ifdef __UTF8

typedef char tchar;
typedef unsigned char tuchar;
#define	_t(x)	x
#define	_T(x)	(x)
#define	_TC(name, str)	__cstr(name, L##str)	// 文字列定数定義
#define	_TC1(name, str)	__cstr1(name, L##str)	// 文字列定数定義(１文字版)
//#define	tsizeof(type)	BYTETOLEN(sizeof(type))
#define	tsizeof(type)	sizeof(type)	//TODO: BYTETOLENが必要？
inline char *NEXT_CHAR( const char *str ){ return (char*)utf8CharNext(str); }

#define	LENTOBYTE(len)	(len)

#define	FP_DIFF( p1, p2 )	( (uint)( (char*)(p1) - (char*)(p2) ) )
#define	STR_DIFF( p1, p2 )	( (uint)( (char*)(p1) - (char*)(p2) ) )	//TODO: 未対応

#else	// !__UTF8 (UTF16)

typedef wchar_t tchar;
typedef tchar tuchar;
#define	_t(x)	L##x
#define	_T(x)	L##x
#define	_TC(name, str)	const tchar *name = L##str		// 文字列定数定義
#define	_TC1(name, str)	const tchar *name = L##str		// 文字列定数定義(１文字版)
#define	_TW(str)	((unsigned short)(_T(str)[0]))	// ２バイト文字(ushort)
//#define	tsizeof(type)	BYTETOLEN(sizeof(type))
#define	tsizeof(type)	BYTETOLEN(sizeof(type))

inline tchar *NEXT_CHAR( const tchar *str ){ return (tchar*)str+1; }	//TODO: サロゲートペア

#define	LENTOBYTE(x)		((x)*sizeof(tchar))
#define	BYTETOLEN(x)		((x)/sizeof(tchar))

#define	FP_DIFF( p1, p2 )	( (uint)( (char*)(p1) - (char*)(p2) ) )
#define	STR_DIFF( p1, p2 )	( (uint)( (tchar*)(p1) - (tchar*)(p2) ) )

#endif	// !__UTF8

#ifndef NULL
#define	NULL		(0)
#endif
#define	O_BINARY	0
#define	_MAX_PATH	256		//TODO: いくつ？

#define	__override	virtual
#define	__thread_unsafe

typedef tchar TCHAR;

#if !defined(CYGWIN)
typedef unsigned short	WORD;
typedef	unsigned long	DWORD;
#ifndef SYSMAC_H
typedef long			LONG;
#endif
typedef unsigned int	UINT;
typedef unsigned long	UINT_PTR;
typedef	unsigned char	BYTE;
#ifndef SYSMAC_H
typedef	int				BOOL;
#endif
#define	FALSE			0
#define	TRUE			1
// 以下はWindowsと非コンパチブル
typedef	const tchar *	LPCTSTR;
typedef	tchar *			LPTSTR;
typedef	tchar *			NPSTR;
typedef	void *			LPVOID;
typedef	DWORD*			LPDWORD;
typedef	long *			LPLONG;
typedef	WORD *			LPWORD;
typedef	int *			LPINT;
typedef	BYTE *			LPBYTE;
typedef	void *			LPVOID;
#endif	// !defined(CYGWIN)

typedef union _LARGE_INTEGER {
  struct {
	unsigned LowPart;
	int HighPart;
  };
  struct {
	unsigned LowPart;
	int HighPart;
  } u;
  __int64 QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

#ifdef __UTF8

#define	_ttoi			atoi
#define	_istdigit		isdigit

#define	__tcslen		strlen
#define	__tcscpy		strcpy
#define	__tcscmp		strcmp
#define	__tcsstr		strstr
#define	__tcstok		strtok
#define	__ttoi			atoi
#define	__totupper		_totupper
#define	_tcsdup			strdup

#define	_tcslen		strlen
#define	_tcscpy		strcpy
#define	_tcscat		strcat
#define	_tcsncpy	strncpy
#define	_tcscmp		strcmp
#define	_tcsncmp	strncmp
#define	_tcschr		strchr
#define	_tcsrchr	strrchr
#define	_tcsstr		strstr
#define	_tcsicmp	stricmp
#define	_tcsnicmp	strnicmp
#define	_tcscspn	strcspn
#define	_totupper	toupper
#define	_totlower	tolower
#define	_itot		itoa
#define	_tfullpath	_fullpath
#define	_tcstok		strtok

// 本当の文字列のバイト単位の長さ(最後の'\0'を含まない)
#define	_tcsbyte(x)		strlen(x)
// 本当の文字列のバイト単位の長さ(最後の'\0'を含む
#define	_tcsbyte1(x)	(strlen(x)+1)
#define	_tcslen1(x)		(strlen(x)+1)

#define	_tmbsnicmp		_tcsnicmp
#define	_tmbsicmp		_tcsicmp
#define	_tmbsrchr		_tcschr
#define	_tmbschr		_tcschr
#define	_tmbstok		_tcstok
#define	_tmbslen		_tcslen

#define	_tmkdir			mkdir

#else	// !__UTF8

#define	_ttoi			_wtoi
#define	_istdigit		iswdigit

#define	__tcslen		_tcslen
#define	__tcscpy		_tcscpy
#define	__tcscmp		_tcscmp
#define	__tcsstr		_tcsstr
#define	__tcstok		_tcstok
#define	__ttoi			_ttoi
#define	__totupper		_totupper

#define	_tcsdup		wcsdup
#define	_tcslen		wcslen
#define	_tcscpy		wcscpy
#define	_tcscat		wcscat
#define	_tcsncpy	wcsncpy
#define	_tcscmp		wcscmp
#define	_tcsncmp	wcsncmp
#define	_tcschr		wcschr
#define	_tcsrchr	wcsrchr
#define	_tcsstr		wcsstr
#define	_tcsicmp	wcsicmp
#define	_tcsnicmp	__wcsnicmp
#define	_tcscspn	wcscspn
#define	_snprintf	snprintf
#define	_totupper	towupper
#define	_totlower	towlower
#define	_itot		_itow
#define	_tfullpath	_wfullpath
#define	_tcstok		wcstok

// 本当の文字列のバイト単位の長さ(最後の'\0'を含まない)
#define	_tcsbyte(x)		LENTOBYTE(_tcslen(x))
// 本当の文字列のバイト単位の長さ(最後の'\0'を含む
#define	_tcsbyte1(x)	(_tcsbyte(x)+sizeof(tchar))
#define	_tcslen1(x)		(_tcslen(x)+1)

#define	_tmbsnicmp		_tcsnicmp
#define	_tmbsicmp		_tcsicmp
#define	_tmbsrchr		_tcschr
#define	_tmbschr		_tcschr
#define	_tmbstok		_tcstok
#define	_tmbslen		_tcslen

#define	_tmkdir			_wmkdir

#define	wsnprintf		swprintf
//#define	_vsnwprintf		vswprintf	// not supported in Android
#include <stdarg.h>
int _vsnwprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, va_list args);

#include <wchar.h>
#include <ctype.h>
#include "altmbstr.h"

#ifndef __BORLANDC__
#include <stdlib.h>
inline void srand(unsigned s) { srand48(s); }
inline int rand(void) { return (int)lrand48(); }
#endif

int wcsicmp(const wchar_t *s1, const wchar_t *s2);
int __wcsnicmp(const wchar_t *s1, const wchar_t *s2, int n);
char *itoa(int value, char *string, int radix);
tchar *_itow(int value, wchar_t *string, int radix);
int _wtoi(const wchar_t *s);
//wchar_t *wcstok(wchar_t *, const wchar_t *, wchar_t **);
wchar_t *wcstok(wchar_t *, const wchar_t *);	//TODO: multithread
wchar_t *wcsdup(const wchar_t *);
FILE *fopen(const wchar_t *filename, const char *mode);
int _wmkdir(const wchar_t *dirname);
wchar_t *_wfullpath(wchar_t *absPath, const wchar_t *relPath, size_t maxLength);
DWORD GetTempPath(DWORD size, LPTSTR path);
#if defined(UNICODE) && !defined(__UTF8)
#define	CharNext	CharNextW
#else
LPTSTR CharNext(LPCTSTR s);
#endif
wchar_t *CharNextW(const wchar_t *s);
BOOL IsDBCSLeadByte(BYTE c);
void Hourglass( BOOL bOn );
BOOL DeleteFile(const tchar *filename);

#endif	// !__UTF8

#include "unix.h"

#else	// !__ANDROID__

#if defined(_WIN32)
#ifndef _Windows
#define	_Windows
#endif
#ifndef _WINDOWS
#define	_WINDOWS
#endif
#endif

// WinCE関連 //
#ifdef _WIN32_WCE
#ifndef WINCE
#define	WINCE
#endif
#ifndef _Windows
#define	_Windows
#endif
#endif

#define	TNANSI	// Ansi codeを使用する

#if defined(_Windows) || defined(_WINDOWS)

#include <tchar.h>

#if defined(WIN32_PLATFORM_HPC2000)
// H/PC 2000
#define	WINHPC
#elif defined(WIN32_PLATFORM_HPCPRO)
// H/PC Pro
#define	WINHPC
#elif defined(WIN32_PLATFORM_PSPC)
// Pocket PC
#ifndef WINPPC
#define	WINPPC
#endif
#if (WIN32_PLATFORM_PSPC == 1)
// Pocket PC 2000
#ifndef PKTPC
#define	PKTPC
#endif
#elif (WIN32_PLATFORM_PSPC >= 310)
// Pocket PC 2002
#define	PKTPC
#else
// Some other Pocket PC
#endif
#elif defined(WIN32_PLATFORM_WFSP)
// Smartphone
#error	not supported
#else
// etc.
#endif

#ifdef _MSC_VER
#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )
#ifndef __WIN32__
#  define	__WIN32__
#endif
#endif
#ifdef __TURBOC__
#define DllImport	_import
#define DllExport	_export
#endif

#ifndef _Windows
#define _Windows
#endif

#ifndef _WINDOWS
#define	_WINDOWS
#endif

#ifndef STRICT
#define	STRICT
#endif

#if defined(__WIN32__) || defined(_WIN32)
#  ifndef WIN32
#    define	WIN32
#  endif
#else
#define	WIN16
#endif

#endif	// defined(_Windows) || defined(_WINDOWS)

// 各種モデル用define
#ifdef __EWL
#define	_WINDOWS
#define	WIN16
#define	STRICT
#define	WIN31
#define	NOUSEOWL
#endif

#ifdef OWL2
#define	_WINDOWS
#define	STRICT
#define	WIN31
#endif

#if defined(WIN16) && ( defined(__SMALL__) || defined(__MEDIUM__) )
#define	WIN16NP			// デフォルトのポインター長とLPが異なる
#endif

#define	TYPEDEF_UCHAR

typedef unsigned char byte;
typedef	unsigned char uchar;
typedef unsigned char uint8_t;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char boolean;
#ifdef _WIN64
typedef unsigned __int64 uint_ptr;
#else
typedef unsigned uint_ptr;
#endif

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define	wchar_size	2
typedef wchar_t	utf16_t;
typedef unsigned int	utf32_t;

#if !defined(_MSC_VER) && !(defined(__BORLANDC__) && __BORLANDC__>=0x630 && _Windows)	// __BORLANDC__のversion番号は適当
#define	__override	virtual
#endif

#ifdef UNIX
#define	STR_DIRSEP	"/"
#define	CHR_DIRSEP	'/'
#else
#define	STR_DIRSEP	"\\"
#define	CHR_DIRSEP	'\\'
#endif

#if defined(UNIX) && !defined(UNIX64)
#define	__int64		long long int
#endif

// Windows互換の宣言(終わり) ////////////////////////////////////////////////

#if defined(UNIX)
#define	_mbsnicmp	strnicmp
#define	_mbsicmp	stricmp
//#define	_mbsrchr(s,c)	strrchr((const char*)s, c)
//#define	_mbschr(s,c)	strchr((const char*)s, c)
#define	_mbstok		strtok
#define	_mbslen		strlen
#endif

#ifndef _Windows
#define	_tcslen		strlen
#define	_tcscpy		strcpy
#define	_tcscat		strcat
#define	_tcsncpy	strncpy
#define	_tcscmp		strcmp
#define	_tcschr		strchr
#define	_tcsstr		strstr
#define	_tcsicmp	stricmp
#define	_tcsnicmp	strnicmp
#define	_totupper	toupper
#define	_totlower	tolower
#define	_itot		itoa
#define	_tfullpath	_fullpath
#endif

// UNICODE互換の宣言 ////////////////////////////////////////////////

#define	IS_UNICODE_MARK(c)	((c)==0xFEFF || (c)==0xFFFE)

#if defined(_UNICODE)
#ifdef __UTF8
typedef char tchar;
typedef unsigned char tuchar;
//#define	_TW(str)	((unsigned short)(_T(str)[0]))	// ２バイト文字(ushort)
#define	_TC(name, str)	__cstr(name, L##str)	// 文字列定数定義
#define	_TC1(name, str)	__cstr1(name, L##str)	// 文字列定数定義(１文字版)
#define	_t(x)		x
#else	// UTF-16
typedef wchar_t tchar;
typedef tchar tuchar;
#define	_TW(str)	((unsigned short)(_T(str)[0]))		// ２バイト文字(ushort)
#define	_TC(name, str)	const tchar *name = L##str		// 文字列定数定義
#define	_TC1(name, str)	const tchar *name = L##str		// 文字列定数定義(１文字版)
#define	_t		_T
#endif
#else	// !UNICODE
typedef char tchar;
typedef unsigned char tuchar;
#ifndef _T
#define	_T(x)	(x)
#endif
#define	_t(x)	x
#define	_TW(str)	(((unsigned short*)_T(str))[0])		// ２バイト文字(ushort)
#define	_TC(name, str)	const tchar *name = str			// 文字列定数定義
#define	_TC1(name, str)	const tchar *name = str			// 文字列定数定義(１文字版)
#endif

// 本当の文字列のバイト単位の長さ(最後の'\0'を含まない)
#define	_tcsbyte(x)		LENTOBYTE(_tcslen(x))
// 本当の文字列のバイト単位の長さ(最後の'\0'を含む
#define	_tcsbyte1(x)	(_tcsbyte(x)+sizeof(tchar))
#define	_tcslen1(x)		(_tcslen(x)+1)

#ifdef _UNICODE
#define	_tmbsnicmp		_tcsnicmp
#define	_tmbsicmp		_tcsicmp
#define	_tmbsrchr		_tcschr
#define	_tmbschr		_tcschr
#define	_tmbstok		_tcstok
#define	_tmbslen		_tcslen
#else
#define	_tmbsnicmp		_mbsnicmp
#define	_tmbsicmp		_mbsicmp
#define	_tmbsrchr		_mbsrchr
#define	_tmbschr		_mbschr
#define	_tmbstok		_mbstok
#define	_tmbslen		_mbslen
#endif

#ifdef _UNICODE

//C++Builderの_totupper()にはバグがあるので使用禁止！
//_totupper()を使っている他のRTLも使用禁止！
// _tcsnicmp(), _tcsicmp(), _tcsupr()
#if defined(__BORLANDC__) && (__BORLANDC__>=0x0550)
#undef _totupper
#undef _tcsnicmp
#undef _tcsicmp
#undef _tcscmpi
#undef _tcsupr
#define	_totupper	__totupper
#define	_tcsnicmp	__tcsnicmp
#define	_tcsicmp	__tcsicmp
#define	_tcscmpi	__tcscmpi
#define	_tcsupr		__tcsupr
#endif

tchar __totupper(tchar ch);

#define	__tcsnicmp	__wcsnicmp
#define	__tcsicmp	__wcsicmp
#define	__tcscmpi	__wcsicmp

#endif	// _UNICODE

// UNICODE, UTF8, ANSIで共通なマクロ
#if defined(_UNICODE) && defined(__UTF8)
#define	__tcslen		strlen
#define	__tcscpy		strcpy
#define	__tcscmp		strcmp
#define	__tcsstr		strstr
#define	__tcstok		strtok
#define	__ttoi			atoi
#define	_tcsdup			strdup
#else
#define	__tcslen		_tcslen
#define	__tcscpy		_tcscpy
#define	__tcscmp		_tcscmp
#define	__tcsstr		_tcsstr
#define	__tcstok		_tcstok
#define	__ttoi			_ttoi
#ifndef _tcsdup
#define	_tcsdup			_wcsdup
#endif
#endif

#ifdef _MSC_VER
#ifdef _UNICODE
#define _topendir       wopendir
#define _treaddir       wreaddir
#define _trewinddir     wrewinddir
#define _tclosedir      wclosedir
#define _tDIR           wDIR
#define _tdirent        wdirent
#else
#define _topendir       opendir
#define _treaddir       readdir
#define _trewinddir     rewinddir
#define _tclosedir      closedir
#define _tDIR           DIR
#define _tdirent        dirent
#endif
#define	wDIR	_WDIR
#endif	// _MSC_VER

#define	AnsiToUnicode( str1, str2, tsize2 )		MultiByteToWideChar( CP_ACP, 0, str1, -1, str2, tsize2 )
#define	UnicodeToAnsi( str1, str2, tsize2 )		WideCharToMultiByte( CP_ACP, 0, str1, -1, str2, tsize2, NULL, NULL )

// temporary until 完全にTCHAR/tcharに対応するまで //
#define	LENTOBYTE		_TLENTOBYTE
#define	BYTETOLEN		_TBYTETOLEN

#ifdef _UNICODE
#define	_TBYTETOLEN(x)	((x)>>1)	// for TCHAR
#define	_TLENTOBYTE(x)	((x)<<1)
#ifdef __UTF8
#define	_tBYTETOLEN(x)	(x)			// for tchar
#define	_tLENTOBYTE(x)	(x)
#else
#define	_tBYTETOLEN(x)	((x)>>1)
#define	_tLENTOBYTE(x)	((x)<<1)
#endif
#else
#define	_TBYTETOLEN(x)	(x)			// for TCHAR
#define	_TLENTOBYTE(x)	(x)
#define	_tBYTETOLEN(x)	(x)			// for tchar
#define	_tLENTOBYTE(x)	(x)
#endif

#define	UNICODE_BYTEORDER	0xfeff

#define	tsizeof(type)	BYTETOLEN(sizeof(type))

#if !defined(DOS)
#define	FP_DIFF( p1, p2 )	( (uint)( (char*)(p1) - (char*)(p2) ) )
#define	STR_DIFF( p1, p2 )	( (uint)( (tchar*)(p1) - (tchar*)(p2) ) )
#else
#define	FP_DIFF( p1, p2 )	( FP_OFF( p1 ) - FP_OFF( p2 ) )
#endif


// 文字列処理
#if defined(__UTF8)
#include "utf.h"
inline char *NEXT_CHAR( const char *str ){ return (char*)utf8CharNext(str); }

#elif	defined(ANSI)
inline char *NEXT_CHAR( const char *str ){ return (char*)str+1; }	// 漢字コードは？？？？？？？？
#else	/* !ANSI */
#  ifdef _Windows
#    ifdef WIN32
#      define	NEXT_CHAR( s )	CharNext( s )
#    else
#      define	NEXT_CHAR( s )	AnsiNext( s )
#    endif
#  else	/* !_Windows */
#ifndef UNIX
#      include <mbctype.h>
#endif	/* !UNIX */
inline char *NEXT_CHAR( const char *str ){ if ( _ismbblead( *str ) ) return (char*)str+2; else return (char*)str+1; }
#  endif	/* !_Windows */
#endif		/* !ANSI */

#define	Bit2Bool( fVal, bit )	( ( fVal & bit ) == bit )
#define	Bool2Bit( bVal, bit )	( bVal ? bit : 0 ) 

// WinCEの代替処理
#ifdef WINCE
#define	InvertRect( hdc, rc )		PatBlt( hdc, (rc)->left, (rc)->top, (rc)->right, (rc)->bottom, DSTINVERT )
#define	TPM_RIGHTBUTTON				0
#endif
#endif	// !__ANDROID__

#define	UINT_PTR_MAX	((uint_ptr)-1)

#define	_delete( obj )	delete obj; obj = 0
#define	_delete_( obj )	delete[] obj; obj = 0
#define	__delete( obj )	_delete( obj )	// deprecated
#define	__delete_( obj )	_delete_( obj )	// deprecated

int __wcsicmp(const wchar_t *str1, const wchar_t *str2);
int __wcsnicmp(const wchar_t *str1, const wchar_t *str2, int n);

#define	map_find(obj, key) \
	( obj.find(key) != obj.end() )

#if 1
#ifndef _EXPLICIT
#define	_EXPLICIT
#endif
#ifndef _RWSTD_THROW_SPEC_NULL
#define	_RWSTD_THROW_SPEC_NULL
#endif
#ifndef _RWSTD_CONST_CAST
#define	_RWSTD_CONST_CAST(a,b)	const_cast<a>(b)
#endif

// very useful version of 'auto_ptr'
template<class X> class autoptr
{
//#ifndef _RWSTD_NO_MEM_CLASS_TEMPLATES
#if 0
	template <class Y> class autoptr_ref
	{
	public:
	  autoptr<Y>& p;
	  autoptr_ref(autoptr<Y>& a) : p(a) {}
	};
#endif

	public:
	typedef X element_type;

	//
	// construct/copy/destroy
	//
	_EXPLICIT autoptr (X* p = 0)  _RWSTD_THROW_SPEC_NULL
	 : the_p(p)
	{ ; }

	autoptr (autoptr<X>& a)  _RWSTD_THROW_SPEC_NULL
	 :  the_p((_RWSTD_CONST_CAST(autoptr<X>&,a)).release()) 
	{ ; }

	autoptr<X>& operator= (autoptr<X>& rhs)  _RWSTD_THROW_SPEC_NULL
	{ 
	  reset(rhs.release());
	  return *this;
	}
//#ifndef _RWSTD_NO_MEMBER_TEMPLATES
#if 0
	template <class Y>
	autoptr (autoptr<Y>& a)  _RWSTD_THROW_SPEC_NULL
	 : the_p((_RWSTD_CONST_CAST(autoptr<Y>&,a)).release()) 
	{ ; }

	template <class Y>
	autoptr<X>& operator= (autoptr<Y>& rhs)  _RWSTD_THROW_SPEC_NULL
	{ 
	  reset(rhs.release());
	  return *this;
	}
#endif

	~autoptr () _RWSTD_THROW_SPEC_NULL { delete the_p; }
	//
	// members
	//
	X& operator*  ()  const _RWSTD_THROW_SPEC_NULL { return *the_p;   }
	X* operator-> ()  const _RWSTD_THROW_SPEC_NULL { return the_p;    }
	X* get        ()  const _RWSTD_THROW_SPEC_NULL { return the_p;    }

	X* release    ()  _RWSTD_THROW_SPEC_NULL
	{ 
	  X* tmp = the_p;
	  the_p = 0; 
	  return tmp; 
	}

	void reset (X* p = 0) _RWSTD_THROW_SPEC_NULL
	{ 
	  if (the_p != p)
	  {
		delete the_p;
		the_p = p;
	  }
	}

//#ifndef _RWSTD_NO_MEM_CLASS_TEMPLATES
#if 0
	autoptr(autoptr_ref<X> r) _RWSTD_THROW_SPEC_NULL
	{
	  reset(r.p.release());
	}
#ifndef _RWSTD_NO_MEMBER_TEMPLATES
	template <class Y> operator autoptr_ref<Y>() _RWSTD_THROW_SPEC_NULL
	{
	  return autoptr_ref<Y>(*this);
	}
	template <class Y> operator autoptr<Y>() _RWSTD_THROW_SPEC_NULL
	{  
	  autoptr<Y> tmp;
	  tmp.reset(release());
	  return tmp;
	}
#endif // _RWSTD_NO_MEMBER_TEMPLATES
#endif // _RWSTD_NO_MEM_CLASS_TEMPLATES

	operator X *() { return the_p; }
	bool operator !() { return !the_p; }

	private:
	X* the_p;
};
#endif


#endif	// __TNDEFS_H

