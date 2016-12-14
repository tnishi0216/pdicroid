//---------------------------------------------------------------------------

#include "tnlib.h"
#pragma hdrstop

#include "LangProcDef.h"
#include "LangProcBase.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

//---------------------------------------------------------------------------
//	TLangProcBase
//---------------------------------------------------------------------------
TLangProcBase::TLangProcBase()
{
}

bool TLangProcBase::GetWord( const tchar *str, int pos, int &start, int &end, int &prevstart, bool fLongest, int wordcount, bool about)
{
	return GetWordStd(str, pos, start, end, prevstart, fLongest, wordcount, about);
}
bool TLangProcBase::mbGetWord( const tchar *str, int pos, int &start, int &end, bool fLongest, int wordcount)
{
	return mbGetWordStd(str, pos, start, end, fLongest, wordcount);
}

#include "jtype.h"

#define	USE_PREV2	1	// ２つ前からの単語を切り出す

// posから単語を切り出す
// posはstrからのオフセット
// start,endはstrからのオフセットを返す
// prevstartは、startの１つ前の単語、無い場合はstartと同じ値
// fLongestがtrueの場合は、最高wordcount語までgetする(.)を終わりとみなす
// fMultiByte : false : 切り取り単位が英字以外
//              true  : 切り取り単位が全角を含めた区切り記号
// about   : いい加減なところをクリックしても単語さえあれば必ずgetする
//static
bool TLangProcBase::GetWordStd( const tchar *str, int pos, int &start, int &end, int &prevstart, bool fLongest, int wordcount, bool about )
{
	if ( !about )
		if ( !IsWordChar( *(str+pos) ) )
			return false;
//	while ( *(str+pos) == ' ' || *(str+pos) == '\t' ){
//		pos++;
//	}

	// 改行の場合は無し ver.4.05
	if ( str[pos] == '\r' || str[pos] == '\n' ){
		return false;
	}

	// posが文字列の終端、あるいは文字列の終端から１つ前の改行である場合はだめ //
	if ( !str[pos] || (!str[pos+1] && ((tuchar)str[pos] < ' ')) )
		return false; 

	const tchar *orgp = str;
	const tchar *p = orgp;
	const tchar *wordtop = NULL;
	while ( *p ){
		if ( IsWordChar( *p ) ){
			wordtop = p;
			break;
		}
		p++;
	}
	if ( !wordtop )
		wordtop = p; 
	const tchar *wordtail = NULL;
	const tchar *wordprev = NULL;
	const tchar *wordprev2 = NULL;
	bool fSpc = false;
	// posにある単語のstartとendを求める
	while ( *p ){
		if ( !IsWordChar( *p ) ){
			if ( orgp + pos < p ){
				wordtail = p;
				break;
			}
			fSpc = true;
		} else {
			if ( fSpc ){
#if USE_PREV2
				wordprev2 = wordprev;
#endif
				wordprev = wordtop;
				wordtop = p;
				fSpc = false;
			}
		}
//		p = CharNext( p );
		p++;
	}

	// wordtopがposより後ろ
	if ( !about && (wordtop > orgp + pos) ){
		// 単語の区切りだった
		return false;
	}

	if ( fLongest ){
//		int wordcount = 10;	// 10語まで
		while ( *p && *p != '.' ){
			if ( !IsWordChar( *p ) ){
				if ( !fSpc ){
					if ( --wordcount == 0 )
						break;
					fSpc = true;
				}
			} else {
				fSpc = false;
			}
			p = CharNext( p );
		}
		wordtail = p;
	}

	if ( !wordtail )
		wordtail = p;
	start = STR_DIFF( wordtop, orgp );
	end = STR_DIFF( wordtail, orgp );
	if ( start == end )
		return false;
	if ( wordprev ){
		prevstart = STR_DIFF( wordprev2 ? wordprev2 : wordprev, orgp );
	} else {
		prevstart = start;
	}
	return true;
}
//static
bool TLangProcBase::mbGetWordStd( const tchar *str, int pos, int &start, int &end, bool fLongest, int wordcount )
{
	ushort c;
	const tchar *sp = str + pos;
	LD_CHAR( c, sp );
	if ( !mbIsWordChar( c ) )
		return false;

	const tchar *orgp = str;
	const tchar *p = orgp;
	const tchar *wordtop = p;
	const tchar *wordtail = NULL;
	bool fSpc = false;
	while ( 1 ){
		sp = p;
		LD_CHAR( c, p );
		if ( (tuchar)c == 0x00 )
			break;
		if ( !mbIsWordChar( c ) ){
			if ( orgp + pos < sp ){
				wordtail = sp;
				break;
			}
			fSpc = true;
		} else {
			if ( fSpc ){
				wordtop = sp;
				fSpc = false;
			}
		}
	}
	p = sp;
	if ( wordtop > orgp + pos ){
		// 単語の区切りだった
		return false;
	}

	if ( fLongest ){
//		int wordcount = 10;	// 10語まで
		while ( 1 ){
			sp = p;
			LD_CHAR( c, p );
			if ( (tuchar)c == 0x00 )
				break;
			if ( c == '.' )
				break;
			if ( !mbIsWordChar( c ) ){
				if ( !fSpc ){
					if ( --wordcount == 0 )
						break;
					fSpc = true;
				}
			} else {
				fSpc = false;
			}
		}
		wordtail = sp;
	}

	if ( !wordtail )
		wordtail = sp;
	start = STR_DIFF( wordtop, orgp );
	end = STR_DIFF( wordtail, orgp );
	return true;
}

const tchar *TLangProcBase::GetConjugateWords()
{
	return _t("a;an;at;be;for;in;of;off;on;the;to;with");
}


// Helpers
//TODO: move to common
void trimright(tnstr &s)
{
	unsigned i;
	unsigned lastpos = -1;
	int l = s.length();
	for (i=0;i<l;i++){
		if ((tuchar)s[i]<=' ')
			lastpos = i;
		else
			lastpos = -1;
	}
	if (lastpos!=-1)
		s[lastpos] = '\0';
}
