//---------------------------------------------------------------------------

#include "tnlib.h"
#pragma hdrstop
#include <dirent.h>

#include "DicBackup.h"
#include "dic.h"
#include "pfs.h"
#include "pdprof.h"
#include "filestr.h"
#include "fileio.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

// Backup policy
//
// Check policy

enum BrokenStatus {
	BRS_NONE,
	BRS_CANDIDATE,	// チェック対象辞書
	BRS_FOUND,		// 問題が見つかった
};

bool TDicBackup::Enabled = true;
tnstr TDicBackup::BackupPath;
int TDicBackup::MaxCount = 3;	// 最大バックアップ数
int TDicBackup::MaxDicSize = 0x800000;
unsigned TDicBackup::Interval = 3;	// 最小バックアップ間隔[sec]
bool TDicBackup::DicBackup = false;
//bool TDicBackup::DicCheck = true;	// 辞書チェック機能
int TDicBackup::MaxDicCheckSize = 10;	// 最大辞書チェックサイズ[MB]
bool TDicBackup::InProgress = false;

TDicBackup *DicBackup = NULL;

#ifdef _DEBUG
void CheckDictionary(const tchar *filename)
{
	if (DicBackup){
		int r = DicBackup->_CheckDictionary(filename);
		DBW("Check Result:%d", r);
	}
}
#endif

TDicBackup::TDicBackup()
{
	DicChecking = false;
}

TDicBackup::~TDicBackup()
{
	Clear();
}

void TDicBackup::Initialize()
{
#if 0
	Enabled = prof.ReadInteger(PFS_BACKUP, PFS_ENABLED, Enabled);
	BackupPath = prof.ReadString(PFS_BACKUP, PFS_PATH, BackupPath);
	MaxCount = prof.ReadInteger(PFS_BACKUP, PFS_MAXCOUNT, MaxCount);
	MaxDicSize = prof.ReadInteger(PFS_BACKUP, PFS_MAXDICSIZE, MaxDicSize);
	Interval = prof.ReadInteger(PFS_BACKUP, PFS_INTERVAL, Interval);
#endif
//	DicCheck = prof.ReadInteger(PFS_BACKUP, PFS_DICCHECK, DicCheck);

	LastUpdateTime = GetTickCount();
}
void TDicBackup::Clear()
{
	for (mod_counter_t::iterator it = LastBackupModCounter.begin();it!=LastBackupModCounter.end();){
		it->first->Release();
		LastBackupModCounter.erase(it++);
	}
}

// 辞書更新時に呼ばれる
// dic can be NULL to update 'LastUpdateTime'.
void TDicBackup::NotifyMod(PdicBase *_dic)
{
	if (_dic){
		Pdic *dic = dynamic_cast<Pdic*>(_dic);
		if (dic){
			if (LastBackupModCounter.count(dic)==0){
				dic->AddRef();
			}
			LastBackupModCounter[dic] = dic->ModifyCounter();	//TODO: ++でなくて大丈夫？
		}
	}
	LastUpdateTime = GetTickCount();
}

#if 0
bool TDicBackup::IsBackupNeeded(Pdic *dic)
{
	if (!dic)
		return false;

	DWORD now = GetTickCount();
	if (now-LastUpdateTime<Interval*1000){
		return false;
	}

	if (LastBackupModCounter.count(dic)==0)
		return false;

	int count = LastBackupModCounter[dic];
	if (count==dic->ModifyCounter()){
		DBW("Not modified???");
		return false;	// not modified. ←これありうる？
	}
	

	return true;
}
#endif

int TDicBackup::IdleProc()
{
	if (LastBackupModCounter.size()>0){
		// need backup
		return Interval*1000;
	}
	return 0;
}

void TDicBackup::TimerExpired()
{
	if (LastBackupModCounter.size()==0
		|| GetTickCount()-LastUpdateTime<Interval*1000
		|| InProgress)
		return;

	DoBackup();

	LastUpdateTime = GetTickCount();
}

const tchar *TDicBackup::GetBrokenDic()
{
	if (!broken_dics.size()) return NULL;
	foreach(broken_dics, it, brokenlist_t){
		if (it->second==BRS_FOUND){
			return it->first;	// dictionary filename
		}
	}
	return NULL;
}

void TDicBackup::SetDicFixed(const tchar *filename)
{
	if (broken_dics.count(filename)){
		broken_dics.erase(filename);
	}
}

void TDicBackup::DoBackup()
{
	if (InProgress) return;

	InProgress = true;

	for (mod_counter_t::iterator it = LastBackupModCounter.begin();it!=LastBackupModCounter.end();){
		// mod counterに関係なく強制的にbackup
		if (DoBackup(it->first)){
			it->first->Release();
			LastBackupModCounter.erase(it++);
		} else it++;
	}

	InProgress = false;
}

bool TDicBackup::DoBackup(Pdic *dic)
{
	const tchar *filename = dic->GetFileName();

	if (DicBackup){
		// create a backup filename
		tnstrbuf backupname;
		GetBackupFileName(filename, backupname);
		// copy file
		if (!CopyFile(filename, backupname, FALSE)){	
			DBW("CopyFile failed: %d : %ws %ws", GetLastError(), dic->GetFileName(), backupname.c_str());
			return false;
		}
	}

	// dictionary check
	if (prof.IsAutoDicCheck() && !DicChecking){
		DicChecking = true;
		if (broken_dics.count(filename)==0){
			if (filesize(filename) / 0x100000 < MaxDicCheckSize){
		dbw("Dicchecking:%ws", filename);
				alog("start dic check");
				if (CheckDictionary(filename)==0){
					broken_dics[filename] = BRS_FOUND;
					NotifyError(dic);
					DicChecking = false;
					dbw("DicCheck-failed");
					alog("dic check failed");
					return false;
				}
				alog("dic check done");
#ifdef _DEBUG
				if (_tcsstr(filename, _t("broken"))){
					broken_dics[filename] = BRS_FOUND;
					NotifyError(dic);
				}
#endif
		dbw("DicCheck-end");
			}
		}
		DicChecking = false;
	}

	if (DicBackup){
		// delete old file
		DeleteOldFiles(filename);
	}
	
	return true;
}

int TDicBackup::FindBackupFiles(const tchar *filename, tnstr_vec &files)
{
	tnstrbuf targetname;
	int basename_len = GetBackupBaseName(filename, targetname);

	const tchar *ext = GetFileExtension(filename);	// get the actual extension

	_tDIR *d = _topendir( BackupPath );
	if (!d)
		return 0;
	struct _tdirent *ent;
	while ((ent = _treaddir(d)) != NULL){
		tnstr path(AddYen(BackupPath));
		path += ent->d_name;
		if (is_dir(path)){
			// directory
			continue;
		}
		if (_tcsnicmp(ent->d_name, targetname, basename_len)){
			continue;	// another file
		}
		const tchar *text = GetFileExtension(ent->d_name);
		if ((ext && !text) || (!ext && text)) continue;	// different file type
		if (ext){
			if (_tcscmp(ext, text)) continue;	// different file type
		}
		files.push(ent->d_name);
	}
	_tclosedir( d );

	return files.size();
}

void TDicBackup::GetBackupFileName(const tchar *filename, tnstrbuf &backupname)
{
	tnstrbuf basename;
	GetBackupBaseName(filename, basename);
	if (BackupPath.exist()){
		backupname = BackupPath;
		AddSlash(backupname);
	} else {
		const tchar *lp = GetFileName( filename );
		backupname.set(filename, STR_DIFF(lp, filename));
	}
	backupname += basename;
	time_t now;
	time(&now);
	struct tm *lt = localtime(&now);
	backupname += tnsprintf(_t("%04d%02d%02d%02d%02d%02d"),
		lt->tm_year + 1900,
		lt->tm_mon+1,
		lt->tm_mday,
		lt->tm_hour,
		lt->tm_min,
		lt->tm_sec
		);
	const tchar *ext = _tcschr(GetBaseName(filename), '.');
	if (ext){
		backupname += ext;
	}
}		

int TDicBackup::GetBackupBaseName(const tchar *filename, tnstrbuf &basename)
{
	int basename_len;
	const tchar *basep = GetBaseName(filename);
	const tchar *ext = _tcschr(basep, '.');
	if (ext){
		basename_len = STR_DIFF(ext, basep);
	} else {
		basename_len = _tcslen(basep);
	}
	basename.set(basep, basename_len);
	basename += _t("_");	// backup filename separator

	return basename_len+1;
}

#include "utydic.h"

class PdicUNoUI : public PdicU {
typedef PdicU super;
public:
	virtual bool ProcessUI(PDDICCHECKPARAM *dcp)
	{
		return false;
	}
};

// 1:OK
// -1:not checked (open error)
// 0:NG
int TDicBackup::CheckDictionary(const tchar *dicname)
{
	bool readonly = true;
	const bool fix = !readonly;

	DWORD start = GetTickCount();

	int r = -1;

	try {
		autoptr<PdicU> dic(new PdicUNoUI);

		if ( dic->OpenRetry( dicname, readonly ) == -1){
			return -1;
		}

		if (fix){
			dic->EnableFix( );
		}

		int emptyblock;
		if ( dic->checkindex( NULL ) != -1 ){
			dbw("in:%ws", dicname);
			r = dic->chk_lostlink( NULL, fix, &emptyblock );
			dbw("out:%d:%ws", r, dicname);
			if (r==0){
				r = dic->checkdata( NULL, emptyblock );
			}
		}
		dic->Close();
	} catch (...){
		DBW("CheckDictionary exception");
	}

	DBW("Checking Dictionary:%ws [%d msec]", dicname, GetTickCount()-start);

	return r==0;
}

void TDicBackup::DeleteOldFiles(const tchar *filename)
{
	tnstrbuf basename;
	int basename_len = GetBackupBaseName(filename, basename);

	while (1){
		tnstr_vec bkupfiles;
		FindBackupFiles(filename, bkupfiles);
		if (bkupfiles.size()<MaxCount)
			break;

		const tchar *oldest = NULL;
		foreach_tnstr_vec(bkupfiles, file){
			if (file->length()<basename_len){
				continue;	// unknown type file
			}
			if (oldest){
				const int cmplen = 4+2+2+2+2+2;	// ymdhms
				if (_tcsnicmp(oldest+basename_len, file->c_str()+basename_len, cmplen)>0){
					oldest = file->c_str();
				}
			} else {
				oldest = file->c_str();
			}
		}
		if (oldest){
			if (!DeleteFile(oldest)){
				// failed.
				DBW("DeleteFile failed : %d %ws", GetLastError(), oldest);
				break;
			}
		}
	}
}

