#include <windows.h>
#include "bc.h"
#pragma	hdrstop
#include "tndefs.h"
#include "tnstr.h"

// nFileOffsetはOPENFILENAME構造体のnFileOffsetメンバ値
void GetCommFiles( const tchar *_str, tnstr_vec &files, int delim, int nFileOffset )
{
#if 1
	tchar *str;
	tchar *p;
	if ( delim ){
		str = new tchar[ _tcslen(_str) + 2 ];
		if (!str)
			return;
		_tcscpy( str, _str );
		p = str;
		while ( *p ){
			if ( *p == '|' ) *p++ = '\0';
			p = CharNext(p);
		}
		p++;
		*p = '\0';
	} else {
		str = (tchar*)_str;
	}
	tnstr curpath;
	tnstr filename;
	files.clear();

	if ( nFileOffset == -1 ){
		p = str + _tcslen(str) + 1;
		if ( *p != '\0' ){
			// ファイルが２つ以上ある
			nFileOffset = _tcslen(str) + 1;
		} else {
			// ファイルは１つ
			nFileOffset = 0;
		}
	}

	if ( nFileOffset < 1 || str[ nFileOffset - 1 ] == CHR_DIRSEP ){
		files.add( str );
		if ( str != _str ) delete[] str;
		return;
	}

    // 1998.3.13 nFileOffset-1を削除
	curpath.set( str /* , nFileOffset-1 */ );
	const tchar *cp = str + nFileOffset;
	// 1998.3.13 この行を追加
    if ( !*cp ) cp++;

	for ( ;*cp; ){
		tnstr *buf = new tnstr( curpath );
		if (!buf)
			break;
		const tchar *_cp = cp;
		bool fPath = false;	// パスを含む？
		for ( ;*cp; cp = CharNext( cp ) ){
			if ( *cp == ' ' )
				break;
			if ( *cp == CHR_DIRSEP )
				fPath = true;
		}
		if ( cp != _cp ){
			if ( !fPath ){
				buf->cat( _T(STR_DIRSEP) );
				buf->cat( _cp, STR_DIFF(cp,_cp) );
			} else {
				buf->set( _cp, STR_DIFF(cp,_cp) );
			}
			files.add( buf );
			if ( *cp )
				cp++;
			cp++;
		} else {
			delete buf;
			if ( str != _str ) delete[] str;
			return;
		}
	}
	if ( str != _str ) delete[] str;
#else
	TNChar curpath;
	TNChar filename;
	files.clear();

	const tchar *cp = str;
	const tchar *_cp = cp;
	for ( ; *cp ; cp = CharNext(cp) ){
		if ( *cp == ' ' )
			break;
	}
	if ( cp != _cp )
		curpath.set( _cp, STR_DIFF( cp, _cp ) );

	if ( *cp )
		cp++;
	else {
		if ( curpath[0] )
			files.add( new TNChar( curpath ) );
		return;
	}

	for ( ;*cp; ){
		TNChar *buf = new TNChar( curpath );
		_cp = cp;
		BOOL fPath = FALSE;	// パスを含む？
		for ( ;*cp; cp = CharNext( cp ) ){
			if ( *cp == ' ' )
				break;
			if ( *cp == CHR_DIRSEP )
				fPath = TRUE;
		}
		if ( cp != _cp ){
			if ( !fPath ){
				buf->cat( STR_DIRSEP );
				buf->cat( _cp, STR_DIFF(cp,_cp) );
			} else {
				buf->set( _cp, STR_DIFF(cp,_cp) );
			}
			files.add( buf );
			if ( *cp )
				cp++;
			cp++;
		} else {
			delete buf;
			return;
		}
	}
#endif
}
