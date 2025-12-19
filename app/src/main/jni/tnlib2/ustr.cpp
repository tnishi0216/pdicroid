//---------------------------------------------------------------------------

#pragma hdrstop
#include <string.h>

#include "utf.h"
#include "ustr.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

__ustr::__ustr( const char *str )
{
	buffer = (void*)str;
	length = strlen(str);
	type = 1;
	newbuffer = NULL;
}
__ustr::__ustr( const char *str, int len )
{
	buffer = (void*)str;
	length = len;
	type = 1;
	newbuffer = NULL;
}
__ustr::__ustr( const wchar_t *str )
{
	buffer = (void*)str;
	length = wcslen(str);
	type = 2;
	newbuffer = NULL;
}
__ustr::~__ustr()
{
	delete[] newbuffer;
}
__ustr::operator char *()
{
	if ( type == 1 ) return (char*)buffer;
	if ( newbuffer ) return (char*)newbuffer;

	// wchar_t to UTF8
	{
		newbuffer = new char[ length*4 + 1 ];
#if wchar_size==2
		int size = UTF16toUTF8( (uint16_t*)buffer, length, newbuffer );
#endif
#if wchar_size==4
		int size = UTF32toUTF8( (uint32_t*)buffer, length, newbuffer );
#endif
		newbuffer[size] = '\0';
	}
	return (char*)newbuffer;
}
__ustr::operator wchar_t *()
{
	if ( type == 2 ) return (wchar_t*)buffer;
	if ( newbuffer ) return (wchar_t*)newbuffer;

	// UTF8 to wchar_t
	{
#if wchar_size==2
		*(wchar_t**)&newbuffer = new wchar_t[ length*2 + 1 ];
		UTF8toUTF16( (char*)buffer, length, (uint16_t*)newbuffer );
#endif
#if wchar_size==4
		*(wchar_t**)&newbuffer = new wchar_t[ length + 1 ];
		UTF8toUTF32( (char*)buffer, length, (uint32_t*)newbuffer );
#endif
	}
	return (wchar_t*)newbuffer;
}
