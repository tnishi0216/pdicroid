#ifndef	__SRCHOUT_H
#define	__SRCHOUT_H

#include "japa.h"

// 検索結果出力用クラス(DdeDicManの基本クラスでもある)
class  SrchOutBase {
public:
	int outflag;		// 出力(0x01:英単語 0x02:レベル 0x04:訳 0x08:用例 0x10:発音記号 0x20:
	int output;			// 出力先(0:なし 0x01:クリップボード 0x02:ファイル)
	tnstr filename;		// 出力先ファイル名
	tnstr *buffer;		// 出力先文字列
	int format;			// 出力形式(DT_...)
	tnstr fmttemplate;	// ユーザ定義テンプレート
	class Squre *squ;
	class Dictionary *out;	// 出力先
	// Delayed処理用 //
	bool DelayedOutput;		// １単語分遅らせて出力する(for user templateの$$)
							// copyする処理が増えるのでdefalutではfalseにする
	bool NeedExtension;		// crypt辞書で制限される機能を利用している
	bool NeedDeleteFile;	// 出力ファイルをclose後削除する必要がある
protected:
	tnstr DWord;
	Japa DJapa;
	int DFreq;
public:
	SrchOutBase();
	virtual ~SrchOutBase();
	int Open( TWinControl *parent );
	void Close( );
	BOOL Output( const tchar *word, Japa &japa, int freq=0 );
};

// 検索結果ファイル/クリップボード出力用制御構造体
class SrchOut : public SrchOutBase {
public:
//	BOOL over;					// 上書き出力？(クリップボードでは新規、追加の指定)
	long maxnum;				// 最大出力単語数

	SrchOut( class Squre *_squ );	//TODO: to be deleted
//	SrchOut( class TSquareFrame *_squ );
	~SrchOut();
	int GetErrorCode( );
};

#endif	// __SRCHOUT_H

