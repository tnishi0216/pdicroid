#include <windows.h>
#pragma hdrstop
#include "tndefs.h"
#include "tnassert.h"
#include "tnarray.h"
#include "profile.h"
#include "strlib.h"

#include <memory>
using namespace std;

//---------------------------------------------------------------------------

#pragma package(smart_init)

#include "tnwinx.h"
#include "uustr.h"
#include "tnstrbuf.h"

#define	LEN_INT_DIGITS	(11)	// int型の最大桁数(-符号含む)

// Note:
// Unicode版の場合、RegCreateKeyExW()の引数SubKey, RegSetValueEx()の引数ValueNameは、
// Helpでは、LPCTSTRとなっているが、その文字列が(C言語における）CONST領域（？）の変数であると
// ERROR_NOACCESS を返す。(Windowsのバグ？)
// こちらでわざわざoverheadを作るのもばからしいので、
// 呼び出し側で変数が"keyname"といった定数ではなく、ローカル変数やヒープ領域の
// 変数であることに注意すること。
// Windowsはほっとにバグだらけだ。。。

/////// RegKey ///////////////////////////////////////////////////////////////

TRegKey::TRegKey( )
{
}
TRegKey::~TRegKey( )
{
}

#if 0
BOOL TRegKey::QueryInfoKey( DWORD *maxvalue, DWORD *maxdata )
{
	tchar *classname = new tchar[ 512 ];	// こんなもんでいいかなぁ？
	DWORD classnamesize = 512;
	DWORD SubKeys;
	DWORD MaxSubKey;
	DWORD MaxClass;
	DWORD Values;
	DWORD MaxValueName;
	DWORD MaxValueData;
	DWORD SecurityDescriptor;
	FILETIME LastWriteTime;
	if ( ::RegQueryInfoKey( hkey, classname, &classnamesize,
		NULL,
		&SubKeys,
		&MaxSubKey,
		&MaxClass,
		&Values,
		&MaxValueName,
		&MaxValueData,
		&SecurityDescriptor,
		&LastWriteTime
		) != ERROR_SUCCESS ){
		delete[] classname;
#if REG_TEST
		 *(char*)0 = 0;
#endif
		return FALSE;
	}
	if ( maxvalue )
		*maxvalue = MaxValueName;
	if ( maxdata )
		*maxdata = MaxValueData;
	delete[] classname;
	return TRUE;
}
#endif

void TRegKey::DeleteKey( const tchar *subkey )	// subkeyはWIN32APIと違いNULLに可能(=全て)
{
	if ( subkey ){
		TRegKey *rg = this->OpenKey(subkey, false);
		if (!rg){
			return;
		}
		rg->DeleteValue( NULL );
		rg->DeleteKey( NULL );
		delete rg;
		DeleteKeyImpl(subkey);
	} else {
		tnstr_vec array;
		EnumKeys( array );
		for ( int i=0;i<array.get_num();i++ ){
			DeleteKey( array[i] );
		}
	}
}

#if 0
void TRegKey::Read( const tchar *key, TNChar &val )
{
	val = ReadString(key, val);
}
#endif
#if 0
tnstr TRegKey::ReadString( const tchar *key, const tchar *defval )
{
	tnstr s;
	if (Read(key, s)){
		return s;
	} else {
		// no entry
		return defval;
	}
}
#endif
#if 0
void TRegKey::WriteMultiString(const tchar *key, const tchar *str)
{
	const int buffer_size = 1024;
	tchar buffer[buffer_size+2];
	const tchar *sp = str;
	tchar *dp = buffer;
	tnstr text;
	tchar *dp_end = dp + buffer_size - 4;
	for (;;){
		for (;;){
			tuchar c = *sp++;
			if (!c){
				if (!*sp){
					// double terminator.
					goto jend;
				}
				*dp++ = '\\';
				*dp++ = ' ';
				break;
			}
			if (c<' ' || c=='\\'){
				*dp++ = '\\';
				c |= 0x20;
			}
			*dp++ = c;
			if (dp>=dp_end){
				// end of buffer.
				*dp = '\0';
				text += buffer;
				dp = buffer;
			}
		}
	}
jend:;
	*dp = '\0';
	text += buffer;
	WriteString(key, buffer);
}
#endif
#if 0
// Delete the returned value if not null.
tchar *TRegKey::ReadMultiString(const tchar *key)
{
	tnstr text = ReadString(key, _t(""));
	const tchar *sp = text;
	int text_len = __tcslen(sp);
	tchar *buf = new tchar[text_len+1];
	if (!buf)
		return NULL;
	tchar *dp = buf;
	for (;;){
		tchar c = *sp++;
		if (!c)
			break;
		if (c=='\\'){
			c = *sp++ & ~0x20;
		}
		*dp++ = c;
	}
	*dp++ = '\0';
	*dp = '\0';
	return buf;
}
#endif

// TRegKey 拡張 //////////////////

void TRegKey::Write( const tchar *name, RECT &r )
{
	UINT values[4] = {r.left, r.top, r.right, r.bottom};
	PutVarArray(name, values, 4);
}
void TRegKey::Read( const tchar *name, RECT &r )
{
	UINT values[4] = {r.left, r.top, r.right, r.bottom};
	GetVarArray(name, values, 4);
	r.left = values[0];
	r.top = values[1];
	r.right = values[2];
	r.bottom = values[3];
}

void TRegKey::Read( const tchar *name, tnstr_vec &items, int delim )	// REG_SZ形式
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
#if 0
// It should be deleted.
void TRegKey::Read( const tchar *name, FlexCharArray &items, int delim )	// REG_SZ形式
{
	items.clear();

	tnstr buf = ReadString(name);
	const tchar *top = &buf[0];
	do {
		const tchar *p = _tcschr( top, delim );
		if (!p)
			break;
		items.add( new TNChar(top, STR_DIFF(p,top)) );
		top = p+1;
	} while (1);
	if (top[0] || top!=&buf[0]){
		items.add(top);
	}
#if 0	// old way
	const tchar *p = __tcstok( &buf[0], del );
	while ( p ){
		items.add( p );
		p = __tcstok( NULL, del );
	}
#endif
}
#endif
bool TRegKey::Write( const tchar *name, tnstr_vec &items, int delim )
{
	tnstr buf;

	tchar del[2];
	del[0] = (tchar)delim;
	del[1] = '\0';
	for ( int i=0;i<items.get_num();i++ ){
		if ( i != 0 )
			buf.cat( del );
		buf.cat( items[i] );
	}
	return WriteString( name, buf );
}
#if 0
// It should be deleted.
bool TRegKey::Write( const tchar *name, FlexCharArray &items, int delim )
{
	tnstr buf;

	tchar del[2];
	del[0] = (tchar)delim;
	del[1] = '\0';
	for ( int i=0;i<items.get_num();i++ ){
		if ( i != 0 )
			buf.cat( del );
		buf.cat( items[i] );
	}
	return WriteString( name, buf );
}
#endif
bool TRegKey::PutVarArray( const tchar *name, UINT *array, int number )
{
	tnstr_vec strs;
	for (int i=0;i<number;i++){
		tchar buf[20];
		itos(array[i], buf);
		strs.add(new tnstr(buf));
	}
	return Write(name, strs, ',');
}
bool TRegKey::GetVarArray( const tchar *name, UINT *array, int number )
{
	tnstr_vec strs;
	Read(name, strs, ',');

	for (int i=0;i<min(strs.get_num(),number);i++){
		array[i] = __ttoi(strs[i]);
	}
	return true;
}

#if 0
#ifdef __BORLANDC__
// VCL //
WideString TRegKey::ReadStringW( const tchar *key)
{
	WideString s;
	ReadString(key, s);
	return s;
}
WideString TRegKey::ReadString(WideString &s)
{
	return ReadStringW(s.c_bstr());
}
#if 0
bool TRegKey::GetValueNames(class TStrings *strings)
{
	//TODO:
	return false;
}
#endif
bool TRegKey::GetValueNames(class TTntStrings *strings)
{
	DWORD buffersize = NAMEBUFFSIZE;
	tchar *buffer = new tchar[ buffersize ];
	if (!buffer)
		return false;
	if (!strings)
		return false;

	for ( int i=0;;i++ ){
		if (RegEnumValue(hkey, i, buffer, &buffersize, 0, NULL, (LPBYTE)NULL, NULL) != ERROR_SUCCESS){
			//TODO: バッファサイズ不足エラー処理
			break;
		}
		strings->Add(buffer);
	}
	delete[] buffer;
	return true;
}
#endif
bool TRegKey::Read(const tchar *name, class TString &str)
{
	DWORD len = GetValueLength( name );
	if ( len ){
		tchar *buf = new tchar[ len ];
		if (!QueryValue( name, buf, len )){
			delete[] buf;
			return false;
		}
		str.setBuf( buf );
	}
	return true;
}
#endif

/////// TRegKeyR ///////////////////////////////////////////////////////////////
#define	NAMEBUFFSIZE	512

TRegKeyR::TRegKeyR()
{
	hKey = NULL;
}
TRegKeyR::TRegKeyR(HKEY _hkey)
{
	hKey = _hkey;
}
TRegKeyR::TRegKeyR(HKEY _hkey, const tchar *keyname)
{
	hKey = CreateKey(_hkey, keyname);
}
TRegKeyR::~TRegKeyR()
{
	Close();
}
#if 0
bool TRegKeyR::Create( HKEY _hkey, const tchar *keyname)
{
	if ( hkey )
		Close( );
	hkey = CreateKey(_hkey, keyname);
	return hkey != NULL ? TRUE : FALSE;
}
#endif
TRegKey *TRegKeyR::CreateKey(const tchar *keyname)
{
	HKEY hkey = CreateKey(this->GetKey(), keyname);
	if (!hkey)
		return NULL;
	return new TRegKeyR(hkey);
}
TRegKey *TRegKeyR::OpenKey(const tchar *keyname, bool readonly)
{
	HKEY hkey = OpenKey(this->GetKey(), keyname, readonly?KEY_READ:KEY_ALL_ACCESS);
	if (!hkey)
		return NULL;
	return new TRegKeyR(hkey);
}
void TRegKeyR::Close( )
{
	if ( hKey ){
		::RegCloseKey( hKey );
		hKey = NULL;
	}
}
bool TRegKeyR::KeyExists(const tchar *keyname)
{
	HKEY newkey = OpenKey(hKey, keyname, KEY_READ);
	if (!newkey)
		return false;
	RegCloseKey(newkey);
	return true;
}
bool TRegKeyR::ValueExists(const tchar *valuename)
{
	DWORD type;
	DWORD len;
	return RegQueryValueEx( hKey, valuename, 0, &type, NULL, &len) == ERROR_SUCCESS;
}

bool TRegKeyR::EnumKeys( tnstr_vec &array )
{
	for ( int i=0;;i++ ){
		tnstr *name = new tnstr;
		if ( EnumKey( i, *name ) ){
			array.add( name );
		} else {
			delete name;
			break;
		}
	}
	return true;
}

bool TRegKeyR::EnumValue( DWORD index, tnstr &name, void *pbuffer, DWORD *maxlen, DWORD *type )
{
	TCHAR *buffer = new TCHAR[ NAMEBUFFSIZE ];
	if (!buffer)
		return false;

	DWORD buflen = NAMEBUFFSIZE;
	if ( RegEnumValue( hKey, index, buffer, &buflen, 0, type, (LPBYTE)pbuffer, maxlen ) == ERROR_SUCCESS ){
		name = buffer;
		delete[] buffer;
		return true;
	}
	//TODO: NAMEBUFFSIZE以上のレジストリを取得ができない
	delete[] buffer;
	return false;
}

bool TRegKeyR::DeleteValue( const tchar *name )
{
	if ( name )
		return ::RegDeleteValue( hKey, _uustrT(name) ) == ERROR_SUCCESS;
	else {
		// 全値の削除
		// わざわざすべての名前を取得してから出ないとすべて削除出来ない
		::RegDeleteValue( hKey, NULL );		// これだけでは削除されない？？
		tnstr_vec array;
		int i;
		for ( i=0;;i++ ){
			tnstr *name = new tnstr;
			if ( EnumValue( i, *name ) ){
				array.add( name );
			} else {
				delete name;
				break;
			}
		}
		for ( i=0;i<array.get_num();i++ ){
			::RegDeleteValue( hKey, _uustrT((tchar*)(const tchar *)array[i]) );
		}

#if 0
		// 全値の削除
		// この方法では動かない！！-->Microsoftの馬鹿ヤロー
		::RegDeleteValue( hkey, NULL );		// これだけでは削除されない？？
		DWORD size = NAMEBUFFSIZE;
		tchar *name = new tchar[ size ];
		int i = 0;
		while ( 1 ){
			LONG r;
			if ( (r=::RegEnumValue( hkey, i, name, &size, NULL, NULL, NULL, NULL )) == ERROR_NO_MORE_ITEMS && i==0 ){
				break;
			}
			if ( r == ERROR_SUCCESS )
				DeleteValue( name );
			if ( r == ERROR_MORE_DATA )		// こんなわけのわからない処理をしないと動かない！
											// レジストリを設計した奴は誰だ！Microsoftの馬鹿ヤロー
				i++;
			else
				i = 0;
		}
		delete[] name;
#endif
		return TRUE;
	}
}
// 指定したサブキーを削除
void TRegKeyR::DeleteKeyImpl(const tchar *subkey)
{
	__assert(subkey!=NULL);
	::RegDeleteKey( hKey, _uustrT(subkey) );
}

#if 0
bool TRegKeyR::Read( const tchar *name, tnstr &str )
{
	DWORD len;
	if ( RegQueryValueEx( hKey, name, 0, NULL, NULL, &len ) != ERROR_SUCCESS ){
		// may be no entry
		return false;
	}
	if ( len ){
		tchar *buf = new tchar[ len ];
		if (!QueryValue( name, buf, len )){
			delete[] buf;
			return false;
		}
		str.SetBuf( buf );
	}
	return true;
}
#endif

tnstr TRegKeyR::ReadString(const tchar *name, const tchar *defval)
{
	DWORD len;
	if ( RegQueryValueEx( hKey, _uustrT(name), 0, NULL, NULL, &len ) != ERROR_SUCCESS ){
		// may be no entry
		return defval;
	}
	if (len==0){
		return _T("");
	}
	if (len<1024){
		// Uses auto variable.
		tchar buf[1024];
#ifdef _UNICODE
		buf[0] = '\0';	// regedit.exeで空の文字列を作成すると１バイトの'\0'ができあがるためこの処理が必要(Win2Kで確認)
#endif
		if (!QueryValue( name, buf, len )){
			return defval;
		}
		return tnstr(buf);
	} else {
		tchar *buf = new tchar[ len ];
		if (!buf)
			return defval;
		if (!QueryValue( name, buf, len )){
			delete[] buf;
			return _t("");
		}
		auto_ptr<tchar> tmp_buf(buf);
		return tnstr(tmp_buf.get());
	}
}

void TRegKeyR::WriteInteger( const tchar *key, int val )
{
	DWORD value = val;
	SetValue( key, &value, sizeof(DWORD), REG_DWORD );
}

int TRegKeyR::ReadInteger( const tchar *name, int defval )
{
	DWORD v;
	DWORD maxlen = sizeof(DWORD);
	if ( QueryValue( name, &v, maxlen ) ){
		return (int)v;
	}
	return defval;
}

bool TRegKeyR::WriteString( const tchar *name, const tchar *str )
{
	return Set( name, str, (str?__tcslen(str):0)+1, REG_SZ );
}

#if 0
void TRegKeyR::WriteMultiString(const tchar *key, const tchar *str)
{
#ifdef __UTF8
	_uustrT _str(str);
#else
	#define	_str	str
#endif
	RegSetValueEx( GetKey(), _uustrT(key), 0, REG_MULTI_SZ, (BYTE*)str, (_tcslen(_str)+1)*sizeof(TCHAR));
}
#endif

#if 0
// Delete the returned value if not null.
tchar *TRegKeyR::ReadMultiString(const tchar *key)
{
	DWORD len = 0;
	DWORD type = REG_MULTI_SZ;
	if ( RegQueryValueEx( hKey, _uustrT(key), 0, &type, NULL, &len ) != ERROR_SUCCESS ){
		return NULL;
	}
	if ( len <= 0 )
		return NULL;

	tchar *buf = new tchar[ len/sizeof(tchar) + 1 ];
	if (!buf)
		return NULL;
	RegQueryValueEx( hKey, _uustrT(key), 0, &type, (BYTE*)buf, &len );
	return buf;
}
#endif

bool TRegKeyR::ReadMulti( const tchar *name, tnstr_vec &array )	// REG_MULTI_SZ形式
{
	array.clear();
	DWORD len = 0;
	DWORD type = REG_MULTI_SZ;
	if ( RegQueryValueEx( hKey, _uustrT(name), 0, &type, NULL, &len ) != ERROR_SUCCESS ){
		return false;
	}
	if (type!=REG_MULTI_SZ)
		return false;

	if ( len <= 0 )
		return true;	// no data

	tchar *buf = new tchar[ len/sizeof(tchar) + 1 ];
	if (!buf)
		return false;
	RegQueryValueEx( hKey, _uustrT(name), 0, &type, (BYTE*)buf, &len );

	const tchar *p = buf;
	const tchar *top = p;
	for (;;){
		if (!*p){
			array.push(new tnstr(top, STR_DIFF(p,top)));
			p++;
			top = p;
			if (!*p)
				break;	// end of multi-strings.
		}
		p++;
	}

	delete[] buf;

	return true;
}

bool TRegKeyR::QueryValue( const tchar *name, void *buffer, DWORD &maxlen, DWORD *type )
{
#if REG_TEST
	switch ( RegQueryValueEx( hkey, name, 0, type, (LPBYTE)buffer, &maxlen ) ){
	case ERROR_SUCCESS:
		return TRUE;
	case ERROR_FILE_NOT_FOUND:
		return FALSE;
	default:
//		*(char*)0 = 0;
		return FALSE;
	}
#else
	return RegQueryValueEx( hKey, _uustrT(name), 0, type, (LPBYTE)buffer, &maxlen ) == ERROR_SUCCESS;
#endif
}

bool TRegKeyR::SetValue( const tchar *name, const void *buffer, DWORD len, DWORD type )
{
	return SetValueEx( _uustrT(name), type, (CONST BYTE *)buffer, len ) == ERROR_SUCCESS;
}

HKEY TRegKeyR::CreateKey( HKEY _hkey, const tchar *keyname )
{
	HKEY newkey;
	DWORD result;
	if ( RegCreateKeyEx( _hkey, _uustrT(keyname), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &newkey, &result ) == ERROR_SUCCESS ){
		return newkey;
	}
	return NULL;
}
HKEY TRegKeyR::OpenKey( HKEY hkey, const tchar *keyname, REGSAM sam )
{
	HKEY newkey;
	if ( RegOpenKeyEx( hkey, _uustrT(keyname), 0, sam, &newkey ) == ERROR_SUCCESS ){
		return newkey;
	}
	return NULL;
}

bool TRegKeyR::EnumKey( DWORD index, tnstr &name )
{
	TCHAR buffer[ NAMEBUFFSIZE ];
		
	TCHAR *clsbuf = NULL;
	DWORD buflen = NAMEBUFFSIZE;
	DWORD clslen = NAMEBUFFSIZE;
	FILETIME filetime;
	if ( RegEnumKeyEx( hKey, index, buffer, &buflen, 0, clsbuf, &clslen, &filetime ) == ERROR_SUCCESS ){
		name = _uustrT(buffer);
		return true;
	}
	return false;

}

bool TRegKeyR::EnumValues(tnstr_vec &array) const
{
	for (int i=0;;i++){
		tnstr *name = new tnstr;
		if (EnumValue(i, *name)){
			array.add(name);
		} else {
			delete name;
			break;
		}
	}
	return true;
}

DWORD TRegKeyR::GetValueLength( const tchar *name )
{
	DWORD len = 0;
#if REG_TEST
	switch ( RegQueryValueEx( hkey, name, 0, NULL, NULL, &len ) ){
	case ERROR_SUCCESS:
	case ERROR_FILE_NOT_FOUND:
		return len;
	default:
//		*(char*)0 = 0;
		break;
	}
#else
	if ( RegQueryValueEx( hKey, _uustrT(name), 0, NULL, NULL, &len ) == ERROR_SUCCESS )
	{
		return len;
	}
#endif
	return 0L;
}
int TRegKeyR::GetValueLengthEx( const tchar *name )
{
	DWORD len = 0;
	if ( RegQueryValueEx( hKey, _uustrT(name), 0, NULL, NULL, &len ) == ERROR_SUCCESS )
	{
		return len;
	}
	return -1;	// not exist
}

bool TRegKeyR::Set( const tchar *name, const tchar *str, DWORD len, DWORD type )
{
	if ( str )
		return SetValue( name, str, LENTOBYTE(len), type );
	else
		return ::RegDeleteValue( hKey, _uustrT(name) ) == ERROR_SUCCESS;
}

////////////////// TRegKeyIni ///////////////////////////////////////////////

#define	SECTION_SEPARATOR		_t("/")
#define	SECTION_SEPARATOR_CHAR	'/'

TRegKeyIni::TRegKeyIni(const tchar *filename, const tchar *sectionname)
	:FileName(filename)
	,SectionName(sectionname)
{
}

TRegKey *TRegKeyIni::CreateKey(const tchar *keyname)
{
	return OpenKey(keyname, false);
}

TRegKey *TRegKeyIni::OpenKey(const tchar *keyname, bool /*readonly*/)
{
	if (SectionName.IsEmpty()){
		return new TRegKeyIni(FileName, keyname);
	} else {
		return new TRegKeyIni(FileName, SectionName + SECTION_SEPARATOR + keyname);
	}
}
void TRegKeyIni::Close()
{
	Flush();
}
void TRegKeyIni::Flush( )
{
	WritePrivateProfileString(NULL, NULL, NULL, _uustrT(FileName));
}
void TRegKeyIni::DeleteKeyImpl(const tchar *subkey)
{
	//TODO: shoud test
	__assert(subkey!=NULL);
	tnstrbuf fullpath(SectionName);
	fullpath += SECTION_SEPARATOR;
	fullpath += subkey;
	WritePrivateProfileString(_uustrT(fullpath), NULL, NULL, _uustrT(FileName));
}
bool TRegKeyIni::DeleteValue( const tchar *name )	// nameにNULLも可能(=全て)
{
	return WritePrivateProfileString(_uustrT(SectionName), _uustrT(name), NULL, _uustrT(FileName)) ? true : false;
}
bool TRegKeyIni::KeyExists(const tchar *name)
{
	//TODO: shoud test
	const int buffer_size = 8;
	TCHAR buffer[buffer_size+2];
	tnstrbuf keyname(SectionName);
	keyname += SECTION_SEPARATOR;
	keyname += name;
	DWORD dwRet = GetPrivateProfileString(_uustrT(keyname), NULL, _T(""), buffer, buffer_size, _uustrT(FileName));
	return dwRet>0;	// one or more names exist in the sub section.
}
bool TRegKeyIni::ValueExists(const tchar *valuename) const
{
	tnstr_vec values;
	EnumValues(values);
	for (int i=0;i<values.size();i++){
		if (_tcscmp(valuename, values[i])==0){
			return true;
		}
	}
	return false;
}
bool TRegKeyIni::EnumKeys( tnstr_vec &array )
{
	int buffer_size = 16384;

	array.clear();
	for (;;){
		TCHAR *buffer = new TCHAR[buffer_size+2];
		if (!buffer)
			return false;
		DWORD dwRet = GetPrivateProfileSectionNames(buffer, buffer_size, FileName);
		if ((int)dwRet==buffer_size-2){
			// Buffer size is too small.
			delete[] buffer;
			if (buffer_size >= 0x100000){
				// Too big!!!
				__assert__;
				return false;
			}
			buffer_size *= 2;
			continue;
		}
		// Add the section names to the array.
		int nSectionName = _tcslen(SectionName);
		TCHAR *p = buffer;
		for (;*p;){
			if (_tcsncmp(SectionName, p, nSectionName)==0){
				// same section
				if (p[nSectionName]==SECTION_SEPARATOR_CHAR)
					p = &p[nSectionName+1];
				else
				if (!p[nSectionName]){
					p = &p[nSectionName];
				}
				if (p[0]){
					if (!_tcschr(p, SECTION_SEPARATOR_CHAR)){
						// no any child sections.
						// same section
						array.add(p);
					}
				}
			}
			p += _tcslen(p)+1;
		}
		break;
	}
	return true;
}
bool TRegKeyIni::EnumValues(tnstr_vec &array) const
{
	const int MAXIMUM_CONTENT_SIZE = 16384;	// これ以上大きいデータは読み込みできない
	int buffer_size = MAXIMUM_CONTENT_SIZE;
	TCHAR *buffer = new TCHAR[buffer_size+2];
	if (!buffer)
		return false;

	array.clear();
	DWORD dwRet = GetPrivateProfileString(SectionName, NULL, NULL, buffer, buffer_size, FileName);
	if (dwRet!=0){
		__assert((int)dwRet<buffer_size-2);	// Too small buffer size.
		TCHAR *p = buffer;
		for (;*p;){
			array.add(p);
			p += _tcslen(p)+1;
		}
	}
	delete[] buffer;
	return true;
}
int TRegKeyIni::ReadInteger( const tchar *name, int defval ) const
{
	return GetPrivateProfileInt(SectionName, name, defval, FileName);
}
tnstr TRegKeyIni::ReadString(const tchar *key, const tchar *defval) const
{
	DWORD buffer_size = 1024;
	tchar auto_buf[1024+1];
	DWORD dwRet = GetPrivateProfileString(SectionName, key, defval, auto_buf, buffer_size, FileName);
	if (dwRet<buffer_size-1){
		// Enough buffer size.
		return tnstr(auto_buf);
	}
	for (;;){
		tchar *buf = new tchar[buffer_size+1];
		if (!buf)
			return defval;
		dwRet = GetPrivateProfileString(SectionName, key, defval, buf, buffer_size, FileName);
		if (dwRet>=buffer_size-1){
			// Too small.
			delete[] buf;
			buffer_size *= 2;
			if (buffer_size>=0x100000){
				// Too big!!
				__assert__;
				return defval;
			}
			continue;
		}
		auto_ptr<tchar> tmp_buf(buf);
		return tnstr(tmp_buf.get());
	}
}
void TRegKeyIni::WriteInteger( const tchar *key, int val )
{
	tchar buf[20];
	_itot(val, buf, 10);
	WritePrivateProfileString(SectionName, key, buf, FileName);
}
bool TRegKeyIni::WriteString( const tchar *key, const tchar *str )
{
	return WritePrivateProfileString(SectionName, key, str, FileName) ? true : false;
}

bool TRegKeyIni::GetSectionNames(tnstr_vec &section_names)
{
	int buffer_size = 1024;

	for (;;){
		tchar *buffer = new tchar[buffer_size];
		if (!buffer)
			return false;	// No memory.
		int size = buffer_size-1;
		DWORD ret = ::GetPrivateProfileSectionNames( buffer, size, FileName );
		if ((int)ret==size-2){
			buffer_size += 1024;
			if (buffer_size>=1024*16){
				return false;	// Something wrong?
			}
			continue;
		}

		auto_ptr<TCHAR> _buffer(buffer);	// auto deleter

		tchar *p = buffer;
		//tchar *nextp = p;
		for(;*p;){
			section_names.add(p);
			while ( *p++ );
		}
		break;
	}
	return true;
}


