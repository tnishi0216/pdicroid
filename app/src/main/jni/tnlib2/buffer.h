#ifndef	__BUFFER_H
#define	__BUFFER_H

class Buffer {
protected:
	uint8_t *buf;
	int size;
public:
	Buffer( int _size );
	Buffer( const void *data, int _size );
	// ï°êª
	Buffer( Buffer *buffer );
	~Buffer();
	operator char *()
		{ return (char*)buf; }
	operator uint8_t *()
		{ return buf; }
//		char &operator[]( int i )
//			{ return buf[i]; }
	uint8_t &operator[]( int i )
		{ return buf[i]; }
	int GetSize( )
		{ return size; }
};


#endif	// __BUFFER_H
