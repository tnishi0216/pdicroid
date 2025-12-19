#ifndef __RANGE_CODEC_H
#define	__RANGE_CODEC_H

//int rcCompress( const unsigned char *sp, int sp_len, char *dp );
//int rcDecode( const char *sp, char **sp_next, char *dp );

struct RC_HEADER {
	unsigned char size;
	unsigned char reserved[3];
	char method[4];	// 'rc1'
	unsigned int compsize;
	unsigned int orgsize;
	unsigned int blocksize;	// 0のときは、blocksize=orgsize
};

int RCDecode( const uint8_t *src, int srclen, uint8_t *dest, int &destlen );
int RCEncode( const uint8_t *src, int srclen, uint8_t *dest, int &destlen );
unsigned int RCGetOrgSize( const uint8_t *header );
int RCGetErrorCode( );
#ifdef _WINDOWS
void RCSetWindow( HWND hwnd );
#endif

// オリジナルサイズの取得（拡張ヘッダーサイズは含まない）
inline unsigned int RCGetOrgSize( const uint8_t *header )
{
	return ((const RC_HEADER *)header)->orgsize;
}

#endif	/* __RANGE_CODEC_H */
