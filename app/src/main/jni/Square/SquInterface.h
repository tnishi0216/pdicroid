//---------------------------------------------------------------------------

#ifndef SquInterfaceH
#define SquInterfaceH
//---------------------------------------------------------------------------

#include "tndefs.h"
#include "dicdef.h"
#include "winsqu.h"

class SrchOut;
class TSquare;
class TPopupSearch;

class TSquareIF {
private:
	class TSquareFrame *Frame;
	class TSquareView *View;
	TSquare *squ;
public:
	TSquareIF(class TSquareFrame *frame, TSquareView *view);
	virtual ~TSquareIF();

    // getter //
    TSquareView *GetView()
		{ return View; }
	TSquare *GetSquare()
		{ return squ; }

	// setter //
	void SetUIMain(class TSquUIMain *uimain);
	
	// setup //
	void Setup();
	void SetupView();
	void SetHWindow(HWND hwnd);
	void LoadProfile();

	// dictionary //
	class DicGroup &GetDicGroup();
	void SetDicGroup(class DicGroup &dg);
	class MPdic *GetDic();
	void Open();
	void Close();
	void TempClose();
	void Reopen();
	int ODACloseReq();
	bool CanClose(bool withmsg);
	bool IsTempClosed();
	bool IsDicOpened();
	int OpenDictionary(const class DicGroup &dg, class DicNames *dicnames=NULL, int *dicno=NULL);
	void SaveDicProfiles( class TRegKey *section, TRegKey *common);
	void LoadDicProfiles( class TRegKey *section, TRegKey *common);
	void Clear();
	void Reset( );
	void Escape();
	void ResetSearch();
	void StopSearch();
	void EndSearch();
	int IncSearch(const tchar*str, diclist_t diclist=dl_val_all );
	bool RequestScroll(int offset, bool backward);
	void StartExtSearch();
	void CancelExtSearch();
	void SetUpDown(bool f);
	void clsStar();
	int GetCur();
	bool IsCurOn();
	bool IsCurRight();

	bool CanExecuteObject();
	bool EditObject();

	bool JumpHyperLink(bool test=false);

	bool IsHideSearching();
	bool IsSearching();
	bool IsIncSearch();

	// Message //
	const tchar *GetMessage(int *pasttime);
	
	// View //
	void SetOrg();
	void Paint(HDC hdc, RECT &rc);
	void ForceDraw();
	void DispStar(int cur);
	void DispStar(const tchar *word, int dicno);
	void MoveStarLocAbs(int index);
	void SetIndexOffset(int index);
	bool IsViewOneLine();
	void SetViewOneLine(bool oneline);
	void ResetFont();
	void ResetColor();
	void Invalidate();
	void InvalidateLines();
	bool IsSelected();
	void ChangeMaxDispLevel(int offset);
	int GetFLIconic() const;
	void SetFLIconic(int index);
	void NotifyItemViewChanged();
	bool CanDispDicName() const;

	// Commands //
	void ToggleAttr(uchar bit);
	void ToggleAttr(const tchar *, uchar bit);
	void ChangeAttr(const tchar *word, int attr, bool f);
	//void ChangeAttr(wa_t bit);
	void DeleteWord();
	// same as that of TSquare and id.h
	enum {
		COPY_WORD = 110,
		COPY_JAPA = 111,
		COPY_WORD_JAPA = 112,
		COPY_WORD_JAPA_EXP = 113,
	};
	void Copy(int part);
	void PlayTTS();

#ifdef _Windows
	// Event Handler //
	LRESULT EvUpdate(TMessage msg);
	LRESULT EvMenuSelect(TMessage msg);
	LRESULT EvMenuKeyDown(TMessage msg);
	LRESULT EvMenuKeyDown2(TMessage msg);
	LRESULT EvHyperLink(TMessage msg);
#endif
	void EvActivate(bool active);

	// Key operation //
	void KeyScrollUp( );
	void KeyScrollDown( );
	void PrevPage();
	void NextPage();
	void HomePage();
	void EndPage();
	
	// Pool //
	int GetNumWords();
	const tchar *GetWord();
	int GetReverseStatus();

	// scroller //
	int ScrollDown(bool fClsRegion=false, bool down=false);		// 返り値はスクロール単語数
	int ScrollDown1(bool stopatoboom);
	int ScrollUp(bool fClsRegion=false);
	int LineScrollUp();
	int LineScrollDown();
	int MicroScrollUp(int offs=0);

	// Search //
	int Search( const tchar *str, SrchMode mode, diclist_t diclist, GenericRexp *grexp, SrchOut *srchout=NULL, int level1=WA_MINLEVEL, int level2=WA_MAXLEVEL, int MemOnOff=0 );
	int SearchHistory( struct TSearchHistoryParam &shp );
	void SearchFast(const tchar *str, SrchMode mode=(SrchMode)0, diclist_t diclist=dl_val_empty);
	void StartSpecialSearch(const tchar *str);
	void StartSubSearch(const tchar *str);
	void StopSubSearch();
	
	// Operations //
	void SelectLinkObject( int i );	// -2: 次へ -3:前へ -1:非選択 0-オブジェクト直接指定

	// Command //
	void cmdBatchDeleteWord();
	
	// Clipboard //
	void CopySelection();

	// Mouse hanlders //
	void LButtonDown(WPARAM wParam, LPARAM lParam);
	void MouseMove(WPARAM wParam, LPARAM lParam);
	void LButtonUp(WPARAM wParam, LPARAM lParam);

	// Timer handler //
	void Timer(UINT id);
	
	// Command //
	void CommandProc(int id) const
		{ squ->CommandProc(id); }
	// Popup //
	void CloseAllPopup();
	// Popup Window //
	TPopupSearch *GetPS();
	void SetPS(TPopupSearch *ps);
	bool IsPopupOpened() const;

	// Auto link //
	bool StartAutoLink();
	
	// Misc //
	bool IdleProc();
	void SetDeleteFile( const tchar *filename );

	// History //
	void MoveHistory( bool forward );
	bool CanMoveHistory( bool forward );

	// Debug //
	void dump_pool();
};

#endif

