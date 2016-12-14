//---------------------------------------------------------------------------

#ifndef PopupSrchH
#define PopupSrchH
//---------------------------------------------------------------------------

#include "PopupDefs.h"
#include "IPopupSearch.h"

class TPopupBaseWindowUser;

// PopupForWords用作業変数
struct TPopupForWordsParam {
	tnstr wholeword;
	const tchar *word;
	const tchar *prevword;
	int click_pos;	// relative position from word.
	FlexArray<ulong> wordlengths;
	const tchar *curp;
	int curi;

	TPopupForWordsParam()
		:click_pos(0)
	{}
	void ForwardWords( int skipchar );
	void BackwardWords();
	void SetWord(const tchar *__word, const tchar *__prevword, int __click_pos);
};

#if 0
class IPopupSearchImpl : public IPopupSearch {
protected:
	class TPopupSearch *ps;
public:
	IPopupSearchImpl(TPopupSearch *_ps):ps(_ps){}
	virtual TWinControl *GetParent() const;
	virtual IPopupSearch *GetRoot();
	virtual bool IsYourWindow( HWND hwnd, bool recur, HWND stop ) const;
	virtual bool IsOpened() const;
	virtual HWND GetChildHandle() const;
};
#endif

class TPopupSearch : public IPopupSearch {
private:
	static TPopupSearch *Root;
	static int MsgLoop;
	static bool DoingPreSearch;
	tnstr srchWord;	// (I/O)検索単語
protected:
	int ExcStyle;		// exclusive style
	POINT orgpt;		// (I)表示位置
//	POINT Size;			// (I)PSF_FIXEDSIZE指定時のwidth(x),height(y)
	unsigned Flags;			// (I)各種フラグ(PSF_xxx)
	unsigned ViewFlags;		// (I)view flags
public:
	int MaxWordNum;		// (I)最大表示候補数
	int type;			// (I)検索タイプ
#ifdef EPWING
	short bookno;		// (I)文書番号(EPWING)
	long pos;			// (I)ファイル位置(EPWING)
	class Pdic *epdic;			// (I)Pdic辞書(HLT_FILE,HLT_EPWINGで必要)
#endif
private:
	IPopupSearch *Parent;
	TPopupSearch *Child;
	TWinControl *Receiver;
	TComponent *Owner;		// (I)root parent
	class TPopupWindowUser *Popup;	// (O)ポップアップウィンドウ
	class TMenuWindowUser *MenuWin;// (O)メニューウィンドウ
#if SQUFONT
	HFONT hMenuFont;	// (O)作成されたメニューフォント
#endif
public:
	class MatchArray *HitWords;	// (O)ヒットした単語リスト
private:
	POINT pt;			// (N) 表示座標位置(ここにセットしては駄目！！)
	int HitWordIndex;	// (N) HitWordsのindex
	HWND hwndPopup;		// (N) popupのwindow handle
	HWND hwndMenuWin;	// (N) menuwinのwindow handle
	TPopupForWordsParam pfw; // (N)
	int freeId;			// internal : 0=誰でもfree可能 0以外=同じIDのscopeでしかdeleteできない

public:
	TPopupSearch();
	virtual ~TPopupSearch();
	// Overrides for super class.
	__override void AssignParameters(IPopupSearch *ps);
	__override IPopupSearch *GetParent() const
		{ return Parent; }
	__override void SetParent(IPopupSearch *parent)
		{ Parent = parent; }
	__override IPopupSearch *GetRoot();
	__override bool IsYourWindow( HWND hwnd, bool recur, HWND stop ) const;
	__override bool IsOpened( ) const
		{ return IsWindow( hwndPopup ) || IsWindow( hwndMenuWin ); }
	__override HWND GetChildHandle() const;
	__override bool Close(bool wait=false);
	__override void CloseNotify();
	__override IPopupSearch *GetChild() const
		{ return Child; }
	__override void SetChild(IPopupSearch *ps)
		{ Child = (TPopupSearch*)ps; }
	__override void GetNums(int &entry, int &numHitWords);
	__override int GetStyle();
	__override void SetStyle(int style);
	__override const tchar *GetCompleteWord();

	static TPopupSearch *GetTopMost();
	static TPopupSearch *GetFromPos(POINT pt);
	static bool IsAllOpened();
	static void CloseAll();

public:
	void Initialize();
	int GetBaseStyle() const;
	void SearchHitWords( const tchar *_word, const tchar *_prevword, tnstr_vec &hitwords );
	void PopupForWords( const tchar*word, int click_pos, const tchar *prevword, tnstr *fondword, bool tmp_clickpopup, const tchar *url=NULL );
	static void PopupForWordsLoop( TPopupSearch *ps, tnstr *foundword, bool tmp_clickpopup, const tchar *url=NULL );
	int PopupForWordsLoopStart(bool tmp_clickpopup, const tchar *url=NULL);
	void Open( TPopupSearch *parent, const tchar *word, bool tmp_fixedpopup, const tchar *url=NULL );
	void OpenAsMenu( const tchar *word );
	static bool IsMessageLooping()
		{ return MsgLoop>0; }
protected:
	static BOOL setfunc( int num, const tchar *word, class Japa &japa, void *menuwin );
public:
	void PostClose();
	void EnableActiveCheck();

	// Parameters //
	void SetMaxWordNum(int maxnum)
		{ MaxWordNum = maxnum; }
	void SetType(int _type)
		{ type = _type; }
#ifdef EPWING
	void SetEPWingParams(class Pdic *_epdic, short _bookno, long _pos)
		{ epdic = _epdic; bookno = _bookno; pos = _pos; }
#endif
	inline unsigned GetFlags() const
		{ return Flags; }
	inline void SetFlags(unsigned flags)
		{ Flags = flags; }
	inline void AddFlags(unsigned flags)
		{ Flags |= flags; }
	inline void SetViewFlags(unsigned flags)
		{ ViewFlags = flags; }
	inline void SetOrgPt(POINT pt)
		{ orgpt = pt; }
	POINT &GetOrgPt()
		{ return orgpt; }
	void SetNoFixSizeMove()
		{ ExcStyle |= (MWS_FIXEDMODE|MWS_FIXEDSIZE); }	// 強制的に位置固定を解除
	bool IsToMainSrchWord();

	// Getter/Setter //
	void SetSrchWord(const tchar *pat)
		{ srchWord.set(pat); }
//	tnstr GetSrchWord() const
//		{ return srchWord; }
	const tchar *GetSearchWord() const
		{ return srchWord; }

	TPopupBaseWindowUser *GetChild( );
	void SetReceiver(TWinControl *recv)
		{ Receiver = recv; }
	TWinControl *GetOwner( );
	void SetOwner(TComponent *owner)
		{ Owner = owner; }
	TPopupWindowUser *GetPopupCommon( POINT *pt );
	TPopupWindowUser *GetPointPopup( POINT pt );
	TPopupWindowUser *GetTopMostPopup();
	bool IsPopupOpened() const
		{ return IsWindow( hwndPopup )==TRUE; }
	bool IsMenuOpened() const
		{ return MenuWin && IsWindow( hwndMenuWin )==TRUE; }
	void NextHitWord(bool next=true);
	void PrevHitWord();
	const tchar *GetHitWord();	// 現在表示中の単語を返す
protected:
	void ReadData(int index, Japa *japa);

public:
	const struct MATCHINFO *GetHitWord(int index);
	int GetNumHitWords() const;
	TPopupBaseWindowUser *GetActiveWindow()
		{ return Popup ? (TPopupBaseWindowUser*)Popup : (TPopupBaseWindowUser*)MenuWin; }
	HWND GetWindowHandle() const
		{ return hwndPopup ? hwndPopup : hwndMenuWin; }
	int GetLastAction( TPopupSearch **ps );
	void SetFreeId(int id)
		{ freeId = id; }

	void Free(int id);
	void EvKeyDown(int key, int shift);
	bool EvChar(int key);

	int PrePopupSearch(const tchar *word, const tchar *prevword, int curpos, bool complete, int option);
	class PrePopupSearchThread *PrePopupSearchTH(const tchar *word, const tchar *prevword, int curpos, bool complete, int option);
	int OpenPopupSearch(TComponent *owner, IPopupSearch *parent, const tchar *_word, int curpos, const tchar *_prevword, bool complete, int option, const tchar *url=NULL);

protected:
	// external search longest word
	bool SLWRunning;
	int ThreadKey;
	void SLWStop();
	static int SLWExtCallback(class TWebSearchThread *th, int type, int param, int user);
	int SLWExtCallback(int type, int param);
	void _UpdateHitWords(int hitword_index /*, bool sync*/);
	void UpdateNotify(/*bool sync*/);
	static int cbUpdateContent(int param);
	int UpdateContent();

	// Synchronization //
#if 0
	static CRITICAL_SECTION Mutex;
	static bool MutexInitialized;
	static bool Locked;
	bool LockThread();
	static void cbLockProc(int param);
	void LockProc();
	void UnlockThread();
#endif
};

#endif	// PopupSrchH

