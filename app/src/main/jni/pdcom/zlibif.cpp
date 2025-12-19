#include "tnlib.h"
#pragma hdrstop
#include "zlibif.h"
#include "../dicLib/est/zlib/zlib.h"

#define	METHOD_SIG	"zl1"
#define	DEF_BLOCKSIZE 0	//TODO:

#ifdef _Windows
static HWND hwndZLIB;
#endif

int zlibEncode( const uint8_t *sp, long sp_len, uint8_t *dp, long &destlen )
{
	ZLIB_HEADER *header = (ZLIB_HEADER*)dp;
	header->size = sizeof(ZLIB_HEADER);
	memset(header->reserved,0,sizeof(header->reserved));
	memcpy( header->method,METHOD_SIG,4);
	header->orgsize = sp_len;
	header->blocksize = DEF_BLOCKSIZE;
	header->compsize = 0;

	destlen = 0;
	
	__assert(sp_len<0x1000000);	// To avoid overflow.
	unsigned long compsize = sp_len*101/100+12;	// \•ª‚È—Ìˆæ‚ª‚ ‚é‚Æ‰¼’è
	// __assert(compsize>=sp_len*1.001+12) <- exact expression
	if (compress(dp, &compsize, sp, sp_len)!=Z_OK){
		return 0;	// error
	}
	header->compsize = compsize;
	destlen = header->compsize+sizeof(ZLIB_HEADER);
	return 1;
}

int zlibDecode( const uint8_t *src, long srclen, uint8_t *dest, long &destlen )
{
	ZLIB_HEADER *header = (ZLIB_HEADER*)src;
	if (header->size<sizeof(ZLIB_HEADER)
		|| strcmp(header->method, METHOD_SIG) ){
		return 0;
	}
	destlen = 0;
	src += header->size;
	unsigned long orgsize = header->orgsize;
	if (uncompress(dest, &orgsize, src, header->compsize)!=Z_OK){
		return 0;	// error
	}
	destlen = orgsize;
	return 1;
}

#ifdef _WINDOWS
void zlibSetWindow( HWND hwnd )
{
	hwndZLIB = hwnd;
}
#endif

