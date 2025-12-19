#include "stdafx.h"
#if defined(__ANDROID__) && defined(__BORLANDC__)
#include <System.hpp>
#endif
#include "tnlib.h"
#pragma hdrstop
#include "tnstr.h"
#include "utf.h"

/// __tnstrA ///
__tnstrA::__tnstrA(const wchar_t *s, bool utf8)
{
#ifdef TNANSI
	if (utf8){
		setUTF8(s);
	} else {
		setAnsi(s);
	}
#else
	setUTF8(s);
#endif
}

#if 0
int __tnstrA::find(const char*pat)
{
	if (!buf)
		return -1;
	char *p = strstr(buf, pat);
	if (!p)
		return -1;
	return (int)(p-buf);
}
#endif

// Code Conversion //
#if wchar_size==2
// UTF16 >> UTF8
void __tnstrA::setUTF8(const wchar_t *utf16, int utf16_len)
{
	if (!utf16 || !utf16[0] || utf16_len==0){
		clear();
		return;
	}
	if (utf16_len<0){
		utf16_len = wcslen(utf16);
	}
	char *buf_new = new char[utf16_len*4+1];
	if (!buf_new)
		return;	// error
	UTF16toUTF8(utf16, utf16_len, buf_new);
	delete[] buf;
	buf = buf_new;
}
#endif
#if wchar_size==4
// UTF32 >> UTF8
void __tnstrA::setUTF8(const wchar_t *utf32, int utf32_len)
{
	if (!utf32 || !utf32[0] || utf32_len==0){
		clear();
		return;
	}
	if (utf32_len<0){
		utf32_len = wcslen(utf32);
	}
	char *buf_new = new char[utf32_len*4+1];
	if (!buf_new)
		return;	// error
	UTF32toUTF8((const utf32_t*)utf32, utf32_len, buf_new);
	delete[] buf;
	buf = buf_new;
}
#endif

#ifdef TNANSI
// UTF16 >> ANSI
void __tnstrA::setAnsi(const wchar_t *utf16, int utf16_len)
{
	if (!utf16 || !utf16[0] || utf16_len==0){
		clear();
		return;
	}
	if (utf16_len<0){
		utf16_len = wcslen(utf16);
	}
	int buf_size = utf16_len*2+1;
	char *buf_new = new char[buf_size];
	if (!buf_new)
		return;	// error
	int ret = WideCharToMultiByte(CP_ACP, 0, utf16, utf16_len, buf_new, buf_size, NULL, NULL );
	if (ret==0){
		delete[] buf_new;
		return;	// error
	}
	buf_new[ret] = '\0';
	delete[] buf;
	buf = buf_new;
}
#endif

__tnstrW __tnstrA::getWUTF8()
{
	__tnstrW ret;
	ret.setUTF8(*this);
	return ret;
}
#ifdef TNANSI
__tnstrW __tnstrA::getWAnsi()
{
	__tnstrW ret;
	ret.setAnsi(*this);
	return ret;
}
#endif

#if 0
#if !defined(__UTF8)
__tnstrA __tnstrA::getUTF8()
{
	//TODO: 2段階変換
	__tnstrW strw;
	strw.setAnsi(c_str());
	__tnstrA stra;
	stra.setUTF8(strw);
	return stra;
}
#endif
#endif

/// __tnstrW ///
__tnstrW::__tnstrW(const char *s1, bool utf8)
{
#ifdef TNANSI
	if (utf8){
		setUTF8(s1);
	} else {
		setAnsi(s1);
	}
#else
	setUTF8(s1);
#endif
}
int __tnstrW::find(const wchar_t*pat)
{
	if (!buf)
		return -1;
	wchar_t *p = wcsstr(buf, pat);
	if (!p)
		return -1;
	return (int)(p-buf);
}

// Code Conversion //
// UTF-8 >> wchar_t
// utf8_len is byte unit.
void __tnstrW::setUTF8(const char *utf8, int utf8_len)
{
	if (!utf8 || utf8_len==0){
		clear();
		return;
	}
	if (utf8_len<0){
		utf8_len = strlen(utf8);
	}
#if wchar_size==2
	wchar_t *buf_new = new wchar_t[utf8_len*2+1];
	if (!buf_new)
		return;	// error
	UTF8toUTF16(utf8, utf8_len, buf_new);
#endif
#if wchar_size==4
	wchar_t *buf_new = new wchar_t[utf8_len+1];
	if (!buf_new)
		return;	// error
	UTF8toUTF32(utf8, utf8_len, (utf32_t*)buf_new);
#endif
	delete[] buf;
	buf = buf_new;
}

#ifdef TNANSI
// ANSI >> UTF16
void __tnstrW::setAnsi(const char *ansi, int ansi_len)
{
	if (!ansi || ansi_len==0){
		clear();
		return;
	}
	if (ansi_len<0){
		ansi_len = strlen(ansi);
	}
	int buf_len = ansi_len*2;
	wchar_t *buf_new = new wchar_t[buf_len+1];
	if (!buf_new)
		return;	// error
	int wlen = MultiByteToWideChar(CP_ACP, 0, ansi, ansi_len, buf_new, buf_len);
	buf_new[wlen] = '\0';
	delete[] buf;
	buf = buf_new;
}
#else
void __tnstrW::setAsc(const char *asc, int asc_len)
{
	if (!asc || asc_len==0){
		clear();
		return;
	}
	if (asc_len<0){
		asc_len = strlen(asc);
	}
	int buf_len = asc_len*2;
	wchar_t *buf_new = new wchar_t[buf_len+1];
	if (!buf_new)
		return;	// error
	wchar_t *dp = buf_new;
	while(*asc){
		*dp++ = *asc++;
	}
	buf_new[asc_len] = '\0';
	delete[] buf;
	buf = buf_new;
}
#endif

__tnstrW __tnstrW::operator + (int v)
{
	wchar_t tmp[40];
	itosW(v, tmp);
	return __tnstrW(buf, tmp);
}
#ifndef UNIX
__tnstrW __tnstrW::operator + (double v)
{
	wchar_t tmp[40];
	_snwprintf(tmp, tsizeof(tmp), L"%g", v);
	return __tnstrW(buf, tmp);
}
#endif


#if 0
// UTF16 >> UTF8
__tnstrA __tnstrW::getUTF8()
{
	__tnstrA stra;
	stra.setUTF8(c_str());
	return stra;
}
#endif

__tnstrA __tnstrA::operator + (int v)
{
	char tmp[40];
	itosA(v, tmp);
	return __tnstrA(buf, tmp);
}
__tnstrA __tnstrA::operator + (double v)
{
	char tmp[40];
	_snprintf(tmp, sizeof(tmp), "%g", v);
	return __tnstrA(buf, tmp);
}

#ifdef __INC_VCL__
// VCL depends.
#include <StdCtrls.hpp>

/// __tnstrA ///
__tnstrA::operator System::AnsiString () const
{
	return AnsiString(c_str());
}
#endif

__tnstrA tnsprintfA(const char *fmt, ...)
{
	va_list(ap);
	va_start(ap, fmt);
	__tnstrA ret = _tnsprintfA(fmt, ap);
	va_end(ap);
	return ret;
}

__tnstrA _tnsprintfA(const char *fmt, va_list arg)
{
	char abuf[256];
	char *buf = NULL;
	int nsize = sizeof(abuf);
	int rsize = vsnprintf(abuf, nsize-1, fmt, arg);
	if (rsize>=nsize-1){
		nsize = vsnprintf(NULL, 0, fmt, arg)+1;
		buf = new char[nsize];
		if (buf){
			vsnprintf(buf, nsize, fmt, arg);
		} else {
			abuf[0] = '\0';
		}
	}
	if (buf){
		autoptr<char> _buf(buf);
		return __tnstrA(buf);
	} else {
		return __tnstrA(abuf);
	}
}

#ifdef __INC_USTRING
__tnstrW::__tnstrW(const System::UnicodeString &us)
#if wchar_size==2
	:super(us.c_str())
#endif
{
#if wchar_size==4
	int len = us.Length();
	if (len>0){
		wchar_t *nbuf = new wchar_t[len+1];
		UTF16toUTF32((uint16_t*)us.c_str(), len, (utf32_t*)nbuf, len);
		setBuf(nbuf);
	}
#elif wchar_size==2
#else
#error
#endif
}
#endif	// __INC_USTRING

#ifdef __INC_VCL__
/// __tnstrW ///
__tnstrW::operator System::WideString () const
{
	return WideString(buf ? buf : buf_null);
}

__tnstrW &__tnstrW::operator = (const System::WideString &ws)
{
	set(ws.c_bstr());
	return *this;
}
#endif

__tnstrW tnsprintfW(const wchar_t *fmt, ...)
{
	va_list(ap);
	va_start(ap, fmt);
	__tnstrW ret = _tnsprintfW(fmt, ap);
	va_end(ap);
	return ret;
}

__tnstrW _tnsprintfW(const wchar_t *fmt, va_list arg)
{
#if 0
	wchar_t abuf[1025];
	wvsprintf(abuf, fmt, arg);
	return __tnstrW(abuf);
#else
	wchar_t abuf[256];
	wchar_t *buf = NULL;
	int nsize = sizeof(abuf)/sizeof(wchar_t);
	int rsize = _vsnwprintf(abuf, nsize-1, fmt, arg);
	//Note:
	// BCC Bug?
	// nsize-1では収まらない場合、rsizeが-1になるみたい。
	// 256バイト単位で増やして確保している
	if (rsize==-1 || rsize>=nsize-1){
		nsize = _vsnwprintf(NULL, 0, fmt, arg)+1;
		if (nsize==0){
			nsize = 512;
		}
		do {
			buf = new wchar_t[nsize];
			if (buf){
				rsize = _vsnwprintf(buf, nsize, fmt, arg);
				if (rsize<0){
					delete[] buf;
					nsize += 256;
					continue;
				}
			} else {
				abuf[0] = '\0';
			}
			break;
		} while (1);
	}
	if (buf){
		autoptr<wchar_t> _buf(buf);
		return __tnstrW(buf);
	} else {
		return __tnstrW(abuf);
	}
#endif
}

char *__cstr::utf8()
{
	if (buffer) return buffer;
	int buffer_len = wcslen(str)*8;
	buffer = new char[buffer_len+1];
	if (!buffer) return NULL;
#if wchar_size==2
	UTF16toUTF8(str, -1, buffer, buffer_len);
#endif
#if wchar_size==4
	UTF32toUTF8((utf32_t*)str, -1, buffer, buffer_len);
#endif
	return buffer;
}

char *__cstr1::utf8()
{
	if (buffer[0]) return buffer;
#if wchar_size==2
	UTF16toUTF8(str, -1, buffer, sizeof(buffer));
#endif
#if wchar_size==4
	UTF32toUTF8((utf32_t*)str, -1, buffer, sizeof(buffer));
#endif
	return buffer;
}

