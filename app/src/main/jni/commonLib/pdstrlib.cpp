#include "tnlib.h"
#pragma hdrstop
#include "tnstr.h"
#include "defs.h"
#include "pdstrlib.h"
#include "jtype.h"

bool _strequ(const wchar_t *s1, const wchar_t*s2)
{
	if (s1 && s2){
		return !_tcscmp(s1, s2);
	}
	if (!s1)
		return !s2 || !s2[0];
	if (!s2)
		return !s1 || !s1[0];
	return false;
}
/*======================================================================//
//	itocs()
//----------------------------------------------------------------------//
// Description:
//	unsigned 数値を10進数文字列に(３桁カンマ付き
//	textにバッファアドレス
//	戻り値は変換された文字列の先頭の位置
//	バッファは20バイト以上であること
// Arguments:
//
// Return Value:
//	コピーした文字列の先頭アドレス
//======================================================================*/
tchar *itocs( int val, tchar *text )
{
	int c = 0;
	text += 19;
	*text-- = '\0';
	bool sign;
	if (val<0){
		sign = true;
		val = -val;
	} else {
		sign = false;
	}
	while ( val != 0 ){
		if ( (c % 3 == 0) && c != 0 ){
			*text-- = ',';
		}
		*text = (tchar)((val % 10)+'0');
		val = val / 10;
		text--;
		c++;
	}
	if ( c == 0 ) *text-- = '0';
	else if (sign) *text-- = '-';
	return text+1;
}
#if 0	// 今のところ必要がないのでcomment out
tchar *utocs( unsigned val, tchar *text )
{
	int c = 0;
	text += 19;
	*text-- = '\0';
	while ( val != 0 ){
		if ( (c % 3 == 0) && c != 0 ){
			*text-- = ',';
		}
		*text = (tchar)((val % 10)+'0');
		val = val / 10;
		text--;
		c++;
	}
	if ( c == 0 ) *text-- = '0';
	return text+1;
}
#endif
tnstr itocs(int val)
{
	tchar buf[20];
	return itocs(val, buf);
}

// text should be more than 40 bytes.
tchar *itos64(__int64 val, tchar *text)
{
	return _itocs64(val, text, false);
}

tchar *itocs64(__int64 val, tchar *text)
{
	return _itocs64(val, text, true);
}

// text should be more than 40 bytes.
tchar *_itocs64(__int64 val, tchar *text, bool comma)
{
	int c = 0;
//	tchar *text = buf+tsizeof(buf)-1;
	text += 39;
	*text-- = '\0';
	while ( val != 0 ){
		if (comma){
			if ( (c % 3 == 0) && c != 0 ){
				*text-- = ',';
			}
		}
		*text = (tchar)((val % 10)+'0');
		val /= 10;
		text--;
		c++;
	}
	if ( c == 0 ) *text-- = '0';
	return text+1;
}
tnstr itocs64(__int64 val)
{
	tchar buf[40];
	return itocs64(val, buf);
}

int atox( const tchar *str, tchar **next )
{
	unsigned val = 0;
	while ( 1 )
	{
		tuchar c = (tuchar)__totupper( *str );
		if ( c >= '0' && c <= '9' ){
			val = (val<<4) + (c - '0');
		} else if ( c >= 'A' && c <= 'F' ){
			val = (val<<4) + ( c - 'A' + 10 );
		} else {
			break;
		}
		str++;
	}
	if ( next ) *next = (tchar*)str;
	return val;
}

// 指定文字数を数値に
int atoin(const tchar *str, int len)
{
	int val = 0;
	while (*str && len>0){
		tchar c = *str++;
		if (c>='0' && c<='9'){
			val = val * 10 + c-'0';
		} else
			break;
		len--;
	}
	return val;
}

// from wpdcom.cpp
// 固定小数点文字列を固定小数点整数に変換
// digit : 小数点以下の桁数
int atofp( const tchar *str, int digit )
{
	if (!str)
		return 0;
	int val = 0;
	bool inpoint = false;	// 小数点以下の計算？
	for(;*str;){
		tchar c = *str++;
		if ( c >= '0' && c <= '9' ){
			val = val * 10 + (c-'0');
			if (inpoint){
				digit--;
				if (digit==0)
					break;
			}
		} else
		if ( c == '.' ){
			if (inpoint)
				break;	// 二重の小数点
			inpoint = true;
		} else {
			break;
		}
	}
	// 残りの小数点を付加
	for(;digit>0;digit--){
		val *= 10;
	}
	return val;
}

const tchar *skiptospc(const tchar *str)
{
	while (!(*(tuchar*)str<=' ')) str++;
	return str;
}

const char *skiptospc(const char *str)
{
	while (!(*(unsigned char*)str<=' ')) str++;
	return str;
}

// 全角スペースを含めたskiptostr
const tchar *skiptospcz(const tchar *str)
{
	while (1){
		tuchar c = *(tuchar*)str;
		if (c<=' ' || c==CODE_SPACE)
			break;
		str++;
	}
	return str;
}

tchar getlastchar(const tchar *str)
{
	tchar c = 0;
	for (;*str;){
		c = *str++;
	}
	return c;
}

// obsolete
//	sizeがわかっているのならnewはいらんでしょ！！
// binからbytes分だけhex文字列に変換
// 戻り値はdelete[]すること
tchar *BinToHex( const byte *bin, int bytes )
{
	tchar *buffer = new tchar[(bytes<<1)+1];
	tchar *dp = buffer;
	for (int i=0;i<bytes<<1;i++){
		byte c = bin[i>>1];
		if (!(i&1)) c>>=4;
		c &= 0xF;
		if (c>=0xA){
			*dp++ = c-0xA+'a';
		} else {
			*dp++ = c+'0';
		}
	}
	*dp = '\0';
	return buffer;
}
// buffer : サイズは (bytes*2+1)
tchar *bintohex( tchar *buffer, const byte *bin, int bytes )
{
	tchar *dp = buffer;
	for (int i=0;i<bytes<<1;i++){
		byte c = bin[i>>1];
		if (!(i&1)) c>>=4;
		c &= 0xF;
		if (c>=0xA){
			*dp++ = c-0xA+'a';
		} else {
			*dp++ = c+'0';
		}
	}
	*dp = '\0';
	return buffer;
}
// binからbytes分だけhex文字列に変換
tnstr bintohex( const byte *bin, int bytes )
{
	tnstr s;
	s.reserve(bytes*2);
	bintohex(s.data(), bin, bytes);
	return s;
}
// strの中で target に一致する文字列があった場合、newstringに置き換える
bool ReplaceString( tchar *str, const tchar *target, const tchar *newstring )
{
	tchar *p = _tcsstr( str, target );
	if ( p ){
		int s1 = _tcslen( target );
		int s2 = _tcslen( newstring );
		memmove( p+s2, p+s1, LENTOBYTE(_tcslen(p+s1)+1) );
		return true;
	}
	return false;
}

tnstr replace(const tchar *str, const tchar *pat, const tchar *sub)
{
	tnstr ret;
	while (*str){
		const tchar *p = _tcsstr(str, pat);
		if (!p){
			ret += str;
			break;
		}
		ret.cat(str, STR_DIFF(p, str));
		ret += sub;
		str = p + _tcslen(pat);
	}
	return ret;
}

// str内の改行コードを半角スペースに変更
// ただし、文字列の最後である場合は削除するだけ。
void ReplaceCRLF(tnstr &str)
{
	tchar *p = str.c_str();
	tchar *dp = NULL;
	tchar *sp = NULL;
	while (*p){
		if (*p=='\r' || *p=='\n'){
			if (sp){
				if (str.c_str()!=dp)
					*dp++ = ' ';
				if (dp!=sp)
					memmove(dp, sp, FP_DIFF(p, sp));
				dp += STR_DIFF(p,sp);
				sp = NULL;
			}
			if (!dp){
				dp = p;
			}
		} else {
			if (dp && !sp){
				sp = p;
			}
		}
		p++;
	}
	if (sp){
		if (p!=sp && str.c_str()!=dp)
			*dp++ = ' ';
		if (dp!=sp)
			memmove(dp, sp, FP_DIFF(p, sp));
		dp += STR_DIFF(p,sp);
	}
	if (dp)
		*dp = '\0';
}


#ifndef SMALL
// %xx文字列に変換
void EscapeString( const tchar *str1, tnstr &str2 )
{
	tchar *buf = new tchar[ _tcslen(str1) * 3 + 1 ];
	tchar *dp = buf;
	for(;;){
		tuchar c = (tuchar)*str1++;
		if (!c)
			break;
		if (isalnum(c)){
			*dp++ = c;
		} else {
			*dp++ = '%';
			*dp++ = (c>>4)>=0xA ? (c>>4)+'A'-0xA : (c>>4) + '0';
			*dp++ = (c&0xF)>=0xA ? (c&0xF)+'A'-0xA : (c&0xF) + '0';
		}
	}
	*dp = '\0';
	str2.setBuf( buf );
}
// URIでescapeしなければならない文字flag, etc..
static char uriescape[] = {
	// 0x20
	1, 0, 1, 1, 1, 1, 1, 0,
	0, 0, 0, 1, 1, 0, 0, 1,
	// 0x30
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 1, 1, 1, 1, 1,
	// 0x40
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	// 0x50
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 1, 1, 0,
	// 0x60
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	// 0x70
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 1, 0, 1
};
// %xx文字列に変換
void EscapeURIString( const tchar *str1, tnstr &str2 )
{
	tchar *buf = new tchar[ _tcslen(str1) * 3 + 1 ];
	tchar *dp = buf;
	for(;;){
		tuchar c = (tuchar)*str1++;
		if (!c)
			break;
		if (c>=0x20 && c<=0x7F){
			if (uriescape[c-0x20]&1){
				// escape char
			} else {
				*dp++ = c;
				continue;
			}
		}
		*dp++ = '%';
		*dp++ = (c>>4)>=0xA ? (c>>4)+'A'-0xA : (c>>4) + '0';
		*dp++ = (c&0xF)>=0xA ? (c&0xF)+'A'-0xA : (c&0xF) + '0';
	}
	*dp = '\0';
	str2.setBuf( buf );
}
void EscapeURIString( const char *str1, tnstrA &str2 )
{
	char *buf = new char[ strlen(str1) * 3 + 1 ];
	char *dp = buf;
	for(;;){
		unsigned char c = (unsigned char)*str1++;
		if (!c)
			break;
		if (c>=0x20 && c<=0x7F){
			if (uriescape[c-0x20]&1){
				// escape char
			} else {
				*dp++ = c;
				continue;
			}
		}
		*dp++ = '%';
		*dp++ = (c>>4)>=0xA ? (c>>4)+'A'-0xA : (c>>4) + '0';
		*dp++ = (c&0xF)>=0xA ? (c&0xF)+'A'-0xA : (c&0xF) + '0';
	}
	*dp = '\0';
	str2.setBuf( buf );
}
#endif
// 正規表現制御文字を\でescape
// multibyte考慮あり
bool EscapeRegular(const tchar *str, tnstr &escstr, bool /*multibyte*/)
{
#ifdef _UNICODE
	//multibyte = false;
#endif
	tchar *buf = new tchar[_tcslen(str)*2+1];
	if (!buf)
		return false;	// memory error

	bool ret = false;
	tchar *dp = buf;
	for(;;){
		tuchar c = (tuchar)*str++;
		if (!c)
			break;
		if (IsRegularChar(c)){
			*dp++ = '\\';
			*dp++ = c;
			ret = true;
			goto j1;
		}
		// そのままcopy
		{
			*dp++ = c;
			const tchar *next = CharNext(str-1);
			for(;next!=str;){
				*dp++ = *str++;
			}			
		}
	j1:;
	}
	*dp = '\0';
	escstr = buf;
	delete[] buf;

	return ret;
}
// 改行コードをcrlfに統一する
// str : 変換元
// buf : 変換先
void NormalizeCRLF( const tchar *str, tchar *buf, const tchar *crlf )
{
	const tchar *cp;
	
	for(;;str++){
		switch (*str){
			case '\0':
				*buf = '\0';
				goto jend;
			case '\r':
				if (str[1]!='\n')
					goto jchange;	// '\r'のみ
				continue;	// ignore
			case '\n':
		jchange:;
				cp = crlf;
				for(;*cp;) *buf++ = *cp++;
				continue;
			default:
				*buf++ = *str;
				break;
		}
	}
jend:;
}

tnstr time2str(timex_t tt)
{
	struct tm *t = xlocaltime(&tt);
	tchar buf[40];
	wsnprintf(buf, tsizeof(buf), _t("%04d/%02d/%02d %02d:%02d:%02d"),
		t->tm_year+1900,
		t->tm_mon+1,
		t->tm_mday,
		t->tm_hour,
		t->tm_min,
		t->tm_sec);
	return tnstr(buf);
}

#if 0
// RTLにbugあるため (wcsicmpは使用禁止！）
int __wcsicmp(const wchar_t *str1, const wchar_t *str2)
{
	wchar_t c1, c2;

	while ((c1 = (wchar_t)__totupper(*str1)) == (c2 = (wchar_t)__totupper(*str2)) && c1 != '\0')
	{
		str1++;
		str2++;
	}
	return (c1 - c2);
}

int __wcsnicmp(const wchar_t *str1, const wchar_t *str2, int n)
{
	wchar_t c1=0, c2=0;

	while (n>0 && ((c1 = (wchar_t)__totupper(*str1)) == (c2 = (wchar_t)__totupper(*str2)) && c1 != '\0'))
	{
		str1++;
		str2++;
		n--;
	}
	return (c1 - c2);
}

#endif
