#ifndef	__WINSQU_H
#define	__WINSQU_H

#include "pdconfig.h"

//#include "draw4.h"
#include "CharHT.h"
#include	"debugx.h"
#include	"wsqubase.h"
#include	"wsqudef.h"
#include	"srchstat.h"
#include "squfont.h"
#include "diclist.h"

#define	USE_FASTDISP	1	// 画面の最終行にある見出し語は最後まで描画しないことで高速化する

// line number display
#define	SQU_DISP_NUM	0

// １行表示
#ifdef SMALL
#define	SQUONELINE	1
#else
#define	SQUONELINE	1
#endif

#define	USE_ASSOCMENU	0

#ifdef _Windows
#define	USE_MEMICON		1		// 暗,修はiconを使用
#else
#define	USE_MEMICON		0		// 暗,修はiconを使用
#endif
								 
// posは英単語、日本語訳、用例の場合のみ有効

#define	SIZE_ATTR		2			// １属性当りの文字数(例：暗=2バイト)

#define	SQU_DELETE		1
#define	SQU_UPDATE		2
#define	SQU_ADD			3

#define	SQU_REV_FULL	3
#define	SQU_REV_GRAY	2
#define	SQU_REV_NONE	0

class SrchOut;

//#define SQUREBASE	TFrameWindow
#define	SQUREBASE TSquareBase

class TSquareBase {
public:
	virtual ~TSquareBase(){}
};

#ifdef USE_OLE
#include "eolist.h"
#endif

#ifndef USE_BMP
#define	USE_BMP	0
#endif

#ifdef OLE2
#define	IDT_DD			3
#endif

//TODO: OWLが不要になったら完全に削除

#ifndef wmMessage
#define	wmMessage(id, func)	LRESULT func(WPARAM wParam, LPARAM lParam)
#endif

#ifndef __TNCLASS_H
#define	wmDestroy()	void EvDestroy()
#endif

#undef wmLButtonDblClk
#undef wmLButtonDown
#undef wmLButtonUp
#undef wmMouseMove
#undef wmRButtonDown
#undef wmRButtonUp

#define	wmLButtonDblClk( )	void EvLButtonDblClk( UINT , POINT & )
#define	wmLButtonDown( )	void EvLButtonDown( UINT , POINT & )
#define	wmLButtonUp( )		void EvLButtonUp( UINT , POINT & )
#define	wmMouseMove( )		void EvMouseMove( UINT , POINT & )
#define	wmRButtonDown()		void EvRButtonDown( UINT , POINT & )
#define	wmRButtonUp()		void EvRButtonUp( UINT , POINT & )

struct TRDicConfig {
	LOGFONT *lf[ N_SECTION+1+1 ];	// 最後の+1はEPWING用
	struct WidthAttr *wa;
};

// 履歴単語/単語帳検索用 //
struct TSearchHistoryParam {
	bool bookmark;	// 0:history/1:bookmark
	int order;	// 0:単語順 1:頻度順
	class SrchOut *srchout;
	bool levelflag;
	int level_min;
	int level_max;
	bool memoryflag;
	bool memory_on;
	bool memory_off;
};

// 表示行情報 //
struct TSquLineInfo {
	int NumLines;
//	POINT Loc[7];	// for SN_NUMBER-SN_EXP1, SN_EXP1 is jlink.
					// x:左端からの座標
					// y:その見出し語の先頭からのY offset
};

///////////////////////////////

class TRegKey;
class TSearchMediator;

class Squre : public SQUREBASE {
typedef SQUREBASE super;

	// WinSqureBase
friend class TMainWindow;
friend class WSTarget;
friend class TMouseViewIF;
friend class TMouseCapture;

//////////////////////////////////////////////////////////////
// 基礎変数
//////////////////////////////////////////////////////////////
protected:
	HWND HWindow;	//TODO: dummy
//	TWinControl *Self;	// visual component of myself
	class TSquareFrame *Frame;
	class TSquareView *View;

public:
	HWND GetWHandle()
		{ return HWindow; }	//TODO: 最終的には削除
    void SetHWindow(HWND hwnd)
		{ HWindow = hwnd; }
	TWinControl *GetControl()
		{ return (TWinControl*)Frame; }
	TSquareView *GetView() const
		{ return View; }
	TSquareFrame *GetFrame() const
		{ return Frame; }
	
//////////////////////////////////////////////////////////////
// 基礎関数
//////////////////////////////////////////////////////////////
public:
	Squre(class TSquareFrame*, class TSquareView *, class TSquItemView &itemView);
	virtual ~Squre();

	void Setup( HDC hdc );
	void SetOrg( int lx, int ly );

	void Activate();
	void _Deactivate();
	
//	virtual void GetWindowClass( WNDCLASS& wc );
	virtual const tchar *GetClassName()
		{ return _t("PDICSQU"); }

//////////////////////////////////////////////////////////////////
//	辞書関連
//////////////////////////////////////////////////////////////////
protected:
	class DicGroup *dicGroup;
	class MPdic &Dic;
	class TInetDicUIMan *InetDicMan;
	bool Opening;
	bool Closing;
public:
	DicGroup &GetDicGroup()
		{ return *dicGroup; }
	void SetDicGroup(DicGroup &dg);
	class MPdic *GetDic();
	class TInetDicUIMan *GetInetDicMan() const
		{ return InetDicMan; }
	SVIRTUAL void Open( );		// 辞書をオープンしたとき
protected:
	void OpenSrchMed();
public:
	void Close( );		// 辞書をクローズするとき
	void TempClose( );	// 辞書を一時的クローズするとき
protected:
	void CloseCommon();
public:
	void Reopen( );	// 一時的クローズした辞書を再オープンしたとき
	int ODACloseReq();
	bool CanClose( );
	bool CanCloseMessage( );	// メッセージ付きのCanClose()
	int OpenDictionary(const class DicGroup &dg, class DicNames *dicnames=NULL, int *dicno=NULL);
protected:
	void CloseDictionary();
	bool IsDicOpened();
public:
	bool IsDicOpenedStable();
public:
#if 1	//TODO: TSquareFrameに移動するので、いずれ削除
	bool IsTempClosed() const
		{ return bTempClose; }
//	void SetTempClose(bool f)
//		{ bTempClose = f; }
protected:
	bool bTempClose;	// 一時的にクローズした
						// WM_PAINTが来たときは再描画の必要あり
#endif

//////////////////////////////////////////////////////////////
// 基本GDI関連
//////////////////////////////////////////////////////////////
public:
	HDC hdc;
public:
	HDC GetDC( );
	void ReleaseDC( );
	void SetCursor(const tchar *name);

//////////////////////////////////////////////////////////////////
//	フォント制御
//////////////////////////////////////////////////////////////////

public:
	void CMChangeFont( );
	void CMChangeFont( int inx, int uniqflag );
	void CMChangeEFont( );
#ifdef NEWDIC2
	void CMChangePFont( );
#endif
	void SelectCommonFont( );

//////////////////////////////////////////////////////////////
// 表示共通
//////////////////////////////////////////////////////////////
protected:
	void InitView();
	void InitFont();
public:
	void SetupView(HDC hdc);
protected:
	void SetOrg();
public:
	int GetHomeY( )	// 一覧表示先頭座標（ドット単位）
		{ return 0; }
	int GetLX();
	int GetLY( );		// 項目バーを除いた縦のドット数
	int GetX0( )
		{ return 0; }
	int GetY0( )
		{ return 0; }

	int GetOffs( int type );
	COLORREF GetColor( int type );
	COLORREF GetLinkColor( int type );
	const tchar *GetTitle( int type );
	int GetWidth( int type );
	void SetTitle( int type, const tchar *newtitle );
	void SetColor( int type, COLORREF color );
	void SetLinkColor( int type, COLORREF color );
	bool IsVisible(int type);

	void SetHintText(const tchar *msg);
	
//////////////////////////////////////////////////////////////
// Profile
//////////////////////////////////////////////////////////////
public:
	void LoadProfile( );
//	void SaveProfile( );
	void LoadDicProfiles( TRegKey *section, TRegKey *common );
	void LoadDicProfiles( TRegKey *, TRegKey *, HDC hdc );
	void SaveDicProfiles( class TRegKey *section, TRegKey *common);
#if SQUFONT
	void LoadProfile_Font(TRegKey *key);
	void LoadProfile_Font(TRegKey *key, int inx, TFontAttr &fa, TFontAttr *faASC=NULL);
	void SaveProfile_Font(TRegKey *key);
	void SaveProfile_Font(TRegKey *key, int inx, TFontAttr &fa, TFontAttr *faASC=NULL);
#else
	void LoadProfile_Font(){}
	void LoadProfile_Font( int , TFontAttr &, TFontAttr * =NULL ){}
	void SaveProfile_Font( ){}
	void SaveProfile_Font( int , TFontAttr &, TFontAttr * =NULL ){}
#endif
	void LoadProfileView(TRegKey *key);
	void SaveProfileView(TRegKey *key);

	void Get( TRDicConfig &diccfg );

//////////////////////////////////////////////////////////////////////////
// Data Transfer
//////////////////////////////////////////////////////////////////////////
	void Load(class TRConfigViewItemImpl &tr);
	void Save(class TRConfigViewItemImpl &tr);
	void Load(struct TRChangeFont &tr, int section);
	void Save(struct TRChangeFont &tr, int section);
protected:
	bool SaveFontSection(struct TRChangeFontItem &tr_item, int sn_section, int sqm_section, TFontAttr &font, TRegKey *key);

//////////////////////////////////////////////////////////////
// Font and colors
//////////////////////////////////////////////////////////////
private:
	// システムフォント属性
	int cxSys;		// = tm.tmAveCharWidth;
	int	cySys;		// = tm.tmHeight + tm.tmExternalLeading;

	// 一覧表示文字属性
#if SQU_DISP_NUM
	int cxTextMax;	// =max(cxText,cxAttr)
#endif
	int cyText;
	// 属性文字大きさ(英単語以外のフォントの大きさ)
	int cxAttr;
	int cyAttr;
	int cxAttr2;		// 暗,修の横幅
	int cxLevelText;

	COLORREF backcolor;	// 背景色
	COLORREF charcolor;	// 文字色
	COLORREF graycolor;

#if USE_MEMICON
	// wsview.cpp
	static class TNImage imgMemory;
	static class TNImage imgModify;
#endif

public:
	void ResetFont();
	void ResetColor();
#ifndef NOSQUCOLOR
	void ResetBackColor( );
#else
	void ResetBackColor( ){}
#endif
	void _SetupTextFonts( HDC hdc, TFontAttr &fa, int &cxMax, int &cyMax, int type );
	void SetupTextFont( HDC hdc );
//////////////////////////////////////////////////////////////
// 表示行関連
//////////////////////////////////////////////////////////////
protected:
	int LastIndex;			// 正常に表示出来た最後の相対インデックス番号
	int MaxLines;			//最大表示行数(ドット単位)
	TSquLineInfo LinesList[ MAX_NDISP ];	// 表示行数リスト
	int IndexOffset;			// 表示開始インデックス番号
	int IndexMiOffset;			// 微小スクロール
#if USE_DISPLEVEL
//	int MaxDispLevel;			// 最大表示レベル
#endif
public:
	void ClearLastIndex();
	void InvalidateLastIndex();
	void SetMaxLine( );
	void InvalidateLine(int index);
	void InvalidateLines( );	// 行数バッファを無効にする
	void RecalcAllLines( );		// バッファにある全単語の行数を再計算
	void RecalcLines( int start, int end );
	void RecalcLine( int i )
	{
		if ( LinesList[i].NumLines==0 )
			RecalcLines( i, i );
	}
	void RecalcLineForce(int abs_index);
	void DeleteLine( int abs );	// 絶対番号absを行数リストから削除、残りはつめる
	inline int GetLines( int i )
		{ return LinesList[i+IndexOffset].NumLines; }
	inline int GetAbsLines(int i)
		{ return LinesList[i].NumLines; }
//		int GetLineNum( int i );
	int GetOffsY( int i );			// 相対Y座標を得る(ドット単位）
	int GetAbsOffsY( int i );
	int GetLocateY( int i );		// 絶対Y座標を得る（ドット単位）(IndexMiOffset入り)

	void ChangeMaxDispLevel(int offset);

protected:
#if SQUONELINE
	bool ViewOneLine;
#endif
public:
	// 自動ポップアップ表示 //
#if USE_VIEWAUTOPOP
	bool ViewAutoPop;	// 自動的にポップアップ表示
	void OpenAutoPop( int absindex, bool delay=false );
	void CloseAutoPop()
		{ ClosePopup(); }
#endif
#if USE_SLASH
	bool ConcatExp;		// 訳 / 用例連結表示
#endif
public:
	bool IsViewOneLine() const
		{ return ViewOneLine; }
	void SetViewOneLine(bool oneline)
	{
		if (ViewOneLine==oneline)
			return;
		ViewOneLine = oneline;
		InvalidateLines();
		Invalidate();
	}
protected:
	int cury;					//現在のカーソル位置

#ifdef __ANDROID__
	PoolView pool;
#else
	Pool pool;
#endif
	long BaseNum;			// 配列の一番最初のデータの通し番号（番号表示用）
	int pagelines;			// 表示されているものの全行数

	// Auto Link //
	bool IsAutoLinkEnabled() const;
#if USE_DT2
	// 自動リンク検索 //
//#define	TM_ALTIMER		143
#define	TM_ALTIMERMOVE	144
//#define	TM_ALTIMERCLICK	145
#define	TM_ALTIMERMOVE2	146
	bool fAutoLinkCapture;

	int SearchAutoLink( int absno );
	//int ResearchAutoLink( int absno );
	int ResearchAutoLinkUpdate( int relno );
	void DrawUnderline( int item, int start0, int end0, int start, int end, int hitno );
//	void EraseUnderline( );
	THyperLink *GetHyperLink(int wordIndex, int itemIndex);
	THyperLink &_GetHyperLink(int wordIndex, int itemIndex);
	int HitTestHyperLink(int index, POINT pt);
public:
	tnstr GetHyperLinkText(int index, POINT pt);
protected:
	bool ReverseAutoLink( POINT *pt, bool checkhyp, bool autolink );
#endif
protected:
	void CloseAutoLink();

#if USE_DT2
	void StartHypPopup( THyperLink &hl, POINT &pt, bool delayed );
	bool JumpWordLink(bool test=false);
	void HypLinkJump( THyperLink &hl, POINT &pt, int poolindex );
	bool HypLinkJumpTest( int index, POINT pt );
public:
	bool StartAutoLink();
	void OpenAutoLinkPopup( TWinControl *Parent, POINT pt, int type, const tchar *word, THyperLink &hl );
	void CloseAutoLinkPopup();
	bool IsAutoLinkPopupOpened();
	void CloseAllPopup( );	// 完全にpopupを閉じる

	void HoldPopup( bool flag );
	void LinkJump( const tchar *keyword );
#endif

	// 履歴 //
protected:
	FlexCharArray WordHistory;
	int CurHist;
public:
	void MoveHistory( bool forward );
	bool CanMoveHistory( bool forward );
	void AddHistory( const tchar *orgword, const tchar *jmpword );
	void ClearHistory( )
		{ WordHistory.clear(); CurHist = 0; }
		
#ifdef WINCE
	POINT LastClickPoint;	// Client Area
#endif
	bool IsOver( );				// インクリメンタルサーチで表示しきれていないかどうか
	int GetIndexFromLines(int lines=0);
	int GetNextPage( );			// 次ページのインデックス番号を返す(相対インデックス番号）
	int _GetNextPage( );
	int GetLastPageOffset( );	// 最終ページの先頭の単語のオフセットを求める
								// ただし、linelistがすべてセットされていないと-1を返す
								// また、SS_FWDがあっても考慮しない
	void MakeToVisible( int cur );
	bool fRecalcLine;			// バックグラウンドで行数を再計算する必要あり
	int GetLength( int type );

public:
	int GetLineNum( );

//////////////////////////////////////////////////////////////////
//	オブジェクト関連処理										//
//////////////////////////////////////////////////////////////////
#ifdef USE_OLE
protected:
	EditObjList &eolist;
#endif
#ifdef USE_JLINK
	int LastMoveIndex;
	int LastMoveObjIndex;
#endif
public:
#ifdef USE_JLINK
	JLink *GetJLinkObject(int index, int objIndex);
#endif
	// mouse event handlers for objects.
#if 0
	void MouseMoveObject(POINT &pt, int index,int objIndex);
	void LButtonDownObject(UINT key, POINT &pt, int index, int objIndex, CharHT &cht);
	void LButtonUpObject(POINT &pt, int index, int objIndex, UINT key);
	bool DblClkObject(POINT &pt, int index, int objIndex);
	void RButtonDownObject(POINT &pt, int index, int objIndex, UINT key, CharHT &cht);
	void RButtonUpObject(POINT &pt, int index, int objIndex);
#endif
	void StartDDObject(int index, int objIndex);
	void RedrawObject(int index, int objIndex);

	bool CanExecuteObject();
	bool EditObject( int no, int objno, int verb );
#ifdef USE_JLINK
	void JLinkPopupMenu(JLink *jl, POINT pt);
#endif
	bool EditObject();
#ifdef USE_OLE
	void CopyObject( class OleItem *obj );
	bool LinkObject( class OleItem *obj );

	// objをeolistから探し、そのobject矩形の左上座標をclient座標系で返す
	// この処理は若干重いので注意
	bool GetObjectPos( class OleItem*obj, int *xpos, int *ypos );
	void UpdateOleData(int index);
	bool UpdateOleData(const tchar *word, JLOle &jl);
#endif

	void CmPasteCreate( int id );
	void CmClear( int inx, int itemno );
	bool TransferObject( int operation,
		int srctype, int srcno, int srcitem,
		int desttype, int destno,
		const tchar *word=NULL,
		FlexBufferArray *buffer=NULL, LPVOID option=NULL, int *recdicno=NULL );

// 操作モード,転送内容
#define	TOM_MOVE	0x01
#define	TOM_COPY	0x02
#define	TOM_LINK	0x04	// 転送元がTOM_CLIPBOARD,TOM_OLEDRAGDROPの場合のみ
#define	TOM_TEXT		0x10
#define	TOM_FILELINK	0x20
#define	TOM_FILEIMAGE	0x40
#define	TOM_OLE			0x80
#define	TOM_IMAGE		0x100
#define	TOM_VOICE		0x200
#define	TOM_OBJECT		0x3e0

// 転送元・先
#define	TOM_CLIPBOARD	1	// クリップボードから
#define	TOM_BUFFER		2	// WinSquのバッファから
#define	TOM_NOTHING		3	// 転送先のみ
#define	TOM_CREATEOLE	4	// 転送元のみ有効
#define	TOM_CREATEFILE	5	// 転送元のみ有効
#define	TOM_CREATEFILEIMAGE 6	// 転送元のみ有効
#define	TOM_CREATEVOICE	7
#define	TOM_MEMORY		8	// 転送元のみ有効（今のところ）
#define	TOM_RAWDATA		9	// 転送元のみ、ファイルリンクのみ(bufferにファイル名のみが入る形式）
#define	TOM_DIC			10	// 転送先のみ
#define	TOM_OLEDRAGDROP	11	// 転送元のみ for OLE2 (optionにLPDATAOBJECTが入る!)

// 制約条件 //
// 転送元がClipboardの場合
// 転送先は必ず、BUFFERかDIC
//
// 転送元がTOM_CREATE...の場合
// 転送先はBUFFERかDIC
//
// TOM_MOVEは
// 転送元が、TOM_BUFFERかTOM_DICの場合のみ

//////////////////////////////////////////////////////////////////
//	追加・削除処理												//
//////////////////////////////////////////////////////////////////
public:
	bool add( tnstr *word, Japa *j, int dicno );
	int insert2( int i, tnstr *word, Japa *j, int dicno, bool visible, int level=0 );
	void del( int i );
	int GetInsertLoc(const tchar *word);
	void UpdateProc( int i );
#define	UPF_NONE	0
#define	UPF_DELETE	1

	void Update( const tchar *word, int upf=UPF_NONE );	// wordの単語を修正、削除、新規登録した
		// upfは確定している場合のみ

//////////////////////////////////////////////////////////////////
//	情報設定・取得												//
//////////////////////////////////////////////////////////////////

public:
	int get_num( )
		{ return pool.fw.get_num(); }	//表示単語数の取得
	int GetNumWords()
		{ return get_num(); }

	bool IsHideSearching()
		{ return IsSearching() && IsIncSearch() && !bAllowDisp; }
	bool IsSearching( )
		{ return ss.IsSearching() /* || sub_ss.IsSearching()*/; }
	bool IsIncSearch( )
		{ return ss.IsIncSearch(); }	// インクリメンタルサーチか？
#if 0
	void set_mode( int _dicmode )
		{ dicmode = _dicmode; }
	int get_mode( )
		{ return dicmode;}		//最後に検索したときの表示モードの取得
#endif
	const tchar *GetWord( int i )
		{ return pool.fw[ IndexOffset + i ]; }
	const tchar *GetWordAbs( int i )
		{ return pool.fw[ i ]; }
	const tchar *GetWord( )		//現在のカーソル位置の単語のポインタを返す
		{ return pool.fw[ IndexOffset+cury ]; }
	const tchar *GetCWord();
	int GetDicCount( int i )
		{ return pool.GetFDN(IndexOffset + i); }
	int GetDicCount( )
		{ return pool.GetFDN(IndexOffset + cury); }
	Japa &GetJapa( int i )
		{ return pool.GetJapaObj(IndexOffset + i); }
	void GetJapa( Japa &j )		//現在のカーソル位置の日本語訳の先頭を返す（含む：単語属性）
		{ j = pool.GetJapaObj(IndexOffset + cury); }

	void ChangeAttr( int i, uchar bit, bool f );			// iは相対番号
	void ChangeAttr(const tchar *word, uchar bit, bool f);
	void ChangeAttr(uchar bit, bool f);
	void ChangeAttr( const tchar *word, wa_t orgattr, uchar bit, bool f );
	uchar GetAttr( int i )
		{ return GetJapa( i ).GetAttr(); }
	void ChangeLevel( int i, uchar level, diclist_t diclist );		// 単語レベルの変更 iは相対番号
	uchar GetLevel( int i )
		{ return (uchar)(GetAttr( i ) & WA_LEVELMASK); }
	void ToggleAttr( int i, uchar bit );
	void ToggleAttr( const tchar *word, uchar bit );
	void ToggleAttr(uchar bit);
	const tchar *GetPron( int i )
		{ return GetJapa( i ).GetPron(); }
	const tchar *GetExp( int i )
		{ return GetJapa( i ).GetExp(); }
	int GetDispLevel(int i)
		{ return pool.GetDispLevel(IndexOffset+i); }
	void NotifyChangeAttr(const tchar *word, int oldattr, int newattr);

	int GetFirstObjectType();
	HWND GetPopupEdit();

//////////////////////////////////////////////////////////////////
//	表示関連													//
//////////////////////////////////////////////////////////////////
	// Item View
protected:
	class TSquItemView &ItemView;
	// 描画処理
public:
	void cls( );
	void Invalidate(bool resend=false);
	void InvalidateLinks();

protected:
	void CreateTextFonts( );
	void DeleteTextFonts( );

public:
	void Paint(HDC hdc, RECT &rect);
	void display( int start=0, int end=MAX_NDISP );	// start:表示開始相対インデックス番号
										// end:表示最終相対インデックス番号
	void ForceDraw( );			// bAllowDispがFALSEでも再表示し、bAllowDispをTRUEにする
protected:
	void GetTopLeft( int no, RECT &rc, int flag );
	SVIRTUAL int DispOneLine( int no, const tchar *word, int rev, Japa &japa, int dispf, CharHT *cht=NULL );
	int DispOneLine1( int no, const tchar *word, int rev, Japa &japa, int dispf, CharHT *cht );
#if USE_DT2
	int DispOneLine( int i, int dispf, CharHT *cht=NULL )
		{ return DispOneLine( i, GetWord( i ), i==cury, GetJapa( i ), dispf, cht ); }
	int _AdjDisp( RECT &rc, TNFONT &tnfont, const tchar *str, int dispf, int swidth, CharHT *cht=NULL, EnphTextVec*et=NULL )
		{ return DrawText2( tnfont, str, &rc, swidth, dispf & DF_DISP ? dispf : (DF_CALCRECT|(dispf&DF_WHOLECHAR)), cht, et ); }
	void GetHitPosition( EnphTextVec &et, const tchar *text, int len, bool useAndSearch=false );
	#define	AdjDisp	_AdjDisp
#else
	int DispOneLine( int i, int dispf, CharHT *cht=NULL )
		{ return DispOneLine( i, GetWord( i ), i==cury, GetJapa( i ), dispf, cht ); }
	int _AdjDisp( RECT &rc, const tchar *str, int dispf, int eright );
	#define	AdjDisp( rc, tnfont, str, dispf, swidth, cht, et ) \
		_AdjDisp( rc, str, dispf | ((tnfont).fSingleByte ? DF_SINGLEBYTE:0), swidth );
#endif
	void DispObject(int no, int i, JLink &jl, int dispf, RECT &rc, int &l, CharHT *cht=NULL);
	//void DispState( int y );
private:
#if !USE_DT2
	// 反転バー表示のための変数・関数
	COLORREF oldtext;
	COLORREF oldback;
	void SetupColors( int rev, int type=SN_WORD );	// type : SN_WORD, ....
	void CleanupColors( );
#endif

	// 各項目の表示
protected:
#if SQU_DISP_NUM
	int DispNumber( RECT &rc, int no, int dispf );
#endif
	SVIRTUAL int DispWord( RECT &rc, const tchar *word, int dispf, CharHT *cht=NULL, THyperLinks *hls=NULL );
	int DispAttr( RECT &rc, uchar attr, int dispf, CharHT *cht=NULL );
	int DispAudio(RECT &rc, int dispf, int swidth, int objIndex, JLink &jl, CharHT *cht=NULL);
	int DispPron( RECT &rc, const tchar *pron, int dispf, CharHT *cht=NULL, THyperLinks *hls=NULL );
	int _DispLevel( RECT &rc, int level, int dispf );
	int DispLevel( RECT &rc, int level, int dispf, int &cyMax, CharHT *cht=NULL );
	int DispJapa( RECT &rc, const tchar *japa, int dispf, CharHT *cht=NULL, THyperLinks *hls=NULL );

	int DispExp( RECT &rc, const tchar *exp, int dispf,
#if USE_SLASH
		int initleft=0,
#endif
		CharHT *cht=NULL, THyperLinks *hls=NULL );

#ifndef SMALL
protected:
	enum {
		FL_FILENAME = 0,
		FL_ICON = 1,
		FL_CONTENT = 2
	};
	int FLIconic;		// ファイルリンクオブジェクト
						// 0 : ファイル名のみ表示
						// 1 : アイコン表示
						// 2 : 内容表示
public:
	int GetFLIconic() const { return FLIconic; }
	void SetFLIconic(int index);
#endif
	void NotifyItemViewChanged();

// 反転バーなど

public:
	int GetReverseStatus();
	void clsStar( );
	// 返り値は相対値(lastindexをオーバする事があるので注意)
	int GetStarLoc( const tchar *word, int dicno, int order )
		{ return GetStarLocAbs( word, dicno, order, IndexOffset ) - IndexOffset; }
	int GetStarLocAbs( const tchar *word, int , int order, int start=0 );
	int dispStar(int i, bool linkui=false);
	void dispStar( const tchar *word, int dicno );		//第２引き数は派生クラスで使用
	int FindNearestVisibleLevel(int index);
	void AdjustOffset( );		// offset値の調整
	void SetJMerge();
protected:
	void _dispStar( int no, int rev );
#ifndef SMALL
	bool fLongBar;		// 長い反転バー
	bool JMerge;		// 日本語訳マージ
	bool PronCR;		// 発音記号改行
#endif
private:
	int _HtmlEnabled;	// 簡易HTML
	bool _DispDicName;	// 辞書名を表示するか？
protected:
	void SetHtmlEnabled(int enabled);
	void SetDispDicName(bool on);

public:
#ifndef SMALL
	bool IsLongBar() const { return fLongBar; }
	bool IsJMerge() const { return JMerge; }
#endif
	inline int IsHtmlEnabled() const { return _HtmlEnabled; }
	inline bool IsDispDicName() const { return _DispDicName; }
	bool CanDispDicName() const { return (!JMerge && IsDispDicName()); }

protected:
	tnstr Message;	// 現在表示中のmessage
	int PastTime;
	//int _FoundCount;
public:
	void SetMessage( const tchar * =NULL, int pasttime=-1, int count=-1 );
	void SetMessage(int id, int pasttime=-1, int count=-1);
	void ClearMessage()
		{ SetMessage((const tchar*)NULL); }
	const tchar *GetMessage(int *pasttime);
	int GetSrchMsgId() const;
	int GetFoundCount();
protected:
	void ForceDrawMessage( );

//////////////////////////////////////////////////////////////////
//	スクロール関連												//
//////////////////////////////////////////////////////////////////

protected:
	int LastVRange;	// the last get_num() value in SetVRange.
	int GetVPos();
	void SetVPos( int pos, bool fRepaint=true );
	void SetVRange( int iMax, bool fRepaint=true );
	void SetVRangeSearching();
	void SetVRange2( bool fRepaint=true );			// IndexOffset,get_num()からレンジを計算(最適化あり)
	void SetHPos( int pos, bool fRepaint=true );
	void SetHRange( int iMax, bool fRepaint=true );
	void EnableVScroll( bool f );

public:
	void KeyScrollUp( );
	void KeyScrollDown( );
	void NextPage( );
	void PrevPage( );
	void HomePage( );
	void EndPage( );
	void clear( );				//すべてクリア
	void Clear( );				// クリア＋画面消去
    void Escape();

	void AutoView();

	//参照処理
	bool IsCurOn()
		{ return cury!=-1; }
	bool IsCurRight( )
		{ return cury != -1 && IndexOffset+cury < get_num(); }
	int getCur( )
		{ return cury; }
	int GetCur( )
		{ return cury; }

	// タイマー処理
//	UINT mousetimer;
	void EvTimer(UINT);
	void AlTimerMove(int timerId);
	void AlTimerMove2(int timerId);
	void AlTimer(int timerId);

	// スクロール処理
	void EvVScroll( UINT code, int &pos, HWND );	// return:new scroll pos

	int ScrollDown(bool fClsRegion=false, int down=0);		// 返り値はスクロール単語数
	int ScrollDown1(bool stopatoboom);
	int ScrollUp(bool fClsRegion=false);
	int LineScrollUp();
	int LineScrollDown();
	int MicroScrollDown( int offs=0 );
	int MicroScrollUp( int offs=0 );
	int _MicroScrollDown( int offs=0, bool repaint=false );
	int _MicroScrollUp( int offs=0 );

	void SetSelIndex( int i )
		{ dispStar( i ); }
	int GetSelIndex( )
		{ return cury; }

//////////////////////////////////////////////////////////////////
//	キー・マウス操作処理										//
//////////////////////////////////////////////////////////////////

	// マウス処理
protected:
	class TMouseCapture *MouseCap;	//TODO: いずれstatic instanceへ
public:
	int HitTest( POINT pt, UINT flag=0xffff, RECT *rcArea=NULL );			// HT_WORDITEM, HT_UPPER, HT_LOWERの判定
	bool IsLevel( int inx, POINT pt );			// HitTest()の返り値がHT_WORDITEMのとき、
										// ptがレベルにあるかどうかの判定
#if USE_DT2
	// 範囲選択に関するメンバ	///////////
	bool CharHitTest( int no, CharHT &cht, int add_flags=0 );
#endif
	void ClsRegion();
#if USE_DT2
	const tchar *GetRegion( int *len );	// 選択範囲のテキストを得る
	const tchar *GetText(int wordIndex, int itemIndex);
#endif
	bool IsSelected( );	// 範囲選択されているか？(テキストコピー用)
#if USE_DT2
	bool IsValidHitno(int selIndex);	// hitnoが有効な番号を指しているかどうか？
	bool IsVisibleHitno(int selIndex);	// hitnoが表示可能領域か？
#ifdef USE_JLINK
	class JLink &GetHitJLink( );
#endif
	class Japa &GetHitJapa( );
	bool IsLinkObjectSelected( );
	///////////////////////////////////////
#endif

#ifndef SMALL
	POINT prevpt;
	wmMouseMove();
#endif
	wmLButtonDown();
	wmLButtonUp();
	wmLButtonDblClk();
protected:
	void MouseSelected(int index/*, int way*/);
	void MouseCancelCapture();
	void MouseChangeAttr(int index, uchar bit, const RECT &rcCapAttr);
	void DrawCaptureAttr(const RECT &rcArea, RECT *rcCapArea, int httype);
	void MousePopupMenu(int index, POINT pt);
	void MouseReturnProc();
public:
	wmMessage( UM_HYPERLINK, UmHyperLink );
	void EvActivate(bool active);

	// キー処理
#ifdef _Windows
protected:
	wmKeyDown();
#endif
	// 右クリック関連
protected:
//	class TNPopupMenu *squmenu;
	void PopupMenu( HMENU hSubMenu1, POINT &pt );
public:
	wmRButtonDown();
	wmRButtonUp();
protected:
	void SetupMenu( HMENU hMenu, int inx, bool levelmenu=false );
	HMENU BuildPopupMenu( HMENU hSubMenu1, int inx, POINT &pt );
	void SetupPlayVoiceMenu( HMENU hm );

	// Clipboard //////////////////////////////
public:
	tnstr LinkURL;
	
	// OLE関連 ////////////////////////////////
public:
	void SelectLinkObject( int i );	// -2: 次へ -3:前へ -1:非選択 0-オブジェクト直接指定

#ifdef USE_OLE
public:
	LRESULT EvOleMessage(WPARAM wparam, LPARAM lparam);
	// D&D関連 ///////////////////////////////
protected:
#ifndef OLE2
	virtual void DefWndProc( TMessage &Msg );		// 注～～～～～意！！！！OWL 1.0の関数を使っている！
	tnstr ddWord;		// D&D対象単語
	DWORD ddID;			// D&D対象ID番号
	class Pdic *ddDic;	// D&D対象辞書
	UINT DragDropProc( UINT msg, struct TIDRAGDROP * );
#endif
	bool fDragging;
	bool ddfDrawFocus;
	int ddfinx;
	RECT ddrcDraw;
#ifdef OLE2
	void *ddTarget;
#endif
public:
	void InitTarget( );
	void FinishTarget( );
	void ddDraw( bool f, int inx );
	bool ddHitTestProc( class TIDRAGDROP *tdd );
	bool ddScrollProc( TIDRAGDROP *tdd, bool fScroll );
	bool ddSetSelIndex();
#ifdef OLE2
	// Activation関連 ///////////////////////////////
	bool IsObjectActivating();
	void DeactivateObject();
	void ScrolledWindow();
#else
	void Deactivate(){}
	void ScrolledWindow(){}
#endif
#else	// USE_OLE
	void Deactivate(){}
	void ScrolledWindow(){}
#endif
protected:
	void ClearObject( );
	static bool CALLBACK OnDrop( int UserData, void *pIDataSource, long grfKeyState, const POINTL pt, long &dwEffect);
	bool OnDrop( void *pIDataSource, long grfKeyState, const POINTL pt, long &dwEffect);

//////////////////////////////////////////////////////////////////
//	UI															//
//////////////////////////////////////////////////////////////////
protected:
	class TSquUIWord *UIWord;
	class TSquUIMain *UIMain;
	TSquUIMain *GetMainWindow()
		{ return UIMain; }
public:
	void SetUIMain(TSquUIMain *uimain);
//////////////////////////////////////////////////////////////////
//	検索処理													//
//////////////////////////////////////////////////////////////////

// 検索共通

protected:
	SrchStat ss;
	TSearchMediator *SrchMed;
	class TMultiAllSearchParams *SrchParams;
	class TMultiAllSearchParam *SrchParam;
private:
	bool bAllowDisp;			// 検索結果表示フラグ
	bool AllowDispMode;
	bool ForceUpdate;			// multithread検索時、表示を強制する。scroll処理でcancel
public:
	bool IsForceUpdate() const { return ForceUpdate; }
	bool SetAllowDispMode( bool f )
		{
			bool old = AllowDispMode;
			AllowDispMode = f;
			return old;
		}
	bool GetAllowDisp() const
		{ return bAllowDisp; }
protected:
	bool fUpdateDisp;			// 再表示モード
public:
	void SetUpDown(bool);

protected:
	SrchOut *srchout;		// 検索ファイル出力

public:
	bool Request( int reqoffs, bool fBack );	// offsetからのデータをfBack方向へ必要とする場合に呼ぶ
											// 処理内容：必要に応じてデータ先読みを行なう
											// IndexOffset,basenumがずれるので要注意！！
											// curyは変更しない

protected:
	SVIRTUAL int SearchProc( bool bOnlyOne=false );
	void AllowDisp( bool bOnlyOne );
public:
	bool IdleProc( );
	void EndSearch( );
	void StopSearch( );
	void PauseSearch( );

// インクリメンタルサーチ

private:
	tnstr SearchString;					//検索対象文字列
protected:
	int nSearch(const tchar *word, int nsearch, int &nfind, int dicno );
public:
#if !NEWINCSRCH
	int FoundLength;					//見つかったときの文字数
	int NotFoundLength;				//見つからなかったときの文字数
#endif

protected:
	int BSearch( const tchar*str )
		{ return pool.BSearch( str ); }
	void Keep( int i1, int i2 );
	void StartIncSearch( );

public:
	int incsearch(const tchar*str, diclist_t diclist=~0 );
	int IncSearchProc( const tchar *str, int len, diclist_t diclist );
	int SearchOnWindow( );

	// External Search //
protected:
	int ExtSrchThreadKey;
	static int cbExtSearch(class TWebSearchThread *th, int type, int param, int user);
	int ExtSrchCallback(int type, int param);
public:
	void StartExtSearch();
	void CancelExtSearch();

	void MoveStar( const tchar *str );
	void MoveStarLocAbs( int inx );
	void SetIndexOffset( int inx );
	bool ReSearch( const tchar *word, bool fBack, int fcnum );
	void ResetSearch( );
	void Reset( );
	void NeedReset( );
	void StartBackward();

// Incrental Search Plus //
#if INCSRCHPLUS
#define	TM_INCSRCHPLUS	200
	bool IncSrchPlus;
	bool ISPWaiting;
public:
	bool IsIncSrchPlus()
		{ return IncSrchPlus; }
	void SetIncSrchPlus(bool enabled);
	void StartISPTimer();
	void StopISPTimer();
	bool ISPProc();
#else
	void StartISPTimer(){}
	void StopISPTimer(){}
#endif

#ifdef USE_DELAYED_DELFILE
#define	MaxDelFiles	10
	// Delayed Delete File //
	tnstr_vec DelFiles;
	void SetDeleteFile(const tchar *filename);
#endif

// Sub Search (background) //
protected:
	tnstr sub_srchword;
	SrchStat sub_ss;
	class TSubWordItems *sub_words;
	// 連続してStartSpecialSearchを呼び出すと前回使用したsub_srchparamが
	// 検索threadでまだ使われているため、複数用意してresponseを改善する
	class TMultiAllSearchParams *sub_srchparams;
	class TMultiAllSearchParam *sub_srchparam;	// ref pointer to sub_srchparams

	//tnstr_vec sub_words0;	// word buffer for popup-search
	//tnstr_vec sub_words2;	// word buffer for second phase search.
	map<tnstr,bool> sub_words_map;	// for duplicate check
	int SubSearchProc( );
public:
	void StartSpecialSearch(const tchar *str);
	void StartSubSearch(const tchar *str);
	bool IsAutoSpecialSearchEnabled();
	void StopSubSearch();
protected:
	void _StopSubSearch();
public:
	void ClearSubSearch();
	TMultiAllSearchParam *GetSrchParam(bool wait=false);
	TMultiAllSearchParam *GetSubSrchParam();
protected:
	bool CanStartSubSearch();
	void _StartSubSearch(const tchar *str=NULL, bool first_phase=true);
	bool StartSpecialSearch(SrchStat &ss, diclist_t uDicList, const tchar *_str, bool first_phase);
	void ChangeSubSearchToMain(const tchar *word);
	bool StartSubSearchForNormalSearch(const tchar *word, SrchMode mode, diclist_t uDicList, GenericRexp *grexp, int level1, int level2, int MemOnOff);
	void AddSubWordsToMain(int &phase);
	int CopyToPool(tnstr_vec &words, diclist_t uDicList, bool insert_top);
	int CopyToPool(class TSubWordItems &words, diclist_t uDicList, bool insert_top);
	int CopyToPool(class TGenericWordVec &words, diclist_t uDicList, bool insert_top);

// 関連語検索用 //
public:
#if USE_ASSOCMENU
	class TMenuWindowUser *AssocMenu;	// 関連後検索
#endif
public:
	void CmSrchAssociatedWord( );
	bool OpenAssocMenu( );
	void CloseAssocMenu( );
	bool IsAssocMenuOpened( );

// 検索用

public:
	// 注意！！
	//	grexpの所有権はこのクラスに移るので要注意！！
	int search( const tchar *str, SrchMode mode, diclist_t diclist, GenericRexp *grexp, SrchOut *srchout=NULL, int level1=WA_MINLEVEL, int level2=WA_MAXLEVEL, int MemOnOff=0 );
	int SearchHistory( TSearchHistoryParam &shp );
	void SearchFast(const tchar *str, SrchMode mode=(SrchMode)0, diclist_t diclist=0);

	//////////////////////////////////////////////////////////////////////////
	// 日本語訳編集															//
	//////////////////////////////////////////////////////////////////////////
	wmMessage( UM_UPDATE, UmUpdate );	// 英単語更新通知

	wmMessage( UM_MENUSELECT, UmMenuSelect );
	wmMessage( UM_MENUKEYDOWN, UmMenuKeyDown );
	wmMessage( UM_MENUKEYDOWN2, UmMenuKeyDown2 );

	tnstr umWord;

	DECLARE_RESPONSE_TABLE( Squre );

//////////////////////////////////////////////////////////////////
//	Commands													//
//////////////////////////////////////////////////////////////////
public:
	// same as that of TSquareIF and id.h
	enum {
		COPY_WORD = 110,
		COPY_JAPA = 111,
		COPY_WORD_JAPA = 112,
		COPY_WORD_JAPA_EXP = 113,
	};
public:
	void CommandProc(int id);
	void cmdChangeAttr(wa_t bit);
	void cmdClipSearch();
	bool cmdDeleteWord(int _cury=-1, bool inquery=true);
	void cmdBatchDeleteWord();
	void cmdEditObject();
	void cmdEditFileLinkInfo();
	void cmdTransferObject(int tom);
	void cmdPasteSpecial();
	void cmdPasteFormat();
	void cmdPaste(int tom);
	void cmdEdit(int id);
	void cmdCopyPaste();
	void cmdCopy(int id);
	void cmdCopyLink();
	tnstr cmdCopyMake(int id, int _cury=-1);
	void cmdChangeLevel(int id);
	void cmdPlayTTS();
	bool cmdJumpHyperLink(bool test=false);
public:
	// alternative function for Win32API //
	int MessageBox(const tchar *text, const tchar *caption, UINT type);
	int MessageBox(int text, int caption, UINT type);
	int MessageBox(HWND hwnd, const tchar *text, const tchar *caption, UINT type);
	int MsgBox(const tchar *text, const tchar *caption, UINT type);
	int MsgBox(int text, int caption, UINT type);
	void UpdateWindow()
		{ ::UpdateWindow(HWindow); }
	void DefaultProcessing(){}
	UINT SetTimer(UINT id, UINT elap, TIMERPROC proc)
		{ return ::SetTimer(HWindow, id, elap, proc); }
	BOOL KillTimer(UINT id)
		{ return ::KillTimer(HWindow, id); }
	HWND SetCapture()
		{ return ::SetCapture(HWindow); }

	HINSTANCE GetInstance();
	void PostMessage(UINT msg, WPARAM wparam, LPARAM lparam=0)
		{ ::PostMessage(HWindow, msg, wparam, lparam); }
	LRESULT SendMessage(UINT msg, WPARAM wparam, LPARAM lparam)
		{ return ::SendMessage(HWindow, msg, wparam, lparam); }

	//TODO: for compatibility
	TWinControl *GetVCL();
	//////////////////////////////////////////////////////////////////////////
	// Popup Window															//
	//////////////////////////////////////////////////////////////////////////
private:
	struct TPopupSearch *ps;
public:
	TPopupSearch *GetPS()
		{ return ps; }
	void SetPS(TPopupSearch *_ps);
	bool _IsPopupOpened() const;
	void ClosePopup();

	// debug
public:
	void dump_pool();
};

class TSquare : public Squre
{
typedef Squre super;
public:
	TSquare(TSquareFrame*main, TSquareView *view, TSquItemView &itemView)
		:super(main, view, itemView){}
};

class TIncSquare : public TSquare
{
typedef TSquare super;
public:
	TIncSquare(TSquareFrame *main, TSquareView *view, TSquItemView &itemView)
		:super(main, view, itemView){}
};

int __nstr( const tchar *str, const tchar *str2 );	// normal order
#if defined(NEWDIC2) && !defined(UNICODE)
inline int nstr_0( const tchar *str1, const tchar *str2 )	// normal order
{
	return __nstr( str1, str2 );
}
int nstr_1( const tchar *str1, const tchar *str2 );	// 同一視
int nstr_2( const tchar *str1, const tchar *str2 );
int nstr_3( const tchar *str1, const tchar *str2 );	// 降順
inline int nstr( const tchar *str1, const tchar *str2, int order )	//一致する文字数を返す
{
	return    ( order == SK_NORMAL ) ? nstr_0( str1, str2 )
			: ( order == SK_IGNCASE ) ? nstr_1( str1, str2 )
			: ( order == SK_DICORDER ) ? nstr_2( str1, str2 )
			: nstr_3( str1, str2 );
}
#else
#define	nstr( str1, str2, order ) __nstr( str1, str2 )
#endif

extern int tmpcopy;		// テンプレートコピー
//extern bool IncSrchPlus;
extern int LevelValue;	// レベル表示方法
extern bool KeepStarTop;	// 常にcursor位置は固定

void ChangedIncSrchPlus( );

class TMultiAllSearchParams : public FlexObjectArray<TMultiAllSearchParam>
{
typedef FlexObjectArray<TMultiAllSearchParam> super;
protected:
	int MaxNum;
	int LastIndex;
public:
	TMultiAllSearchParams(int dicnum);
	void SetMode(int mode);
	int GetMaxNum() const { return MaxNum; }
	TMultiAllSearchParam *GetNext(MPdic &dic);
};

#endif	// __WINSQU_H

