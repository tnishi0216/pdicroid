//---------------------------------------------------------------------------

#include "tnlib.h"
#pragma hdrstop
#include "draw4def.h"
#include "draw4.h"
#include "draw4parser.h"
#include "draw4com.h"
#include "pdstrlib.h"
#include "hyplink.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

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
#define	CY_LINEINTERVAL			CY_DRAW_LINEINTERVAL	// 行間

#ifdef _UNICODE
#define	_CharNext(p, single)		CharNext(p)
#define	IF_SingleByte(c, p, nextp)	if ((c>>8)==0)
#define	__arg_asc(name)
#else
#define	_CharNext(p, single)		((single) ? (p+1) : CharNext(p))
#define	IF_SingleByte(c, p, nextp)	if (FP_DIFF(nextp, p)==1)
#define	__arg_asc(name)					, name
#endif

struct AttrInfo {
	int loc;	// location of value
	int length;	// length of value
	tnstr value;
};
typedef map<tnstr, AttrInfo> AttrInfoMap;
////////////////////////////////
//	Prototypes
////////////////////////////////
COLORREF ParseColor(const tchar *strColor);
static const tchar *FindDelimiter(const tchar *leftp, int &delimtype, vector<int> &wordindex, TDrawSetting &setting, TFontAttrEx *fa, const tchar **tagtop, TFontTagStack &faStack, THyperLinks *hls __arg_asc(bool fSingleByte));
static const tchar *FindBreakPoint(vector<int> &wordindex, int wordindex_offs, const tchar *leftp, int end);

const tchar *ParseHtml(const tchar *tagp, TDrawSetting &setting, TFontAttrEx &fa, TFontTagStack &faStack, THyperLinks *hls);
const tchar *ParseHtmlTag(const tchar *text, const tchar **name_end, AttrInfoMap *attrs);

const tchar *ParsePdicHyperLink(const tchar *tagp, int &type);
const tchar *ParsePdicHyperLink2(const tchar *text, const tchar *top);

const tchar *StrKinsokuZ = _T("、。，．？！）］｝＞゜゛」〕〉》』】’；：％ぁぃぅぇぉゃゅょっァィゥェォャュョッ");
const tchar *StrKinsokuK = _T("､｡ｧｨｩｪｫｬｭｮｯﾟﾞ｣");
const tchar *StrKinsokuH = _T(",.?!)]}>';:%");

COLORREF rgbLinkColor = RGB(0x00, 0x00, 0xFF);

bool IsKinsokuA( ushort c )
{
	const tchar *p = StrKinsokuH;
	while ( *p ){
		if ( (tuchar)*p == (tuchar)c )
			return true;
		p++;
	}
	return false;
}

bool IsKinsokuJ( ushort c )
{
	const tchar *p;
#ifdef _UNICODE
	p = StrKinsokuZ;
	while ( *p ){
		if ( *p == c )
			return true;
		p++;
	}
	p = StrKinsokuK;
	while ( *p ){
		if ( *p == c )
			return true;
		p++;
	}
#else
	if ( c > 0xff ){
		// 漢字
		p = StrKinsokuZ;
		uchar c1 = (uchar)(c>>8);
		while ( *p ){
			if ( (uchar)*p == c1 && (uchar)*(p+1) == (uchar)c ){
				return true;
			}
			p += 2;
		}
	} else {
		p = StrKinsokuK;
		while ( *p ){
			if ( (uchar)*p == (uchar)c )
				return true;
			p++;
		}
		return IsKinsokuA( c );
	}
#endif
	return false;
}

// Helper function.
// FontAttrsにあればそれを利用、なければ新規作成
static void AssignFont(HDC hdc, TDispLineInfo &info, TFontAttrMap &FontAttrs, TFontAttrEx &faNext)
{
	if (FontAttrs.count(faNext)){
		info.pfont = FontAttrs[faNext];
	} else {
		info.pfont = FontAttrs[faNext] = new TNFONT(hdc, faNext, 0);
	}
	const bool sel = info.FontAttr.NeedFontSelect(faNext);
	info.FontAttr = faNext;
	if (sel)
		info.pfont->Select();
}
void ParseHtmlHitPosition( TNFONT &tnfont, const tchar *str, RECT *rc, THyperLinks &hls )
{
	TDispLinesInfo linesinfo;
	TFontAttrMap FontAttrs;
	TDrawSetting &setting = GetDefDrawSetting();
#if 0
	ParseText(tnfont, linesinfo, str, rc, setting, FontAttrs, &hls);
#else
	//TODO:
	//Note:
	// mouse clickの優先順位から一つ処理が必要となる。
	// <bx>タグは出現順でhlsへ積まれていくが、pdic hyperlinkは予めhlsに入っているため、出現順にならない。
	// そのため、<bx>タグ内にpdic hyperlinkに入っているとmouse clickを補足できない。
	// 最も正しいのはpdic hyperlinkを予め処理せず、ParseText()内で行うべき。（ExtractStaticWordsの統合)
	// その変更の影響範囲が読めないため、暫定処理。
	// ただ、出現順に積むのが正しいかどうかはっきりしないため、それが変わればここの記述は正しくない。
	// mouse click判断はSqure::HitTestHyperLink()にある。
	int oldnum = hls.size();
	ParseText(tnfont, linesinfo, str, rc, setting, FontAttrs, &hls);
	// 先頭のoldnum個を後ろへ移動
	for (int i=0;i<oldnum;i++){
		hls.move(0, hls.size()-1);
	}
#endif
}
void ParseText(TNFONT &org_tnfont, TDispLinesInfo &linesinfo, const tchar *str, RECT *rc, TDrawSetting &setting, TFontAttrMap &FontAttrs, THyperLinks *hls)
{
	int maxcx = rc->right - rc->left;	// １行の最大ドット数
	if (maxcx<=0){
		return;		//TODO: どうすべきか？（すぐreturnしてもいい？）
	}
	int cx = setting.InitLeft;	// １行のｘドット数
	int lines = 0;	// 行数
	bool added = false;	// 改行無しで追加flag
	const tchar *leftp = str;	// 行の先頭文字列
	int top = rc->top;	// 現在行の頭の座標

#ifndef _UNICODE
	bool fUseAsc = false;
	bool fCurAsc = false;
	if ( tnfont.hAscFont ){
		fUseAsc = true;
	}
#endif

	TNFONT &cur_font = org_tnfont;
	
#ifndef _UNICODE
	bool fSingleByte = tnfont.fSingleByte;
#endif

	TDispLineInfo info;
	info.pfont = &org_tnfont;
	info.FontAttr = org_tnfont.FontAttr;
	info.FontAttr.Color = GetTextColor(org_tnfont);
	TFontAttrEx faNext(info.FontAttr);
	int cyMax = info.pfont->cy;
	int delimtype = 0;

	if (cx>=maxcx){
		// すでに行をオーバーしていた場合
		cx = 0;
		top += cyMax+CY_LINEINTERVAL;	//TODO:[A] その行の最大高さを引き継ぐ必要あり！！nInitLeft!=0の場合はすべて。
	}

	if (hls){
		hls->top = str;
		hls->tag = NULL;
	}
	
	while (*leftp){
		vector<int> wordindex;
		const tchar *nextp;	// next pointer (skipped the HTML tags)
		const tchar *endp = NULL;	// end of text to be displayed.
		COLORREF color_saved = -1;
		int delimtype_saved;	// hyperlinkの開始が強制改行位置であった場合の対策（もっといい方法があれば）
		if (setting.HtmlParse){
			if (delimtype & (DMT_HYPLINK1|DMT_HYPLINK2)){
				// Special state for PDIC hyper link.
				// Already parsed "<xxx:" part for "<xxx:yyy>" format.
				// Continue to parse the remaining part "yyy>" for change the text attribute.
				color_saved = faNext.Color;
				faNext.SetColor(org_tnfont.LinkColor);
				AssignFont(cur_font, info, FontAttrs, faNext);
				if (delimtype & DMT_HYPLINK1){
					endp = nextp = _tcschr(leftp, '>');
				} else {
					// delimtype & DMT_HYPLINK2
					nextp = leftp;
					for (;;){
						if (IsHypLinkTerm(*nextp)){
							break;
						}
						nextp = _CharNext(nextp, fSingleByte);
					}
					endp = nextp;
				}
				__assert(nextp);
				faNext.SetColor(color_saved);
				delimtype_saved = delimtype;
				delimtype = 0;
				if (leftp==nextp)
					continue;
			} else {
				if (faNext!=info.FontAttr){
					AssignFont(cur_font, info, FontAttrs, faNext);
				}
				nextp = FindDelimiter(leftp, delimtype, wordindex, setting, &faNext, &endp, linesinfo.FAStack, hls __arg_asc(fSingleByte));
				if (!endp)	//Q: なんでここだけこの条件が必要なんだ？
					endp = nextp;
			}
		} else {
			endp = nextp = FindDelimiter(leftp, delimtype, wordindex, setting, NULL, NULL, linesinfo.FAStack, hls __arg_asc(fSingleByte));
		}
		int textloc = STR_DIFF(leftp,str);
		int textlen = STR_DIFF(endp,leftp);
		int wordindex_offs = 0;
		const tchar *saved_leftp = leftp;

		bool altmode = false;
		tchar cc[2];
		_DBW("%ws: %x %x %08X %x", leftp, info.FontAttr.AttrEx, faNext.AttrEx, faNext.HyperLink, delimtype);
		if (info.FontAttr.IsHidden()){
			altmode = true;
			textlen = 0;
			leftp = _t("");
			endp = leftp;
			if (faNext.IsHiddenNext()){
				faNext.SetHidden(true);
				//faNext.SetHiddenNext(false);
				// 非表示領域の属性はキャンセル
				delimtype &= DMT_TERMINAL|DMT_CR|DMT_OVERLINE;
			}
		} else {
			if (info.FontAttr.HyperLink){
				const THyperLink &hl = *info.FontAttr.HyperLink;
				if (!hl.state){
					// bx tag, closed
					altmode = true;
					textloc = hl.loc;	// original text上でもlocation
					textlen = hl.length;
					leftp = hl.key;
					endp = leftp + textlen;
					if (faNext.IsHiddenNext()){
						faNext.SetHidden(true);
						//faNext.SetHiddenNext(false);
						// 非表示領域の属性はキャンセル
						delimtype = 0;
					}
					//DBW("altmode: %ws len=%d %08X", hl.key.c_str(), textlen, &hl);
				}
				//info.FontAttr.SetHyperLink(NULL);
				//faNext.SetHyperLink(NULL);
			}
			if (delimtype & DMT_SPCCHAR){
				// special character
				altmode = true;
				const int cclen= 1;
				textlen = cclen;
				cc[0] = CodeToChar(leftp+2, NULL);
				cc[1] = '\0';
				leftp = cc;
				endp = leftp + cclen;
			}
		}

		const tchar *saved_endp = endp;
		for (;;){
			cyMax = max(cyMax, info.pfont->cy);
			int nFit;
			int sz_cx;
			int left = cx;
			bool overline = false;
			if (leftp==endp /*|| (!setting.ExpVisible && info.FontAttr.IsExp())*/){
				// no text to be displayed.
				sz_cx = 0;
				nFit = 0;
			} else {
				SIZE sz;
				if (delimtype&(DMT_TAB|DMT_ILLEGAL)){
					int w;
					switch (delimtype){
						case '\t':
							w = GetTabWidth(cx, cur_font.GetSpcWidth() * setting.TabCol);
							break;
						default:	// illchar
#if ILLCHAR_AS_PERIOD
							w = cur_font.GetIllWidth();
#else
							w = 0;
#endif
							break;
					}
					nFit = textlen;
					sz_cx = w;
				} else {
					if (!GetTextExtentExPoint(cur_font, leftp, textlen, maxcx-left, &nFit, NULL, &sz)){
						// cannot display? - DrawTextに再入した可能性が高い
						DBW("textlen=%d nMaxExtent=%d LastErr=%d", textlen, maxcx-left, GetLastError());
						__assert_debug;
						leftp = nextp;	// to the next text
						continue;
					}
					sz_cx = sz.cx;
				}
				if (nFit==0){
					// １文字も表示できる余裕がない
					if (left==0){
						// 行の左端→表示しない
						__assert(leftp!=endp);
						leftp = nextp;
						break;
					}
					// 強制改行させる
					//faNext = info.FontAttr;	// text直後に</font>などのtagがあると次回loop時に反映されてしまうため
												// 2010.11.25 ↑意味がわからない。
												// 行末にtext1<font ...>text2</font>\r\ntext3...というテキストがあった場合、
												// text2が行頭に来るようにwindow sizeを調整するとtext3が<font ...>で指定した
												// 属性で表示されてしまうため、この意味のわからない処理は外した。
					if (color_saved!=(COLORREF)-1){
						faNext.SetColor(color_saved);
						delimtype = delimtype_saved;
					}
					if (linesinfo.Lines.size()>0){
						linesinfo.Lines[linesinfo.Lines.size()-1].delimtype |= DMT_OVERLINE;	//TODO: 強引過ぎるか？
					}
					goto joverline;
				}
				if (nFit!=textlen){
					overline = true;
					if (setting.WordBreak){
						endp = FindBreakPoint(wordindex, wordindex_offs, leftp, nFit);
						if (endp<=leftp){
							// Only one word -> break the word.
							endp = leftp+nFit;
						} else {
							nFit = STR_DIFF(endp, leftp);
							//TODO: 初回のExtentで取得したpoint情報を使うのが良いのか、現状の方法が良いのか？
							GetTextExtentExPoint(cur_font, leftp, nFit, maxcx-left, NULL, NULL, &sz);
							sz_cx = sz.cx;
						}
						wordindex_offs += nFit;
					} else {
						// break the word.
						endp = leftp+nFit;
					}
				}
			}

			cx += sz_cx;

			// 次のleftpを求める
			/* Block1 */ {
			int delimtype_cur = delimtype;
			const tchar *next_leftp;
			const tchar *text_endp;	// end pointer of the plain text.
			int tlength;
			if (overline){
				text_endp =
				next_leftp = leftp+nFit;
				delimtype_cur = DMT_OVERLINE;
				tlength = nFit;
			} else {
				text_endp = endp;
				int w;
				#if 0
				if (delimtype&(DMT_TAB|DMT_CR|DMT_ILLEGAL|DMT_HTML)){
					delimtype_cur = 0;
					for (;;){
						tuchar c = *nextp++;
						if (c<' '){
							int dmt;
							switch (c){
								case '\t':
									w = GetTabWidth(cx, cur_font.GetSpcWidth() * setting.TabCol);
									dmt = DMT_TAB;
									break;
								case '\r':
									delimtype_cur |= DMT_CR;
									// 改行
									if (*nextp=='\n'){
										nextp++;
									}
									goto jnext;
								case '\n':
									// 改行
									delimtype_cur |= DMT_CR;
									goto jnext;
								case '\0':
									delimtype_cur |= DMT_TERMINAL;
									goto jend;
								default:	// illchar
									dmt = DMT_ILLEGAL;
#if ILLCHAR_AS_PERIOD
									w = cur_font.GetIllWidth();
#else
									w = 0;
#endif
									break;
							}
							if (!faNext.IsHidden()){
								if (cx+w>maxcx){
									// over draw region.
									overline = true;
									delimtype_cur |= DMT_OVERLINE;
									nextp--;
									break;
								}
								delimtype_cur |= dmt;
								cx += w;
							}
							endp = nextp;
						} else {
jend:;
							// normal char.
							nextp--;
							break;
						}
					}
jnext:;
					if (!faNext.IsHidden()){
						delimtype |= delimtype_cur;
					}
				}
				#endif
				#if 1	// ver.2
				if (delimtype&(DMT_TAB|DMT_CR|DMT_ILLEGAL/*|DMT_HTML*/)){
					delimtype_cur = 0;
					for (;;){
						tuchar c = *nextp++;
						if (c<' '){
							int dmt;
							switch (c){
								case '\t':
									w = GetTabWidth(cx, cur_font.GetSpcWidth() * setting.TabCol);
									dmt = DMT_TAB;
									break;
								case '\r':
									delimtype_cur |= DMT_CR;
									// 改行
									if (*nextp=='\n'){
										nextp++;
									}
									goto jnext;
								case '\n':
									// 改行
									delimtype_cur |= DMT_CR;
									goto jnext;
								case '\0':
									delimtype_cur |= DMT_TERMINAL;
									goto jend;
								default:	// illchar
									dmt = DMT_ILLEGAL;
#if ILLCHAR_AS_PERIOD
									w = cur_font.GetIllWidth();
#else
									w = 0;
#endif
									break;
							}
							if (!faNext.IsHidden()){
								if (cx+w>maxcx){
									// over draw region.
									overline = true;
									delimtype_cur |= DMT_OVERLINE;
									nextp--;
									break;
								}
								delimtype_cur |= dmt;
								cx += w;
							}
							endp = nextp;
						} else {
jend:;
							// normal char.
							nextp--;
							break;
						}
					}
jnext:;
					if (!faNext.IsHidden()){
						delimtype |= delimtype_cur;
					}
				}
				#endif
				next_leftp = nextp;
				tlength = textlen;
			}

			if ( linesinfo.MaxRight < cx )
				linesinfo.MaxRight = cx;

			if (info.FontAttr.IsHidden()){
				leftp = next_leftp;
				break;
			}

			int length = altmode ? tlength : STR_DIFF(next_leftp,saved_leftp);

			if (/*!altmode &&*/ leftp==endp){
				// １文字も表示できない or 改行のみ
				if (!(delimtype_cur&DMT_CR)){
					// 改行のみでない場合
					if (textlen==0){
						// delim charと仮定
						__assert((delimtype&DMT_HYPLINK2) || (delimtype!=0 && saved_leftp!=nextp));
						leftp = nextp;
					} else {
						leftp = saved_leftp + textlen;
					}
					break;
				}
			}

			_DBW("loc=%d length=%d tlength=%d left=%d top=%d cx=%d dmt=%x", textloc, length, tlength, left, top, cx, delimtype_cur);
			if (!info.FontAttr.IsHidden()){
				__assert((altmode ? tlength : length)>=tlength);
				info.set(
					textloc,
					length,
					tlength,
					left,
					top,
					cx-left,
					delimtype_cur
					);
				linesinfo.Lines.push_back(info);
			}

			leftp = next_leftp;
			if (!overline){
				if (delimtype & DMT_CR /* || delimtype==0*/){	// delimtype==0必要？2007.11.5
					lines++;
					cx = 0;
					top += cyMax+CY_LINEINTERVAL;
					linesinfo.cyMaxes.push_back(cyMax+CY_LINEINTERVAL);
					cyMax = 0;
					added = false;
				} else {
					added = true;
				}
				break;
			}
			// １行に表示しきれなかった
			textlen -= tlength;
			textloc += length;
			endp = saved_endp;
			saved_leftp = leftp;
			}	/* Block1 */
joverline:
			lines++;
			cx = 0;
			top += cyMax + CY_LINEINTERVAL;
			linesinfo.cyMaxes.push_back(cyMax+CY_LINEINTERVAL);
			cyMax = 0;
			added = false;
		}
	}

	int height;
	
	if (added){
		lines++;
		linesinfo.cyMaxes.push_back(cyMax+CY_LINEINTERVAL);
		height = top + cyMax+CY_LINEINTERVAL - rc->top;	// 未改行の分も含めた高さ
	} else {
		if (delimtype & DMT_CR){
			// 最後が改行のみ
			lines++;
			cx = 0;
			cyMax = info.pfont->cy;
			top += cyMax+CY_LINEINTERVAL;
			linesinfo.cyMaxes.push_back(cyMax+CY_LINEINTERVAL);
		}
		height = top - rc->top;	// 2016.7.21 修正。旧バージョンは大丈夫？
	}

#ifdef _DEBUG
//	DBW("height=%d top=%d rc->top=%d lines=%d cyMax=%d", height, top, rc->top, lines, cyMax+CY_LINEINTERVAL);
#endif
	
	linesinfo.Height = height;
	linesinfo.LastRight = cx;
	linesinfo.NumLines = lines;
	linesinfo.LastTop = top;
	linesinfo.LastCYMax = cyMax + CY_LINEINTERVAL;
}

// fa : HTML parseを有効にする場合はset
// tagtop : HTML parseを有効にする場合は!=NULL
const tchar *FindDelimiter(const tchar *leftp, int &delimtype, vector<int> &wordindex, TDrawSetting &setting, TFontAttrEx *fa, const tchar **tagtop, TFontTagStack &faStack, THyperLinks *hls __arg_asc(bool fSingleByte))
{
	delimtype = 0;
	ushort c;
	bool word_reset = true;
	const tchar *p = leftp;
	while ( *p ){
		const tchar *nextp;
		if ( *p ){
			nextp = _CharNext(p, fSingleByte);
			c = *(tuchar*)p;
			IF_SingleByte(c, p, nextp)
			{
				// single byte //
				if ( isalnum( c )
					|| ( (uchar)c >= 0x80 )	// 0x80以降はどれがワードブレークになるかわからないためすべて??
					){
jwordchar:
					if (word_reset){
						word_reset = false;
						wordindex.push_back(STR_DIFF(p, leftp));
					}
				} else {
					if (!word_reset){
						word_reset = true;
						wordindex.push_back(STR_DIFF(p,leftp));
					}
					if ( c < ' ' ){
						int dmt = 0;
						// 制御コード
						switch (c){
							case '\t':
								dmt = DMT_TAB;
								// タブ
								break;
							case '\r':
								dmt = DMT_CR;
								// 改行
								if (*nextp=='\n'){
									p++;
								}
								break;
							case '\n':
								dmt = DMT_CR;
								// 改行
								break;
							default:
								dmt = DMT_ILLEGAL;
								break;
						}
						delimtype = dmt;	// Ver.2同様
						return p;
					} else {
						// normal char.
						if (fa){
							const tchar *nextp2;
							if (c=='<'){
								*tagtop = p;
								do {
									// HTML?
									nextp2 = ParseHtml(p, setting, *fa, faStack, hls);
									if (nextp2==p){
										// Not HTML or end
										// if (PdicHyperLink enabled)
										int type;
										nextp2 = ParsePdicHyperLink(p, type);
										if (!nextp2){
											// No hyperlink
											break;
										}
										// PDIC hyper link tag
										delimtype = DMT_HYPLINK1;
										*tagtop = nextp2;

#if 0	//作りかけ 2016.8.5
										if (hls){
											THyperLink *hl = new THyperLink;
											hl->type = type;
											hl->item = hls->curitem;
											text = hl->GetLink(p, _text, text);
										}
#endif
										return nextp2;
									}
									// Yes, HTML
									p = nextp2;
									//c = *p;
								} while(0);
								//while (c=='<');	//TODO: なぜこれでloopする必要あり？ 2016.8.5
								if (*tagtop!=p){
									// HTML exists.
									delimtype = DMT_HTML;
									return p;
								}
								*tagtop = NULL;
							}
							else if (c=='&' && p[1]=='#' && CodeToChar(p+2, &nextp)){
								// 変換文字をどこで保持する？
								delimtype = DMT_SPCCHAR;
								return nextp;
							}
#if 1	// <>の無い http://対応。強調表示vectorを使用せず、ここで解析する場合
							else
							if (c==':'){
								nextp2 = ParsePdicHyperLink2(p, leftp);
								if (nextp2){
									// PDIC hyper link tag
									delimtype = DMT_HYPLINK2;
									*tagtop = nextp2;
									return nextp2;
								}
								*tagtop = NULL;
							}
#endif
						}
					}
				}
			} else {
				// 禁則文字
				if (c <= 0x2B8){
					goto jwordchar;
				}
				if (!word_reset){
					word_reset = true;
					wordindex.push_back(STR_DIFF(p,leftp));
				}
			}
		}
		p = nextp;
	}
	if (*p=='\0') delimtype = DMT_TERMINAL;
	return p;
}

// tagp : points to the top of the tag.
const tchar *ParseHtml(const tchar *tagp, TDrawSetting &setting, TFontAttrEx &fa, TFontTagStack &faStack, THyperLinks *hls)
{
	const tchar *name_end;
	const tchar *np = ParseHtmlTag(tagp, &name_end, NULL);
	if (!np)
		return tagp;

	// Found HTML tag
	const tchar *nextp = tagp+1;
	bool on;
	if (*nextp=='/'){
		on = false;	// OFF
		nextp++;
	} else {
		on = true;	// ON
	}
	if (nextp==name_end){
		// no tag name
		return tagp;
	}
	// Found the tag name.
	// -- No attribute tags --
	int name_len = STR_DIFF(name_end, nextp);
	if (!_tcsnicmp(nextp, _t("b>"), name_len+1)){
		fa.SetBold(on);
	} else
	if (!_tcsnicmp(nextp, _t("i>"), name_len+1)){
		fa.SetItalic(on);
	} else
	if (!_tcsnicmp(nextp, _t("u>"), name_len+1)){
		fa.SetUnderline(on);
	} else
	if (!_tcsnicmp(nextp, _t("s>"), name_len+1)){
		fa.SetStrikeOut(on);
	} else
	if (!_tcsnicmp(nextp, _t("small>"), name_len+1)){
		if (on){
			fa.SetAddSize(-1);
		} else {
			fa.SetAddSize(0);
		}
	} else
	if (!_tcsnicmp(nextp, _t("big>"), name_len+1)){
		if (on){
			fa.SetAddSize(+1);
		} else {
			fa.SetAddSize(0);
		}
	} else
	// -- attribute available tags --
#if 0	// 必要ないか？
	if (!_tcsnicmp(nextp, _t("r"), name_len)){
		// PDIC specific
		if (on){
			bool no_color = true;
			faStack.push(fa);	// Save the current font attr.
			tnstr_map attrs;
			ParseHtmlTag(tagp, NULL, &attrs);
			COLORREF color = fa.Color;
			if (attrs.count(_t("color"))){
				color = ParseColor(attrs[_t("color")]);
				if (color!=(COLORREF)-1){
					fa.Color = fa.BgColor;
					fa.BgColor = color;
					no_color = false;
				}
			}
			if (no_color){
				fa.Color = fa.BgColor;
				fa.BgColor = color;
			}
		} else {
			// font off
			goto jstackpop;
		}
	} else
#endif
	if (!_tcsnicmp(nextp, _t("font"), name_len)){
		if (on){
			faStack.push(fa);	// Save the current font attr.
			AttrInfoMap attrs;
			ParseHtmlTag(tagp, NULL, &attrs);
			if (attrs.count(_t("size"))){
				tnstr strSize = attrs[_t("size")].value;
				tchar c = strSize[0];
				if (c=='+' || c=='-'){
					fa.SetAddSize(_ttoi(strSize));
				} else
				if (c>='0'&&c<='9'){
					fa.SetHtmlFontSize(_ttoi(strSize));
				}
			}
			if (attrs.count(_t("color"))){
				COLORREF color = ParseColor(attrs[_t("color")].value);
				if (color!=(COLORREF)-1){
					fa.Color = color;
				}
			}
			// PDIC specific
			if (attrs.count(_t("bgcolor"))){
				COLORREF color = ParseColor(attrs[_t("bgcolor")].value);
				if (color!=(COLORREF)-1){
					fa.BgColor = color;
				}
			}
		} else {
			// font off
			goto jstackpop;
		}
	} else
	if (!_tcsnicmp(nextp, _t("a"), name_len)){
		if (on){
			AttrInfoMap attrs;
			ParseHtmlTag(tagp, NULL, &attrs);
			if (attrs.count(_t("href"))){
				faStack.push(fa);	// Save the current font attr.
				fa.SetUnderline(on);
				fa.Color = rgbLinkColor;
				if (hls && (hls->req_parse & hls->curitem)){
					THyperLink *hl = new THyperLink();
					hl->type = HLT_HTML_HREF;
					hl->item = hls->curitem;
					hl->key = attrs[_t("href")].value;
					hl->loc = STR_DIFF(np, hls->top);
					hls->add(hl);
					// save the values temporarily
					hls->left = np;
					hls->tag = hl;
				}
			}
		} else {
			if (hls && hls->tag){
				hls->tag->length = STR_DIFF(tagp, hls->left);
				hls->tag = NULL;
			}
			fa.SetUnderline(false);
			goto jstackpop;
		}
	} else
	if (!_tcsnicmp(nextp, _t("ex>"), name_len+1)){
		if (on){
			if (!setting.ExpVisible){
				fa.SetHidden(true);
			}
		} else {
			fa.SetHidden(false);
		}
	} else
	if (!_tcsnicmp(nextp, _t("bx"), name_len)){
		if (on){
			AttrInfoMap attrs;
			ParseHtmlTag(tagp, NULL, &attrs);
			fa.SetHyperLink(NULL);	//TODO: ほんとうに必要か？必要な場合は↓のようにする
			//fa.pushed(); ←push()内でhl = NULL;
			faStack.push(fa);	// Save the current font attr.
			//dbw("push: %08X %x", fa.HyperLink, fa.AttrEx);

			COLORREF color;

			if (hls){
				THyperLink *hl;
				if (hls->req_parse & hls->curitem){
					// Add hyper link object to make clickable
					hl = new THyperLink();
					hl->type = HLT_HTML_BXTAG;
					hl->item = hls->curitem;
					hl->bxtag = 1;
					hl->state = setting.BoxOpen;
					hls->add(hl);
				} else {
					hl = hls->Next(HLT_HTML_BXTAG);
				}
				if (hl->state){
					// open state
					hl->loc = STR_DIFF(np, hls->top);
					hls->hliStack.push( THyperLinkInfo(hl, np) );	// save the values temporarily
				} else {
					// close state

					// text color //
					if (attrs.count(_t("color"))){
						color = ParseColor(attrs[_t("color")].value);
						if (color != (COLORREF)-1){
							fa.Color = color;
						}
					}

					// background color //
					color = RGB(0,255,255);
					if (attrs.count(_t("bgcolor"))){
						color = ParseColor(attrs[_t("bgcolor")].value);
					}
					fa.BgColor = color;

					if (attrs.count(_t("title"))){
						AttrInfo &ai = attrs[_t("title")];
						if (hl->key.empty())	// 同じkeyを何度も設定するのを抑制
							hl->key = ai.value;
						hl->loc = ai.loc + STR_DIFF(tagp, hls->top);
						hl->length = ai.length;
					} else {
						hl->loc = STR_DIFF(tagp, hls->top);
						hl->length = 0;
					}
					hls->hliStack.push( THyperLinkInfo(hl, NULL) );	// save the values temporarily
					fa.SetHiddenNext(true);
				}
				fa.SetHyperLink(hl);
			}
		} else {
			if (hls && hls->hliStack.size()>0){
				THyperLinkInfo &hli = hls->hliStack.top();
				if (hli.left) hli.hl->length = STR_DIFF(np, hli.left);
				hls->hliStack.pop();
			}
			goto jstackpop;
		}
	} else {
		// Not supported tag.
		return tagp;
	}
	return np;
jstackpop:
	if (faStack.size()){
		fa.AssignFontTag(faStack.top());
		faStack.pop();
		//fa.SetHiddenNext(false);	// このタイミングはあまりよろしくないが。。
		//dbw("pop: %08X %x", fa.HyperLink, fa.AttrEx);
	}
	return np;
}

struct THTMLColorNames {
	const char *name;
	COLORREF value;
};
static THTMLColorNames HTMLColorNames[] = {
	{"black",	RGB(0,0,0)},
	{"gray",	RGB(0x80, 0x80, 0x80)},
	{"silver",	RGB(0xc0, 0xc0, 0xc0)},
	{"white",	RGB(0xff, 0xff, 0xff)},
	{"maroon",	RGB(0x00, 0x80, 0x00)},
	{"red",		RGB(0xff, 0x00, 0x00)},
	{"purple",	RGB(0x80, 0x00, 0x80)},
	{"fuchsia",	RGB(0xff, 0x00, 0xff)},
	{"green",	RGB(0x00, 0x80, 0x00)},
	{"lime",	RGB(0x00, 0xff, 0x00)},
	{"olive",	RGB(0x80, 0x80, 0x00)},
	{"yellow",	RGB(0xff, 0xff, 0x00)},
	{"navy",	RGB(0x00, 0x00, 0x80)},
	{"blue",	RGB(0x00, 0x00, 0xff)},
	{"teal",	RGB(0x00, 0x80, 0x80)},
	{"aqua",	RGB(0x00, 0xff, 0xff)},
	{NULL}
};

//TODO: 汎用関数へ
// ASCII charのみで構成されるwide char s1とs2の比較
static bool strequ(const wchar_t *s1, const char *s2)
{
	while (1){
		unsigned char c1 = (unsigned char)*s1;
		if (c1!=*(unsigned char*)s2){
			return false;
		}
		if (c1==0)
			return true;
		s1++;
		s2++;
	}
}

COLORREF ParseColor(const tchar *strColor)
{
	if (strColor[0]=='#'){
		int color = atox(&strColor[1], NULL);
		return ((color>>16)&0xFF) | (color&0xFF00) | ((color&0xFF)<<16);
	}
	THTMLColorNames *cn = &HTMLColorNames[0];
	while (cn->name){
		if (strequ(strColor, cn->name)){
			return cn->value;
		}
		cn++;
	}
	return (COLORREF)-1;
}

static void tolower(tnstr &str)
{
	for (int i=0;i<str.size();i++){
		if (str[i]>='A' && str[i]<='Z')
			str[i] = str[i] | 0x20;
	}
}

static void setTagAttr(AttrInfoMap &attrs, const tchar *text, const tchar *name_top, const tchar *name_tail, const tchar *val_top, const tchar *val_tail)
{
	tnstr key(name_top, STR_DIFF(name_tail, name_top));
	tolower(key);
	if (*val_top=='"' || *val_top=='\''){
		val_top++;
		val_tail--;
	}
	int vallen = STR_DIFF(val_tail, val_top);
	tnstr val(val_top, vallen);
	attrs[key].loc = STR_DIFF(val_top, text);
	attrs[key].length = vallen;
	attrs[key].value = val;
}

static void setTagAttr(AttrInfoMap &attrs, const tchar *text, const tchar *name_top, const tchar *name_tail)
{
	tnstr key(name_top, STR_DIFF(name_tail, name_top));
	tolower(key);
	attrs[key].loc = STR_DIFF(name_top, text);
	attrs[key].length = 0;
	attrs[key].value = _t("1");
}

// Parse "<...>"
// locs : attrsに代入したtagの文字位置
const tchar *ParseHtmlTag(const tchar *text, const tchar **name_end, AttrInfoMap *attrs __arg_asc(bool fSingleByte))
{
	const tchar *p = text+1;
	int quoted = 0;
	#define	quote_single	1
	#define	quote_double	2

	// Parse name
	if (*p=='/')
		p++;
	for (;isalpha(*p);){
		p++;
	}
	if (name_end) *name_end = p;

	if (*p!=' '&&*p!='\t'&&*p!='>'){
		return NULL;	// Not HTML
	}
	while (*p==' '||*p=='\t') p++;
	const tchar *name_top = NULL;
	const tchar *name_tail = NULL;
	const tchar *val_top = NULL;

	for (;;){
		const tchar *nextp = _CharNext(p, fSingleByte);
		ushort c = *(tuchar*)p;
		IF_SingleByte(c, p, nextp){
			if (quoted){
				switch (c){
					case '\r':
					case '\n':
					case '\0':
						return NULL;	// Not HTML
					case '"':
						if (quoted&quote_double){
							quoted = 0;
						}
						break;
					case '\'':
						if (quoted&quote_single){
							quoted = 0;
						}
						break;
				}
			} else {
				switch (c){
					case '\r':
					case '\n':
					case '\0':
						return NULL;	// Not HTML
					case '"':
						quoted = quote_double;
						break;
					case '\'':
						quoted = quote_single;
						break;
					case '>':
						if (attrs){
							if (val_top){
								setTagAttr(*attrs, text, name_top, name_tail, val_top, p);
							}
#if 0
							foreach(*attrs, it, tnstr_map){
								tnstr key = it->first;
								tnstr val = it->second;
								DBW("key=%ws val=%ws", it->first.c_str(), val.c_str());
							}
#endif
						}
						return nextp;
					case '=':
						if (attrs){
							if (!val_top){
								if (!name_tail)
									name_tail = p;
							} else {
								// unknown format such like "name=value=..."
							}
						}
						goto jnext;
					case ' ':
					case '\t':
						if (attrs){
							if (val_top){
								setTagAttr(*attrs, text, name_top, name_tail, val_top, p);
								name_top = name_tail = val_top = NULL;
							} else
							if (name_top){
								__assert(!name_tail);
								// nameのみ
								setTagAttr(*attrs, text, name_top, p);
								name_top = NULL;
							}
						}
						goto jnext;
					default:
						break;
				}
				if (attrs){
					//if (isalpha(c))
					if (c>' ')
					{
						goto jnormal;
					}
				}
jnext:;
			}
		} else {
jnormal:
			if (attrs){
				if (!name_top){
					// The top of the name.
					name_top = p;
				} else
				if (name_tail){
					if (!val_top){
						// The top of value.
						val_top = p;
					}
				}
			}
		}
		p = nextp;
	}
}

struct NameList {
	const tchar *name;
	int len;
	int skiplen;
	int type;
};

static const NameList namelist[] =
{
	{ _T("word:"), 5, 5, HLT_WORD },
	{ _T("→"), BYTETOLEN(2), BYTETOLEN(2), HLT_WORD2 },
	{ _T("http:"), 5, 0, HLT_HTTP },
	{ _T("https:"), 6, 0, HLT_HTTP },
	{ _T("mailto:"), 7, 0, HLT_MAILTO },
	{ _T("file:"), 5, 5, HLT_FILE },
	{ _T("text:"), 5, 5, HLT_TEXT },
//	{ _T("html:"), 5, HLT_HTML },	// HTMLのpop-up
};

// text : points to the top of the tag.
// return : points to the top of the link address.
const tchar *ParsePdicHyperLink(const tchar *text, int &type)
{
	text++;	// Skip '<'.
	for ( int i=0;i<sizeof(namelist)/sizeof(NameList);i++ ){
		if ( !_tcsncmp( text, namelist[i].name, namelist[i].len ) ){
			const tchar *linkp = text + namelist[i].len;
			const tchar *p = linkp;
			// '>' check
			for (;;){
				if (*p=='>'){
					// Found terminator.
					type = namelist[i].type;
					return text;
				}
				if (*(tuchar*)p<' '){
					// terminator
					return NULL;	// not pdic hyperlink
				}
				p = _CharNext(p, false);
			}
		}
	}
	return NULL;	// not found any tag.
}

// text : points to the ':'
// top : points to the top of the valid text string including 'text'
// return : points to the top of the link address
const tchar *ParsePdicHyperLink2(const tchar *text, const tchar *top)
{
	for ( int i=0;i<sizeof(namelist)/sizeof(NameList);i++ ){
		int offs = namelist[i].len-1;
		if (text-offs<top)
			continue;	// out of string
		if ( !_tcsncmp( text-offs, namelist[i].name, namelist[i].len ) ){
			const tchar *linkp = text+1;
			const tchar *p = linkp;
			// terminator check
			for (;;){
				if (IsHypLinkTerm(*p)){
					// Found terminator.
					return p;
				}
				p = _CharNext(p, false);
			}
		}
	}
	return NULL;	// not found any tag.
}

// wordindex : 昇順に並んだ単語位置情報(even index:単語開始位置 odd index:単語終了位置+1)
// wi_offs : wordindex offset = wordindex[]の値がずれたときのoffset for fast operation.
//			 wi_offs>0のとき、returned pointer<leftpになる場合がある→leftpとして扱うこと
static const tchar *FindBreakPoint(vector<int> &wordindex, int wi_offs, const tchar *leftp, int endloc)
{
	int left = 0;
	int right = wordindex.size()-1;
	for(;left<=right;){
		int mid = (left+right)/2;
		int dif = endloc - (wordindex[mid]-wi_offs);
		if (dif<0){
			right = mid-1;
		} else
		if (dif>0){
			left = mid+1;
		} else {
			// Not word broken.
			return leftp+endloc;
		}
	}
	__assert(right<0 || left>=wordindex.size() || endloc>(wordindex[right]-wi_offs) && endloc<(wordindex[left]-wi_offs));
	if (right&1){
		// odd index -> not word broken
		return leftp+endloc;
	}
	return leftp+(wordindex[right]-wi_offs);
}

// &#nn;変換
// strは#の次へのpointer
//Note: surrogateは非対応
wchar_t CodeToChar(const tchar *str, const tchar **nextp )
{
	wchar_t cc = 0;

	tchar c = *str;
	if (c=='x'){
		str++;
		while (1){
			c = *str++;
			if (c==';'){
				break;
			} else
			if (c>='0' && c<='9'){
				c -= '0';
			} else
			if (c>='a' && c<='f'){
				c = c - 'a'+0xA;
			} else
			if (c>='A' && c<='F'){
				c = c - 'F'+0xA;
			} else {
				return 0;	// no code
			}
			cc = (cc<<4) | c;
		}
	} else {
		while (1){
			c = *str++;
			if (c==';'){
				break;
			} else
			if (c>='0' && c<='9'){
				cc = cc * 10 + (c-'0');
			} else {
				return 0;	// no code
			}
		}
	}
	if (nextp) *nextp = str;
	return cc;
}

