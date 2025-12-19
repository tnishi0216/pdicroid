#include "uustr.h"
#include "utf.h"

uustr::uustr( const char *str )
{
	constructor(str, str ? strlen(str) : 0);
}
uustr::uustr( const wchar_t *str )
{
	constructor(str, str ? wcslen(str) : 0);
}
void uustr::constructor(const char *str, int len)
{
	buffer = (void*)str;
	length = len;
	type = 1;
	newbuffer = NULL;
}
void uustr::constructor(const wchar_t *str, int len)
{
	buffer = (void*)str;
	length = len;
	type = 2;
	newbuffer = NULL;
}
uustr::~uustr()
{
	delete[] newbuffer;
}
uustr::operator char *()
{
	if ( type == 1 ) return (char*)buffer;
	if ( newbuffer ) return (char*)newbuffer;

	// wchar_t to UTF8
	newbuffer = new char[ length*4 + 1 ];
#if wchar_size==2
	UTF16toUTF8((utf16_t*)buffer, length, newbuffer);
#endif
#if wchar_size==4
	UTF32toUTF8((utf32_t*)buffer, length, newbuffer);
#endif
	return (char*)newbuffer;
}
uustr::operator wchar_t *()
{
	if ( type == 2 ) return (wchar_t*)buffer;
	if ( newbuffer ) return (wchar_t*)newbuffer;

	// UTF8 to wchar_t
#if wchar_size==2
	*(wchar_t**)&newbuffer = new wchar_t[ length*2 + 1 ];
	UTF8toUTF16((char*)buffer, length, (utf16_t*)newbuffer);
#endif
#if wchar_size==4
	*(wchar_t**)&newbuffer = new wchar_t[ length + 1 ];
	UTF8toUTF32((char*)buffer, length, (utf32_t*)newbuffer);
#endif
	return (wchar_t*)newbuffer;
}

