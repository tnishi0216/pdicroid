#include "tndefs.h"
#pragma hdrstop
#include <string.h>

#ifdef _Windows
#include "UserMsg.h"
#endif

/*------------------------------------------*/
/*		Configuration						*/
/*------------------------------------------*/
#define	DEF_BLOCKSIZE	0x2000	// 8KB

/*------------------------------------------*/
/*		Type Definitions					*/
/*------------------------------------------*/

#include "RangeCodec.h"

typedef unsigned short freq_t;	// type definition for fequency
typedef unsigned int code_t;	// type definition of range coder value

struct TRangeCoder {
	unsigned low;	// low value of range
	unsigned range;	// range
	unsigned totfreq;	// total frequency(number of data)
	uint8_t *dp;		// destination pointer to be stored
};
#define RC_CODE_BITS	32
#define RC_TOPVAL		((code_t)1<<24)
#define RC_BOTTOMVAL	(RC_TOPVAL>>8)

struct TGammaParamC {
	unsigned putcount;
	unsigned bitbuf;
	uint8_t *dp;		// destination pointer to be stored
};

struct TGammaParamD {
	int getcount;
	unsigned bitbuf;
	const uint8_t *sp;		// data source
};

int rcEncode( const uint8_t *sp, int sp_len, uint8_t *dp );
int rcDecode( const uint8_t *sp, uint8_t *dp );
static void PutGamma( TGammaParamC &gp, unsigned short code );
static void PutGammaEnd( TGammaParamC &gp );

static unsigned GetGamma( TGammaParamD &gp );

void DBW(const char *,...);	// for debug

#ifdef _Windows
static HWND hwndRC;
#endif

// return value:
//	1 : OK
//	0 : NG(逆に大きくなる)
int RCEncode( const uint8_t *sp, int sp_len, uint8_t *dp, int &destlen )
{
	RC_HEADER *header = (RC_HEADER*)dp;
	header->size = sizeof(RC_HEADER);
	memset(header->reserved,0,sizeof(header->reserved));
	memcpy( header->method,"rc1",4);
	header->orgsize = sp_len;
	header->blocksize = DEF_BLOCKSIZE;
	header->compsize = 0;

	dp += sizeof(RC_HEADER);
	int restsize = sp_len;
	for(;sp_len;){
		unsigned short size;	// 圧縮後のサイズ
		int complen = sp_len < header->blocksize ? sp_len : header->blocksize;
		size = (unsigned short)rcEncode( sp, complen, dp+sizeof(short) );
		*(unsigned short*)dp = size;
		size += sizeof(short);	// size including sizeof(short)
		header->compsize += size;
		dp += size;
		sp += complen;
		sp_len -= complen;
		restsize -= size;
		if (restsize<DEF_BLOCKSIZE*2){
			// 圧縮しても小さくならない
			if (sp_len>DEF_BLOCKSIZE){
				return 0;
			}
		}
	}
	destlen = header->compsize+sizeof(RC_HEADER);
//	DBW("org=%d cmp=%d",header->orgsize,header->compsize);
	return 1;
}
int RCDecode( const uint8_t *src, int srclen, uint8_t *dest, int &destlen )
{
	RC_HEADER *header = (RC_HEADER*)src;
	if (header->size<sizeof(RC_HEADER)
		|| strcmp(header->method,"rc1") ){
		return 0;
	}
	destlen = 0;
	src += header->size;
	const uint8_t *end = src + header->compsize-256/8;
	for(;src<end;){
		unsigned short size = *(unsigned short*)src;
		src += sizeof(short);
		destlen += rcDecode( src, dest );
		src += size;
		dest += header->blocksize;
	}
	return 1;
}
inline void rc_encode( TRangeCoder &rc, unsigned cumfreq, unsigned freq )
{
	rc.range /= rc.totfreq;
	rc.low += cumfreq * rc.range;
	rc.range *= freq;
	while ((rc.low ^ (rc.low+rc.range)) < RC_TOPVAL){
		*rc.dp++ = (uint8_t)(rc.low>>24);
		rc.range <<= 8;
		rc.low <<= 8;
	}
	while (rc.range<RC_BOTTOMVAL){
		*rc.dp++ = (uint8_t)(rc.low>>24);
		rc.range = ((-rc.low)&(RC_BOTTOMVAL-1))<<8;
		rc.low <<= 8;
	}
}

// sp_len must be less than max value of freq_t
// dp はあらかじめ用意したバッファ（2048バイト以上あれば大丈夫かな・・・かなり怪しい）
//int rcCompress( const unsigned char *sp, int sp_len, char *dp )
int rcEncode( const uint8_t *sp, int sp_len, uint8_t *dp )
{
	unsigned freq[257];	// 出現度数table
	freq_t cumfreq[258];// 累積度数table
	unsigned blocksize;	// 一度にRangeCoder圧縮を行うsize[byte]
						// must be less than 64KB

	TRangeCoder rc;
	unsigned i;
	unsigned l;
	const uint8_t *p;

	int index;

	blocksize = sp_len;	// size of data

	// Count frequency and update counts table //
	for ( i=0;i<256;i++ )
		freq[i] = 0;
	l = blocksize;
	p = sp;

	// 出現頻度のカウント
	while (l-->0){
		freq[*p++]++;
	}

	// 出現頻度の最大値を256未満に抑える
	unsigned hmax = 0;
	for (i=0;i<256;i++)
		if (freq[i]>hmax) hmax = freq[i];
	if (hmax>=256)
		for (i=0;i<256;i++)
			freq[i] = (freq[i]*255+hmax-1)/hmax;

	// Encode frequency table //
	TGammaParamC gp;
	gp.putcount = 8;	// 初期化 for γ符号
	gp.dp = (uint8_t*)dp;	// 初期化 for γ符号
	gp.bitbuf = 0;

	for (i=0;i<256;i++)
		PutGamma( gp, freq[i]+1 );
	PutGammaEnd( gp );
//	DBW("GammaSize=%d",(int)gp.dp-(int)dp);

	// Initialize rc //
	rc.low = 0;
	rc.range = 0xffffffffu;

	// 出現度数 → 累積出現度数
	freq[256]=1;
	cumfreq[0] = 0;
	for (i=1;i<258;i++)
		cumfreq[i] = cumfreq[i-1] + freq[i-1];

	// Encode rc_data //
	rc.totfreq = cumfreq[257];
	rc.dp = (uint8_t*)gp.dp;
	for (i=0;i<blocksize;i++){
		index = sp[i];
		rc_encode( rc, cumfreq[index], freq[index] );
#ifdef _Windows
		if ( hwndRC && (i&0x3FFFF)==0 ){
			SendMessage( hwndRC, UM_COMP, MSGCOMP_RATIO, i*100/blocksize );
		}
#endif
	}
	// end flag
	for (i=0;i<8;i++){
		rc_encode( rc, cumfreq[256], 1 );
	}

	return (int)((uint8_t*)rc.dp - dp);
}
#define rightbits(n,x) ((x)&((1U<<(n))-1U))
static
void putbit( TGammaParamC &gp, unsigned bit )
{
	gp.putcount--;

	if( bit != 0 )
		gp.bitbuf |= (1 << gp.putcount);

	if( gp.putcount == 0 )
	{
		*gp.dp++ = gp.bitbuf;
		gp.bitbuf = 0;
		gp.putcount = 8;
//		gp.bytecount++;
	}
}

/* 整数用対数 log2 num を返す filt=2^(log2 num) */
static
unsigned I_log(unsigned num, unsigned *filt)
{
	*filt = 1;

	unsigned i;
	for( i = 0; *filt <= num; i++ )
		*filt <<= 1;

	return i;
}

/* filt 通りのbit幅で codeを符号化 */
static
void fit_code( TGammaParamC &gp, unsigned code, unsigned filt)
{
	unsigned bit;

	filt >>= 1;
	while( filt > 0 )
	{
		bit = ((code & filt) == 0) ? 0 : 1;
		putbit( gp, bit );
		filt >>= 1;
	}
}

/* 一致記号数の符号化 code:1~31 */
static
void PutGamma( TGammaParamC &gp, unsigned short code )
{
	unsigned i;
	unsigned ret;
	unsigned leng;

	leng = I_log( code, &ret );

	for( i = 1; i < leng; i++ )
		putbit(gp,0);

	fit_code( gp, code, ret );
}
static
void PutGammaEnd( TGammaParamC &gp )
{
	if ( gp.putcount == 8 ) return;
	gp.bitbuf |= rightbits( gp.putcount, 0 );
	*gp.dp++ = gp.bitbuf;
}

int rcDecode( const uint8_t *sp, uint8_t *dp )
{
	// start decoding //
	// start decoding //
	freq_t freq[257];
	freq_t cumfreq[258];
	int i;

	// decode gamma coding //
	TGammaParamD gp;
	gp.getcount = 0;
	gp.bitbuf = 0;
	gp.sp = sp;
	for (i=0;i<256;i++)
		freq[i] = GetGamma(gp) - 1;
	// 出現度数 → 累積出現度数
	freq[256]=1;
	cumfreq[0] = 0;
	for (i=1;i<258;i++)
		cumfreq[i] = cumfreq[i-1] + freq[i-1];

	const uint8_t *rsp = gp.sp;
	unsigned low = 0;
	unsigned range = 0xffffffffu;
	unsigned code = 0;
	for (i=0;i<4;i++)
		code = (code<<8) | *rsp++;

	uint8_t *rdp = dp;
	unsigned totfreq = cumfreq[257];

	for (;;){
		unsigned cf;		// frequency

		// get frequency //
		range /= totfreq;
		cf = (code-low) / range;
		if (cf >= totfreq){
			// error
			// 残りすべてnullとして扱う
//				memset( rdp, 0, blocksize-((int)rdp-(int)dp) );
			//printf("DRC32 Error: input data corrupt\n");
			break;
		}

		int lo, hi;
		// find symbol using binary search
		{
			lo = 0;
			hi = 256;
			while (lo < hi){
				int mid = (lo+hi)>>1;
				if (cf < cumfreq[mid+1])
					hi = mid;
				else
					lo = mid+1;
			}
			if (lo==256){
				// EOF
				break;
			}
		}
		*rdp++ = (uint8_t)lo;
		// decode //
		low += cumfreq[lo] * range;
		range *= freq[lo];
		while ((low ^ (low+range)) < RC_TOPVAL){
			code = (code<<8) | *rsp++;
			range <<= 8;
			low <<= 8;
		}
		while (range<RC_BOTTOMVAL){
			code = (code<<8) | *rsp++;
			range = ((-low)&(RC_BOTTOMVAL-1))<<8;
			low <<= 8;
		}
	}
	return (uint_ptr)rdp-(uint_ptr)dp;
}
inline
unsigned getbit(TGammaParamD &gp)
{
	if ( --gp.getcount >= 0 )
		return (gp.bitbuf >> gp.getcount) & 1U;
	gp.getcount = 7;
	gp.bitbuf = *gp.sp++;
	return (gp.bitbuf >> 7) & 1U;
}
static unsigned GetGamma( TGammaParamD &gp )
{
	int leng = 0;
	while( !getbit(gp) )
		leng++;

	unsigned code = 1;
	while (leng-- > 0)
		code = (code << 1) + getbit(gp);
	return code;
}
#ifdef _Windows
void RCSetWindow( HWND hwnd )
{
	hwndRC = hwnd;
}
#endif

