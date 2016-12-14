#ifndef __tnstr_h
#define	__tnstr_h

#include "tndefs.h"

// TNSTR_ANSI : single byteをすべてANSIとして扱う場合

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

#if defined(VCL_H) || defined(__VCL) || defined(__VCL0_H__)
#define	__INC_VCL__
#endif
#if defined(__BORLANDC__) && __BORLANDC__>=0x0610 && defined(USTRING_H)
#define	__INC_USTRING
#endif

template <class T>
T *itos(int value, T *buf)
{
	int v = value;
	int n;	// 桁数
	if ( v < 0 ){
		buf[0] = '-';
		n = 1;
		value = -value;
	} else {
		n = 0;
	}
	for(;v!=0;){
		v /= 10;
		n++;
	}
	if ( n == 0 ){
		*buf++ = '0';
		*buf = '\0';
		return buf;
	}
	T *dp = buf + n;
	*dp-- = '\0';
	for(;value!=0;){
		*dp-- = (T)((value % 10) + '0');
		value /= 10;
	}
	return buf + n;
}

template <class T>
class __tnstrT {
protected:
	T *buf;
	static T buf_null[1];
//	static T buf_LoadString[256];
	static int sizeT;
public:
	__tnstrT()
		{ buf=0; }
	__tnstrT(const T *s)
		{ Constructor(s); }
	__tnstrT(const T *s, int len)
		{ buf=0; set(s, len); }
	__tnstrT(const __tnstrT &o)
		{ Constructor(o.buf); }
	__tnstrT(const T c)
		{ buf = new T[2]; buf[0] = c; buf[1] = '\0'; }
	__tnstrT(const T *s1, const T *s2)
	{
		int len1 = __length(s1);
		int len2 = __length(s2);
		buf = new T[len1+len2+1];
		if (buf){
			T *dp = buf;
			if (s1){
				memcpy(dp, s1, len1*sizeof(T));
				dp += len1;
			}
			if (s2){
				memcpy(dp, s2, len2*sizeof(T));
				dp += len2;
			}
			*dp = '\0';
		}
	}
	__tnstrT(const T *s1, const T *s2, const T *s3)
	{
		Constructor(s1, s2, s3, NULL);
	}
	__tnstrT(const T *s1, const T *s2, const T *s3, const T *s4)
	{
		Constructor(s1, s2, s3, s4);
	}
#if 0
	__tnstrT(int value)
	{
		T b[20];
		itos(value, b);
		buf = 0;
		set(b);
	}
#endif
	virtual ~__tnstrT()
	{
		if (buf) delete[] buf;
	}

	// Simple Operations //
	T *set(const T *str)
		{ return set(str, __length(str)); }
	T *set(const T *str, int len)
	{
		T *p = new T[len + 1];
		if ( !p )
			return NULL;
		T *buf_del = buf;
		buf = p;
		if (len && str){
			const T *str_end = str+len;
			for (;str<str_end;){
				if (!*str)
					break;
				*p++ = *str++;
			}
		}
		p[0] = '\0';
		if (buf_del){
			delete[] buf_del;
		}
		return str ? (T*)str : 0;
	}
	void cat(const T *str)
		{ return cat(str, __length(str)); }
	void cat(const T *str, int len)
	{
		int l = __length(buf);
		T *newbuf = new T[ l + len + 1 ];
		if ( !newbuf )
			return;
		if (buf)
			memcpy( newbuf, buf, l*sizeof(T) );
		T *p = newbuf;
		p += l;
		const T *str_end = str + len;
		for (;str<str_end;){
			if (!*str)
				break;
			*p++ = *str++;
		}
		p[0] = '\0';
		if (buf)
			delete[] buf;
		buf = newbuf;
	}
	void cat( const T *str1, const T *str2 )
	{
		int l = buf ? __length( buf ) : 0;
		int len1 = __length( str1 );
		int len2 = __length( str2 );
		tchar *p = new T[ l + len1 + len2 + 1 ];
		if ( !p )
			return;
		if (buf)
			memcpy( p, buf, l*sizeof(T) );
		memcpy( p+l, str1, len1*sizeof(T) );
		memcpy( p+l+len1, str2, len2*sizeof(T) );
		p[l+len1+len2] = '\0';
		if (buf)
			delete[] buf;
		buf = p;
	}
	void cat(const T c)
	{
		int l = __length(buf);
		T *p = new T[ l + 1 + 1 ];
		if ( !p )
			return;
		memcpy( p, buf, l*sizeof(T) );
		p[l] = c;
		p[l+1] = '\0';
		if (buf)
			delete[] buf;
		buf = p;
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
	void clear() { if (buf){buf[0] = '\0';} }
	bool empty() const { return buf ? !buf[0] : true; }
	bool exist() const { return buf && buf[0] ? true : false; }
	void reserve(int size)
	{
		if (buf){
			int len = __length(buf);
			if (len>=size)
				return;
			// expand
			T *newbuf = new T[size+1];
			if (!newbuf)
				return;
			memcpy(newbuf, buf, (len+1)*sizeof(T));
			buf = newbuf;
		} else {
			buf = new T[size+1];
			if (!buf)
				return;
			buf[0] = '\0';
		}
	}
	void setBuf(T *newbuf)
	{
		if (buf) delete[] buf;
		buf = newbuf;
	}
	T *discard()
	{
		T *ret;
		if (buf){
			ret = buf;
			buf = NULL;
			return ret;
		} else {
			ret = new T[1];
			ret[0] = '\0';
			return ret;
		}
	}
	// bufがない場合はNULLを返す
	T *_discard()
	{
		T *ret = buf;
		buf = NULL;
		return ret;
	}
	void move(__tnstrT &src)
	{
		setBuf(src._discard());
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

	// Useful Methods //
#if 0
	__tnstrT<T> substr(int start, int count)
	{
		return __tnstrT<T>(&buf[start], count);
	}
#endif
	void trunc( int c );	// 後ろの余分なcを削る

	// Compatible Useful Methods //

	// Operators //
	operator const T *() const { return c_str(); }
//	operator const void *() { return (const void*)c_str(); }
	T &operator []( int i ) const	{ return buf ? buf[i] : buf_null[i];}
	__tnstrT<T> &operator = ( const T *str ) {set( str ); return *this;}
	__tnstrT<T> &operator = ( T *str ) {set( str ); return *this;}
	__tnstrT<T> &operator = ( const __tnstrT<T> &o ) {set(o.c_str()); return *this;}
	__tnstrT<T> &operator += (const T *str) { cat(str); return *this; }
	__tnstrT<T> &operator += (const T c) { cat(c); return *this; }
//	__tnstrT<T> operator + (const __tnstrT<T> &o) {
//		return __tnstrT<T>(buf, o.buf);
//	}
	__tnstrT<T> operator + (const T *s){ return __tnstrT<T>(buf, s); }
	__tnstrT<T> operator + (const T c){ T b[2]; b[0] = c; b[1] = '\0'; return __tnstrT<T>(buf, b); }

protected:
	void Constructor(const T*str)
	{
		if (!str || !str[0]){
			buf = 0;
			return;
		}
		int len = __length(str)+1;
		buf = new T[len];
		if (buf){
			memcpy((void*)buf, (void*)str, len*sizeof(T));
		}
	}
	void Constructor(const T *s1, const T *s2, const T *s3, const T *s4)
	{
		int i;
		const int maxnum = 4;
		int len[maxnum];
		int len_total = 0;
		const T *strs[maxnum];
		strs[0] = s1;
		strs[1] = s2;
		strs[2] = s3;
		strs[3] = s4;
		for (i=0;i<maxnum;i++){
			len[i] = __length(strs[i]);
			len_total += len[i];
		}
		buf = new T[len_total+1];
		if (buf){
			T *dp = buf;
			for (i=0;i<maxnum;i++){
				if (strs[i]){
					memcpy(dp, strs[i], len[i]*sizeof(T));
					dp += len[i];
				}
			}
			*dp = '\0';
		}
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
T __tnstrT<T>::buf_null[1] = {0};
//template <class T>
//T __tnstrT<T>::buf_LoadString[256];

template <class T>
int __tnstrT<T>::sizeT = sizeof(T);

class __tnstrA : public __tnstrT<char> {
typedef __tnstrT<char> super;
public:
	__tnstrA(){}
	__tnstrA(const char *s):super(s){}
	__tnstrA(const char *s, int l):super(s,l){}
	__tnstrA(const wchar_t *s, bool utf8);
	__tnstrA(const __tnstrA &o):super(o){}
//	__tnstrA(const class __tnstrW &o);
//	__tnstrA(int value):super(value){}
	inline __tnstrA(const char *s1, const char *s2):super(s1, s2){}

	int length() const
		{ return buf ? strlen(buf) : 0; }
	int size() const { return length(); }
	int Length() const { return length(); }
	int GetLength() const { return length(); }
	int byte() const { return length() * sizeT; }

	// Useful methods /
	int find(const char*pat);
//	bool LoadString(int id);

	// Code Conversion //
	// wide char -> multi byte
	void setUTF8(const wchar_t *utf16, int utf16_len=-1);
#ifdef TNANSI
	void setAnsi(const wchar_t *utf16, int utf16_len=-1);
#endif
	void setAsc(const char *asc, int asc_len=-1)		// only ASCII code.
		{ set(asc, asc_len); }

	// multi byte -> wide char
	class __tnstrW getWUTF8();
	class __tnstrW getWAnsi();

#if 0	// ほかのmethodをうまく組み合わせればできると思うので削除
	// multi byte -> UTF8
	__tnstrA getUTF8()
#ifdef __UTF8
		{ return *this; }		// UTF8->UTF8
#else
		;						// Ansi->UTF8
#endif
#endif

	// Useful Methods //
	__tnstrA substr(int start, int count) const
	{
		return __tnstrA(&buf[start], count);
	}

	// Operators //
	operator const unsigned char *() const { return (const unsigned char*)c_str(); }
	bool operator == (const __tnstrA &s) const
		{ return strcmp(buf?buf:buf_null, s)==0; }
	bool operator == (const char *s) const
		{ return strcmp(buf?buf:buf_null, s)==0; }
	bool operator != (const __tnstrA &s) const
		{ return strcmp(buf?buf:buf_null, s)!=0; }
	bool operator != (const char *s) const
		{ return strcmp(buf?buf:buf_null, s)!=0; }
	int operator < (const char *s) const
		{ return strcmp(c_str(), s)<0; }
	int operator <= (const char *s) const
		{ return strcmp(c_str(), s)<=0; }
	int operator > (const char *s) const
		{ return strcmp(c_str(), s)>0; }
	int operator >= (const char *s) const
		{ return strcmp(c_str(), s)>=0; }

	__tnstrA &operator = ( const char *str ) {set( str ); return *this;}
//	__tnstrA &operator = ( char *str ) {set( str ); return *this;}
//	__tnstrA operator + (const __tnstrA &s) { return (__tnstrA)super::operator + (s); }
	__tnstrA operator + (const char *s) { return (__tnstrA)super::operator + (s); }
	__tnstrA operator + (int v);
	__tnstrA operator + (double v);

// +TNChar //
#if defined(TNChar) && !defined(_UNICODE)
//	__tnstrA(const TNChar &s):super(s){}
#endif

#ifdef __INC_VCL__
	__tnstrA(const AnsiString &s)
		:super(s.c_str()){}

	// Operators //
	__tnstrA &operator = (const System::AnsiString &s)
	{
		set(s.c_str());
		return *this;
	}
#endif

#if defined(TNChar) && !defined(_UNICODE) && !defined(__UTF8)
//	__tnstrA &operator = (const TNChar &s)
//		{ set(s); return *this; }
#endif
	operator System::AnsiString () const;
protected:
};

class __tnstrW : public __tnstrT<wchar_t> {
typedef __tnstrT<wchar_t> super;
public:
	__tnstrW(){}
	__tnstrW(const wchar_t *s):super(s){}
	__tnstrW(const wchar_t *s, int l):super(s,l){}
	__tnstrW(const __tnstrW &o):super(o){}
	__tnstrW(const wchar_t *s1, const wchar_t *s2):super(s1, s2){}
	__tnstrW(const wchar_t *s1, const wchar_t *s2, const wchar_t *s3):super(s1, s2, s3){}
	__tnstrW(const char *s1, bool utf8);
//	__tnstrW(int value):super(value){}

	int length() const
		{ return buf ? wcslen(buf) : 0; }
	int size() const { return length(); }
	int Length() const { return length(); }
	int GetLength() const { return length(); }
	int byte() const { return length() * sizeT; }
	wchar_t *c_bstr() const { return c_str(); }

	// Useful methods //
	int find(const wchar_t*pat);
//	bool LoadString(int id);

	// Code Conversion //
	void setUTF8(const char *utf8, int utf8_len=-1);	// UTF8 >> UTF16
#ifdef TNANSI
	void setAnsi(const char *ansi, int ansi_len=-1);	// ANSI >> UTF16
	void setAsc(const char *asc, int asc_len=-1)		// only ASCII code.
		{ setAnsi(asc, asc_len); }
#else
	void setAsc(const char *asc, int asc_len=-1);		// only ASCII code.
#endif

#if 0	// ほかのmethodをうまく組み合わせればできると思うので削除
	__tnstrA getUTF8();		// UTF16->UTF8
#endif

	__tnstrW substr(int start, int count) const
	{
		return __tnstrW(&buf[start], count);
	}

	// Operators //
	bool operator == (const __tnstrW &s) const
		{ return wcscmp(buf?buf:buf_null, s)==0; }
	bool operator == (const wchar_t *s) const
		{ return wcscmp(buf?buf:buf_null, s)==0; }
	bool operator != (const __tnstrW &s) const
		{ return wcscmp(buf?buf:buf_null, s)!=0; }
	bool operator != (const wchar_t *s) const
		{ return wcscmp(buf?buf:buf_null, s)!=0; }
	int operator < (const wchar_t *s) const
		{ return wcscmp(c_str(), s)<0; }
	int operator <= (const wchar_t *s) const
		{ return wcscmp(c_str(), s)<=0; }
	int operator > (const wchar_t *s) const
		{ return wcscmp(c_str(), s)>0; }
	int operator >= (const wchar_t *s) const
		{ return wcscmp(c_str(), s)>=0; }

	__tnstrW &operator = ( const wchar_t *str ) {set( str ); return *this;}
#ifdef TNSTR_ANSI
	__tnstrW &operator = (const char *str) { setAnsi(str); return *this; }
#endif
//	__tnstrW &operator = ( wchar_t *str ) {set( str ); return *this;}
//	__tnstrW operator + (const __tnstrW &s) { return (__tnstrW)super::operator + (s); }
	__tnstrW operator + (const wchar_t *s) { return (__tnstrW)super::operator + (s); }
	__tnstrW operator + (const wchar_t c){ return (__tnstrW)super::operator + (c); }
	__tnstrW operator + (const char c){ return (__tnstrW)super::operator + (c); }
	__tnstrW operator + (int v);
	__tnstrW operator + (double v);

// +TNChar //
#if defined(TNChar) && defined(_UNICODE)
//	__tnstrW(const TNChar &s):super(s){}
#endif

// +UnicodeString //
#ifdef __INC_USTRING
	__tnstrW(const System::UnicodeString &us);
#endif
// +VCL //
#ifdef __INC_VCL__
	__tnstrW(const System::WideString &ws)
		:super(ws.c_bstr()){}
#if 0
	__tnstrW(const AnsiString &o)
		{ setAnsi(o.c_str()); }
#endif
	__tnstrW &operator = ( const AnsiString &o)
		{ setAnsi(o.c_str()); return *this; }
#endif

	// Operators //
	__tnstrW &operator = (const System::WideString &ws);
	// Note: compile errorを回避するため(WideStringにWideString(int)があるためだと思われる
	__tnstrW &operator = (int value)
	{
		wchar_t buf[20];
		itos(value, buf);
		set(buf);
		return *this;
	}
#if defined(TNChar) && defined(_UNICODE) && !defined(__UTF8)
//	__tnstrW &operator = (const TNChar &s)
//		{ set(s); return *this; }
#endif
	operator System::WideString () const;
protected:
};

class __tnstrAx : public __tnstrA {
typedef __tnstrA super;
public:
	__tnstrAx(const wchar_t *s):super(s, false){}
	__tnstrAx(const __tnstrW &o):super(o, false){}
#ifdef __INC_VCL__
	__tnstrAx(const System::WideString &o):super(o.c_bstr(), false){}
	__tnstrA &operator = (const System::WideString &s)
		{ setAnsi(s.c_bstr()); return *this; }
#endif
};

class __tnstrWx : public __tnstrW {
typedef __tnstrW super;
public:
	__tnstrWx(const char *s):super(s, false){}
	__tnstrWx(const __tnstrA &o):super(o, false){}
#ifdef __INC_VCL__
//	__tnstrWx(const AnsiString &o):super(o.c_str(), false){}
	__tnstrW &operator = (const AnsiString &s)
		{ setAnsi(s.c_str()); return *this; }
#endif
};


#include <stdarg.h>

__tnstrW tnsprintfW(const wchar_t *fmt, ...);
__tnstrW _tnsprintfW(const wchar_t *fmt, va_list arg);
__tnstrA tnsprintfA(const char *fmt, ...);
__tnstrA _tnsprintfA(const char *fmt, va_list arg);

#ifndef __NO_WINDOWS_H
typedef map<__tnstrW,__tnstrW> tnstrW_map;
typedef map<__tnstrA,__tnstrA> tnstrA_map;
#endif

#if defined(_UNICODE) && !defined(__UTF8)
#define	__tnstr	__tnstrW
#define	tnstr	__tnstrW
#define	tnsprintf	tnsprintfW
#define	_tnsprintf	_tnsprintfW
#define	tnstr_vec	tnstrW_vec
#define	tnstr_map	tnstrW_map
#else
#define	__tnstr	__tnstrA
#define	tnstr	__tnstrA
#define	tnsprintf	tnsprintfA
#define	_tnsprintf	_tnsprintfA
#define	tnstr_vec	tnstrA_vec
#define	tnstr_map	tnstrA_map
#endif

#define	_tnstrA	__tnstrA
#define	_tnstrW	__tnstrW

#define	_tnstrAx	__tnstrAx
#define	_tnstrWx	__tnstrWx

#include "tnarray.h"

//#define	USE_TNSTRVEC_STL	// tnvec uses the vector of the STL.

#ifdef USE_TNSTRVEC_STL
#  define	TNSTRVEC_SUPER	vector<tnstrT>
#else
#  define	TNSTRVEC_SUPER	FlexObjectArray<tnstrT>
#endif

template <class tnstrT, class T>
class tnstrT_vec : public TNSTRVEC_SUPER {
typedef TNSTRVEC_SUPER super;
public:
	tnstrT_vec<tnstrT,T>(const tnstrT_vec<tnstrT,T> &);
	tnstrT_vec<tnstrT,T>(){}
#ifdef USE_TNSTRVEC_STL
#else
	tnstrT_vec<tnstrT,T>(int slotnum):super(slotnum){}
#endif

#ifdef USE_TNSTRVEC_STL
	void push_back(const T *o)
		{ super::push_back(o); }
	void push_back(const T *o, int len)
		{ super::push_back(tnstrT(o, len)); }
	void push_back(tnstrT *o)
		{ super::push_back(*o); delete o; }	// これがやっかい
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
	void pop()
		{ if (super::get_num()>0) super::del(super::get_num()-1); }
	void pop_back()
		{ pop(); }
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
		bool operator != (const T *s)
		{
			return (**(tnstrT**)cur) != s;
		}
		bool operator == (const iterator &o)
		{
			return cur==o.cur;
		}
		bool operator == (const T *s)
		{
			return (**(tnstrT**)cur) == s;
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
		T operator [] (int i) const
		{
			return (**(tnstrT**)cur)[i];
		}
	};
	iterator begin()
	{
		return iterator(super::__get_array());
	}
	iterator end()
	{
		return iterator(super::__get_array(), super::get_num());
	}
#endif	// !USE_TNSTRVEC_STL

	void split(const T *str, const T *delim);
	tnstrT join(const T *delim);

	// safe version of operator []
	const T *index(int i) const
		{ return i>=0 && i<size() ? (*this)[i].c_str() : NULL; }
};

template <class tnstrT, class T>
void tnstrT_vec<tnstrT,T>::split(const T *str, const T *delim)
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
tnstrT tnstrT_vec<tnstrT,T>::join(const T *delim)
{
	tnstr str;
	for (tnstrT_vec<tnstrT,T>::iterator it=begin();it!=end();it++){
		if (str.empty()){
			str = (*it);
		} else {
			str += delim;
			str += (*it);
		}
	}
	return str;
}

// UTF16で定義した文字列を必要ならばUTF8で返す
class __cstr {
protected:
	const wchar_t *str;
	char *buffer;
public:
	__cstr( const wchar_t *_str )
		:str(_str)
		,buffer(NULL)
	{
	}
	~__cstr()
	{
		if (buffer) delete[] buffer;
	}
	operator char *()
		{ return utf8(); }
	char *utf8();
	operator wchar_t *()
		{ return (wchar_t*)str; }
};

// UTF16で定義した文字列を必要ならばUTF8で返す(__cstrの１文字版）
class __cstr1 {
protected:
	const wchar_t *str;
	char buffer[8+1];
public:
	__cstr1( const wchar_t *_str )
		:str(_str)
	{
		buffer[0] = '\0';
	}
	~__cstr1()
	{
	}
	operator char *()
		{ return utf8(); }
	char *utf8();
	operator wchar_t *()
		{ return (wchar_t*)str; }
};

#if 1
typedef tnstrT_vec<__tnstrA, char> tnstrA_vec;
typedef tnstrT_vec<__tnstrW, wchar_t> tnstrW_vec;
#else
class tnstrW_vec : public tnstrT_vec<wchar_t> {
typedef tnstrT_vec<wchar_t> super;
public:
};

class tnstrA_vec : public tnstrT_vec<char> {
typedef tnstrT_vec<char> super;
public:
};
#endif

#ifndef foreach
#define	foreach(obj, it, type) \
	for (type::iterator it=(obj).begin();it!=(obj).end();it++)
#endif

#define	foreach_tnstrA_vec(obj, it) foreach(obj, it, tnstrA_vec)
#define	foreach_tnstrW_vec(obj, it) foreach(obj, it, tnstrW_vec)
#define	foreach_tnstrA_map(obj, it) foreach(obj, it, tnstrA_map)
#define	foreach_tnstrW_map(obj, it) foreach(obj, it, tnstrW_map)

#ifdef _UNICODE
#define	foreach_tnstr_vec	foreach_tnstrW_vec
#define	foreach_tnstr_map	foreach_tnstrW_map
#else
#define	foreach_tnstr_vec	foreach_tnstrA_vec
#define	foreach_tnstr_map	foreach_tnstrA_map
#endif

#ifdef __BORLANDC__	
#pragma option pop
#endif

#endif	/* __tnstr_h */

