#ifndef	__UTYDIC_H
#define	__UTYDIC_H

#include	"dic.h"
#undef _mchar
#include	"file.h"

#include "utydefs.h"

// for check //
#if 0	// これは何のため？
#if !defined(DLL) && !defined(DIC_TEST)
#if DICDBG1 || !defined(PDICW) || !defined(_WINDOWS) || !defined(GUI) || !defined(_Windows) || !USE_DICFASTMODE || !defined(ND3ONLY) || defined(USE_ESTDB)
#error
#endif
#endif
#endif

class TLangProc;
class TCCTable;

class Dictionary {
public:
	// same values as DICERR_
	enum ErrorCode {
		ECNone = 0,
		ECNoFile = 1,
		ECOpenError = 1,
		ECWriteError = 5,
		ECTooLong = 29,
		// utydic specific error
		ECReadFormat = 43,
	};
protected:
	tnstr name;
	int dictype;
	bool canmerge;				//マージできる辞書は1
	MergeMode mergemode;
public:
	int app;			//新規登録数
	int mod;			//修正
	int def;			//欠落単語数

	int Frequency;		//出現頻度

	int error;		//エラーが起きたときその種類の数値が入る
#if DICDBG1
	int readcount;
	int maxcount;
#endif
public:
	int outflag;	// 出力項目:OF_..
	tnstr optionstr;
protected:
	// Note: CFlagsX can be changed after open.
	UINT CFlags;	// コンバート用汎用フラグ
	UINT CFlags2;	// auxiary convert flags.
public:
	void SetCFlag( UINT flag, bool f )
		{ CFlags = (CFlags & ~flag) | ( f ? flag : 0 ); }
	void SetCFlags(UINT flags)
		{ CFlags = flags; }
	void SetCFlag2(UINT flag, bool f)
		{ CFlags2 = (CFlags2 & ~flag) | (f?flag:0); }
	void SetCFlags2(UINT flags)
		{ CFlags2 = flags; }
protected:
	int KeyTransIndex;	// キーワード生成用translate table index
	TLangProc *LangProc;

	//長すぎ単語のファイル

	TOFile overf;
#ifdef PDICW
protected:
	class PronTable *prontable;
public:
	void SetPronTable( PronTable *pt )	// 所有権が渡るので注意!!
		{ prontable = pt; }
#endif
protected:
	class TUtyDicView *View;
public:
#ifdef	_WINDOWS
	// Pdic2でrecord()を呼ぶ場合は必ずセットする必要がある
	//	ただし、inqf=FALSEなどのように、問い合わせが発生しない場合は必要ない
	void SetParent(class TConvertingDialog *parent);
#endif
public:
	int over_open( const tchar *filename );
	void over_close( void );
	int inqf;					//マージの際の問い合わせ
protected:
	virtual int getWord( tnstr &word) = 0;
	virtual int getJapa( Japa &japa ) = 0;
public:
	Dictionary();
	virtual ~Dictionary();

	// PdicObjectを仮想基本クラスにしないので非常に面倒くさい
	int GetErrorCode( );
	virtual int _getError( )	{return error;}
	int GetAttrLen( );

	//Open, close関連
	virtual void SetFastMode( )	{}
	virtual int Open( const tchar *fname, int mode );
#ifdef GUI
	virtual int CanOpen( TWinControl *, const tchar *fname, int mode ) = 0;
#endif
	virtual void Close( ) = 0;

	//ファイル情報
	virtual int percent(void)=0;
	const tchar *GetFileName()	{ return name; }
	int GetDicType( ) { return dictype; }
	bool canMerge( )	{return canmerge;}
	void setMergeMode(MergeMode mm)	{mergemode = mm;}
	MergeMode getMergeMode(void)	{return mergemode;}

	//読み込み
	virtual int readPare( tnstr &word, Japa &japa );
		//必ず、readPare()か、getWord()+getJapa()どちらかをオーバライドすること

	//書き込み
	virtual int record(const tchar *word, Japa &japa )	{return 0;}	//（準）必須

	int getApp(void)	{return app;}
	int getMod(void)	{return mod;}
	int getDef(void)	{return def;}		//欠落単語数

	// Char code conversion table.
protected:
	// These tables are enabled when TransCode is true except Pdic2 class.
	// The owner of these objects is this class.
	TCCTable *cctWord;
	TCCTable *cctPron;
	TCCTable *cctJapa;
	TCCTable *cctExp;
	wchar_t *CodeConvBuf;
	int CodeConvBufSize;
public:
	void SetCCTables(TCCTable *_cctWord, TCCTable *_cctPron, TCCTable *_cctJapa, TCCTable *_cctExp);
	void ConvertCodeWord(const char *src, tnstr &dst )
		{ ConvertCode(cctWord, src, dst); }
	void ConvertCode( const class CJapa &src, Japa &dst );
protected:
	void ConvertCode( TCCTable *table, const char *src, tnstr &dst );
	virtual void HandleCCError(const char *src){}
	virtual void HandleCCWarning(const char *src){}
};

class PdicU : public Pdic {
private:
	bool bFix;	// 修正が可能であるかどうか
	bool bFixed;	// 修正したかどうか
protected:
	bool Debug;
public:
	PdicU();
	void EnableFix( )
		{ bFix = true; }
	bool IsFix() const
		{ return bFix; }
	bool IsFixed() const
		{ return bFixed; }
	void debug(bool enabled)
		{ Debug = enabled; }
	bool debug() const
		{ return Debug; }
///////////////////////////////////////////////////
///	Ｏｐｔｉｍｉｚｅ：最適化に関するメソッド	///
///////////////////////////////////////////////////
private:
	int wordnum;			//単語数
	int usebyte;			//使用バイト数
protected:
	void initCountWord( )	{wordnum=0L;usebyte=0L;}
		//countWordの初期化専用
	void countWord( );	//databufにある単語数を数える
							//usebyteはブロック内で使用しているバイト数
public:
	int optimize( struct PDDICCHECKPARAM *dcp );
private:

///////////////////////////////////////////////////
///	辞書のチェックに関するメソッド				///
///////////////////////////////////////////////////
private:
	const _mchar *wordptr;		// wbufを差すポインタ

								// disppos()で使用

protected:
#ifdef	PDICW
	class TProgressDialog *curdlg;
#endif
protected:
	int recover( );
	void dispinfo( );
	void disppos(int lbn, int u);

public:
#ifdef	PDICW
	int Message( int id1, int id2, UINT flag );
	int Message( int id, UINT flag );
	int checkindex( TProgressDialog * );
#endif
#ifdef GUI
	int checkdata( class TDicCheckingDialog *, int numempty );
	int chk_lostlink( class TProgressDialog *dlg, int fix, int *numempty );	//ロストリンクを探す
#else
	int checkdata( );
	int chk_lostlink( int fix);	//ロストリンクを探す
#endif
protected:
	// エラー位置表示を伴う
	int PosMessage( int lbn, int u, const tchar *str );

	bool ProcessUI(PDDICCHECKPARAM *dcp);
	DWORD PrevTime;
	bool IsUpdateTime();
	void InitUpdateTimer();
};

class UserDic;

#define	CAST_PDIC2( cls )	((Pdic2*)cls)

#if (defined(_Windows) || defined(UNIX)) && !defined(SML)
#define	USEREFMODE	1
#else
#define	USEREFMODE	0
#error	// こちらにするとrefmodeを前提とした利用をしているところがあるため正常に動かない場合がある(ex.テキスト形式辞書)
#endif

class PdicUni;
class PdicAnsi;
class DllDictionary;

class Pdic2 : public Dictionary {		//pduty用のPDIC3辞書
protected:
	Pdic *pdic;
	DllDictionary *dlldic;
	//int dictype;	// DT_PDIC/DT_PDIC_OLD_ANS/DT_PDIC_OLD_UNI, available at opened.
public:
#if USEREFMODE
	bool fRefMode;		// pdicをリファレンスモードでオープンした場合
	int linkattr;		// refmode時の保存用
#endif
protected:
	virtual int getWord( tnstr &)	{ return -1; }
	virtual int getJapa( Japa &  ){ return -1; }
public:
	Pdic2();
	virtual ~Pdic2();
	virtual int Open(const tchar *fname, int mode);
	int Open( Pdic *_pdic, int mode );
#ifdef GUI
	virtual int CanOpen( TWinControl *, const tchar *fname, int mode );
#endif
	virtual void Close( );
#if USE_DICFASTMODE
	virtual void SetFastMode( )
		{ pdic->SetFastMode( );}
#endif
	virtual int record(const tchar *word, Japa &japa );
	int InquiryLevelMerge( const tchar *word, int slev, int dlev );
	int InquiryMerge( const tchar *word, Japa &srcjp, Japa &destj, Japa &newj, Japa **recj, int overflg, Pdic *pdic );
	virtual int readPare( tnstr &word, Japa &japa );
	virtual long length( );
	virtual int percent( );
	virtual int _getError( );
	void setError( int _error )
		{
			if ( pdic ) pdic->SetError( _error );
			Dictionary::error = _error;
		}
protected:
	void over_flow( const tchar *word, const tchar *srcjapa );

public:
	void SetAllSearch( const tchar *word, SrchMode mode );
#ifndef ND3ONLY
	void SetDivRatio( int ratio )
		{ pdic->SetDivRatio( ratio ); }						//格納率
#endif
	int NextAllSearch( tnstr &word, Japa *japa)
		{ return pdic->NextAllSearch_( word, japa ); }
	int ResetAttr( const tchar *word, uchar bit )
		{ return pdic->reset_attr( word, bit ); }

	int bsearch( const _mchar *word )
		{ return pdic->BSearch( word ); }
	void getfjapa( Japa &j )
		{ pdic->getfjapa( j ); }
	const _mchar *getfword( )
		{ return pdic->getfword(); }

#ifdef USE_USERDLL
	UserDic *srcdic;
#endif

	Pdic *GetPdic()
		{ return pdic; }	// pdic can be NULL.
	void SetCompFlag(int flag);
	void SetLinkAttr(int attr);

	DllDictionary *GetDllDic()
		{ return dlldic; }
};

class OtherDictionary : public Dictionary {
protected:
	File *iof;
public:
#ifdef _UNICODE
	bool TransCode;
	bool bom;
	int textmode;
#endif
protected:
	int getlineT(tnstr &word, TCCTable *cct);

protected:
	virtual int getWord( tnstr &word) = 0;
	virtual int getJapa( Japa &japa ) = 0;
	int PutMultiLine( const tchar *p );
	int PutText( Japa &japa, BOOL fExtPdicText );
public:
	OtherDictionary( );
	virtual ~OtherDictionary();
	void NonFileMode();
	virtual long length( )
		{return filelength(iof->get_fd());}
	virtual int percent( )
			{return (int)((unsigned long long)tell(iof->get_fd()) * 100 / length());}
#ifdef GUI
	virtual int CanOpen( TWinControl *, const tchar *fname, int mode );
#endif
	virtual int Open( const tchar *filename, int mode );
	void Close( );

	TIFile &tif()
		{ return *(TIFile*)iof; }
	TOFile &tof()
		{ return *(TOFile*)iof; }
	BIFile &bif()
		{ return *(BIFile*)iof; }
	BOFile &bof()
		{ return *(BOFile*)iof; }
};

class DOSdic : public OtherDictionary {
protected:
	int getWord( tnstr &)	{return -1;}
	int getJapa( Japa & )	{return -1;}
public:
	DOSdic();
	virtual int Open(const tchar *fname, int mode);
	int readPare( tnstr &word, Japa &japa );
};

#define BUF_SIZE	1024

class Perd12 : public OtherDictionary {
public:
	int record(const tchar *word, Japa &japa );
protected:
	virtual int getWord( tnstr &word);
	virtual int getJapa( Japa &japa );
public:
	Perd12( );
#ifdef _Windows
	int record( const tchar *word, const tchar *japa );
#endif
};

class WX2 : public Perd12 {
protected:
	const tuchar *jp;
protected:
#ifdef	_WINDOWS
	int record(const tchar *word, Japa &japa );
#endif
	int getWord( tnstr &word);
	int getJapa( Japa &japa );
public:
	WX2( );
};

class WLevel : public Perd12 {
protected:
	int getWord( tnstr &)	{return -1;}
	int getJapa( Japa & )	{return -1;}
public:
	WLevel( );
	int readPare( tnstr &word, Japa &japa );
	int record(const tchar *word, Japa &japa );
};

class ExtPdicText : public Perd12 {
protected:
	tnstr *fbuf;
public:
	ExtPdicText( );
	~ExtPdicText( );
protected:
	virtual int record(const tchar *word, Japa &japa );
	virtual int readPare( tnstr &word, Japa &japa );
};

class ExtPdicText2 : public ExtPdicText {
typedef ExtPdicText super;
protected:
	virtual int record(const tchar *word, Japa &japa );
};

// 汎用入力専用テキストファイル
class TextFile : public Perd12 {
protected:
	int LineCount;
public:
	TextFile( );
	int GetLine( tnstr &buf )
		{ return tif().getline( buf ); }
#if defined(_UNICODE) && defined(TNANSI)
	int GetLineA( tnstr &buf )
		{ return tif().getlineA( buf ); }
#else
	int GetLineA( tnstr &buf )
		{ return GetLine( buf ); }
#endif
};

class TSVFile : public ::TextFile {
typedef ::TextFile super;
protected:
	tchar *LineBuf;
	int LineBufLen;
	tchar Delim;
	int IniQuote;
	bool FirstLine;
	// for read operation
	enum Parameters {
		MaxTrs = 9,
	};
	int NumTrs;
	int WordCol;		// Word column number
	int KeywordCol;		// Keyword column number
	int ItemTrs[MaxTrs];	// ItemID list
	int ItemCol[MaxTrs];	// Item Column number list
	//tchar IllChar;		// 不正文字コード時の代替文字
public:
	TSVFile();
	virtual ~TSVFile();
	virtual int Open( const tchar *filename, int mode );
	virtual int readPare( tnstr &word, Japa &japa );
	virtual int record(const tchar *word, Japa &japa );
protected:
	bool readHeader(tnstr_vec &items);
	bool writeHeader();
};

class CSVFile : public TSVFile {
public:
	CSVFile( );
	~CSVFile( );
//	virtual int readPare( tnstr &word, Japa &japa );
//	virtual int record(const tchar *word, Japa &japa );
};

class Utf8File : public ::TextFile {
typedef ::TextFile super;
public:
	int Open( const tchar *filename, int mode );
protected:
	bool putline(const char *text);
	bool put(const char *text, int len);
	bool put(const char *text)
		{ return put(text, strlen(text)); }
	bool putxml(const char *text);
#if defined(UNICODE) && !defined(__UTF8)
	bool putxml(const tchar *text);
#endif
};

// export only
class PdicXmlFile : public Utf8File {
typedef Utf8File super;
protected:
	bool FirstLine;
	struct TInetDicInfo *InetInfo;
public:
	PdicXmlFile();
	~PdicXmlFile();
	virtual int Open( const tchar *filename, int mode );
	virtual void Close( );
	virtual int record(const tchar *word, Japa &japa );
	void SetCryptKey(const char *key);
protected:
	bool writeHeader();
	bool writeFooter();
	bool putxml(const tchar *text);
};

#define	CAST_XML(dic)	((PdicXmlFile*)dic)

class FENGFile : public ::TextFile {
protected:
	// 行先読み
	BOOL fLine;	// 前行はまだ未処理
	tnstr line;	// 前行の残り
//		const tchar *pline;	// 前行の残りのポインタ

	// 複数単語
	tnstr_vec swords;	// 複数単語用のバッファ
//		tnstr sword;	// 複数単語用のバッファ
//		const tchar *pword;	// 複数単語用のポインタ
	tnstr sjapa;	// 前回の日本語訳(用例は省略）
	tnstr lbr;
	tnstr rbr;
//		BOOL fSameJapa;	// 前回と同じ日本語訳を使う
public:
	FENGFile( );
#ifdef GUI
	int CanOpen( TWinControl *parent, const tchar *fname, int mode );
#endif
	virtual int Open(const tchar *fname, int mode);
	void SetLabel( const tchar *lbr, const tchar *rbr );
protected:
	virtual int record(const tchar *word, Japa &japa );
	virtual int readPare( tnstr &word, Japa &japa );
};

#if USE_BM
// export only
class TBookmarkStream : public Dictionary {
typedef Dictionary super;
protected:
	class TBookmarkItem *TopNode;
	class TBookmarkItems *Child;
	timex_t TimeStamp;
	__override int getWord( tnstr &word) { return -1; }
	__override int getJapa( Japa &japa ) { return -1; }
public:
	TBookmarkStream( );
	//virtual ~TBookmarkStream(){}
	__override int CanOpen( TWinControl *, const tchar *fname, int mode ) { return 0; }
	__override int Open( const tchar *filename, int mode );
	__override void Close();
	__override int percent(){ return 0; }

	__override int record(const tchar *word, Japa &japa );
};
#endif

#if defined(PDICW) && !defined(UNIX)
#include "exptemp.h"
class UserFile : public TextFile {
typedef ::TextFile inherited;
public:
	UserFile( );
	~UserFile();
	bool LastLine;
	ExpTempParam etp;
protected:
	virtual int Open(const tchar *fname, int mode);
	virtual int record( const tchar *word, Japa &japa );
	virtual int readPare( tnstr &, Japa & ) { return -1; }
};

#ifdef PDICW
class RTFFile : public TextFile {
protected:
public:
	RTFFile( );
	virtual int Open(const tchar *fname, int mode);
protected:
	virtual int record(const tchar *word, Japa &japa );
	virtual int readPare( tnstr &word, Japa &japa );
};
#endif

#ifdef USE_ESTDB

class TQDBMDict : public OtherDictionary {
typedef OtherDictionary super;
protected:
	class TEstraierDllLoader *Dll;
	bool Load();
	void Unload();
public:
	TQDBMDict();
#ifdef GUI
	virtual int CanOpen( TWinControl *, const tchar *fname, int mode );
#endif
};

class TQDBMDepot : public TQDBMDict {
typedef TQDBMDict super;
protected:
	struct DEPOT *db;
public:
	TQDBMDepot();
	~TQDBMDepot();
	virtual int Open( const tchar *fname, int mode );
	virtual void Close( );
	virtual int record(const tchar *word, Japa &japa );

	//ファイル情報
	virtual int percent();

protected:
	virtual int getWord( tnstr &word);
	virtual int getJapa( Japa &japa );
};

class TQDBMVilla : public TQDBMDict {
typedef TQDBMDict super;
protected:
	struct VILLA *db;
public:
	TQDBMVilla();
	~TQDBMVilla();
	virtual int Open( const tchar *fname, int mode );
#ifdef GUI
	virtual int CanOpen( TWinControl *, const tchar *fname, int mode );
#endif
	virtual void Close( );
	virtual int record(const tchar *word, Japa &japa );

	//ファイル情報
	virtual int percent();

protected:
	virtual int getWord( tnstr &word);
	virtual int getJapa( Japa &japa );
};

#endif	// USE_ESTDB

#ifdef USE_USERDLL

#ifndef __USERDIC_H
#define	INVOKE_USERDIC
#include "userdic.h"
#endif

class UserDic : public Perd12 {
protected:
	static LPFNSETUP lpfnSetup;
	static LPFNCLEANUP lpfnCleanup;
	static LPFNOPEN lpfnOpen;
	static LPFNCANOPEN lpfnCanOpen;
	static LPFNCLOSE lpfnClose;
	static LPFNGETWORD lpfnGetWord;
	static LPFNMERGE lpfnMerge;
	static LPFNRECORD lpfnRecord;
	static LPFNGETPERCENT lpfnGetPercent;
	static HINSTANCE hDll;
	static int count;		// オブジェクト生成数
	PDICDATA data;
public:
	UserDic( );
	~UserDic();
	int Setup( const tchar *dllname );
	void Cleanup( );
	void SetupHWND( HWND hwnd )
		{ data.hWnd = hwnd; }
public:
	int record(const tchar *word, Japa &japa );
protected:
	virtual int getWord( tnstr &){ return -1;}
	virtual int getJapa( Japa &japa ){ return -1; }
	virtual int readPare( tnstr &word, Japa &japa );
public:
	int Open( const tchar *fname, int mode );
#ifdef GUI
	virtual int CanOpen( TWinControl *, const tchar *fname, int mode );
#endif
	BOOL Merge( const tchar *word, Japa &src, Japa &dest );
	virtual void Close( );
	virtual int percent( );
};
#endif	// end of USE_USERDLL
#endif


//void stripattr(tchar *japa);	// To be deleted.

Dictionary *MakeDic( int type );

#endif	// __UTYDIC_H


