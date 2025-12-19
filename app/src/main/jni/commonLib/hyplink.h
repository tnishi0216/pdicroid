#ifndef __HYPLINK_H
#define	__HYPLINK_H

#include <stack>

//#include "draw2.h"
#include "CharHT.h"

#if USE_DT2
#define	USE_HYPLINK		1
#else
#define	USE_HYPLINK		0
#endif

// hyplink.cppのnamelist[]と一致させる
#define	HLT_NONE		0
#define	HLT_WORD		1
#define	HLT_WORD2		2
#define	HLT_HTTP		3
#define	HLT_HTTPS		4
#define	HLT_MAILTO		5
#define	HLT_FILE		6
#define	HLT_TEXT		7
#define	HLT_HTML		8
#define	HLT_TEXTFILE	9
#define	HLT_EPWING		0x10
#define	HLT_HTML_HREF	0x20	// <a href="url">...</a>
#define	HLT_HTML_BXTAG	0x30	// <fd title="title">...</m>

// SQM_...と同じ数値！
#define	HLI_PRON	0x08
#define	HLI_JAPA	0x01
#define	HLI_EXP		0x02
#define	HLI_WORD	0x10
#define	HLI_EPWING	0x40	// 0x10 -> 0x40 changed. (2012.8.12)

struct THyperLink {
	union {
		unsigned int unnamed;
		struct {
			uint8_t type;		// マウスクリック時の動作
			uint8_t item;		// 発音記号、日本語訳、用例・・・
			uint8_t bookno;	// EPWING用
			uint8_t bxtag : 1;	// HTML Tag(bx)
			uint8_t state : 7;	// 1:BoxOpen/0:Close for HLT_HTML_BXTAG
		};
	};
	tnstr key;		// 検索用の単語, URL for HLT_HTML_HREF, title for HLT_HTML_BXTAG
	THitArea area;	// マウスのヒットエリア
	int loc;		// 単語位置
	int length;		// 単語長さ
	int wordcount;	// EPWING用
	int datapos;	// EPWING用 Data Page
	class Pdic *dic;	// EPWING用文書識別

	THyperLink();
	~THyperLink();
	void Clear();
	bool HitTest( POINT pt )
		{ return area.HitTest( pt ); }
	void GetKeyWord( tnstr &word, const tchar *text );
	tchar *GetLink(const tchar *p, const tchar *_text, const tchar *text);
};

struct THyperLinkInfo {
	THyperLink *hl;
	const tchar *left;
	THyperLinkInfo(THyperLink *_hl, const tchar *_left)
		:hl(_hl)
		,left(_left)
	{}
};

class THyperLinks : public FlexObjectArray<THyperLink> {
public:
	bool done;
	int req_parse;	// HLI_xxx The flags that request for html hyper link parser when draw.

	// The belows are temporary vars. for html href parser.
	const tchar *top;
	const tchar *left;
	THyperLink *tag;
	int curitem;
	stack<THyperLinkInfo> hliStack;

protected:
	int nextIndex;

public:
	THyperLinks( );
	int ExtractStaticWords( uint8_t item, const tchar *text );

	void StartEnum();
	THyperLink *Next(uint8_t type);
};

extern "C" {
THyperLink * WINAPI HyperLinkAdd( THyperLinks *hls );
}

extern class Pdic *HypLinkCurrentDic;
extern uint8_t HypLinkCurrentItem;

#endif
