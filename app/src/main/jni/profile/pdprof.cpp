#include "tnlib.h"
#pragma	hdrstop
#include <shlobj.h>
#include	"pdconfig.h"
#include	"multidic.h"
#include	"dicname.h"
#include	"pdprof.h"
#include	"dicgrp.h"
#include	"harray.h"
#include	"winmsg.h"
#include "filestr.h"

#include "UtyUI.h"
#include "wpdcom.h"

// Default Values //
#define	VUPNotifyDef		true
#define	TextFileBOMDef		true
#define	SaveAsRTFDef		false
#define	TextFileCodeDef		TFM_DEFAULT
#define	OleFullDef			false
#define	AutoDownloadDef		true
#define	AltEnterDef			false
#define	TaskTrayDef			false
//#if USE_BM
#define	OldMemoryDef		false
//#else
//#define	OldMemoryDef		true
//#endif
#define	WebWindowDef		true
#define	OldWheelScrollDef	false
#define	OnDemandAccessDef	true
#define	AutoDicCheckDef		false

#define	DELIM_GROUPNAME		':'		//TODO: 辞書グループ名レジストリ区切り文字

//TODO: いずれ汎用的なところへ
// globalなvariableを終了時に自動削除
template <class T>
class auto_delete {
protected:
	T **ptr;
public:
	auto_delete(T *&p)
		:ptr(&p)
	{}
	~auto_delete()
	{
		if (*ptr){
			delete *ptr;
		}
	}
};

PdProfile prof;
TRegKey *CurGroupReg = NULL;
static auto_delete<TRegKey> _CurGroupReg(CurGroupReg);

tnstr LastExePath;

const tchar CompanyName[] = _T(REG_COMPANYNAME);
const tchar AppRegName[] = _T(REG_APPNAME);
static const tchar AppRegPath[] = _T(REG_SOFTWARE) _T("\\") _T(REG_COMPANYNAME) _T("\\") _T(REG_APPNAME);
//const tchar AppRegLoc[] = _T("SOFTWARE\\ReliefOffice\\CurrentVersion\\App Paths\\PDIC2.EXE");
const tchar AppLanguage[] = _T("JP");

int uniq = 0;

static UINT fflag = SQM_JAPA;			// IME制御
static UINT yflag = 0;			// 全角半角変換

//#ifndef SMALL
#if 0
static BOOL IsDigit( const tchar *str, DWORD &val )
{
	const tchar *p = str;
	if ( *p == '-' )
		p++;
	while ( *p ){
		if ( (tuchar)*p < '0' || (tuchar)*p > '9' ){
			// 非数字
			return FALSE;
		}
		p = CharNext( p );
	}
	if ( p != str ){
		val = _ttoi( str );
		return TRUE;
	}
	return FALSE;
}

// INI to Regの仮想関数
//TODO: 大幅に変更したので動作確認必要
bool PdIni2Reg::Record( TRegKey *curkey, const tchar *valuename, const tchar *data )
{
	if ( !_tcscmp( sectionname, PFS_LABELING ) || sectionname[0] == '-' ){
		// ラベリング
		return true;
	}
	DWORD val;
	if ( sectionname[0] == '=' ){
		// 辞書グループ
		TRegKey *key = regProf->SetSection( PFS_GROUP );
		TRegKey *child = key->CreateKey((const tchar *)sectionname + 1);
		if ( IsDigit( data, val ) ){
			child->WriteInteger( valuename, val );
		} else {
			child->WriteString( valuename, data );
		}
		delete child;
		return true;
	}
	if ( !_tcscmp( sectionname, PFS_GROUP ) ){
		// 辞書グループ（ファイルリスト）
		regProf->SetSection( sectionname );
		const tchar *p = data;
		const tchar *top = p;
		tnstr_vec array;
		while ( *p ){
			if ( *p == ' ' ){
				// 区切り
				array.add( new tnstr( top, STR_DIFF( p, top ) ) );
				p++;
				top = p;
				continue;
			}
			p = CharNext( p );
		}
		if ( top != p )
			array.add( new tnstr( top ) );
		if ( array.get_num() ){
			regProf->GetSectionKey()->Write( valuename, array, DELIM_GROUPNAME );
		}
		return true;
	}
	if ( IsDigit( data, val ) ){
		// 数字
		regProf->SetSection( sectionname );
		curkey->Write( valuename, val );
		return true;
	} else {
		// 非数字
		regProf->SetSection( sectionname );
		return Ini2Reg::Record( curkey, valuename, data );
	}
}
#endif	//

PdProfile::PdProfile( )
	:super( AppRegPath )
{
	rkCommon = NULL;
	Loaded = false;
	NetAutoDownload = AutoDownloadDef;
	OldMemory = OldMemoryDef;
	WebWindow = WebWindowDef;
	OldWheelScroll = OldWheelScrollDef;
	OnDemandAccess = OnDemandAccessDef;
	AutoDicCheck = AutoDicCheckDef;
}

PdProfile::~PdProfile( )
{
	if (rkCommon) delete rkCommon;
}

void PdProfile::Setup()
{
	if (Loaded)
		return;
	NetAutoDownload = ReadInteger(PFS_NETWORK, PFS_AUTODOWN, NetAutoDownload);
	OldMemory = rkCommon->ReadInteger(PFS_OLDMEM, OldMemory);
	WebWindow = rkCommon->ReadInteger(PFS_WEBWINDOW, WebWindow);
	OldWheelScroll = rkCommon->ReadInteger(PFS_WHEELSCROLL, OldWheelScroll);
	OnDemandAccess = rkCommon->ReadInteger(PFS_ONDEMANDACCESS, OnDemandAccess);
	AutoDicCheck = ReadInteger(PFS_BACKUP, PFS_DICCHECK, AutoDicCheck);
	Loaded = true;
}

void PdProfile::SetIniMode(int ini_level)
{
	IniModeLevel = ini_level;
	if (ini_level){
		// Set to the ini mode.
		tnstr iniName = CommandPath;
		iniName += GetBaseName(Path);
		iniName += _t(".ini");
		super::SetIniFile(iniName);
	} else {
		// Set to the registry mode.
		super::SetIniFile(NULL);
	}
}


TRegKey *PdProfile::GetCommonKey()
{
	if (!rkCommon) rkCommon = CreateKey(PFS_COMMON);
	return rkCommon;
}

tnstr MakeIniModeRelPath(const tchar *path)
{
	if (!path[0])
		return NULL;
//	return MakeRelativeDrive(path, CommandPath);
	return MakeRelativePath(path, CommandPath, true);
}

tnstr MakeIniModeFullPath(const tchar *path)
{
	if (!path[0])
		return NULL;
//	return MakeAbsoluteDrive(CommandPath, path);
	return MakeAbsolutePath(CommandPath, path);
}

// Path //
tnstr PdProfile::GetDictionaryPath(bool create)
{
	tnstr path = ReadPath(PFS_DICTIONARYPATH, NULL);
	if (path.exist()){
		return AddYen(path);
	}
	if (IsIniMode()){
		path = CommandPath+_t("dic");
	} else {
		path = GetSpecialFolder(CSIDL_PERSONAL);
		if (path.empty()){
			return AddYen(CommandPath);
		}
		path += _t("\\Pdic");
	}
	if (create){
		_tmkdir(path);
	}
	return AddYen(path);
}

void PdProfile::SetDictionaryPath(const tchar *path)
{
	WritePath(PFS_DICTIONARYPATH, path);
}

tnstr PdProfile::GetEstDBPath()
{
	return GetDictionaryPath();	// 当面は辞書と同じ
}

tnstr PdProfile::GetPersonalPath(bool create)
{
	tnstr path = ReadPath(PFS_WORKINGPATH, NULL);
	if (path.exist()){
		return AddYen(path);
	}
	if (IsIniMode()){
		path = CommandPath+_t("setup");
	} else {
		path = GetSpecialFolder(CSIDL_PERSONAL);
		if (path.empty()){
			return CommandPath;
		}
		path += _t("\\Pdic");
	}
	if (create){
		_tmkdir(path);
	}
	return AddYen(path);
}

tnstr PdProfile::GetWorkingPath()
{
	return ReadPath(PFS_WORKINGPATH, NULL);
}
void PdProfile::SetWorkingPath(const tchar *path)
{
	WritePath(PFS_WORKINGPATH, path);
}

// Language Processor module path
tnstr PdProfile::GetLangProcPath(bool create)
{
	tnstr path = GetPersonalPath(create);
	path += _t("\\LangProc");
	if (create){
		_tmkdir(path);
	}
	return AddYen(path);
}

tnstr PdProfile::GetSWDPath(bool create)
{
	tnstr path = GetLangProcPath(create);
	path += _t("\\SWD");
	if (create){
		_tmkdir(path);
	}
	return AddYen(path);
}

tnstr PdProfile::GetDefIrregDicName(bool save)
{
	return GetPersonalFile(_LT(TID_IrregDic), save);
}

tnstr PdProfile::GetAdditionalIrregDicName(bool save)
{
	return GetPersonalFile(_LT(TID_IrregDicAdd), save);
}

tnstr PdProfile::GetLabelFileName(bool save)
{
	return GetPersonalFile(_LT(TID_FILE_LABELS), save);
}

tnstr PdProfile::GetTemplatePath(const tchar *filename, bool save)
{
	return GetPersonalFile(filename, save);
}

// Common method for personal files.
tnstr PdProfile::GetPersonalFile(const tchar *filename, bool save)
{
	tnstr path = GetPersonalPath() + filename;
	if (save){
		// Save path.
		return path;
	} else {
		// Load path.
		if (fexist(path))
			return path;
		return CommandPath + filename;
	}
}

tnstr PdProfile::GetCCTablePath()
{
	return CommandPath;
}

// .ctt file path
// lang proc - definition table path
tnstr PdProfile::GetCTTPath()
{
	return CommandPath;
}

tnstr PdProfile::GetPronTableFileName()
{
	return CommandPath + _LT(TID_FILE_PRON_TABLE);
}

// Environments //
UINT PdProfile::GetJEFlags(int sqm)
{
	return (
		( ( fflag & sqm ) ? JE_IME : 0 )
			| ( ( yflag & sqm ) ? JE_ZENHAN : 0 )
	);
}
void PdProfile::SetJEFlags(int sqm, int type, bool on)
{
	__assert(!(type&~(JE_IME|JE_ZENHAN)));
	if (type & JE_IME){
		if (on)
			fflag |= sqm;
		else
			fflag &= ~sqm;
	}
	if (type & JE_ZENHAN){
		if (on)
			yflag |= sqm;
		else
			yflag &= ~sqm;
	}
}

tnstr PdProfile::GetDefFontName()
{
	return _LT(TID_DEF_FONT_NAME);
}
tnstr PdProfile::GetDefPronFontName()
{
	return _LT(TID_DEF_PRON_FONT_NAME);
}
bool PdProfile::IsFastSearchPronEnabled()
{
	return rkCommon->ReadInteger(PFS_FASTSEARCH_PRONENABLED, false);
}

// Global Configurations //

bool PdProfile::IsDebug()
{
	return rkCommon->ReadInteger(PFS_DEBUG, 0);
}

bool PdProfile::IsVUPNotify()
{
	return ReadInteger(PFS_VERSION, PFS_VUPNOTIFY, VUPNotifyDef);
}

bool PdProfile::IsTextFileBOM()
{
	return rkCommon->ReadInteger(PFS_TEXTFILEBOM, TextFileBOMDef);
}

#if 0
bool PdProfile::SaveAsRTF()
{
	return rkCommon->ReadInteger(PFS_SAVEASRTF, SaveAsRTFDef);
}
#endif

// return:
//	TFM_xxx defined in tnlib2/file.h
int PdProfile::GetTextFileCode()
{
	return rkCommon->ReadInteger(PFS_TEXTFILECODE, TextFileCodeDef);
}

bool PdProfile::IsOleFull()
{
	return rkCommon->ReadInteger(PFS_OLEFULL, OleFullDef);
}

bool PdProfile::IsNetAutoDownload()
{
	Setup();
	return NetAutoDownload;
}

bool PdProfile::IsAltEnter()
{
	return rkCommon->ReadInteger(PFS_ALTENTER, AltEnterDef);
}

bool PdProfile::IsTaskTray()
{
	return rkCommon->ReadInteger(PFS_TASKTRAY, TaskTrayDef);
}

bool PdProfile::IsOldMemory()
{
	Setup();
	return OldMemory;
}

void PdProfile::SetOldMemory(bool on)
{
	if (IsOldMemory()!=on){	// 違うときのみ保存
		rkCommon->WriteInteger(PFS_OLDMEM, on);
		OldMemory = on;
	}
}

bool PdProfile::IsWebWindow()
{
	Setup();
	return WebWindow;
}

bool PdProfile::IsOldWheelScroll()
{
	Setup();
	return OldWheelScroll;
}

void PdProfile::SetOldWheelScroll(bool on)
{
	Setup();
	if (OldWheelScroll!=on){
		rkCommon->WriteInteger(PFS_WHEELSCROLL, OldWheelScroll);
		OldWheelScroll = on;
	}
}
bool PdProfile::IsOnDemandAccess()
{
	Setup();
	return OnDemandAccess;
}
void PdProfile::SetOnDemandAccess(bool on)
{
	Setup();
	if (OnDemandAccess!=on){
		OnDemandAccess = on;
		rkCommon->WriteInteger(PFS_ONDEMANDACCESS, OnDemandAccess);
	}
}
bool PdProfile::IsAutoDicCheck()
{
	Setup();
	return AutoDicCheck;
}
void PdProfile::SetAutoDicCheck(bool on)
{
	Setup();
	if (AutoDicCheck!=on){
		AutoDicCheck = on;
		prof.WriteInteger(PFS_BACKUP, PFS_DICCHECK, on);
	}
}
void PdProfile::WritePath(const tchar *key, const tchar *str, TRegKey *reg)
{
	tnstr val;
	if (IsIniMode()){
		val = MakeIniModeRelPath(str);
		str = val;
	}
	if (reg)
		reg->WriteString(key, str);
	else
		rkCommon->WriteString(key, str);
}
void PdProfile::WritePath(int id, const tchar *str, TRegKey *reg)
{
	WritePath(_LT(id), str, reg);
}
tnstr PdProfile::ReadPath(const tchar *key, const tchar *defval, TRegKey *reg)
{
	tnstr val = (reg ? reg->ReadString(key, defval) : rkCommon->ReadString(key, defval));
	if (!IsIniMode()){
		return val;
	}
	return MakeIniModeFullPath(val);
}
tnstr PdProfile::ReadPath(int id, const tchar *defval, TRegKey *reg)
{
	return ReadPath(GetString(id), defval, reg);
}

bool PdProfile::WritePath(const tchar *key, tnstr_vec &array, int delim, TRegKey *reg)
{
	if (!IsIniMode()){
		return reg ? reg->Write(key, array, delim) : Write(key, array, delim);
	}
	tnstr_vec tmp;
	foreach_tnstr_vec(array, it){
		tmp.push_back(MakeIniModeRelPath(*it));
	}
	return reg ? reg->Write(key, tmp, delim) : Write(key, tmp, delim);
}
void PdProfile::ReadPath(const tchar *key, tnstr_vec &array, int delim, TRegKey *reg)
{
	if (!IsIniMode()){
		if (reg) reg->Read(key, array, delim); else Read(key, array, delim);
		return;
	}
	tnstr_vec tmp;
	if (reg) reg->Read(key, tmp, delim); else Read(key, tmp, delim);
	array.clear();
	foreach_tnstr_vec(tmp, it){
		array.push_back(MakeIniModeFullPath(*it));
	}
}

void PdProfile::RootKeyOpened()
{
	super::RootKeyOpened();
	GetCommonKey();
}

void SaveAllProfile( )
{
	TRegKey &reg = *prof.GetCommonKey();
//	reg.Write( PFS_GROUPSEL, dicgrp.GetSel() );
#ifndef SML
	if ( deffile.GetTop( ) ){
		reg.WriteString( PFS_DEFFILE, deffile.GetTopString() );
	}
#endif

#ifndef SMALL
	SaveImeProfile();
//	prof.WriteInteger( PFS_AUTOOPTIMIZE, optflag );	// not yet
#endif

//	TLabelDlg::SaveProfile( );
}

void LoadAllProfile( )
{
	// グループ辞書の読み込み
	ReadGroupDicList( );

	TRegKey &reg = *prof.GetCommonKey();
#ifndef SML
	tnstr s = reg.ReadString(PFS_DEFFILE);
	if (s[0])
		deffile.AddString(s);

	LoadImeProfile();

//	optflag = reg.ReadInteger( PFS_AUTOOPTIMIZE, optflag );
#endif

	// グループ辞書の選択(COMMON)
//	int wTemp = reg.ReadInteger( PFS_GROUPSEL, 0 );
//	dicgrp.SetSel( wTemp );
}

void SaveImeProfile()
{
	TRegKey &reg = *prof.GetCommonKey();
	reg.WriteInteger( PFS_ZEN2HAN, yflag );
	reg.WriteInteger( PFS_IMECTRL, fflag );
}

void LoadImeProfile()
{
	TRegKey &reg = *prof.GetCommonKey();
	yflag = reg.ReadInteger( PFS_ZEN2HAN, yflag );
	fflag = reg.ReadInteger( PFS_IMECTRL, fflag );
}

#ifdef USE_FILEHISTORY
void WriteHistory( const tchar *section, HistoryArray &ha, bool path )
{
	autoptr<TRegKey> reg( prof.CreateKey( section ) );
	// delete section
	reg->EraseAllEntry();
	for (int i=0;i<ha.GetCount();i++){
		tchar entry[10];
		_itot(i,entry,10);
		if (path)
			prof.WritePath(entry, ha[i].string, reg);
		else
			reg->WriteString(entry, ha[i].string);
	}
}

void ReadHistory( const tchar *section, HistoryArray &ha, bool path )
{
	ha.Clear();
	autoptr<TRegKey> reg(prof.CreateKey( section ));
	for (int i=0;;i++){
		tchar entry[10];
		_itot(i,entry,10);
		tnstr str;
		if (path)
			str = prof.ReadPath( entry, _t(""), reg );
		else
			str = reg->ReadString( entry );
		if (str.IsEmpty())
			break;
		ha.AddString( str );
	}
}
#endif	// USE_FILEHISTORY



UINT LoadInitWindow( TRegKey &reg, HWND hwnd, const tchar *pfsWndSize, const tchar *pfsWndShow, int nCmdShow, RECT *_rc )
{
#ifndef WINCE
	RECT rcScr;
	GetScreenSize(hwnd, &rcScr);
	int sx = rcScr.right - rcScr.left;
	int sy = rcScr.bottom - rcScr.top;

	RECT r;
	memset( &r, 0, sizeof(RECT) );
	UINT show = 0;
	reg.Read( pfsWndSize, r );
	reg.Read( pfsWndShow, show );
	if ( r.bottom - r.top ){
		// profileにあった場合
		if ( r.right - r.left > sx )
			r.right = sx + r.left;
		if ( r.bottom - r.top > sy )
			r.bottom = sy + r.top;
	} else {
		// profileにない場合
		r.left = r.top = 0;
		r.right = rcScr.right;
		r.bottom = rcScr.bottom;
		if ( r.right > 700 ){
			r.right = r.right * 2 / 3;
			r.bottom = r.bottom * 2 / 3;
		}
		MoveCenter( r );		// 立ち上げ時のウィンドウを中央に持っていくかどうか
	}

	if ( _rc )
		*_rc = r;

	if ( hwnd ){
//		MoveWindow( hwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, TRUE );	// Win95ではだめなそうな！
		STARTUPINFO si;
		WINDOWPLACEMENT wp;
		memset( &wp, 0, sizeof(wp) );
		wp.length = sizeof(wp);
		wp.flags = ( show ? WPF_RESTORETOMAXIMIZED : 0 );
		if ( nCmdShow != -1 ){
			GetStartupInfo( &si );
			if ( si.dwFlags & STARTF_USESHOWWINDOW ){
				if ( (si.wShowWindow == SW_RESTORE || si.wShowWindow == SW_SHOWNORMAL ) && show ){
					wp.showCmd = SW_SHOWMAXIMIZED;
				} else {
					wp.showCmd = si.wShowWindow;
				}
			} else {
				wp.showCmd = show ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
			}
		} else {
			wp.showCmd = show ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
		}

		wp.ptMinPosition.x = wp.ptMinPosition.y = 0;
		wp.ptMaxPosition.x = wp.ptMaxPosition.y = 0;
		wp.rcNormalPosition = r;
		SetWindowPlacement( hwnd, &wp );
	}

	if ( show ){
#if 0
		// アイコン化されているときはSW_SHOWMAXIMIZEDをするとまずい
		// のはずなのだが、nCmdShowとwp.showCmdは必ず、SW_SHOWDEFAULT？？？？
		WINDOWPLACEMENT wp;
		memset( &wp, 0, sizeof(WINDOWPLACEMENT) );
		wp.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement( &wp );
		if ( wp.showCmd == SW_SHOWMINIMIZED ){
			wp.flags = WPF_RESTORETOMAXIMIZED;
			SetWindowPlacement( &wp );
		} else
#endif
		{
			show = SW_SHOWMAXIMIZED;
		}
	}
    return show;
#else
	return SW_SHOW;
#endif
}
void SaveInitWindow( TRegKey &reg, HWND hwnd, const tchar *pfsWndSize, const tchar *pfsWndShow )
{
#ifndef WINCE
	WINDOWPLACEMENT wp;
	wp.length = sizeof( WINDOWPLACEMENT );
	GetWindowPlacement( hwnd, &wp );
	UINT showProf = wp.showCmd == SW_SHOWMAXIMIZED;
	RECT rcProf = wp.rcNormalPosition;
	reg.Write( pfsWndSize, rcProf );
	reg.Write( pfsWndShow, showProf );
#endif
}

////////////////// RegProfile2 ///////////////////////////////////////////////

RegProfile2::RegProfile2(const tchar *_path)
	:super(_path)
{
}

RegProfile2::~RegProfile2( )
{
	CloseAll( );
}

#if 0
bool CopyValue( TRegKey *srcreg, TRegKey *destreg )
{
	DWORD maxvaluename, maxvaluedata;
	if ( !srcreg->QueryInfoKey( &maxvaluename, &maxvaluedata ) ){
		maxvaluename = 512;
		maxvaluedata = 4096;
	}
	tchar *buf = new tchar[ maxvaluedata + 1 ];
	for ( int i=0;;i++ ){
		tnstr name;
		DWORD len = maxvaluedata;
		DWORD type;
		if ( !srcreg->EnumValue( i, name, buf, &len, &type ) ){
			break;
		}
		destreg->Set( name, buf, len, type );
	}
	delete[] buf;
	return true;
}
#endif

// registry to registry/ini
void CopyRegistry(TRegKeyR *src, TRegKey *dst)
{
	tnstr_vec names;
	src->EnumValues(names);
	foreach_tnstr_vec(names, n){
		DWORD maxlen;
		DWORD type;
		if (src->QueryValue(*n, NULL, maxlen, &type)){
			switch (type){
				case REG_DWORD:
					dst->WriteInteger(*n, src->ReadInteger(*n, 0));
					break;
				case REG_SZ:
					dst->WriteString(*n, src->ReadString(*n, _t("")));
					break;
				default:
					__assert__;
					break;
			}
		} else {
			__assert__;
		}
	}
}

// registry/ini to registry/ini
void CopyRegistry(TRegKey *src, TRegKey *dst)
{
	if (src->RegMode()){
		CopyRegistry((TRegKeyR*)src, dst);
		return;
	}
	// for .ini file
	tnstr_vec names;
	src->EnumValues(names);
	foreach_tnstr_vec(names, n){
		dst->WriteString(*n, src->ReadString(*n, _t("")));
	}
}

void Read(TRegKey *reg, const tchar *section, diclist_t &dl)
{
	ReadArray(reg, section, dl_array(dl), dl_num(dl));
}

void ReadArray(TRegKey *reg, const tchar *section, int *array, int num)
{
	tnstrbuf name;
	for (int i=0;i<num;i++){
		name.clear();
		if (i==0){
			name << section;
		} else {
			name << section << itos(i);
		}
		array[i] = reg->ReadInteger(name, array[i]);
	}
}

void Write(TRegKey *reg, const tchar *section, const diclist_t &dl)
{
	WriteArray(reg, section, dl_array(dl), dl_num(dl));
}

void WriteArray(TRegKey *reg, const tchar *section, const int *array, int num)
{
	tnstrbuf name;
	for (int i=0;i<num;i++){
		name.clear();
		if (i==0){
			name << section;
		} else {
			name << section << itos(i);
		}
		reg->WriteInteger(name, array[i]);
	}
}
