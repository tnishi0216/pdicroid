//---------------------------------------------------------------------------

#ifndef LangProcBaseH
#define LangProcBaseH
//---------------------------------------------------------------------------

#include "LangProc.h"

//
// Language Processor base (abstract class)
//
// LangProcStdとLangProcSimpleで共通にできるものはここに置く
// より汎用性の高いもの、構造的なものはTLangProcへ
class TLangProcBase : public TLangProc {
typedef TLangProc super;
public:
	TLangProcBase();
	//__override bool Open();	// should override
	__override bool GetWord( const tchar *str, int pos, int &start, int &end, int &prevstart, bool fLongest, int wordcount=10, bool about=false );
	__override bool mbGetWord( const tchar *str, int pos, int &start, int &end, bool fLongest, int wordcount=10 );
	// Popup Search //
	//__override bool Compare( struct COMPARE_STRUCT &cs, const int flags )
	//	{ return CompareStd(cs, flags); }
	//__override int FindLoop(COMPARE_STRUCT &cs);
	__override const tchar *GetConjugateWords();
	//__override bool OpenIrreg();
public:
	static bool GetWordStd( const tchar *str, int pos, int &start, int &end, int &prevstart, bool fLongest, int wordcount=10, bool about=false );
	static bool mbGetWordStd( const tchar *str, int pos, int &start, int &end, bool fLongest, int wordcount=10 );
};



void trimright(tnstr &s);

#endif

