#include	"bc.h"
#pragma	hdrstop
#include "tndefs.h"
#include	"vbuffer.h"

VarBuffer::VarBuffer( int _slotsize )
	:slotsize( _slotsize )
{
	slotnum = 1;
	buff = new tchar[ slotsize ];
	len = 0;
}

VarBuffer::~VarBuffer()
{
	delete[] buff;
}

tchar *VarBuffer::increase( )
{
	slotnum++;
	tchar *p = (tchar*)::realloc( buff, LENTOBYTE(slotsize * slotnum) );
	if ( !p ){
		slotnum--;
		return NULL;
	}
	buff = p;
	return p;
}

int VarBuffer::add( const tchar *str )
{
	int l = _tcslen( str );
	if ( l + len > slotsize * slotnum ){
		if ( !increase() ){
			return -1;
		}
	}
	memcpy( buff + len, str, LENTOBYTE(l) );
	len += l;
	return 0;
}

int VarBuffer::add( tchar c )
{
	if ( len >= slotsize * slotnum ){
		if ( !increase( ) ){
			return -1;
		}
	}
	buff[len++] = c;
	return 0;
}
