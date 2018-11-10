#include "tnlib.h"
#pragma hdrstop
#include "pdprof_android.h"
#include "defs_android.h"

PdProfile prof;
tnstr LastExePath;	//TODO: tbd
tnstr CommandPath;	//TODO: tbd

const tchar PFS_GROUP_NAMES[] = _t("::GroupNames::");


TRegKey::TRegKey(const tchar *name)
	:Name(name)
{
}

TRegKey *TRegKey::CreateKey(const tchar *keyname)
{
	return new TRegKey(keyname);
}
void TRegKey::EraseKey( const tchar *keyname )
{

}
void TRegKey::EraseEntry( const tchar *keyname )
{
}
bool TRegKey::PutVarArray( const tchar *entry, UINT *array, int number )
{
	return false;
}
bool TRegKey::GetVarArray( const tchar *entry, UINT *array, int number )
{
    return false;
}
tnstr TRegKey::ReadString(const tchar *key, const tchar *defval)
{
	if (Name==PFS_GROUP){
		if (!_tcscmp(key, PFS_GROUP_NAMES)){
			return _t("Sample");
		}
	}
	return _t("");
}
void TRegKey::Read( const tchar *name, tnstr_vec &items, int delim )
{
	items.clear();

	tnstr buf = ReadString(name);
	const tchar *top = &buf[0];
	do {
		const tchar *p = _tcschr( top, delim );
		if (!p)
			break;
		items.add( new tnstr(top, STR_DIFF(p,top)) );
		top = p+1;
	} while (1);
	if (top[0] || top!=&buf[0]){
		items.add(top);
	}
}
bool TRegKey::ReadMulti( const tchar *name, tnstr_vec &array )
{
	if (Name==PFS_GROUP){
		if (!_tcscmp(name, _t("Sample"))){
			array.push_back(_t("Sample.dic"));
//			array.push_back(_t("pdej.dic"));
			return true;
		}
	}
	return false;
}

const tchar *GetDocumentsPath()
{
	extern tnstr TempPath;
	return TempPath;
}

tnstr PdProfile::GetDictionaryPath(bool create)
{
	tnstr s(GetDocumentsPath());
	s += _t("/");
	return s;
}
tnstr PdProfile::GetCTTPath()
{
	return GetDocumentsPath();
}
tnstr PdProfile::GetPersonalFile(const tchar *filename, bool save)
{
	tnstr s(GetDocumentsPath());
	s += _t("/");
	s += filename;
	return s;
}
tnstr PdProfile::GetDefIrregDicName()
{
	tnstr s(GetDocumentsPath());
	s += _t("/IrregDic.txt");
	return s;
}
tnstr PdProfile::GetAdditionalIrregDicName()
{
	return GetDocumentsPath();
}
tnstr PdProfile::ReadString( const tchar *section, const tchar *key, const tchar *defval)
{
	TRegKey reg(section);
	return reg.ReadString(key, defval);
}
int PdProfile::ReadInteger( const tchar *section, const tchar *id, int defval )
{
	return defval;
}
int PdProfile::ReadInteger( const tchar *id, int defval )
{
    return defval;
}
void PdProfile::WriteInteger( const tchar *key, int value )
{

}
bool PdProfile::WritePath(const tchar *key, tnstr_vec &array, int delim, TRegKey *reg)
{
	return false;
}

void PdProfile::ReadPath(const tchar *key, tnstr_vec &array, int delim, TRegKey *reg)
{
	if (reg){
		reg->Read(key, array, delim);
	} else {
		//TODO:
		//Read(key, array, delim);
	}
}

TRegKey *PdProfile::CreateKey(const tchar *keyname)
{
	return new TRegKey(keyname);
}
TRegKey *PdProfile::SetSection( const tchar *appname )
{
	if (RegKey){
		if (!_tcscmp(RegKey->GetName(), appname)){
			return RegKey;
		}
		delete RegKey;
	}
	return RegKey = new TRegKey(appname);
}

void Read(TRegKey *reg, const tchar *section, diclist_t &dl)
{
	ReadArray(reg, section, dl_array(dl), dl_num(dl));
}
void Write(TRegKey *reg, const tchar *section, const diclist_t &dl)
{

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

bool write_bom(TOFile &tof)
{
	long l = tof.tell();
	if ( l == 0 ){
		tof.settextmode(prof.GetTextFileCode());
		if (prof.IsTextFileBOM())
			tof.bom();
	}
	return true;
}
