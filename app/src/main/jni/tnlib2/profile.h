//---------------------------------------------------------------------------

#ifndef __profile_h
#define __profile_h
//---------------------------------------------------------------------------

#include "charray.h"
#include "getstr.h"		// 当面GetString()のみ
#include "tnarray.h"
#include "tnstr.h"
//#include "char.h"

class TRegKey {
public:
	TRegKey( );
	virtual ~TRegKey( );
	virtual bool RegMode() const = 0;
	bool IniMode() const { return !RegMode(); }
	bool Open( TRegKey *regkey, const tchar *keyname, REGSAM sam=KEY_ALL_ACCESS );
	virtual TRegKey *CreateKey(const tchar *keyname) = 0;
	virtual TRegKey *OpenKey(const tchar *keyname, bool readonly=false) = 0;
	inline TRegKey *OpenKeyRead(const tchar *keyname)
		{ return OpenKey(keyname, true); }
	virtual void Close( ) = 0;
#ifndef WINCE
	virtual void Flush( ) = 0;
#endif
	virtual bool KeyExists(const tchar *name) = 0;
	virtual bool ValueExists(const tchar *name) = 0;
	virtual bool EnumKeys( tnstr_vec &array ) = 0;
	virtual bool EnumValues(tnstr_vec &array) = 0;
	virtual bool DeleteValue( const tchar *name ) = 0;	// nameにNULLも可能(=全て)
	virtual void DeleteKey( const tchar *subkey );	// subkeyはWIN32APIと違いNULLに可能(=全て)
	void DeleteAllKey( )		// すべてのサブキーを削除(エントリーは削除されない)
	{
		DeleteKey( NULL );
	}
	void DeleteAllEntries( )	// すべての値、キーを削除
	{
		DeleteValue( NULL );
		DeleteAllKey( );
	}

	// 情報取得
	BOOL QueryInfoKey( DWORD *lpccMaxValueName, DWORD *lpccMaxValueData );	// キーと値の最大長を得る
		// lpccMaxValueName : 最も長い値名の長さ
		// lpccMaxValueData : 最も長い値の長さ

	// String //
//	void Read( const tchar *key, TNChar &val );	// for fast access.
	virtual tnstr ReadString(const tchar *key, const tchar *defval=_t("")) = 0;
	tnstr ReadString(int id, const tchar *defval=_t(""))
		{ return ReadString(GetString(id), defval); }
	virtual bool WriteString( const tchar *key, const tchar *str ) = 0;
	bool WriteString(int id, const tchar *str)
		{ return WriteString(GetString(id), str); }
	//virtual void WriteMultiString(const tchar *key, const tchar *str);

	// Integer //
	virtual int ReadInteger( const tchar *name, int defval ) = 0;
	int ReadInteger( int id, int defval )
		{ return ReadInteger(GetString(id), defval); }
	virtual void WriteInteger( const tchar *key, int val ) = 0;
	void WriteInteger( int id, int val )
		{ WriteInteger(GetString(id), val); }

	// 準削除予定 //
//	void Read( int id, TNChar &val )
//		{ Read(GetString(id), val); }
	int Read(const tchar *name, int defval) const
		{ return ReadInteger(name, defval); }
	void Write(const tchar *key, int val)
		{ WriteInteger(key, val); }
	void Write(const tchar *key, const tchar *str)
		{ WriteString(key, str); }

	int Read(int id, int defval) const
		{ return ReadInteger(id, defval); }
	void Write(int id, int val)
		{ WriteInteger(id, val); }
	void Write(int id, const tchar *str)
		{ WriteString(id, str); }

	// FlexCharArray - REG_SZ //
	void Read( const tchar *name, tnstr_vec &array, int delim );	// REG_SZ形式
	void Read( const tchar *name, FlexCharArray &array, int delim );	// REG_SZ形式
	void Read(int id, FlexCharArray &array, int delim)
		{ Read(GetString(id), array, delim); }
	bool Write( const tchar *name, tnstr_vec &array, int delim );	// REG_SZ形式
	bool Write( const tchar *name, FlexCharArray &array, int delim );	// REG_SZ形式
	bool Write(int id, FlexCharArray &array, int delim)
		{ return Write(GetString(id), array, delim); }

	// tnstr_vec - REG_MULTI_SZ - supported by only TRegKeyR //
	virtual bool ReadMulti( const tchar *name, tnstr_vec &array ){ return false; }	// REG_MULTI_SZ形式

	// 準削除予定 //
	void Write( const tchar *name, RECT &rect );
	void Write(int id, RECT &rect)
		{ Write(GetString(id), rect); }
	void Read( const tchar *name, RECT &rect );
	void Read(int id, RECT &rect)
		{ Read(GetString(id), rect); }

	// Erase //
	void EraseAllEntry( )					// サブキーは削除されないので注意！！
		{ DeleteValue( NULL ); }
	void EraseKey( const tchar *keyname )	// サブキーkeynameを削除(NULLにすると全てのサブキーを削除(entryは削除されない))
		{ DeleteKey( keyname ); }
	void EraseEntry( const tchar *keyname )
		{ DeleteValue( keyname ); }
	void EraseEntry( int id )
		{ EraseEntry( GetString( id ) ); }

	// 可変長文字列数値配列
	bool PutVarArray( const tchar *entry, UINT *array, int number );
	bool PutVarArray( int id, UINT *array, int number ) { return PutVarArray( GetString( id ), array, number ); }
	bool GetVarArray( const tchar *entry, UINT *array, int number );
	bool GetVarArray( int id, UINT *array, int number ){ return GetVarArray( GetString( id ), array, number ); }

protected:
	virtual void DeleteKeyImpl( const tchar *subkey ) = 0;
};

class TRegKeyR : public TRegKey {
typedef TRegKey super;
protected:
	HKEY hKey;
public:
	TRegKeyR();
	TRegKeyR(HKEY hkey);
	TRegKeyR(HKEY hkey, const tchar *keyname);
	virtual ~TRegKeyR();
	virtual bool RegMode() const { return true; }
	HKEY GetKey() const
		{ return hKey; }
	virtual TRegKey *CreateKey(const tchar *keyname);
	TRegKeyR *CreateKeyReg(const tchar *keyname)
		{ return (TRegKeyR*)CreateKey(keyname); }
	virtual TRegKey *OpenKey(const tchar *keyname, bool readonly=false);
	TRegKeyR *OpenKeyReg(const tchar *keyname, bool readonly=false)
		{ return (TRegKeyR*)OpenKey(keyname, readonly); }
	virtual void Close();
	virtual void Flush( )
		{ ::RegFlushKey( hKey ); }
	virtual bool KeyExists(const tchar *name);
	virtual bool ValueExists(const tchar *name) const;
	virtual bool EnumKeys(tnstr_vec &array);
	virtual bool EnumValues(tnstr_vec &array) const;
	virtual bool DeleteValue( const tchar *name );	// nameにNULLも可能(=全て)
protected:
	virtual void DeleteKeyImpl( const tchar *subkey );	// subkeyはWIN32APIと違いNULLに可能(=全て)
public:
	virtual int ReadInteger( const tchar *name, int defval ) const;
	//virtual void WriteMultiString(const tchar *key, const tchar *str);
	//virtual tchar *ReadMultiString(const tchar *key);
	virtual tnstr ReadString(const tchar *key, const tchar *defval=_t("")) const;
	virtual void WriteInteger( const tchar *key, int val );
	virtual bool WriteString( const tchar *key, const tchar *str );

	// tnstr_vec - REG_MULTI_SZ - supported by only TRegKeyR //
	virtual bool ReadMulti( const tchar *name, tnstr_vec &array );	// REG_MULTI_SZ形式

	// Registry depends //
public:
	bool QueryValue( const tchar *name, void *buffer, DWORD &maxlen, DWORD *type=NULL ) const;
		// bufferをNULLにするとmaxlenにデータの長さを返す
	bool SetValue( const tchar *name, const void *buffer, DWORD len, DWORD type );

protected:
	HKEY CreateKey(HKEY hkey, const tchar *keyname);
	HKEY OpenKey(HKEY hkey, const tchar *keyname, REGSAM sam=KEY_ALL_ACCESS);
//	bool Open( HKEY hkey, const tchar *keyname );

	bool EnumKey( DWORD index, tnstr &name );
	bool EnumValue( DWORD index, tnstr &name, void *buffer=NULL, DWORD *maxlen=NULL, DWORD *type=NULL ) const;
			// maxlen は渡すときにバッファのサイズ、正常終了すると受け取ったデータのサイズを表す
			// bufferはNULLにすることができるその場合は、値の名前だけ得る

	DWORD GetValueLength( const tchar *name );
	int GetValueLengthEx( const tchar *name );

	LONG SetValueEx(LPCTSTR lpValueName, DWORD dwType, CONST BYTE *lpData, DWORD cbData)
		{ return ::RegSetValueEx(hKey, lpValueName, 0, dwType, lpData, cbData); }

	bool Set( const tchar *name, const tchar *str, DWORD len, DWORD type );	// 任意
};

class TRegKeyIni : public TRegKey {
typedef TRegKey super;
protected:
	tnstr SectionName;
	tnstr FileName;
public:
	TRegKeyIni(const tchar *filename, const tchar *section_name);
	virtual bool RegMode() const { return false; }
	virtual TRegKey *CreateKey(const tchar *keyname);
	TRegKeyIni *CreateKeyIni(const tchar *keyname)
		{ return (TRegKeyIni*)CreateKey(keyname); }
	virtual TRegKey *OpenKey(const tchar *keyname, bool readonly=false);
	TRegKeyIni *OpenKeyIni(const tchar *keyname, bool readonly=false)
		{ return (TRegKeyIni*)OpenKey(keyname, readonly); }
	virtual void Close();
	virtual void Flush( );
	virtual bool DeleteValue( const tchar *name );	// nameにNULLも可能(=全て)
protected:
	virtual void DeleteKeyImpl( const tchar *subkey );	// subkeyはWIN32APIと違いNULLに可能(=全て)
public:
//	virtual bool Read( const tchar *key, tnstr &val );
	virtual bool KeyExists(const tchar *name);
	virtual bool ValueExists(const tchar *name) const;
	virtual bool EnumKeys( tnstr_vec &array );
	virtual bool EnumValues(tnstr_vec &array) const;
	virtual int ReadInteger( const tchar *name, int defval ) const;
	virtual tnstr ReadString(const tchar *key, const tchar *defval=_t("")) const;
	virtual void WriteInteger( const tchar *key, int val );
	virtual bool WriteString( const tchar *key, const tchar *str );
protected:

	// INI operation dedicated.
public:
	bool GetSectionNames(tnstr_vec &section_names);
};

//bool CopyValue( TRegKey *srcreg, TRegKey *destreg );

#endif

