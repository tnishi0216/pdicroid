//---------------------------------------------------------------------------

#include "tnlib.h"
#pragma hdrstop

#include "LangProcDef.h"
#include "LangProc.h"
#include "wordcount.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

// すでにpointが計算されている場合はそれを返す
int MATCHINFO::GetCalcPoint()
{
	if (point!=0)
		return point;
	return point = CalcPoint();
}

int MATCHINFO::CalcPoint() const
{
	const tchar *cword = find_cword_pos(word);
	int _numword = WordCount(cword);
	bool numsub = false;
	int _point = addpoint + _numword << 6;	// 64倍
	if (_numword < numword){
		// this->numwordは検索に使用した単語数であるため
		// この値とWordCount()との乖離で減点
		_point -= (numword - _numword)*1;
		numsub = true;	// 2011.2.2 added
	}
	if (flag & SLW_PENALTY2){
		// clicked wordがない場合はpenalty大
		_point -= 60;
		// 2018.12.11 ここの調整は難しい
		// "have reshuffled the deck when it comes to"でreshuffledをclickしてもhitしないため、
		// 60以上にする必要あり
	}
	if ((flag & SLW_ENGLISH) && cword[0] >= 'A' && cword[0] <= 'Z'){
		_point -= 16;	// 先頭が大文字の場合は固有名詞であるため変化形によるヒットはpenalty
	}
	if (flag==0){
		// none
		//_point += 64-1;	// premium point
		_point += 32-1;	// 2011.2.2 value changed.
	} else
	if (flag==SLW_PENALTY){
		// penaltyのみである場合はpremiumも減点も無し 2009.12.29
	} else
	if (flag&(SLW_ELIMHYPHEN2|SLW_ELIMHYPHEN3|SLW_PENALTY)){
		// -以前、以降の削除がある
		// ペナルティが大きい
		_point -= 64+1;	// １つ語数の少ない完全ヒット単語に劣る
	} else
	if ((flag&~(SLW_ELIMHYPHEN1|SLW_ELIMHYPHEN4|SLW_ENGLISH|SLW_SYMBOLS|SLW_REPLACEIRREG))==0){
		// 完全一致とほぼ同等
		_point--;
	} else {
		_point -= 16+BitCount(flag)*4;	// フラグのビット数に応じて減点
		if ((flag & SLW_REPLACEANY) && !numsub){
			_point -= 32;	// replace anyはさらに減点
							// ただし、すでに語数違いで引かれている場合は対象外(2011.2.2 added)
		}
	}
	return _point;
}

int MATCHINFO::ComparePoint(MATCHINFO &mi)
{
	return this->GetCalcPoint() - mi.GetCalcPoint();
}
// すでに計算されている場合は無駄な計算が発生する
int MATCHINFO::cComparePoint(const MATCHINFO &mi) const
{
	return this->CalcPoint() - mi.CalcPoint();
}
int MATCHINFO::CompareFlag(const MATCHINFO &mi) const
{
	return BitCount(this->point) - BitCount(mi.point);
}

void MatchArray::AddComp( const tchar *word, int flag, int numword, int point )
{
	MATCHINFO *mi = new MATCHINFO( word, flag, numword, point );

	int index = FindWord(word);
	if (index != -1){
		if ((*this)[index].ComparePoint(*mi) >= 0){
			// thisのほうがhigher point
			delete mi;
			return;
		}
		replace(index, mi);
	} else {
		inherited::add(mi);
	}
}

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
		const int point = m.CalcPoint();
		m.point = (point<<16) | i;	// 同じpoint同士で順番を維持するため
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

// mergeを行う
// dst += src
// dstの中にsrcと同じ単語があるかどうか探し、見つかった場合はflagが少ないほう、またはwordcountが多い方を優先する
// (たぶん同じだと思うが）
// そして、見つかった単語をsrcから削除する
// 見つからなかった場合は何もしない
void MergeHitWords(MatchArray &dst, MatchArray &src)
{
	for (int i=0;i<src.size();i++){
		for (int j=0;j<dst.size();j++){
			if (!_tcscmp(src[i].word, dst[j].word)){
				// found
				const int srccount = BitCount(src[i].flag);
				const int dstcount = BitCount(dst[j].flag);
				if (srccount <= dstcount){
					// srcのほうがflag bitが少ない
					if (srccount == dstcount){
						if (src[i].numword > dst[j].numword){
							// replace
							dst[j] = src[i];
						}
					} else {
						// replace
						dst[j] = src[i];
					}
				}
				// srcから重複分を削除
				src.del(i);
				i--;
				goto jnext;
			}
		}
jnext:;
	}
}

// srcにあるitemのうち、flag=0のみdstへmergeする
void MergeNoTransWords(MatchArray &dst, MatchArray &src)
{
	for (int i=0;i<src.size();i++){
		MATCHINFO &mi = src[i];
		if (mi.flag == 0 && mi.word.exist()){
			int index = dst.FindWord(mi.word);
			if (index >= 0){
				// すでに同じ単語がある
				if (dst[index].ComparePoint(mi) >= 0)
					continue;	// dstのほうがhigh priority
				dst[index] = mi;	// dst側を上書き
			} else {
				dst.AddCompLast(new MATCHINFO(mi));
			}
		}
	}
}
