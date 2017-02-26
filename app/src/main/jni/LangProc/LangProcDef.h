//---------------------------------------------------------------------------

#ifndef LangProcDefH
#define LangProcDefH
//---------------------------------------------------------------------------

// 以下のビットを増やしたとき、cutword2.cppのBITCOUNT()のBitCount()のnbitsにも注意！！（現在32bitsに固定）
#define	SLW_ELIMHYPHEN		0x0000020F
#define	SLW_ELIMHYPHEN1		0x00000001	// ハイフンを削除して単語を結合
#define	SLW_ELIMHYPHEN2		0x00000002	// ハイフン以降を削除
#define	SLW_ELIMHYPHEN3		0x00000004	// ハイフン前半の単語を削除(ハイフンを削除)
#define	SLW_ELIMHYPHEN4		0x00000008	// ハイフンを半角スペースに変換
#define	SLW_ELIMHYPHEN5		0x00000200	// ハイフン前半の単語を削除（ハイフンを残す）

#define	SLW_REPLACE			0x000001F0
#define	SLW_REPLACEANY		0x00000010	// 置換チルダに対応
#define	SLW_REPLACEANY2		0x00000020	// 追加チルダに対応
#define	SLW_REPLACEIRREG	0x00000040	// 不規則変化辞書
#define	SLW_REPLACEDEL		0x00000080	// 一般的な単語を削除（a,anなど）
#define	SLW_REPLACEANY3		0x00000100	// __に対応
#define	SLW_REPLACEANYx		(SLW_REPLACEANY|SLW_REPLACEANY2|SLW_REPLACEANY3)

#if 0
#define	SLW_CASEIGNORE		0x00000E00
//#define	SLW_CASEIGNORE1		0x00000100	// 全文字小文字(内部処理用)
#define	SLW_CASEIGNORE2		0x00000200	// 先頭文字case反転(内部処理用)
#define	SLW_CASEIGNORE3		0x00000400	// 先頭以外全小文字
#define	SLW_CASEIGNORE4		0x00000800	// 全大文字
#endif

#define	SLW_ENGLISH			0x0000F000	// 英語の活用形処理
#define	SLW_DESINENCE1		0x00001000	// 語尾変化
#define	SLW_DESINENCE2		0x00002000	// 語尾変化2 (-dのみ) SLW_DESINECEと一緒に指定しないほうがbetter
#define	SLW_DESINENCE3		0x00004000	// -red,-led
#define	SLW_APOSTROPHE		0x00008000	// 省略形、所有形の'を削除(語尾変化とは両立しない)

#define	SLW_UK				0x00010000	// 英国式->米国式変換

#define	SLW_CONJUGATE		0x00020000	// 関連語

#define	SLW_SYMBOLS			0x00040000	// !?. の記号を省略する

#define	SLW_UMLAUT			0x00F00000	// UMLAUT1 + UMLAUT2 + UMLAUT3
#define	SLW_UMLAUT1			0x00100000	// umlaut --> ae, ...
#define	SLW_UMLAUT2			0x00200000	// ae,... --> umlaut
#define	SLW_UMLAUT3			0x00400000	// umlaut --> a,u,o,e,i
#define	SLW_UMLAUT4			0x00800000	// a,u,o,e,i --> umlaut

#define	SLW_DEUTCH			0x07000000	// SLW_DEUTCHx
#define	SLW_DEUTCH1			0x01000000	// ge-を削除
#define	SLW_DEUTCH2			0x02000000	// -en,-se,-e,-emを削除,-te -> -en, t -> -en
#define	SLW_DEUTCH3			0x04000000	// -nを削除

#define	SLW_WORDDELIM		0x08000000	// 英単語に,-があってもOK

#define	SLW_PENALTY			0x10000000	// penalty point (前置単語有りで二語目を含んだときにヒットしない)
#define	SLW_PENALTY2		0x20000000	// clicked wordが含まれずにhit

#define	SLW_SPELLCHECK		0x40000000	// スペルチェック専用

#define	SLW_COMPLETE		0x80000000	// 単語全体一致のみ

struct MATCHINFO {
	tnstr word;	// ヒット語
	tnstr ext;	// additional text
	int flag;		// 変換フラグ
	int numword;	// 検索語単語数
	int start;		// optional. 開始位置情報
	int point;		// SortHitWord用
	MATCHINFO( const tchar *_word, int _flag, int _numword, const tchar *_ext=NULL, int _start=0 )
		:word( _word )
		,ext(_ext)
	{
		flag = _flag;
		numword = _numword;
		start = _start;
	}
	MATCHINFO( MATCHINFO &mi )
		:word( mi.word )
		,ext(mi.ext)
	{
		flag = mi.flag;
		numword = mi.numword;
		start = mi.start;
	}
};

class MatchArray : public FlexObjectArray<MATCHINFO> {
typedef FlexObjectArray<MATCHINFO> inherited;
public:
	MatchArray( int slotsize=10 )
		:inherited( slotsize )
		{}
	void AddComp( const tchar *word, int flag, int numword );
	int AddComp( MATCHINFO *mi, bool first_pri=true );
	int AddCompLast( const tchar *word, int flag, int numword )
		{ return AddCompLast( new MATCHINFO(word, flag, numword) ); }
	int AddCompLast( MATCHINFO *mi );
	int FindWord(const tchar *word);
	void Add( const tchar *word, int flag, int numword )
		{ inherited::add( new MATCHINFO( word, flag, numword ) ); }
	// 一番近い単語を返す
	const tchar *GetTopWord()
		{ return get_num() ? (const tchar*)((*this)[ get_num()-1 ].word) : (const tchar*)NULL; }
};

struct COMPARE_STRUCT {
	int flags;
	const tchar *str;
	tchar *dp;
	const tchar *sp;
	const tchar *nextsp;
	bool fComplete;
	bool fWordDelim;
	class MultiPdic *dic;
	int srcflags;				// srccompのflag
	MatchArray *notrans_part;
	MatchArray *dstpart;
	MatchArray *dstcomp;
	MatchArray *dstcomp2;	// 準完全一致(英単語の区切り文字以降一致)

	int numword;			// 検索語の現在の単語数→2017.2.22 実際に検索に使用された語数に変更
	int orgnumword;			// 最後に検索語に使用されたときのnumword

	// 内部作業変数 //
	bool FoundHyphen;
};

// Search Longest Word external //
typedef int (*FNLPSLWExtCallback)(class TWebSearchThread *, int type, int param, int user);

enum {
	LPSLW_NONE = 0,
	LPSLW_COMPLETED,
	LPSLW_UPDATED,
//	LPSLW_UPDATED_SYNC,	// need sync
	LPSLW_CANCEL,
//	LPSLW_CANCEL_WAIT,	// 作業完了するまでwaitする
#if 0
	LPSLW_LOCKTHREAD,
	LPSLW_UNLOCKTHREAD,
	LPSLW_SYNCHRONIZE,
#endif
};

// Prototypes //
int compSortHitWords( MATCHINFO **a, MATCHINFO **b );
void SortHitWords( MatchArray &ma );
//void InsertHitWords( MatchArray &dest, MatchArray &src );
void InsertHitWords2( MatchArray &dest, MatchArray &src );
void InsertHitWords3( MatchArray &dest, MatchArray &src );

#endif
