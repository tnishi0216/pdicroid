#include	"tnlib.h"
#pragma	hdrstop
#include	"file.h"

int BIFile::open( const tchar* _filename )
{
	filename = _filename;
	text = 0;
	err = 0;
	if ( fd == -1 ){
		fd = ::_topen( filename, O_RDONLY|O_BINARY );
		if (fd == -1){
			err = 1;
			return -1;
		}
	}
	return 0;
}

//バイナリファイル専用読み込みルーチン
int BIFile::read( void *buf, int len )
{
	int d;
	if ( ( d = ::read( fd, buf, len )) <= 0 ){
		return -1;
	}
	return d;
}

