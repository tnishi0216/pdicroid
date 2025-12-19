#include "tnlib.h"
#pragma hdrstop
#include "SrchMed.h"
#include "SrchPatParser.h"
#include "wordcount.h"
#include "pdstrlib.h"
#include "LangProc.h"
#include "BookmarkMan.h"
#include "pdprof.h"

//#ifdef _DEBUG
#if 0
#define	D	DBW
#else
inline void D(...) {}
#endif

static bool SetupRegs(Regs &regs, tnstr_vec &patterns);

bool TSpecialSearchParams::Setup(const tchar *str)
{
	// Start Search //
	SrchMode srchitem = SRCH_WORD;	// 検索対象項目
	srchmode = (SrchMode)(srchitem|SRCH_REGEXP|SRCH_IGN);
	andSrchmode = (SrchMode)0;
	const tchar *andNext;

	if (!SearchPatternParser(str, patterns, &andNext)){
		// error
		return false;
	}
	if (andNext){
		if (andNext[1]){
			if (!SearchPatternParser(andNext+1, andPatterns, NULL)){	// +1 means to skip '+' mark.
				// error
				return false;
			}
		} else {
			andNext = NULL;	// invalid
		}
	}

	if (patterns.get_num()==0){
		// no pattern. error.
		return false;
	}

	if (!SetupRegs(regs, patterns))
		return false;

	if (andNext && andPatterns.get_num()){
		andSrchmode = (SrchMode)(SRCH_JAPA|SRCH_REGEXP|SRCH_IGN);
		if (!SetupRegs(andRegs, andPatterns))
			return false;
		andWord = andNext+1;
	} else {
		andRegs.Close();
		andWord.clear();
	}
	
	return true;
}

static bool SetupRegs(Regs &regs, tnstr_vec &patterns)
{
	// Make pattern.
	regs.Open( );
	if (!regs.IsOpened()){
		return false;
	}
	const bool fuzzy = false;
	const bool wordalign = false;
	regs.Setup( 0, true, fuzzy, false, wordalign);
	regs.SetPattern(patterns[0]);
	Regs *curRegs = &regs;
	for (int i=1;i<patterns.get_num();i++){
		Regs *nregs = new Regs;
		curRegs->next = nregs;
		nregs->Open();
		nregs->Setup(0, true, fuzzy, false, wordalign);
		if (patterns[i][0]=='-'){
			nregs->_not = true;
			nregs->SetPattern(&patterns[i][1]);
		} else {
			nregs->_not = false;
			nregs->SetPattern(patterns[i]);
		}
		nregs->connection = 0;	// and
		curRegs = nregs;
	}
	return true;
}
//
// TSearchMediator class
//
#define	KCONV	0
// 2014.10.16 なぜbocuにわざわざ変換してから比較していたのか不明
// それにcomp_wordはkeyword部でしか比較しないためゆるい基準の比較になってしまう
#include "dic.h"
int TSubWordItems::BSearch( const tchar *str, unsigned char level )
{
	int left = 0;
	int right = get_num();
	if ( right == 0 ) return 0;
#if (MIXDIC || defined(KMIXDIC)) && KCONV
	_kwstrdef( _str, str );
#else
	#define	_str	str
#endif
	do {
		int mid = ( left + right ) /2;
		int k;
		if (level==(*this)[mid].level){
#if KCONV
			k = comp_word( _str, (const _kchar*)_kwstr((*this)[mid].word->c_str(), _str.GetTranslator()) );
#else
			k = comp_word( _str, (*this)[mid].word->c_str() );
#endif
		} else {
			k = (*this)[mid].level - level;	// ascending order
		}
		if (k == 0){
			return mid;
		}
		if (k < 0){
			right = mid;
		} else {
			left = mid + 1;
		}
	} while (left < right);
	return left;
}

// Add an item to the proper position in the array.
// return:
//	added position.
int TSubWordItems::add(tnstr *str, unsigned char level)
{
	int index = BSearch(str->c_str(), level);
	D("add:%d %d %ws", index, level, str->c_str());
	if (index>=get_num()){
		super::add(new TSubWordItem(str, level));
		return get_num()-1;
	} else {
		super::insert(index, new TSubWordItem(str, level));
		return index;
	}
}

int TSubWordItems::FindWord(const tchar *word, int startIndex)
{
	for (int i=startIndex;i<get_num();i++){
		if (*(*this)[i].word == word){
			return i;
		}
	}
	return -1;
}

//
// TSearchMediator class
//

#include "srchstat.h"
#include "windic.h"
#include "pool.h"
#include "winsqu.h"

TSearchMediator::TSearchMediator(Squre *_squ, MPdic &dic, SrchStat &_ss, SrchStat &_sub_ss, Pool &_pool, TSubWordItems &_sub_words, map<tnstr, bool> &_sub_words_map)
	:squ(_squ)
	,Dic(dic)
	,ss(_ss)
	,sub_ss(_sub_ss)
	,pool(_pool)
	,sub_words(_sub_words)
	,sub_words_map(_sub_words_map)
{
	UseFirstPhase = true;
	SubWordsIndex = -1;
	TopOffset = 0;
	LastFoundWordIndex = -1;
	NeedJump = false;
}

void TSearchMediator::ClearSubWords()
{
	sub_words_map.clear();
	sub_words.clear();
	SubWordsIndex = -1;
}

// return:
//	actual phase.
int TSearchMediator::SetupPhase(int phase, int state)
{
	TopOffset = 0;
	SubWordsIndex = pool.get_num();

	if (pool.get_num()>=MAX_NDISP){
		phase = 0;
	}
	return phase;
}

// return:
//	AS_NORMAL_SEARCHの場合は通常の辞書検索で取得
const tchar *TSearchMediator::GetWord()
{
	int type = ss.GetSearchType();
	if (!(type&SST_SUBSEARCH))
		return (const tchar*)AS_NORMAL_SEARCH;	// only normal search

	if (ss.fBackWard){
		if (SubWordsIndex<0)
			return (const tchar *)AS_END;
		__assert(SubWordsIndex>=0 && SubWordsIndex<sub_words.get_num());
		return *sub_words[SubWordsIndex--].word;
	} else {
		if (SubWordsIndex<0)
			return (const tchar *)AS_CONTINUE;	// 2009.5.26 added. 
		if (SubWordsIndex>=sub_words.get_num()
			/*|| SubWordsIndex>=TopOffset+pool.get_num()*/){	// 2015.10.10 条件を外した(全文検索でscrollされない不具合対策)
			if (sub_ss.IsSearching()){
				return (const tchar*)AS_CONTINUE;
			} else {
				return (const tchar*)AS_END;
			}
		}
		return *sub_words[SubWordsIndex++].word;
	}
}

// GetWord()で取得したwordを戻す
void TSearchMediator::RewindWord()
{
	int type = ss.GetSearchType();
	if (!(type&SST_SUBSEARCH))
		return;

	if (ss.fBackWard){
		if (SubWordsIndex<0)
			return;
		if (SubWordsIndex<sub_words.get_num()-1)
			SubWordsIndex++;
	} else {
		if (SubWordsIndex>=1)
			SubWordsIndex--;
	}
}

// return:
//	true - continue to sub search
//	false - end of search/use normal search
bool TSearchMediator::SetupNextSearch(bool backward, bool as_end)
{
	int type = ss.GetSearchType();
	if (!(type&SST_SUBSEARCH))
		return false;	// only normal search

	D("Setup:TopOffset=%d", TopOffset);
	if (ss.fBackWard==backward){
		// 同一方向
		if (as_end){
			// 現在の検索が完了したときの処理
			if (ss.fBackWard){
				return false;	// End of search.
			} else {
				if (sub_ss.IsSearching()){
					return true;	// continue to search
				} else {
					return false;	// end of search
				}
			}
		} else {
			if (ss.fBackWard){
				//SubWordsIndex = 0;
				return true;	// no search.
			} else {
				if (SubWordsIndex==-1){
					SubWordsIndex = pool.get_num()-TopOffset;	// 2014.10.5 SubWordsIndex=-1のときこれがないと継続検索できないため
				}
				return true;	// continue to search.
			}
		}
	} else {
		// 異なる方向
		__assert(!as_end);	// not supported for as_end=true.
		//TODO: 境界テストを必ずやること！！
		if (backward){
			SubWordsIndex = TopOffset-1;
			SetPhase(0);
			return true;
		} else {
			SubWordsIndex = TopOffset+pool.get_num();
			return true;
		}
	}
//	__assert__;	// never come here.
//	return false;
}

bool TSearchMediator::StartSpecialSearch(SrchStat &ss, const tchar *str, bool first_phase)
{
	return ::StartSpecialSearch(Dic, ss, ss.uDicList, str, first_phase, squ->GetSubSrchParam());
}

void TSearchMediator::RestartSpecialSearch(SrchStat &ss, const tchar *word)
{
	const tchar *jword;
	switch (ss.GetSearchPhase()){
		case SST_PHASE1:
			StartSpecialSearch(ss, word, true);
			jword = GetLastSearchWord();
			if (jword)
				Dic.JumpAS(jword, false);	// 順方向の一番最後に設定
			break;
		case SST_PHASE2:
			StartSpecialSearch(ss, word, false);
			jword = GetLastSearchWord();
			if (jword)
				Dic.JumpAS(jword, false);	// 順方向の一番最後に設定
			break;
		default:
			//__assert__;	// 検索対象外の場合
			break;
	}
}

bool TSearchMediator::AddPool( tnstr *word, Japa *j, int dicno )
{
#if 0
	if (ss.GetSearchType()&SST_SUBSEARCH && ss.GetSearchPhase()!=0){
		int index;
		if (ss.GetSearchType()&SST_PHASE1){
			if (map_find(sub_words_map, word->c_str())){
				// ignore it
				return true;
			}
			index = AddSubWord1(new tnstr(word->c_str()));
			sub_words_map[word->c_str()] = true;	//TODO: wordの構成ではじくようにする（メモリ節約のため）
			//DBW("A1:%d %ws", index, word->c_str());
		} else {
			__assert(ss.GetSearchType()&SST_PHASE2);
			if (map_find(sub_words_map, word->c_str())){
				// ignore it
				return true;
			}
			index = AddSubWord2(new tnstr(word->c_str()));
			//DBW("A2:%d %ws", index, word->c_str());
		}
		if (index<=SubWordsIndex){
			SubWordsIndex--;
		}
		if (index-TopOffset>=pool.get_num()){
			// Poolより後ろ
			if (pool.get_num()>=MAX_NDISP){
				// no operation to the pool.
			} else {
				squ->add(word, j, dicno);
			}
			return true;
		} else {
			if (index<TopOffset){
				// Poolより前にinsert
				TopOffset++;
				return true;
			} else {
				// Pool内
				squ->insert(index-TopOffset, word, j, dicno);
				return true;
			}
		}
	}
#endif
	// one item is added to the pool.
	D("AddPool:%d %d", TopOffset, pool.get_num());
	if (pool.get_num()==MAX_NDISP){
		// the top item will be dropped from the pool.
		TopOffset++;
	}
	bool ret = squ->add(word, j, dicno);
#ifdef _DEBUG
	if (ss.IsSubSearch() && sub_words.get_num()>0 && pool.get_num()>0){
		if (SubWordsIndex>=0){
			// sub_wordsがpoolへ１つ以上copyされている
			// poolの内容がsub_wordsのmirrorになっているかの確認
			if (TopOffset+pool.get_num()>sub_words.get_num()){
				D("TopOffset=%d pool=%d sub_words=%d SubWordsIndex=%d", TopOffset, pool.get_num(), sub_words.get_num(), SubWordsIndex);
				for (int i=0;i<pool.get_num();i++){
					if (sub_words.get_num()<=TopOffset+i){
						//__assert__;
						goto jexit;
					}
					if (_tcscmp(pool.fw[i], sub_words[TopOffset+i].word->c_str())){
						D("i=%d:%ws", i, pool.fw[i].c_str());
					}
				}
			}
			__assert(_tcscmp(pool.fw[0], sub_words[TopOffset].word->c_str())==0);
			__assert(_tcscmp(pool.fw[pool.get_num()-1], sub_words[TopOffset+pool.get_num()-1].word->c_str())==0);
		}
	jexit:;
	}
#endif
	return ret;
}
//TODO:
// 2014.9.16
// 従来からあるInsertPool()は、i==pool.get_num()のときはAddPool()内でUpdateProc()が呼ばれるが、
// i!=pool.get_num()のときは、UpdateProc()が呼ばれないという仕様上の問題があった。
// そのため、動作が明確のものはInsertPoolFixed()への呼び出しに変更すること。
int TSearchMediator::InsertPoolOld( int i, tnstr *word, Japa *j, int dicno)
{
	return InsertPoolFixed(i, word, j, dicno, false);
}
// i : the index on the pool.
// return:
//	1  = poolの先頭を破棄した(TopOffsetがずれ、SubWordsIndexに影響有り)
//	-1 = poolの最後を破棄した
//TODO:
//	correct引数は動作確認取れたら削除OK!!!
int TSearchMediator::InsertPoolFixed( int i, tnstr *word, Japa *j, int dicno, bool correct)
{
	if (i==pool.get_num()){
		AddPool(word, j, dicno);
		return 0;
	}
	__assert(ss.GetSearchPhase()==0);
	D("B:TopOffset=%d sub_words=%d pool.num=%d %d", TopOffset, sub_words.get_num(), pool.get_num(), SubWordsIndex);
	int ret = squ->insert2(i, word, j, dicno, false);
	D("B:%d", SubWordsIndex);
	if (ret==1){
		TopOffset++;
	} else
	if (i==0){
		if (TopOffset>0)
			TopOffset--;
	}
	if (correct){
		squ->UpdateProc(i);
	}
#ifdef _DEBUG
	if (SubWordsIndex>=0){
		if (sub_words.get_num()>0 && pool.get_num()>0){
			D("A:TopOffset=%d sub_words=%d", TopOffset, sub_words.get_num());
			D("top=%ws",find_cword_pos(pool.fw[0]));
			D(" %ws", find_cword_pos(sub_words[TopOffset].word->c_str()));
			D("last=%ws", find_cword_pos(pool.fw[pool.get_num()-1]));
			if (TopOffset+pool.get_num()<=sub_words.get_num()){
				D(" %ws", find_cword_pos(sub_words[TopOffset+pool.get_num()-1].word->c_str()));
				__assert(_tcscmp(pool.fw[0], sub_words[TopOffset].word->c_str())==0);
				__assert(_tcscmp(pool.fw[pool.get_num()-1], sub_words[TopOffset+pool.get_num()-1].word->c_str())==0);
			} else {
				__assert_debug;
				dbw("");
			}
		}
	}
#endif
	return ret;
}

int TSearchMediator::FindInsertPoint(const tchar *word)
{
	return squ->GetInsertLoc(word);
}

bool TSearchMediator::IsInPool(int pool_index)
{
	if (pool_index<0){
		return false;
	} else
	if (pool_index>=pool.get_num()){
		// Poolより後ろ
		if (pool.get_num()>=MAX_NDISP){
			return false;
		} else {
			return true;
		}
	} else {
		return true;
	}
}

// the pool_index on the sub_words is visible?
bool TSearchMediator::IsVisible(int pool_index, int index_offset, int last_index)
{
	if (pool_index<index_offset){
		// 表示されているpoolより前
		return false;
	} else
	if (pool_index<=index_offset+last_index+1){
		return true;
	} else {
		return false;
	}
}

// The ownership of the word is transferred.
void TSearchMediator::AddSubWord0(tnstr *word)
{
	sub_words.insert(0, new TSubWordItem(word, SLEV0));
	if (LastFoundWordIndex>=0)
		LastFoundWordIndex++;
}

// The ownership of the word is transferred.
// return:
//	the index whose origin is the top of the pool.
int TSearchMediator::AddSubWord1(tnstr *word)
{
	return AddSubWordX(word, SLEV1);
}

// The ownership of the word is transferred.
// return:
//	the index whose origin is the top of the pool.
int TSearchMediator::AddSubWord2(tnstr *word, bool no_level)
{
	return AddSubWordX(word, no_level ? NOLEV : SLEV2);
}

//Note:
// return valueは負の数になるときがある。
//	INT_MINのときがsub_wordsに追加できず、wordをdeleteする必要がある
int TSearchMediator::AddSubWordX(tnstr *word, unsigned char level_base)
{
	int addedIndex = sub_words.add(word, level_base==NOLEV ? 0 : level_base+CountLevel(word->c_str()));
	if (addedIndex < 0) return INT_MIN;

	LastFoundWordIndex = addedIndex;
	if (addedIndex<TopOffset){
		TopOffset++;
	}
	if (addedIndex<SubWordsIndex){
		SubWordsIndex++;
	}
	int index = addedIndex - TopOffset;	// the index on the pool.
//	__assert(!ss.IsSubSearch() || TopOffset+pool.get_num()<=sub_words.get_num());
	//Note:
	// incremental search開始時にsub_wordsをclearするが、poolはkeepするためこのassertionは不正
	if (ss.IsSubSearch()){
		// sub search is showing in the main.
		D("index=%d addedIndex=%d TopOffset=%d %d", index, addedIndex, TopOffset, SubWordsIndex);
		// Is the added item in the pool ?
		if (IsInPool(index)){
			D("InPool:pool=%d sub_words=%d", pool.get_num(), sub_words.get_num());
			Japa *j = new Japa;
			if (j){
				NeedJump = true;
				__EnableDDNBegin(CanDispDicName());
				int r = Dic.Read(word->c_str(), j, sub_ss.uDicList);
				__EnableDDNEnd();
				if (r){
					BM_JSetMark(find_cword_pos(word->c_str()), *j);
					// found the word
					// Add the item to the pool.

					// 2009.7.7 added the next two lines.
					// InsertPool()の呼び出しは２カ所から有り、index=0のときは挙動が異なる。
					// ここでのInsertPool()は、sub_wordsへの追加とpoolへの追加であるため、
					// TopOffsetは（最終的に）変わらないが、ほかでのInsertPool()は
					// すでにsub_wordsにあるものをpoolの先頭へ追加するため、TopOffsetが１つずれる。
					// InsertPool()は後者を前提にしているため、その帳尻を合わせるために、
					// ここでTopOffsetを１つ増やしている。
					// （そろそろ限界。全面的に書き直した方が良い）
					if (!squ->IsForceUpdate()){
						if (index==0 && index<pool.get_num())
							TopOffset++;
					}

					int ret = InsertPoolOld(index, new tnstr(*word), j, Dic.GetLastFindDicNo());
					//Note:
					// 本来Squre::SearchProc()の処理でsub_words->poolへ追加すべきだが、
					// SearchProc()ではランダムなinsertionに対応していない（対応が難しい）
					// そのため、ここで無理にpoolへ追加する
					// その副作用として、SubWordsIndexが本来あるべきところとずれる場合がある
					// そのための処理
					// SearchProc()でやっているbEnd処理も必要だが、
					// 処理が単純ではないため、ここで行わずSearchProc()に任せる
					if (ret==1){
						// 先頭部分をpoolから外した
						if (SubWordsIndex>=0 && SubWordsIndex<addedIndex){
							SubWordsIndex++;
						}
					} else
					if (ret==-1){
						// 最後の部分をpoolから外した
						if (SubWordsIndex>addedIndex){
							SubWordsIndex--;
						}
					} else {
						// 単純add/insert
						if (SubWordsIndex==addedIndex){
							SubWordsIndex++;
						}
					}
					// 2014.10.23
					// SubWordsIndex==TopOffset+pool.get_num() → SubWordsIndex>=TopOffset+pool.get_num()
					// に、条件を変更。pool full時に、pool内へinsertされた場合、pool[MAX_NDISP-1]がdeleteされ(追い出され)、
					// SubWordsIndexがpool外になってしまうため
					__assert(SubWordsIndex==-1 || SubWordsIndex==TopOffset-1 || SubWordsIndex>=TopOffset+pool.get_num()
						|| (TopOffset+pool.get_num()==sub_words.get_num() && SubWordsIndex==sub_words.get_num()-1));
					D("ret=%d index=%d TopOffset=%d SubWordsIndex=%d", ret, index, TopOffset, SubWordsIndex);
					if( !(SubWordsIndex==-1 || SubWordsIndex==TopOffset-1 || SubWordsIndex>=TopOffset+pool.get_num()
						|| (TopOffset+pool.get_num()==sub_words.get_num() && SubWordsIndex==sub_words.get_num()-1)) ){
						dbw("TopOffset=%d SubWordsIndex=%d pool.get_num=%d sub_words.get_num=%d", TopOffset, SubWordsIndex, pool.get_num(), sub_words.get_num());
					}
				} else {
					delete j;
					// deleted?
					Del(index);
					//return -1;
				}
			}
		}
	}
	return index;
}

int TSearchMediator::CountLevel(const tchar *word)
{
	int match = GetMatchLevel(word);
	D("%2d : %ws", match, find_cword_pos(word));	// match確認
	if (match==0)
		return 0;
	int ret = 15 - min(match-1, 15);
	return ret;
}

// return:
//	0 - no proper match info
//	1~  matched (1 is best matched)
int TSearchMediator::GetMatchLevel(const tchar *word)
{
	if (!sub_ss.grexp)
		return 0;
	TMatchInfos matches;
	if ( sub_ss.grexp->Compare( word, &matches ) ){
		if (matches.get_num()==0){
			//__assert__;	// 2014.10.23 ～を含まない 全文検索の場合ここに来る
			return 0;
		}
		if (matches.get_num()<2)
			return 0;
#if 1	// multi words
		// まず先頭から順にそれぞれの単語の距離を求める
		// それをもとにレベルを求める
		int num = matches.get_num();
		int *dist = new int[num];
		if (!dist)
			return 0;
		for (int i=0;i<num-1;i++){
			int loc0 = matches[i].loc;
			int loc1 = matches[i+1].loc;
			int len0 = matches[i].len;
			int len1 = matches[i+1].len;
			bool reversed;
			if (loc0<loc1){
				// normal order
				reversed = false;
			} else {
				// reverse
				reversed = true;
				swap(loc0, loc1);
				swap(len0, len1);
			}
			const tchar *p = skiptospcz(word+loc0+len0);
			if (!p){
				__assert__;
				delete[] dist;
				return 0;
			}
			dist[i] = WordCount(p, word+loc1);
			if (reversed)
				dist[i] = -dist[i];
		}
		int level = 1;
		for (int i=0;i<num-1;i++){
			if (dist[i]>=0){
				level += dist[i]*2;
			} else {
				level += (-dist[i])*2+1;
			}
		}
		delete[] dist;
		D("%d %d : %ws", num, level, find_cword_pos(word));
		return level;
#else	// double word
		int loc[2];
		int length[2];
		int offset;			//TODO: どういう優先順位にするか？
		if (matches[0].loc<matches[1].loc){
			// normal
			loc[0] = matches[0].loc;
			length[0] = matches[0].len;
			loc[1] = matches[1].loc;
			//length[1] = matches[1].len;
			offset = 1;
		} else {
			// reversed
			loc[0] = matches[1].loc;
			length[0] = matches[1].len;
			loc[1] = matches[0].loc;
			//length[1] = matches[0].len;
			offset = 2;
		}
		const tchar *p = skiptospcz(word+loc[0]+length[0]);
		if (!*p)
			return 0;
		int count = WordCount(p, word+loc[1]);
		return offset+count*2;
#endif
	} else {
		//__assert__;	// 2014.10.23 ～で始まらない などの全文検索の場合ここに来る
		return 0;
	}
}

void TSearchMediator::Del(int pool_index)
{
	sub_words.del(pool_index+TopOffset);
}

const tchar *TSearchMediator::GetTopSearchWord()
{
	return pool.fw[0];	// only normal search
}
const tchar *TSearchMediator::GetLastSearchWord()
{
	int type = ss.GetSearchType();
	if (!(type&SST_SUBSEARCH)){
		if (pool.get_num()==0)
			return NULL;
		return pool.fw[pool.get_num()-1];	// only normal search
	}
	if (LastFoundWordIndex<0 || LastFoundWordIndex>=sub_words.get_num())
		return NULL;
	return sub_words[LastFoundWordIndex].word->c_str();
}

bool TSearchMediator::CanDispDicName()
{
	return squ->CanDispDicName();
}

void TSearchMediator::SetPhase(int phase)
{
	ss.SetPhase(phase);
}

//
// common search function Squre and TSearchMediator.
//
bool StartSpecialSearch(MPdic &dic, SrchStat &ss, diclist_t uDicList, const tchar *str, bool first_phase, TMultiAllSearchParam *sub_srchparam)
{
	//D("StartSubSearch:%d", first_search);
	__assert(str);

	str = skipspc(str);
	if (!str[0])
		return false;	// no word
	
	// search setup //
	ss.uDicList = uDicList;

	// Setup the search status. //
	TSpecialSearchParams sparams;
	if (!sparams.Setup(str)){
		// error
	}
	int search_type = SST_SUBSEARCH | SST_SEARCH;
	SrchMode mode = (SrchMode)0;
	const tchar *srch_word;
	tnstr srch_word_buf;
	if (first_phase){
		// 頭出し検索
		search_type |= SST_PHASE1;
		mode = (SrchMode)(sparams.srchmode | SRCH_HEAD);
		const tchar *p = skiptospcz(str);
		srch_word_buf = dic.GetLangProc()->Normalize(tnstr(str, STR_DIFF(p,str)));	// keywordで検索
		srch_word = srch_word_buf;
		if (!srch_word[0])
			return false;	// no word
		sub_srchparam->multiThread = false;
	} else {
		// new search
		search_type |= SST_PHASE2;
		mode = sparams.srchmode;
		srch_word = str;
		sub_srchparam->multiThread = prof.ReadInteger(PFS_COMMON, PFS_SEARCHMT, true);
	}
	GenericRexp *grexp = sparams.regs.GetRexp();
	sparams.regs.Discard();	// release the ownership.
	ss.SetupSearch( search_type, srch_word, mode, ss.uDicList, WA_MINLEVEL, WA_MAXLEVEL, 0 );
	ss.SetGRexp(grexp);
#if USE_ANDSRCH
	// AND search
	grexp = sparams.andRegs.GetRexp();
	sparams.andRegs.Discard();	// release the ownership.
	ss.SetupAndSearch( sparams.andWord, sparams.andSrchmode, grexp );
#endif
	if (ss.IsRexp()){
		if ( ss.CanUseRexp() ){
			if ( !ss.Compile( ) ){
				// コンパイルエラー
				// 通常の検索を使用
				int error = ss.GetRexpErrorCode( );
				ss.SetGRexp( NULL );
#if USE_ANDSRCH
				ss.SetupAndSearch(sparams.andWord, sparams.andSrchmode, NULL);
#endif
				if ( error != GRE_NOREGULAR
					&& (str[0] && error != GRE_NOSTRINGS)){	// "など記号を検索するときに発生するため
					DBW("RegularExp Error:%d", error);
					return false;	// fatal error
				}
				// use normal search
			}
		} else {
			return false;	// Cannot use regular expression.
		}
	}

	ss.ResetCount();
	ss.StartSearch(false);

	__assert(ss.word[0]);
	sub_srchparam->Stop(true);
	dic.setAS(ss.word, ss.mode, ss.grexp, sub_srchparam);
	return true;
}

void TSearchMediator::debug()
{
//	dbw("%d %d %d %d %d %d", ss.GetSearchType(), sub_ss.GetSearchType(), SubWordsIndex, sub_words.get_num(), TopOffset, pool.get_num());
}
