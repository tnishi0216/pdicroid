#ifndef	__SRCHCOM_H
#define	__SRCHCOM_H

#include "pdconfig.h"

#include "rexpgen.h"

#include "srchstat.h"
#include "faststr.h"

#include "SearchDefs.h"

class TLangProc;

// len,pos,error以外をセットし、検索終了まで保持しておく
// swordを変更したら、Setupし直すこと
class SearchCommonStruct {
public:
	const tchar *sword;	// 検索語
protected:
	tnstrbuf sword_buf;
public:
	bool fIgnore;
	bool fRegular;
	bool fFuzzy;
	bool fWord;
//	bool fSingle;		// シングルバイト文字？
	GenericRexp *grexp;		// I'm not owner
	int pos;
	int error;
	TMatchInfos matches;	// Available in case of fRegular || fFuzzy.
	int match;			// 一致長（正規表現、曖昧検索時）
protected:
	int len;			// 内部処理用
	_tFindStr *fs;
#ifdef MIXMJ
	_jFindStr *jfs;
#endif
	TLangProc *LangProc;	// ref pointer
	tnstrbuf CompBuf;
public:
	SearchCommonStruct( );
	SearchCommonStruct( SrchStat *srchstat, TLangProc *lp, bool useAndSrch=false );
	~SearchCommonStruct();
	void Init(SrchStat *srchstat, TLangProc *lp, bool useAndSrch=false );
	void SetFS( const tchar *word, bool fIgnore );
	_tFindStr *GetTFS()
#ifdef MIXMJ
		{ return jfs; }
#else
		{ return fs; }
#endif
	int Setup(bool fRecompile=true);
	int Search(const tchar *str);
};


//int SearchCommonSetup( SearchCommonStruct &scs, bool fRecompile=TRUE );
//int SearchCommon( const tchar *str, SearchCommonStruct &scs );
bool SearchCommonError( HWND hwnd, int status, int errorno );

#endif	// __SRCHCOM_H

