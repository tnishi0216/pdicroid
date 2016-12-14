#ifndef tnstrbufH
#define tnstrbufH

#include <string.h>
#ifndef __NO_WINDOWS_H
#include <vector>
#include <map>
#endif
using namespace std;

#ifdef __BORLANDC__
#pragma option push
#pragma warn -8027
#endif

#include "tnstr.h"

#define	__TNDEF_BUFSLOTSIZE	256

template <class T>
class __tnstrbufT {
protected:
	enum { BufSlotSize = __TNDEF_BUFSLOTSIZE };
	T *buf;
	int bufsize;
	int strsize;
	static T buf_null[1];
	static int sizeT;
public:
	__tnstrbufT()
		{ buf=0; bufsize=0; strsize=0; }
	__tnstrbufT(const T *s)
		{ Constructor(s); }
	__tnstrbufT(const T *s, int len)
		{ buf=0; bufsize=0; strsize=0; set(s, len); }
	__tnstrbufT(const __tnstrbufT &o)
		{ Constructor(o.buf); }
	__tnstrbufT(const __tnstrT<T> &o)
		{ Constructor(o.c_str()); }
	__tnstrbufT(const T c)
	{
		buf = new T[BufSlotSize];
		buf[0] = c;
		buf[1] = '\0';
		bufsize=BufSlotSize;
		strsize=(buf[0]?1:0);
	}
	virtual ~__tnstrbufT()
	{
		if (buf) delete[] buf;
	}

	// Simple Operations //
	T *set(const T *str)
		{ return set(str, __length(str)); }
	T *set(const T *str, int len)
	{
		T *buf_del = 0;
		T *p;
		if (len+1>bufsize){
			int newsize = allocsize(max(len+1,(int)BufSlotSize));
			p = new T[newsize];
			if ( !p )
				return NULL;
			bufsize = newsize;
			buf_del = buf;
			buf = p;
		} else {
			p = buf;
		}
		if (len && str){
			uint_ptr str_end = (uint_ptr)str+len*sizeof(T);	// to avoid code guard error
			for (;(uint_ptr)str<str_end;){
				if (!*str)
					break;
				*p++ = *str++;
			}
		}
		p[0] = '\0';
		strsize = (int)(p-buf);
		if (buf_del){
			delete[] buf_del;
		}
		return str ? (T*)str : 0;
	}
	void cat(const T *str)
		{ return cat(str, __length(str)); }
	void cat(const T *str, int len)
	{
		int newlen = strsize + len + 1;
		T *buf_del = 0;
		T *p;
		if (newlen>bufsize){
			newlen = allocsize(newlen);
			p = new T[ newlen ];
			if ( !p )
				return;
			bufsize = newlen;
			p[0] = '\0';
			if (buf){
				if (buf[0])
					memcpy( p, buf, strsize*sizeof(T) );
				buf_del = buf;
			}
			buf = p;
		} else {
			p = buf;
		}
		p += strsize;
		const T *str_end = str + len;
		for (;str<str_end;){
			if (!*str)
				break;
			*p++ = *str++;
		}
		p[0] = '\0';
		strsize = (int)(p-buf);
		if (buf_del)
			delete[] buf_del;
	}
	inline void cat(const T c)
	{
		cat(&c, 1);
	}
	void trim(int len)
	{
		if (strsize>len){
			strsize = len;
			buf[len] = '\0';
		}
	}
	T *get(T *str, int len)
	{
		const T *src = buf;
		const T *str_end = str + len;
		for (;str<str_end;){
			if (!*src)
				break;
			*str++ = *src++;
		}
		str[len] = '\0';
		return str;
	}
	T *c_str() const { return buf ? buf : buf_null; }
	T *data() const { return buf; }
	void clear() { if (buf){buf[0] = '\0'; strsize=0; } }
	bool empty() const { return strsize==0; }
	bool exist() const { return strsize>0; }
	void setBuf(T *newbuf, int _bufsize)
	{
		if (buf) delete[] buf;
		buf = newbuf;
		bufsize = _bufsize;
		strsize = __length(buf);
	}

	void expand(int size)
	{
		if (size<=bufsize)
			return;

		size = allocsize(size);
		T *p = new T[ size ];
		if ( !p )
			return;
		bufsize = size;
		p[0] = '\0';
		if (buf){
			if (buf[0])
				memcpy( p, buf, strsize*sizeof(T) );
		}
		delete[] buf;
		buf = p;
	}

	// STL compatibilities //
	void assign(const T *str)
	{
		set(str);
	}
	void assign(const T *str, int len)
	{
		set(str, len);
	}

	// Compatible Simple Methods //
	bool IsEmpty() const { return empty(); }

	// Operators //
	operator const T *() const { return c_str(); }
//	operator const void *() { return (const void*)c_str(); }
	T &operator []( int i )	{ return buf ? buf[i] : buf_null[i];}
	__tnstrbufT<T> &operator = ( const T *str ) {set( str ); return *this;}
	__tnstrbufT<T> &operator = ( T *str ) {set( str ); return *this;}
	__tnstrbufT<T> &operator = ( const __tnstrbufT<T> &o ) {set(o.c_str()); return *this;}
	__tnstrbufT<T> &operator = ( const __tnstrT<T> &o ) { set(o.c_str()); return *this; }
	__tnstrbufT<T> &operator += (const T *str) { cat(str); return *this; }
	__tnstrbufT<T> &operator += (const T c) { cat(c); return *this; }
	__tnstrbufT<T> &operator << (const T *str) { cat(str); return *this; }
//	__tnstrbufT<T> operator + (const __tnstrbufT<T> &o) {
//		return __tnstrbufT<T>(buf, o.buf);
//	}
//	__tnstrbufT<T> operator + (const T *s){ return __tnstrbufT<T>(buf, s); }
//	__tnstrbufT<T> operator + (const T c){ T b[2]; b[0] = c; b[1] = '\0'; return __tnstrbufT<T>(buf, b); }

protected:
	void Constructor(const T*str)
	{
		if (!str || !str[0]){
			buf = 0;
			bufsize = 0;
			strsize = 0;
			return;
		}
		strsize = __length(str);
		int len = strsize+1;
		bufsize = allocsize(max(len,(int)BufSlotSize));
		buf = new T[bufsize];
		if (buf){
			memcpy((void*)buf, (void*)str, len*sizeof(T));
		}
	}
	inline int allocsize(int len)
	{
		return ((len+BufSlotSize-1)/BufSlotSize)*BufSlotSize;
	}
	int  __length(const T *s) const
	{
		if (!s)
			return 0;
		const T *top = s;
		for (;*s;){ s++; }
		return (int)(s-top);
	}
};

#ifndef SYSMAC_H
namespace System {
class AnsiString;
class WideString;
};
#endif

template <class T>
T __tnstrbufT<T>::buf_null[1] = {0};

template <class T>
int __tnstrbufT<T>::sizeT = sizeof(T);

class __tnstrbufA : public __tnstrbufT<char> {
typedef __tnstrbufT<char> super;
public:
	__tnstrbufA(){}
	__tnstrbufA(const char *s):super(s){}
	__tnstrbufA(const char *s, int l):super(s,l){}
	__tnstrbufA(const __tnstrbufA &o):super(o){}
	__tnstrbufA(const __tnstrA &o):super(o){}
//	__tnstrbufA(const class __tnstrbufW &o);
//	__tnstrbufA(int value):super(value){}

	int length() const
		{ return strsize; }
	int size() const { return length(); }
	int Length() const { return length(); }
	int GetLength() const { return length(); }

	// Operators //
	operator const unsigned char *() const { return (const unsigned char*)c_str(); }
	bool operator == (const __tnstrbufA &s)
		{ return strcmp(buf?buf:buf_null, s)==0; }
	bool operator == (const char *s)
		{ return strcmp(buf?buf:buf_null, s)==0; }
	bool operator != (const __tnstrbufA &s)
		{ return strcmp(buf?buf:buf_null, s)!=0; }
	bool operator != (const char *s)
		{ return strcmp(buf?buf:buf_null, s)!=0; }
	__tnstrbufA &operator = ( const char *str ) {set( str ); return *this;}
//	__tnstrbufA &operator = ( char *str ) {set( str ); return *this;}
//	__tnstrbufA operator + (const __tnstrbufA &s) { return (__tnstrbufA)super::operator + (s); }
//	__tnstrbufA operator + (const char *s) { return (__tnstrbufA)super::operator + (s); }
	int operator < (const char *s) const
		{ return strcmp(c_str(), s)<0; }
	int operator <= (const char *s) const
		{ return strcmp(c_str(), s)<=0; }
	int operator > (const char *s) const
		{ return strcmp(c_str(), s)>0; }
	int operator >= (const char *s) const
		{ return strcmp(c_str(), s)>=0; }
	__tnstrbufA &operator << (const char *str) { cat(str); return *this; }
	__tnstrbufA &operator << (int val) { char buf[21]; itoa(val, buf, 10); cat(buf); return *this; }

// +TNChar //
#if defined(TNChar) && !defined(_UNICODE)
//	__tnstrbufA(const TNChar &s):super(s){}
#endif

#if defined(VCL_H) || defined(__VCL)
	__tnstrbufA(const AnsiString &s)
		:super(s.c_str()){}

	// Operators //
	__tnstrbufA &operator = (const AnsiString &s);
	operator System::AnsiString () const
		{ return AnsiString(c_str()); }
#endif
protected:
};

class __tnstrbufW : public __tnstrbufT<wchar_t> {
typedef __tnstrbufT<wchar_t> super;
public:
	__tnstrbufW(){}
	__tnstrbufW(const wchar_t *s):super(s){}
	__tnstrbufW(const wchar_t *s, int l):super(s,l){}
	__tnstrbufW(const __tnstrbufW &o):super(o){}
	__tnstrbufW(const __tnstrW &o):super(o){}

	int length() const
		{ return buf ? wcslen(buf) : 0; }
	int size() const { return length(); }
	int Length() const { return length(); }
	int GetLength() const { return length(); }
	wchar_t *c_bstr() const { return c_str(); }

	// Operators //
	bool operator == (const __tnstrbufW &s) const
		{ return wcscmp(buf?buf:buf_null, s)==0; }
	bool operator == (const wchar_t *s) const
		{ return wcscmp(buf?buf:buf_null, s)==0; }
	bool operator != (const __tnstrbufW &s) const
		{ return wcscmp(buf?buf:buf_null, s)!=0; }
	bool operator != (const wchar_t *s) const
		{ return wcscmp(buf?buf:buf_null, s)!=0; }
	__tnstrbufW &operator = ( const wchar_t *str ) {set( str ); return *this;}
	__tnstrbufW &operator = ( __tnstrW &str ) {set( str ); return *this;}
//	__tnstrbufW &operator = ( wchar_t *str ) {set( str ); return *this;}
//	__tnstrbufW operator + (const __tnstrbufW &s) { return (__tnstrbufW)super::operator + (s); }
//	__tnstrbufW operator + (const wchar_t *s) { return (__tnstrbufW)super::operator + (s); }
//	__tnstrbufW operator + (const wchar_t c){ return (__tnstrbufW)super::operator + (c); }
//	__tnstrbufW operator + (const char c){ return (__tnstrbufW)super::operator + (c); }
	int operator < (const wchar_t *s) const
		{ return wcscmp(c_str(), s)<0; }
	int operator <= (const wchar_t *s) const
		{ return wcscmp(c_str(), s)<=0; }
	int operator > (const wchar_t *s) const
		{ return wcscmp(c_str(), s)>0; }
	int operator >= (const wchar_t *s) const
		{ return wcscmp(c_str(), s)>=0; }
	__tnstrbufW &operator << (const wchar_t *str) { cat(str); return *this; }
#if defined(_UNICODE) && !defined(__UTF8)
	__tnstrbufW &operator << (int val) { wchar_t buf[21]; _itow(val, buf, 10); cat(buf); return *this; }
#endif

// +VCL //
#if defined(VCL_H) || defined(__VCL)
	__tnstrbufW(const System::WideString &ws)
		:super(ws.c_bstr()){}
#endif

	// Operators //
#if 0
	__tnstrbufW &operator = (const System::WideString &ws);
	// Note: compile error‚ð‰ñ”ð‚·‚é‚½‚ß(WideString‚ÉWideString(int)‚ª‚ ‚é‚½‚ß‚¾‚ÆŽv‚í‚ê‚é
	__tnstrbufW &operator = (int value)
	{
		wchar_t buf[20];
		itos(value, buf);
		set(buf);
		return *this;
	}
#endif
#if defined(VCL_H) || defined(__VCL)
	operator System::WideString () const
	{
		return System::WideString(buf);
	}
	bool operator != (const System::WideString &o) const
	{
		if (empty() && !o.c_bstr())
			return false;
		if (empty() || !o.c_bstr())
			return true;
		return wcscmp(buf, o.c_bstr())!=0;
	}
	bool operator == (const System::WideString &o) const
	{
		if (empty() && !o.c_bstr())
			return true;
		if (empty() || !o.c_bstr())
			return false;
		return wcscmp(buf, o.c_bstr())==0;
	}
#endif
protected:
};

// __tnstr_autobufT //

template <class T>
class __tnstr_autobufT {
protected:
	enum { BufSlotSize = __TNDEF_BUFSLOTSIZE };
	T *autobuf;
	T *buf;
	int bufsize;
	int strsize;
	//static T buf_null[1];
	//static int sizeT;
public:
	__tnstr_autobufT(T *_autobuf)	// autobuf must be equal or larger than BufSlotSize.
	{
		autobuf = _autobuf;
		buf=autobuf;
		buf[0]='\0';
		bufsize=BufSlotSize;
		strsize=0;
	}
	__tnstr_autobufT(T *_autobuf, const T *src, int len=-1)	// autobuf must be equal or larger than BufSlotSize.
	{
		autobuf = _autobuf;
		buf=autobuf;
		bufsize=BufSlotSize;
		strsize=0;
		set(src, len>=0?len:__length(src));
	}
	virtual ~__tnstr_autobufT()
	{
		if (autobuf!=buf) delete[] buf;
	}

	int length() const
		{ return strsize; }
	int size() const { return length(); }
	int Length() const { return length(); }
	int GetLength() const { return length(); }

	// Simple Operations //
	T *set(const T *str)
		{ return set(str, __length(str)); }
	T *set(const T *str, int len)
	{
		T *buf_del = 0;
		T *p;
		if (len+1>bufsize){
			int newsize = allocsize(max(len+1,(int)BufSlotSize));
			p = new T[newsize];
			if ( !p )
				return NULL;
			bufsize = newsize;
			buf_del = buf==autobuf?NULL:buf;
			buf = p;
		} else {
			p = buf;
		}
		if (len && str){
			const T *str_end = str+len;
			for (;str<str_end;){
				if (!*str)
					break;
				*p++ = *str++;
			}
		}
		p[0] = '\0';
		strsize = (int)(p-buf);
		if (buf_del){
			delete[] buf_del;
		}
		return str ? (T*)str : 0;
	}
	void cat(const T *str)
		{ return cat(str, __length(str)); }
	void cat(const T *str, int len)
	{
		int newlen = strsize + len + 1;
		T *buf_del = 0;
		T *p;
		if (newlen>bufsize){
			newlen = allocsize(newlen);
			p = new T[ newlen ];
			if ( !p )
				return;
			bufsize = newlen;
			p[0] = '\0';
			if (buf[0])
				memcpy( p, buf, strsize*sizeof(T) );
			buf_del = buf!=autobuf?buf:NULL;
			buf = p;
		} else {
			p = buf;
		}
		p += strsize;
		const T *str_end = str + len;
		for (;str<str_end;){
			if (!*str)
				break;
			*p++ = *str++;
		}
		p[0] = '\0';
		strsize = (int)(p-buf);
		if (buf_del)
			delete[] buf_del;
	}
	inline void cat(const T c)
	{
		cat(&c, 1);
	}
	T *get(T *str, int len)
	{
		const T *src = buf;
		const T *str_end = str + len;
		for (;str<str_end;){
			if (!*src)
				break;
			*str++ = *src++;
		}
		str[len] = '\0';
		return str;
	}
	T *c_str() const { return buf; }
	T *data() const { return buf; }
	void clear() { buf[0] = '\0'; strsize=0; }
	bool empty() const { return strsize==0; }
	bool exist() const { return strsize>0; }
	void expand(int size)
	{
		if (size<=bufsize)
			return;

		size = allocsize(size);
		T *p = new T[ size ];
		if ( !p )
			return;
		bufsize = size;
		p[0] = '\0';
		if (buf[0])
			memcpy( p, buf, strsize*sizeof(T) );
		if (buf!=autobuf)
			delete[] buf;
		buf = p;
	}

	// STL compatibilities //
	void assign(const T *str)
	{
		set(str);
	}
	void assign(const T *str, int len)
	{
		set(str, len);
	}

	// Compatible Simple Methods //
	bool IsEmpty() const { return empty(); }

	// Operators //
	operator const T *() const { return c_str(); }
//	operator const void *() { return (const void*)c_str(); }
	T &operator []( int i )	{ return buf[i];}
	__tnstr_autobufT<T> &operator = ( const T *str ) {set( str ); return *this;}
	__tnstr_autobufT<T> &operator = ( T *str ) {set( str ); return *this;}
	__tnstr_autobufT<T> &operator = ( const __tnstr_autobufT<T> &o ) {set(o.c_str()); return *this;}
	__tnstr_autobufT<T> &operator += (const T *str) { cat(str); return *this; }
	__tnstr_autobufT<T> &operator += (const T c) { cat(c); return *this; }
//	__tnstr_autobufT<T> operator + (const __tnstr_autobufT<T> &o) {
//		return __tnstr_autobufT<T>(buf, o.buf);
//	}
//	__tnstr_autobufT<T> operator + (const T *s){ return __tnstr_autobufT<T>(buf, s); }
//	__tnstr_autobufT<T> operator + (const T c){ T b[2]; b[0] = c; b[1] = '\0'; return __tnstr_autobufT<T>(buf, b); }

protected:
	inline int allocsize(int len)
	{
		return ((len+BufSlotSize-1)/BufSlotSize)*BufSlotSize;
	}
	int  __length(const T *s) const
	{
		if (!s)
			return 0;
		const T *top = s;
		for (;*s;){ s++; }
		return (int)(s-top);
	}
};

class __tnstr_autobufA : public __tnstr_autobufT<char> {
typedef __tnstr_autobufT<char> super;
public:
	__tnstr_autobufA(char *autobuf):super(autobuf){}
	__tnstr_autobufA(char *autobuf, const char *s, int l=-1):super(autobuf, s,l){}

	// Operators //
	operator const unsigned char *() const { return (const unsigned char*)c_str(); }
	bool operator == (const __tnstrbufA &s)
		{ return strcmp(buf, s)==0; }
	bool operator == (const __tnstr_autobufA &s)
		{ return strcmp(buf, s)==0; }
	bool operator == (const char *s)
		{ return strcmp(buf, s)==0; }
	bool operator != (const __tnstrbufA &s)
		{ return strcmp(buf, s)!=0; }
	bool operator != (const __tnstr_autobufA &s)
		{ return strcmp(buf, s)!=0; }
	bool operator != (const char *s)
		{ return strcmp(buf, s)!=0; }
	__tnstr_autobufA &operator = ( const char *str ) {set( str ); return *this;}
//	__tnstr_autobufA &operator = ( char *str ) {set( str ); return *this;}
//	__tnstr_autobufA operator + (const __tnstr_autobufA &s) { return (__tnstr_autobufA)super::operator + (s); }
//	__tnstr_autobufA operator + (const char *s) { return (__tnstr_autobufA)super::operator + (s); }
	int operator < (const char *s) const
		{ return strcmp(buf, s)<0; }
	int operator <= (const char *s) const
		{ return strcmp(buf, s)<=0; }
	int operator > (const char *s) const
		{ return strcmp(buf, s)>0; }
	int operator >= (const char *s) const
		{ return strcmp(buf, s)>=0; }

#if defined(VCL_H) || defined(__VCL)
	__tnstr_autobufA(const AnsiString &s)
		:super(s.c_str()){}

	// Operators //
	__tnstr_autobufA &operator = (const AnsiString &s);
	operator System::AnsiString () const
		{ return AnsiString(c_str()); }
#endif
protected:
};

class __tnstr_autobufW : public __tnstr_autobufT<wchar_t> {
typedef __tnstr_autobufT<wchar_t> super;
public:
	__tnstr_autobufW(wchar_t *autobuf):super(autobuf){}
	__tnstr_autobufW(wchar_t *autobuf, const wchar_t *s, int l=-1):super(autobuf, s,l){}

	wchar_t *c_bstr() const { return c_str(); }

	// Operators //
	bool operator == (const __tnstrbufW &s) const
		{ return wcscmp(buf, s)==0; }
	bool operator == (const __tnstr_autobufW &s) const
		{ return wcscmp(buf, s)==0; }
	bool operator == (const wchar_t *s) const
		{ return wcscmp(buf, s)==0; }
	bool operator != (const __tnstrbufW &s) const
		{ return wcscmp(buf, s)!=0; }
	bool operator != (const __tnstr_autobufW &s) const
		{ return wcscmp(buf, s)!=0; }
	bool operator != (const wchar_t *s) const
		{ return wcscmp(buf, s)!=0; }
	__tnstr_autobufW &operator = ( const wchar_t *str ) {set( str ); return *this;}
//	__tnstr_autobufW &operator = ( wchar_t *str ) {set( str ); return *this;}
//	__tnstr_autobufW operator + (const __tnstr_autobufW &s) { return (__tnstr_autobufW)super::operator + (s); }
//	__tnstr_autobufW operator + (const wchar_t *s) { return (__tnstr_autobufW)super::operator + (s); }
//	__tnstr_autobufW operator + (const wchar_t c){ return (__tnstr_autobufW)super::operator + (c); }
//	__tnstr_autobufW operator + (const char c){ return (__tnstr_autobufW)super::operator + (c); }
	int operator < (const wchar_t *s) const
		{ return wcscmp(c_str(), s)<0; }
	int operator <= (const wchar_t *s) const
		{ return wcscmp(c_str(), s)<=0; }
	int operator > (const wchar_t *s) const
		{ return wcscmp(c_str(), s)>0; }
	int operator >= (const wchar_t *s) const
		{ return wcscmp(c_str(), s)>=0; }

// +VCL //
#if defined(VCL_H) || defined(__VCL)
	__tnstr_autobufW(const System::WideString &ws)
		:super(ws.c_bstr()){}
#endif

	// Operators //
#if 0
	__tnstr_autobufW &operator = (const System::WideString &ws);
	// Note: compile error‚ð‰ñ”ð‚·‚é‚½‚ß(WideString‚ÉWideString(int)‚ª‚ ‚é‚½‚ß‚¾‚ÆŽv‚í‚ê‚é
	__tnstr_autobufW &operator = (int value)
	{
		wchar_t buf[20];
		itos(value, buf);
		set(buf);
		return *this;
	}
#endif
#if defined(VCL_H) || defined(__VCL)
	operator System::WideString () const
	{
		return System::WideString(buf);
	}
	bool operator != (const System::WideString &o) const
	{
		if (empty() && !o.c_bstr())
			return false;
		if (empty() || !o.c_bstr())
			return true;
		return wcscmp(buf, o.c_bstr())!=0;
	}
	bool operator == (const System::WideString &o) const
	{
		if (empty() && !o.c_bstr())
			return true;
		if (empty() || !o.c_bstr())
			return false;
		return wcscmp(buf, o.c_bstr())==0;
	}
#endif
protected:
};

#define	__tnstr__autobufW(name)	wchar_t __autobuf__##name[__TNDEF_BUFSLOTSIZE]; __tnstr_autobufW name(__autobuf__##name);
#define	__tnstr__autobufA(name)	char __autobuf__##name[__TNDEF_BUFSLOTSIZE]; __tnstr_autobufA name(__autobuf__##name);


#include <stdarg.h>

#ifndef __NO_WINDOWS_H
typedef map<__tnstrbufW,__tnstrbufW> tnstrbufW_map;
typedef map<__tnstrbufA,__tnstrbufA> tnstrbufA_map;
#endif

#if defined(_UNICODE) && !defined(__UTF8)
#define	__tnstrbuf	__tnstrbufW
#define	tnstrbuf	__tnstrbufW
#define	tnstr_autobuf	__tnstr_autobufW
#define	tnstrbuf_vec	tnstrW_vec
#define	tnstrbuf_map	tnstrbufW_map
#else
#define	__tnstrbuf	__tnstrbufA
#define	tnstr_autobuf	__tnstr_autobufA
#define	tnstrbuf	__tnstrbufA
#define	tnstrbuf_vec	tnstrA_vec
#define	tnstrbuf_map	tnstrbufA_map
#endif

#include "tnarray.h"

//#define	USE_TNSTRVEC_STL	// tnvec uses the vector of the STL.

#ifdef USE_TNSTRVEC_STL
#  define	TNSTRVEC_SUPER	vector<tnstrT>
#else
#  define	TNSTRVEC_SUPER	FlexObjectArray<tnstrT>
#endif

template <class tnstrT, class T>
class tnstrbufT_vec : public TNSTRVEC_SUPER {
typedef TNSTRVEC_SUPER super;
public:
	tnstrbufT_vec<tnstrT,T>(const tnstrbufT_vec<tnstrT,T> &);
	tnstrbufT_vec<tnstrT,T>(){}
#ifdef USE_TNSTRVEC_STL
#else
	tnstrbufT_vec<tnstrT,T>(int slotnum):super(slotnum){}
#endif

#ifdef USE_TNSTRVEC_STL
	void push_back(const T *o)
		{ super::push_back(o); }
	void push_back(const T *o, int len)
		{ super::push_back(tnstrT(o, len)); }
	void push_back(tnstrT *o)
		{ super::push_back(*o); delete o; }	// ‚±‚ê‚ª‚â‚Á‚©‚¢
	void add(const T *o)
		{ super::push_back(tnstrT(o)); }
	void add(tnstrT *o)
		{ super::push_back(o); }
#else	// !USE_TNSTRVEC_STL
	int size() const
		{ return super::get_num(); }
	void push(const T *o)
		{ super::add(new tnstrT(o)); }
	void push(const T *o, int len)
		{ super::add(new tnstrT(o, len)); }
	void push(tnstrT *o)
		{ super::add(o); }
	void push_back(const T *o)
		{ push(o); }
	void push_back(const T *o, int len)
		{ push(o, len); }
	void push_back(tnstrT *o)
		{ push(o); }
	void add(const T *o)
		{ push(o); }
	void add(const T *o, int len)
		{ push(o, len); }
	void add(tnstrT *o)
		{ push(o); }
	void insert(int index, const T *o)
		{ super::insert(index, new tnstrT(o)); }
	void insert(int index, const T *o, int len)
		{ super::insert(index, new tnstrT(o, len)); }
	void insert(int index, tnstrT *o)
		{ super::insert(index, o); }

	class iterator {
	protected:
		void **array;
		void **cur;
	public:
		iterator(void **arg_array)
			:array(arg_array)
		{
			cur = &array[0];
		}
		iterator(void **arg_array, int num)
			:array(arg_array)
		{
			cur = &array[num];
		}
		void operator ++ ()
		{
			cur++;
		}
		iterator &operator ++ (int)
		{
			cur++;
			return *this;
		}
		bool operator != (const iterator &o)
		{
			return cur!=o.cur;
		}
		bool operator == (const iterator &o)
		{
			return cur==o.cur;
		}
		iterator &operator = (const iterator &o)
		{
			array = o.array;
			cur = o.cur;
			return *this;
		}
		tnstrT *operator -> ()
		{
			return *(tnstrT**)cur;
		}
		tnstrT &operator * ()
		{
			return **(tnstrT**)cur;
		}
		operator tnstrT &() const
		{
			return **(tnstrT**)cur;
		}
	};
	iterator begin()
	{
		return iterator(super::get_array());
	}
	iterator end()
	{
		return iterator(super::get_array(), super::get_num());
	}
#endif	// !USE_TNSTRVEC_STL

	void split(const T *str, const T *delim);
	tnstrT join(const T *delim);
};

template <class tnstrT, class T>
void tnstrbufT_vec<tnstrT,T>::split(const T *str, const T *delim)
{
	const T *sp = str;
	for(;*sp;){
		const T *p = sp;
		for (;;){
			T c = *p;
			if (!c)
				break;
			const T *delimp = delim;
			for (;*delimp;){
				if (*delimp==c)
					goto j1;
				delimp++;
			}
			p++;
		}
	j1:;
		push_back(sp, (int)(p-sp));
		if (!*p)
			break;
		sp = p+1;
	}
}

template <class tnstrT, class T>
tnstrT tnstrbufT_vec<tnstrT,T>::join(const T *delim)
{
	tnstrbuf str;
	for (tnstrbufT_vec<tnstrT,T>::iterator it=begin();it!=end();it++){
		if (str.empty()){
			str = (*it);
		} else {
			str += delim;
			str += (*it);
		}
	}
	return str;
}

#if 1
typedef tnstrbufT_vec<__tnstrbufA, char> tnstrbufA_vec;
typedef tnstrbufT_vec<__tnstrbufW, wchar_t> tnstrbufW_vec;
#else
class tnstrbufW_vec : public tnstrbufT_vec<wchar_t> {
typedef tnstrbufT_vec<wchar_t> super;
public:
};

class tnstrbufA_vec : public tnstrbufT_vec<char> {
typedef tnstrbufT_vec<char> super;
public:
};
#endif

#ifndef foreach
#define	foreach(obj, it, type) \
	for (type::iterator it=(obj).begin();it!=(obj).end();it++)
#endif

#define	foreach_tnstrbufA_vec(obj, it) foreach(obj, it, tnstrbufA_vec)
#define	foreach_tnstrbufW_vec(obj, it) foreach(obj, it, tnstrbufW_vec)

#ifdef _UNICODE
#define	foreach_tnstrbuf_vec	foreach_tnstrbufW_vec
#else
#define	foreach_tnstrbuf_vec	foreach_tnstrbufA_vec
#endif

#ifdef __BORLANDC__	
#pragma option pop
#endif

#endif
