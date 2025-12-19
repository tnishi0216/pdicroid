#ifndef __Hdicdata_h
#define	__Hdicdata_h

#include "dicdata.h"

#define	FIELDTYPE		0x8000

///////////////////////
///	HPdicDataクラス	///
///////////////////////
// Hyper Pdic Data class
#define	HBLKNUM(x)		((t_blknum)((x)&~FIELDTYPE))
#define	HPDICDATA(x)	((HPdicData*)x)
class HPdicData : public PdicData {
typedef PdicData super;
#ifdef ND3ONLY
friend PdicData;
#endif
protected:
	// Field1,2兼用関数
	int _write_databuf(t_pbn2 pbn);
	class TDicDec *DicDec;
	char *TempBuffer;
	int TempBufferSize;
public:
	HPdicData( int &error, FileBuf &_file);
	virtual ~HPdicData();

	virtual int Open( const HEADER *header, class IndexData *inxdat );

	// Field1,2兼用関数
	int GetTopLoc( )			{ return sizeof(t_blknum); }

	int read(t_pbn2 pbn, TDataBuf *databuf=NULL, bool fit_size=true);
protected:
	bool ReallocTempBuffer(unsigned nsize);

public:
	int recSearch( const _kchar * sword, _kchar *wbuf, FINDWORD &fw, TDataBuf &buf );
#if !defined(SMALL)
	int recSearchDirect( const _kchar *word, _kchar *wbuf, FINDWORD &fw, TDataBuf &databuf );
#endif
	int eraseBlock(t_pbn2 pbn);
	t_pbn2 GrowBlock( t_pbn2 pbn, t_blknum oldblknum, t_blknum num, int dataset );
	void ShrinkBlock( t_pbn2 pbn, t_blknum num );
	uint GetMaxBlockSize( )
		{ return blocksize * MAX_BLOCKNUM; }
	t_pbn2 newBlock( int num );		//返り値：物理ブロック番号 -1=エラー
	t_pbn2 NewBlock( int num );

	// Field1専用関数
	int addWord( const _kchar * word, uint wordlen, const uint8_t *japa, uint japalen, FINDWORD &fw, DivInfo *divinfo );
	int addWord2( const _kchar * word, uint wordlen, const uint8_t *japa, uint japalen, t_pbn2 pbn );
	int updateWord( const uint8_t *japa, uint japalen, FINDWORD &fw, DivInfo *divinfo );
	int updateWord( const _kchar *, const uint8_t *, uint , FINDWORD &, DivList *, int &, int  ){ return -1; }
	int eraseWord( t_pbn2 pbn, int loc );
	int DivBlock( const _kchar *word, const uint8_t *japa, uint japalen, FINDWORD &fw, int fNew, DivInfo *divinfo, PdicIndex *index );
};

#endif	/* __Hdicdata_h */

