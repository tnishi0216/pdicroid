//---------------------------------------------------------------------------

#ifndef WinSquUIH
#define WinSquUIH
//---------------------------------------------------------------------------

#include "defs.h"

// warning type for warning()
// 表示優先順位が高い順
//Note: ここを変更時
// ・N_WARNTYPEも変更のこと
enum {
	WT_FIRST=0,
	WT_DICBROKEN=0,
	WT_INETDIC=1,
	WT_DICVUP=2,
	WT_SUGGEST=3,

	WT_ALL,
};

class ISearchingDlg;

class TSquUIWord {
public:
	virtual void SetText(const tchar *text) = 0;
	virtual tnstr GetText() = 0;
	virtual tnstr GetCompleteWordText() = 0;
//	const tchar *GetTextPtr()			// 副作用があるので使用中止
//		{ return GetText().c_str(); }
	virtual int GetTextLen() = 0;
	virtual bool GetLastUndo() = 0;
};

enum eMainCommand {
	MainCmd_EditWord,	// 訳語編集
	MainCmd_SettingViewItem,
};

class TSquUIMain {
public:
	virtual ~TSquUIMain(){}
	virtual HWND GetHWindow() = 0;	//TODO: 不要にする
	virtual bool ReturnProc(bool fNeedWord=true, int dicno=-1, const tchar *word=NULL, class Japa *japa=NULL, bool fSelDic=false, bool bMoveToTop=false ) = 0;
	virtual void message(int id, int pasttime=-1, int count=-1) = 0;
	virtual void message(const tchar *str, int pasttime=-1, int count=-1) = 0;
	virtual void dispStar(int no, int rev){}
	virtual bool SelObjectDlg(int &cftype) = 0;
	virtual void SetWordText(const tchar *text, bool research=false) = 0;
	virtual tnstr GetWordText() = 0;
	virtual tnstr GetCompleteWordText() = 0;
	virtual void CMUndo() = 0;
	virtual bool IsActive() = 0;
	virtual void SetEnable(bool enabled) = 0;
	virtual ISearchingDlg *OpenSearchingDlg(void *) = 0;
	//virtual void EnableAutoSrch(bool enabled) = 0;
	virtual void ActivateMainWindow() = 0;
	virtual void EnableMainWindow(bool enabled) = 0;
	virtual void StartSpecialSearch(const tchar *str=NULL, bool background=false) = 0;
	virtual void StopSubSearch() = 0;
	virtual void NotifyTabChanged(){}
	virtual class TPSMainForm *MakeFirstPopupWindow() = 0;
	virtual HWND GetPopupWindowEditHandle() = 0;
	virtual void SetTextPopupWindow(const tchar *text) = 0;
	virtual void WordSearch( int mode, const tchar *str ) = 0;
	//virtual bool ConvertDictionaryWizard(const tchar *filename, tnstr &newname){ return false; }

	// Misc. //
	virtual void PostMainCommand(eMainCommand cmd) = 0;

#ifdef __ANDROID__
	// word list update //
	virtual void ListClear() = 0;
	virtual void ListAdd(tnstr *word, Japa *japa, int dicno, int level) = 0;
	virtual void ListInsert(int index, tnstr *word, Japa *japa, int dicno, int level) = 0;
	virtual void ListDel(int index) = 0;
	virtual void ListDel(int index1, int index2) = 0;
#endif
};

class IDialog {
public:
	virtual void DlgMessageProc() = 0;
	virtual void Close() = 0;
};
class ISearchingDlg : public IDialog {
public:
	virtual ~ISearchingDlg(){}
	virtual bool IsStopped() = 0;
	virtual bool IsSpeedUp() = 0;
};

#endif

