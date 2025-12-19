#include	"tnlib.h"
#pragma	hdrstop
#include	"char.h"

FixChar::FixChar( int _maxlen )
	:maxlen( _maxlen )
{
	buf = new tchar[ maxlen+1 ];
	buf[0] = '\0';
}

void FixChar::Constructor( const tchar *str, int _maxlen )
{
	maxlen = _maxlen;
	buf = new tchar[ _maxlen+1 ];
	set( str );
}

FixChar::~FixChar()
{
	delete[] buf;
	buf = 0;
}

void FixChar::set( const tchar *str )
{
	set( str, _tcslen( str ) );
}

void FixChar::set( const tchar *str, int len )
{
	if ( len > maxlen )
		len = maxlen;
	_tcsncpy( buf, str, len );
	buf[len] = '\0';
}

