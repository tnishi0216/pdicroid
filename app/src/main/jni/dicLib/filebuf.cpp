#include	"tnlib.h"
#pragma	hdrstop
#include "dic.h"
#ifdef DOS
#include "herror.h"
#endif
#include "filestr.h"
#if !MIXDIC
#include "uniansi.h"
#endif
#if INETDIC
#include "inetdic.h"
#endif

// Mode flags //
#if DIC_STDIO
#define	F_READWRITE		0x00
#define	F_READONLY		0x01
#define	F_CREATE		0x02
#else	// !DIC_STDIO
#ifdef _Windows
	#ifdef USE_HANDLEFILE
		#define	F_READONLY	(GENERIC_READ)
		#define	F_READWRITE	(GENERIC_READ|GENERIC_WRITE)
		#define	F_CREATE	(GENERIC_READ|GENERIC_WRITE|CREATE_ALWAYS)
	#else
		#define	F_READONLY	(OF_READ|OF_SHARE_DENY_WRITE)
		#define	F_READWRITE	(OF_READWRITE|OF_SHARE_EXCLUSIVE)
		#define	F_CREATE	(OF_CREATE|OF_SHARE_EXCLUSIVE)
	#endif
#elif defined(UNIX)
	#define	F_READONLY	(O_RDONLY)
	#define	F_READWRITE	(O_RDWR)
	#define	F_CREATE	(O_CREAT)
#else
	#define	F_READONLY	(O_RDONLY|SH_DENYWR)
	#define	F_READWRITE	(O_RDWR|SH_DENYRW)
	#define	F_CREATE	(O_CREAT)
#endif
#endif	// !DIC_STDIO

static int get_last_error( );

FileBuf::FileBuf(int &_error)
	:error(_error)
{
	ReadOnly = false;
	ModifyCounter = 0;
	LastModifyCounter = 0;
	fd = INVALID_FD;
}

FileBuf::~FileBuf()
{
	close();
}
int FileBuf::_open( const tchar * _filename, int mode)
{
	ReadOnly = (mode==F_READONLY);
#ifdef DOS
	herror = 0;
#endif
	error = 0;
	if (fd != INVALID_FD){
		error = DICERR_ALREADY_OPEN;
		return -1;
	}
	filename.set( _filename );
#if !defined(CPBTEST)
	// 完全なファイル名の作成
	if ( !makename( fullfilename, _filename, NULL ) ){
		error = 16;
		return -1;
	}
#else
	fullfilename = _filename;	//** とりあえず？
#endif
#ifdef DOS	// 1999.5.17 はずした
	fullfilename[0] = (tchar)_totupper( fullfilename[0] );	// 念のため
#endif
	LastModifyCounter = ModifyCounter;
#if DIC_STDIO
#ifdef USE_FOPEN
	fd = fopen( filename,
#ifdef UNIX
		mode & F_READONLY ? "r" :
		mode & F_CREATE ? "w" : "r+"
#else
		mode & F_READONLY ? "rb" :
		mode & F_CREATE ? "wb" : "r+b"
#endif
		);
	if (!fd){
		error = dos_error_proc( errno );
		return -1;
	}
	fseek( fd, 0, SEEK_SET );
#endif
#ifdef USE_OPEN
	fd = open( filename,
		mode & F_READONLY ? O_RDONLY|O_BINARY :
		mode & F_CREATE ? O_RDWR|O_CREAT|O_TRUNC|O_BINARY : O_RDWR|O_BINARY, S_IREAD|S_IWRITE );
	if (fd!=0){
		error = dos_error_proc( errno );
		return -1;
	}
	lseek( fd, 0, SEEK_SET );
#endif
#else	// !DIC_STDIO
#ifdef	_WINDOWS
#ifdef USE_HANDLEFILE
	if ( ( fd = CreateFile(
		filename,
		(mode&(GENERIC_READ|GENERIC_WRITE)),
		(mode & GENERIC_WRITE) ? 0 : FILE_SHARE_READ,
		NULL,
		(mode & CREATE_ALWAYS) ? CREATE_ALWAYS : OPEN_EXISTING,
		FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_RANDOM_ACCESS,
		NULL
		) ) == INVALID_HANDLE_VALUE )
	{
		error = dos_error_proc( GetLastError() );
		return -1;
	}
#if 0
	if ( mode & CREATE_ALWAYS ){
		SetFilePointer( fd, 0, NULL, FILE_BEGIN );
		SetEndOfFile( fd );
	}
#endif
#else	// !USE_HANDLEFILE
	OFSTRUCT ofs;
	ConvertUniToAnsi( CP_ACP, (const tchar*)filename, __filename, MAXPATH );
	if ( ( fd = OpenFile( __filename, &ofs, mode ) ) == HFILE_ERROR )
	{
		error = dos_error_proc( ofs.nErrCode );
		return -1;
	}
#endif	// !USE_HANDLEFILE

#elif defined(UNIX)
	if ( mode & O_CREAT ){
		fd = ::open( filename, mode | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE );
	} else {
		fd = ::open( filename, mode | O_BINARY );
	}
	if ( fd == -1 ){
		error = dos_error_proc( errno );
		return -1;
	}
#else
	int r;
	if ( mode & O_CREAT ){
		r = ::_dos_creat( filename, _A_NORMAL, &fd );
	} else {
		 r = ::_dos_open( filename, mode, &fd );
	}
	if ( r ){
		fd = -1;
		error = dos_error_proc( r );
		return -1;
	}
#endif
#endif	// !DIC_STDIO

	rwcount = 0;
#ifdef SWAP_ENDIAN
	seqloc = SEQBUFFSIZE;
#endif
	return 0;
}

int FileBuf::Open( const tchar * filename )
{
	return _open( filename, F_READWRITE );
}

int FileBuf::read_open( const tchar * filename )
{
	return _open( filename, F_READONLY );
}

int FileBuf::create( const tchar * filename )
{
	return _open( filename, F_CREATE );
}

void FileBuf::close(void)
{
	if ( fd == INVALID_FD){
		return;
	}
#if DIC_STDIO
	fclose(fd);
#else	// !DIC_STDIO
#ifdef	_WINDOWS
#ifdef USE_HANDLEFILE
	CloseHandle( fd );
#else
	_lclose( fd );
#endif	// !USE_HANDLEFILE
#elif defined(UNIX)
	::close(fd);
#else
	::_close(fd);
#endif
#endif	// !DIC_STDIO
	fd = INVALID_FD;
//	filename.clear( );	// 2012.4.26 comment out to support TempClose/Reopen
}

int FileBuf::read(void FAR *buf, int len)
{
#if DIC_STDIO
	rwcount = fread( buf, 1, len, fd );
	if (rwcount<0){
		// error
		error = dos_error_proc(errno);
		return -1;
	}
#else	// !DIC_STDIO
#ifdef	_WINDOWS
#ifdef USE_HANDLEFILE
	if ( !ReadFile( fd, buf, len, &rwcount, NULL ) )
#else
	rwcount = _lread( fd, buf, len );
	if ( rwcount == INVALID_FD )
#endif
	{
		error = dos_error_proc( 0x001e );	// 読み取りエラー
		return -1;
	}
#elif defined(UNIX)
	rwcount = ::read( fd, buf, len );
	if ( rwcount == -1 ){
		error = dos_error_proc( errno );
		return -1;
	}
#else
	int r = _dos_read( fd, buf, len, &(uint)rwcount );
	if ( r ){
		error = dos_error_proc( r );
		return -1;
	}
#endif
#endif	// !DIC_STDIO
	return rwcount;
}

int FileBuf::write( const void FAR *buf, int len)
{
	ModifyCounter++;
#if DIC_STDIO
	rwcount = fwrite( buf, 1, len, fd );
	if (rwcount<0){
		error = dos_error_proc(errno);
		return -1;
	}
	if ( len != rwcount ){
		error = DICERR_DATAFULL;	// ディスク容量不足と判断
		return -1;
	}
#else	// !DIC_STDIO
#ifdef	_WINDOWS
#ifdef USE_HANDLEFILE
	if ( !WriteFile( fd, buf, len, &rwcount, NULL ) )
#else
	rwcount = _lwrite( fd, (char*)buf, len );
	if ( rwcount == INVALID_FD )
#endif
	{
		error = get_last_error();
		return -1;
	}
	if ( len != rwcount ){	// 1996.12.24
		error = DICERR_DATAFULL;	// ディスク容量不足と判断
		return -1;
	}
#elif defined(UNIX)
	rwcount = ::write( fd, buf, len );
	if ( rwcount == -1 ){
		error = dos_error_proc( errno );
		return -1;
	}
#else
	int r = ::_dos_write( fd, buf, len, &(uint)rwcount );
	if ( r ){
		error = dos_error_proc( r );
		return -1;
	}
#endif
#endif	// !DIC_STDIO
	return rwcount;
}

int FileBuf::seek(__pdc64 loc)
{
#ifdef SWAP_ENDIAN
	seqloc = SEQBUFFSIZE;
#endif
#if DIC_STDIO
	if ( fseek(fd,loc,SEEK_SET) )
#else	// !DIC_STDIO
#ifdef	_WINDOWS
#ifdef USE_HANDLEFILE
  #if sizeof(__pdc64)==8
  	LONG hi = loc>>32;
	if ( SetFilePointer( fd, loc, &hi, FILE_BEGIN ) == 0xFFFFFFFFu )
  #else
	if ( SetFilePointer( fd, loc, NULL, FILE_BEGIN ) == 0xFFFFFFFFu )
  #endif
#else
	if ( _llseek( fd, loc, 0 ) == (LONG)INVALID_FD )
#endif
#else
	if (::lseek(fd, loc, SEEK_SET) == -1L)
#endif
#endif	// !DIC_STDIO
	{
		error = 6;
		return -1;
	}
	return 0;
}
#if !defined(SMALL)
int FileBuf::seekcur(__pdc64 loc)
{
#ifdef SWAP_ENDIAN
	seqloc = SEQBUFFSIZE;
#endif
#if DIC_STDIO
	if ( fseek(fd,loc,SEEK_CUR) )
#else	// !DIC_STDIO
#ifdef	_WINDOWS
#ifdef USE_HANDLEFILE
  #if sizeof(__pdc64)==8
	LONG hi = loc>>32;
	if ( SetFilePointer( fd, loc, &hi, FILE_CURRENT ) == 0xFFFFFFFFu )
  #else
	if ( SetFilePointer( fd, loc, NULL, FILE_CURRENT ) == 0xFFFFFFFFu )
  #endif
#else
	if ( _llseek( fd, loc, FILE_CURRENT ) == (LONG)INVALID_FD )
#endif
#else
	if (::lseek(fd, loc, SEEK_CUR) == -1L)
#endif
#endif	// !DIC_STDIO
	{
		error = 6;
		return -1;
	}
	return 0;
}
#endif	// !SMALL
#if defined(SWAP_ENDIAN)
int FileBuf::getch()
{
	if (seqloc >= SEQBUFFSIZE){
#if DIC_STDIO
		rwcount = fread( seqbuf, 1, SEQBUFFSIZE, fd );
		if (rwcount < 0){
			error = dos_error_proc(errno);
			return -1;
		} else
		if ( rwcount == 0 ){
			return -1;
		}
#else	// !DIC_STDIO
#ifdef	_WINDOWS
#ifdef USE_HANDLEFILE
		if ( !ReadFile( fd, seqbuf, SEQBUFFSIZE, &rwcount, NULL ) )
#else
		rwcount = _lread( fd, seqbuf, SEQBUFFSIZE );
		if ( rwcount == INVALID_FD )
#endif
		{
			error = dos_error_proc( 0x001e );
			return -1;
		} else if ( rwcount == 0 ){
			return -1;
		}
#elif defined(UNIX)
		rwcount = ::read( fd, seqbuf, SEQBUFFSIZE );
		if ( rwcount == -1 ){
			error = dos_error_proc( errno );
			return -1;
		} else if ( rwcount == 0){
			return -1;
		}
#else
		int r = _dos_read( fd, seqbuf, SEQBUFFSIZE, &(uint)rwcount );
		if ( r ){
			error = dos_error_proc( r );
			return -1;
		} else if ( rwcount == 0){
			return -1;
		}
#endif
#endif	// !DIC_STDIO
		if ( rwcount < SEQBUFFSIZE ){
			seqloc = SEQBUFFSIZE - rwcount;
			memmove( seqbuf+seqloc, seqbuf, rwcount );
		} else {
			seqloc = 0;
		}
	}
	return (unsigned char)seqbuf[seqloc++];
}
#endif	// SWAP_ENDIAN

#if DIC_STDIO
__pdc64 FileBuf::chsize(__pdc64 l)
{
	return ftruncate(fileno(fd),l);
}
__pdc64 FileBuf::get_length( ) const
{
	struct stat statbuf;
	fstat( fileno(fd), &statbuf );
	return statbuf.st_size;
}
#elif defined(USE_HANDLEFILE)
__pdc64 FileBuf::chsize(__pdc64 l)
{
  #if sizeof(__pdc64)==8
	LONG hi = l>>32;
	if (SetFilePointer((HANDLE)fd, (DWORD)l, &hi, FILE_BEGIN) == (DWORD)-1)
  #else
	if (SetFilePointer((HANDLE)fd, (DWORD)l, NULL, FILE_BEGIN) == (DWORD)-1)
  #endif
		return -1;
	if ( !SetEndOfFile((HANDLE)fd) ){
		return -1;
	}
	return 0;
}
#endif

static char err_table[ ] = {
	// +0
	0, 98,  1, 16, 10, DICERR_READONLY, 99, 99,
	// +8
	11, 99, 99, 18, 99, 99, 99, 16,
	// +16
	99, 16, 99, 16, 16,  7, 99,  5,
	// +24
	99,  5, 18, 18, 99,  5,  5, 99,
	// +32
	17, 17, 16, 99, 16, 99, 99, 4,
	// +40
	99, 99, 99, 99, 99, 99, 99, 99,
	// +48
	99, 99, 99, 20, 16, 16, 20, 16,
};
int FileBuf::dos_error_proc( int errcode )
{
#ifdef DOS
	if (herror){
		herror = 0;
		return 7;
	}
#endif
	if ( errcode < 0x38 ){
		return err_table[ errcode ];
	} else {
#ifdef USE_HANDLEFILE
		switch ( errcode ){
			case ERROR_INVALID_NAME:
				return 35;	// ファイル名に誤りがある
		}
#endif
		return 20;		// ネットワークエラー
	}
}
static int get_last_error( )
{
#ifdef	_WINDOWS
#ifdef USE_HANDLEFILE
	switch ( GetLastError() ){
		case ERROR_DISK_FULL:
			return DICERR_DATAFULL;
		default:
			return DICERR_READWRITE;
	}
#else
	#error
#endif
#else
	switch (errno){
		case ENOSPC:
			return DICERR_DATAFULL;	//TODO: not tested
		default:
			return DICERR_READWRITE;
	}
#endif
}

#if 0	// 実は使わなくなっていた
__pdc64 FileBuf::getFreeSize( )
{
	return checkdfree( fullfilename[0]-'A' );
}

__pdc64 checkdfree(int drive)		//0=A: 1=B: ...
{
#ifdef USE_HANDLEFILE
  tchar path[ 4 ];
  path[0] = tchar(drive + 'A');
  path[1] = ':';
  path[2] = '\\';
  path[3] = '\0';
  DWORD seccls, bytesec, freecls, numcls;
  if ( GetDiskFreeSpace( path, &seccls, &bytesec, &freecls, &numcls ) ){
	return freecls * seccls * bytesec;
  } else {
	return 0;
  }
#else
	struct dfree ddfree;
	getdfree((uchar)(drive+1), &ddfree);
	return (__pdc64)ddfree.df_avail * ddfree.df_bsec * ddfree.df_sclus;
#endif
}
#endif

