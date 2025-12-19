// 辞書のオープンクローズなどの処理
#include "tnlib.h"
#pragma	hdrstop

#include	"multidic.h"
#include	"id.h"
#include	"winmsg.h"
#include	"dicgrp.h"
#include	"winsqu.h"
#include	"wpdcom.h"
#include	"pdprof.h"
#include	"squfont.h"
#include "msgdlg.h"
#include "browse.h"
#include "filestr.h"
#include "Dicproc.h"
#ifdef _Windows
#include	"retproc.h"
#include "copydata.h"
#include "histfile.h"
#endif
#ifdef USE_TTS
#include "tts.h"
#endif
#include "DicBackup.h"

#define	MYCLASSNAME	_t("TPdicMain.UnicodeClass")		// The class name of the main window.
#define	MYCLASSNAME_W32	_t("PDICW")		// The class name of the main window.

// 返り値：
//		0 正常
//		1 正常終了だが、一度エラーを発生した。(hLogoがある場合は、それをCloseした）
//		0未満 : 異常終了（中止）
int OpenDic( MPdic &dic, DICNAME &dicname )
{
	int r;
	tnstr filename;
	dicname.GetDicName( filename );
#ifdef EPWING
	tnstr epwname;
	dicname.GetEPWingName( epwname );
	tnstr gconvname;
	dicname.GetGConvName( gconvname );
#endif
#ifdef USE_FILELINK
	tnstr flinkpath;
	dicname.GetFileLinkPath( flinkpath );
#endif

	r = dic.Open( -1, filename,
#ifdef EPWING
		epwname, gconvname,
#else
		NULL, NULL,
#endif
#ifdef USE_FILELINK
		flinkpath,
#else
		NULL,
#endif
		(dicname.readonly ? DICFLAG_READONLY : DICFLAG_NONE)
		| (dicname.FastDB ? DICFLAG_FASTDB_CREATE : DICFLAG_NONE)
#if INETDIC
		| (dicname.internet ? DICFLAG_INTERNET : DICFLAG_NONE)
#endif
		| DICFLAG_TXDUP
		 );
	if (r==0){
		dic[dic.GetDicNum()-1].SetBackup(DicBackup);
	}
	return r;
}

// filenameそれ自身以外で辞書を探す
bool FindDic(const tchar *filename, tnstr &fullpath)
{
	const tchar *basename = GetFileName(filename);
	if ( basename == filename )	// pathが含まれていなかった場合
	{
#ifdef _Windows
		fullpath = AddYen(LastExePath);
		fullpath += basename;
		if ( fexist( fullpath ) ){
			// 前回実行パスに辞書が存在した
			return true;
		}
#endif
		fullpath = prof.GetDictionaryPath(false);
		fullpath += basename;
		if (fexist(fullpath))
			return true;	// exists in the default dictionary path
	} else {
		// pathが含まれている場合
		fullpath = AddYen(CommandPath);
		fullpath += basename;
		if (fexist(fullpath))
			return true;	// 現在のexe fileと同じフォルダに存在した
	}
	return false;	// not found
}

#ifdef _Windows
// メインウィンドウ以外で修正した辞書を知らせる
void NotifyModifiedDic( const tchar *filename )
{
	if ( dicgrp.GetNum() && dicgrp.GetSel() >= 0 ){
		DicNames names;
		dicgrp.GetCurNames( names );
		for ( int i=0;i<names.get_num();i++ ){
			if ( IsSameFile( names[i].name, filename ) ){
				// Modified
				break;
			}
		}
	}
}

// true : このPDICでオープンでき、辞書グループが正常にオープンできるはず
// false : ほかのPDICでオープンされている。辞書グループが指定されていない。辞書グループが変更されていない
bool ChangeDic( const tchar *newgrpname, int oldsel )
{
	if ( newgrpname ){
#ifndef SMALL
		HWND hwnd = FindDicGroup(newgrpname);
		if ( hwnd ){
			// ほかのPDICでオープン済みのグループ
			MoveToTop( hwnd );
			dicgrp.SetSel( oldsel );	// 元に戻す
			return false;
		} else
#endif
		{
			if ( oldsel >= dicgrp.get_num() || _tcscmp( dicgrp[oldsel].name, newgrpname ) ){
				// Modified
				return true;
			}
			return false;
		}
	}
	return false;
}

// PDICを探す
class TFindPdic {
	HWND hwndFind;
	tnstr MyClassName;
	tnstr GroupName;
	bool ForWin32;
public:
	TFindPdic(const tchar *grpname=NULL, bool forWin32=false);
	HWND Execute();
protected:
	static BOOL CALLBACK EnumWindowsProc( HWND hwnd, LPARAM lParam );
};
TFindPdic::TFindPdic(const tchar *grpname, bool forWin32)
{
	if (grpname)
		GroupName = grpname;
	hwndFind = NULL;

	ForWin32 = forWin32;
	MyClassName = forWin32 ? MYCLASSNAME_W32: MYCLASSNAME;
}
HWND TFindPdic::Execute()
{
	if (MyClassName.IsEmpty())
		return NULL;
	EnumWindows( EnumWindowsProc, (LPARAM)this );
	return hwndFind;
}
BOOL CALLBACK TFindPdic::EnumWindowsProc( HWND hwnd, LPARAM lParam )
{
	tchar clsname[80];
	if ( !GetClassName( hwnd, clsname, tsizeof(clsname)-1 ) ) return TRUE;
	clsname[tsizeof(clsname)-1] = '\0';
	TFindPdic *self = (TFindPdic*)lParam;
#ifndef SMALL	// SMALLでは起動直後でcheckするため必要なし
	DWORD procId;
	GetWindowThreadProcessId(hwnd, &procId);
	if (procId==GetCurrentProcessId())
		return TRUE;	// 自分は除外
#endif
	if ( _tcscmp( clsname, self->MyClassName) ) return TRUE;
#ifndef SMALL
	if (!self->GroupName.empty()){
		if (SendCopyDataCommand(hwnd, WMCD_CHECKOPEN, self->GroupName)){
			// found!!
			self->hwndFind = hwnd;
			return FALSE;
		}
	} else {
		if (self->ForWin32){
			// for PDIC for Win32
			// found the pdic
			self->hwndFind = hwnd;
			return FALSE;
		} else {
			if (SendCopyDataCommand(hwnd, WMCD_GETVERSION, NULL)==1){
				// found the pdic
				self->hwndFind = hwnd;
				return FALSE;
			}
		}
	}
	return TRUE;	// continue to enumerate
#else
	self->hwndFind = hwnd;
	return FALSE;	// SMALLでは簡略化
#endif
}

HWND FindPdic()
{
	TFindPdic find;
	return find.Execute();
}

HWND FindPdicWin32()
{
	TFindPdic find(NULL, true);
	return find.Execute();
}

bool OpenPdic(HWND hwndTarget, const tchar *grpname)
{
	return SendCopyDataCommand(hwndTarget, WMCD_OPENGROUP, grpname);
}

#ifndef SMALL
HWND FindDicGroup(const tchar *grpname)
{
	if ( !grpname ) return NULL;
	TFindPdic find(grpname);
	return find.Execute();
}
#endif	// ndef SMALL
#endif

