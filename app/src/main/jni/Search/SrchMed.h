#ifndef __SrchMed_h
#define	__SrchMed_h

#include "regs.h"
#include "dicdef.h"
#include "diclist.h"
#include "wsqudef.h"

#define	USE_ANDSRCH			1	// +Ç…ÇÊÇÈñÛåÍåüçı (also defined in srchstat.h)

struct TSpecialSearchParams {
	Regs regs;
	SrchMode srchmode;
	tnstr_vec patterns;
	bool Setup(const tchar *str);
#if USE_ANDSRCH
	tnstr andWord;
	Regs andRegs;
	SrchMode andSrchmode;
	tnstr_vec andPatterns;
#endif
};

// level values
// bigger is top.
#define	SLEV0		0xF0	// popoup search
#define	SLEV1		0x80	// first phase search
#define	SLEV2		0x00	// second phase search
#define	NOLEV		0xFF

#define	MAX_SUBSEARCH	100000	// ç≈ëÂsub search words

struct TSubWordItem {
	tnstr *word;
	unsigned char level;
	TSubWordItem()
	{
		word = NULL;
		level = 0;
	}
	TSubWordItem(tnstr *s)
		:word(s)
	{
		level = 0;
	}
	TSubWordItem(tnstr *s, int _level)
		:word(s)
		,level(_level)
	{
	}
	~TSubWordItem()
	{
		if (word)
			delete word;
	}
};

class TSubWordItems : public FlexObjectArray<TSubWordItem> {
typedef FlexObjectArray<TSubWordItem> super;
protected:
	const struct TKCodeTranslateSet *KCodeTrans;
	const struct TKCodeTranslateSet *GetKCodeTrans() const
		{ return KCodeTrans; }
public:
	TSubWordItems(const TKCodeTranslateSet *trans)
		:KCodeTrans(trans){}
	int BSearch( const tchar *str, unsigned char level );
	void insert(int index, TSubWordItem *obj)
	{
		super::insert(index, obj);
	}
	int add(tnstr *str, unsigned char level=0);
	const tchar *GetWord(int index)
		{ return (*this)[index].word->c_str(); }
	int FindWord(const tchar *word, int startIndex=0);
};

#define	AS_NORMAL_SEARCH	0xFF

//TODO: Ç¢Ç∏ÇÍSqure classÇÃinner classÇ…
class TSearchMediator {
protected:
	bool UseFirstPhase;
	class Squre *squ;
	class MPdic &Dic;
	struct SrchStat &ss;	// main search status
	struct SrchStat &sub_ss;
	class Pool &pool;
	TSubWordItems &sub_words;
	map<tnstr, bool> &sub_words_map;	// duplicate check
	int LastFoundWordIndex;	// the index on the sub_words that is to get the latest word to restart the search.
	int SubWordsIndex;	// sub words absolute index in the sub_words to be read.
	int TopOffset;		// the offset from the top of the sub_words to the top of the pool.
public:
	bool NeedJump;
public:
	TSearchMediator(Squre *_squ, MPdic &dic, SrchStat &_ss, SrchStat &_sub_ss, Pool &_pool, TSubWordItems &_sub_words, map<tnstr, bool> &_sub_words_map);
	bool IsFirstPhaseUsed() const
		{ return UseFirstPhase; }

	// for debug
	inline  int GetSubWordsIndex() const
		{ return SubWordsIndex; }
	inline int GetTopOffset() const
		{ return TopOffset; }
		
	void ClearSubWords();
	int SetupPhase(int phase, int state);
	const tchar *GetWord();
	void RewindWord();
	bool SetupNextSearch(bool backward, bool as_end);
	bool StartSpecialSearch(SrchStat &ss, const tchar *str, bool first_phase);
	void RestartSpecialSearch(SrchStat &ss, const tchar *word);
	// Operation of pool //
	bool AddPool( tnstr *word, Japa *j, int dicno );
	int InsertPoolOld( int i, tnstr *word, Japa *j, int dicno);
	int InsertPoolFixed( int i, tnstr *word, Japa *j, int dicno, bool correct );
	int FindInsertPoint(const tchar *word);
	bool IsInPool(int pool_index);
	bool IsVisible(int pool_index, int index_offset, int last_index);

	void AddSubWord0(tnstr *word);
	int AddSubWord1(tnstr *word);
	int AddSubWord2(tnstr *word, bool no_level);
protected:
	int AddSubWordX(tnstr *word, unsigned char level_base);
public:
	int CountLevel(const tchar *word);
	int GetMatchLevel(const tchar *word);

	void Del(int pool_index);
	
	const tchar *GetTopSearchWord();
	const tchar *GetLastSearchWord();

	bool CanDispDicName();
	
protected:
	void SetPhase(int phase);

public:
	void debug();
};

bool StartSpecialSearch(MPdic &dic, SrchStat &ss, diclist_t uDicList, const tchar *str, bool first_phase, class TMultiAllSearchParam *srchparam);

#endif	/* __SrchMed_h */

