#ifndef __UIMain_h
#define	__UIMain_h

#include "WinSquUI.h"

#define	TPdicMain	IPdicMain
class IPdicMain {
public:
	virtual class MPdic *GetActiveDic() = 0;
	virtual ~IPdicMain(){}
};

class IWordEdit {
public:
	virtual tnstr GetText() = 0;
};
class IWordList {
public:
	virtual void Clear() = 0;
	virtual void Add(const tchar *word, const tchar *japa, const tchar *pron, const tchar *exp) = 0;
	virtual void Insert(int index, const tchar *word, const tchar *japa, const tchar *pron, const tchar *exp) = 0;
	virtual void Delete(int index) = 0;
	virtual void Select(int index, int rev) = 0;
};

class TUIMain : public TSquUIMain {
private:
	TPdicMain *Main;
	IWordEdit *Edit;
	IWordList *WordList;
	static TUIMain *Instance;
private:
	TUIMain(TPdicMain *main, IWordEdit *edit, IWordList *wordList);
public:
	virtual ~TUIMain();
	static TUIMain *CreateInstance(TPdicMain *main, IWordEdit *edit, IWordList *wordList);
	static TUIMain *GetInstance()
		{ return Instance; }
	virtual HWND GetHWindow(){ return NULL; }

	MPdic *GetActiveDic();

	virtual bool ReturnProc(bool fNeedWord=true, int dicno=-1, const tchar *word=NULL, class Japa *japa=NULL, bool fSelDic=false, bool bMoveToTop=false ){ return false; }
	// Message //
	virtual void message(int id, int pasttime=-1, int count=-1){}
	virtual void message(const tchar *str, int pasttime=-1, int count=-1){}
	void warning(int msgid, int type){}
	void netstat(int msgid, int count, int total){}
	virtual void dispStar(int no, int rev);
	// Dialog //
	virtual bool SelObjectDlg(int &cftype){ return false; }
	// Word //
	virtual void SetWordText(const tchar *text, bool research=false){}
	virtual tnstr GetWordText();
	tnstr GetCompleteWordText();
	virtual void CMUndo(){}
	// View //
	virtual bool IsActive(){ return true; }
	virtual bool IsEnabled(){ return true; }
	virtual void SetEnable(bool enabled){}
	virtual void ActivateMainWindow(){}
	virtual void EnableMainWindow(bool enabled){}
	__override void NotifyTabChanged(){}
	virtual void NotifyFontChanged(){}
	virtual void ApplyFontChanged(){}
	virtual void StartSetting(){}
	virtual void NotifySettingChanged(){}
	// Search //
	virtual ISearchingDlg *OpenSearchingDlg(void *){ return NULL; }
	//virtual void EnableAutoSrch(bool enabled);
	virtual void ResetSearch( ){}
	virtual void StartSpecialSearch(const tchar *str=NULL, bool background=false){}
	virtual void StopSubSearch(){}
	// Edit //
	void ToggleAttr(const tchar *word, int attr){}
	void ChangeAttr(const tchar *word, int attr, bool f){}
	// Popup Window //
	virtual class TPSMainForm *MakeFirstPopupWindow(){ return NULL; }
	virtual HWND GetPopupWindowEditHandle(){ return NULL; }
	virtual void SetTextPopupWindow(const tchar *text){}
	virtual void WordSearch( int mode, const tchar *str ){}
	// Profile //
	bool SaveCurDicProfiles(){ return false; }
	virtual void Load(struct TRConfigViewItemImpl &tr){}
	virtual void Save(struct TRConfigViewItemImpl &tr){}
	// Misc. //
	void DoSuggest(){}
	void PostMainCommand(eMainCommand cmd){}
	void NotifyAltEnter(){}
	void NotifyTaskTray(){}
	void IdleProc(bool &done){}

	// word list update //
	virtual void ListClear();
	virtual void ListAdd(tnstr *word, Japa *japa, int dicno, int level);
	virtual void ListInsert(int index, tnstr *word, Japa *japa, int dicno, int level);
	virtual void ListDel(int index);
	virtual void ListDel(int index1, int index2);
};

#endif

