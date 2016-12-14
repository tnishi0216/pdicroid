#include "tnlib.h"
#pragma	hdrstop
#include	"multidic.h"
#include	"dicname.h"
#include	"pdprof.h"
#include	"dicgrp.h"
#include	"idh.h"
#include	"winmsg.h"
#include	"wpdcom.h"
#include	"filestr.h"
#include	"faststr.h"
#include	"dicproc.h"
#include "pdstrlib.h"

// 辞書グループ処理

#define	COMPAT		0	// 旧方式と同じ辞書グループ管理にする
						// 辞書グループの順番を取るために必要

#define	DELIM_GROUPNAME		'\t'	//TODO 5: グループ名のレジストリの区切り文字

#if !COMPAT
const tchar PFS_GROUP_NAMES[] = _t("::GroupNames::");
//VCL Bug:
// C++Builder/Code Guardをonにすると、global unicode string pointerを奇数番地にアライメントするバグがある。
// RegQueryValueEx()のkey nameは奇数番地アライメントのunicode stringを渡されるとエラーを返す。
// そのため、tchar[]で宣言。
#endif

SelGroupArray dicgrp;

//HistoryArray dicgrphist( 8 );
//bool dicgrphistfixed = false;	// コマンドライン指定による辞書グループ切り替えの指定

int DicGroup::LastId = 1;

static void _EraseWrite(TRegKey *key, const tchar *keyname, const tchar *value, bool path=true);
//static void _EraseWrite(TRegKey *key, const tchar *keyname, int value);

// return value must be deleted when it is never used.
TRegKey *CreateSectionU(const tchar *name, TRegKey *groupKey)
{
//#ifdef __PROTO
#if 0
	// 変なところに作ったkeyを移動
	if (!prof.IsIniMode()){
		TRegKeyR *oldkey = (TRegKeyR*)prof.GetRootKey()->OpenKey(name);
		if (oldkey){
			TRegKey *group = prof.SetSection(PFS_GROUP);
			if (group){
				TRegKey *dst = group->CreateKey(name);
				if (dst){
					CopyRegistry(oldkey, dst);
					delete dst;
				}
			}
			delete oldkey;
			// delete the old key.
			oldkey = (TRegKeyR*)prof.SetSection(name);
			if (oldkey){
				oldkey->EraseAllEntry();
				prof.GetRootKey()->EraseKey(name);
			}
		}
	}
#endif
	if (!groupKey)
		groupKey = prof.SetSection(PFS_GROUP);
	return groupKey->CreateKey(name);
}


////////////////////////////////////////////////////////////////////////////////

// プロファイルから読み込み、dicgrpにセット
//

// 固有データ
static bool WriteUniqData( DicGroup &dg, DicNames &dn, TRegKey *groupKey=NULL )
{
	TRegKey *key = CreateSectionU(dg.name, groupKey);
	if (!key)
		return false;
	Write( key, PFS_READONLY, dn.GetReadOnly( ) );
#if INETDIC
	Write( key, PFS_INET, dn.GetInet( ) );
#endif
#ifdef USE_COMP
	UINT comp[MAX_MULTIDIC];
	memset( comp, 0, sizeof(comp) );
	for ( int i=0;i<dn.get_num();i++ ){
		comp[i] = dn[i].comp;
	}
	key->PutVarArray( PFS_COMPPOLICY, comp, dn.get_num() );
#endif
//	key->WriteString( PFS_LANGPROC, dn.LangProc );	//TODO: LangProc
#if defined(EPWING) || defined(USE_FILELINK)
	for ( int i=0;i<dn.get_num();i++ ){
#ifdef EPWING
		tnstr keyname;
		keyname = tnsprintf(_t("EPWFile%d"), i);
		_EraseWrite(key, keyname, dn[i].epwname);
		keyname = tnsprintf(_t("GTrans%d"), i);
		_EraseWrite(key, keyname, dn[i].gtransname );
#endif
#ifdef USE_FILELINK
		keyname = tnsprintf(_t("FLinkPath%d"), i);
		_EraseWrite(key, keyname, dn[i].flinkpath, false);
#endif
#if 0
		keyname = tnsprintf(_t("FastDB%d"), i);
		_EraseWrite(key, keyname, dn[i].FastDB);
#endif
	}
#endif
	delete key;
	return true;
}

static bool ReadUniqData( DicGroup &dg, DicNames &dn, TRegKey *groupKey=NULL )
{
#ifdef NEWDIC2
	TRegKey *key = CreateSectionU( dg.name, groupKey );
	if (!key)
		return false;
#if defined(USE_REG)
	dl_def_empty(readonly);
	Read(key, PFS_READONLY, readonly);
	dn.SetReadOnly( readonly );
#if NETDIC
#endif
#if INETDIC
	dl_def_empty(network);
	Read(key, PFS_INET, network);
	dn.SetInet( network );
#endif
#endif
	dn.LangProc = key->ReadString( PFS_LANGPROC, _T("") );
#ifdef USE_COMP
	// 圧縮ポリシー
	UINT comp[MAX_MULTIDIC];
	int i;
	for ( i=0;i<MAX_MULTIDIC;i++ ){
		comp[i] = CP_COMP;
	}
	key->GetVarArray( PFS_COMPPOLICY, comp, MAX_MULTIDIC );
	for ( i=0;i<dn.get_num();i++ ){
		dn[i].comp = comp[i];
	}
#endif	// USE_COMP
#if USE_NETWORK
	key->Get( PFS_NETWORK, dg.fNetwork );
#endif
#endif
#if defined(EPWING) || defined(USE_FILELINK)
	for ( i=0;i<dn.get_num();i++ ){
#ifdef EPWING
		tnstr keyname = tnsprintf(_t("EPWFile%d"), i);
		dn[i].epwname = prof.ReadPath( keyname, dn[i].epwname, key );

		// 外字変換ファイル名
		keyname = tnsprintf(_t("GTrans%d"), i);
		dn[i].gtransname = prof.ReadPath( keyname, dn[i].gtransname, key );
#endif
#ifdef USE_FILELINK
		keyname = tnsprintf(_t("FLinkPath%d"), i);
		dn[i].flinkpath = key->ReadString( keyname, dn[i].flinkpath );
#endif
#if 0
		keyname = tnsprintf(_t("FastDB%d"), i);
		dn[i].FastDB = key->ReadInteger(keyname, dn[i].FastDB);
#endif
	}
#endif
	delete key;
	return true;
}

///////////////////////////////////////////////////////////////////////////////

// 辞書グループ名を探す
// expindex : 除外index
int GroupArray::Search( const tchar *grpname, int expindex )
{
	if (!grpname[0])
		return -1;
	for ( int i=0;i<GetNum();i++ ){
		if (i==expindex) continue;
		if ( _tcsicmp( grpname, (*this)[i].name ) == 0 ){
			return i;
		}
	}
	return -1;
}

int GroupArray::Find(const DicGroup &dg)
{
	if (dg.id!=-1){
		// find by id
		for (int i=0;i<GetNum();i++){
			__assert((*this)[i].id!=-1);
			if (dg.id==(*this)[i].id){
				return i;
			}
		}
	}
	// find by name
	return Find(dg.name);
}

// 返り値：
//	-1 : これ以上登録できない
//	-2 : 同じグループ名が既に登録されている
//  -3 : 使用できない名前
//	上記以外：登録された場所
// dgは所有権が移るので注意！
int GroupArray::Add( DicGroup *dg, DicNames &names )
{
#if 1
	return Insert(GetNum(), dg, names);
#else
	if ( GetNum() >= MAX_GROUPDIC ){
		delete dg;
		return -1;		// これ以上登録できない
	}
	int i = Search( dg->name );
	if ( i >= 0 ){
		delete dg;
		return -2;	// Duplicated name
	}
	add( dg );
	WriteGroupDic( GetNum() - 1, names );
	WriteGroupNames();
	return GetNum() - 1;
#endif
}

// dgは所有権が移るので注意！
int GroupArray::Insert(int index, DicGroup *dg, DicNames &names)
{
	if ( GetNum() >= MAX_GROUPDIC ){
		delete dg;
		return -1;		// これ以上登録できない
	}
	int i = Search( dg->name );
	if ( i >= 0 ){
		delete dg;
		return -2;		// Duplicated name
	}
	insert(index, dg);
	if (!Write(index, names)){
		del(index);
		return -3;	// The name is invalid.
	}
	WriteGroupNames();
	return index;
}

bool GroupArray::Write(int i, DicNames &names)
{
	if ( i >= get_num() )
		return false;
	return WriteGroupDic((*this)[i], names);
}

// dgに該当するdictionary groupをnamesへ
bool GroupArray::Write(const DicGroup &dg, DicNames &names)
{
	__assert(dg.id!=-1);
	int i = Find(dg);
	if (i<0)
		return false;
	return WriteGroupDic((*this)[i], names);
}

// i で示されるグループ辞書の辞書ファイル名をnamesへ取得
bool GroupArray::Read( int i, DicGroup &dg, DicNames &names, bool fOnlyFileName )
{
	if ( i < 0 || i >= get_num() )
		return false;

	dg.Copy((*this)[i]);	// copy id and name.
	names.clear();

	return ReadGroupDic((*this)[i].name, dg, names, fOnlyFileName);
}

// dgに該当するdictionary groupをnamesへ
bool GroupArray::Read(const DicGroup &dg, DicNames &names, bool fOnlyFileName)
{
	__assert(dg.id!=-1);
	int i = Find(dg);
	if (i<0)
		return false;	// not found
	// found!!
	DicGroup dummyDG;
	return Read(i, dummyDG, names, fOnlyFileName);
}

bool GroupArray::Delete( int i )
{
	if (i<0 || i>=get_num())
		return false;
	TRegKey *groupKey = prof.SetSection( PFS_GROUP );
	groupKey->EraseEntry( (*this)[i].name );
	groupKey->EraseKey( (*this)[i].name );

	del( i );
	SetSel( i );
	return true;
}

// -2:すでに同じ名前が登録されている
int GroupArray::Modify( int i, DicGroup &dg, DicNames &names )
{
	int j = Search( dg.name );
	if ( j >= 0 && i != j )
		return -2;
	// プロファイルから削除
	prof.SetSection( PFS_GROUP );
//	prof.EraseEntry( (*this)[i] );

	// 修正
	(*this)[i].Assign(dg);
	Write( i, names );
	WriteGroupNames();
	return 0;
}

// この関数を呼んだときは最後にWriteGroupNamesをすること
void GroupArray::Exchange( int i, int j )
{
	DicGroup *p = (DicGroup*)array[j];
	array[j] = array[i];
	array[i] = p;
}

// 辞書グループ名を変更
void GroupArray::Rename( int i, const tchar *newname )
{
	tnstr oldname( (*this)[i].name );
	if ( !_tmbsicmp( oldname, newname ) ){
		return;	// The name is not changed.
	}

	// 設定データをバッファに保存
	DicGroup dg;
	DicNames names;
	Read( i, dg, names );

    TRegKey *groupkey = prof.CreateKey( PFS_GROUP );
    TRegKey *srcreg = groupkey->CreateKey(oldname);
    TRegKey *destreg = groupkey->CreateKey(newname);
    CopyRegistry( srcreg, destreg );
	delete srcreg;
    delete destreg;
    delete groupkey;

	(*this)[i].name.set( newname );

	// 削除
	TRegKey *groupKey = prof.SetSection( PFS_GROUP );
	groupKey->EraseEntry( oldname );

	// 辞書グループ固有の設定の削除
	groupKey->EraseKey( oldname );

	// 辞書グループ固有の設定書き直し
	if (!Write( i, names )){
		//TODO: The new name is invalid.
	}

	// 辞書グループリスト再書き込み
	WriteGroupNames( );

	prof.Close( );
}
// 新しい辞書グループ名を作成する
// 最大数を越えていると、NULLを返す
// name : char[ 20 ];
tchar *GroupArray::CreateNewName( tchar *name )
{
	if ( GetNum() >= MAX_GROUPDIC )
		return NULL;
	for ( int i=1;;i++ ){
		wsnprintf( name, 20, _T("Group-%d"), i );
		if ( Search( name ) == -1 )
			break;
	}
	return name;
}

//
// SelGroupArray class
//
SelGroupArray::SelGroupArray()
{
	groupsel = -2;
}

int SelGroupArray::GetSel( )
{
	if ( groupsel == -2 ){
		prof.SetSection( PFS_COMMON );
		groupsel = prof.ReadInteger( PFS_GROUPSEL, groupsel );
		if ( groupsel == -2 ) groupsel = 0;		// 2000.4.16
	}
	return groupsel;
}

// 現在選択中の辞書グループ名を得る
// 選択がない場合はNULLを返す
const tchar *SelGroupArray::GetCurGroupName( )
{
	int groupsel = GetSel();
	if ( GetNum() && groupsel >= 0 ){
		return (*this)[groupsel].name;
	}
	return NULL;
}


void SelGroupArray::SetSel( int _groupsel )
{
	if ( _groupsel >= GetNum() ){
		_groupsel = GetNum() - 1;
	}
	groupsel = _groupsel;
	prof.SetSection( PFS_COMMON );
	prof.WriteInteger( PFS_GROUPSEL, groupsel );
}

bool SelGroupArray::GetCurNames( DicNames &names, bool fOnlyFileName )
{
	int i = GetSel();
	DicGroup dg;
	return Read( i, dg, names, fOnlyFileName );
}

// 返り値：読み込んだグループ辞書数
int ReadGroupDicList( )
{
	dicgrp.clear();
#ifdef WINCE
#if 0
	WinPdic *_dic = new WinPdic;
	_dic->Open( -1, _T("\\SD ｶｰﾄﾞ\\My Documents\\test-utf8.dic"), NULL, NULL, 0 );
	delete _dic;
#endif
#endif
#if COMPAT
	TRegKey *reg = prof.SetSection(PFS_GROUP);
	tnstr_vec names;
	reg->EnumValues(names);
	foreach_tnstr_vec(names, n){
		if (n->empty())
			continue;
		dicgrp.add(new DicGroup(*n));
	}
#else
	tnstr GroupNames = prof.ReadString(PFS_GROUP, PFS_GROUP_NAMES, NULL);
	const tchar *p = _tcstok(GroupNames.c_str(), _T("\t"));
	while (p){
		dicgrp.add( new DicGroup(p) );
		p = _tcstok(NULL, _T("\t"));
	}
#endif
	if ( dicgrp.GetSel() >= dicgrp.get_num() )
		dicgrp.SetSel( 0 );
	return dicgrp.get_num();
}

// dgで示されるグループ辞書を書き込む
bool WriteGroupDic(DicGroup &dg, DicNames &names, TRegKey *reg)
{
	if (!reg){
		reg = prof.SetSection( PFS_GROUP );
		if (!reg)
			return false;
	}
	if (prof.WritePath( dg.name, names, DELIM_GROUPNAME, reg ))
	//if (reg->Write( dg.name, names, DELIM_GROUPNAME ))
	{
		WriteUniqData( dg, names, reg );
		return true;
	}
	return false;
}

bool ReadGroupDic( const tchar *grpname, DicGroup &dg, DicNames &names, bool fOnlyFileName, TRegKey *reg )
{
	if (!reg){
		reg = prof.SetSection( PFS_GROUP );
		if (!reg)
			return false;
	}

	tnstr_vec nm;
	if (!reg->ReadMulti(grpname, nm)){
		prof.ReadPath( grpname, nm, DELIM_GROUPNAME, reg );
		//reg->Read( grpname, nm, DELIM_GROUPNAME );
	}
	names.SetNames( nm );
	dg.name = grpname;	// 2008.2.19 added

	ReadUniqData( dg, names, reg );
	if ( fOnlyFileName ){
		tchar buf[ L_FILENAME ];
		for ( int i=0;i<names.get_num();i++ ){
			get_filename( names[i].name, buf );
			names[i].name.set( buf );
		}
	}
	return true;
}

// ファイル名の一覧のみ取得
bool ReadGroupDicFiles(const tchar *grpname, tnstr_vec &filenames)
{
	TRegKey *reg = prof.SetSection( PFS_GROUP );
	if (!reg)
		return false;

	tnstr_vec nm;
	if (!reg->ReadMulti(grpname, nm)){
		reg->Read( grpname, nm, DELIM_GROUPNAME );
	}
	foreach_tnstr_vec(nm, n){
		filenames.add(*n);
	}
	return true;
}

// Write group names to keep the order of the groups.
// If you change the order of the groups, you should call me.
void WriteGroupNames()
{
#if !COMPAT
	tnstr names;
	for (int i=0;i<dicgrp.get_num();i++){
		if (!names.IsEmpty()){
			names += _T("\t");
		}
		names += dicgrp[i].name;
	}
	prof.WriteString(PFS_GROUP, PFS_GROUP_NAMES, names);
#endif
}

static DicNames *CurDicNames = NULL;

bool GetCurDicNames( DicNames &names )
{
	if ( !CurDicNames ){
		// 通常
		if ( dicgrp.GetSel() < 0 ){
			return false;
		} else {
			dicgrp.GetCurNames( names );
		}
	} else {
		// テンポラリ
		names = *CurDicNames;
	}
	return names.get_num() != 0;
}

void SetCurDicNames( DicNames *names )
{
	CurDicNames = names;
}

// grpnameでGroupの下に辞書グループ情報用のレジストリを作成
// 戻り値は必ず delete する必要がある
//Note:
//	DicAutoConf.cppからも参照しているので注意
TRegKey *CreateGroupSection( const tchar *grpname, int &uniq )
{
	TRegKey *section = prof.SetSection(PFS_GROUP);
	if (!section)
		return NULL;
	TRegKey *reg = section->CreateKey(grpname);	// grpnameでGroupの下に新規作成
	if (!reg)
		return NULL;
	uniq = reg->ReadInteger(PFS_UNIQ, 0 );
	return reg;
}

bool ValidateGroupName(const tchar *name)
{
	return true;
}

// Copy the dictionary group uniq data for dic group duplication.
bool CopyUniqData(const tchar *sname, const tchar *dname)
{
	TRegKey *regSrc = CreateSectionU(sname);
	if (!regSrc)
		return false;	// failed.
	TRegKey *regDst = CreateSectionU(dname);
	if (!regDst)
		return false;	// failed.
	CopyRegistry(regSrc, regDst);
	delete regSrc;
	delete regDst;
	return true;
}

// Dictionary Version //
bool ValidateUpgradeKey(const tchar *key)
{
	const int keylen = 18;
	int len = 0;
	for (;*key;){
		if (!ishex(*key++)){
			return false;
		}
		len++;
	}
	return len==keylen;
}
bool ValidateVersion(const tchar *s)
{
	bool exist_num = false;
	for (;*s;){
		tchar c = *s++;
		if (c>='0' && c<='9'){
			exist_num = true;
			continue;
		}
		if (c=='.')
			continue;
		return false;
	}
	return exist_num;
}

bool ReadUpgradeKeys(tnstr_map *ukey_to_name, tnstr_map *ukey_to_ver)
{
	TRegKey *keyGroup = prof.CreateKey(PFS_GROUP);
	if (!keyGroup)
		return false;	// error
	for (int i=0;i<dicgrp.size();i++){
		TRegKey *keyItem = keyGroup->OpenKey(dicgrp[i].name);
		if (keyItem){
			tnstr ukey = keyItem->ReadString(PFS_UPGRADEKEY);
			if (ValidateUpgradeKey(ukey)){
				if (ukey_to_name)
					(*ukey_to_name)[ukey] = dicgrp[i].name;
				if (ukey_to_ver)
					(*ukey_to_ver)[ukey] = keyItem->ReadString(PFS_VERSION);
			}
			delete keyItem;
		}
	}
	delete keyGroup;
	return true;
}

static void _EraseWrite(TRegKey *key, const tchar *keyname, const tchar *value, bool path)
{
	if (value && value[0]){
		if (path)
			prof.WritePath(keyname, value, key);
		else
			key->WriteString(keyname, value);
	} else
		key->DeleteValue(keyname);
}


