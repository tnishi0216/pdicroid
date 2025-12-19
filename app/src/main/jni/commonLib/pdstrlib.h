#ifndef __pdstrlib_h
#define	__pdstrlib_h

#include "tnstr.h"

// 汎用性の高いstrlibを置く
// いずれtnlibのstrlibへ移動する

bool _strequ(const wchar_t *s1, const wchar_t*s2);	// s1, s2 can be NULL ; _がついたstrequが汎用的というのは・・・
#ifdef __INC_VCL__
inline bool strequ(WideString &s1, const tchar *s2)
	{ return _strequ(s1.c_bstr(), s2); }
inline bool strequ(WideString &s1, WideString &s2)
	{ return _strequ(s1.c_bstr(), s2.c_bstr()); }
inline bool strequ(const tchar *s1, WideString &s2)
	{ return _strequ(s1, s2.c_bstr()); }
#endif

tchar *itocs( int val, tchar *text );
tnstr itocs(int val);
//tchar *utocs(unsigned val, tchar *text);
tchar *itocs64(__int64 val, tchar *text);
tnstr itocs64(__int64 val);
tchar *_itocs64(__int64 val, tchar *text, bool comma);
int atox( const tchar *str, tchar **next=NULL );
int atoin(const tchar *str, int len);
int atofp( const tchar *str, int digit );

const tchar *skiptospc(const tchar *str);
const char *skiptospc(const char *str);
const tchar *skiptospcz(const tchar *str);

tchar getlastchar(const tchar *str);

inline bool ishex(unsigned char c)
{
	if ((c>='0' && c<='9')
		|| (c>='A' && c<='F')
		|| (c>='a' && c<='f'))
		return true;
	return false;
}

// 文字列 //
tchar *BinToHex( const uint8_t *bin, int bytes );	// obsolete, use bintohex
tchar *bintohex( tchar *buffer, const uint8_t *bin, int bytes );
tnstr bintohex( const uint8_t *bin, int bytes );
bool ReplaceString( tchar *str, const tchar *target, const tchar *newstring );
tnstr replace(const tchar *str, const tchar *pat, const tchar *sub);
void ReplaceCRLF(tnstr &str);
void EscapeString( const tchar *str1, tnstr &str2 );
void EscapeURIString( const tchar *str1, tnstr &str2 );
void EscapeURIString( const char *str1, __tnstrA &str2 );
bool IsRegularChar(tchar c);
bool EscapeRegular(const tchar *str, tnstr &escstr, bool multibyte);
void NormalizeCRLF( const tchar *str, tchar *buf, const tchar *crlf );
tchar *bocu1DecodeStr( const uint8_t *data, const uint8_t **_end=NULL );

// Time //
tnstr time2str(timex_t t);

#if 0	// to tnlib
//C++Builderの_totupper()にはバグがあるので使用禁止！
//_totupper()を使っている他のRTLも使用禁止！
// _tcsnicmp(), _tcsicmp(), _tcsupr()
// 動作：
// _totupper()を呼び出すと、_ltowupper()が呼ばれる。
// locale->isCLocaleがゼロ以外である（通常）と 
// return _upper[ ch & 0x00ff];
// となり、戻り値は上位８ビットがクリアされている。
// そのため、ASCII以外の文字を使った場合、正しい結果が得られない。
inline tchar __totupper(tchar ch)
	{ DWORD c = ch; return (tchar)CharUpper((LPTSTR)c); }

#define	__tcsnicmp	__wcsnicmp
#define	__tcsicmp	__wcsicmp

int __wcsicmp(const wchar_t *str1, const wchar_t *str2);
int __wcsnicmp(const wchar_t *str1, const wchar_t *str2, int n);
#endif

#endif	// __pdstrlib_h

