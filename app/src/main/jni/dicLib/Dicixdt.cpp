#include	"tnlib.h"
#pragma	hdrstop
#include "pdconfig.h"
#include "pdtypes.h"
#include "LangProc.h"
#include "LangProcMan.h"
#include "dic.h"
#ifdef	_WINDOWS
#include	"jreusr.h"
#endif
#include	"japa.h"
#include	"faststr.h"
#if defined(USE_JLINK)
#include	"jlink.h"
#ifdef USE_FILELINK
#include "JLFileSearch.h"
#endif
#endif
#include	"dicdbg.h"
#if NETDIC
#include "netdic.h"
#endif
#if INETDIC
#include "inetdic.h"
#endif

#if USE_NKF
#include "nkfdll.h"
#endif

#define	DEBUG_DIC	0
#define	DBDIC		DBW

#define	__kwstr(var)	_kwstr(var, GetKCodeTrans())	// for keyword

// macro for NEWDIC4
#define	_SetAll( buf, len, _this, _field2, _attr )	SetAll( buf, len, _this, _field2, (_attr) )
#define	SEL_FIELD2( flag, field2, field1 )	((flag) ? (field2) : (field1))

#define	_ATTRBYTE	0

#define	NEWINDEX_ALLOC_SIZE		(16)		// index areaをdata areaから取得する際の単位

#define	MIN_FIELD2_JLEN			(0xFFF0-LWORD)	// Field2にするかどうかの判断最小バイト数（適当に余裕のある数値に設定）

static tnstrA SrchQ;	// string '?' as bocu-1 if bocu dictionary invoked.

//////////////////////////////////////////////////////////////////////////
// AllSearchParam class
//////////////////////////////////////////////////////////////////////////
AllSearchParam::AllSearchParam()
{
	generation = 0;
	wbuf = srchwbuf + 2;
	fs = NULL;
	LangProc = NULL;
#ifdef USE_REGEXP
	re = NULL;
	re_owner = false;
#endif
#ifdef USE_BOCU
	CompareBuffer = NULL;
	CompareBufferLen = 0;
#endif
	databuf = NULL;
	databuf_created = false;
#if USE_NKF
	nkf = NULL;
#endif

	srch_thread = NULL;
}
AllSearchParam::~AllSearchParam()
{
	Close();
}
void AllSearchParam::Stop(bool wait)
{
	if (srch_thread){
		srch_thread->Stop(wait);
	}
}
void AllSearchParam::Close()
{
	if (srch_thread){
		srch_thread->Stop(true);
		delete srch_thread;
		srch_thread = NULL;
	}

	_delete(fs);
	FreeBuffers();
	if (databuf && databuf_created){
		delete databuf;
		databuf = NULL;
	}
	if (LangProc){
		LangProc->Release();
		LangProc = NULL;
	}
#ifdef USE_REGEXP
	if (re_owner && re){
		delete re;
		re = NULL;
		re_owner = false;
	}
#endif
#if USE_NKF
	_delete(nkf);
#endif
}
void AllSearchParam::FreeBuffers()
{
#ifdef USE_BOCU
	if (CompareBuffer){
		delete[] CompareBuffer;
		CompareBuffer = NULL;
		CompareBufferLen = 0;
	}
#endif
}
void AllSearchParam::SetDataBuf(TDataBuf *databuf, bool owned)
{
	if (this->databuf && databuf_created){
		delete this->databuf;
	}
	this->databuf = databuf;
	this->databuf_created = owned;
}

// regp : 
//	!=NULLの場合: regpを使用する(re_owner=false)
//	==NULLの場合: this->re/re_ownerを使用する
bool AllSearchParam::SetupSearch(const tchar *word, SrchMode mode, GENERICREXP *regp, TLangProc *lp)
{
	Stop(true);

	srchwbuf[0] = '\r';
	srchwbuf[1] = '\n';
	lbn = -1;	/* 論理ブロック番号 */
	loc = -1;
#ifdef USE_REGEXP
	GENERICREXP *del_re;
	if (re && re_owner){
		del_re = re;
	} else {
		del_re = NULL;
	}
	if (mode&SRCH_REGEXP){
		if (regp){
			re = regp;
		} else {
			del_re = NULL;
		}
		if (!re)
			return false;
	} else {
		re = NULL;
	}
	if (del_re){
		delete del_re;
		re_owner = false;
	}
#endif
	this->mode = mode;
	mode2 = (SrchMode)(mode & SRCH_TARGET_MASK);

	if ( ( mode & SRCH_HEAD ) && ( mode2 == SRCH_WORD )
		/* && !( mode & ( SRCH_IGN | SRCH_EXTMASK ) ) */	// 2008.1.5 条件から削除
		 ){
		//高速頭だし検索(見出語のみで、文字を区別する場合のみ
		fasthead = true;
#ifdef	USE_REGEXP
//		re = NULL;
#endif
	} else {
		fasthead = false;
	}
#ifdef USE_SRCH_FUZZY
	fuzzy = (mode & SRCH_FUZZY) ? true : false;
#endif
#ifdef USE_FILELINK
	if (mode2 & SRCH_FILECONT) SearchFileLinkSetup();
#endif

	if (LangProc){
		LangProc->Release();
		LangProc = NULL;
	}

	// 単語レベルの範囲調整
	KCodeTrans = NULL;
	FCodeTrans = NULL;
	if ( mode & SRCH_LEVEL ){
		if ( word[0] > word[1] ){
			level_min = word[1]-'0';
			level_max = word[0]-'0';
		} else {
			level_min = word[0]-'0';
			level_max = word[1]-'0';
		}
	} else
	if ( word ){
		KCodeTrans = lp->GetKCodeTrans();
		__assert(KCodeTrans);
#ifdef KMIXDIC
		if (fasthead){
			// for keyword search
			bool word_k_comp = is_composit_word(word);
			word_k = (_kchar*)_kwstr(word_k_comp?word:lp->Normalize(word).c_str(), GetKCodeTrans());
			if (!word_k[0])	// for no-keyword translation
				word_k = (_kchar*)word;
			len_k = _kcslen(word_k);
		} else
#endif	// KMIXDIC
		{
			// for search except keyword
			if (fuzzy){
				lp->IncRef();
				LangProc = lp;
			}
			this->word.set( (_ktchar*)_kstrn(word) );
			len = _ktcslen( this->word );
			if ( fs ) delete fs;
			fs = new _ktFindStr( this->word, mode & SRCH_IGN );
#ifdef MIXMJ
			all_len_j = _jcslen(this->word);
			if ( all_fs_j ) delete all_fs_j;
			all_fs_j = new _jFindStr( this->word, mode & SRCH_IGN );
#endif
		}
	} else {
		len = 0;
		this->word.clear();
	}

	// setup string '?'
	if (SrchQ.empty()){
		static const tchar *StrQOrg = _t("?");
#ifdef USE_BOCU
		uchar buf[3];
		uchar *dp = bocu1EncodeT(StrQOrg, StrQOrg+1, buf);
		*dp = '\0';
		SrchQ = (char*)buf;
#else
		SrchQ = StrQOrg;
#endif
	}
	
	SetCompFunc();
	return true;
}

int AllSearchParam::prevWord( )
{
	return databuf->prevWord((uint&)loc, wbuf);
}

const uint8_t *AllSearchParam::srchTail( )	// databuf()の一番最後の英単語を返す(返り値はバッファの位置)
{
	return databuf->srchTail(wbuf);
}

void AllSearchParam::SetCompFunc()
{
	// Note:
	// all_strcmpは見出語でも、日本語訳でも共通にしてあるので、
	// 同一検索条件であれば、同じ関数を代入するようにしておくこと
	if (mode & SRCH_IGN){
#if defined(PDICW) && !defined(_UNICODE)
		if ( mode & SRCH_WORD_S ){
			if ( mode & SRCH_HEAD ){
				all_strcmp = &AllSearchParam::all_mcsnicmp;
				all_strncmp = NULL;
			} else {
				all_strcmp = &AllSearchParam::all_stristr;
				all_strncmp = &AllSearchParam::all_strnistr;
			}
		} else
#endif
		{
			if ( mode & SRCH_HEAD ){
				all_strcmp = &AllSearchParam::all_mmbsnicmp;
				all_strncmp = NULL;
#ifdef MIXMJ
				all_strcmp_j = &AllSearchParam::all_mmbsnicmp;
#endif
			} else {
				all_strcmp = &AllSearchParam::all_stristr;
				all_strncmp = &AllSearchParam::all_strnistr;
#ifdef MIXMJ
				all_strcmp_j = &AllSearchParam::all_stristr;
#endif
			}
		}
	} else {
		if ( ( mode & SRCH_HEAD ) ){
			all_strcmp = &AllSearchParam::all_mcsncmp;
			all_strncmp = NULL;
#ifdef MIXMJ
			all_strcmp_j = &AllSearchParam::all_mcsncmp;
#endif
		} else {
#if defined(PDICW) && !defined(_UNICODE)
			if ( mode & SRCH_WORD_S ){
				all_strcmp = &AllSearchParam::all_strstr;
				all_strncmp = &AllSearchParam::all_strnstr;
			} else
#endif
			{
				all_strcmp = &AllSearchParam::all_strstr;
				all_strncmp = &AllSearchParam::all_strnstr;
#ifdef MIXMJ
				all_strcmp_j = &AllSearchParam::all_strstr;
#endif
			}
		}
	}
}

#ifdef DIC_UTF8
//#define	all_re_compare_kword(s)	all.re->CompareUTF8(s)	// compare for keyword
#define	all_re_compare(s)	this->re->CompareUTF8(s)
#define	all_re_compare_t	all_re_compare
#elif defined(DIC_BOCU)
//#include "ktable.h"
//#define	all_re_compare_kword(s)	all.re->CompareBocu1(s, decodeKT)	// compare for keyword. require to include 'ktable.h'
#define	all_re_compare(s)	this->re->CompareBocu1(s, NULL)
#define	all_re_compare_t(s)	this->re->Compare(s)
#else
//#define	all_re_compare_kword(s)	all.re->Compare(s)
#define	all_re_compare(s)	this->re->Compare(s)
#define	all_re_compare_t	all_re_compare
#endif

int AllSearchParam::Compare( const _mtchar *sp )
{
#ifdef USE_REGEXP
	if ( re ){
		if (all_re_compare_t( sp ))
			return 1;
	} else
#endif
	{
		if ( strcmp_do( sp ) ){
			return 1;
		}
	}
	return 0;
}

int AllSearchParam::Compare( const _mtchar *sp, int len )
{
#ifdef USE_REGEXP
	if ( re ){
		if (all_re_compare_t( sp ))
			return 1;
	} else
#endif
	{
		if ( strcmp_do( sp, len ) ){
			return 1;
		}
	}
	return 0;
}

// SRCH_HEAD専用
int AllSearchParam::all_mcsnicmp( const _mtchar *str )
{
	return _mtcsnicmp( str, word, len )==0;
}
// SRCH_HEAD専用
int AllSearchParam::all_mcsncmp( const _mtchar *str )
{
	return _mtcsncmp( str, word, len )==0;
}
// SRCH_HEAD専用
int AllSearchParam::all_mmbsnicmp( const _mtchar *str )
{
	return _mtmbsnicmp( str, word, len )==0;
}
int AllSearchParam::all_stristr( const _mtchar *str )
{
	return (int)(uint_ptr)fstristr( str, _mtcslen(str), *fs );
}
int AllSearchParam::all_strstr( const _mtchar *str )
{
	return (int)(uint_ptr)fstrstr( str, _mtcslen(str), *fs );
}
int AllSearchParam::all_strnistr( const _mtchar *str, int len )
{
	return (int)(uint_ptr)fstristr( str, len, *fs );
}
int AllSearchParam::all_strnstr( const _mtchar *str, int len )
{
	return (int)(uint_ptr)fstrstr( str, len, *fs );
}

#if defined(MIXMJ)
// DIC_UTF8では、_jchar == wchar_t
// SRCH_HEAD専用
int AllSearchParam::all_mcsnicmp( const _jchar *str )
{
	return _jcsnicmp( str, all.word, all_len_j )==0;
}
// SRCH_HEAD専用
int AllSearchParam::all_mcsncmp( const _jchar *str )
{
	return _jcsncmp( str, all.word, all_len_j )==0;
}
// SRCH_HEAD専用
int AllSearchParam::all_mmbsnicmp( const _jchar *str )
{
	return _jmbsnicmp( str, all.word, all_len_j )==0;
}
int AllSearchParam::all_stristr( const _jchar *str )
{
	return (int)fstristr( str, _jcslen(str), *all_fs_j );
}
int AllSearchParam::all_strstr( const _jchar *str )
{
	return (int)fstrstr( str, _jcslen(str), *all_fs_j );
}
int AllSearchParam::all_strnistr( const _jchar *str, int len )
{
	return (int)fstristr( str, len, *all_fs_j );
}
int AllSearchParam::all_strnstr( const _jchar *str, int len )
{
	return (int)fstrstr( str, len, *all_fs_j );
}
#endif // MIXMJ

#ifdef USE_BOCU
int AllSearchParam::CompareBocu( const char *sp, int maxlength, const char **nextp, FNPreCodeTranslate fn )
{
	if (CompareBufferLen<maxlength){
		if (CompareBuffer)
			delete[] CompareBuffer;
		CompareBufferLen = maxlength;
		CompareBuffer = new _jchar[maxlength];
	}
	int dstlen;
	bocu1DecodeT( (const uint8_t**)&sp, (const uint8_t*)UINT_PTR_MAX, maxlength, CompareBuffer, &dstlen, fn );
	if (fuzzy){
		tchar *dst = LangProc->NormalizeSearchPattern(CompareBuffer);
		dstlen = (int)(dst-CompareBuffer);
	}
	if (nextp){
		*nextp = sp;
	}
#ifdef USE_REGEXP
	if ( re ){
		if ( re->Compare( CompareBuffer ) ){
			return 1;
		}
	} else
#endif
	{
		if (strcmp_do( CompareBuffer, dstlen )){
			return 1;
		}
	}
	return 0;
}
#endif	// USE_BOCU

#if 0
#ifdef USE_REGEXP
// wordのcword部のみの検索
bool AllSearchParam::CompareWordReg(const _kchar *word)
{
#ifdef DIC_UTF8
#elif defined(DIC_BOCU)
	return re->CompareBocu1(find_cword_pos(word), KCodeTrans->decodeKT);	// compare for keyword. require to include 'ktable.h'
#else
#endif
}
#endif	// USE_REGEXP
#endif

// 圧縮textのcompare
#ifdef USE_COMP
bool AllSearchParam::CompareCompress(const uint8_t *src, uint jtblen )
{
	uint decodelen;
	uint8_t *decode = Japa::Decode( src, jtblen, decodelen );
	if ( decode ){
		if ( Compare( (_mtchar*)decode ) ){
			delete[] decode;
			return true;	// matched.
		}
		delete[] decode;
	}
	return false;	// not matched or error
}
#endif

#if USE_NKF
TNkf *AllSearchParam::GetNkf()
{
	if (!nkf)
		nkf = new TNkf(NkfDllPath);
	return nkf;
}
#endif
bool AllSearchParam::StartThread(IndexData &dic, bool forward)
{
	if (!srch_thread){
		srch_thread = new AllSearchThread(*this, dic);
		if (!srch_thread) return false;
	}
	return srch_thread->Start(forward);
}
inline int AllSearchParam::GetSearchResult(_KChar &word, Japa *japa)
{
	return srch_thread->GetResult(word, japa);
}
inline void AllSearchParam::SearcNext()
{
	srch_thread->Next();
}
// AllSearchThread class //
AllSearchThread::AllSearchThread(AllSearchParam &_all, IndexData &_dic)
	:super(true)	// create suspend
	,all(_all)
	,dic(_dic)
{
	ThreadSem = NULL;
	ThreadEvent = NULL;
	ThreadDone = true;
	ThreadStatus = AS_END;

	Forward = true;

	fword = NULL;
	fjapa = NULL;
}
AllSearchThread::~AllSearchThread()
{
	Stop(false);

	_delete(fword);
	_delete(fjapa);
}
bool AllSearchThread::Start(bool forward)
{
	Forward = forward;

	if (ThreadSem){
		return true;	// already running
	}

	// create thread
	ThreadEnd = false;
	ThreadDone = false;
	ThreadStatus = AS_NONE;

	do {
		ThreadSem = new TSem(0);
		if (!ThreadSem) break;
		ThreadEvent = new TEvent;
		if (!ThreadEvent) break;
		StartThread();
		ThreadStatus = AS_RUNNING;
		ThreadEvent->Wake();
		return true;
	} while(0);

	// error
	_delete(ThreadSem);
	_delete(ThreadEvent);
	return false;
}
void AllSearchThread::Stop(bool wait)
{
	ThreadEnd = true;
	ThreadEvent->Wake();
	if (wait){
		StopWait();
	}
}
void AllSearchThread::StopWait()
{
	WaitFor();
}
void AllSearchThread::Next()
{
	__assert(ThreadStatus==AS_CONTINUE || ThreadStatus==AS_FOUND);
	ThreadStatus = AS_RUNNING;
	ThreadEvent->Wake();
}
bool AllSearchThread::Wait(int timeout)
{
	if (!ThreadSem) return false;
	return ThreadSem->Wait(timeout);
}
int AllSearchThread::GetResult(_KChar &word, Japa *japa)
{
	int ts = ThreadStatus;
	switch (ts){
		case AS_FOUND:
			word = *fword;
			if (japa) *japa = *fjapa;
			Wait(0);
			Next();
			break;
		case AS_CONTINUE:
			Wait(0);
			Next();
			break;
		case AS_END:
		case AS_ERROR:
			break;
	}
	return ts;
}
//static LONG ThreadCount = 0;
//TODO:
//	AS_CONTINUEは、他の検索のためにthreadの同期を取るために使用している。
//	もしそれが必要なければAS_CONTINUEは不要になる
//Note:
//	threadが終了してもhThreadはcloseされないことに注意。
//	closeするには、
//	- AS_END/AS_ERRORの場合は、GetResult()する
//	- Stop(true)を呼び出す
//	- delete object
//	※hThreadがcloseされないとStart()で再開できない。
void AllSearchThread::Execute()
{
	//InterlockedIncrement(&ThreadCount);
	//DBW("Start:%08X:%08X:%08X %08X %d", this, &dic, GetCurrentThreadId(), &all, ThreadCount);
	for (;;){
		ThreadEvent->Wait();
		if (ThreadEnd)
			break;
		if (!fword) fword = new _KChar;
		if (!fjapa) fjapa = new Japa;

		//dic.SearchLock();
		ThreadStatus = AS_RUNNING;
		ThreadStatus = Forward ? dic.NextAllSearch_k(*fword, fjapa, &all) : dic.PrevAllSearch_k(*fword, fjapa, &all);
		//dic.SearchUnlock();

		ThreadSem->Lock();
		if (ThreadStatus==AS_END || ThreadStatus==AS_ERROR){
			break;
		}
	}
	//InterlockedDecrement(&ThreadCount);
	//DBW("End:%08X:%08X:%08X %d", this, &dic, GetCurrentThreadId(), ThreadCount);
	_delete(ThreadEvent);
	ThreadDone = true;
	_delete(ThreadSem);
}

#if USE_NKF
tnstr AllSearchParam::NkfDllPath(_t("nkfu.dll"));
#endif

//////////////////////////////////////////////////////////////////////////
// IndexData class
//////////////////////////////////////////////////////////////////////////
IndexData::IndexData( )
	:fw(all.fw)
{
	error = 0;
	data = NULL;
	index = NULL;
	compflag = CP_NOCOMP;	// 圧縮しない
#ifdef MIXMJ
	all_fs_j = NULL;
#endif
	generation = 0;
#if NETDIC || INETDIC
	NetworkMode = NM_NONE;
	NDic = NULL;
#endif
	IndexBlocks = NULL;
	LangProc = NULL;

	LogEnabled = false;
}

IndexData::~IndexData()
{
	if (IndexBlocks)
		delete IndexBlocks;
	FreeBuffers(&all);
	if (LangProc)
		LangProc->Free();
#if NETDIC || INETDIC
	if ( NDic ){
		delete NDic;
	}
#endif
	if ( data ){
		delete data;
	}
	if ( index ){
		delete index;
	}
#ifdef MIXMJ
	if ( all_fs_j ){
		delete all_fs_j;
	}
#endif
#if 0
	// kstr,mstrのslab解放
#endif
}

int IndexData::Open( PDICINDEX *_index, PDICDATA *_data, HEADER &header )
{
	DDOpen();
	DD( DD_OPEN );
	index = _index;
	data = _data;
	version = header.version;

	// Language Processor
	int lpinx = LangProcMan.FindNearestId(header.lang_proc_id);
	if (lpinx<0){
		error = DICERR_NOLANGPROC;
		DBW("IndexData::Open - lang proc error:%d", header.lang_proc_id);
		goto error;
	}
	__assert(!LangProc);
	LangProc = LangProcMan.GetLangProc(lpinx);
	KCodeTrans = LangProc->GetKCodeTrans();
	__assert(LangProc);

	if ( index->Open( &header ) == -1 ) goto error;
#if defined(USE_DICORDER)
	index->SetOrder( header.dicorder );
#endif
	if ( data->Open( &header, this ) == -1 ){
		index->Close();
		goto error;
	}
#if defined(USE_DICORDER)
	data->SetOrder( header.dicorder );
#endif
	compflag = header.dictype & DICID_COMP ? CP_COMP1 : CP_NOCOMP;
	compflag |= CP_USEFIELD2;
	all.SetDataBuf(data->GetDataBuf(), false);

#ifdef _Windows	// Windowsのみ？
	// debug //
	if (DicLogEnabled){
		if (DicLogEnabled==2){
			LogEnabled = true;
		} else {
			tnstr dbgname = _index->GetFile().GetFileName();
			dbgname += _t(".dbg");
			if (fexist(dbgname)){
				LogEnabled = true;
			}
		}
	}
	if (LogEnabled){
		Updated = false;
		tnstr filename = _index->GetFile().GetFileName();
		LogName = filename;
		LogName += _t(".dbg.log");
		tchar dir[_MAX_PATH];
		GetTempPath(tsizeof(dir), dir);
		BackupDir = dir;
		diclog(_t("opened : %s"), filename.c_str());
	}
#endif

	return 0;
error:
	delete data;
	delete index;
	data = NULL;
	index = NULL;
	return -1;
}

void IndexData::Close( )
{
	StopAllSearchMT(false);

	if (LangProc){
		LangProc->Free();
		LangProc = NULL;
	}
	all.Close();
#ifdef MIXMJ
	if ( all_fs_j ){
		delete all_fs_j;
		all_fs_j = NULL;
	}
#endif
	if ( data ){
		data->Close();
		delete data;
		data = NULL;
	}
	if ( index ){
		index->Close();
		delete index;
		index = NULL;
	}
	DDClose();
	if (LogEnabled){
		diclog(_t("closed"));
	}
}

#if 0
void IndexData::SetLangProc(TLangProc *proc)
{
	LangProc = proc;
	KCodeTrans = LangProc->GetKCodeTrans();
}
#endif

void IndexData::SetCompFlag( int _compflag )
{
	if ( compflag ){
		compflag = _compflag | CP_COMP | (compflag & CP_USEFIELD2);
#ifdef DIC_BOCU
		compflag &= ~CP_TEXT;		// BOCUはテキスト圧縮は対応しない
#endif
	}
}

#ifdef EXTHEADER
// index部の開始位置をnumブロック分だけずらす
bool IndexData::ChangeIndexOffset( int num )
{
	if (num>=0){
		for(;num>0;){
			t_blknum n = GrowIndex(num,true);
			if (n==(t_blknum)-1)
				return false;
			num -= n;
		}
	} else {
		// num<0
		// index領域を広げるだけにする
		// datablockへの空き領域の追加はShrinkIndex()で行う
		index->MoveGrowIndex( -num );
	}
	return true;
}
#endif	// EXTHEADER

/*======================================================================*/
/*IndexData::BSearch													*/
/*======================================================================*/
/*引数：lbn = 論理ブロック番号											*/
/*      pbn = 物理ブロック番号											*/
/*      lp  = 該当する箇所の位置（ブロックの先頭からのオフセット）		*/
/*      fword = 該当する見出語と日本語訳（'\0'になるときもある）		*/
/* 		word1 = fwordの前の非圧縮の見出語								*/
/*返り値：-1 ディスクエラー												*/
/*        1  見つかった(同じ単語が見つかったわけではない?)				*/
/*        2  Field2だった(forfast=1時)									*/
/*        0  １語も登録されていない、見つからない（＝一番最後）			*/
/*======================================================================*/
int IndexData::_BSearch( const _kchar *word, bool forfast, AllSearchParam *_all )
{
	AllSearchParam &all = _all ? *_all : this->all;
	FINDWORD &fw = _all ? _all->fw : this->all.fw;	// tricky

	if ( index->GetIndexNum() == 0){
		fw.clear();
		return 0;
	}
	fw.pbn = index->indexBlock( fw.lbn = index->indexSearch( word ) ); // 論理/物理ブロック番号を求める
	// for fast access // データは読込まれないので注意！！
	if ( forfast && fw.lbn < index->GetIndexNum() && all.GetDataBuf()->CheckField2( fw.pbn ) == 1 ){
		// Field2
		fw.lp = 0;
		index->indexWord( fw.fword, fw.lbn );
		return 2;
	}

	if (!all.GetDataBuf()){
		// 少なくともthis->allではないので、新規にdatabufを作成
		all.SetDataBuf(data->CreateDataBuf(), true);
	} else {
		all.GetDataBuf()->SetData( data );
		//if (all.generation!=generation)
		// 2012.8.30 : 更新された後、同じblockを検索すると更新されたデータが読み込まれないため、無条件に無効化。
		{
			//__assert(!all.GetDataBuf()->isDirty());
			if (!all.GetDataBuf()->invalidate()){
				return -1;
			}
		}
	}
	all.generation = generation;

	if ( data->read(fw.pbn, all.GetDataBuf()) == -1)
		return -1;

	_kchar wbuf[LWORD+1];

	index->indexWord(wbuf, fw.lbn);		// ブロックの先頭の見出語を取得
	int ret = data->recSearch(word, wbuf, fw, *all.GetDataBuf());
	if (ret==0){
		// ブロックの終端であった場合は、次のブロックの先頭にする ( 1995.1.5 )
		if ( fw.lbn < index->GetIndexNum() - 1 ){	// ブロック番号のチェック
			fw.pbn = index->indexBlock( ++fw.lbn );
			if ( data->read( fw.pbn, all.GetDataBuf() ) == -1 ){
				return -1;
			}
			index->indexWord( wbuf, fw.lbn );
			ret = data->recSearch( word, wbuf, fw, *all.GetDataBuf() );
		} else {
			// 一番最後の単語であるため、見つからない
			return 0;	// bug fix 1995.7.5
		}
	}
	return ret;
}

void IndexData::getfjapa( Japa &j, AllSearchParam *_all )
{
	AllSearchParam &all = _all ? *_all : this->all;

	__assert(all.generation==generation);
	TDataBuf &databuf = *all.GetDataBuf();
	__assert(&databuf);
	FieldFormat *p = databuf.GetDataPtr( all.fw.lp );
	tfield2 flen;
	const _kchar *cp;
	wa_t attr;
	if ( databuf.isField2() ){
		flen = FF2(p)->fieldlen;
		cp = FF2(p)->word;
		attr = FF2(p)->attr;
	} else
	{
		flen = p->fieldlen;
		cp = p->word;
		attr = p->attr;
	}
	while ( *cp++ ) flen-=sizeof(_kchar);
	flen-=sizeof(_kchar);
	j._SetAll( (uint8_t*)cp, (int)flen, this, databuf.isField2(), attr );
}

/*======================================================================*/
/*IndexData::setAllSearch												*/
/*======================================================================*/
/*	辞書全体から一致する単語全てを探す。								*/
/*	この関数ではその初期化を行うだけ									*/
/*	mode = 1:見出語 2:日本語訳 3:全て									*/
/*	注意！：wordの内容は検索終了まで、保持していること					*/
/*======================================================================*/
// modeにSRCH_IGNがあってもVWXを使いません.

// The common procedure for both PdicBase and PdicExternalDB.
bool IndexData::SetupAllSearch(const tchar *word, SrchMode mode, GENERICREXP *regp, AllSearchParam *_all)
{
	AllSearchParam &all = _all ? *_all : this->all;

	if (!all.GetDataBuf()){
		// 少なくともthis->allではないので、新規にdatabufを作成
		all.SetDataBuf(data->CreateDataBuf(), true);
	} else {
		all.GetDataBuf()->SetData( data );
		//if (all.generation!=generation)
		// 2012.8.30 : 更新された後、同じblockを検索すると更新されたデータが読み込まれないため、無条件に無効化。
		{
			//__assert(!all.GetDataBuf()->isDirty());
			if (!all.GetDataBuf()->invalidate()){
				return false;
			}
		}
	}
	if (_all){
		// generationに関係なく、他のAllSearchParamで検索を行う場合は必ずflush
		if (data)
			data->Flush();
	}
	all.generation = generation;

	if (!all.SetupSearch(word, mode, regp, LangProc)){
		return false;
	}
	if (all.fasthead){
		JumpAS_k( all.word_k, false, true, &all );
	}
	return true;
}

// AS_FOUND		: 見つかった
// AS_CONTINUE	: 検索中
// AS_ERROR		: エラー
// AS_END		: 全て終了
int IndexData::NextAllSearch_k( _KChar &word, Japa *japa, AllSearchParam *_all)
{
	AllSearchParam &all = _all ? *_all : this->all;

	if (all.generation!=generation){
		// jumpAS()をしないでいきなりこの関数を呼ぶことはできない
		error = DICERR_GEN_CHANGED;
		return AS_ERROR;
	}

	if (!index || index->GetIndexNum()==0)
		return AS_END;

	TDataBuf &databuf = *all.GetDataBuf();

	bool fField2 = databuf.isField2();
	const FieldFormat *all_cp = (FieldFormat*)( databuf.getDataBuf() + all.loc );
	__assert(all.loc==-1 || databuf.getDataBuf());
	if ( all.loc == -1 || all_cp->fieldlen == 0 ){
		all.lbn++;
		if ( all.lbn < 0 || all.lbn >= index->GetIndexNum()){
			all.lbn = index->GetIndexNum();
			all.loc = data->GetTopLoc();		// 次のprevAllSearchのため
			return AS_END;		/* すべての検索終了 */
		}
#if 0	// 1996.4.11
		//頭検索の終了検査
		if ( all.fasthead ){
			index->indexWord( all.wbuf, all.lbn );
			if ( cmpnword( all.wbuf, GetAllWord(), GetAllLen(), data->GetOrder() ) > 0 ){
				return AS_END;
			}
//			if ( jcharcmp( all.wbuf, all.word ) > 0 ){
//				return AS_END;	// 全て終了
//			}
		}
#endif

		if ( data->read( index->indexBlock(all.lbn), &databuf, false ) == -1){
			return AS_ERROR;
		}
		all.loc = data->GetTopLoc( );
		all_cp = (FieldFormat*)(databuf.getDataBuf() + all.loc);
		fField2 = databuf.isField2();
		if ( fField2 ){
			goto jmp0_2;
		} else
			goto jmp0;
	}
	const _kchar *nextp;
	const _kchar *p;
	wa_t attr;
	for (;;) {
		if ( fField2 ){
			//TODO: which?
			all_cp = (FieldFormat*)NextField2( (FieldFormat2*)all_cp );
			if ( ((FieldFormat2*)all_cp)->fieldlen == 0 )
			{
				all.loc = FP_DIFF( (char*)all_cp, databuf.getDataBuf() );
				return AS_CONTINUE;
			}
jmp0_2:
			index->indexWord( all.wbuf, all.lbn );
			p = ((FieldFormat2*)all_cp)->word;
			attr = ((FieldFormat2*)all_cp)->attr;
			nextp = (const _kchar *)NextField2( (FieldFormat2*)all_cp );
		} else
		{
			all_cp = NextField( all_cp );
			if ( all_cp->fieldlen == 0 ){
				all.loc = FP_DIFF( (uint_ptr)all_cp, (uint_ptr)databuf.getDataBuf() );
				return AS_CONTINUE;
			}
jmp0:
			p = all_cp->word;
			_kcscpy( all.wbuf + all_cp->complen, p );
			attr = all_cp->attr;
			nextp = (const _kchar *)NextField( all_cp );
		}
		while ( *p++ ) ;
		if ( all.fasthead ){
			if ( index->cmpnword( all.wbuf, all.word_k, all.len_k ) ){
				return AS_END;
			}
			if (all.mode & SRCH_REGEXP){
				// 正規表現が有効な場合はその条件が一致した場合のみ
				if ( AllSearchCommon( (uint8_t*)p, (uint8_t*)nextp, attr, &all) )
					break;
			} else {
				break;
			}
		} else {
			if ( AllSearchCommon( (uint8_t*)p, (uint8_t*)nextp, attr, &all) ){
				break;
			}
		}
	}
	//全検索 or 一致したもの
	word.set( all.wbuf );

	if ( japa ){
		japa->_SetAll( (uint8_t*)p, FP_DIFF( nextp, p ), this, fField2, SEL_FIELD2( fField2, ((FieldFormat2*)all_cp)->attr, all_cp->attr ) );
	}
	all.loc = FP_DIFF( (char*)all_cp, databuf.getDataBuf() );
	return AS_FOUND;
}
#if MIXDIC
int IndexData::NextAllSearch_( tnstr &word, Japa *japa, AllSearchParam *_all )
{
	_MChar _word;
	int r = NextAllSearch_k( _word, japa, _all );
	if (r!=AS_FOUND)
		return r;
	word = __kwstr(_word);
	return AS_FOUND;
}
int IndexData::PrevAllSearch_( tnstr &word, Japa *japa, AllSearchParam *_all )
{
	_MChar _word;
	int r = PrevAllSearch_k( _word, japa, _all );
	if (r!=AS_FOUND)
		return r;
	word = __kwstr(_word);
	return AS_FOUND;
}
#endif

#if !defined(SMALL)
// NEWDIC3のみ対応
void IndexData::SetAllScan(AllSearchParam *_all)
{
	AllSearchParam &all = _all ? *_all : this->all;

	all.lbn = -1;	/* 論理ブロック番号 */
	all.loc = -1;

	all.len = 0;
	all.word.clear();

//	all.mode = SRCH_ALL;
//	all.mode2 = (SrchMode)(mode & SRCH_TARGET_MASK);

	all.fasthead = false;
}

#if !defined(DIC_UTF8) && !defined(DIC_BOCU)	// DIC_UTF8用はまだ作っていない
// AS_FOUND		: 見つかった
// AS_CONTINUE	: 検索中(AllScanでは返さない)
// AS_ERROR		: エラー
// AS_END		: 全て終了
// japaは、訳語部しかセットしない
//
// AllSearchと変数の意味が異なるので注意！！
// all.loc : t_blknumを除いたdatabufの先頭からのoffset
//           次のfieldlenを示す
int IndexData::NextAllScan( _KChar &word, Japa *japa, AllSearchParam &all )
{
	if (all.generation!=generation){
		// jumpAS()をしないでいきなりこの関数を呼ぶことはできない
		error = DICERR_GEN_CHANGED;
		return AS_ERROR;
	}

	t_blknum blknum = 0;

	FileBuf &file = data->GetFileBuf();

	for (;;){
		if ( all.loc == -1 ){
			all.lbn++;
			if ( all.lbn >= index->GetIndexNum() )
				return AS_END;
			all.loc = 0;

			data->seek( index->indexBlock( all.lbn ) );
			file.read( &blknum, sizeof(t_blknum) );	// for NEWDIC3
			_kcscpy( all.wbuf, index->indexWord( all.lbn ) );
		} else {
			data->seek( index->indexBlock( all.lbn ), all.loc + sizeof(t_blknum) );
		}

		int ljapa;
		if ( blknum & FIELDTYPE ){
			// Field 2
			all.loc = -1;
			if ( japa ){
				FieldFormat2 fft2;
				file.read( &fft2, L_FieldHeader2 );
//				file.read( all.wbuf + fft2.complen, _MLENTOBYTE(LWORD - fft2.complen) );
				file.seekcur(
					sizeof(_mchar)	// = '\0'
					);
				ljapa = fft2.fieldlen + sizeof(_mchar);
			}
		} else
		{
			FieldFormat fft;

			file.read( &fft, L_FieldHeader );
			if ( fft.fieldlen == 0 ){
				// 最後
				all.loc = -1;
				continue;
			}
			int cnt = file.read( all.wbuf + fft.complen, _MLENTOBYTE(LWORD - fft.complen) );
			all.loc += fft.fieldlen + L_FieldHeader;
			if ( japa ){
				// 圧縮時見出語＋'\0'＋属性の長さを求める
				ljapa = _kcsbyte1(all.wbuf+fft.complen);
				// 日本語訳の先頭へseek
				file.seekcur( - (cnt - ljapa) );
				ljapa = fft.fieldlen - ljapa;
			}
		}
		word.set( all.wbuf );
		if ( japa ){
			_mchar buf[LJAPA+1];
			file.read( buf, sizeof(buf) );	// 日本語訳ロード
			if ( ljapa <= LJAPA )
				buf[ljapa] = '\0';
			japa->japa.set( buf );
		}
		return AS_FOUND;
	}
}
#endif	// !DIC_UTF8
#endif	// AllScan

// p : 訳語部の先頭アドレス（属性のあるアドレス）
// nextp : 次の見出語の先頭アドレス
int IndexData::AllSearchCommon( const uint8_t *p, const uint8_t *nextp, wa_t attr, AllSearchParam *_all)
{
	AllSearchParam &all = _all ? *_all : this->all;

	_mchar uc;
	switch ( all.mode2 ){
		case SRCH_MEMORY:	//記憶単語検索
			if ( attr & WA_MEMORY)
				return 1;
			return 0;
		case SRCH_JEDIT:
			if ( attr & WA_JEDIT )
				return 1;
			return 0;
		case SRCH_ALL:
			return 1;
		case SRCH_LEVEL:
			if ( ( ( attr & WA_LEVELMASK ) >= all.level_min ) && ( ( attr & WA_LEVELMASK ) <= all.level_max ) ){
				return 1;
			}
			return 0;
		case SRCH_Q:
			if ( p[_ATTRBYTE] == (uchar)SrchQ[0] ){
				return 1;
			}
			return 0;

		default:
			break;
	}
	int r;
#ifdef USE_REGEXP
	if ( all.re ){
		// 正規表現検索
		if ( all.mode2 & SRCH_WORD ){
			// 見出語部
			if ( all.CompareWord( all.srchwbuf+2 ) ){	// JRE,REXPではよけいな改行を必要としない
				return 1;
			}
		}
		if ( all.mode2 & SRCH_JAPA ){
			// 訳語部
			uc = *(_mchar*)nextp;
			*(_mchar*)nextp = '\0';
#ifdef USE_BOCU
			r = all.CompareBocu( (_mchar*)(p + _ATTRBYTE), LJAPA+1 );
#else	// !USE_BOCU
			r = all.re->Compare( (_mchar*)(p + _ATTRBYTE) );
#endif	// !USE_BOCU
			*(_mchar*)nextp = uc;
			if ( r ){
				return 1;
			}
		}
		if ( all.mode2 & ( SRCH_EXP | SRCH_TITLE | SRCH_PRON | SRCH_FILENAME | SRCH_FILECONT) ){
			goto jmp2;
		}
		return 0;
	}
#endif	// USE_REGEXP
	if ( all.mode2 & SRCH_WORD ){
		/* 見出語検索 */
		if ( all.CompareWord(all.wbuf) )
			return 1;
	}
	if ( all.mode2 & SRCH_JAPA ){
		/* 日本語訳検索 */
#ifdef USE_BOCU
		uc = *(_mchar*)nextp;
		*(_mchar*)nextp = '\0';
		r = all.CompareBocu( (_mchar*)(p + _ATTRBYTE), LJAPA+1 );
		*(_mchar*)nextp = uc;
#else
		const int len = (int)((_mchar*)nextp - (_mchar*)(p+_ATTRBYTE));
		r = all.strcmp_do( (_mchar*)(p+_ATTRBYTE), len )?1:0;
#endif
		if ( r ){
			return 1;
		}
	}
	// (正規表現も同時に可能)
	if ( all.mode2 & ( SRCH_EXP | SRCH_TITLE | SRCH_PRON | SRCH_FILENAME | SRCH_FILECONT) ){
#ifdef USE_REGEXP
jmp2:
#endif
		// 拡張
		if ( attr & WA_EX ){
			while ( *(*(_mchar**)&p)++ );	// skip japa

			while ( 1 ){
				uchar jt = *p++;
#ifdef NEWDIC4UNI
				p++;	// reserved
#endif
				const uint8_t *_p;
				uint jtblen;
				if ( all.GetDataBuf()->isField2() ){
					jtblen = *((t_jtb2*)p);
					_p = p + sizeof(t_jtb2);
				} else
				{
					jtblen = *((t_jtb*)p);
					_p = p + sizeof(t_jtb);
				}
#ifdef EPWING
				if ( jt == JT_EPWING ){
					//** 未対応
					p += sizeof(EPWingField);
					continue;
				} else
#endif
				if ( jt & (JT_EXP|JT_PRON) ){
					if (
							( ( all.mode2 & SRCH_EXP ) && ( jt & JT_EXP ) )
						|| 	( ( all.mode2 & SRCH_PRON ) && ( jt & JT_PRON ) )
						){
						// Note:
						// BOCU1とJT_COMPの同時利用は許されない
						if ( jt  & JT_COMP ){
							// 圧縮していた場合
#ifdef USE_COMP
							if (all.CompareCompress(_p, jtblen)){
								return 1;
							}
#endif	// USE_COMP
							goto jmp10;
						} else {
#ifdef USE_BOCU
							if ( all.CompareBocu( (_mchar*)p, max(LPRON+1,LEXP+1), (const _mchar**)&p ) )
								return 1;
#ifdef NEWDIC4UNI
							p = (uint8_t*)((((int)p)+2)&~1);
#endif
							goto jmp1;
#else	// !USE_BOCU
							if ( Compare( (_mchar*)p ) )
								return 1;
#endif	// !USE_BOCU
						}
					} else {
						// 検索対象語ではない
						if ( jt & (JT_COMP|JT_BYTE) )
							goto jmp10;
					}
					while ( *(*(_mchar**)&p)++ );	// skip text
#ifdef USE_BOCU
			jmp1:;
#endif
				} else
				if ( jt & JT_LINK ){
					if ( all.mode2 & SRCH_TITLE ){
						const _mchar *title;
						// Note: BOCU1とJT_COMPの同時利用は許されない
						if ( jt & JT_COMP ){
							// 圧縮していた場合
							if ( *(t_noc*)_p > sizeof(t_id) ){
								title = (const _mchar*)(_p + sizeof(t_noc) + sizeof(t_jlink) + sizeof(t_id));
								if ( all.Compare( (const _mtchar*)title ) )
									return 1;
							}
						} else {
							// 圧縮していない場合
							title = (const _mchar*)(_p + sizeof(t_jlink) + sizeof(t_id));
							if ( title[0] ){
								if ( all.CompareBocu( title, _BOCUtoWLEN(_mcslen(title)) ) )
									return 1;
							}
						}
					}
#ifdef USE_FILELINK
					if ( all.mode2 & SRCH_FILENAME ){
						if ( jt & JT_COMP ){
							// 圧縮していた場合
							// 未対応
						} else {
							// 圧縮していない場合
							if ( *(t_jlink*)_p == JL_FILE
#ifdef USE_ICONFILE
								|| *(t_jlink*)_p == JL_ICONFILE
#endif
							){
								const uint8_t *pTitle = _p + sizeof(t_jlink) + sizeof(t_id);
								while (*pTitle++);	// Skip title
								FileLinkField &jlf = *(FileLinkField*)pTitle;
								if ( all.CompareBocu( (_mchar*)jlf.GetDataPtr(), _BOCUtoWLEN(_mcslen((_mchar*)jlf.GetDataPtr())) ) ){
									return 1;
								}
							}
						}
					}
#if defined(USE_FILELINK)
					if ( all.mode2 & SRCH_FILECONT ){
						if (SearchFileLink(all, _p, jt, jtblen))
							return 1;
					}
#endif
#endif	// USE_FILELINK
					goto jmp10;
				} else
				if ( jt & JT_BYTE ){
			jmp10:
					p = _p + jtblen;
				} else if ( jt & JT_END ){
					break;
				} else {
					while ( *(*(_mchar**)&p)++ );	// 上記以外はテキストとする
				}
			}

		}
	}
	return 0;
}

#if defined(USE_FILELINK)
#include "JLFileSearch.h"
bool IndexData::SearchFileLink(AllSearchParam &all, const uint8_t *src, uchar jt, uint jtblen)
{
	return ::SearchFileLink((class Pdic*)this, all, src, jt, jtblen);
}
#endif

#if defined(_UNICODE) && defined(_MSC_VER) && !defined(DIC_UTF8) && !defined(DIC_UTF16)
// WINCEでは漢字を扱う関数がない？？？
int _mbsnicmp( const char *str1, const char *str2, unsigned len )
{
	char c,d;
	for(;len>0;){
		c = *str1++;
		d = *str2++;
		if ( _ismbblead(c) ){
			c -= d;
			if ( c != 0 )
				return c;
			if ( d == '\0' )
				return 0;	// 完全一致（漢字は不正）
			d = *str2++;
			c = *str1++ - d;
			if ( c != 0 )
				return c;
			if ( d == '\0' )
				return 0;	// 完全一致
			len -= 2;
		} else {
			if ( c >= 'a' && c <= 'z' ){
				c -= 0x20;
			}
			if ( d >= 'a' && d <= 'z' ){
				d -= 0x20;
			}
			c -= d;
			if ( c != 0 )
				return c;
			if ( d == '\0' )
				return 0;	// 完全一致
			len--;
		}
	}
	return 0;	// 部分一致
}
#endif
//1 : 見つかった
//0 : 検索中
//-1:エラー
//-2:全て終了
int IndexData::PrevAllSearch_k( _MChar &word, Japa *japa, AllSearchParam *_all)
{
	AllSearchParam &all = _all ? *_all : this->all;

	if (all.generation!=generation){
		// jumpAS()をしないでいきなりこの関数を呼ぶことはできない
		error = DICERR_GEN_CHANGED;
		return AS_ERROR;
	}

	TDataBuf &databuf = *all.GetDataBuf();

#if !defined(DOS) && !defined(NOFIELD2)
	bool fField2 = databuf.isField2();
#endif
	if ( all.loc <= sizeof(t_blknum) || all.loc == -1 ){
		all.lbn--;
		if (all.lbn < 0 ){
			all.lbn = -1;
			all.loc = -1;	// 次のnextAllSearchのため
			return -2;		/* すべての検索終了 */
		}

		if ( data->read(index->indexBlock(all.lbn), &databuf, false) == -1){
			return -1;
		}
		const FieldFormat *all_cp;
		fField2 = databuf.isField2();
		if ( fField2 ){
			index->indexWord( all.wbuf, all.lbn );
			all_cp = (FieldFormat*)NextField2( FF2(databuf.getDataBuf()+data->GetTopLoc()) );
		} else
		{
			all_cp = NextField( (FieldFormat*)all.srchTail( ) );
		}
		all.loc = FP_DIFF( (char*)all_cp, databuf.getDataBuf() );
	}
	const _mchar *nextp;
	const _mchar *p;
	wa_t attr;
	for (;;) {
		if ( all.loc <= sizeof(t_blknum) ){
			return AS_CONTINUE;
		}
		int nextloc = all.loc;
		all.prevWord();
		if ( fField2 ){
			p = ((const FieldFormat2*)( databuf.getDataBuf() + all.loc ))->word;
			attr = ((const FieldFormat2*)( databuf.getDataBuf() + all.loc ))->attr;
			while ( *p ) p++; p++;	// skip word
		} else
		{
			p = ((const FieldFormat*)( databuf.getDataBuf() + all.loc ))->word;
			attr = ((const FieldFormat*)( databuf.getDataBuf() + all.loc ))->attr;
			while ( *p++ ) ;	// skip word
		}
		nextp = (_mchar*)(databuf.getDataBuf() + nextloc);
		if ( all.fasthead ){
			if ( index->cmpnword( all.wbuf, all.word_k, all.len_k ) ){
				return AS_END;
			}
			if (all.mode & SRCH_REGEXP){
				// 正規表現が有効な場合はその条件が一致した場合のみ
				if ( AllSearchCommon( (const uint8_t*)p, (const uint8_t*)nextp, attr, &all) )
					break;
			} else {
				break;
			}
		} else {
			if ( AllSearchCommon( (const uint8_t*)p, (const uint8_t*)nextp, attr, &all) )
				break;
		}
	}

	//全検索 or 一致したもの
	word.set( all.wbuf );
	if ( japa ){
		japa->_SetAll( (const uint8_t*)p, FP_DIFF( nextp, p ), this, fField2,
			SEL_FIELD2( fField2, ((const FieldFormat2*)( databuf.getDataBuf() + all.loc ))->attr, ((const FieldFormat*)( databuf.getDataBuf() + all.loc ))->attr )
		);
	}
	return 1;
}

// 現在のポインタが最初と最後にある場合は無効
int IndexData::CurAllSearch( _MChar &word, Japa *japa, int, AllSearchParam *_all )
{
	AllSearchParam &all = _all ? *_all : this->all;

	word.set( all.wbuf );
	if ( all.loc == -1 || all.loc <= sizeof(t_blknum) ){
		if ( japa ){
#if 0	// 何のためにやっているのだろう？？？
			for ( int i=0;i<GetAttrLen();i++ ){
				(*japa)[i] = WA_NORMAL;
			}
			(*japa)[i] = '\0';
#endif
			japa->clear();
		}
		return 0;
	}

	TDataBuf &databuf = *all.GetDataBuf();
	
	const FieldFormat*all_cp = (FieldFormat*)( databuf.getDataBuf() + all.loc );
	const _mchar *p;
	if ( databuf.isField2() ){
		p = ((FieldFormat2*)all_cp)->word;
		while ( *p ) p++; p++;	// skip word
		if ( japa ){
#ifdef DOS
			japa->_SetAll( (uint8_t*)p, 0, this, true, ((FieldFormat2*)all_cp)->attr );
#else
			japa->_SetAll( (uint8_t*)p, FP_DIFF( NextField2( (FieldFormat2*)all_cp ), p ), this, true, ((FieldFormat2*)all_cp)->attr );
#endif
		}
	} else
	{
		p = all_cp->word;
		while ( *p++ ) ;
		if ( japa ){
			japa->_SetAll( (uint8_t*)p, FP_DIFF( NextField( all_cp ), p ), this, false, all_cp->attr );
		}
	}
	return 0;
}

#if MIXDIC
int IndexData::JumpAS_( const tchar *word, bool fBack, bool fSameBack, AllSearchParam *all )
	{ return JumpAS_k( (const _mchar*)__kwstr(word), fBack, fSameBack, all ); }
#endif

// fSameBack : jump先が同じ単語であった場合、もう一つ前へ移動する(前方検索時のみ有効）
int IndexData::JumpAS_k( const _mchar *word, bool fBack, bool fSameBack, AllSearchParam *_all )
{
	AllSearchParam &all = _all ? *_all : this->all;
	if ( !word[0] ){
		// 単語が無い場合 1995.9.22
		// ここには来ないようにプログラムを組んだ方が良い
		if ( fBack ){
			all.lbn = index->GetIndexNum();
			all.loc = -1;
			all.wbuf[0] = '\0';
		} else {
			all.lbn = -1;
			all.loc = -1;
			all.wbuf[0] = '\0';
		}
		all.generation = generation;
		return 0;
	}
	if ( _BSearch( word, false, _all) == -1 ){
		return -1;
	}
	if (_all){	// tricky
		all.lbn = all.fw.lbn;
		all.loc = all.fw.lp;
		_kcscpy( all.wbuf, all.fw.fword );
	} else {
		all.lbn = fw.lbn;
		all.loc = fw.lp;
		_kcscpy( all.wbuf, fw.fword );
	}

	// all.lbn,all_locのセットのしかた
	// ※wordの次の単語を探すとする
	// ※all_locが見つかった単語の位置を示すため、ちょっと面倒
	// 前方検索の場合
	//	見出語が一致した場合(word==all.wbuf)
	//		次にヒットすべき単語はall_wbufより後の単語なので、そのまま。
	//	見出語が一致しない場合(word!=all.wbuf)
	//		必ずword < all_wbufなので次にヒットすべき単語はall_wbuf以降の単語である。
	//		従って、PrevWord()で戻る必要がある。
	// 後方検索の場合
	//	見出語が一致した場合(word==all.wbuf)
	//		次にヒットすべき単語はall_wbufより前の単語なので、そのまま。
	//	見出語が一致しない場合(word!=all.wbuf)
	//		必ずword < all_wbufなので次にヒットすべき単語はall_wbufより前の単語。
	//		従って、何もしなくて良い。

	// ※PrevWord()のとき、all_locが先頭であるかどうかのチェックが必要！

	if ( !fBack ){
		if ( fSameBack || _kcscmp( all.wbuf, word ) ){
			PrevWord(_all);
		}
	} else {
		// None.
	}
	all.generation = generation;

	return 0;
}

#ifndef DOS
// lbnブロックへジャンプ
int IndexData::DirectJump( int lbn, AllSearchParam *_all )
{
	AllSearchParam &all = _all ? *_all : this->all;

	if ( (uint)lbn >= (uint)index->GetIndexNum() ){
		return 0;	// 失敗
	}
	all.lbn = lbn-1;
	all.loc = -1;	// 1996.3.8 0 だったため、次のnextAllSearchでうまく動作しなかった

	return 1;	// 成功
}
#endif	// DOS

// 全検索のカレントポインターを１つ前へ
// 前方検索のみ有効（？）
void IndexData::PrevWord(AllSearchParam *_all)
{
	AllSearchParam &all = _all ? *_all : this->all;

	if ( all.loc > sizeof(t_blknum) ){
		all.prevWord( );
	} else {
		all.lbn--;
		all.loc = -1;
	}
}

/////////////////////////////////////////////////////
// read/update/erase
/////////////////////////////////////////////////////
//現在のカーソル位置へ単語の追加
//必ずbsearch済みである事！
/*	word, japa は追加する単語
	databufを利用
	FINDWORD.word1, word2が必要！！
	容量不足はこの関数で起こる可能性が多きい、必ず返り値をチェックすること */
// wordlen : charactor count of word
int IndexData::_record( const _mchar * word, uint wordlen, const Japa &japa )
{
	DD( DD_RECORD, 0, 0, word );
	if ( wordlen == 0 )
		wordlen = _mcslen( word );
	uint japalen;
	uint8_t *buf = japa.Get2( japalen, compflag, GetLimitSize( ) - _MLENTOBYTE(wordlen), this );
	if ( !buf ){
		error = DICERR_TOOLONG;	// 登録データが長すぎる（またはメモリ不足）
		return -1;
	}

	TDataBuf &databuf = *data->GetDataBuf();
	if ( japalen & 0x80000000 || japalen>=MIN_FIELD2_JLEN){
		// Field2 //
		japalen &= ~0x80000000;

		if ( index->getRemain() < sizeof(INDEX) + _MLENTOBYTE(LWORD) + sizeof(int)*2 ){
			// あらかじめ確保しておく
			if ( CreateNewIndex( NEWINDEX_ALLOC_SIZE )  ){
				return -1;
			}
		}

		uint newsize = sizeof(t_blknum) + L_FieldHeader2 + _MLENTOBYTE(wordlen+1) + japalen + sizeof(tfield2);
		t_blknum newblknum = data->CalcBlockNum( newsize );
		int lbn = fw.lbn;
		if ( lbn!=-1 && fw.lp > sizeof(t_blknum) ){
			if ( databuf.GetDataPtr( fw.lp )->fieldlen ){
				// ブロックの最後でない場合(分割処理)
				generation++;
				if ( HPDICDATA(data)->DivBlock( word, buf, japalen, fw, TRUE, NULL, index ) ){
					return -1;
				}
			}
			lbn++;
		}
		if ( lbn < 0 ) lbn = 0;
		if ( (fw.pbn = data->newBlock( newblknum | FIELDTYPE )) == BLK_ERROR){
			return -1;		//ディスク容量不足
		}
		if ( AddIndex( lbn, fw.pbn, word ) ){
			data->eraseBlock( fw.pbn );
			return -1;
		}
		HPDICDATA(data)->addWord2( word, wordlen, buf, japalen, fw.pbn );	// 必ず成功する
		return 0;
	}

	int first = 0;
	if (fw.lbn == -1){
		if ( (fw.pbn = data->newBlock( 1 )) == BLK_ERROR){
			return -1;		//ディスク容量不足
		}
		if ( AddIndex(0, fw.pbn, word) ){
			data->eraseBlock( fw.pbn );
			return -1;		//容量不足エラー
		}
		fw.lbn = 0;
		first = 1;
		generation++;
	}
	// Field2にField1データを登録する場合(=新規block)
	else if ( databuf.isField2() ){
		if ( fw.lp <= sizeof(t_blknum) ){
		} else {
			fw.lp = 0;
			fw.lbn++;
		}
		// 先頭に追加
		if ( (fw.pbn = data->newBlock( 1 )) == BLK_ERROR){
			return -1;		//ディスク容量不足
		}
		if ( AddIndex(fw.lbn, fw.pbn, word) ){
			data->eraseBlock( fw.pbn );
			return -1;		//容量不足エラー
		}
	}

	DivInfo divinfo;

	int r = data->addWord( word, wordlen, buf, japalen, fw, &divinfo );
	if ( r == -1 ){
		return -1;
	} else if ( r == 0 || r == -3 ){
		// ブロックの先頭に追加？
		if ( r == -3 ){
			// 物理ブロック番号変更
			index->setIndexBlock( fw.lbn, fw.pbn );
		}
		if ( fw.lp <= sizeof(t_blknum) && !first ){
			if ( index->renIndex( fw.lbn, word ) ){
				if ( index->GetErrorCode() == 3 )
				{
					if ( !CreateNewIndex( 2 ) && !index->renIndex( fw.lbn, word ) ){
						goto success1;
					}
				}
				data->eraseWord( fw.pbn, 0 );	// 先頭の単語を削除
				data->write( fw.pbn );
				return -1;
			}
		}
	success1:;

	} else
	if ( r == -2 ){
		if ( index->getRemain() < (sizeof(INDEX) + _MLENTOBYTE(LWORD) + sizeof(int)*2)*2 ){
			// あらかじめ確保しておく
			// DivBlockでaddIndexとrenIndexが両方行われるとoverする可能性があるため、
			// やや余計にindex領域は２単語分の追加として見積もる
			if ( CreateNewIndex( NEWINDEX_ALLOC_SIZE )  ){
				return -1;
			}
		}
		if (((HPdicData*)data)->DivBlock( word, buf, japalen, fw, 1, &divinfo, index )!=0){
			return -1;
		}
#if 1
//		if ( data->IsFastMode() )
		{
#ifndef SMALL
			// 大きいデータのことを考え、たまに一気にやるより、常に少しずつやったほうが良い
			VeryEasyOptimize(data->GetNumWord()%10==0 ? 5:1);
#endif
		}
#endif
	}

	generation++;

	error = 0;

//	DBOFF();
	return 0;
}
#if !NOUPDATEWORD
int IndexData::_update( const _mchar * word, uint wordlen, const Japa &japa )
{
#if DEBUG_DIC
	DBDIC( "UPDATE:word=%s", word );
#endif
	if ( wordlen == 0 )
		wordlen = _mcslen( word );
	uint japalen;
	uint8_t *buf = japa.Get2( japalen, compflag, GetLimitSize( ) - _MLENTOBYTE(wordlen), this );
	if ( !buf ){
		error = DICERR_TOOLONG;	// 登録データが長すぎる(またはメモリ不足)
		return -1;
	}
	if (fw.lbn == -1){
		error = DICERR_PARAMETER;
		return -1;
	}

	TDataBuf &databuf = *data->GetDataBuf();
	if ( japalen & 0x80000000 || japalen>=MIN_FIELD2_JLEN){
		// Field2 //
		HPdicData *hdata = HPDICDATA(data);
		japalen &= ~0x80000000;

		if ( index->getRemain() < sizeof(INDEX) + _MLENTOBYTE(LWORD) + sizeof(int)*2 ){
			// あらかじめ確保しておく
			if ( CreateNewIndex( 2 )  ){
				return -1;
			}
		}

		generation++;
		uint newsize = sizeof(t_blknum) + L_FieldHeader2 + wordlen + 1 + japalen + sizeof(tfield2);
		t_blknum newblknum = (t_blknum)(data->CalcBlockNum( newsize ) | FIELDTYPE);
		int lbn = fw.lbn;
		t_pbn2 delpbn;	// 削除する単語のあるpbn
		int delloc;
		int dellbn;
		bool fDelField2 = databuf.isField2();
		t_blknum delblknum = databuf.getBlockNum();
		if ( fw.lp > sizeof(t_blknum) ){
			// 最初でない場合(Field1)
			if ( NextField(databuf.GetDataPtr( fw.lp ))->fieldlen ){
				// 途中の場合
				if ( hdata->DivBlock( word, buf, japalen, fw, TRUE, NULL, index ) ){
					return -1;
				}
				lbn++;
				dellbn = lbn+1;
				delpbn = index->indexBlock(lbn);
				delloc = 0;
			} else {
				// 最後の場合
				delpbn = fw.pbn;
				delloc = fw.lp;
				dellbn = lbn;
				lbn++;
			}
		} else {
			// 最初 or Field2へ上書き
			delpbn = fw.pbn;
			delloc = fw.lp;
			dellbn = fw.lbn+1;
		}
		// 新規ブロック追加 or Shrink
		t_pbn2 newpbn;
		if ( !fDelField2 ){
			// update先がField1
			if ( (newpbn = hdata->newBlock( newblknum )) == BLK_ERROR ){
				return -1;
			}
		} else if ( HBLKNUM(newblknum) > delblknum ){
			// update先が拡大する場合
			if ( (newpbn=hdata->GrowBlock( fw.pbn, delblknum, newblknum, 1 )) == BLK_ERROR ){
				return -1;
			}
		} else {
			newpbn = fw.pbn;
			if ( delblknum > HBLKNUM(newblknum) ){
				// Shrink
				hdata->ShrinkBlock( fw.pbn, HBLKNUM(newblknum) );
			}
		}
		// インデックス追加
		// 単語書き込み
		// 単語数はaddWord2とeraseでつじつまが合う(^^;
		hdata->addWord2( word, wordlen, buf, japalen, newpbn );
		// 単語削除
		if ( fDelField2 ){
			data->DecNWord();
			index->setIndexBlock( fw.lbn, newpbn );
		} else {
			AddIndex( lbn, newpbn, word );	// エラーは発生しないはず
			fw.pbn = delpbn;
			fw.lp = delloc;
			fw.lbn = dellbn;
			data->read( fw.pbn );
			_erase();
		}
		return 0;
	} else {
		// Field2への書き込みかどうかをチェックする
		if ( databuf.isField2() ){
			// Field2へField1の書き込み
			// overheadが大きいが・・・
			if ( _erase() ) return -1;
			_BSearch( word );
			return _record( word, wordlen, japa );
		}
	}

	DivInfo divinfo;

	int r = data->updateWord( buf, japalen, fw, &divinfo );
	if ( r == -1 ){
		return -1;
	}
	if  ( r == -2 ){
		if ( index->getRemain() < sizeof(INDEX) + _MLENTOBYTE(LWORD) + sizeof(int)*2 ){
			// あらかじめ確保しておく
			if ( CreateNewIndex( 2 )  ){
				return -1;
			}
		}
		generation++;
		((HPdicData*)data)->DivBlock( word, buf, japalen, fw, 0, &divinfo, index );
	}
	else if ( r == -3 ){
		// 物理ブロック番号変更
		index->setIndexBlock( fw.lbn, fw.pbn );
	}

	generation++;

	error = 0;

	return 0;
}
#endif	// !NOUPDATEWORD

/////////////////////////////////////////////////////
// Block operations
/////////////////////////////////////////////////////

// ここのnumの値で変換の速さが非常に変わる
int IndexData::CreateNewIndex( int num )
{
	t_blknum blknum = GrowIndex( num, false );
	if (blknum==(t_blknum)-1)
		return -1;
	fw.pbn-=blknum;

	if ( blknum > 0 ){
		num -= blknum;	// 追加できていないblock数を求める
		if ( num > 0 ){
			return CreateNewIndex( num );
		}
	}
	return 0;
}

// Grow Index //
// indexの領域を最大numブロック分だけ大きくする
// extheaderの拡張時にも使用する(move=true)
// return : 実際に解放されたブロック数（<numのときもある？）
t_blknum IndexData::GrowIndex( int num, bool move )
{
	if (!move){
		if (!index->CanGrowIndex(num)){
			error = DICERR_INDEXFULL;
			return -1;	// index area is full.
		}
	}
	t_blknum blknum;
	// 先頭の物理ブロックを探す
	if ( data->GetNBlock() == 0 ){
		// 未使用
		if ( data->newBlock( num ) == BLK_ERROR ) return -1;
		blknum = num;
	} else {
		int lbn = index->searchLbn( 0 );
		if ( lbn == -1 ){
			// 先頭は空きブロックであると判断
			data->OpenEmptyList( );
			if ( data->emptylist->Free( 0 ) == -1 ){
				// 追加途中のときであると判断
				goto j1;
			}
			blknum = 1;
		} else {
	j1:;
			// データを移動する
			blknum = num;
			t_pbn2 dstpbn = data->MoveBlock( 0, blknum );
			if ( dstpbn == BLK_ERROR ) return -1;
			if ( blknum > num ) blknum = num;	// ブロック数はnum個に制限
			if ( fw.pbn == 0 ) fw.pbn = dstpbn;
			if ( lbn != -1 ){
				index->setIndexBlock( lbn, dstpbn );
			}
		}
	}
	generation++;
	// 増やした部分を0にする
	data->ClearBlock( 0, blknum );
	data->ChangeOffset( blknum );
	if (move){
		index->MoveIndex( blknum );
	} else {
		index->GrowIndex( blknum );
	}
	return blknum;
}

// Shrink Index //
// ある条件を満足したら、shrinkを行う
void IndexData::ShrinkIndex( )
{
	if ( !data->emptylist ) return;	// emptylistがopenされていない場合は実行しない?
	// 現在のアルゴリズム //
	// ブロックサイズの16個以上の空き？
	// ブロックサイズ8個分解放
	if ( index->getRemain() >= data->GetUnitBlockSize() * 16 ){
		index->GrowIndex( -8 );
		data->ChangeOffset( -8 );
		data->emptylist->Register( 0, 8 );
		fw.pbn += 8;
	}
	// 最終ブロックが空きブロックである場合はファイルサイズを縮小する
	int seqblknum;
	t_pbn2 pbn = data->emptylist->NewBlock( 0x7fff, &seqblknum );
	if ( pbn != BLK_ERROR ){
		if ( pbn + seqblknum == data->GetNBlock() ){
			// ファイルサイズ縮小
			data->SetNBlock( pbn );
			data->Chsize();
		} else {
			// 空きブロック再登録
			data->emptylist->Register( pbn, seqblknum );
		}
	}
}

//現在のカーソル位置の単語を削除
int IndexData::_erase()
{
	//データブロックを読み込む
	//現在は無し

	TDataBuf &databuf = *data->GetDataBuf();
	if ( databuf.isField2() ){
		if ( HPDICDATA(data)->eraseBlock( fw.pbn ) ) return -1;
		index->delIndex( fw.lbn );
		data->DecNWord();
		return 0;
	}

	//データブロックから見出語の削除
#if !defined(SMALL)
	int r =
#endif
	data->eraseWord( fw.pbn, fw.lp );

	if ( databuf.isEmpty() ){
		/* 空きブロックの登録 */
		t_pbn2 pbn = index->indexBlock(fw.lbn);
		index->delIndex(fw.lbn);
		data->eraseBlock( pbn );
#if !defined(SMALL)
		VeryEasyOptimize();	// 1997.5.10(Ver.3.11)
#endif
		generation++;
		return 0;
	} else
	if (fw.lp <= sizeof(t_blknum)){		//ブロックの先頭のときはindexも更新
		const _mchar *topword;
		topword = (_mchar*)(data->getDataBuf() + sizeof(t_blknum) + data->GetDataBuf()->GetFieldHeaderSize());
		if ( index->renIndex(fw.lbn, topword) == -1){
			if ( index->GetErrorCode() == 3 )
			{
				if ( CreateNewIndex( 1 ) == -1 ){
					return -1;
				}
				if ( index->renIndex( fw.lbn, topword ) == -1 ){
					return -1;
				}
			} else
				return -1;
		}
	}

	if ( data->write(fw.pbn) == -1)
		return -1;
#if !defined(SMALL)
	if ( r == 1 ){
		if ( VeryEasyOptimize() > 0 )	// 1997.5.10(Ver.3.11)
			data->read(fw.pbn);	// Ver.3.13
	}
#endif

	generation++;

	return 0;
}

// CreateNewIndexありのindex追加処理
int IndexData::AddIndex( int lbn, t_pbn2 pbn, const _mchar *word )
{
	if ( index->addIndex(lbn, pbn, word) == -1){
		if ( index->GetErrorCode() == DICERR_INDEXFULL )
		{
			if ( !CreateNewIndex( NEWINDEX_ALLOC_SIZE ) && !index->addIndex( lbn, pbn, word ) ){
				return 0;
			}
		}
		return -1;
	}
	return 0;
}

#if 0
static HWND hWin = NULL;
static const char *clsname = "TDbgMsgForm";
static const char *winname = "Debug Messanger";
void DBON( )
{
	if ( !hWin ){
		hWin = FindWindow( clsname, winname );
		if ( !hWin ) return;
	}
	COPYDATASTRUCT cds;
	cds.dwData = 0x10;	// Indicate String
	cds.cbData = 0;
	cds.lpData = NULL;
	SendMessage( hWin, WM_COPYDATA, NULL, (LPARAM)&cds );
}
void DBOFF( )
{
	if ( !hWin ){
		hWin = FindWindow( clsname, winname );
		if ( !hWin ) return;
	}
	COPYDATASTRUCT cds;
	cds.dwData = 0x11;	// Indicate String
	cds.cbData = 0;
	cds.lpData = NULL;
	SendMessage( hWin, WM_COPYDATA, NULL, (LPARAM)&cds );
}

#endif

#ifndef DOS
// JLinkのヘッダー部(非圧縮部)のみを直接更新
// サイズが変わる更新、圧縮部にある(または圧縮されうる)データは更新できない
// ファイル上の構造は、
//		t_exattr:
//		t_jtb2:
//		t_noc nocomplen:(圧縮してある場合)
//		t_jlink:
//		t_id:
//		title:
//		short headersize: = sizeof(xxxxField) << 重要！
// となっていること
// offset : headersize からのオフセット値
// size   : offsetから更新するサイズ
// offset + size <= headersize,
// offset + size + sizeof(t_jlink) + sizeof(t_id) + _tcslen(title) + sizeof(short) <= nocomplen であること！！
// 戻り値：
//		0 : offset, sizeが不正であるため更新できず,単語は登録されていない、オブジェクトが見つからない
//		1 : 更新できた
//		-1: エラー発生
int IndexData::UpdateObjectHeader( const _mchar *word, t_id id, int offset, int size, const uint8_t *udata )
{
	if (_BSearch(word) == -1) return -1;

	if ( all.lbn != fw.lbn ){
		generation++;
	}
	if ( _mcscmp( word, fw.fword ) )
		return 0;

	TDataBuf &databuf = *data->GetDataBuf();
	uint8_t *p = (uint8_t*)databuf.GetDataPtr( fw.lp ) + databuf.GetFieldHeaderSize();
	while ( *(_mchar*)p ) p += sizeof(_mchar); p += sizeof(_mchar);	// skip word

	if ( !(*p & WA_EX) ) return 0;
	while ( *(_mchar*)p++ ) ;	// 日本語訳
	int jtbsize;
	if ( databuf.isField2() ){
		jtbsize = sizeof(t_jtb2);
	} else {
		jtbsize = sizeof(t_jtb);
	}

	// IDを探す！！
	while ( 1 ){
		t_exattr jt = *p++;
		if ( jt == JT_END ) return 0;
		uint8_t *next = p;
		uint jtb;
		if ( jtbsize == sizeof(t_jtb2) ){
			jtb = *(*(t_jtb2**)&p)++;
		} else
		{
			jtb = *(*(t_jtb**)&p)++;
		}
		if ( jt & JT_BYTE ){
			next = p + jtb;
		}
		t_noc nocomplen;
		if ( jt & JT_COMP ){
			nocomplen = *(t_noc*)p;
			p += sizeof(t_noc) + sizeof(t_jlink);
		} else {
			p += sizeof(t_jlink);
		}
		if ( *(t_id*)p == id ){
			// 発見！！
			p += sizeof(t_id);
			int l = _mcsbyte1((_mchar*)p);	// title
			if ( jt & JT_COMP ){
				if ( offset + size + sizeof(t_jlink) + sizeof(t_id) + l + sizeof(short) > nocomplen ){
					return 0;
				}
			}
			p += l;
			if ( *(ushort*)p < offset + size ) return 0;
			memcpy( p + offset, udata, size );	// Copy new data
			break;
		}
		if ( jt & JT_BYTE ){
			p = next;
		} else {
			while ( *next++ ) ;
		}
	}

	if ( data->write(fw.pbn) == -1)
		return -1;
	return 1;

}
#endif	// DOS

#if 1
// 全検索時の削除( nextAllSearch() の場合のみ使用可能　）
int IndexData::EraseWordAS(AllSearchParam *_all)
{
	AllSearchParam &all = _all ? *_all : this->all;

	fw.lbn = all.lbn;
	fw.pbn = index->indexBlock( all.lbn );
	fw.lp = all.loc;
	_mcscpy( fw.fword, all.wbuf );
	if ( _erase() == -1 ){
		return -1;
	}

	if ( all.loc <= sizeof(t_blknum) || fw.pbn != index->indexBlock(fw.lbn) ){	//ブロックが削除された場合、ブロックの先頭の場合
		all.loc = -1;
		all.lbn--;
	} else {
		all.prevWord();						// 1つ前へ
	}
	return 0;
}
#endif

// Multithread Search //
int IndexData::NextAllSearchMT( _KChar &word, Japa *japa, AllSearchParam *all)
{
	return CommonAllSearchMT(word, japa, all, true);
}
int IndexData::PrevAllSearchMT( _KChar &word, Japa *japa, AllSearchParam *all)
{
	return CommonAllSearchMT(word, japa, all, false);
}
void IndexData::StopAllSearchMT(bool wait)
{
#if 0	//TODO: どうやってとめる？
	// 現状、検索に使用したAllSearchParam.Stop(true)で止める必要がある
	ThreadEnd = true;
	SetEvent(ThreadEvent);
	if (wait){
		while (!ThreadDone){
			Sleep(10);
		}
	}
#endif
}
void IndexData::SearchMTNext()
{
#if 0
	__assert(ThreadStatus==AS_CONTINUE || ThreadStatus==AS_FOUND);
	ThreadStatus = AS_RUNNING;
	SetEvent(ThreadEvent);
#endif
}
int IndexData::CommonAllSearchMT( _KChar &word, Japa *japa, AllSearchParam *all, bool forward)
{
	__assert(all!=NULL);

	if (!all->StartThread(*this, forward)){
		error = DICERR_NOMEMORY;
		return AS_ERROR;
	}

	return all->GetSearchResult(word, japa);
}
// End of Multithread Search //

#ifndef DOS
int IndexData::GetPercent(AllSearchParam *_all)
{
	if (index->GetIndexNum()==0)
		return 0;

	AllSearchParam &all = _all ? *_all : this->all;

	if (!IndexBlocks){
		// default available
		SetPercentExact();
	}
	
	if (IndexBlocks){
		// 累積block数によるpercentage
		int num = IndexBlocks->get_num();
		if (num>0 && (*IndexBlocks)[0]>0){
			return (*IndexBlocks)[all.lbn]*100/(*IndexBlocks)[num-1];
		}
	}
	
	return (int)((int)all.lbn*100/index->GetIndexNum());
}

bool IndexData::SetPercentExact(bool strict)
{
	if (!index)
		return false;	// programming error?
	if (IndexBlocks){
		IndexBlocks->clear();
	} else {
		IndexBlocks = new FlexArray<int>;
	}
	// for fast operation
	const int MaxStrictBlocks = 500;	// これ以上は間引く
	const int SkipBlocks = 8;			// 間引くblock数
	if (index->GetIndexNum()<MaxStrictBlocks){
		strict = true;
	}
	int total = 0;
	int prev_num = 0;
	for (int i=0;i<index->GetIndexNum();i++){
		int num;
		if (!strict && (i%SkipBlocks != 0)){
			num = prev_num;
		} else {
			t_pbn2 pbn = index->IndexBlockNo(i);
			num = data->ReadBlockNum(pbn);
			if (num==BLK_ERROR){
				delete IndexBlocks;
				IndexBlocks = NULL;
				return false;	// error??
			}
			prev_num = num;
		}
		total += num;
		IndexBlocks->add(total);
	}
	return true;
}

// 見出語のみの変更
// Field2のときのみ意味がある
// oldwordは存在していないとerror(programming error)
// newwordは存在していると辞書破壊！！
int IndexData::_Rename( const _kchar *oldword, const _kchar *newword, int newwordlen )
{
	if (!newwordlen)
		newwordlen = _kcslen(newword);
	if ( _BSearch( oldword )<=0 )	// 2009.6.6 fixed
		return -1;
	Japa japa;
	getfjapa( japa );
	if ( _erase( ) == -1 ) return -1;
	_BSearch( newword );
	return _record( newword, newwordlen, japa );
}

#ifndef SMALL
// Easy Optimization //
// Applicationによって、removalmediaは禁止できるようにした方が良い
// 継続処理を行うための引数
// _ipbn : 論理ブロック番号
// _pbn  : 最適化後の物理ブロック番号
int IndexData::EasyOptimize( t_pbn2 *_ipbn, t_pbn2 *_pbn )
{
	int nindex = index->GetIndexNum();

	if ( nindex == 0 ){
		return 0;
	}

	if (!data->Flush()){
		return -1;
	}

	// 不一致になる lbn -> pbn relationを探す //
	data->OpenEmptyList( );
	EmptyList &el = *data->emptylist;
	el.Sort();

	t_pbn2 pbn = 0;
	t_pbn2 ipbn = 0;
	if ( _ipbn ) ipbn = *_ipbn;
	if ( _pbn ) pbn = *_pbn;

	t_blknum blknum;
	TempBuff tb;

	int ntemp = 0;

	int modnum = 0;	// 最適化回数

	while ( ipbn < nindex ){
		t_pbn2 pi = index->indexBlock( ipbn );
		if ( pbn == pi ){
			// 一致している
			if ( data->file_read( pbn, &blknum, sizeof(blknum) ) < sizeof(blknum) ) return -1;
			blknum &= ~FIELDTYPE;
			pbn += blknum;
			ipbn++;
			if ( _ipbn ){
				*_ipbn = ipbn;
				*_pbn = pbn;
			}
			continue;
		}

		// piをpbnへコピーするだけの領域を確保する
		t_pbn2 __pbn = pbn;

		// ブロック数を得る場合、テンポラリにある場合もある
		t_blknum pbnnum = tb.getblknum( ipbn );
		if ( pbnnum == 0 ){
			data->file_read( pi, &pbnnum, sizeof(pbnnum) );
			pbnnum &= ~FIELDTYPE;
		}

		while ( 1 ){

			t_pbn2 i = ipbn;
			// 現在の物理ブロック番号を指しているインデックスを探す
			for ( ;i<nindex;i++){
				if ( index->indexBlock(i) == __pbn)
					break;
			}
			if ( i < nindex ){
				// 空ブロックでない場合はテンポラリバッファまたは他の空ブロックへ
				data->read( index->indexBlock(i), NULL, false );
				t_blknum blknum;
				blknum = (t_blknum)((*(t_blknum*)data->getDataBuf()) & ~FIELDTYPE);
				if ( !tb.save( data->getDataBuf(), i, data->GetUnitBlockSize() *  blknum ) ){
					//テンポラリバッファが一杯になったので、
					//ひとまず空きブロックへ全て掃き出す
					//pbn++をしない！！
					if ( SaveAllTempBuff( ntemp, tb, el, &__pbn ) == -1 ) return -1;
					continue;
				} else {
					el.Register( __pbn, blknum );
				}
				__pbn += blknum;
			} else {
				// このブロックは空ブロックとみなす
				__pbn++;
			}
			if ( __pbn >= pbn + pbnnum )
				break;
		}

		uint8_t *buf;
		t_pbn2 loadlbn;
		t_blknum loadnum;
		if ( ( buf = tb.load( ipbn, loadlbn ) ) == NULL ){	//テンポラリバッファに無い場合は
			if (data->read(pi, NULL, false) == -1) return -1;
			el.Register( pi, (t_blknum)(*(t_blknum*)data->getDataBuf() & ~FIELDTYPE) );
			loadlbn = pbn;
			loadnum = (t_blknum)(*(t_blknum*)data->getDataBuf() & ~FIELDTYPE);
		} else {
			t_blknum n;
			n = (t_blknum)(*(t_blknum*)buf & ~FIELDTYPE);
			data->SetDataBuf( pbn, buf, n );
			delete[] buf;
			loadlbn = pbn;
			loadnum = n;
		}
		if (data->write(loadlbn) == -1) return -1;
		if (!data->Flush()) return -1;
		el.Free( loadlbn, loadnum );	// 空ブロックであるかどうかを確認していない（けど良いだろう）
		index->setIndexBlock( ipbn, loadlbn );
		pbn += loadnum;
		ipbn++;
		modnum++;
		if ( _ipbn ){
			*_ipbn = ipbn;
			*_pbn = pbn;
		}
		break;
	}
	if (tb.isExist()){
		if ( SaveAllTempBuff( ntemp, tb, el, NULL ) ) return -1;
	}
	if ( modnum ){
		return 1;
	}
	return 0;
}

int IndexData::VeryEasyOptimize(int max_mod)
{
	int nindex = index->GetIndexNum();

	if ( nindex == 0 ){
		return 0;
	}

	if (!data->Flush()) return -1;

	// 空きブロックリストをもとに空きブロックを詰めていく //
	data->OpenEmptyList( );
	EmptyList &el = *data->emptylist;
	el.Sort();
//	if ( !el.next ) return 0;

	int modnum = 0;	// 最適化回数

	// 空きブロックリストが２つ以上ないとうまくない //
	// １つでも良いが、そうすると最終空きブロックであるかどうか判断する場合
	// 辞書破壊を起こしていると無限ループに入る可能性がある
	EmptyList *cel = &el;
	t_pbn2 cpbn = cel->offset;
	while ( cel->next && cel->next->next ){
		EmptyList *nextcel = cel->next;
		t_pbn2 spbn = cpbn + nextcel->num;	// 移動元
		t_pbn2 dpbn = cpbn;	// 移動先
		t_pbn2 npbn = ( nextcel->next ? cpbn + nextcel->offset : data->GetNBlock() );	// 次の空きブロックの物理ブロック番号
		if ( nextcel->num * 20 < npbn - spbn ){
			// 移動ブロック数が空きブロック数の20倍以上である場合は次へ
			cpbn += nextcel->offset;
			cel = nextcel;
			continue;
		}
		while ( spbn < npbn ){
			// 次の空きブロックのあるところまでブロック移動を行う //
			// spbnの論理ブロック番号を探す
			int lbn = index->searchLbn( spbn );
			if ( lbn == -1 ){
				error = 31;
				return -1;	// 辞書が壊れている
			}
			t_pbn2 num = data->ReadBlockNum(spbn);
			if (!data->file_copy(spbn, dpbn, num))
				return -1;
			index->setIndexBlock( lbn, dpbn );	// 新しいインデックスに更新
			spbn += num;
			dpbn += num;
		}
		if (!data->Flush()) return -1;

		// 空きブロック変更
		// 次の空きブロックと連結する必要がある
		cel->offset += dpbn - cpbn;
		nextcel->offset = nextcel->num;
		// 次の空きブロックの個数を足す
		nextcel->num += nextcel->next->num;
		// 次の空きブロックを解放
		nextcel->_Free();
		modnum++;

		if (modnum>=max_mod)
			break;
		cpbn += dpbn - cpbn;
	}
	return modnum;
}
inline void DBOPT(...){}
int IndexData::SaveAllTempBuff( int &ntemp, TempBuff &tb, EmptyList &el, t_pbn2 *startpbn )
{
	DBOPT("テンポラリバッファが一杯になったので、すべて空きブロックへ掃き出します\n");
	ntemp = 0;
	t_pbn2 blk;
	uint8_t *buf;
	while ( ( buf = tb.allLoad( blk ) ) != NULL ){
		t_blknum num;
		num = (t_blknum)(*(t_blknum*)buf & ~FIELDTYPE);
		t_pbn2 vblk = el.NewBlock( num, NULL, startpbn );
		if ( vblk == BLK_ERROR ){
			DBOPT( "BLK_ERRORだよー\n" );
			vblk = data->GetNBlock();
			data->SetNBlock( data->GetNBlock() + num );		// NEWDIC2形式になると、ディスク容量不足で
															// エラーになる可能性が・・・
		}
		data->SetDataBuf( vblk, buf, num );
		delete[] buf;
		DBOPT( "setIndexBlock:blk=%d vblk=%d\n", (int)blk, (int)vblk );
		index->setIndexBlock( blk, vblk );
		if (data->write(vblk) == -1){
			return -1;
		}
		if (!data->Flush()){
			return -1;
		}
		DBOPT(">>[lbn:%d,%d]を[pbn:%d,%d]へ\n", blk, num,
			pbnGetBlock(vblk), pbnGetNum(vblk) );
	}
	return 0;
}
#endif	// DOS
#endif	// SMALL

#include "filestr.h"

// 検索などで使用していたbufferを解放する
void IndexData::FreeBuffers(AllSearchParam *_all)
{
	AllSearchParam &all = _all ? *_all : this->all;
	all.FreeBuffers();
}

// Debug //
int DicLogEnabled = 0;

void IndexData::DoBackup()
{
	if (!index)
		return;

	FileBuf &file = index->GetFile();
	tnstrbuf backupname = BackupDir.c_str();
	backupname += _t("\\");
	backupname += ::GetFileName(file.GetFileName());

	time_t now;
	now = time(NULL);
	struct tm *t = localtime(&now);
	backupname += tnsprintf(_t(".%02d%02d%02d_%02d%02d%02d_bak"),
		t->tm_year%100, t->tm_mon+1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec);
	int wfd = _topen(backupname, O_WRONLY|O_TRUNC|O_CREAT, S_IREAD|S_IWRITE);
	if (wfd<0){
		diclog(_t("Cannot backup : %s"), backupname.c_str());
		return;
	}
	
	bool open = false;
	if (file.isOpen()){
		
	} else {
		if (file.read_open(file.GetFileName())!=0){
			diclog(_t("Open error for the backup"));
			return;
		}
		open = true;
	}

	__pdc64 loc = file._tell();
	file.seek(0);

	size_t bufsize = 0x10000;
	char *buf = new char[bufsize];
	if (!buf){
		diclog(_t("Memory error for the backup"));
	} else {
		while (1){
			int rlen = file.read(buf, bufsize);
			if (rlen<=0)
				break;
			int wlen = write(wfd, buf, rlen);
			if (wlen!=rlen){
				diclog(_t("write length error : %d->%d"), rlen, wlen);
				break;
			}
		}
		delete[] buf;
	}
	
	
	file.seek(loc);
	close(wfd);
	
	if (open){
		file.close();
	}
}

void IndexData::diclog( const tchar *format, ... )
{
#ifdef _Windows	// とりあえずwindowsのみ
	va_list ap;
	va_start( ap, format );

	if (index){
		tchar buf[0x1000+1];
		const int buflen = tsizeof(buf)-1;

		int fd = _topen(LogName, O_WRONLY|O_BINARY|O_CREAT|O_APPEND, S_IREAD|S_IWRITE);
		if (fd>=0){
			time_t now;
			now = time(NULL);
			struct tm *t = localtime(&now);
			int len = swprintf(buf, _t("%02d/%02d/%02d %02d:%02d:%02d : "),
				t->tm_year%100, t->tm_mon+1, t->tm_mday,
				t->tm_hour, t->tm_min, t->tm_sec);
			write(fd, buf, len*sizeof(tchar));

			vsnwprintf( buf, buflen, format, ap );
			write(fd, buf, _tcslen(buf)*sizeof(tchar));
			write(fd, _t("\n"), sizeof(tchar));
			close(fd);
		}
	}

	va_end( ap );
#endif
}

