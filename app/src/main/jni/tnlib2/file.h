#ifndef	__FILE_H
#define	__FILE_H

//Note:
// If you use string stl class, please include tnlib.h instead of tnstd.h.

#include <string>
#include "tndefs.h"
#include "tnstr.h"

enum eTEXTFILEMODE {
	TFM_DEFAULT = 0,	// SJIS if Ansi, UTF-16 if Unicode.
	TFM_ANSI = 1,
	TFM_ASCII = 20127,
	TFM_UTF8 = 65001,	// Available when using unicode.
	TFM_UTF16LE = 1200,	// UTF-16 little endian
	TFM_UTF16BE = 1201,	// UTF-16 big-endian
	TFM_EUCJP = 51932,
	TFM_JIS = 50222,
};

#if !defined(WINCE) && !defined(UNIX) && !defined(__ANDROID__)
#include <io.h>
#include <direct.h>
#endif
//#include "char.h"

#if defined(UNIX)
#include <unistd.h>
#define	__off_t		off_t
#ifndef S_IREAD
#define	S_IREAD		0
#endif
#ifndef S_IWRITE
#define	S_IWRITE	0
#endif
#ifdef __UTF8
#define	_topen		open
#define	_tchmod		chmod
#define	_tremove	remove
#else
#define	_topen		wopen
#define	_tchmod		wchmod
#define	_tremove	wremove
int wopen(const tchar*  path, int  flags);
int wopen(const tchar*  path, int  flags, mode_t mode);
int wchmod(const wchar_t *filename, mode_t mode);
int wremove(const wchar_t *pathname);
#endif
inline int _close( int handle )
	{ return close( handle ); }
inline off_t tell( int handle )
	{ return lseek( handle, 0, SEEK_CUR ); }
inline off_t _lseek(int handle, off_t offset, int fromwhere)
	{ return lseek( handle, offset, fromwhere ); }
inline off_t eof( int handle )
	{ return lseek( handle, 0, SEEK_END ); }
inline off_t _eof( int handle )
	{ return eof( handle ); }
off_t filelength( int handle );
#else
#include <sys/stat.h>
typedef __int64	__off_t;
#endif

#define	BUFFSIZE	4096

#ifdef WINCE	// 標準ライブラリの代替
#define O_RDONLY    0
#define O_WRONLY    1
#define O_BINARY    0x8000  /* no translation   */
#define O_CREAT     0x0100  /* create and open file */
#define O_TRUNC     0x0200  /* open with truncation */
int _tcreat( const tchar *filename, int mode );
int _topen( const tchar *filename, int mode );
inline int _topen( const tchar *filename, int mode, int /* smode */ )
	{ return _topen( filename, mode ); }
int _twrite( int handle, const void *buf, unsigned bytes );
int _tread(int handle, void *buf, unsigned bytes );
inline void _tclose( int handle )
	{ CloseHandle( (HANDLE)handle ); }
inline long _tlseek(int handle, long offset, int fromwhere)
	{ return SetFilePointer( (HANDLE)handle, offset, NULL, fromwhere ); }
inline long _ttell(int handle)
	{ return SetFilePointer( (HANDLE)handle, 0, NULL, FILE_CURRENT ); }
#else	// !WINCE

#if !defined(_UNICODE)
#ifndef _tcreat
inline int _tcreat( const tchar *filename, int mode )
	{ return ::creat( filename, mode ); }
#endif
#define	_topen	open
#endif

#ifndef _twrite
#define	_twrite	write
#endif

#define	_tread	read
//TOOD: 64bit非対応@C++Builder5
inline __off_t _tlseek(int handle, __off_t offset, int fromwhere)
	{ return _lseek( handle, (long)offset, fromwhere ); }
inline __off_t _ttell(int handle)
	{ return tell( handle ); }
inline int _tclose( int handle )
	{ return _close( handle ); }

#endif

class TCharCodeStreamBase {
public:
	virtual int read(void *Buffer, int Count) = 0;
	virtual int write(const void *Buffer, int Count) = 0;
	virtual __off_t seek(__off_t Offset, int Origin) = 0;
	virtual __off_t tell() = 0;
};

class TCharCodeBase {
public:
//	virtual int Detect(const tchar *str){ return 0; }
	virtual bool Valid() const { return false; }
	virtual int Detect(const tchar *filename, TCharCodeStreamBase *stream){ return 0; }
	virtual int Convert( int src_code, int dst_code, const char *src, int *srclen, utf16_t *outbuf, int bufsize ) { return 0; }
	virtual int GetDetectedCode(){ return 0; }
};

class File {
protected:
	int fd;
	int err;
	int text;
	const tchar *filename;
	TCharCodeBase *cc;	// ref
public:
	File();
	virtual ~File();
	void SetCC(TCharCodeBase *_cc)
	{
		cc = _cc;
	}
	virtual int open( const tchar *_filename ) = 0;
	virtual int create( const tchar *_filename ) = 0;
	virtual void close( );
	virtual void home( )
		{ seek( 0L ); }
	virtual void end( )
		{ _tlseek( fd, 0L, SEEK_END ); }
	virtual void seek( long l )
		{ _tlseek( fd, l, SEEK_SET ); }
//		virtual void seek( long l, int dir )
//			{ _tlseek( fd, l, dir ); }
	virtual __off_t tell( void )
		{ return _ttell( fd ); }
	int get_fd(void)
		{return fd;}
#ifndef WINCE
	int eof(void)
		{return ::_eof(fd);}
	long get_length(void)
		{return ::filelength(fd);}
#endif
	int operator ! ()
		{return err;}
	int is_open( void )
		{return fd != -1;}
	const tchar *get_filename( void )
		{return filename;}
	long get_handle()
#ifdef _Windows
		{ return _get_osfhandle(fd); }	// Return the handle of OS.
#else
		{ return fd; }
#endif
};

class IFile : public File {
	protected:
	public:
		IFile(){}
		virtual ~IFile(){}
		virtual int create( const tchar * )
			{ return -1; }
};

class TTextFileBuffer;

class TIFile : public IFile {
typedef IFile super;
protected:
	TTextFileBuffer *readbuff;
	int textmode;
public:
	TIFile();
	virtual ~TIFile();
	int gettextmode() const
		{ return textmode; }
	virtual int open(const tchar* _filename ) { return open( _filename, 0 ); }
	int open(const tchar* _filename, int defcharcode );
	void flush( void );
	int getline( tchar *buf, int maxlen );
	int getline( tnstr &buf );
//	int getline( __Char &buf );
//	int getline2( __Char &buf );		// can use '\r' text as linefeed(Mac)

#if defined(_UNICODE) && defined(TNANSI)
	int getlineA( tnstr &buf );		// ANSI code
//	int getlineA( __Char &buf );		// ANSI code
#endif
	int getA();
#if defined(__STRING_STL) || defined(_STLP_STRING) || defined(_STRING_) /* _STRING_ :MSVC */ || defined(UNIX)
	int getline(std::string &buf);
#endif
//	long tellA( );

	int skipline();

	int getword( tchar *buf, int maxlen );
//	int getitem( __Char &buf, int delim );
	void seek(long l);
	__off_t tell( );
	virtual int get( );
#ifdef _UNICODE
protected:
	unsigned short skipmark();
#endif
public:
#ifdef _UNICODE
	virtual void home( );
#endif
protected:
	bool unicode;		// read bytes as unicode
public:
	utf16_t bom;		// Unicode byte order mark(BOM)
	bool isunicode() const	// open()後に有効
		{ return unicode; }
protected:
	bool createFixedTextBuffer(int buffsize);
};

class BIFile : public IFile {
	public:
		BIFile(){}
		virtual ~BIFile(){}
		virtual int open( const tchar* _filename );
		int read( void *buf, int len );
};

class OFile : public File {
	protected:
		virtual int open( const tchar *_filename, int trunc );
	public:
		OFile(){}
		virtual ~OFile(){}
		virtual int open( const tchar *_filename )
			{ return open( _filename, 0 ); }
		virtual int create( const tchar *_filename )
			{ return open( _filename, 1 ); }
//		virtual void close( );
//		void seek( long l, int dir );
};

class TOFileBase : public OFile {
typedef OFile super;
protected:
	int curp;
	virtual int open( const tchar *filename, int trunc )
		{ return OFile::open( filename, trunc ); }
	int textmode;
public:
	TOFileBase();
	virtual int open( const tchar *_filename );
	virtual int create( const tchar *_filename );
	virtual void close( );
	virtual int flush( ) = 0;
	virtual void seek(long l);
	virtual __off_t tell( );
	virtual void end( );
	void settextmode(int _textmode);
	int bom();
	int putbin( const char *str, int len );
protected:
	int write(const void *buf, int size)
		{ return ::_twrite(fd, buf, size); }
};

template <class T>
class TOFileT : public TOFileBase {
typedef TOFileBase super;
protected:
	T *writebuff;
public:
	TOFileT();
	virtual ~TOFileT();
};

template <class T>
TOFileT<T>::TOFileT()
{
	writebuff = new T[ BUFFSIZE ];
}

template <class T>
TOFileT<T>::~TOFileT()
{
	delete[] writebuff;
}

class TOFileW : public TOFileT<utf16_t> {
typedef TOFileT<utf16_t> super;
public:
	virtual ~TOFileW();
	int flush( );
	int put(const wchar_t *str, int length);
	int put(const wchar_t *str);
	int putline( const wchar_t *str );
	int put(const wchar_t c);
	int putdir(const wchar_t c);
	OFile &operator << ( const wchar_t *str )
		{put( str ); return *this;}
};

class TOFileA : public TOFileT<char> {
typedef TOFileT<char> super;
public:
	virtual ~TOFileA();
	int flush( );
	int put(const char *str, int length);
	int put(const char *str);
	int putline( const char *str );
	int put(const char c);
	int putdir(const char c);
	OFile &operator << ( const char *str )
		{put( str ); return *this;}
	inline int putasbinary(const char *s, int len)	// backward compatibility
		{ return put(s, len); }
};

#ifdef _UNICODE
class TOFile : public TOFileW {
typedef TOFileW super;
};
#else
class TOFile : public TOFileA {
typedef TOFileA super;
};
#endif

#if 0
class TIOFile : public TIFile, public TOFile {
	public:
		TIOFile(){}
		virtual ~TIOFile(){}
		virtual int open( const tchar *fname );
		void close( );
		void seek( long l );
};

#endif


class BOFile : public OFile {
	protected:
		virtual int open( const tchar *filename, int trunc )
			{ return OFile::open( filename, trunc ); }
	public:
		BOFile(){}
		virtual ~BOFile(){}
		virtual int open( const tchar *_filename );
		virtual int create( const tchar *_filename );
		int write( const void *buf, int len );
};

class BIOFile : public BIFile, public BOFile {
	protected:
		virtual int open( const tchar *filename, int trunc )
			{ return BOFile::open( filename, trunc ); }
	public:
		BIOFile(){}
		virtual ~BIOFile(){}
		int open( const tchar *fname );
};

//Note: C++Builder5/64bit seek非対応
class TCharCodeStreamFD : public TCharCodeStreamBase {
protected:
	int fd;
public:
	TCharCodeStreamFD(int _fd)
		:fd(_fd)
	{}
	__override int read(void *Buffer, int Count)
	{
		return ::read(fd, Buffer, Count);
	}
	__override int write(const void *Buffer, int Count)
	{
		return ::write(fd, Buffer, Count);
	}
	__override __off_t seek(__off_t Offset, int Origin)
	{
		return _lseek(fd, (off_t)Offset, Origin);
	}
	__override __off_t tell()
	{
		return ::tell(fd);
	}
};

#ifdef _Windows
class TCharCodeStreamW32 : public TCharCodeStreamBase {
protected:
	HANDLE Handle;
public:
	TCharCodeStreamW32(HANDLE _handle);
	__override int read(void *Buffer, int Count);
	__override int write(const void *Buffer, int Count);
	__override __off_t seek(__off_t Offset, int Origin);
	__override __off_t tell();
};
#endif

__int64 FileTime(const tchar *filename);
bool is_dir(const tchar *path);

#endif	// __FILE_H
