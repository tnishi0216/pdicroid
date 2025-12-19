#ifndef __filebuf_h
#define	__filebuf_h

#include "dicdef.h"

#if defined(WIN32) && !defined(UNIANSI)
#  define	USE_HANDLEFILE
#  define	INVALID_FD	INVALID_HANDLE_VALUE
#  define	DIC_STDIO	0	// uses open
#else
#  if defined(UNIX)
#    define	DIC_STDIO	1	// uses fopen
#    define	INVALID_FD	NULL
#    define USE_FOPEN
#  else		// !UNIX
#    define	DIC_STDIO	0	// uses open
#    ifdef _Windows
#      define	INVALID_FD	HFILE_ERROR
#    else
#      define	INVALID_FD	-1
#    endif
#    define USE_OPEN
#  endif	// !UNIX
#endif

#if DIC_STDIO
#  define	HANDLE_FB	FILE *
#elif defined(USE_HANDLEFILE)
#  define	HANDLE_FB	HANDLE
#elif defined(USE_OPEN)
#  define	HANDLE_FB	int
#else
#  define	HANDLE_FB	HFILE
#endif

class FileBuf {

#ifdef WIN32
#define	SEQBUFFSIZE	256
#else
#define	SEQBUFFSIZE	64
#endif

private:
	HANDLE_FB fd;
	tnstr filename;		// ファイル名
	tnstr fullfilename;	// フルファイル名
	bool ReadOnly;
	int ModifyCounter;					//辞書修正 counter
	int LastModifyCounter;
#ifdef USE_HANDLEFILE
	DWORD rwcount;
#else
	int rwcount;				//実際に読み書きされたバイト数
#endif
	int &error;
#ifdef SWAP_ENDIAN
	uint8_t seqbuf[SEQBUFFSIZE];	//シーケンシャルアクセスバッファ
	int seqloc;
#endif
private:
	int _open( const tchar *, int mode);
//		void error_proc(void);
public:
	static int dos_error_proc( int errcode );
public:
	FileBuf(int &_error);
	virtual ~FileBuf();
	HANDLE_FB GetFileHandle( ) const
		{ return fd; }
	const tchar *GetFileName( )
		{ return filename; }
	const tchar *GetFullFileName( )
		{ return fullfilename; }
	int Open( const tchar * );
	int read_open( const tchar * );
	int create( const tchar * );
	int Create( const tchar *fname )
		{ return create( fname ); }
	void close( );
	void Close( )
		{ close(); }
	bool isOpen( ) const
		{return fd != INVALID_FD;}
	int GetErrorCode()
		{ return error; }

	//ランダムアクセス
	int read( void *buf, int len);
	int write( const void *buf, int len);
	int seek(__pdc64 off);
	int seekcur(__pdc64 offs);
	void SeekEnd( );

	//シーケンシャルアクセス（バッファリングあり）
	//ランダムアクセスのread,writeは絶対に使わない
	int getch();

	bool isReadOnly() const { return ReadOnly; }
	bool isModify( ) const	{return ModifyCounter!=LastModifyCounter;}
	bool modifyCounter() const { return ModifyCounter; }
	void cancelModify()
		{ LastModifyCounter = ModifyCounter; }
	void setModify()
		{ ModifyCounter++; }
	__pdc64 getFreeSize(void) const;
#if defined(USE_HANDLEFILE) || DIC_STDIO
	__pdc64 chsize(__pdc64 l);
#elif defined(UNIX) || defined(__ANDROID__)
	__pdc64 chsize(__pdc64 l)	{return ::ftruncate(fd, l);}
#else
	__pdc64 chsize(__pdc64 l)	{return ::chsize(fd, l);}
#endif
	__pdc64 get_length( ) const
#ifdef USE_HANDLEFILE
	{
		DWORD hi;
		DWORD size = ::GetFileSize( (HANDLE)fd, &hi );
  #if sizeof(__pdc64)==8
		if (size==0xFFFFFFFF)
			return -1;
		return MAKE64(hi, size);
  #else
		return size;
  #endif
	}
#elif DIC_STDIO
	;
#else
		{ return ::filelength( fd ); }
#endif
	// getch()を使用している場合のみ有効！！
	__pdc64 tellch( ) const
#ifdef USE_HANDLEFILE
	{
#ifdef SWAP_ENDIAN
		__pdc64 size = _tell();
		if (size==-1)
			return -1;
		return size - ( SEQBUFFSIZE - seqloc );
#else
		return _tell();
#endif
	}
#elif DIC_STDIO
		{ return ::ftell(fd); }
#else
		{ return ::tell( fd )
#ifdef SWAP_ENDIAN
			- ( SEQBUFFSIZE - seqloc )
#endif
			; }
#endif
	// getch()を使用している場合は無効！！
	__pdc64 _tell( ) const
#ifdef USE_HANDLEFILE
	{
  #if sizeof(__pdc64)==8
		LONG hi = 0;
		DWORD size = ::SetFilePointer( fd, 0, &hi, FILE_CURRENT );
		if (size==0xFFFFFFFF)
			return -1;
		return MAKE64(hi, size);
  #else
		return ::SetFilePointer( fd, 0, NULL, FILE_CURRENT );
  #endif
	}
#elif DIC_STDIO
		{ return ::ftell(fd); }
#else
		{ return ::tell( fd ); }
#endif
};

#endif	/* __filebuf_h */

