#ifndef	__DICNAME_H
#define	__DICNAME_H

#include "pdconfig.h"
#include "multidef.h"
#include "dicdef.h"
#include "diclist.h"

enum eDicNameFlag {
	DNF_DICTXDUP = 0x01,
};

struct DICNAME {
	tnstr name;
	bool readonly;
	bool nosearch;
#ifdef USE_COMP
	int comp;
#endif
	bool FastDB;
#if INETDIC
	bool internet;
#endif
#ifdef EPWING
	tnstr epwname;
	tnstr gtransname;
#endif
#ifdef USE_FILELINK
	tnstr flinkpath;
#endif

	// temporary members //
	unsigned num_words;
	unsigned file_size;
	tnstr FastDBName;
	int flags;

	DICNAME( );
	DICNAME( const tchar *name, bool readonly, bool nosearch
#ifdef USE_COMP
		, int comp
#endif
#ifdef EPWING
		, const tchar *epwname=NULL
		, const tchar *gtransname=NULL
#endif
#ifdef USE_FILELINK
		, const tchar *flinkpath=NULL
#endif
#if INETDIC
		, bool internet = false
#endif
	);
	DICNAME( const DICNAME & );
	const tchar *GetShortName( tnstr &name );
	const tchar *GetDicName( tnstr &_name );	// obsolete
	tnstr GetDicName();
	void SetDicName( const tchar *name );
	operator const tchar *()
		{ return (const tchar *)name; }
#ifdef EPWING
	const tchar *GetEPWingName( tnstr &name );
	const tchar *DICNAME::GetGConvName( tnstr &_name );
#else
	const tchar *GetEPWingName( tnstr & /* name */ )
		{ return NULL; }
	const tchar *GetGConvName( tnstr & /* name */ )
		{ return NULL; }
#endif
	const tchar *GetFileLinkPath( tnstr &name)
#ifdef USE_FILELINK
		;
#else
		{ return NULL; }
#endif
	bool CanComp()
#ifdef USE_COMP
    	{ return comp!=CP_NOCOMP; }
#else
		{ return false; }
#endif
#ifdef USE_COMP
	bool IsWellComp()
		{ return comp==CP_NOCOMP || comp==CP_COMP1 ? false : true; }
	void SetComp(int _comp)
		{ comp = _comp; }
	void SetWellComp(bool well)
    	{
        	if (well){
				comp = CP_COMP2|CP_TEXT;
			} else {
				comp = CP_COMP1;
			}
		}
#endif
};

class DicNames : FlexObjectArray<DICNAME> {
typedef FlexObjectArray<DICNAME> inherited;
protected:
	tnstr_vec FileNameTemp;	// Temporary file names for operator =tnstr_vec &.
public:
	tnstr basepath;	// 辞書グループを作成した時のPDICW32.EXEのpath
						// ファイル名が相対パスで、その相対パスでオープンできない時、
						// このpathをくっつけて再度オープンを行うこともできる
	tnstr LangProc;	// Language Processor
public:
	DicNames();
	DicNames(DicNames&);
	int filecomp( const tchar *fname, int dicno );
	void clear();
	void del( int i )
		{ inherited::del(i); }
	int get_num() const
		{ return inherited::get_num(); }
	void add( DICNAME *obj )
		{ inherited::add( obj ); }
	void insert( int i, DICNAME *obj )
		{ inherited::insert(i, obj); }
	void exchange( int i, int j )
		{ inherited::exchange(i, j); }
	void move(int src, int dst)
		{ inherited::move(src, dst); }
	void SetReadOnly( diclist_t flags );	// ビットフラグからreadonlyをセット
	diclist_t GetReadOnly( );				// readonlyをビットフラグへ
	void SetNoSearch( diclist_t flags );
	diclist_t GetNoSearch( );
#if INETDIC
	void SetInet( diclist_t flags );
	diclist_t GetInet( );
#endif
	operator tnstr_vec &();
	void SetNames( tnstr_vec &names );
	int Search( const tchar *filename );

//	int CanRecord( const tchar *filename );
	int add( const tchar *path, const tchar *str );
	int insert( int no, const tchar *path, const tchar *name );

	void operator = ( DicNames & );
	DICNAME &operator [](int i)	const {return *(DICNAME*)array[i];}
};


#if defined(PDIC32) || defined(CMD32)
class DicPath : public tnstr {
	public:
		void set( const tchar *str );
};
#endif

// 現在オープンしているマルチ辞書からファイル名と、オープン状態を取得する
void SetMultiDicName( class DicNames &dicname, class MultiPdic &dic );

#if	defined(PDICW) && !defined(PDOUT)
#else
#ifdef	MINI
int all_open( class MiniPdic &pdic, DicNames &dicnames );
#else
int all_open( class MultiPdic &pdic, DicNames &dicnames );
#endif
#endif

#endif	// __DICNAME_H
