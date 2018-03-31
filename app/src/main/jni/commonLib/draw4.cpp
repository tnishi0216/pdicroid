// draw4 engine

//Memo:
// hyper link text情報からhittest用のLocList(ccLoc)を作成しているところはどこか？
// draw4com.cpp:EnphTextWork::sEnphSort()

//TODO:
//	Clipboard Copyのテキスト内容(HTML時)はどうするか？
//	巨大テキスト＋１行１文字表示のメモリ最適化
//	EnphTextとTDispLineInfoの統合 - TDispLineInfoをcallerに持たせるときに考える
//
// HTML Tag仕様
//	タグ開始直後の余計な空白は許さない < b>など
//	タグ内の改行はNG
//	サポートされていないtag nameはそのまま表示
//	サポートされているtagのうち、サポートされていないattributeは無視

#include "tnlib.h"
#pragma hdrstop
#include "draw4.h"
#include "CharHT.h"
#include "hyplink.h"

#pragma warn +use

// temporary debug //
#if 0
#define	_DBW	DBW
#define	_dbw	dbw
#else
inline void _DBW(...){}
inline void _dbw(...){}
#endif

// Configuration //
#define	CY_UNDERLINE_OFFSET		2	// underline表示のオフセット。行間(CY_LINEINTERVAL)はこれ以上の値にする必要あり。

#define	foreach(obj, it, type) \
	for (type::iterator it=(obj).begin();it!=(obj).end();it++)

#ifdef _UNICODE
#define	_CharNext(p, single)		CharNext(p)
#define	IF_SingleByte(c, p, nextp)	if ((c>>8)==0)
#else
#define	_CharNext(p, single)		((single) ? (p+1) : CharNext(p))
#define	IF_SingleByte(c, p, nextp)	if (FP_DIFF(nextp, p)==1)
#endif

////////////////////////////////
//	Inlines
////////////////////////////////
inline bool PointRange(int pt, int x1, int x2) { return pt >= x1 && pt < x2; }

////////////////////////////////
//	Prototypes
////////////////////////////////
static void TabbedTextOutEx(TNFONT &font, int x, int y, const tchar *text, int len, int tlen, int tabwidth);

////////////////////////////////
//	Switches
////////////////////////////////

// for old compatibility
TDrawSetting DefDrawSetting;
//int DEFAULT_REVERSE_SIZE = 0;

TDrawSetting &GetDefDrawSetting()
{
	return DefDrawSetting;
}

int GetTabCol()
{
	return DefDrawSetting.TabCol;
}

void SetTabCol( int tabcol )
{
	DefDrawSetting.TabCol = tabcol;
}

int GetDefReverseSize()
{
	return DefDrawSetting.DefReverseSize;
}

void SetDefReverseSize(int size)
{
	DefDrawSetting.DefReverseSize = size;
}

// 強調反転の高さ
// ratio : 0-10(nothing-full)
void SetRevHeight( int ratio )
{
	DefDrawSetting.SetRevHeight(ratio);
}

void SetInitLeft( int initleft )
{
	DefDrawSetting.InitLeft = initleft;
}

#if 0	// GetLastRight() - rc->leftで代用すること
int GetInitLeft()
{
	return DefDrawSetting.InitLeft;
}
#endif

int GetLastRight( )
{
	return DefDrawSetting.LastRight;
}

int GetLastTop()
{
	return DefDrawSetting.LastTop;
}
int GetDrawMaxRight()
{
	return DefDrawSetting.MaxRight;
}
int GetDrawCYMax()
{
	return DefDrawSetting.CYMax;
}

int DrawText2( TNFONT &tnfont, const tchar *str, RECT *rc, int swidth, UINT flags, CharHT *cht, const EnphTextVec *et, THyperLinks *hls )
{
	return DrawText(tnfont, str, rc, swidth, flags, cht, et, hls);
}
// End of compatibility //

// ptを与えると、ptが含まれる文字の位置及び、その文字の矩形を得る
// ヒットした場合は 0x8000をORする
// 戻り値は縦のドット数
// swidth : right側が余った場合にfillするときに使われる。(標準の幅）
// h1 : 通常フォント
// h2 : 英字フォント
// 表示が１行の場合は、rc->rightを変更するので注意！
// rc->bottom はDF_CALCRECTの場合のみ出力として使用するのみ
// 強調表示関連 //
// DF_UNREDRAWがあるとフルハイトの反転
// ないと、アンダーライン
// CharHTによる反転と、EnphTextによる反転は同時に表示出来ない！！
int DrawText( TNFONT &tnfont, const tchar *str, RECT *rc, int swidth, UINT flags, CharHT *cht, const EnphTextVec *et, THyperLinks *hls )
{
	return DrawText( tnfont, str, rc, swidth, flags, cht, et, false, hls );
}
// DrawText with Update CP
int DrawTextUCP( TNFONT &tnfont, const tchar *str, RECT *rc, int swidth, UINT flags, CharHT *cht, const EnphTextVec *et )
{
	return DrawText( tnfont, str, rc, swidth, flags, cht, et, true );
}

int DrawText( TNFONT &tnfont, const tchar *str, RECT *rc, int swidth, UINT flags, CharHT *cht, const EnphTextVec *et, bool updatecp, THyperLinks *hls )
{
#ifdef _DEBUG
	static bool in = false;
	__assert(!in);
	in = true;
#endif
	TDispLinesInfo linesinfo;
	TFontAttrMap FontAttrs;
	TDrawSetting &setting = DefDrawSetting;
//	setting.DefReverseSize = DEFAULT_REVERSE_SIZE;
#if 1	//TODO: 呼出し側で行うようにする
	tnfont.FontAttr.Color = GetTextColor(tnfont);
	tnfont.FontAttr.BgColor = GetBkColor(tnfont);
#endif
	ParseText(tnfont, linesinfo, str, rc, setting, FontAttrs, hls);
	if (flags&DF_CALCRECT){
		HitTest(tnfont, linesinfo, str, rc, flags, cht, et, setting);
	} else {
		DrawText(tnfont, linesinfo, str, rc, swidth, flags, cht, et, setting);
	}

	// for compatibility
	setting.MaxRight = linesinfo.MaxRight;
	setting.LastRight = rc->left+linesinfo.LastRight;
	setting.LastTop = linesinfo.LastTop;
	setting.CYMax = linesinfo.LastCYMax;

	// updatecp=trueだとrc->bottom = linesinfo.LastTop + linesinfo.Heightとなり、おかしくなる
	// なので、updatecp処理の前に移動した（副作用はないはずだが) 2007.11.14
	if ( flags & DF_CALCRECT ){
		rc->bottom = rc->top + linesinfo.Height;
	}

	if (updatecp){
		// 座標を更新する(UPDATECPと同じ)→変更したので正しく動作するか不明 2007.10.2
		setting.InitLeft = linesinfo.LastRight;
		if ( linesinfo.NumLines > 1 ){
			rc->top = linesinfo.LastTop;
		}
	} else {
		setting.InitLeft = 0;
		// １行未満の場合は、rightを補正する！！(DrawTextには無い仕様！）
		if ( linesinfo.NumLines <= 1 ){
			rc->right = setting.LastRight;
		}
	}

#ifdef _DEBUG
	in = false;
#endif

	return linesinfo.Height;
}

void HitTest( TNFONT &tnfont, TDispLinesInfo &linesinfo, const tchar *str, RECT *rc, UINT flags, CharHT *cht, const EnphTextVec *et, TDrawSetting &setting)
{
	__assert(flags&DF_CALCRECT);
	// 強調文字列表示用
	// etwを再構築しないで利用できないか？
	EnphTextWork etw(et);
	etw.AdjustEnph(cht, flags, rc);

	int lines = 0;	// 行数

#ifndef _UNICODE
	bool fUseAsc = false;
	bool fCurAsc = false;
	if ( tnfont.hAscFont ){
		fUseAsc = true;
	}
#endif

#ifndef _UNICODE
	bool fSingleByte = tnfont.fSingleByte;
#endif

	bool hity = false;

	const TFontAttrEx *faPrev = NULL;
	foreach(linesinfo.Lines, it, vector<TDispLineInfo>)
	//for(vector<TDispLineInfo>::reverse_iterator it = linesinfo.Lines.rbegin();it!=linesinfo.Lines.rend();it++)
	{
		TDispLineInfo &linfo = *it;
		TNFONT &font = *linfo.pfont;

		// Change font if attributes changed.
		if (faPrev){
			if (linfo.FontAttr.NeedFontSelect(*faPrev)){
				font.Select();
			}
		} else {
			// first time
			font.Select();
		}
		faPrev = &linfo.FontAttr;

		const tchar *p = str + linfo.loc;
//		const tchar *leftp = p;	// 行の先頭文字列

#ifndef _UNICODE
		if (linfo.single){
			if ( fUseAsc && !fCurAsc ){
				font.SelectAsc();
			}
		} else {
			if ( fUseAsc && fCurAsc ){
				font.Select();
			}
		}
#endif

		int top = linfo.pos.y;	// 現在行の頭の座標
		int left = linfo.pos.x;
		int right = linfo.pos.x+linfo.size.cx;	// 実際に表示される文字列の右端(+1ドット)座標(ただし、rc->left分は含んでいない）
		//dbw("hittest: top=%d left=%d right=%d", top, left, right);

		// ヒットテスト
		if ( cht && !etw.Hit() ){
			// ｙ座標判定
			POINT pt = cht->pt;
			int cy = linesinfo.cyMaxes[lines];
			if ( PointRange(pt.y, top, top+cy) ){
				if ( etw.OverLeft() && ( lines == 0 || pt.x < rc->left ) ){
					// 左方オーバの場合
					if ( cht->pos1 == -1 ){
						// 先に現れた方を優先
						cht->pos1 = linfo.loc;
						cht->item1 = cht->curitem;
					}
					etw.SetHit(true);
					return;
				} else
				// 右方チェック
				if ( linfo.line_end() && pt.x >= rc->left + right ){
//			   		if ( cht->item1 == cht->curitem ){
						cht->fOver = CHT_RIGHT;
						cht->pos2 = linfo.loc + linfo.length;
						cht->item2 = cht->curitem;
//			   		}
					etw.SetHit(true);
					return;
				} else {
					// x座標判定
					if (PointRange(pt.x, rc->left+left, rc->left + right)){
						// ヒットした！
						// ヒット文字の算出
						int hitloc = HitTestX(font, linfo, str, *rc, flags, *cht, setting);
						//DBW("hitloc=%d", hitloc);
						if (hitloc>=0){
							cht->pos = hitloc;
							cht->item = cht->curitem;
							etw.SetHit(true);
							return;
							//FAQ: hitlocしているのにhitしない(cursorがHANDにならない等)
							// →HyperLink.areaのrightがleftより小さい値になっている(Squre::HitTestHyperLink()で確認)
							// ←DrawEnph()でstartがあってもendが呼ばれない
							//   ↑parserでの間違いの可能性が高い
						}
					}
				}
				hity = true;
			}
		}

		if (linfo.line_end()){
			if (hity)
				break;	// no more hit area
			lines++;
		}
		if ( !*p ){
			break;	// 最終行の場合は終了
		}
	}

	if ( cht ){
		// 下方チェック
		if ( cht->pt.y >= rc->bottom ){
//			if ( cht->item != -1 || cht->item == cht->curitem ){
				// item指定の場合は一致するときのみ
				//cht->pos2 = STR_DIFF( p, str );
				cht->pos2 = _tcslen(str);
				cht->item2 = cht->curitem;
				cht->fOver = CHT_UNDER;
//			}
			etw.SetHit(true);
		}
	}
}

#if 0	// old way
// １つのLineInfoのみの解析 (hit test用)
// parser for one line info.
// str : top of the text.
#else
// point to loc
// cht.pt.xから文字位置を求める
int HitTestX(TNFONT &tnfont, TDispLineInfo &linfo, const tchar *str, const RECT &rc, UINT flags, CharHT &cht, const TDrawSetting &setting)
{
	__assert(cht.pt.x>=rc.left);

	HDC hdc = tnfont;

	int pt_x = cht.pt.x-rc.left;
	const tchar *leftp = str + linfo.loc;	// 左端の文字列pointer
	int left = linfo.pos.x;

	ushort c;
	
	int nFit;
	SIZE sz;
	if (!GetTextExtentExPoint(hdc, leftp, linfo.tlength, pt_x-left, &nFit, NULL, &sz)){
		__assert__;
		return -1;	// undefined error.
	}

	const tchar *p;	// working pointer.

	if (nFit==linfo.tlength && pt_x>left+sz.cx){
		// over the right
		if (!(linfo.delimtype & (DMT_TAB|DMT_ILLEGAL))){
			// no more text.
			return -1;
		}
		// check the tab/illegal chars.
		// 別関数にしたいところ。。
		const tchar *p = leftp+linfo.tlength;
		const tchar *endp = leftp+linfo.length;
		int w;
		for (;p<endp;){
			tuchar c = *p;
			const tchar *nextp = _CharNext(p, tnfont.fSingleByte);
			__assert(c<' ');
			switch (c){
				case '\t':
					w = GetTabWidth(left+sz.cx, tnfont.GetSpcWidth() * setting.TabCol);
					break;
				case '\r':
				case '\n':
				case '\0':
					return -1;
				default:	// illchar
#if ILLCHAR_AS_PERIOD
					w = tnfont.GetIllWidth();
#else
					w = 0;
#endif
					break;
			}
			if (PointRange(pt_x, left+sz.cx, left+sz.cx+w)){
				if (flags & DF_WHOLECHAR){
					return STR_DIFF( p, str );
				} else {
					if ( pt_x >= left+sz.cx + (w>>1) )
						return STR_DIFF( nextp, str );	// 文字の右半分
					else
						return STR_DIFF( p, str );		// 文字の左半分
				}
			}
			p = nextp;
			sz.cx += w;
		}
	}

	//TODO: szはnFitまでではなく、linfo.tlength全体を計算した値らしい・・・なんともアホな仕様だが
	// 自分のアルゴリズムがおかしいのか？？？
	if (!GetTextExtentExPoint(hdc, leftp, nFit, pt_x-left, &nFit, NULL, &sz)){
		__assert__;
		return -1;	// I belive never come.
	}

	p = leftp + nFit;

	if ( flags & DF_WHOLECHAR ){
		return STR_DIFF( p, str );
	} else {
		int w;
		c = *(tuchar*)p;
		if ( !GetCharWidth32( hdc, c, c, &w) )
			GetCharWidth( hdc, c, c, &w );

		if ( pt_x >= left + sz.cx + (w>>1) )
			return STR_DIFF( _CharNext(p, fSingleByte), str );	// 文字の右半分
		else
			return STR_DIFF( p, str );		// 文字の左半分
	}
}
#endif

#if 0
// １つのLineInfoのみの解析 (hit test用)
// parser for one line info.
// str : top of the text.
#else
// loc to point
// linfoにある文字位置情報を、etw.ccLoc[cur以降]から探し、該当するものが見つかったらそのX座標をetw.ccPoint[]へ設定する
//Memo: この関数はほとんどEnphTextWorkのメンバー関数っぽいが。。
void HitTestX(TNFONT &tnfont, TDispLineInfo &linfo, const tchar *str, const RECT &rc, UINT flags, EnphTextWork &etw, const TDrawSetting &setting)
{
	HDC hdc = tnfont;

	const tchar *leftp = str + linfo.loc;	// 左端の文字列pointer
	int left = linfo.pos.x;

	SIZE sz;
	int loc;

	for (;;){
		loc = etw.GetNextLoc();
		if (loc==-1)
			return;
		_DBW("HTX: loc=%d linfo.loc=%d length=%d tlength=%d", loc, linfo.loc, linfo.length, linfo.tlength);
		if (loc>=linfo.loc+linfo.tlength){
			// locはlinfoテキストより後ろ
			//if (etw.MergeMode())
				break;
			//continue;
		}

		if (loc<linfo.loc){
			// 前回の行末だった(etw.DrawReverse()でresetされている）
			etw.SetPoint(left);
			_dbw("SetPoint1: %d", left);
			continue;
		}
			
		// Get the extent of the text in loc.
		if (!GetTextExtentExPoint(hdc, leftp, loc-linfo.loc, INT_MAX, NULL, NULL, &sz)){
			__assert__;
			DBW("%s:%d %d", __FUNC__, __LINE__, GetLastError());
			return;	// undefined error.
		}
		etw.SetPoint(left+sz.cx);
		_dbw("SetPoint2: %d", left+sz.cx);
	}

	if (!(linfo.delimtype & (DMT_TAB|DMT_ILLEGAL|DMT_CR|DMT_TERMINAL))){
		//DBW("no more text");
		// no more text.
		return;
	}

	// check the tab/illegal chars.
	if (!GetTextExtentExPoint(hdc, leftp, linfo.tlength, INT_MAX, NULL, NULL, &sz)){
		__assert__;
		return;	// undefined error.
	}

	// loc is valid value until here.
	
	// 別関数にしたいところ。。
	const tchar *p = leftp+linfo.tlength;
	const tchar *endp = leftp+linfo.length;
	while (p<=endp){
		int w;
		tuchar c = *p;
		const tchar *nextp = _CharNext(p, tnfont.fSingleByte);
		//IF_SingleByte(c, p, nextp)
		if (c<' ')	// 2010.4.1 fixed.
		{
			switch (c){
				case '\t':
					w = GetTabWidth(left+sz.cx, tnfont.GetSpcWidth() * setting.TabCol);
					break;
				case '\r':
				case '\n':
				case '\0':
					w = 0;
					break;
				default:
					// illchar
#if ILLCHAR_AS_PERIOD
					w = tnfont.GetIllWidth();
#else
					w = 0;
#endif
					break;
			}
		} else {
			// normal char
			w = 0;
		}

		if (loc<=STR_DIFF(p,str)){
			if (p==endp && linfo.tlength<linfo.length && (p[-1]!='\r'||p[-1]!='\n')){
				// 2010.4.2 改行の終端であった場合無視する。なぜなら
				// <file:abc.txt>\r\n
				// <file:abcdef.txt>
				// とあった場合、p=endpが２行目の先頭を指す。
				// そのため、誤ったpoint情報をセットしてしまう。
				//Note:
				// ここで解決するのではなく、根本的にロジックを考え直す必要がある
				break;
			}
			
			etw.SetPoint(left+sz.cx);
			_dbw("SetPoint3: %d", left+sz.cx);

			loc = etw.GetNextLoc();
			if (loc==-1)
				break;
		}

		if (c=='\0')
			break;

		sz.cx += w;
		p = nextp;
	}
}
#endif

inline int GetRevH(int cy, int flags, TDrawSetting &setting)
{
//	cy -= CY_UNDERLINE_OFFSET;
	return (flags & DF_UNREDRAW ?
			( setting.DefReverseSize == 0 ? 0 : cy-setting.DefReverseSize-CY_UNDERLINE_OFFSET) : cy*setting.GetRevHeight()/16);
}

void DrawText( TNFONT &org_tnfont, TDispLinesInfo &linesinfo, const tchar *str, RECT *rc, int swidth, UINT flags, CharHT *cht, const EnphTextVec *et, TDrawSetting &setting)
{
	__assert(!(flags&DF_CALCRECT));
	// 強調文字列表示用
	EnphTextWork etw(et);
	etw.AdjustEnph(cht, flags, rc);

	HDC _hdc = org_tnfont;

	int lines = 0;	// 行数

	// OPAQUE用ブラシ
	HBRUSH hbr = NULL;
	if ( flags & DF_OPAQUE ){
		hbr = CreateSolidBrush( GetBkColor(_hdc) );
	}

#ifndef _UNICODE
	bool fUseAsc = false;
	bool fCurAsc = false;
	if ( tnfont.hAscFont ){
		fUseAsc = true;
	}
#endif

#ifndef _UNICODE
	bool fSingleByte = tnfont.fSingleByte;
#endif

	const TFontAttrEx *faPrev = NULL;
	int last_right = 0;
	foreach(linesinfo.Lines, it, vector<TDispLineInfo>){
		TDispLineInfo &linfo = *it;
		TNFONT &font = *linfo.pfont;

		// Change font if attributes changed.
		if (faPrev){
			if (linfo.FontAttr.NeedFontSelect(*faPrev)){
				font.SelectEx(flags&DF_REVERSE?true:false);
			}
		} else {
			// first time
			font.SelectEx(flags&DF_REVERSE?true:false);
		}
		faPrev = &linfo.FontAttr;

		const tchar *leftp = str + linfo.loc;
		int tlength = linfo.tlength;

		bool altmode = false;
		if (linfo.FontAttr.HyperLink){
			const THyperLink &hl = *linfo.FontAttr.HyperLink;
			if (!hl.state){
				altmode = true;
				//tlength = hl.length;
				leftp = hl.key + linfo.loc - hl.loc;
				//endp = leftp + tlength;
				//dbw("altmode: %ws len=%d", hl.key.c_str(), tlength);
			}
		}
		tchar cc[2];
		if (linfo.delimtype & DMT_SPCCHAR){
			altmode = true;
			tlength = 1;
			cc[0] = CodeToChar(leftp+2, NULL);
			cc[1] = '\0';
			leftp = cc;
		}

#ifndef _UNICODE
		if (linfo.single){
			if ( fUseAsc && !fCurAsc ){
				tnfont.SelectAscEx(flags&DF_REVERSE?true:false);
			}
		} else {
			if ( fUseAsc && fCurAsc ){
				tnfont.SelectEx(flags&DF_REVERSE?true:false);
			}
		}
#endif

		const int top = linfo.pos.y;	// 現在行の頭の座標
		const int left = linfo.pos.x;
		const int right = left+linfo.size.cx;	// 実際に表示される文字列の右端(+1ドット)座標(ただし、rc->left分は含んでいない）

		//if (setting.ExpVisible || !linfo.FontAttr.IsExp())
		{	// 非表示時の用例でなければ描画

			if ( etw.CheckHit() ){
				// 強調表示範囲のチェック
				if (etw.IsIncluded(linfo.loc+linfo.length)){	// etw.ccLoc[cur].loc が 現在のlinfoテキストの右端より左側か？
					HitTestX(font, linfo, str, *rc, flags, etw, setting);
					last_right = right;
				}
			}

			if ( (flags & DF_UNREDRAW) ){
				// 反転表示処理
				// start,endがダブることがないと仮定して処理している
				if (linfo.line_end()){
					const int cy = linesinfo.cyMaxes[lines];
					const int revh = GetRevH(cy, flags, setting);
					etw.DrawReverse(font, right, linfo.cr, rc, top, revh, cy-CY_UNDERLINE_OFFSET);
				}
			} else {	// !(flags & DF_UNREDRAW)
				//dbw("%d: %ws", tlength, leftp);
				int y = top+linesinfo.cyMaxes[lines] - font.cy - CY_UNDERLINE_OFFSET;
				if (linfo.tabillchar()){
					TabbedTextOutEx(font, rc->left+linfo.pos.x, y, leftp, linfo.length, tlength, font.GetSpcWidth() * setting.TabCol);
				} else {
					ExtTextOut( font, rc->left+left, y, 0, NULL, leftp, tlength, NULL );
				}
				if ( etw.CheckHit2() ){
					if (linfo.line_end()){
						// 強調表示処理
						const int cy = linesinfo.cyMaxes[lines];
						const int revh = GetRevH(cy, flags, setting);
						etw.DrawEnph(font, linfo, right, rc->left, top, revh, cy);
					}
				}
			}
		}

		if (linfo.line_end()){
			etw.Next();

			// 残りの空白
			if ( hbr && right < swidth ){
				RECT _rc;
				SetRect( &_rc, rc->left + right, top, rc->left+swidth, top+linesinfo.cyMaxes[lines] );
				FillRect( font, &_rc, hbr );
			}

			lines++;
		}
		if ( !altmode && !*leftp ){
			break;	// 最終行の場合は終了
		}
		if ( top > rc->bottom ){
			break;	// 表示範囲を超えた
		}
	}

	if ( etw.CheckHit() ){
		// まだ残りがある（行末まで強調表示がある場合）
		// 2008.5.19
		//int loc = etw.GetNextLoc();
		//if (loc!=-1)
		{
			if (/* et && et->size()>0 && */	// 強調表示に限定
				(linesinfo.cyMaxes.size()>0)){	// 2008.5.27 条件追加(linesinfo.cyMaxes[linesinfo.cyMaxes.size()-1]で例外が発生する場合があるため（あまり深く追求していない）
				if (etw.CanSetPoint()){
					int loc;
					do {
						etw.SetPoint(last_right);
						loc = etw.GetNextLoc();
					} while (loc!=-1);
				}
				__assert(linesinfo.Lines.size()>0);
				const int cy = linesinfo.cyMaxes[linesinfo.cyMaxes.size()-1];
				const int revh = GetRevH(cy, flags, setting);
				TDispLineInfo &linfo = linesinfo.Lines[linesinfo.Lines.size()-1];
				TNFONT &font = *linfo.pfont;
				const int top = linfo.pos.y;
				if ( (flags & DF_UNREDRAW) ){
					// 反転表示処理
					// start,endがダブることがないと仮定して処理している
					etw.DrawReverse(font, last_right, linfo.cr, rc, top, revh, cy-CY_UNDERLINE_OFFSET);
				} else {
					etw.DrawEnph(font, linfo, last_right, rc->left, top, revh, cy);
				}
			}
		}
	}

	// 表示文字列がないとき、Opaqueされないため
	if ( hbr && lines == 0 ){
		RECT _rc;
		SetRect( &_rc, rc->right, rc->top, rc->left + swidth, rc->top+linesinfo.cyMaxes[lines] );
		FillRect( org_tnfont, &_rc, hbr );
	}

	//TODO: recover font.
//	org_font.SelectEx();
	
	if ( hbr )
		DeleteObject( hbr );
}

// textの最後尾にtab or illcharがある前提（ただし、最後に改行は可能＝end markとして扱う）
// tlen - 通常表示可能なcharacter length
// len - tab/illchar含めたlength of the arg text.
static void TabbedTextOutEx(TNFONT &font, int x, int y, const tchar *text, int len, int tlen, int tabwidth)
{
	__assert(len>=tlen);
	const tchar *endp = text + tlen;
	if (len!=tlen){
		ExtTextOut(font, x, y, 0, NULL, text, STR_DIFF(endp,text), NULL);
#if ILLCHAR_AS_PERIOD
		SIZE sz;
		if (!GetTextExtentExPoint(font, text, tlen, INT_MAX, NULL, NULL, &sz)){
			__assert__;
		}
		x += sz.cx;
		text = endp;
		endp += (len-tlen);
#endif
	}
#if ILLCHAR_AS_PERIOD
	while (text<endp){
		tuchar c = *text++;
		__assert(c<' ');
		if (c=='\t'){
			x += GetTabWidth(x, tabwidth);
		} else
		if (c=='\r' || c=='\n'){
			break;
		} else {
			ExtTextOut(font, x, y, 0, NULL, _T("."), 1, NULL);
			x += font.IllWidth;
		}
	}
#endif
	//TODO: Not opaque, right?
}

/*
Test Senario
- Tabで始まるテキスト
- 行末がTabで終わる
- １行で表示しきれない単語
- １行に複数の強調表示
*/


