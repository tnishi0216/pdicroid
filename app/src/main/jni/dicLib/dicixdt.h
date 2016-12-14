#ifndef __dicixdt_h
#define	__dicixdt_h

#ifdef DIC_BOCU
#include "dicUtil.h"	// to avoid compile error
#endif

#ifdef _Windows
#define	USE_NKF		1
#else
#define	USE_NKF		0
#endif

class TLangProc;
class IndexData;

class AllSearchParam {
public:
	int generation;
	SrchMode mode;			/* 検索モード */
	SrchMode mode2;			// 検索モードの下位８ビット
	int level_min;
	int level_max;
#ifdef MIXMJ
	TNMixChar word;			/* 検索文字列 */
	FindStr *fs;
	_jFindStr *fs_j;
	int len;				/* 検索文字列の長さ(_mchar単位の長さ) */
	int len_j;
#else	// !MIXMJ
	_KTChar word;			/* 検索文字列 */
	_ktFindStr *fs;
	int len;				/* 検索文字列の長さ */
#ifdef KMIXDIC
	_KChar word_k;
	int len_k;
#else
#  define	word_k	word
#  define	len_k	len
#endif
#endif	// !MIXMJ
//	bool word_k_comp;		// composit word or not for fast head search
	bool fasthead;			// 高速頭だし検索
	bool fuzzy;

protected:
	const TKCodeTranslateSet *KCodeTrans;
	const TKCodeTranslateSet *GetKCodeTrans() const
		{ return KCodeTrans; }
	// Code translator for fuzzy search.
	const TKCodeTranslateSet *FCodeTrans;
	TLangProc *LangProc;
public:
	
#ifdef USE_REGEXP
	GENERICREXP *re;
	bool re_owner;
#endif

	//検索状態
	int lbn;				// 現在検索中の論理ブロック番号
	int loc;			// 現在検索中の英単語の位置(最後のときは -1 )
	_kchar *wbuf;
	_kchar srchwbuf[ LWORD + 3 ];		// 検索用英単語バッファ

	FINDWORD fw;

protected:
	// Data Buffer //
	TDataBuf *databuf;
	bool databuf_created;
	
	// Methods //
public:
	AllSearchParam();
	virtual ~AllSearchParam();
protected:
	typedef int (AllSearchParam::*f_all_strcmp)( const _mtchar *str );
	f_all_strcmp all_strcmp;		// 文字列比較関数
	typedef int (AllSearchParam::*f_all_strncmp)(const _mtchar *str, int len);
	f_all_strncmp all_strncmp;
public:
	void Stop(bool wait);
	void Close();
	void FreeBuffers();
	void SetDataBuf(TDataBuf *databuf, bool owned);
	TDataBuf *GetDataBuf()
		{ return databuf; }
	bool SetupSearch(const tchar *word, SrchMode mode, GENERICREXP *regp, TLangProc *lp);
	int prevWord( );
	const byte *srchTail( );	// databuf()の一番最後の英単語を返す(返り値はバッファの位置)
	const _kchar *GetFoundWord() const
		{ return fw.fword; }
	//void GetFoundJapa(Japa &j);
protected:
	void SetCompFunc();
public:
	int Compare( const _mtchar *sp );
	int Compare( const _mtchar *sp, int len );
	int strcmp_do( const _mtchar *str )
		{ return (this->*all_strcmp)( str ); }
	int strcmp_do( const _mtchar *str, int len )
		{ return (this->*all_strncmp)( str, len ); }
protected:
	int all_mcsnicmp( const _mtchar *str );
	int all_mcsncmp( const _mtchar *str );
	int all_mmbsnicmp( const _mtchar *str );
	int all_stristr( const _mtchar *str );
	int all_strstr( const _mtchar *str );
	int all_strnistr( const _mtchar *str, int len );
	int all_strnstr( const _mtchar *str, int len );
#ifdef MIXMJ
	typedef int (IndexData::*f_all_strcmp_j)( const _jchar *str );
	f_all_strcmp_j all_strcmp_j;		// 文字列比較関数
	int all_strcmp_do( const _jchar *str )
		{ return (this->*all_strcmp_j)( str ); }
	int all_mcsnicmp( const _jchar *str );
	int all_mcsncmp( const _jchar *str );
	int all_mmbsnicmp( const _jchar *str );
	int all_stristr( const _jchar *str );
	int all_strstr( const _jchar *str );
	int all_strnistr( const _mtchar *str, int len );
	int all_strnstr( const _mtchar *str, int len );
#else	// !MIXMJ
#define	f_all_strcmp_j	f_all_strcmp
#define	all_strcmp_j	all_strcmp
#endif	// !MIXMJ

public:
#ifdef DIC_BOCU
//Note: you should include LangProc.h before including this header to use CompareWord().
#ifdef LangProcH
inline int CompareWord( const char *sp )
	{ return CompareBocu(find_cword_pos(sp), LWORD_X, NULL, KCodeTrans->decodeKT); }
#endif
#else
	inline int CompareWord( const char *s )
		{ return strcmp_do( s ); }
#endif

#ifdef USE_BOCU1
	int CompareBocu( const char *sp, int maxlength, const char **nextp=NULL, FNPreCodeTranslate fn=NULL );
	tchar *CompareBuffer;
	int CompareBufferLen;
#else	// !USE_BOCU1
	int CompareBocu( const char *sp, int /*maxlength*/, const char **nextp=NULL, FNPreCodeTranslate fn=NULL )
		{ return Compare( sp ); }
#endif	// !USE_BOCU1

	// bool CompareWordReg(const _kchar *word);
	bool CompareCompress(const byte *src, ulong jtblen );

#if USE_NKF
protected:
	class TNkf *nkf;
public:
	TNkf *GetNkf();
	static tnstr NkfDllPath;
#endif

protected:
	// for MT
	class AllSearchThread *srch_thread;
public:
	bool StartThread(IndexData &dic, bool forward);
	int GetSearchResult(_KChar &word, Japa *japa);
	void SearcNext();
};


#include "tnthread.h"
#include "events.h"
class AllSearchThread : public tnthread {
typedef tnthread super;
protected:
	AllSearchParam &all;
	class IndexData &dic;

	TSem *ThreadSem;
	TEvent *ThreadEvent;

	bool ThreadEnd;
	bool ThreadDone;
	int ThreadStatus;
public:
	bool Forward;
protected:
	_KChar *fword;	// 全文検索で見つかった見出し語(for MT)
	Japa *fjapa;	// 同Japa
public:
	AllSearchThread(AllSearchParam &all, IndexData &dic);
	~AllSearchThread();
	bool Start(bool forward);
	void Stop(bool wait);
	void StopWait();
	void Next();
	bool Wait(int timeout=-1);
	int GetResult(_KChar &word, Japa *japa);
protected:
	void Execute();
};

///////////////////////
///	IndexDataクラス	///
///////////////////////
class IndexData
#ifdef USE_UNIVDIC
 : public TUniversalDic
#endif
{
protected:
	int error;
	PDICINDEX *index;
	PDICDATA *data;
	short version;
	int compflag;	// 圧縮ポリシー
	class TLangProc *LangProc;	// reference pointer.
								// Pdic classのみのpointerしか無い場合に使いやすくするため。
								// 本物はPdic classを生成したobjectが責任を持つ
	const class TKCodeTranslateSet *KCodeTrans;
public:
	void SetCompFlag( int _compflag );
	int GetCompFlag( )
		{ return compflag; }
	uint GetLimitSize( )	// 登録できる英単語+日本語訳の最大長
		{ return data->GetMaxBlockSize( ) - sizeof(FieldFormat) - 1 - sizeof(tfield); }

private:
#if	__DLL__
	short loc;
#endif
protected:
	int Open( PDICINDEX *index, PDICDATA *data, HEADER &header );
	void Close( );

	// Language Processor //
public:
	//void SetLangProc(TLangProc *proc);
	TLangProc *GetLangProc() const
		{ return LangProc; }
	inline const TKCodeTranslateSet *GetKCodeTrans() const
		{ return KCodeTrans; }

public:
	IndexData( );
	virtual ~IndexData();
	PDICINDEX *GetIndex()		{ return index; }
	PDICDATA *GetData()			{ return data; }
	bool ChangeIndexOffset( int offset );

	// find/search //
protected:
	FINDWORD &fw;
public:
	EVIRTUAL int _BSearch( const _kchar * word, bool forfast=false, AllSearchParam *all=NULL );	// forfast=1で、Field2の場合は、databufにロードされない！！
#if MIXDIC || defined(KMIXDIC)
	int BSearch( const tchar * word, bool forfast=false, AllSearchParam *all=NULL );	// forfast=1で、Field2の場合は、databufにロードされない！！
#endif
	//見つかった単語などの参照
	EVIRTUAL void getfjapa( Japa &j, AllSearchParam *all=NULL );
	const _kchar *getfword()	{return all.fw.fword;}

	virtual bool SetAllSearch( const tchar * word, SrchMode mode, GENERICREXP *jre, AllSearchParam *all=NULL )
		{ return SetupAllSearch(word, mode, jre, all); }
	bool SetupAllSearch(const tchar *word, SrchMode mode, GENERICREXP *jre, AllSearchParam *all=NULL);
#if MIXDIC || defined(KMIXDIC)
	virtual int NextAllSearch_( tnstr &word, Japa *japa, AllSearchParam *all=NULL);
	virtual int PrevAllSearch_( tnstr &word, Japa *japa, AllSearchParam *all=NULL);
#endif
	EVIRTUAL int NextAllSearch_k( _KChar &word, Japa *japa, AllSearchParam *all=NULL);
	EVIRTUAL int PrevAllSearch_k( _KChar &word, Japa *japa, AllSearchParam *all=NULL);
protected:
	EVIRTUAL int CurAllSearch( _KChar &word, Japa *japa, int dirflag, AllSearchParam *all=NULL );		//現在位置からの英単語、日本語訳を得る
public:

#if !defined(SMALL)
	void SetAllScan(AllSearchParam *all=NULL);
#if MIXDIC || defined(KMIXDIC)
	int NextAllScan( _KChar &word, Japa *japa, AllSearchParam *all=NULL );
#endif
#endif
	int JumpAS_k( const _kchar *word, bool fBack, bool fSameBack=FALSE, AllSearchParam *all=NULL );
#if MIXDIC
	int JumpAS_( const tchar *word, bool fBack, bool fSameBack=FALSE, AllSearchParam *all=NULL );
#endif
	int DirectJump( int lbn, AllSearchParam *all=NULL );
	void PrevWord(AllSearchParam *all=NULL);
	int EraseWordAS( AllSearchParam *all=NULL );

// Multithread Search //
public:
	//void InitNextAllSearchMT();
	int NextAllSearchMT( _KChar &word, Japa *japa, AllSearchParam *all);
	int PrevAllSearchMT( _KChar &word, Japa *japa, AllSearchParam *all);
	void StopAllSearchMT(bool wait);
	void SearchMTNext();
	int CommonAllSearchMT( _KChar &word, Japa *japa, AllSearchParam *all, bool forward);
	void SearchLock() { SearchSem.ReadLock(); }
	void SearchUnlock() { SearchSem.ReadUnlock(); }
protected:
	int AllSearchThread(bool forward, AllSearchParam *all);

	TReadWriteLock SearchSem;
	
//全検索に関するメソッド
protected:
	//検索要素
	AllSearchParam all;

	int generation;			// write generation.

	// 正規表現検索を行う場合は、第３引数をセットする必要がある
	// 第３引数のポインターはコンパイル済みのものを渡す事！！
	// もし、コンパイルに失敗した場合などの場合、呼出側でエラー処理行い、
	// 検索が正常に行えるようにしておく事！
	// 正規表現検索を行わない場合は NULL をセットする
	int AllSearchCommon( const byte *ptr, const byte *nextp, wa_t attr, AllSearchParam *all=NULL );
	bool SearchFileLink(AllSearchParam &all, const byte *src, uchar jt, ulong jtblen);

	// record/update/erase //
protected:
	virtual int _record( const _kchar * word, uint wordlen, const Japa &japa);
	virtual int _update( const _kchar * word, uint wordlen, const Japa &japa);
	int _Rename( const _kchar *oldword, const _kchar *newword, int newwordlen );	// 単語名のみの変更
	virtual int _erase();

	// Status Information //
protected:
	FlexArray<int> *IndexBlocks;	// 各index blockの累積block数
public:
	int GetPercent(AllSearchParam *all=NULL);
	bool SetPercentExact(bool strict=false);
	UVIRTUAL const _ktchar *GetAllWord( )
		{ return all.word; }	// 検索文字列
	int GetAllLen( )
		{ return all.len; }

protected:
#ifdef USE_BOCU1
	bool bocu;
public:
	bool IsBocu() const
		{ return bocu; }
#else
public:
	bool IsBocu() const
		{ return false; }
#endif

	// Block operations //
private:
	int CreateNewIndex( int num );
	t_blknum GrowIndex( int num, bool move );
//	int MakeNewBlock( const _kchar *word, int reqlen, int modf );
protected:
	void ShrinkIndex();
	int AddIndex( int lbn, t_pbn2 pbn, const _kchar *word );
public:
	int UpdateObjectHeader( const _kchar *word, t_id id, int offset, int size, const byte *data );

	// Utilities //
protected:
	int EasyOptimize( t_pbn2 *_ipbn, t_pbn2 *_pbn );
public:
	int VeryEasyOptimize(int max_mod=1);
protected:
	int SaveAllTempBuff( int &ntemp, class TempBuff &tb, EmptyList &, t_pbn2 *startpbn );

#if NETDIC
#define	NM_NONE		0
#define	NM_LAN		1
#define	NM_INET		2
protected:
	int NetworkMode;
	tnstr TemporaryName;
	bool KeepTemporary;
	class NetworkDicBase *NDic;
public:
	int GetNetworkMode()
		{ return NetworkMode; }
	void SetNetworkMode( int network )
		{ NetworkMode = network; }
	void SetTemporaryName( const tchar *name )
		{ TemporaryName.set( name ); }
	const tchar *GetTemporaryName()
		{ return TemporaryName; }
	NetworkDicBase *GetNetworkDic()
		{ return NDic; }
	class InetDic *GetInetDic()
		{ return NetworkMode==NM_INET ? (InetDic*)NDic : NULL; }
#endif
	void FreeBuffers(AllSearchParam *all=NULL);	// 検索などで使用していたbufferを解放する

	// debug //
protected:
	bool LogEnabled;
	tnstr LogName;
	tnstr BackupDir;
	bool Updated;
	void DoBackup();
	void diclog( const tchar *format, ... );
};

#endif	/* __dicixdt_h */


