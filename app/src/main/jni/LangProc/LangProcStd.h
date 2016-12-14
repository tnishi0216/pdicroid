//---------------------------------------------------------------------------

#ifndef LangProcStdH
#define LangProcStdH
//---------------------------------------------------------------------------

#include "LangProcBase.h"

//TODO: ‚Ù‚©‚Ì‚Æ‚±‚ë‚Å’è‹`‚·‚é‚©include‚·‚éŒ`‚ª—Ç‚¢‚¾‚ë‚¤
#ifdef _Windows
#define	USE_WEBSRCH		1
#else
#define	USE_WEBSRCH		0
#endif

//
// Standard Language Processor
//
class TLangProcStd : public TLangProcBase {
typedef TLangProcBase super;
public:
	enum { LangProcId = 0x00010000 };
public:
	TLangProcStd();
	~TLangProcStd();
	__override bool Open();
	__override bool OpenIrreg();
	__override bool Compare( struct COMPARE_STRUCT &cs, const int flags )
		{ return CompareStd(cs, flags); }
	__override int Search( COMPARE_STRUCT &cs, const tchar *words, tchar *str, MatchArray *HitWords )
		{ return SearchStd(cs, words, str, HitWords); }
protected:
	bool CompareStd( struct COMPARE_STRUCT &cs, const int flags );
	int SearchStd( COMPARE_STRUCT &cs, const tchar *words, tchar *str, MatchArray *HitWords );
	int FindLoop(COMPARE_STRUCT &cs);
public:
	// Poup Search related
	__override int SearchLongestWord( class MultiPdic *dic, const tchar *words, const tchar *prevwords, int curpos, int option, class MatchArray *HitWords );
	__override int SearchLongestWordExt(class MultiPdic *dic, const tchar *words, const tchar *prevwords, class MatchArray *HitWords, int &thread_key, FNLPSLWExtCallback callback, int user);
	__override int SearchLongestWordExtCmd(int cmd, int thread_key);

	// Morphologic Analysis //
	__override tnstr MakeMorphKeyword(const tchar *word);
	__override bool BuildMorphWords(const tchar *word, tnstr_vec &items, tnstr_vec *features);
protected:
	// SLW specific search //
	int SearchLongestWord( MultiPdic &dic, const tchar *words, const tchar *prevwords, int flags, MatchArray *HitWords );
	int mbSearchLongestWord( MultiPdic &dic, const tchar *words, int curpos, int /* flags */, MatchArray *found );
	int SearchLongestWordOptional( MultiPdic &dic, const tchar *words, const tchar *prevwords, int flags, MatchArray *HitWords );
	int mbSearchLongestWordOptional( MultiPdic &dic, const tchar *words, int curpos, int flags, MatchArray *found );
#if USE_WEBSRCH
	// web search thread //
	TSearchLongestWordExt searchLongestWordExt;
#endif
};

// standard lang proc old version
class TLangProcStd0 : public TLangProcStd {
typedef TLangProcStd super;
public:
	enum { LangProcId = 0x00000000 };
public:
	TLangProcStd0();
};

class TLangProcCust : public TLangProcStd {
typedef TLangProcStd super;
public:
	TLangProcCust(){}
};

#endif

