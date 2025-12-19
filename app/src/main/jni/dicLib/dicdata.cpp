#include	"tnlib.h"
#pragma	hdrstop
#include "dic.h"
#if defined(DOS)
#include	"env.h"
#endif
#include	"japa.h"
#include "dicdbg.h"
#include "mmf.h"
//#include <assert.h>

#if 0
#define	DBDIC	DBW
#else
inline void DBDIC(const char *,...){}
#endif

TDataBufBase::TDataBufBase(PdicData *data)
{
	Data = data;
	DataBuf = NULL;
}
TDataBufBase::~TDataBufBase()
{
	if (DataBuf){
		delete[] DataBuf;
	}
}

#ifdef _Windows
//
// TDataBufMemMap
//
TDataBufMemMap::TDataBufMemMap(PdicData *data)
	:super(data)
{
	BlockSize = 0;
	LastPtr = NULL;

	MM = NULL;
	BaseAddr = NULL;
}
TDataBufMemMap::~TDataBufMemMap()
{
	Close();
}
bool TDataBufMemMap::Open(unsigned blocksize)
{
	if (MM)
		return false;	// already opened
	BlockSize = blocksize;
	MM = new TMmfMap(Data->file.GetFileHandle());
	BaseAddr = (uint8_t*)MM->map(Data->GetOffset(), Data->GetDataSize(), Data->file.isReadOnly());
	if (!BaseAddr){
		delete MM;
		MM = NULL;
		return false;
	}
	DBDIC("Open : BaseBuf=%08X data_offset=0x%X size=0x%X", (unsigned)BaseAddr, Data->GetOffset(), Data->GetDataSize());
	if (!Data->file.isReadOnly()){
		//TODO: Can I optimize?
		Data->file.setModify();
	}
	return true;
}
bool TDataBufMemMap::Reopen()
{
	return Open(BlockSize);
}
void TDataBufMemMap::Close()
{
	if (MM){
		delete MM;
		MM = NULL;
	}
	DataBuf = NULL;
	LastPtr = NULL;
}
uint8_t *TDataBufMemMap::alloc(t_pbn2 pbn, t_blknum _blknum)
{
	DataBuf = getDataPtr(pbn);
	*(t_blknum*)DataBuf = _blknum;
	return DataBuf;
}
void TDataBufMemMap::setDataPbn(t_pbn2 pbn)
{
	DataBuf = BaseAddr + pbn*BlockSize;
}
int TDataBufMemMap::seek(t_pbn2 pbn, int offs)
{
	return seek_alloc(pbn, offs);
}
int TDataBufMemMap::seek_alloc(t_pbn2 pbn, int offs)
{
	if (MM->size()<pbn*BlockSize+offs){
		assert(offs==0);
		unsigned offs = (unsigned)DataBuf - (unsigned)BaseAddr;
		// Not enough space
		MM->unmap();
		BaseAddr = (uint8_t*)MM->map(Data->GetOffset(), pbn*BlockSize, Data->file.isReadOnly());
		if (!BaseAddr){
			LastPtr = NULL;
			DataBuf = NULL;
			return -1;
		}
		DBDIC("seek_alloc : BaseBuf=%08X data_offset=0x%X size=0x%X", (unsigned)BaseAddr, Data->GetOffset(), pbn*BlockSize);
		if (DataBuf){
			DataBuf = BaseAddr + offs;
		}
	}
	LastPtr = getDataPtr(pbn)+offs;
	return 0;
}
int TDataBufMemMap::read(t_pbn2 pbn, void *data, int len, int off)
{
	uint8_t *DataBuf = getDataPtr(pbn)+off;
	memcpy(data, DataBuf, len);
	LastPtr = DataBuf+len;
	return len;
}
int TDataBufMemMap::read(void *data, int len)
{
	memcpy(data, LastPtr, len);
	LastPtr += len;
	return len;
}
#if 0
int TDataBufMemMap::read_direct(t_pbn2 pbn, void *data, int len, int off)
{
	memcpy(data, getDataPtr(pbn)+off, len);
	LastPtr = DataBuf+len;
	return len;
}
#endif
int TDataBufMemMap::write(t_pbn2 pbn)
{
//	DataBuf = getDataPtr(pbn);
	return 0;
}
int TDataBufMemMap::write(t_pbn2 pbn, const void *data, int len, int off)
{
	LastPtr = getDataPtr(pbn)+off;
	memcpy(LastPtr, data, len);
	LastPtr = LastPtr+len;
	return len;
}
int TDataBufMemMap::write(const void *data, int len)
{
	memcpy(LastPtr, data, len);
	LastPtr += len;
	return len;
}
int TDataBufMemMap::fill(uint8_t data, int len)
{
	memset(LastPtr, data, len);
	LastPtr += len;
	return 0;
}
void TDataBufMemMap::changeOffset(unsigned prev_offset, unsigned new_offset)
{
	BaseAddr = (uint8_t*)MM->changeOffset(new_offset);
	DataBuf = NULL;
	LastPtr = NULL;
}
bool TDataBufMemMap::writeBlockNum(t_pbn2 pbn, t_blknum blknum)
{
	*(t_blknum*)getDataPtr(pbn) = blknum;
	return true;
}
void TDataBufMemMap::set(t_pbn2 pbn, uint8_t *data, t_blknum blknum)
{
	uint8_t *databuf = getDataPtr(pbn);
	*(t_blknum*)databuf = blknum;
	memcpy(databuf, data, blknum*BlockSize);
}
void TDataBufMemMap::grow(t_pbn2 oldpbn, t_pbn2 newpbn, int old_blknum, int new_blknum)
{
	uint8_t *oldptr = getDataPtr(newpbn);
	uint8_t *newptr = getDataPtr(oldpbn);
	if (oldptr!=newptr){
		memcpy(oldptr, newptr, old_blknum*BlockSize);
	}
	memset(oldptr+old_blknum*BlockSize, 0, (new_blknum-old_blknum)*BlockSize);
	*(t_blknum*)oldptr = (isField2(*(t_blknum*)oldptr)?FIELDTYPE:0) | new_blknum;
}
void TDataBufMemMap::shrink(int new_blknum)
{
	*(t_blknum*)getDataBuf() = (t_blknum)(isField2()?FIELDTYPE:0) | new_blknum;
}
#endif

//
// TDataBufFile
//
TDataBufFile::TDataBufFile(PdicData *data)
	:super(data)
{
	fast_mode = 0;
	writef = false;
	lastpbn = BLK_ERROR;
	blknum = 0;
	allocnum = 0;
}
TDataBufFile::~TDataBufFile()
{
}
bool TDataBufFile::Open(unsigned blocksize)
{
	blknum = 0;
	lastpbn = BLK_ERROR;
	BlockSize = blocksize;
	return true;
}
void TDataBufFile::Close()
{
	if ( DataBuf ){
		delete[] DataBuf;
		DataBuf = NULL;
	}
}
// Note:
// _blknum can include FIELDTYPE
uint8_t *TDataBufFile::alloc(t_pbn2 pbn, t_blknum _blknum)
{
	blknum = _blknum;
	allocnum = blknum&~FIELDTYPE;
	if (allocnum==0)
		allocnum++;	// for empty block
	setDataBuf(new uint8_t[ allocnum*Data->blocksize ], blknum, pbn);
	return DataBuf;
}
uint8_t *TDataBufFile::alloc_opt(t_pbn2 pbn, t_blknum _blknum)
{
	if (allocnum>=(_blknum & ~FIELDTYPE)){
		// no need to realloc
		_setDataBuf(_blknum, pbn);
		return DataBuf;
	}
	return alloc(pbn, _blknum);
}
void TDataBufFile::setDataBuf(uint8_t *ptr, t_blknum _blknum, t_pbn2 pbn)
{
	if (DataBuf){
		delete[] DataBuf;
	}
	DataBuf = ptr;
	allocnum = _blknum&~FIELDTYPE;
	_setDataBuf(_blknum, pbn);
}
void TDataBufFile::_setDataBuf(t_blknum _blknum, t_pbn2 pbn)
{
	blknum = _blknum;
	if (DataBuf){
		*(t_blknum*)DataBuf = blknum;
		writef = true;
		lastpbn = pbn;
	}
}

#if USE_DICFASTMODE
void TDataBufFile::setFastMode(int _fast_mode)
{
	if (fast_mode){
		flush();
	}
	fast_mode = _fast_mode;
}
#endif
int TDataBufFile::write(t_pbn2 pbn)
{
#if USE_DICFASTMODE
	if (fast_mode){
		if (writef){
			if (lastpbn == pbn)
				return 0;
			if ( Data->_write_databuf(lastpbn) == -1){
				return -1;
			}
		}
		writef = true;
	} else
#endif
	{
		if ( Data->_write_databuf(pbn) == -1)
			return -1;
	}
	lastpbn = pbn;
	return 0;
}
bool TDataBufFile::flush( )
{
	if (fast_mode){
		if ( writef ){
			if ( Data->_write_databuf(lastpbn) == -1 )
				return false;	// writefはそのままで良いかな？
		}
		writef = false;
	}
	return true;
}
bool TDataBufFile::invalidate( )
{
#if USE_DICFASTMODE
	if (!flush())
		return false;
#endif
	lastpbn = BLK_ERROR;
	return true;
}
void TDataBufFile::changeOffset(unsigned prev_offset, unsigned new_offset)
{
	if (lastpbn != BLK_ERROR)
		lastpbn-=(new_offset-prev_offset)/BlockSize;
}
// setDataBuf() + write()
void TDataBufFile::set(t_pbn2 pbn, uint8_t *data, t_blknum blknum)
{
	setDataBuf( data, blknum, pbn );
	Data->write( pbn );	// Undo write
}
t_blknum TDataBufFile::readBlockNum(t_pbn2 pbn)
{
	if (lastpbn == pbn){		// 読みだし＆修正後、同じブロックの読みだしを行わないことを前提としている
		return *(t_blknum*)getDataBuf();
	} else {
		t_blknum _blknum;
		if (Data->file_read( pbn, &_blknum, sizeof(t_blknum) )==-1)
			return (t_blknum)BLK_ERROR;
		return _blknum;
	}
}
bool TDataBufFile::writeBlockNum(t_pbn2 pbn, t_blknum blknum)
{
	return Data->file_write(pbn, &blknum, sizeof(blknum))!=-1;
}
void TDataBufFile::shrink(int new_blknum)
{
	blknum = (blknum & FIELDTYPE) | new_blknum;
	*(t_blknum*)getDataBuf() = (t_blknum)blknum;
}

#if 0
	int real_len;
	if (len&0x80){
		if (len&0xC0==0xC0){
			if (len&0xE0==0xE0){
				real_len = len*8-128*(0+1+3+7);	// 384-632
			} else {
				real_len = len*4-128*(0+1+3);	// 256-380
			}
		} else {
			real_len = len*2-128*(0+1);	// 128-254
		}
	} else {
		real_len = len-128*(0);	// 0-127
	}
#endif

//
// TDataBuf
//
FieldFormat *TDataBuf::GetTopDataPtr( )
{
	return (FieldFormat*)(getDataBuf()+((HPdicData*)GetData())->GetTopLoc());
}
FieldFormat *TDataBuf::GetDataPtr( int loc )
{
	return (FieldFormat*)(getDataBuf()+ (loc ? loc : sizeof(t_blknum)));
}
#ifdef _DEBUG
// 先頭の２バイト(blknum_t)を除いたsum
unsigned TDataBuf::calcSum()
{
	unsigned sum = 0;
	int num = ((blknum&~FIELDTYPE) * BlockSize-2) / sizeof(unsigned);
	const unsigned *p = (const unsigned*)(getDataBuf()+2);
	for (int i=0;i<num;i++){
		sum += *p++;
	}
	return sum;
}
int TDataBuf::length()
{
	return (blknum&~FIELDTYPE) * BlockSize;
}
#endif

//
// EmptyList class
//
EmptyList::EmptyList( EmptyList *_next, long _offset, int _num )
{
	next = _next;
	offset = _offset;
	num = _num;
	resort = 1;
}

EmptyList::~EmptyList( )
{
	if ( next ){
		Clear();
	}
}

void EmptyList::Clear()
{
	if ( next ){
		next->Clear();
		delete next;
		next = NULL;
	}
}

void EmptyList::Register( t_pbn2 pbn, int blknum )
{
	if ( blknum == 0 ) return;
	DBDIC("Register : pbn=0x%X blknum=%d", pbn, blknum);
	resort = 1;
	next = new EmptyList( next, next ? (offset - pbn) : BLK_ERROR, blknum );
	offset = pbn;
	DD( DD_REGIST, pbn, blknum );
}

// pbn1個を解放する
// Sortがないから1つの場合はこちらのほうが高速
int EmptyList::Free( t_pbn2 pbn )
{
	if ( !next ){
		return -1;
	}

	DBDIC("Empty::Free : pbn=0x%X", pbn);
	
	long offs;
	EmptyList *el = Search( pbn, offs );

	if ( !el ){
		return -1;
	}

	DD( DD_FREE, pbn, 1 );

	EmptyList *eldel = el->next;	// これは絶対にNULLではない

	eldel->num--;
	if ( offs == 0 ){
		if ( eldel->num <= 0 ){
			el->offset += eldel->offset;
			el->next = eldel->next;
			eldel->next = NULL;	// 子を削除しないために
			delete eldel;
		} else {
			el->offset++;
			eldel->offset--;
		}
	}

//	Check( "Free" );
	return 0;
}

// pbnから連続するblknum個を解放する
int EmptyList::Free( t_pbn2 pbn, int blknum )
{
	if ( !next ){
		return -1;
	}

	if ( blknum == 1 ) return Free( pbn );

	DBDIC("Empty::Free : pbn=0x%X num=%d", pbn, blknum);
	
	Sort();

	long offs;
	EmptyList *el = Search( pbn, offs );

	if ( !el ){
		return -1;
	}

	EmptyList *eldel = el->next;	// これは絶対にNULLではない

	if ( offs == 0 ){
		eldel->num -= blknum;
		if ( eldel->num <= 0 ){
			el->offset += eldel->offset;
			el->next = eldel->next;
			eldel->next = NULL;	// 子を削除しないために
			delete eldel;
		} else {
			el->offset+=blknum;
			eldel->offset-=blknum;
		}
	} else {
		if ( eldel->num == offs + blknum ){
			// 途中から最後までの場合
			eldel->num -= blknum;
		} else {
			// 途中から途中まで
			int rblknum = eldel->num - offs - blknum;
			eldel->num = offs;
			Register( pbn + blknum, rblknum );
		}
	}

	DD( DD_FREE, pbn, blknum );
//	Check( "Free" );
	return 0;
}

// 自分の次のリストを削除する //
void EmptyList::_Free( )
{
	EmptyList *eldel = next;
	if ( !eldel ) return;
	offset += eldel->offset;
	next = eldel->next;
	eldel->next = NULL;	// 子を削除しないために
	delete eldel;
}

// １つ前のEmptyListを返すので注意！！
// offsは見つかったlistの中のoffset
EmptyList *EmptyList::Search( t_pbn2 pbn, long &offs )
{
	t_pbn2 cpbn = offset;
	EmptyList *cel = this;
	while ( cel->next ){
		if ( pbn >= cpbn && pbn < cpbn + cel->next->num ){
			offs = pbn - cpbn;
			return cel;
		}
		cpbn += cel->next->offset;
		cel = cel->next;
	}
	return NULL;
}

// 以下の機能は却下！！
// ppcel!=NULLの場合、連続reqnum以上のブロックを探し、
// 見つかった場合は、１つ前のEmptyList pointerと
// 見つかった物理ブロック番号を返す
// この場合、Sortが完全ではないので注意すること！！
// もし見つからなかった場合は最終ブロックの１つ前の
// EmptyList pointerと最終ブロック番号を返す
// (最終ブロックが連続していた場合はその先頭番号)
// 上記に該当しない場合はBLK_ERRORを返す
int EmptyList::Sort( )
{
	if ( !next ) return 0;
	EmptyList *cel;
	EmptyList *pcel;

	if ( resort ){
		pcel = this;
		cel = pcel->next;
		while ( cel->next ){
			if ( cel->offset < 0 ){
				// nextとcelの順番を入れ替え
				EmptyList *nextcel = cel->next;
	
				long coffs = cel->offset;
				pcel->offset += coffs;
				cel->offset += nextcel->offset;
				nextcel->offset = -coffs;
	
				pcel->next = nextcel;
				cel->next = nextcel->next;
				nextcel->next = cel;
	
				cel = nextcel;
	
				if ( pcel->offset < 0 ){
					// 最初からやり直し
//					Concat();
					pcel = this;
					cel = pcel->next;
					// もしここで、this->offset < 0だとバグあり！！
					if ( this->offset < 0 )
						return -1;
				}
			} else {
				pcel = cel;
				cel = cel->next;
			}
		}

		// 二度実行すれば確実だが・・・

		Concat();
		resort = 0;
	}
	return 0;
}

// ブロック連結のみ(ReadEmptyBlocks後に呼ぶと高速になる)
void EmptyList::Concat( )
{
	if ( !next ) return;
	EmptyList *cel;

	cel = next;
	EmptyList *pcel = this;
	while ( cel->next ){
		// ﾌﾞﾛｯｸの連結
		if ( cel->offset == cel->num ){
			EmptyList *delel = cel->next;
			cel->next = delel->next;
			cel->offset += delel->offset;
			cel->num += delel->num;
			delel->next = NULL;
			delete delel;
			continue;
		}
		if ( cel->offset == -cel->next->num ){
			EmptyList *nextcel = cel->next;
			pcel->next = nextcel;
			pcel->offset += cel->offset;
			nextcel->num += cel->num;
			cel->next = NULL;
			delete cel;
			cel = nextcel;
			continue;
		}
		pcel = cel;
		cel = cel->next;
	}
}

// 連続したnum個のブロックを解放する
// 成功した場合は、その先頭の物理ブロック番号
// seqnumが!=NULLの場合、blknumが確保されないとき、
// 物理的に一番後ろの空きﾌﾞﾛｯｸの連続ﾌﾞﾛｯｸ数をNewBlockし、
// その個数をseqnumに、物理ブロック番号をreturnする。
// 従って、return値の物理ブロック番号がblknum個であるかどうかを
// seqnumとblknumを比較してチェックする必要がある
//
// startpbnがNULLでない場合、startpbn以降からNewBlockをする for optimize
t_pbn2 EmptyList::NewBlock( int blknum, int *seqnum, t_pbn2 *startpbn )
{
	if ( !HasBlock() ) return BLK_ERROR;
	t_pbn2 newpbn;
	if ( blknum == 1 && !startpbn ){
		//TODO: blknumが１個のlistを探すようにしたほうが良い！！
		newpbn = offset;
		EmptyList *eldel = next;
		eldel->num--;
		if ( eldel->num <= 0 ){
			offset += next->offset;
			next = next->next;
			eldel->next = NULL;	// 子を削除しないために
			delete eldel;
		} else {
			offset++;
			eldel->offset--;
		}
		if ( seqnum ) *seqnum = 1;
//		Check("NewBlock-1");
		DD( DD_FREE, newpbn, 1 );
		DBDIC("NewBlock : pbn=0x%X num=%d", newpbn, blknum);
		return newpbn;
	} else {
		Sort( );
		t_pbn2 newpbn = offset;
		EmptyList *cel = this;
		EmptyList *nextcel = next;
		while ( nextcel ){
			if ( nextcel->num >= blknum ){
				if ( !startpbn || newpbn >= *startpbn )
					break;
			}
			if ( !nextcel->next )
				break;
			newpbn += nextcel->offset;
			cel = nextcel;
			nextcel = cel->next;
		}
		if ( nextcel->num >= blknum || seqnum ){
			if ( startpbn && newpbn < *startpbn )
				return BLK_ERROR;
			// free処理
			int _seqnum = min( nextcel->num, (int)blknum );
			if ( seqnum ){
				*seqnum = _seqnum;
			}
			nextcel->num -= blknum;
			if ( nextcel->num <= 0 ){
				nextcel->num = 0;
				cel->offset += nextcel->offset;
				cel->next = nextcel->next;
				nextcel->next = NULL;	// 子を削除しないために
				delete nextcel;
			} else {
				cel->offset+=blknum;
				nextcel->offset-=blknum;
			}
			DD( DD_FREE, newpbn, blknum );
			DBDIC("NewBlock : pbn=0x%X num=%d", newpbn, _seqnum);
			return newpbn;
		}
		return BLK_ERROR;
	}
}

// pbnから物理的に連続しているブロック数を得る
int EmptyList::GetSeq( t_pbn2 pbn )
{
	Sort();

	long offs;
	EmptyList *p = Search( pbn, offs );
	if ( !p ) return 0;
	return p->next->num - offs;
}

// 現在のリストの個数と全空きブロック数を計算
void EmptyList::GetInfo( int &numlist, int &numblock )
{
	Sort();

	numlist = 0;
	numblock = 0;
	EmptyList *cel = next;
	while ( cel ){
		numlist++;
		numblock += cel->num;
		cel = cel->next;
	}
}

void FINDWORD::clear( )
{
	lbn = -1;
	pbn = 0;
	lp = 0;
	word1.clear();
	fword[0] = '\0';
}

//	word1とword2で先頭から一致する文字数を返す
int get_complen( const _kchar *word1, const _kchar *word2)
{
#if USE_INLINE
	PUSH_SIDI
	asm {
#ifdef FARPTR
		push	ds
		lds	si,word1
		les di,word2
#else
		mov	si,word1
		mov	ax,ds			// 4.00のバグ？esがセットされていなかった
		mov	es,ax
		mov	di,word2
#endif
		xor	cx,cx
	}
jmp1:
	asm {
		cmp	byte ptr [si],0
		jz	exit1
		cmpsb
		jnz	exit1
		inc	cx
		jmp	short jmp1
	}
exit1:
	POP_SIDI
#ifdef	FARPTR
	asm	pop	ds
#endif
	return _CX;
#else
	int i;

	for (i=0;;) {
		_kchar c = *word1;
		if (c != *word2 || c == '\0'){
#if DIC_VERSION>DIC_VERSION51
			return max(i,LWORD_COMPLEN);
#else
			return i;
#endif
		}
		i++;
		word1++;
		word2++;
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
// PdicData class
//////////////////////////////////////////////////////////////////////////
PdicData::PdicData( int &error, FileBuf &_file )
	:file(_file)
	,PdicObject( error )
	,databuf(this)
{
	emptylist = NULL;
}

PdicData::~PdicData()
{
	if ( emptylist ) delete emptylist;
}

int PdicData::Open( const HEADER FAR *header, IndexData *inxdat )
{
	version = header->version;
	offset = header->header_size + header->extheader + header->index_block * header->block_size;
	dictype = header->dictype;
	blocksize = header->block_size;
	emptyblock = header->empty_block2;
	nblock = header->nblock2;
	DD( DD_SETNBLOCK, nblock );
	nword = header->nword;
	canGrow = true;
	if (!databuf.Open(blocksize)){
		error = 2;		//メモリ不足
		return -1;
	}
	return 0;
}

void PdicData::Close( )
{
	if (file.isOpen()){
		CloseEmptyList();
		Flush();
	}
	databuf.Close();
	zeroBuffer.reset();
}

// Should delete the returned object.
TDataBuf *PdicData::CreateDataBuf()
{
	TDataBuf *databuf = new TDataBuf(this);
	if (!databuf)
		return NULL;
	databuf->Open(blocksize);
	return databuf;
}

#if MEMMAPDIC
int PdicData::file_fill(uint8_t data, int len)
{
	return databuf.fill(data, len);
}
#endif	// MEMMAPDIC
#if !MEMMAPDIC
int PdicData::file_seek(t_pbn2 bn, int off)
{
	if (file.seek(get_data_loc(bn)+off) == -1){
		return -1;
	}
	return 0;
}
int PdicData::file_read(t_pbn2 pbn, void *data, int len, int off)
{
	TAutoLock lock(Mutex);
	if (seek(pbn,off)==-1)
		return -1;
	return file.read(data, len);
}
// return value:
//	the number of written bytes
int PdicData::file_write(t_pbn2 pbn, const void *data, int len, int off)
{
	if (seek(pbn,off)==-1)
		return -1;
	return file.write(data, len);
}
int PdicData::file_fill(int len)
{
	if (!zeroBuffer.reserve( len )){
		error = DICERR_MEMORY;
		return -1;
	}

	return file.write(zeroBuffer.data(), len);
}
#endif	// !MEMMAPDIC
//TODO: 呼び出し側も含めて二重memory allocationになっているので
// 呼び出し側がallocateを呼び出すように修正する
bool PdicData::SetDataBuf( t_pbn2 pbn, const uint8_t *p, int _blknum)
{
	uint8_t *newbuf = databuf.alloc(pbn, _blknum);
	if (!newbuf)
		return false;
	memcpy(newbuf, p, _blknum*blocksize);
	return true;
}
int PdicData::ClearBlock( t_pbn2 pbn, t_blknum blknum )
{
	seek(pbn);
	return file_fill(blocksize*blknum);
}

long PdicData::Chsize( )
{
#if MEMMAPDIC
	databuf.Close();
#endif
	long ret = file.chsize(get_data_loc(nblock));
#if MEMMAPDIC
	databuf.Reopen();
#endif
	return ret;
}

bool PdicData::file_copy(t_pbn2 src, t_pbn2 dst, int blknum)
{
	DBDIC("file_copy : src=0x%X dst=0x%X num=%d", src, dst, blknum);
#if MEMMAPDIC
	memcpy(databuf.getDataPtr(dst), databuf.getDataPtr(src), blknum*blocksize);
	return true;
#else
	char *buf = new char[ blocksize ];
	if (!buf){
		error = DICERR_MEMORY;
		return false;
	}
	for ( int i=0;i<blknum;i++ ){
		if (file_read( src+i, buf, blocksize ) == -1
			|| file_write( dst+i, buf, blocksize ) == -1 ){
			delete[] buf;
			error = DICERR_READWRITE;
			return false;
		}
	}
	delete[] buf;
	return true;
#endif
}

int PdicData::UndoUpdate( DivList *dl, int divnum )
{
	for ( int i=1;i<divnum;i++ ){
		HCALLDATA eraseBlock( dl[i].newpbn );
		delete dl[i].word;
	}
	if ( divnum >= 1 ){
		Flush( );
		databuf.set(dl[0].oldpbn, (uint8_t*)dl[0].word, *(t_blknum*)dl[0].word);	// Undo write
	}
	return 0;
}

// 一番最初のフィールドへコピーする
int PdicData::CopyToTopField( FieldFormat *dp, FieldFormat *src, const _kchar *wbuf )
{
	dp->fieldlen = (tfield)(src->fieldlen + src->complen);
	dp->complen = 0;
	memcpy( dp->word, wbuf, src->complen );	// 増加分
	memcpy( dp->word + src->complen, src->word, src->fieldlen );		// 全体
	return 0;

#if 0
	FieldFormat *newtop = (FieldFormat*)databuf;
	newtop->fieldlen = top->fieldlen + top->complen;
	newtop->complen = 0;
	memcpy( newtop->word, topword, top->complen );	// 増加分
	int l = FP_DIFF( tail, top );
	memcpy( newtop->word + top->complen, top->word, l );		// 全体
	*((FieldFormat*)(databuf + top->complen + l ))->tfield = 0;	// 終端
	if ( write( pbn ) )
		return -1;
	return 0;
#endif
}

#if !defined(ND3ONLY) && !NOUPDATEWORD
// 分割専用のアップデート/レコード
// 復活用のdatabufはdivlist[0].wordに入っている(ただし、divnum >= 1 のとき）
// 注意:
// divlist[0] は、特殊
//		divlist[0].divtype	= 2 : 何もしない  1 : 元々のブロックが無くなった -> 呼出側の処理が全てうまく行ったら、oldpbnをeraseBlock()する必要あり！
//		divlist[0].oldpbn	= fw.pbn
//		divlist[0].newpbn	= fw.pbn
//		divlist[0].word		= databufが入っている！！
//                              この関数が正常終了した際は、UndoUpdate()を呼ぶか、呼出側でdeleteしなければならない
//							  divlist[i].wordすべて同様
//
//	アップデートの場合、引数 word は意味をなさない
//
//	分割の必要の無い場合に呼ぶと正常に動作するかどうかわからない
//
// override
int PdicData::updateWord( const tchar *word, const uint8_t *japa, uint japalen, FINDWORD &fw, DivList *divlist, int &divnum, int recmode )
{
	divnum = 0;
	if ( Flush( ) )
		return -1;
	if ( read( fw.pbn ) )
		return -1;

	divnum = 1;
	FieldFormat *ffp, *orgp;
	char *_databuf = divlist[0].word = databuf;
	databuf = NULL;
	lastpbn = -1;
	blknum = 0;

	divlist[0].divtype = 2;
	divlist[0].oldpbn = fw.pbn;
	divlist[0].newpbn = fw.pbn;

	orgp = ffp = (FieldFormat*)_databuf;
	char wbuf[ LWORD + 1 ];
	strcpy( wbuf, ffp->word );
//	int maxblocksize = ( canGrow ? GetMaxBlockSize( ) : blocksize );
	int divsize = get_div_size( );
	t_pbn cur_pbn = BLK_ERROR;
	int len = 0;				// ストア先のストア済み長さ
	FieldFormat *dp = NULL;
	FieldFormat *divp = NULL;	// 一番最初の分割点
	char *nwbuf = NULL;		// 登録時用の追加点(fw.lp)の次の単語（非圧縮）

	tcomp complen;		// 登録単語の圧縮長
	int recflag = 0;				// 登録単語を登録した
	while ( ffp->fieldlen || !recflag ){	// || の右は新規登録の場合
		FieldFormat *np = NextField( ffp );
		int addlen;
		int modf = 0;

		complen = ffp->complen;
		if ( fw.lp == FP_DIFF( ffp, orgp ) && !recflag ){
			if ( recmode ){
				// 登録単語の場合
				modf = 2;
				complen = (tcomp)equ_word( wbuf, word );
				addlen = sizeof(FieldFormat) + strlen( word ) + japalen - complen;
				nwbuf = new char[ strlen( ffp->word ) + ffp->complen + 1 ];
				memcpy( nwbuf, wbuf, ffp->complen );
				strcpy( nwbuf + ffp->complen, ffp->word );
//				nextcomplen = (tcomp)equ_word( word, nwbuf );
				strcpy( wbuf, word );
				np = ffp;
			} else {
				// 修正単語の場合
				const char *p = ffp->word;
				while ( *p++ );
				addlen = japalen + FP_DIFF( p, ffp );		// 変更後の長さ
				modf = 1;
			}
			recflag = 1;
		} else {
			addlen = FP_DIFF( np, ffp );
		}
		if ( modf != 2 ){
			if ( nwbuf ){
				complen = (tcomp)equ_word( wbuf, nwbuf );
				strcpy( wbuf, nwbuf );
				delete nwbuf;
				nwbuf = NULL;
			} else {
				strcpy( wbuf + complen, ffp->word );
			}
		}

		if ( len + addlen + sizeof(tfield) > divsize || !dp /* || len + addlen + sizeof(tfield) > pbnGetNum(cur_pbn) * blocksize */ ){
			// 分割 or Grow が必要
			if ( modf == 2 ){
				// 登録単語の場合、圧縮をなしにする
				addlen += complen;
				complen = 0;
			}
			int blknum = ( addlen + sizeof(tfield) + blocksize - 1 ) / blocksize;
			if ( blknum > 1 && !canGrow ){
				UndoUpdate( divlist, divnum );
				if ( nwbuf )
					delete nwbuf;
				error = 29;
				return -1;
			}
			if ( dp ){
				dp->fieldlen = 0;
				if ( write( cur_pbn ) ){
					UndoUpdate( divlist, divnum );
					if ( nwbuf )
						delete nwbuf;
					return -1;
				}
//				dp = NULL;
			}
			t_pbn newpbn = (t_pbn)newBlock( blknum );
			if ( newpbn == (t_pbn)BLK_ERROR ){
				UndoUpdate( divlist, divnum );
				if ( nwbuf )
					delete nwbuf;
				return -1;
			}
			divlist[divnum].divtype = 0;
			divlist[divnum].oldpbn = BLK_ERROR;
			divlist[divnum].newpbn = newpbn;
			divlist[divnum].word = strdup( modf == 2 ? word : wbuf );
			divnum++;

			// 先頭単語のセット
			dp = (FieldFormat*)databuf;
			if ( modf ){
				dp->fieldlen = (tfield)(addlen - L_FieldHeader + complen);	// 1995.9.26 complenを足した
				dp->complen = 0;
				strcpy( dp->word, recmode ? word : wbuf );
				char *p = dp->word;
				while ( *p++ );
				memcpy( p, japa, japalen );
			} else {
				CopyToTopField( dp, ffp, wbuf );
			}
			if ( !divp )	// 終了時のため
				divp = ffp;
			dp = NextField( dp );
			cur_pbn = newpbn;
			if ( FP_DIFF( dp, databuf ) > divsize ){	// １個のデータでdivsizeを超えていた場合
				dp->fieldlen = 0;		// 終端
				if ( write( cur_pbn ) ){
					UndoUpdate( divlist, divnum );
					if ( nwbuf )
						delete nwbuf;
					return -1;
				}
				cur_pbn = BLK_ERROR;
				dp = NULL;
				addlen = 0;
			}
			len = addlen;
			ffp = np;
		} else {
			if ( modf ){
				dp->fieldlen = (tfield)(addlen - L_FieldHeader);
				dp->complen = complen;
				strcpy( dp->word, recmode ? word+complen : wbuf+complen );
				char *p = dp->word;
				while ( *p++ );
				memcpy( p, japa, japalen );
			} else {
				memcpy( dp, ffp, ffp->fieldlen + L_FieldHeader );
			}
			dp = NextField( dp );
			len += addlen;
			ffp = np;
		}
	}

	if ( nwbuf )
		delete nwbuf;

	if ( dp ){
		dp->fieldlen = 0;	// 終端
		write( cur_pbn );
	}

	if ( divp ){
		if ( divp == orgp ){
			// 元々のブロックが無くなった
			divlist[0].divtype = 1;	// 呼出側が全てうまく行ったらeraseBlock()をしなければならない！
		} else {
#if 0		// ここには来ないはず
			Flush( );
			if ( databuf )
				delete databuf;
			databuf = (char*)orgp;
			tfield l = divp->fieldlen;	// 一時保存
			divp->fieldlen = 0;	// 終端
			write( divlist[0].newpbn );
			divp->fieldlen = l;
			Flush( );
			databuf = NULL;
			lastpbn = -1;
			blknum = 0;
#endif
		}
	}

	if ( recmode )
		nword++;
	return 0;
}
#endif	// !ND3ONLY && !NOUPDATEWORD

// pbnを最後のブロックへ移動する
// この関数はdatabufを使わない！！
// 返り値は移動先物理ﾌﾞﾛｯｸ番号
// pbnは未使用ブロックとして扱われる(空きブロックリンクに登録されない)
// もしpbnが連結ブロックであった場合、pbn+blknum以降は空きブロックリンクに登録される
// blknumは入力が移動できたら空きブロックに登録したくないブロック数、出力は移動したブロックの数
t_pbn2 PdicData::MoveBlock( t_pbn2 pbn, t_blknum &blknum )
{
	if (!Flush()){
		return BLK_ERROR;
	}

	t_blknum _blknum = blknum;
	t_pbn2 pblock;

	if ( file_read( pbn, &blknum, sizeof(t_blknum) ) == -1 ){
		error = DICERR_READWRITE;
		return BLK_ERROR;
	}

	pblock = pbn;
	blknum &= ~FIELDTYPE;

	if ( !emptylist ){
		OpenEmptyList( );
	}

	int new_from_tail = 0;
	t_pbn2 newpbn = emptylist->NewBlock( blknum );
	if ( newpbn == BLK_ERROR ){
		newpbn = nblock;
		new_from_tail = 1;
		if (file_seek_alloc(newpbn+blknum)==-1){
			error = DICERR_DATA_DISK;
			return BLK_ERROR;
		}
	}

	// 移動開始！
	if (!file_copy(pblock, newpbn, blknum)){
		return BLK_ERROR;
	}
	if (new_from_tail){
		nblock += blknum;
		DD( DD_SETNBLOCK, nblock );
	}
	// 空きブロックリンクに登録
	if ( blknum > _blknum ){
		emptylist->Register( pblock+_blknum, (t_blknum)(blknum-_blknum) );
	}
	return newpbn;
}

void PdicData::ChangeOffset( int offs )
{
	unsigned prev_offset = offset;
	offset += offs * blocksize;
	nblock -= offs;
	DD( DD_SETNBLOCK, nblock );
	databuf.changeOffset(prev_offset, offset);
	if ( emptylist ){
		if ( emptylist->next ){
			emptylist->offset -= offs;
			emptyblock = emptylist->offset;
		} else {
			emptyblock = BLK_ERROR;
		}
	} else {
		if ( emptyblock != -1 ) emptyblock -= offs;
	}
}

int PdicData::ReadEmptyBlocks( )
{
//	if ( emptyblock == BLK_ERROR ) return;
	t_pbn2 cpbn = emptyblock;
	while ( cpbn != BLK_ERROR ){
		EmptyBlockFormat2 ebf;
		if ( file_read( cpbn, &ebf, sizeof(ebf) ) < sizeof(ebf) ){
			break;
		}
		if ( ebf.null ) break;
		if (cpbn==ebf.nbn){
			DBW("empty link list - linkage error");
			error = DICERR_CORRUPTED;
			return -1;
		}
		emptylist->Register( cpbn, 1 );
		cpbn = ebf.nbn;
	}
	emptylist->Concat();
	return 0;
}

int PdicData::WriteEmptyBlocks( )
{
	if ( !emptylist ){
		// 未オープンの場合はそのまま
		return 0;
	}
	if ( !emptylist->next ){
		// 空きブロックはなし
		emptyblock = BLK_ERROR;
		return 0;
	}
	emptylist->Sort();
	t_pbn2 cpbn = emptyblock = emptylist->offset;
	EmptyList *cel = emptylist->next;
	EmptyBlockFormat2 ebf;
	ebf.null = 0;
	while ( 1 ){
		for ( int i=0;i<cel->num-1;i++ ){
			ebf.nbn = cpbn+i+1;
			if (file_write(cpbn+i, &ebf, sizeof(ebf)) < sizeof(ebf)){
				error = 4;
				return -1;
			}
		}
		ebf.nbn = cel->next ? cel->offset + cpbn : BLK_ERROR;
		if ( file_write( cpbn + cel->num - 1, &ebf, sizeof(ebf) ) < sizeof(ebf) ){
			error = 4;
			return -1;
		}
		if ( !cel->next ) break;
		cpbn += cel->offset;
		cel = cel->next;
	}
	return 0;
}

void PdicData::OpenEmptyList( )
{
	if ( emptylist )
		return;
	emptylist = new EmptyList( NULL, emptyblock, 0 );
	ReadEmptyBlocks();
}

int PdicData::CloseEmptyList( )
{
	int r = 0;
	if ( emptylist ){
		r = WriteEmptyBlocks( );
//		if ( r == 0 )
		{
			delete emptylist;
			emptylist = NULL;
		}
	}
	return r;
}

t_pbn2 PdicData::GetEmptyBlock( )
{
	if ( emptylist ){
		emptylist->Sort();
		return emptylist->HasBlock() ? emptylist->offset : BLK_ERROR;
	} else {
		return emptyblock;
	}
}

void PdicData::SetEmptyBlock( t_pbn2 _emptyblock )
{
	if ( emptylist ){
		if ( _emptyblock == BLK_ERROR ){
			emptylist->Clear();
		} else {
			emptylist->offset= _emptyblock;
		}
	}
	emptyblock = _emptyblock;
}

