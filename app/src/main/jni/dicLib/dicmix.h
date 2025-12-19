#include "dicdef.h"

#ifndef __DICMIX_H
#define	__DICMIX_H

#ifndef MIXDIC
#define	MIXDIC	0
#endif

// DIC_BOCU
// _kchar	: BOCU+CodeTrans
// _mchar	: BOCU
// _jchar	: wchar_t
// _ktchar	: wchar_t (_kcharであるが、全文検索時のみwchar_t)
// _mtchar	: wchar_t（_mcharであるが、全文検索時のみwchar_t）

#if defined(DIC_UTF16)
#define	_msize	2
typedef wchar_t _mchar;
typedef wchar_t _muchar;
#define	_MTEXT(quote)	L##quote
#define	_MBYTETOLEN(x)	((x)>>1)
#define	_MLENTOBYTE(x)	((x)<<1)
#define	_mcsbyte(s)	(wcslen(s)<<1)
#define	_mcsbyte1(s)	(_mcsbyte(s)+sizeof(wchar_t))
#define	_mcscpy		wcscpy
#define	_mcscmp		wcscmp
#define	_mcslen		wcslen
#define	_mcsncpy	wcsncpy
#define	_mcsncmp	wcsncmp
#define	_mcsicmp	_wcsicmp
#define	_mcsnicmp	_wcsnicmp
#define	_mcschr		wcschr
#define	_mmbsnicmp	_wcsnicmp
#define	_mtoi		_wtoi

#else	// !DIC_UTF16

#define	_msize	1
typedef char _mchar;
typedef unsigned char _muchar;
#define	_MTEXT(str)	str
#define	_MBYTETOLEN
#define	_MLENTOBYTE
#define	_mcsbyte	strlen
#define	_mcsbyte1(x)	(strlen(x)+sizeof(char))
#define	_mcscpy		strcpy
#define	_mcscmp		strcmp
#define	_mcslen		strlen
#define	_mcsncpy	strncpy
#define	_mcsncmp	strncmp
#define	_mcsicmp	stricmp
#define	_mcsnicmp	strnicmp
#define	_mcschr		strchr
// for multi-byte functions
#ifdef DIC_UTF8
#define	_mmbsnicmp	strnicmp
#define	_mtoi		atoi
#define	_misleadbyte(c)	(((uint8_t)(c)>=0xC0)||((uint8_t)(c)<0x80)?true:false)
#define	_mcharlength(c)	((uint8_t)(c)<0x80?1:((uint8_t)(c)<0xE0?2:((uint8_t)(c)<0xF0?3:4)))
#else	// for shift-jis
#define	_mmbsnicmp	_mbsnicmp
#define	_misleadbyte(c)	_ismbblead(c)
#define	_mcharlength(c)	(_misleadbyte(c) ? 2:1)
#endif

#endif	// !DIC_UTF16

// kchar //
#ifdef DIC_BOCU
#define	_ksize	1
typedef char _kchar;
typedef unsigned char _kuchar;
typedef tchar _ktchar;
typedef tchar _mtchar;
#define	_KTEXT(quote)	quote
#define	_KTTEXT(quote)	_TEXT(quote)
#define	_KBYTETOLEN
#define	_KLENTOBYTE
#define	_kcsbyte	strlen
#define	_kcsbyte1(x)	(strlen(x)+sizeof(char))
#define	_kcscpy		strcpy
#define	_kcscmp		strcmp
#define	_kcslen		strlen
#define	_kcsncpy	strncpy
#define	_kcsncmp	strncmp
#define	_kstrcut	_mstrcut

#define	_ktcslen	_tcslen

#define	_mtsize		_tsize
#define	_MTLENTOBYTE	LENTOBYTE
#define	_mtnext		CharNextW
#define	_mtcschr	_tcschr
#define	_mtcsnicmp	_tcsnicmp
#define	_mtcsncmp	_tcsncmp
#define	_mtcslen	_tcslen
#define	_mtmbsnicmp	_tmbsnicmp

#else	// !DIC_BOCU

#define	_ksize		_msize
#define	_kchar		_mchar
#define	_ktchar		_mchar
#define	_mtchar		_mchar
#define	_kuchar		_muchar
#define	_KTEXT		_MTEXT
#define	_KBYTETOLEN	_MBYTETOLEN
#define	_KLENTOBYTE	_MLENTOBYTE
#define	_kcsbyte	_mcsbyte
#define	_kcsbyte1	_mcsbyte1
#define	_kcscpy		_mcscpy
#define	_kcscmp		_mcscmp
#define	_kcslen		_mcslen
#define	_kcsncpy	_mcsncpy
#define	_kcsncmp	_mcsncmp
#define	_kstrcut	_mstrcut

#define	_ktcslen	_mcslen

#define	_mtsize		_msize
#define	_MTLENTOBYTE	_MLENTOBYTE
#define	_mtnext		_mnext
#define	_mtcschr	_mcschr
#define	_mtcsnicmp	_mcsnicmp
#define	_mtcsncmp	_mcsncmp
#define	_mtcslen	_mcslen
#define	_mtmbsnicmp	_mmbsnicmp

#endif	// !DIC_BOCU


// for single byte charactor //
#ifndef NEED_SINGLEBYTE

#define	_nSingleByte	false
#define	_wSingleByte	false
#define	_jSingleByte	false
#define	_eSingleByte	false
#define	_pSingleByte	false

#else	// NEED_SINGLEBYTE

#define	_nSingleByte	false
#define	_wSingleByte	wSingleByte
#define	_jSingleByte	jSingleByte
#define	_eSingleByte	eSingleByte
#define	_pSingleByte	pSingleByte

#endif	// NEED_SINGLEBYTE

#if !MIXDIC
#define	_mstr2(x,y,s)	x,y
#define	_msstr(x,s)	(x)
//#define	_mstrdef(altname,orgname,s)
#else	// MIXDIC
#define	_msstr(x,s)	(char*)_mstr(x,s)
#ifdef NEED_SINGLEBYTE
#define	_mstr2(x,y,s)		_mstr(x,y,s)
#define	_mstrdef(altname,orgname,s)	__mstr altname( orgname, s );
#else	// !NEED_SINGLEBYTE
#define	_mstr2(x,y,s)		_mstr(x,y)
#define	_mstrdef(altname,orgname,s)	__mstr altname( orgname );
#endif	// !NEED_SINGLEBYTE
#endif	// MIXDIC

#ifdef KMIXDIC
#define	_kwstrdef(altname,orgname)	__kstr altname( orgname, GetKCodeTrans() );
#define	_kmstrdef(altname,orgname)	__kstr altname( orgname, &KCodeTranslateSetN );
#else	// !KMIXDIC
#define	_kwstrdef		_mstrdef
#endif


#if defined(WINCE) || defined(__ANDROID__)
#include "altmbstr.h"
#else
#include <mbstring.h>
#endif


// string routines //
#if !MIXDIC
#define	_MChar		tnstr
#define	_mnext		A_next
#define	_mlower		A_lower
#define	_mdiff		STR_DIFF

#else	 // MIXDIC

#define	_MChar		TCharSingle
#ifdef DIC_UTF16
#define	_mnext(p)	(((wchar_t*)(p))+1)
inline wchar_t _mlower( wchar_t us )
	{ return (wchar_t)::CharLowerW( (wchar_t *)us ); }
#define	_mdiff(x,y)		( (uint)( (wchar_t*)(x) - (wchar_t*)(y) ) )
#else	// !DIC_UTF16
inline char _mlower( char c )
	{ return (char)c >= 'A' && (char)c <= 'Z' ? c + 0x20 : c; }
#define	_mdiff(x,y)		( (uint)( (char*)(x) - (char*)(y) ) )
inline char *_mnext( const char *str )
{
	return (char*)str + _mcharlength(*(const unsigned char*)str);
}

#if 0
inline unsigned char *_mnext( const unsigned char *str )
{
	if ( _ismbblead(*(const unsigned char*)str) ){
		return (unsigned char*)str+2;
	} else {
		return (unsigned char*)str+1;
	}
}
#endif

#endif	// !DIC_UTF16

#endif	// MIXDIC

// kchar string routines //
#if defined(DIC_BOCU)
#define	_KChar		TCharSingle
#define	_KTChar		tnstr
#else	// !DIC_BOCU
#define	_KChar		_MChar
#define	_KTChar		_MChar
#endif	// !DIC_BOCU

#ifdef DIC_BOCU
#if defined(UNICODE) && !defined(__UTF8)
#define	bocu1EncodeT	bocu1Encode
#define	bocu1DecodeT	bocu1Decode
#else
#define	bocu1EncodeT	bocu1EncodeUTF8
#define	bocu1DecodeT	bocu1DecodeUTF8
#endif
#endif

typedef __tnstrA TCharSingle;

#if 0
class TCharSingle {
protected:
	char *buffer;
public:
	TCharSingle( const char *str );
	TCharSingle();
	~TCharSingle();
	void set( const char *str );
	void clear()
		{ buffer[0] = '\0'; }
	inline bool exist() const
		{ return buffer[0]?true:false; }
	char &operator[](int index)
		{ return buffer[index]; }
	operator const char *() const
		{ return buffer; }
	operator const unsigned char *() const
		{ return (unsigned char*)buffer; }
	void SetBuf( char *newbuf )
	{
		delete[] buffer;
		buffer = newbuf;
	}
	char *c_str()
		{ return buffer; }
	const char *operator = ( const char *str )
		{ set( str ); return buffer; }
	TCharSingle &operator = ( TCharSingle &s )
		{ set( (const char*)s ); return *this; }
	int operator == ( const char *str )
		{ return strcmp(buffer,str); }
	bool operator != ( const char *str )
		{ return strcmp(buffer,str)!=0; }
};
#endif

#if defined(DIC_UTF8) || MIXDIC
_mchar *_mstrcut( _mchar *str, size_t len );
#else
#define	_mstrcut	jstrcut
#endif

#if defined(UNICODE) || MIXDIC
#include "mstr.h"
#endif

#endif	// __DICMIX_H

