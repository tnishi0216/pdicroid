#ifndef __dicdata_h
#define	__dicdata_h

#define	FIELDTYPE		0x8000

class TDataBufBase {
protected:
	class PdicData *Data;
	unsigned BlockSize;
	uint8_t *DataBuf;
public:
	TDataBufBase(class PdicData *data);
	virtual ~TDataBufBase();
	void SetData( PdicData *data )
		{ Data = data; }
	inline PdicData *GetData()
		{ return Data; }
};

class TDataBufMemMap : public TDataBufBase {
typedef TDataBufBase super;
protected:
	uint8_t *LastPtr;	// for seek/read operation
	class TMmfMap *MM;
	uint8_t *BaseAddr;
public:
	TDataBufMemMap(PdicData *data);
	virtual ~TDataBufMemMap();
	bool Open(unsigned blocksize);
	bool Reopen();
	void Close();
	bool isOpened() const
		{ return MM!=NULL; }
	// databuf //
	uint8_t *alloc(t_pbn2 pbn, t_blknum _blknum);
	void setDataPbn(t_pbn2 pbn);
	uint8_t *getDataBuf() const
		{ return DataBuf; }
	uint8_t *getDataPtr(t_pbn2 pbn)
		{ return BaseAddr + pbn*BlockSize; }

	// mode //
	int getFastMode() const
		{ return true; }
	void setFastMode(int fast_mode){}

	// file operation //
	int seek(t_pbn2 pbn, int offs=0);
	int seek_alloc(t_pbn2 pbn, int offs=0);
	int seekcur(long offs)
		{ LastPtr += offs; return 0; }
	int read(t_pbn2 pbn, void *data, int len, int off);
	int read(void *data, int len);
//	int read_direct(t_pbn2 pbn, void *data, int len, int off);
	int write(t_pbn2 pbn);
	int write(t_pbn2 pbn, const void *data, int len, int off);
	int write(const void *data, int len);
	int fill(uint8_t data, int len);
	bool flush( ){ return true; }
	bool invalidate( ){ return true; }

	// PBN operation //
	void changeOffset(unsigned prev_offset, unsigned new_offset);
	inline int isFastMode() const
		{ return true; }
	inline void setLastPbn(t_pbn2 pbn)
		{ }

	// block number operation //
	t_blknum getBlockNum() const
	{
		if (!DataBuf)
			return 0;
		return (t_blknum)(*(t_blknum*)DataBuf & ~FIELDTYPE);
	}
	bool isField2()
	{
		if (!DataBuf)
			return false;
		return isField2(*(t_blknum*)DataBuf);
	}
	bool isField2(t_blknum blknum)
	{
		return blknum&FIELDTYPE ? true : false;
	}

	// misc //
	bool writeBlockNum(t_pbn2 pbn, t_blknum blknum);
	t_blknum readBlockNum(t_pbn2 pbn)
		{ return *(t_blknum*)getDataPtr(pbn); }
	void set(t_pbn2 pbn, uint8_t *data, t_blknum blknum);
	void resetWrite(){}
	void setDataBuf(uint8_t *new_databuf, t_blknum blknum, t_pbn2 pbn);
	void grow(t_pbn2 oldpbn, t_pbn2 newpbn, int old_blknum, int new_blknum);
	void shrink(int new_blknum);
};

class TDataBufFile : public TDataBufBase {
typedef TDataBufBase super;
protected:
#if USE_DICFASTMODE
	int fast_mode;
	bool writef;
#endif
	t_pbn2 lastpbn;
	int blknum;	// 現在のdatabufのメモリ確保されているブロック数
	int allocnum;	// memory allocation block number
public:
	TDataBufFile(PdicData *data);
	virtual ~TDataBufFile();
	bool Open(unsigned blocksize);
	void Close();

	uint8_t *alloc(t_pbn2 pbn, t_blknum _blknum);
	uint8_t *alloc_opt(t_pbn2 pbn, t_blknum _blknum);
	inline uint8_t *getDataBuf()
		{ return DataBuf; }
	void setDataBuf(uint8_t *ptr, t_blknum blknum, t_pbn2 pbn);
protected:
	void _setDataBuf(t_blknum _blknum, t_pbn2 bpn);
public:
	inline int getFastMode() const
		{ return fast_mode; }
	void setFastMode(int fast_mode);
	int write(t_pbn2 pbn);
	bool flush( );
	bool invalidate( );
	void changeOffset(unsigned prev_offset, unsigned new_offset);
	inline bool isLastPbn(t_pbn2 pbn)
		{ return lastpbn == pbn; }
	inline int isFastMode() const
		{ return fast_mode; }
	inline void setLastPbn(t_pbn2 pbn)
		{ lastpbn = pbn; }
	inline int getLastPbn() const
		{ return lastpbn; }
	inline void resetWrite()
	{
#if USE_DICFASTMODE
		writef = 0;
#endif
	}
	void set(t_pbn2 pbn, uint8_t *data, t_blknum blknum);

	t_blknum readBlockNum(t_pbn2 pbn);
	bool writeBlockNum(t_pbn2 pbn, t_blknum blknum);
	inline t_blknum getBlockNum() const
		{ return (t_blknum)(blknum & ~FIELDTYPE); }	// 直接databufを参照した方が安全だが・・・
	inline t_blknum getBlockNumType() const
		{ return (t_blknum)blknum; }
	inline bool isField2()
		{ return blknum & FIELDTYPE ? true : false; }
	void shrink(int new_blknum);
};

#if MEMMAPDIC
#define	TDataBuf_BaseClass	TDataBufMemMap
#else
#define	TDataBuf_BaseClass	TDataBufFile
#endif

class TDataBuf : public TDataBuf_BaseClass {
typedef TDataBuf_BaseClass super;
public:
	TDataBuf(PdicData *data)
		:super(data)
	{}
	FieldFormat *GetTopDataPtr( );
	FieldFormat *GetDataPtr( int loc );
	unsigned GetBlockSize() const
		{ return BlockSize*getBlockNum(); }
	// defined in HDicData.cpp
	int prevWord(uint &loc ,_kchar *wbuf);
	const uint8_t *srchTail( _kchar *wbuf );
	int GetFieldHeaderSize()
		{ return isField2() ? L_FieldHeader2 : L_FieldHeader; }
	int isEmpty();
	int GetNumWord( );
	void GetJustWord( _kchar *wbuf, int loc );
	t_pbn2 GetNextEmptyBlock( ){ return ((EmptyBlockFormat2*)getDataBuf())->nbn; }
	int CheckField2( t_pbn2 pbn );
	unsigned calcSum();	// for debug
	int length();
};	// TDataBuf

class TZeroBuffer {
protected:
	unsigned size;
	char *buffer;
public:
	TZeroBuffer(unsigned _size=0)
	{
		size = _size;
		if (size>0){
			buffer = (char*)malloc(size);
			memset(buffer, 0, size);
		} else {
			buffer = NULL;
		}
	}
	~TZeroBuffer()
	{
		if (buffer) free(buffer);
	}
	void reset()
	{
		if (buffer){
			free(buffer);
			buffer = NULL;
		}
		size = 0;
	}
	bool reserve(unsigned _size)
	{
		if (_size <= size) return true;
		if (buffer){
			char *temp = (char*)realloc( buffer, _size );
			if (!temp) return false;
			buffer = temp;
			memset(buffer+size, 0, _size - size);
		} else {
			buffer = (char*)malloc( _size );
			if (!buffer) return false;
			memset(buffer, 0, _size);
		}
		size = _size;
		return true;
	}
	const char *data(){ return buffer; }
};

///////////////////////
///	PdicDataクラス	///
///////////////////////
class PdicData : public PdicObject {
	friend TDataBufBase;
	friend TDataBufMemMap;
	friend TDataBufFile;

//	FieldFormat使用時注意事項
//		絶対に、FieldFormat *p で、p += 1などはご法度（構造体の大きさの分だけインクリメントされてしまう）
//		絶対に、FieldFormat *p = (FieldFormat*)databuf+a;はいけない。(FieldFormat*)(databuf+a)が正しい

protected:
	TMutex Mutex;
	TDataBuf databuf;
public:
	TDataBuf *GetDataBuf()
		{ return &databuf; }

protected:
	unsigned offset;			//データブロックの先頭
	unsigned blocksize;			//データブロックサイズ（固定長）
	int nword;					//登録単語数
	int emptyblock;			//空きブロックの先頭(最新の先頭空きブロック番号はemptylist変数に入っている)
	int nblock;				//空きブロックを含めた使用ブロック数
	FileBuf &file;

	bool canGrow;				// ブロックサイズを大きくすることができる？
	uint8_t dictype;
	short version;

	TZeroBuffer zeroBuffer;

protected:
	virtual int _write_databuf(t_pbn2 pbn) = 0;
public:
	PdicData( int &error, FileBuf &_file);
	virtual ~PdicData();

	virtual int Open( const HEADER *header, class IndexData *inxdat );
	void Close( );

	TDataBuf *CreateDataBuf();	// これを呼び出す場合はdatabuf->SetData()の呼び出しの検討も
	
	int write(t_pbn2 pbn)
		{ return databuf.write(pbn); }
#if USE_DICFASTMODE
	bool Flush( )
		{ return databuf.flush(); }
#else
	int Flush( ){ return 0; }
#endif
	int ClearBlock( t_pbn2 pbn, t_blknum blknum );

	int get_data_loc(t_pbn2 pbn)				//派生クラスはこのメソッドを使ってファイルアクセスはしないように
		{ return (int)pbn * blocksize + offset; }
	int seek(t_pbn2 n, int off=0)		//TODO: to be deleted
		{ return file_seek(n, off); }
	int file_seek(t_pbn2 pbn, int off=0)
#if MEMMAPDIC
		{ return databuf.seek(pbn, off); }
#else
		;
#endif
	int file_seek_alloc(t_pbn2 n)
#if MEMMAPDIC
		{ return databuf.seek_alloc(n); }
#else
//		{ return seek(n); }
		{ return file.chsize(get_data_loc(n)); }
#endif
	int file_seekcur(long offs)
#if MEMMAPDIC
		{ return databuf.seekcur(offs); }
#else
		{ return file.seekcur(offs); }
#endif
	int file_read(t_pbn2 pbn, void *data, int len, int off=0)
#if MEMMAPDIC
		{ return databuf.read(pbn, data, len, off); }
#else
		;
#endif
	int file_read_direct(t_pbn2 pbn, void *data, int len, int off=0)
		{ return file_read(pbn, data, len, off); }
	int file_read(void *data, int len)
#if MEMMAPDIC
		{ return databuf.read(data, len); }
#else
		{ return file.read(data, len); }
#endif
	int file_write(t_pbn2 pbn, const void *data, int len, int off=0)
#if MEMMAPDIC
		{ return databuf.write(pbn, data, len, off); }
#else
		;
#endif
	int file_write(const void *data, int len)	// file write without seeking
#if MEMMAPDIC
		{ return databuf.write(data, len); }
#else
		{ return file.write(data, len); }
#endif
	int file_fill(int len);

	long Chsize( );	// 現在のnblockに合わせてファイルサイズを変更する
	bool file_copy(t_pbn2 src, t_pbn2 dst, int blknum);

	void DecNWord() { nword--; }

	unsigned GetOffset() const
		{ return offset; }
	void ChangeOffset( int offs );

	//情報取得
	const uint8_t *getDataBuf(void)
		{ return databuf.getDataBuf(); }		//(_kchar*)でキャスティングしているところがあるので後で直す！
	uint8_t *_getDataBuf( )								//databuf can be change
		{ databuf.flush(); return databuf.getDataBuf(); }
	void InvalidDataBuf();

	// databufを指定のバッファに置き換える
	bool SetDataBuf( t_pbn2 pbn, const uint8_t *p, int _blknum);


	//後方検索（dicsub1.cpp)

	//削除
	t_pbn2 MoveBlock( t_pbn2 pbn, t_blknum &blknum );

	//カレントブロックに対する処理（すでにreadされている事！）
	int UndoAd( DivList *dl, int divnum );
	int CopyToTopField( FieldFormat *dp, FieldFormat *src, const _kchar *wbuf );
	int UndoUpdate( DivList *dl, int divnum );
	int divBlock( t_pbn pbn, t_pbn newpbn, const _kchar *wbuf, int divp );
	int GetDivPtr( t_pbn pbn, tnstr &wbuf, FINDWORD &fw, int &divp, int &newblknum, int reqlen, int &fwlpback, int modf );

#if USE_DICFASTMODE
#if MEMMAPDIC
	void SetFastMode(int _fast_mode){}
	int IsFastMode()
		{ return false; }
#else
	void SetFastMode(int _fast_mode)
		{ databuf.setFastMode(_fast_mode); }
	int IsFastMode()
		{ return databuf.getFastMode(); }
#endif
#endif
	int GetNumWord( )		{return nword;}
	void SetNumWord( int n )
		{ nword = n; }
	int GetNBlock( )		{return nblock;}
	void SetNBlock( t_pbn2 n ) { nblock = n; }
	long GetDataSize()
		{ return nblock * blocksize; }

	int ReadBlockNum(t_pbn2 pbn) { return databuf.readBlockNum(pbn) & ~FIELDTYPE; }
	int GetUnitBlockSize()
		{ return blocksize; }
	t_blknum CalcBlockNum( int datalen )
		{ return (t_blknum)((datalen+blocksize-1) / blocksize);}

	virtual int CheckField2( t_pbn2 ) { return 0;}

protected:
//		int RegistEmptyBlock( t_pbn2 pbn, t_blknum blknum );	// 空きブロックリンクに登録
	// 空きブロック管理
public:
	EmptyList *emptylist;
	void OpenEmptyList();
	int CloseEmptyList();
	int WriteEmptyBlocks();
	int ReadEmptyBlocks();
	void SetEmptyBlock(t_pbn2 _emptyblock);
	t_pbn2 GetEmptyBlock( );
};


int get_complen( const _kchar *word1, const _kchar * word2);
int equ_word( const _kchar *word1, const _kchar * word2);

#endif	/* __dicdata_h */

