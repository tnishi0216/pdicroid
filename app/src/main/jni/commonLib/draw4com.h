#ifndef __draw4com_h
#define	__draw4com_h

// Draw4 common components //

#include "tnfont.h"

#define	DF_OPAQUE		0x0002		// =2
#define	DF_UNREDRAW		0x0100		// 選択範囲表示で再描画をする必要はない(JLOle::Draw()用)
#define	DF_WHOLECHAR	0x0200		// ヒットテストで文字単位でヒットテストを行なう(通常は文字の半分単位）
#define	DF_CALCRECT		0x0400		// ヒットテストまたは表示範囲を求める
#define	DF_REVERSE		0x1000		// 反転表示

// 以下、DrawTextと関係のないフラグ
#define	DF_NODISP		0x0000		//*非表示
#define	DF_DISP			0x0001		// 実際に表示する
#define	DF_SELECTED		0x0008		//*選択
#define	DF_DRAWICON		0x0010		//*アイコン表示にする
#define	DF_UNSELECTED	0x0020		//*選択解除(DF_UNREDRAWも必ず付加される)
#define	DF_DRAWNAME		0x0800		//*名前のみ表示する
#define	DF_PINPOINT		0x2000		//*ピンポイント表示
#define	DF_PAINT		0x4000		//*WM_PAINTによって描画中
#define	DF_NOTITLE		0x10000		//*オブジェクトタイトルは表示しない 
#define	DF_ONLYUNDERLINE	0x20000	//* PDIC(EPWING)専用 アンダーラインのみ(DF_SELECTED(外枠)をomitteさせる)

#include <list>
#include <stack>
using namespace std;

struct THitArea;

#ifndef __EnphText
#define	__EnphText
// 強調文字列(DrawText2へは配列で渡す)
struct EnphText {
	int start;
	int end;
	int wordcount;	// for EPWING
	THitArea *area;
//	int type;	// 0:under line 1:link color

//    COLORREF color;
	EnphText()
		{ area = NULL; /*type=0;*/ }

	EnphText &operator = (const EnphText &o)
		{ assign(o); return *this; }
	void assign(const EnphText &o)
	{
		start = o.start;
		end = o.end;
		wordcount = o.wordcount;
		area = o.area;
		//type = o.type;
	}
	//TODO: to be removed if type is no longer needed.
	inline bool isUnderline() const { return true; }
	inline int getType() const { return 0; }
	inline void setType(int t){}
};

class EnphTextVec : public vector<EnphText> {
public:
	void Sort();
};
#endif

class EnphTextWork {
protected:
	struct LocList {
		int loc;
		int index;
		int seq;	// sequence number to sort
	};
	static int sortfunc( const void *a, const void *b );
	static int sortloclist_by_loc( const void *a, const void *b );
	static int sortloclist_by_seq( const void *a, const void *b );
	int sEnphSort( const EnphTextVec &enph, LocList *loclist );
	int EnphSort()
		{ return sEnphSort(*et, ccLoc); }

	bool mergeMode;

	const EnphTextVec *et;	// ref. pointer

	bool fHit;	// ヒットフラグ

	bool fCheckHit;

	bool fOverLeft;
	int *ccPoint;		// 反転部分X座標配列
	int ccPointNum;			// 有効なccPoint[]要素の数

	struct LocList *ccLoc;
	int ccLocSize;			// the number of the ccLoc elements.
	int ccLocIndex;			// the current index of the ccLoc elements.

	// for DrawReverse //
	int unredraw_start;

	// for DrawEnph //
	int under_start;	// アンダーライン開始座標位置
	int area_start;	// area開始X座標位置
	RECT *area_rect;
public:
	EnphTextWork(const EnphTextVec *_et);
	~EnphTextWork()
	{
		if ( ccLoc ){
			delete[] ccLoc;
		}
		if ( ccPoint ){
			delete[] ccPoint;
		}
	}
	bool MergeMode() const { return mergeMode; }
	bool CheckHit() const { return fCheckHit; }
	bool CheckHit2() const { return fCheckHit && ccPointNum; }
	bool Hit() const { return fHit; }
	void SetHit(bool hit) { fHit = hit; }
	bool OverLeft() const { return fOverLeft; }

	void AdjustEnph(class CharHT *cht, int flags, RECT *rc);
	inline bool IsIncluded(int str_loc)
	{
		return ccLocIndex < ccLocSize && ccLoc[ccLocIndex].loc <= str_loc;
	}
	int GetNextLoc();
	void SetPoints(int str_loc, int cx);
	void SetPoint(int cx)
	{
		__assert(ccLocIndex<ccLocSize);
		ccPoint[ ccLoc[ccLocIndex].index ] = cx;
		ccLocIndex++;
		ccPointNum++;
	}
	inline bool CanSetPoint()
	{
		return ccLocIndex<ccLocSize;
	}

	//TODO: 引数を減らしたい
	void DrawReverse(HDC hdc, int right, bool cr, RECT *rc, int top, int revh, int cy);
	//void DrawEnph(TNFONT &tnfont, TDispLineInfo &linfo, int right, RECT *rc, int top, int revh, int cy, const tchar *leftp, const tchar *orgp, INT *tabstop, int n);
	void DrawEnph(TNFONT &tnfont, class TDispLineInfo &linfo, int right, int rc_left, int top, int revh, int cy);
	void Next();
};

// Extra Attributes
#define	TFAX_HIDDEN		0x01
#define	TFAX_HIDDEN_NEXT 0x02	// 次の項目でTFAX_HIDDENになる
//#define	TFAX_BOXOPEN	0x04
//#define	TFAX_HYPLINK	0x08

class TFontAttrEx : public TFontAttr {
typedef TFontAttr super;
public:
	char AttrEx;	// HDCに影響を与えない(SelectFontする必要のない)属性
	class THyperLink *HyperLink;
public:
	TFontAttrEx()
	{
		AttrEx = 0;
		HyperLink = NULL;
	}

	TFontAttrEx &operator = (const TFontAttrEx &o)
	{
		super::operator = (o);
		AttrEx = o.AttrEx;
		HyperLink = o.HyperLink;
		return *this;
	}
	TFontAttrEx &operator = (const TFontAttr &o)
	{
		return (TFontAttrEx&)super::operator = (o);
	}

	bool NeedFontSelect(const TFontAttrEx &o) const
	{
		return super::operator != (o);
	}

	bool operator != (const TFontAttrEx &o) const
	{
		return super::operator != (o) || AttrEx != o.AttrEx;
	}
	void AssignFontTag(const TFontAttrEx &fa)
	{
		super::AssignFontTag(fa);
		AttrEx = fa.AttrEx;
		HyperLink = fa.HyperLink;
	}

	void SetAttrEx(int flg, bool on)
		{ if (on) AttrEx |= flg; else AttrEx &= ~flg; }

	void SetHidden(bool on) { SetAttrEx(TFAX_HIDDEN, on); }
	inline bool IsHidden() const { return AttrEx & TFAX_HIDDEN ? true : false; }

	void SetHiddenNext(bool on) { SetAttrEx(TFAX_HIDDEN_NEXT, on); }
	inline bool IsHiddenNext() const { return AttrEx & TFAX_HIDDEN_NEXT ? true : false; }
	
#if 0
	void SetBoxOpen(bool on) { SetAttrEx(TFAX_BOXOPEN, on); }
	inline bool IsBoxOpen() const { return AttrEx & TFAX_BOXOPEN ? true : false; }
#endif

	void SetHyperLink(THyperLink *hl) { HyperLink = hl; }
	inline bool IsHyperLink() const { return HyperLink != NULL; }
};

class TFontTagStack : public stack<TFontAttrEx> {};

#define	DMT_TERMINAL	0x0001	// 文字列終端
#define	DMT_TAB			0x0002
#define	DMT_CR			0x0004
#define	DMT_ILLEGAL		0x0008	// 不正文字
#define	DMT_OVERLINE	0x0010	// 強制改行
#define	DMT_HTML		0x0020
#define	DMT_HYPLINK1	0x0040	// PDIC hyper link text <> brancket
#define	DMT_HYPLINK2	0x0080	// PDIC hyper link text no brancket
#define	DMT_SPCCHAR		0x0100	// &#..;

// Note:
// OVERLINEとTAB/ILLEGALが一緒になることがある

// display line info
// 表示行情報
class TDispLineInfo {
public:
	int loc;		// the location on the whole text.
	int length;		// The length of the string including tab/cr/tag.
	int tlength;	// The length of the string, but without tab or illchar.
	POINT pos;		// draw開始座標 - pos.xはrc->leftからの相対座標,pos.yはrc->topからの相対座標
	SIZE size;
	COLORREF color;
	COLORREF bgcolor;
	int delimtype;
#ifndef _UNICODE
	bool single;	// single byte code? //TODO: 将来的に半角文字フォント指定ができる
#endif
	TFontAttrEx FontAttr;
	TNFONT *pfont;	// reference pointer to the font object.
	//int flags;
public:
	TDispLineInfo()
	{
		// Should initialize the members after creation.
		//flags = 0;
	}
	TDispLineInfo(int _loc, int _length, int _tlength, int pos_x, int pos_y, int cx, int _delimtype)
		:loc(_loc)
		,length(_length)
		,tlength(_tlength)
		,delimtype(_delimtype)
		,pfont(NULL)
	{
		pos.x = pos_x;
		pos.y = pos_y;
		size.cx = cx;
		//TODO: size.cy = cy;
		//TODO: color = _color;
		//TODO: bgcolor = _bgcolor;
		//flags = 0;
	}
	void set(int _loc, int _length, int _tlength, int pos_x, int pos_y, int cx, int _delimtype)
	{
		loc = _loc;
		length = _length;
		tlength = _tlength;
		pos.x = pos_x;
		pos.y = pos_y;
		size.cx = cx;
		delimtype = _delimtype;
	}
	bool tabchar() const { return delimtype & DMT_TAB ? true:false; }
	bool cr() const { return delimtype & DMT_CR ? true:false; }
	bool illchar() const { return delimtype & DMT_ILLEGAL ? true:false; }
	bool tabillchar() const { return delimtype & (DMT_TAB|DMT_ILLEGAL) ? true:false; }
	bool line_end() const { return delimtype & (DMT_TERMINAL|DMT_OVERLINE|DMT_CR); }
	TDispLineInfo &operator = (const TDispLineInfo&o);

	inline void SetFontAttr(int flag)
		{ FontAttr.SetAttr(flag); }
	inline void ResetFontAttr(int flag)
		{ FontAttr.ResetAttr(flag); }
	inline void SetFontAttr(int flag, bool on)
		{ if (on) SetFontAttr(flag); else ResetFontAttr(flag); }

//	void SetFlags(int flg, bool on)
//		{ if (on) flags |= flg; else flags &= ~flg; }
};

class TDispLinesInfo {
public:
	int MaxRight;		// 最大の表示幅
	int Height;			// 全行の高さ
	int NumLines;		// 行数 （Lines.size()と一致しないかもしれない???）
	int LastRight;		// 最終行のサイズ（＝rc->leftからの相対座標）
	int LastTop;		// 最終行のtop座標
	int LastCYMax;		// 最終行の高さ
public:
	TDispLinesInfo()
	{
		MaxRight = 0;
		Height = 0;
		NumLines = 0;
		LastRight = 0;
		LastTop = 0;
		LastCYMax = 0;
	}
	vector<TDispLineInfo> Lines;	// vector or list?
	vector<int> cyMaxes;
	TFontTagStack FAStack;		// Font attribute stack for <font>
};

typedef map<TFontAttr, TNFONT*> TFontAttrMap_t;	//Note: TFontAttrExではなくTFontAttr。AttrExはfont生成には関係ないため
class TFontAttrMap : public TFontAttrMap_t {
public:
	~TFontAttrMap();
};

class TDrawSetting {
public:
	int TabCol;
protected:
	int RevHeight;
public:
	int DefReverseSize;
	int InitLeft;
	bool WordBreak;		// word breakを有効にする
	bool HtmlParse;
	bool ExpVisible;	// 用例表示ON/OFF
	bool BoxOpen;		// 常にopen

	// Result status //
	int LastRight;		// 最後に表示したテキストの右端座標
	int LastTop;	// 最終行の高さ
	int MaxRight;	// 最大の表示幅(ただし、DF_CALCRECTの場合のみ)
	int CYMax;		// 最終行の最大高さ

public:
	TDrawSetting();
	int GetRevHeight() const { return RevHeight; }
	void SetRevHeight( int ratio );
};


inline int GetTabWidth(int pos_x, int tabwidth)
	{ return tabwidth - ( pos_x - ( pos_x / tabwidth ) * tabwidth ); }

#endif	/* __draw4com_h */

