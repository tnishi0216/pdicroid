#ifndef	__REXPGEN_H
#define	__REXPGEN_H

#include "tndefs.h"
#include "tnstr.h"

#ifdef _UNICODE
#include "bocu1.h"
#endif

#define	GRXP_NORMAL			0x000
#define	GRXP_IGNORECASE		0x001
#define	GRXP_FUZZY			0x002
#define	GRXP_WORD			0x004
#define	GRXP_ZENHAN			0x008
#define	GRXP_HIRA			0x010
#define	GRXP_ZENHANHIRA		0x018

//	General Rexp

enum GenericRexpError {
	GRE_NORMAL = 0,
	GRE_NOTREADY,	// DLLが利用できない
	GRE_MEMORY,		// メモリが足りない
	GRE_TOOLONG,	// 正規表現が長すぎる
	GRE_COMPLEX,	// 正規表現が複雑すぎる
	GRE_BRACKET,	// 括弧の対応が悪い
	GRE_CHARCLASS,	// キャラクタクラスが不正
	GRE_ESCAPE,		// エスケープシーケンスが理解できない
	GRE_SYNTAX,		// どこかの文法に誤りがある
	GRE_UNEXPECTED,	// 予期しないエラー（未定義エラー）
	GRE_DLLBUG,		// バグの発生
	GRE_APPBUG,		// アプリケーションのバグ発生
	GRE_VERSION,	// バージョンが異なるため利用できない
	GRE_NOSTRINGS,	// 検索文字列がない
	GRE_STOP,		// 中止した
	GRE_NOREGULAR	// 正規表現が無いため実行できない
};

struct TRexpMatchInfo {
	int loc;
	int len;
	TRexpMatchInfo(int _loc, int _len)
		:loc(_loc)
		,len(_len)
	{
	}
	TRexpMatchInfo(){}
};

typedef FlexObjectArray<TRexpMatchInfo> TMatchInfos;

class GenericRexp {
protected:
	tnstr pattern;
	bool matched;	// 最後のcompareでtrue/false?
public:
	int srchflag;	// 補助フラグ
#ifdef _UNICODE
protected:
	wchar_t *Buffer;
	int BufferLen;
#endif
public:
	GenericRexp( );
	virtual ~GenericRexp( );

	virtual BOOL Open( ) = 0;
	virtual void Close( ) = 0;
	virtual BOOL CanUse( ) = 0;

	virtual BOOL Compile( );
	virtual BOOL Compile(const tchar *str);
	virtual BOOL Compare( const tchar *str, TMatchInfos *matches=0, int user=0 );
	virtual BOOL GetMatch( int &len, int &loc, int user=0 );	// len : マッチ長 loc : マッチ位置

	virtual bool CompareEx(const tchar *pattern, const tchar *str, tnstr_vec &strs) = 0;

protected:
	virtual BOOL CompileImpl( const tchar *patern ) = 0;
	virtual BOOL CompareImpl( const tchar *str ) = 0;
	virtual BOOL GetMatchImpl( int &len, int &loc ) = 0;	// len : マッチ長 loc : マッチ位置
	virtual GenericRexp *Create() = 0;
	virtual GenericRexp *GetNextForComp(int user) const { return next; }

public:
	virtual int GetErrorCode( ) = 0;
	void SetFlag( int flag, BOOL f );
#ifdef _UNICODE
	bool CompareBocu1( const char *s, FNPreCodeTranslate );	// for BOCU1
	bool CompareUTF8( const char *s );	// for UTF-8
#endif
	void SetPattern(const tchar *_pattern)
		{ pattern = _pattern; }
	const tchar *GetPattern()
		{ return pattern; }
	virtual GenericRexp *Duplicate();

	// 連結正規表現検索 //
	GenericRexp *next;
	int connection;		// 0:AND 1:OR
	bool _not;			// 否定
};

#endif	// __REXPGEN_H
