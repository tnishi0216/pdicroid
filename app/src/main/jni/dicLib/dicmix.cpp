#include "tnlib.h"
#pragma hdrstop
#include "pdconfig.h"
#include "dicmix.h"

#if 0
TCharSingle::TCharSingle( const char *str )
{
	buffer = NULL;
	set( str );
}
TCharSingle::TCharSingle()
{
	buffer = new char[1];
	buffer[0] = '\0';
}
TCharSingle::~TCharSingle()
{
	delete[] buffer;
}
void TCharSingle::set( const char *str )
{
	if ( buffer )
		delete[] buffer;
	int l = strlen(str)+1;
	buffer = new char[l];
	memcpy( buffer, str, l );
}
#endif

#if defined(DIC_UTF8) || MIXDIC
_mchar *_mstrcut( _mchar *str, size_t len )
{
	int l = strlen(str);
	if ( l > len ){
		_mchar *p = str + l - 1;
		for(;!_misleadbyte(*p)&&p>str;){
			p--;
		}
		*p = '\0';
	}
	return str;
}
#endif	// DIC_UTF8

#ifdef UNICODE
tchar *bocu1DecodeStr( const uint8_t *data, const uint8_t **_end )
{
	const uint8_t *end = data;
	for(;*end;) end++;
	if (_end) *_end = end;
	return bocu1DecodeT( &data, end, ((int)(end-data))*4+sizeof(tchar) );
}
#endif
