#ifndef __WINWAV_H
#define	__WINWAV_H

#define	MME_OPEN		1	// オープンエラー
#define	MME_NOTWAVE		2	// waveファイルでない
#define	MME_FORMATERROR 3	// waveフォーマットエラー
#define	MME_NODATA		4	// データはない
#define	MME_READ		5	// 読み込みエラー
#define	MME_WRITE		6	// 書き込みエラー
#define	MME_MEMORY		7	// メモリ不足

#ifndef __MMSYSTEM_H
#include <mmsystem.h>
#endif

#ifdef WIN32
#else
typedef UINT MMRESULT;
#endif

class WaveIO {
	public:
		HMMIO hmmio;
		int error;
		PCMWAVEFORMAT pcm;
		DWORD datalength;
		BOOL fWriting;		// データ書き込み中
	protected:
		// 書き込み用データ
		MMCKINFO *mminfo1;	// RIFFチャンク
		MMCKINFO *mminfo2;    // Subチャンク
	public:
		WaveIO( );
		~WaveIO( );
		// 読み込み用(file I/O)
		BOOL Open( const tchar *filename, FOURCC fccHeader=NULL );
		BOOL BufferdOpen( const tchar *filename, FOURCC fccHeader=NULL );
		// 読み込み用(memory file)
		BOOL MemoryOpen( const tchar *buffer, long length, FOURCC fccHeader=NULL );
	protected:
    	BOOL OpenCom( FOURCC fccHeader );
	public:

		LRESULT Read( HPSTR buf, long maxlen )
			{ return mmioRead( hmmio, buf, maxlen ); }
		LRESULT Seek( LONG offset, int direction )
			{ return mmioSeek( hmmio, offset, direction ); }
		DWORD GetLength( )
			{ return datalength; }
		// 書き込み用
		// 注意：Create()を呼ぶ前に、メンバ変数pcmを正しくセットしておく事！
		BOOL Create( const tchar *filename, long datalength, FOURCC fccHeader=NULL );
		// buffer,lengthは最小のサイズでも構わない
		// bufferはGlobalAllocしたものであること
		BOOL MemoryCreate( uint8_t *buffer, DWORD length, FOURCC fccHeader=NULL );
		BOOL CreateCom( DWORD length, FOURCC fccHeader );
		LRESULT Write( HPSTR buf, LONG len )
			{ return mmioWrite( hmmio, (HPSTR)buf, len ); }
			// 注意：Create()で指定したdatalengthとWrite()で書き込んだデータの合計数は一致する事！(?)
		// 共通
		void Close( );
		int GetError( )
			{ return error; }
	protected:
		// WindowsのmmioXXXX関数群のカプセル化	///////////////////////////////
		HMMIO MOpen( const tchar *filename )
			{ return mmioOpen( (LPTSTR)filename, NULL, MMIO_READ ); }
		HMMIO MOpen( const tchar *filename, DWORD flags )
			{ return mmioOpen( (LPTSTR)filename, NULL, MMIO_READ | flags ); }
		HMMIO MCreate( const tchar *filename )
			{ return mmioOpen( (LPTSTR)filename, NULL, MMIO_CREATE|MMIO_READWRITE ); }
		HMMIO MCreate( const tchar *filename, DWORD flags )
			{ return mmioOpen( (LPTSTR)filename, NULL, MMIO_CREATE|MMIO_READWRITE | flags ); }
		LRESULT MWrite( void *buf, LONG len )
			{ return mmioWrite( hmmio, (const char*)buf, len ); }
		MMRESULT Descend( LPMMCKINFO mmck1, LPMMCKINFO mmck2, UINT flag )
			{ return mmioDescend( hmmio, mmck1, mmck2, flag ); }
		MMRESULT Ascend( LPMMCKINFO mmck )
			{ return mmioAscend( hmmio, mmck, 0 ); }
		MMRESULT CreateChunk( LPMMCKINFO mmck, UINT option )
			{ return mmioCreateChunk( hmmio, mmck, option ); }

		// 演算子
	public:
		operator PCMWAVEFORMAT &()
			{ return this->pcm; } 
};
#endif	__WINWAV_H
