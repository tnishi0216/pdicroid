#ifndef __uustr_h
#define	__uustr_h

// UTF16 <-> UTF8 conversion string class

#include <string>

class uustr {
protected:
	void *buffer;	// Reference pointer
	char *newbuffer;
	int length;
	int type;
public:
	uustr( const char *str );
	uustr( const char *str, int len )
		{ constructor(str, len); }
	uustr(std::string &str)
		{ constructor(str.c_str(), str.length()); }
	uustr(std::wstring &str)
		{ constructor(str.c_str(), str.length()); }
	uustr( const wchar_t *str );
protected:
	void constructor(const char *str, int len);
	void constructor(const wchar_t *str, int len);
public:
	~uustr();
	operator char *();
	operator unsigned char *()
		{ return (unsigned char*)(char*)(*this); }
	operator wchar_t *();
	char *str()
		{ return (char*)(*this); }
	wchar_t *wstr()
		{ return (wchar_t*)(*this); }
};

#ifdef UNICODE
#ifdef __UTF8
#define	_uustrT(s)	uustr(s)		// tchar <=> TCHAR conversion
#define	_uustr8(s)	(s)				// tchar <=> UTF8 conversion
#else
#define	_uustrT(s)	(s)
#define	_uustr8(s)	uustr(s)
#endif
#else	// !UNICODE
#define	_uustrT(s)	(s)				// tchar(ANSI) <=> TCHAR conversion
//#define	_uustr8(s)	uustr(s)		// tchar(ANSI) <=> UTF8 conversion
#endif

#endif	/* __uustr_h */

