#include "tnlib.h"
#pragma	hdrstop
#include	<memory.h>
#include	"buffer.h"


Buffer::Buffer( int _size )
{
	buf = new uint8_t[ _size ];
	size = _size;
}

Buffer::Buffer( const void *data, int _size )
{
	buf = new uint8_t[ _size ];
	if (buf)
		memcpy( buf, data, _size );
	size = _size;
}

// •¡»
Buffer::Buffer( Buffer *buffer )
{
	size = buffer->size;
	buf = new uint8_t[ size ];
	if (buf)
		memcpy( buf, buffer->buf, size );
}

Buffer::~Buffer()
{
	if ( buf ){
		delete[] buf;
		buf = 0;
	}
}

