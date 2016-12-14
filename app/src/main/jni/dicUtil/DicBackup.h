#ifndef DicBackupH
#define DicBackupH

#include "dic.h"

class Pdic;

class TDicBackup : public TPdicBackup {
protected:
	typedef map<Pdic*, int> mod_counter_t;
	mod_counter_t LastBackupModCounter;
	typedef map<tnstr, int> brokenlist_t;	// filename -> BrokenStatus mapping
	brokenlist_t broken_dics;
protected:
	// backup parameters
	static bool Enabled;
	static tnstr BackupPath;
	static int MaxCount;	// 最大バックアップ数
	static int MaxDicSize;
	static unsigned Interval;	// [Sec]
	static bool DicBackup;
//	static bool DicCheck;
	static int MaxDicCheckSize;
	static bool InProgress;

	DWORD LastUpdateTime;
	bool DicChecking;
public:
	TDicBackup();
	virtual ~TDicBackup();
	void Initialize();
	void Clear();
	__override void NotifyMod(PdicBase *dic);
	bool IsBackupNeeded(Pdic *dic);
	int IdleProc();
	void TimerExpired();

	const tchar *GetBrokenDic();
	void SetDicFixed(const tchar *filename);

#ifdef _DEBUG
	int _CheckDictionary(const tchar *dicname)
	{
		return CheckDictionary(dicname);
	}
#endif
	
protected:
	virtual void NotifyError(Pdic *dic) = 0;

	void DoBackup();
	bool DoBackup(Pdic *dic);
	int FindBackupFiles(const tchar *filename, tnstr_vec &files);
	void GetBackupFileName(const tchar *filename, tnstrbuf &backupname);
	int GetBackupBaseName(const tchar *filename, tnstrbuf &basename);
	int CheckDictionary(const tchar *dicname);
	void DeleteOldFiles(const tchar *filename);
};

extern TDicBackup *DicBackup;

#endif

