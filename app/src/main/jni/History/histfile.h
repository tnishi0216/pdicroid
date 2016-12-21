#ifndef __HISTFILE_H
#define	__HISTFILE_H

#include "pdconfig.h"
#include "defs.h"

// Note:
//	wordとlineを区別するように

//---------------------------------------------------------------------------
// TQueHistory class
//---------------------------------------------------------------------------
// DeleteWordsHistoryのcookieに使用すると便利
struct DELETEWORDPARAM {
	//const tchar *word;	// for cbDeleteWord
	map<tnstr, bool> words;	// for cbDeleteWord
	bool all;			// for cbDeleteWord
	class MPdic *dic;	// for cbDeleteNoRec
	class TQueHistory *obj;	// for cbDeleteOldestWord
	long filesize;		// for cbDeleteOldestWord
	int *ch;			// Pointer to the character. (a little bit tricky)
	tnstr lastline;		// for PrevWord check
	void *user;			// user parameter
	int userparam1;		// user parameter
};
typedef int (*CBDeleteWordsHistory)( const tchar *line, struct DELETEWORDPARAM &dwp);

class TWordHistory;
class TWordUrlList;

#include "tnthread.h"
class THistoryFile {
protected:
	TMutex Mutex;
	tnstr FileName;
	tnstr LastCheckedFile;	// last file format check filename.
	int LastCheckedTextMode;

public:
	THistoryFile(const tchar *filename);
	virtual ~THistoryFile();
	void lock()
		{ Mutex.Lock(); }
	void unlock()
		{ Mutex.Unlock(); }

	virtual tnstr GetFileName();
	void SetFileName(const tchar *name);

	bool Load( TWordHistory &WordHist, int sort, int offset=0, TWordUrlList *urls=NULL );
	bool Load(tnstr_vec &lines, map<tnstr, int> &strindex, vector<int> *dupindex);
	bool Merge(const tchar *srcfile, bool convert_format=false);
	TOFile *CreateOpen();
	void CreateClose(TOFile *tof);
	void Delete();
	int GetCount();
	int DeleteWords(CBDeleteWordsHistory cb, DELETEWORDPARAM *dwp);

	TOFile *OpenForAppend();
	void CheckFileFormat();

	virtual int ReadLine(TIFile &tif, tnstr &line)
	{
		return tif.getline(line);
	}
	virtual int WriteLine(TOFile &tof, const tchar *line)
	{
		return tof.putline(line);
	}

protected:
	// thread unsafe methods
	bool _Load(tnstr_vec &lines, map<tnstr, int> &strindex, vector<int> *dupindex);
	bool _Load( TWordHistory &WordHist, int sort, int offset=0, TWordUrlList *urls=NULL );
	bool _Merge(const tchar *srcfile, bool convert_format=false);
	TOFile *_Create(const tchar *filename);
	int _DeleteWords(CBDeleteWordsHistory cb, DELETEWORDPARAM *dwp);
	// //

	// thread control //
	class TFileThread : public tnthread {
	typedef tnthread super;
	public:
		TFileThread()
			:super(true)
		{
		}
		__override void Execute()
		{
			for (;!Terminated;){
				Sleep(100);
			}
		}
		bool Wait()
		{
			for (int i=0;i<50;i++){
				if (Finished)
					return true;
				Sleep(100);
			}
			return false;	// timeout
		}
	} *Thread;

	bool StartThread()
	{
		if (Thread)
			return false;	// already run
		Thread = new TFileThread();
		//Thread->FreeOnTerminate = true;
		Thread->_Resume();
		return true;
	}
	void StopThread()
	{
		if (!Thread)
			return;
		Thread->Terminate();
	}
	void WaitThread()
	{
		if (!Thread)
			return;
		if (Thread->Wait()){
			delete Thread;
		} else {
			Thread->FreeOnTerminate = true;
		}
		Thread = NULL;
	}
	// //
};

class TQueHistory {
protected:
	class THistoryFile &File;
	bool ExtraMode;

	int Count;	// 履歴数、全ファイルから読み込みをしない限り、実際の履歴数と異なる場合がある
	tnstr PrevWord;
	// for queued file save
	tnstr_vec HistoryQue;		// The temporary queue for saving the history to the file.
	// for Notify
	HWND hwndParent;
	int msgParent;
	// file limitation
	int LimitLower;
	int LimitUpper;
public:
	bool IgnoreDupWord;	// 2回連続登録は無視する
public:
	TQueHistory(THistoryFile &file, bool use_extra);
	THistoryFile &GetFile() const
		{ return File; }
	int GetCount()
		{ return Count; }

	int GetLimitLower() { LoadLimit(); return LimitLower; }
	int GetLimitUpper() { LoadLimit(); return LimitUpper; }
	// 実際にファイルのサイズに反映されるのは次の履歴追加時
	virtual void SetMaxFileSize(int size);
	virtual void Add(const tchar *word, const tchar *url=NULL);
	// Store the history in the queue to the file.
	virtual bool SaveQueFile();
	// Operations //
	void Clear();
	// Delete operations
	bool Delete(tnstr_vec &words, bool all);
	protected: static int cbDeleteOldestWord(const tchar *fword, DELETEWORDPARAM &dwp); public:
	virtual bool DeleteOldestWords();

	int GetCountFile();	// Get the all history count except HistoryQueue.

	// File operation //
	virtual bool LoadFile(tnstr_vec &lines, int maxLines=0, bool dupcheck=false, const tchar *exword=NULL);
	bool SaveFile(tnstr_vec &lines, bool reverse=false);
	// Notification
	void SetNotify( HWND hwnd, int message )
	{
		hwndParent = hwnd;
		msgParent = message;
	}
protected:
	virtual void LoadLimit(){}
	bool DeleteWords(CBDeleteWordsHistory cb=NULL, DELETEWORDPARAM *dwp=NULL);
	virtual void SaveQueOutProc(TOFile &tof);
#if 0
	virtual void ReadLine(TIFile &tif, tnstr &line, int dummy)
	{
		//return tif.getline(line);
	}
	virtual void WriteLine(TOFile &tof, const tchar *line, int dummy)
	{
		//return tof.putline(line);
	}
#endif
	// file size limitation
	virtual bool IsLimitOver(long pos)
	{
		LoadLimit();
		return LimitUpper != 0 && pos > LimitUpper;
	}
	virtual bool IsLimitLower(long filesize)
	{
		LoadLimit();
		return filesize >= LimitLower;
	}
};

void SetHistoryNotify( HWND hwnd, int message );
int GetHistoryMaxFileSize();
void SetHistoryMaxFileSize(int size);

void AddHistory( const tchar *word, const tchar *url=NULL );
void SaveHistoryFile( );

void HistoryProc();
void DeleteOldestWords( );

void DeleteWordsHistory(tnstr_vec &word);

// 一語だけの削除
bool DeleteWordHistory( const tchar *word, bool all=false );

// 一語だけ、すべての同一単語の削除
inline void DeleteWordsHistory( const tchar *word )
	{ DeleteWordHistory( word, true ); }

bool DeleteNoRecHistory( );

int GetCountWordHistory( );

#define	HF_POPUPWINDOW	0x0001
#define	HF_QUICKPOPUP	0x0002
#define	HF_POPUP		0x0003
#define	HF_WORD			0x0004
#define	HF_ALL			0xFFFF

#define	HF_SORT_NONE	0x0000
#define	HF_SORT_DATE	0x0001	// 履歴登録順
#define	HF_SORT_ALPHA	0x0002
#define	HF_SORT_FREQ	0x0004
#define	HF_SORT_CHAR	0x0008	// number of characters
#define	HF_SORT_RECORD	0x0010	// 辞書登録順
#define	HF_SORT_REVERSE	0x8000

extern int HistoryFlag;

// History Array for frequency //
struct HISTFREQ {
	tnstr Word;
	int Num;
	HISTFREQ( const tchar *word )
		:Word( word )
		{ Num = 1; }
};

class TWordHistory : public FlexObjectArray<HISTFREQ> {
typedef FlexObjectArray<HISTFREQ> super;
protected:
	bool ExtraMode;
	static int _SortByNum( HISTFREQ **s1, HISTFREQ **s2 );
	static int _SortByNumWord( HISTFREQ **s1, HISTFREQ **s2 );
	static int _SortByChar( HISTFREQ **s1, HISTFREQ **s2 );
public:
	TWordHistory();
	void EnableExtraMode(){ ExtraMode = true; }
	bool IsExtraMode() const { return ExtraMode; }
	void AddAlphaSorted( const tchar *word );	// alphabet sort付きadd
	//void Add( const tchar *word, bool movetop );	// alphabet sort付きadd
	int BSearch( const tchar *word );
	void SortByNum( )
		{ sort( _SortByNum ); }
	void SortByNumWord()
		{ sort( _SortByNumWord ); }
	void SortByChar()
		{ sort( _SortByChar ); }
	void SortReverse( );
	void RemoveNull();
};

//---------------------------------------------------------------------------
// TFastReadHistory class
//---------------------------------------------------------------------------
#include "perllib.h"
// It is used for reading the many histories from the file to make the Add method faster.
class TFastReadHistory {
protected:
	TWordHistory &History;
	int_tmap WordMap;	// The word map to check the duplication of the word.
public:
	TFastReadHistory(TWordHistory &hist)
		:History(hist)
	{
	}
	virtual ~TFastReadHistory()
	{
	}
	void Add(const tchar *line, bool movetop);
	// Call thie method after all data added into this class.
	void Sort()
	{
		// Shrink the array.
		History.RemoveNull();
	}
};

// word -> URLs mapping
class TWordUrlList : public map< tnstr, vector<tnstr> > {
public:
	void Add(const tchar *word, const tchar *url)	// urlの重複チェックなし/高速
		{ (*this)[word].push_back(url); }
	void AddDupCheck(const tchar *word, const tchar *url);
};

bool LoadHistory( TWordHistory &WordHist, bool alphasort, bool reverse, int offset=0 );
bool LoadHistory( TWordHistory &WordHist, int sort, int offset=0, TWordUrlList *urls=NULL );
tnstr GetHistoryFileName();
void SetHistoryFileName(const tchar *filename);
TOFile *CreateHistoryFile(const tchar *filename);
//void CheckHistoryFileFormat(const tchar *filename);
bool MergeHistoryFile(const tchar *srcfile, bool convert_format=false);
bool IsHistoryFileUpdated(__int64 &ft);

#endif	//__HISTFILE_H

