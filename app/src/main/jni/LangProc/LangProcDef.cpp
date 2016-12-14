//---------------------------------------------------------------------------

#include "tnlib.h"
#pragma hdrstop

#include "LangProcDef.h"
#include "LangProc.h"
#include "wordcount.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

// return:
//	added/replaced index, otherwise -1.
int MatchArray::AddComp( MATCHINFO *mi, bool first_pri )
{
	int index = FindWord(mi->word);
	if (index!=-1){
		if (first_pri){
			// 先入れ優先
			delete mi;
			return -1;
		} else {
			// 後入れ優先
			replace(index, mi);
			return index;
		}
	}
	inherited::add( mi );
	return get_num()-1;
}

int MatchArray::AddCompLast( MATCHINFO *mi )
{
	int index = FindWord(mi->word);
	if (index!=-1){
		// すでにあるものを削除して最後に追加
		del(index);
	}
	inherited::add( mi );
	return get_num()-1;
}

int MatchArray::FindWord(const tchar *word)
{
	for ( int i=0;i<get_num();i++ ){
		if ( !_tcscmp( (*this)[i].word, word ) ){
			return i;
		}
	}
	return -1;
}

int compSortHitWords( MATCHINFO **a, MATCHINFO **b )
{
	int ret = (*a)->point - (*b)->point;
	if (ret)
		return ret;
	return _tcscmp((*a)->word, (*b)->word);
}
// Search()で得られた結果から語数を考慮した優先度に並び替える
// ※Search()は語数を考慮に入れていない
void SortHitWords( MatchArray &ma )
{
	for ( int i=0;i<ma.get_num();i++ ){
		MATCHINFO &m = ma[i];
		int numword = WordCount(find_cword_pos(m.word));
		bool numsub = false;
		m.point = numword << 6;	// 64倍
		if (numword<m.numword){
			// m.numwordは検索に使用した単語数であるため
			// この値とWordCount()との乖離で減点
			m.point -= (m.numword-numword)*1;
			numsub = true;	// 2011.2.2 added
		}
		if (m.flag==0){
			// none
			//m.point += 64-1;	// premium point
			m.point += 32-1;	// 2011.2.2 value changed.
		} else
		if (m.flag==SLW_PENALTY){
			// penaltyのみである場合はpremiumも減点も無し 2009.12.29
		} else
		if (m.flag&(SLW_ELIMHYPHEN2|SLW_ELIMHYPHEN3|SLW_PENALTY)){
			// -以前、以降の削除がある
			// ペナルティが大きい
			m.point -= 64+1;	// １つ語数の少ない完全ヒット単語に劣る
		} else
		if ((m.flag&~(SLW_ELIMHYPHEN1|SLW_ELIMHYPHEN4|SLW_ENGLISH|SLW_SYMBOLS|SLW_REPLACEIRREG))==0){
			// 完全一致とほぼ同等
			m.point--;
		} else {
			m.point -= 16+BitCount(m.flag)*4;	// フラグのビット数に応じて減点
			if ((m.flag & SLW_REPLACEANY) && !numsub){
				m.point -= 32;	// replace anyはさらに減点
								// ただし、すでに語数違いで引かれている場合は対象外(2011.2.2 added)
			}
		}
		m.point = (m.point<<16) | i;	// 同じpoint同士で順番を維持するため
		//DBW("i=%d p=%08X f=%08X numword=%d : %ws", i, m.point, m.flag, numword, find_cword_pos(m.word));
	}
	ma.sort( compSortHitWords );
}

#if 0
void InsertHitWords( MatchArray &dest, MatchArray &src )
{
	// merge & sortを行う //
	// dst += src
	// 検索語と一致している単語数の順番に並び替える
	// 一致している単語数が同じ場合は、
	// ヒットした単語の単語数の多い順(最後からみて)に並びかえる
	// 同じ単語数の場合は、ここでの検索結果を前のほうに置く
	int j = dest.get_num() - 1;
	int orgnum;
	if ( j >= 0 ){
		orgnum = WordCount( dest[j].word );
	}
	int i;
	for ( i=src.get_num()-1;i>=0;i-- ){
		if ( j >= 0 ){
			// 一致単語数による比較
			if ( (dest[j].numword > src[i].numword)
				|| ( (dest[j].numword == src[i].numword)
						&& (BitCount( dest[j].flag ) < BitCount( src[i].flag ))
					)
				 ){
				// destのほうが優先度高い
				if ( j <= 0 ){
					j = -1;
					break;
				}
				j--;
				orgnum = WordCount( dest[j].word );
				i++;
				continue;
			}
			int num = WordCount( src[i].word );
			for (;;){
				if ( orgnum >= num ){
					// destのほうが長いor同じ
					// dest indexを１つ前へ
					if ( j <= 0 ){
						j = -1;
						break;
					}
					j--;
					orgnum = WordCount( dest[j].word );
					continue;
				}
				break;
			}
		}
		dest.insert( j+1, src.discard(i) );
	}
	for ( ;i>=0;i-- ){
		dest.insert( 0, src.discard(i) );
	}
}
#endif

// merge & sortを行う //
// dst += src
//
// InsertHitWords()と異なり、flagのbit数を優先し、同一のbit数に限り語数が多い方を優先に扱う
void InsertHitWords2( MatchArray &dest, MatchArray &src )
{
	int j = dest.get_num() - 1;
	int orgnum;
	if ( j >= 0 ){
		orgnum = BitCount( dest[j].flag );
	}
	int i;
	for ( i=src.get_num()-1;i>=0;i-- ){
		if ( j >= 0 ){
			// 一致単語数による比較
			if ( (BitCount( dest[j].flag ) < BitCount( src[i].flag ))
				|| ( (BitCount( dest[j].flag ) == BitCount( src[i].flag ))
						&& dest[j].numword > src[i].numword
					)
				){
				// destのほうが優先度高い
				if ( j <= 0 ){
					//j = -1;
					break;
				}
				j--;
				orgnum = BitCount( dest[j].flag );
				i++;	// 2009.12.28 こんな大バグがあったとは・・・
				continue;
			}
			int num = BitCount( src[i].flag );
			for (;;){
				if ( orgnum <= num ){
					// destのほうがヒット率高いor同じ
					// dest indexを１つ前へ
					if ( j <= 0 ){
						j = -1;
						break;
					}
					j--;
					orgnum = BitCount( dest[j].flag );
					continue;
				}
				break;
			}
		}
		dest.insert( j+1, src.discard(i) );
	}
	for ( ;i>=0;i-- ){
		dest.insert( 0, src.discard(i) );
	}
}

// 単純に追加
void InsertHitWords3( MatchArray &dest, MatchArray &src )
{
	for ( int i=src.get_num()-1;i>=0;i-- ){
		dest.add( src.discard(i) );
	}
}
