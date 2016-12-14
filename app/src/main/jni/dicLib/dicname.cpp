//
//	辞書ファイル名管理クラス
//

#include "tnlib.h"
#pragma	hdrstop
#include	"multidic.h"
#include	"dicname.h"
#include "wpdcom.h"

#include "filestr.h"


DICNAME::DICNAME( )
{
	readonly = false;
	nosearch = false;
#ifdef USE_COMP
	comp = CP_COMP;
#endif
#if INETDIC
	internet = false;
#endif
	flags = 0;
}

DICNAME::DICNAME( const tchar *_name, bool _readonly, bool _nosearch
#ifdef USE_COMP
		, int _comp
#endif
#ifdef EPWING
		, const tchar *_epwname
		, const tchar *_gtransname
#endif
#ifdef USE_FILELINK
		, const tchar *_flinkpath
#endif
#if INETDIC
		, bool _internet
#endif
	)
{
	name = _name;

	// 絶対パス-->相対パス
	AbsToRel( name );
	readonly = _readonly;
	nosearch = _nosearch;
#ifdef USE_COMP
	comp = _comp;
#endif
	FastDB = false;
#ifdef EPWING
	if ( _epwname )
		epwname = _epwname;
	if ( _gtransname )
		gtransname = _gtransname;
#endif
#ifdef USE_FILELINK
	if (_flinkpath)
		flinkpath = _flinkpath;
#endif
#if INETDIC
	internet = _internet;
#endif
	num_words = 0;
	file_size = 0;
	flags = 0;
}

DICNAME::DICNAME( const DICNAME &dn )
{
	name = dn.name;
	readonly = dn.readonly;
#ifdef USE_COMP
	comp = dn.comp;
#endif
	FastDB = dn.FastDB;
#ifdef EPWING
	epwname = dn.epwname;
	gtransname = dn.gtransname;
#endif
#ifdef USE_FILELINK
	flinkpath = dn.flinkpath;
#endif
#if INETDIC
	internet = dn.internet;
#endif

	// temporary members //
	num_words = dn.num_words;
	file_size = dn.file_size;
	flags = dn.flags;
}

#ifdef EPWING
const tchar *DICNAME::GetEPWingName( tnstr &_name )
{
	if ( epwname[0] ){
		RelToAbs( epwname, _name );
		return _name;
	}
	_name.clear();
	return _name;
}
const tchar *DICNAME::GetGConvName( tnstr &_name )
{
	if ( gtransname[0] ){
		RelToAbs( gtransname, _name );
		return _name;
	}
	_name.clear();
	return _name;
}
#endif
#ifdef USE_FILELINK
const tchar *DICNAME::GetFileLinkPath( tnstr &_name )
{
	if ( flinkpath[0] ){
#if 1	// 2008.7.1
		_name = flinkpath;
#else
		RelToAbs( flinkpath, _name );
#endif
		return _name;
	}
	_name.clear();
	return _name;
}
#endif
const tchar *DICNAME::GetShortName( tnstr &_name )
{
	tchar buf[ L_FILENAME + 1 ];
	tnstr n;
	get_filename( GetDicName( n ), buf );
	_name = buf;
	return _name;
}
// obsolete
const tchar *DICNAME::GetDicName( tnstr &_name )
{
	RelToAbs( name, _name );
	return _name;
}
tnstr DICNAME::GetDicName()
{
	tnstr _name;
	RelToAbs(name, _name);
	return _name;
}
void DICNAME::SetDicName( const tchar *_name )
{
	name = _name;
	AbsToRel( name );
}

DicNames::DicNames( )
{
}
DicNames::DicNames(DicNames &o)
{
	this->operator = (o);
}

int DicNames::filecomp( const tchar *fname, int dicno )
{
	for ( int i=0;i<get_num();i++ ){
		if ( i == dicno )
			continue;
		if ( _tmbsicmp( (*this)[i].name, (tuchar*)fname ) == 0 ){
			return 0;
		}
	}
	return 1;
}
void DicNames::operator = ( DicNames &names )
{
	clear();
	for ( int i=0;i<names.get_num();i++ ){
		this->add( new DICNAME( names[i] ) );
	}
	LangProc = names.LangProc;
}

#ifdef PDICW
void DicNames::SetReadOnly( diclist_t flags )
{
	for ( int i=0;i<get_num();i++ ){
		(*this)[i].readonly = dl_check(flags, i);
	}
}

diclist_t DicNames::GetReadOnly( )
{
	dl_def_empty(flags);
	for ( int i=0;i<get_num();i++ ){
		if ( (*this)[i].readonly){
			dl_set(flags, i);
		}
	}
	return flags;
}
void DicNames::SetNoSearch( diclist_t flags )
{
	for ( int i=0;i<get_num();i++ ){
		(*this)[i].nosearch = dl_check(flags, i);
	}
}
diclist_t DicNames::GetNoSearch( )
{
	dl_def_empty(flags);
	for ( int i=0;i<get_num();i++ ){
		if ( (*this)[i].nosearch){
			dl_set(flags, i);
		}
	}
	return flags;
}

#endif
#if INETDIC
void DicNames::SetInet( diclist_t flags )
{
	for ( int i=0;i<get_num();i++ ){
		(*this)[i].internet = dl_check(flags, i);
	}
}
diclist_t DicNames::GetInet( )
{
	dl_def_empty(flags);
	for ( int i=0;i<get_num();i++ ){
		if ((*this)[i].internet){
			dl_set(flags, i);
		}
	}
	return flags;
}
#endif
void DicNames::clear()
{
	inherited::clear();
	LangProc.clear();
}
DicNames::operator tnstr_vec &()
{
	FileNameTemp.clear();
	for ( int i=0;i<get_num();i++ ){
		FileNameTemp.add( (*this)[i].name );
	}
	return FileNameTemp;
}
void DicNames::SetNames( tnstr_vec &names )
{
	clear();
	for ( int i=0;i<names.get_num();i++ ){
		add( new DICNAME( names[i], false, false,
#ifdef USE_COMP
			CP_COMP
#endif
			) );
	}
}

int DicNames::Search( const tchar *filename )
{
	for ( int i=0;i<get_num();i++ ){
		if ( IsSameFile( filename, (*this)[i].name ) ){
			return i;
		}
	}
	return -1;
}
#if 0
int DicNames::CanRecord( const tchar *filename )
{
	if ( get_num() >= MAX_MULTIDIC )
		return -2;	// too many
	if ( Search( filename ) != -1 )
		return -1;	// duplicate
	return 0;
}
#endif
#if defined(PDIC32)
#if 0
int all_open( MultiPdic &pdic, DicNames &dicnames )
{
	int r;

	pdic.AllClose();
	for ( int i=0;i<dicnames.get_num();i++ ){
		if ( !dicnames[i][0] )
			continue;
		if ( dicnames[i].readonly ){
			r = pdic.Open( -1, dicnames[i], NULL, NULL, DICFLAG_READONLY );
		} else {
			r = pdic.Open( -1, dicnames[i], NULL, NULL, DICFLAG_NONE );
		}
		pdic.SetIrregularDic( dicnames.IrregFile );
		if ( r )
			return r;
	}
	return 0;
}
#endif
#include "faststr.h"
// _fullpathを利用するようにしましょう！！
void DicPath::set( const tchar *str )
{
	tchar *p = _tfullpath( NULL, str, FLEN );
	if ( p ){
#ifdef DOS
		jstrupr( p );
#endif
		if ( jfstrchr( p, '*' ) != 0 || jfstrchr( p, '?' ) != 0 ){
			tchar *q = jfstrrchr( p, '\\' );
			if ( !q ){
				q = jfstrrchr( p, '/' );
			}
			if ( q == 0 ){
				*p = '\0';
			} else {
				*(q+1) = '\0';
			}
		}
//		addyen( p );
		__SetBuf__( p );
	}
}
int DicNames::add( const tchar *path, const tchar *str )
{
	return insert( get_num(), path, str );
}

// -1 : ファイル重複
// -2 : ファイル名が不正
int DicNames::insert( int no, const tchar *path, const tchar *name )
{
	tnstr pbuf;
	if ( name[0] == '^' ){
		if ( path ){	// pathがNULLのときはそのまま
			if ( makename( pbuf, path, name+1 ) == FALSE )
				return -2;
		} else {
			pbuf.set( name );
		}
	} else {
		if ( path ){	// pathがNULLのときはそのまま
			if ( makename( pbuf, path, name ) == FALSE )
				return -2;
		} else {
			pbuf.set( name );
		}
	}
	if ( filecomp( pbuf, -1 ) == 0 ){
		return -1;
	}
	inherited::insert( no, new DICNAME( pbuf, name[0] == '^' ) );
//	readonly.insert( no, name[0] == '^' );
	return 0;
}
#endif

