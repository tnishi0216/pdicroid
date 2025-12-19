#include "tnlib.h"
#pragma hdrstop
#include "pdconfig.h"
#include "kstr.h"
#include "LangProc.h"

// 見出し語専用文字コード変換class

#ifdef NEED_SINGLEBYTE
#error	// no longer support
#endif

#ifdef DIC_BOCU

//#define	USE_SLAB

#ifdef USE_SLAB
#include "slab.h"
#define	slab_alloc(len,type)	(type*)kslab.Alloc((len)*sizeof(type))
#define	slab_free(ptr)			kslab.Free(ptr)
static TSlab kslab( 4 );
#else
#define	slab_alloc(len,type)	new type[len]
#define	slab_free(ptr)			delete[] ptr
#endif

TKCodeTranslateSet KCodeTranslateSetN = { NULL, NULL };

static const char *StrNull = "";

__kstr::__kstr(const TKCodeTranslateSet *trans)
	:Translator(trans)
{
	buffer = (void*)StrNull;
	length = 0;
	type = 1;
	newbuffer = NULL;
}

__kstr::__kstr( const char *str, const TKCodeTranslateSet *trans )
	:Translator(trans)
{
	buffer = (void*)str;
	length = strlen(str);
	type = 1;
	newbuffer = NULL;
}
__kstr::__kstr( const char *str, int len, const TKCodeTranslateSet *trans )
	:Translator(trans)
{
	buffer = (void*)str;
	length = len;
	type = 1;
	newbuffer = NULL;
}
__kstr::__kstr( const wchar_t *str, const TKCodeTranslateSet *trans )
	:Translator(trans)
{
	buffer = (void*)str;
	length = wcslen(str);
	type = 2;
	newbuffer = NULL;
}
__kstr::~__kstr()
{
	if ( newbuffer ) slab_free(newbuffer);
}
void __kstr::clear()
{
	buffer = (void*)StrNull;
	length = 0;
	if (newbuffer){
		newbuffer[0] = '\0';
		newbuffer = NULL;
	}
}

void __kstr::set(const _kchar *str)
{
	buffer = (void*)str;
	length = strlen(str);
	type = 1;
	if (newbuffer){
		slab_free(newbuffer);
		newbuffer = NULL;
	}
}
void __kstr::set(const wchar_t *str)
{
	buffer = (void*)str;
	length = wcslen(str);
	type = 2;
	if (newbuffer){
		slab_free(newbuffer);
		newbuffer = NULL;
	}
}

__kstr::operator char *()
{
	if ( type == 1 ) return (char*)buffer;
	if ( newbuffer ) return (char*)newbuffer;

	// WideChar to BOCU

	*(uint8_t**)&newbuffer = slab_alloc( length*3 + 1, uint8_t );
	*bocu1EncodeT( (tchar*)buffer, (tchar*)buffer+length, (uint8_t*)newbuffer, Translator->encodeKT) = '\0';
	return (char*)newbuffer;
}
__kstr::operator wchar_t *()
{
	if ( type == 2 ) return (wchar_t*)buffer;
	if ( newbuffer ) return (wchar_t*)newbuffer;

	// BOCU to WideChar

	const uint8_t *b = (uint8_t*)buffer;
	*(wchar_t**)&newbuffer = slab_alloc( length*4, wchar_t );
	bocu1DecodeT( &b, (uint8_t*)buffer+length, length*4, (tchar*)newbuffer, NULL, Translator->decodeKT );
	return (wchar_t*)newbuffer;
}
// BOCU1 -> UTF8変換専用class
//#if INETDIC
#if 0
__kstr_utf8::__kstr_utf8( const _kchar *s )
{
	buffer = (void*)s;
	newbuffer = NULL;
}
__kstr_utf8::~__kstr_utf8()
{
	if (newbuffer) delete[] newbuffer;
}
__kstr_utf8::operator const char *()
{
	if ( newbuffer ) return (char*)newbuffer;

	const uint8_t *b = (uint8_t*)buffer;
	int length = strlen((char*)buffer);
	*(char**)&newbuffer = slab_alloc( length*4, char );
	bocu1DecodeUTF8( &b, (uint8_t*)buffer+length, length*4, newbuffer );
	return newbuffer;
}
#endif	// INETDIC
#endif	// DIC_BOCU

