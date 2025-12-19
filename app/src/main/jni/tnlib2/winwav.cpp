//
//	Windows WAVEFORMAT file I/O
//
//	1995.6.4 created by T.Nishikawa
//
//
#include "tnlib.h"
#pragma	hdrstop
#include "winwav.h"


WaveIO::WaveIO( )
{
	hmmio = NULL;
	error = 0;
	fWriting = FALSE;
	mminfo1 = mminfo2 = NULL;
}

WaveIO::~WaveIO( )
{
	if ( hmmio )
		Close( );
}

BOOL WaveIO::Open( const tchar *filename, FOURCC fccHeader )
{
	hmmio = MOpen( filename );
	if ( !hmmio ){
		error = MME_OPEN;
		return FALSE;
	}
    return OpenCom( fccHeader );
}

BOOL WaveIO::MemoryOpen( const tchar *buffer, long length, FOURCC fccHeader )
{
	MMIOINFO mminfo;
	memset( &mminfo, 0, sizeof(MMIOINFO) );
	mminfo.fccIOProc = FOURCC_MEM;
	mminfo.pchBuffer = (LPSTR)buffer;
	mminfo.cchBuffer = length;
	hmmio = mmioOpen( NULL, &mminfo, MMIO_READWRITE );
	if ( !hmmio ){
		error = MME_OPEN;
		return FALSE;
	}
    return OpenCom( fccHeader );
}

BOOL WaveIO::OpenCom( FOURCC fccHeader )
{
	MMCKINFO    mmckinfoParent;  /* parent chunk information structure */
	MMCKINFO    mmckinfoSubchunk; /* subchunk information structure    */

	fWriting = FALSE;
	mmckinfoParent.fccType = fccHeader ? fccHeader : mmioFOURCC('W', 'A', 'V', 'E');
	if (Descend( (LPMMCKINFO) &mmckinfoParent, NULL, MMIO_FINDRIFF)){
		error = MME_NOTWAVE;
		Close( );
		return FALSE;
	}

	mmckinfoSubchunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
	if (Descend( &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK)){
		error = MME_FORMATERROR;
		Close( );
		return FALSE;
	}

	DWORD dwFmtSize = mmckinfoSubchunk.cksize;
	if ( dwFmtSize > sizeof(PCMWAVEFORMAT) ){
		dwFmtSize = sizeof(PCMWAVEFORMAT);
	}
	if ( Read( (HPSTR) &pcm, dwFmtSize) != (LRESULT)dwFmtSize ){
		error = MME_FORMATERROR;
		Close( );
		return FALSE;
	}
	WAVEFORMAT &wf = pcm.wf;
	if ( dwFmtSize < sizeof(PCMWAVEFORMAT) || wf.wFormatTag != WAVE_FORMAT_PCM ){
		error = MME_FORMATERROR;
		Close( );
		return FALSE;
	}

	Ascend( &mmckinfoSubchunk );

	mmckinfoSubchunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
	if ( Descend( &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK)){
		error = MME_NODATA;
		Close( );
		return FALSE;
	}

	datalength = mmckinfoSubchunk.cksize;
	if ( datalength == 0L) {
		error = MME_NODATA;
		Close( );
		return FALSE;
	}

	return TRUE;
}

BOOL WaveIO::CreateCom( DWORD length, FOURCC fccHeader )
{
	// begin of RIFF ///////////////////////////////
	mminfo1 = new MMCKINFO;
	mminfo1->fccType = fccHeader ? fccHeader : mmioFOURCC( 'W', 'A', 'V', 'E' );
	CreateChunk( mminfo1, MMIO_CREATERIFF );

	// write header ////////////////////////////////

	mminfo2 = new MMCKINFO;
	mminfo2->cksize = sizeof( PCMWAVEFORMAT );
	mminfo2->ckid = mmioFOURCC( 'f', 'm', 't', ' ' );
	mminfo2->fccType = 0;
	mminfo2->dwDataOffset = 0;
	mminfo2->dwFlags = 0;
	CreateChunk( mminfo2, 0 );

	if ( (int)Write( (HPSTR)&pcm, sizeof( PCMWAVEFORMAT ) ) != sizeof( PCMWAVEFORMAT ) ){
		error = MME_WRITE;
		Close( );
		return FALSE;
	}
	Ascend( mminfo2 );
	// end of write header /////////////////////////

	// write data //////////////////////////////////

	mminfo2->ckid = mmioFOURCC( 'd', 'a', 't', 'a' );
	mminfo2->fccType = 0;
	mminfo2->dwDataOffset = 0;
	mminfo2->dwFlags = 0;
	mminfo2->cksize = length;
	CreateChunk( mminfo2, 0 );

	fWriting = TRUE;	// データ書き込み許可
	return TRUE;
}

void WaveIO::Close( )
{
	if ( fWriting ){
		Ascend( mminfo2 );

		// end of write data ///////////////////////////

		Ascend( mminfo1 );
		// end of RIFF /////////////////////////////////

		fWriting = FALSE;
	}
	if ( mminfo2 ){
		delete mminfo2;
		mminfo2 = NULL;
	}
	if ( mminfo1 ){
		delete mminfo1;
		mminfo1 = NULL;
	}
	mmioClose( hmmio, 0 );
	hmmio = NULL;
}

// buffer, lengthは初期値を与える(実際のデータに必要なものより小さくても構わない)
// bufferは必ずGlobalAlloc()で割り当てたものであること
BOOL WaveIO::MemoryCreate( uint8_t *buffer, DWORD length, FOURCC fccHeader )
{
	MMIOINFO mminfo;
	memset( &mminfo, 0, sizeof(MMIOINFO) );
	mminfo.fccIOProc = FOURCC_MEM;
	mminfo.pchBuffer = (HPSTR)buffer;
	mminfo.cchBuffer = length;
	mminfo.adwInfo[0] = 8192;	// メモリ拡張の増分
	hmmio = mmioOpen( NULL, &mminfo, MMIO_CREATE|MMIO_READWRITE );
	if ( !hmmio ){
		error = MME_OPEN;
		return FALSE;
	}
    return CreateCom( length, fccHeader );
}

