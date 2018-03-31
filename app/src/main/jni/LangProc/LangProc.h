#ifndef LangProcH
#define LangProcH
//---------------------------------------------------------------------------

// LangProc Id structure
// bit31    : experimental/temporal bit
// bit30-16 : identifier. 0x0000-0x0FFF are reserved for developper.
// bit15-8  : reserved for future
// bit7-0   : ID for LayerA
// 0xFFFF00F0-0xFFFFFFFE can be used for test, local only.
// 0xFFFFFFFF is inhibitted.

/*
0x00000000
0x00010000 : 標準
0x0001FF00 : 汎用
0x00020000 : simple
0x00020001 : simple(old)
0x80100010 : チェコ語(試作版)
0x00100011 : チェコ語(正式版)
Note: 現在のPDICはlayerAが同じであればopenできる仕様になっている
*/

#define	LPID_LA_MASK	0xFF	// LayerA bit mask

// LangProc Id structure - old style
// bit31 : experimental/temporal bit
// bit30-16 : identifier. 0x0000-0x0FFF are reserved for developpers.
// bit15-8 : reserved
// bit7-0  : minor version of the langproc identified by the bit30-16.
// 0xFFFF0000-0xFFFFFFFE can be used for test, local only.
// 0xFFFFFFFF is inhibitted.

#include <vector>
#include <map>

using namespace std;

#define	USE_LPM		0	// 今は使っていない？

#include "LangProcDef.h"
#include "LPTable.h"
#if USE_LPM
#include "LPMLoader.h"
#endif

#include "IrregDic.h"

#ifndef struct_TKCodeTranslateSet
#define	struct_TKCodeTranslateSet
struct TKCodeTranslateSet {
	FNPreCodeTranslate encodeKT;
	FNPreCodeTranslate decodeKT;
};
#endif

class TIrregDics : public FlexObjectArray<TIrregDic> {
public:
	bool IsModified() const;
	bool Read(int index, const tchar *filename, tnstr &s);
};

// Language Processor base/abstract class
class TLangProc {
protected:
	//tnstr Name;
	tnstr FileName;
	TLPTable *Table;
	class TLPMLoader *LPMLoader;
	tchar *Buffer;
	int BufferSize;
	TKCodeTranslateSet KCodeTranslate;	// set by subclass if needed.

	tnstr SrchPatFile;
	TLPTable *SPTable;
	TKCodeTranslateSet FCodeTranslate;	// code translator for fuzzy search
public:
	TLangProc();
	virtual ~TLangProc();
	void IncRef();
	virtual bool Open() = 0;
	void Close();
	void Release();
	void Free() { Release(); }

	// Keyword code table //
	inline const TKCodeTranslateSet *GetKCodeTrans() const
		{ return &KCodeTranslate; }
	inline FNPreCodeTranslate GetEncodeKT()
		{ return KCodeTranslate.encodeKT; }
	inline FNPreCodeTranslate GetDecodeKT()
		{ return KCodeTranslate.decodeKT; }

	// Kyeword generator
	virtual tnstr CompositeWord(const tchar *word);
	tnstr KWord(const tchar *word)
		{ return Normalize(word); }
	
	// Keyword related //
	virtual tnstr Normalize(const tchar *str);

	// Search Pattern
	bool OpenSPTable();
	inline FNPreCodeTranslate GetEncodeFT()
		{ return FCodeTranslate.encodeKT; }
	virtual tchar *NormalizeSearchPattern(tchar *str);

	virtual const tchar *GetAddWords() const;
	virtual bool GetWord( const tchar *str, int pos, int &start, int &end, int &prevstart, bool fLongest, int wordcount=10, bool about=false ) = 0;
	virtual bool mbGetWord( const tchar *str, int pos, int &start, int &end, bool fLongest, int wordcount=10 ) = 0;

	// Popup Search related //
	virtual bool Compare( struct COMPARE_STRUCT &cs, const int _flags ) = 0;
	virtual int Search( COMPARE_STRUCT &cs, const tchar *words, tchar *str, MatchArray *HitWords ) = 0;
	//virtual int FindLoop(COMPARE_STRUCT &cs) = 0;
	virtual const tchar *GetConjugateWords() = 0;
	virtual int SearchLongestWord( class MultiPdic *dic, const tchar *words, const tchar *prevwords, int curpos, int option, class MatchArray *HitWords ) = 0;
	virtual int SearchLongestWordExt(class MultiPdic *dic, const tchar *words, const tchar *prevwords, class MatchArray *HitWords, int &thread_key, FNLPSLWExtCallback callback, int user)
		{ return 0; }
	virtual int SearchLongestWordExtCmd(int cmd, int thread_key){ return 0; }
	
	// Morphologic Analysis //
	virtual tnstr MakeMorphKeyword(const tchar *word){ return NULL; }
	virtual bool BuildMorphWords(const tchar *word, tnstr_vec &items, tnstr_vec *features){ return false; }
protected:
	bool Open(const tchar *filename);
	bool OpenTable(const tchar *table_filename=NULL);
	void CloseTable();

	bool OpenLPM(const tchar *filename);
	void CloseLPM();

	bool OpenSPTable(const tchar *filename);
	void CloseSPTable();

	tchar *GetBuffer(int size);
	void FreeBuffer();

	// 不規則変化
protected:
	tnstr_vec IrregNames;
	TIrregDics Irregs;	// 不規則変化辞書
	class TBiPoolJ *IrregPool;
	bool IrregCheckCase;	// normalize前の単語も比較する
public:
	void AddIrregFile(const tchar *filename);
	bool IsIrregOpened()
		{ return IrregPool!=NULL; }
	virtual bool OpenIrreg();
	void CloseIrreg();
	bool SearchIrreg( const tchar *word, tnstr &trsword );
};

const tchar *find_cword_pos(const tchar *word);
tnstr join_word(const tchar *cword, const tchar *kword);
const tchar *find_cword_pos(const tchar *word);
inline bool is_composit_word(const tchar *word)
	{ return find_cword_pos(word)!=word; }

const tchar *FindWordTop(const tchar *word, int offset, const tchar **top2=NULL);

#endif

