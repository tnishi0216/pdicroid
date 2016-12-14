#include "pdclass.h"
#pragma	hdrstop
#include "winsqu.h"
#include "winmsg.h"
#include "squfont.h"
#include "dictext.h"
#include "srchcom.h"
#ifdef EPWING
#include "depw.h"
#endif
#include "windic.h"		// for pew

#include "wpdcom.h"
#include "WinSquUI.h"
#include "SquItemView.h"
#include "LangProc.h"
#include "tid.h"

#include "draw4def.h"	// for line interval

#include "MouseCapture.h"

#if USE_MEMICON
#include "tnimage.h"
#endif

#define	GAP_ITEM	2	// 項目間の間隔(dot)

#define	DEF_LEVELVALUE	0

int LevelValue = DEF_LEVELVALUE;

// DrawText2 version /////////////////////////////

//	rc.left		: 描画開始X座標(ドット単位)
//	rc.top		: 描画開始Y座標(ドット単位)
//	返り値
//	rc.right	: 表示右端X座標（ドット単位）
#if SQU_DISP_NUM
int Squre::DispNumber( RECT &rc, int no, int dispf )
{
	if ( dispf & DF_DISP ){
		hnFont->Select();
		// 番号＆矢印表示
		tchar buf[ 20 + 1 ];
		itos(no+1, buf);
		SetTextAlign( hdc, TA_RIGHT );
		TextOut( hdc, rc.left + GetWidth( SN_NUMBER ) - cxText, rc.top, buf, lstrlen( buf ) );
		SetTextAlign( hdc, TA_LEFT );
	}
	return 1;
}
#endif

int Squre::DispWord( RECT &rc, const tchar *word, int dispf, CharHT *cht, THyperLinks *hls )
{
	word = find_cword_pos(word);	// 表示用単語のみ

	const int width = GetWidth(SN_WORD);
//	rc.left += GetSystemMetrics( SM_CYBORDER )*2;	// removed 2008.9.4
#if SQUONELINE
	if ( ViewOneLine ){
		rc.right = 0x7FFFFFFF;
	} else
#endif
	{
		rc.right = GetLX();
		rc.bottom = GetY0() + GetLY();
	}
	if ( cht ){
		if ( cht->item != -1 && cht->item != SN_WORD ){
			cht = NULL;	// 反転表示、ヒットテストは自分のところではない
			if (dispf & DF_PINPOINT)
				dispf &= ~DF_DISP;
		} else {
			cht->curitem = SN_WORD;
		}
	}
	int l;
	hwFont->Select();
	HBRUSH hbr = NULL;
	const int GAP_WORD = 2;
	{
#if USE_DT2
		if (hls && (hls->req_parse & HLI_WORD)){
			hls->curitem = HLI_WORD;
			ParseHtmlHitPosition(*hwFont, word, &rc, *hls);
			hwFont->Select();	// 上記関数でhwFontが他のfontに使用された後、元のfontに戻すときにSystemFontに戻すTNFONTの（まずい）仕様のため必要
			hls->req_parse &= ~HLI_WORD;
		}
		EnphTextVec et;
		if ( !(dispf & (DF_CALCRECT|DF_UNREDRAW)) && ss.IsTextSearch( SRCH_WORD ) ){
			GetHitPosition( et, word, _tcslen(word) );
		}
		if ( hls && !(dispf&DF_UNREDRAW) ){
			MakeEnph( et, *hls, HLI_WORD );
		}
#endif
		if (dispf&DF_DISP){
			// 左端のgap
			hbr = CreateSolidBrush( dispf & DF_REVERSE ? GetTextColor( hdc ) : GetBkColor( hdc ) );
			RECT r;
			SetRect( &r, rc.left, rc.top, rc.left+GAP_WORD, rc.top + hwFont->tmHeight );
			FillRect( hdc, &r, hbr );
		}
		rc.left += GAP_WORD;
		l = AdjDisp( rc, *hwFont, word, dispf, width, cht, &et );
	}

#if !defined(SMALL) && !defined(_UNICODE)
	if ( !fWindowsNT ){
		SIZE sz;
		GetTextExtentPoint32( *hwFont, word, _tcslen(word), &sz );
		rc.right = rc.left + sz.cx;
	}
#endif
	// 反転バーの残りをfill
	int bar_width = GetOffs(SN_WORD)+width - rc.right;
	if (bar_width<GAP_WORD)
		bar_width = GAP_WORD;
	if ( dispf & DF_DISP ){
		RECT r;
		SetRect( &r, rc.right, rc.top, rc.right+bar_width, rc.top + hwFont->tmHeight );
		FillRect( hdc, &r, hbr );
	}
	if (hbr){
		DeleteObject(hbr);
	}
	rc.top = GetLastTop();
	rc.right = GetLastRight()+bar_width;
	return l;
}

int Squre::DispAttr( RECT &rc, uchar attr, int dispf, CharHT *cht )
{
	if ( cht ){
		if ( cht->item != -1 && cht->item != SN_ATTR ){
			cht = NULL;	// 反転表示、ヒットテストは自分のところではない
			if (dispf & DF_PINPOINT)
				dispf &= ~DF_DISP;
		} else {
			cht->curitem = SN_ATTR;
		}
	}
	if ( dispf & DF_DISP ){
		HBRUSH hbr = NULL;
#if SQUONELINE
		if ( !ViewOneLine )
#endif
			rc.bottom = rc.top + cyAttr;
		if ( attr & WA_MEMORY ){
#if USE_MEMICON
			imgMemory.Draw(hdc, rc, dispf, false, 0xFFFFFF);
#else
			hnFont->Select();
			AdjDisp( rc, *hnFont, StrMemory, dispf, GetWidth( SN_ATTR ), NULL );
#endif
		} else {
			RECT _rc = rc;
			if (_rc.left+cxAttr2<_rc.right)
				_rc.right = _rc.left+cxAttr2;
			hbr = CreateSolidBrush( GetBkColor( hdc ) );
			FillRect( hdc, &_rc, hbr );
		}
		rc.left = rc.left + cxAttr2;
		if (rc.left<rc.right){
			if ( attr & WA_JEDIT ){
#if USE_MEMICON
				imgModify.Draw(hdc, rc, dispf, false, 0xFFFFFF);
#else
				if ( !(attr & WA_MEMORY) )
					hnFont->Select();
				AdjDisp( rc, *hnFont, StrJEdit, dispf, GetWidth( SN_ATTR ), NULL );
#endif
			} else {
				RECT _rc = rc;
				if (_rc.left+cxAttr2<_rc.right)
					_rc.right = _rc.left+cxAttr2;
				if ( !hbr )
					hbr = CreateSolidBrush( GetBkColor( hdc ) );
				FillRect( hdc, &_rc, hbr );
			}
		}
		if ( hbr )
			DeleteObject( hbr );
	}
	int width = cxAttr2*2;
	if (rc.left+width<rc.right)
		rc.right = rc.left+width;
	return cyText;
}
int Squre::DispAudio(RECT &rc, int dispf, int swidth, int objIndex, JLink &jl, CharHT *cht)
{
	//TODO: rc.leftがswidthに足りないときは改行する
#if 1
	dispf &= ~(DF_SELECTED|DF_UNSELECTED);	// no selection
	int l = jl.Draw(*hnFont, rc, dispf, NULL);
	if ( cht && !dispf){
		if ( PtInRect( &rc, cht->pt ) ){
			cht->item = SN_OBJECT + objIndex;
			cht->cpos = *(POINT*)&rc;
		}
	}
	return l;
#else
	TNImage image(IMG_AUDIO_ICON);
	if ( cht && !dispf){
		RECT _rc = rc;
		_rc.right = _rc.left + image.GetCX();
		_rc.bottom = _rc.top + image.GetCY();
		if (PtInRect(&_rc, cht->pt)){
			cht->item = SN_OBJECT + objIndex;
		}
	} else {
		hnFont->Select( );
		image.Draw(*hnFont, rc, dispf, false);
	}
	rc.right = rc.left + image.GetCX();
	return image.GetCY();
#endif
}

// cxText
//	数字のwidth。LevelValue=1を使用しない場合は不要
int DrawLevel( HDC hdc, int level, int left, int top, int right, int cy_level, int cxText )
{
	int GAP_LEVEL;
	if (LevelValue){
		GAP_LEVEL = (cy_level+15) / 16;
	} else {
		GAP_LEVEL = 0;
	}
	int cx_level = cy_level * 2 / 3;	// level1マス分の幅
//	if ( GAP_LEVEL == 0 ) GAP_LEVEL = 1;
	HBRUSH hbrText;
	HBRUSH hbrBack = NULL;
	COLORREF oldtext, oldback;
	if ( hdc ){
		hbrText = CreateSolidBrush( GetTextColor(hdc) );
		hbrBack = CreateSolidBrush( GetBkColor(hdc) );
		oldtext = SetTextColor( hdc, GetBkColor( hdc ) );
		oldback = SetBkColor( hdc, oldtext );
	}
	RECT _rc;
	_rc.left = left + GAP_LEVEL;
	_rc.top = top + GAP_LEVEL;
	_rc.bottom = _rc.top + cy_level - GAP_LEVEL*2;

	if (hbrBack){
		if (level){
			_rc.right = _rc.left + cx_level * level;
			FillRect(hdc, &_rc, hbrBack);	// fill the background.
		}
	}

	_rc.right = _rc.left + cx_level - 2;

	int i;
	for ( i=0;i<level;i++ ){
		// draw ■
		if ( hdc ){
			FillRect( hdc, &_rc, hbrText );
			if (LevelValue==2){
				switch ( i ){
					tchar buf[2];
					case 0:
						if ( level>=10 ){
							buf[0] = level/10+'0';
						} else {
							buf[0] = level+'0';
						}
						hnFont->Select();
						_rc.right--;
						ExtTextOut( hdc, _rc.left+1, _rc.top, ETO_CLIPPED, &_rc, buf, 1, NULL );
						_rc.right++;
						break;
					case 1:
						if ( level>=10 ){
							hnFont->Select();
							buf[0] = level%10+'0';
							_rc.right--;
							ExtTextOut( hdc, _rc.left+1, _rc.top, ETO_CLIPPED, &_rc, buf, 1, NULL );
							_rc.right++;
							break;
						}
					default:
						break;
				}
			} // LevelValue==2
		}
		_rc.left += cx_level;
		_rc.right += cx_level;
	}
	int r = _rc.left - left;
	if ( hdc ){
		SetTextColor( hdc, oldtext );
		SetBkColor( hdc, oldback );
	}
	if (LevelValue==1
		&& level>=1){
		tchar buf[3];
		_itot(level,buf,10);
		int len = _tcslen(buf);
		if (hdc){
			hnFont->Select();
			SIZE sz;
			GetTextExtentPoint( hdc, buf, len, &sz );
			_rc.right = _rc.left + sz.cx;
			if ( _rc.right < right )
				_rc.right = right;
			ExtTextOut( hdc, _rc.left, _rc.top, ETO_CLIPPED | ETO_OPAQUE, &_rc, buf, len, NULL );
			r += sz.cx;
		} else {
			//TODO: 手抜き。完全対応するのは難しい。。
			r += cxText * len;
		}
	}
	if (hdc){
		// 残りの領域を消去する
		if (left + r < right){
			_rc.left = left+r;
			_rc.right = right;
			FillRect( hdc, &_rc, hbrBack );
		}
		DeleteObject( hbrText );
		if (hbrBack)
			DeleteObject(hbrBack);
	}
	return r;
}

#define	LEVEL_TOP_OFFSET	0	// 若干下にしたほうがバランスが取れる（ために作った？）

int Squre::_DispLevel( RECT &rc, int level, int dispf )
{
#if 1	// ■描画
	rc.right = rc.left +
		DrawLevel(
			(dispf & DF_DISP) ? hdc : NULL, level, rc.left, rc.top+LEVEL_TOP_OFFSET, rc.right, hnFont->cy, cxLevelText );
	return hnFont->cy;
#else	// **描画
	tchar buf[ WA_LEVELNUM ];
	int i;
	for ( i=0;i<level;i++ ){
		buf[i] = '*';
	}
	buf[i] = '\0';
	rc.right = LX;
#if SQUONELINE
	if ( !ViewOneLine )
#endif
		rc.bottom = GetY0() + LY;
	hnFont->Select();
	return AdjDisp( rc, *hnFont, buf, dispf, GetWidth( SN_LEVEL ), NULL, 0, NULL );
#endif
}

int Squre::DispLevel(RECT &rc, int level, int dispf, int &cyMax, CharHT *cht)
{
	RECT _rc;
	int l = 0;
	if ( level || dispf & DF_OPAQUE ){
		rc.right = min(rc.left + GetWidth(SN_LEVEL), rc.right);
		l = _DispLevel( rc, level, dispf );
		cyMax = max( cyMax, hnFont->cy );
	}
	if ( cht ){
		// ここでヒットテスト
		if ( cht->item != -1 && cht->item != SN_LEVEL ){
			// 
		} else {
			// hit test
			_rc = rc;
			_rc.right = min(_rc.left + GetWidth( SN_LEVEL ), rc.right);
			if ( _rc.bottom == _rc.top )	// レベルが無い場合のため
				_rc.bottom += cyText;
			if ( PtInRect( &_rc, cht->pt ) ){
				cht->item = SN_LEVEL;
				cht->pos = 0;
			}
		}
	} else
	if ( dispf & DF_OPAQUE ){
		// 単語レベル表示幅が最大レベル表示より狭いとき、レベルを下げると残るときがある
		// 2011.4.17 長い見出し語があると、レベル表示の左端がずれてしまうため、
		// rc.leftではなくGetOffs(SN_LEVEL)にして正規の表示位置からはみ出した場合、という条件にした
		if ( rc.right > GetOffs(SN_LEVEL) /*rc.left*/ + GetWidth( SN_LEVEL ) ){
			HBRUSH hbr = CreateSolidBrush( GET_BACKCOLOR( backcolor ) );
			_rc.left = rc.right;
			_rc.right = GetLX();
			_rc.top = rc.top+LEVEL_TOP_OFFSET;
			_rc.bottom = _rc.top + l;
			FillRect( hdc, &_rc, hbr );
			DeleteObject( hbr );
		}
	}
	return l;
}
int Squre::DispPron( RECT &rc, const tchar *pron, int dispf, CharHT *cht, THyperLinks *hls )
{
	rc.right = GetLX();
#if SQUONELINE
	if ( !ViewOneLine )
#endif
		rc.bottom = GetY0() + GetLY();
	hpFont->Select( );
	if ( cht ){
		if ( cht->item != -1 && cht->item != SN_PRON ){
			cht = NULL;	// 反転表示は自分のところではない
			if ( dispf & DF_PINPOINT )
				dispf &= ~DF_DISP;
		} else {
			cht->curitem = SN_PRON;
		}
	}
	if (!(IsHtmlEnabled()&SQM_PRON)){
		GetDefDrawSetting().HtmlParse = false;
	}
#if USE_DT2
	if (GetDefDrawSetting().HtmlParse){
		if (hls && (hls->req_parse & HLI_PRON)){
			hls->curitem = HLI_PRON;
			ParseHtmlHitPosition(*hpFont, pron, &rc, *hls);
			hpFont->Select();	// 上記関数でhwFontが他のfontに使用された後、元のfontに戻すときにSystemFontに戻すTNFONTの（まずい）仕様のため必要
			hls->req_parse &= ~HLI_PRON;
		}
	}
	EnphTextVec et;
	if ( !(dispf & (DF_CALCRECT|DF_UNREDRAW)) && ss.IsTextSearch( SRCH_PRON ) ){
		GetHitPosition( et, pron, _tcslen(pron) );
	}
	if ( hls && !(dispf&DF_UNREDRAW) ){
		MakeEnph( et, *hls, HLI_PRON );
	}
#endif
	int l = AdjDisp( rc, *hpFont, pron, dispf, GetWidth( SN_PRON ), cht, &et );
	GetDefDrawSetting().HtmlParse = true;
	rc.top = GetLastTop();
	rc.right = GetLastRight();
	return l;
}

int Squre::DispJapa( RECT &rc, const tchar *japa, int dispf, CharHT *cht, THyperLinks *hls )
{
	rc.right = GetLX();
#if SQUONELINE
	if ( !ViewOneLine )
#endif
		rc.bottom = GetY0() + GetLY();
	if ( cht ){
		if ( cht->item != -1 && cht->item != SN_JAPA ){
			cht = NULL;	// 反転表示は自分のところではない
			if ( dispf & DF_PINPOINT )
				dispf &= ~DF_DISP;
		} else {
			cht->curitem = SN_JAPA;
		}
	}
#if USE_DT2
	if (hls && (hls->req_parse & HLI_JAPA)){
		hls->curitem = HLI_JAPA;
		ParseHtmlHitPosition(*hjFont, japa, &rc, *hls);
		hjFont->Select();	// 上記関数でhwFontが他のfontに使用された後、元のfontに戻すときにSystemFontに戻すTNFONTの（まずい）仕様のため必要
		hls->req_parse &= ~HLI_JAPA;
	}
	EnphTextVec et;
	if ( !(dispf & (DF_CALCRECT|DF_UNREDRAW)) ){
		int ljapa = 0;
		if (ss.IsTextSearch( SRCH_JAPA ) ){
			GetHitPosition( et, japa, ljapa = _tcslen(japa) );
		}
		if (ss.IsAndSearch()){
			GetHitPosition( et, japa, ljapa ? ljapa : _tcslen(japa), true );
		}
	}
	if ( hls && !(dispf&DF_UNREDRAW) ){
		MakeEnph( et, *hls, HLI_JAPA );
	}
#endif
	hjFont->Select();
	return AdjDisp( rc, *hjFont, japa, dispf, rc.right - rc.left, cht, &et );
}
int Squre::DispExp( RECT &rc, const tchar *exp, int dispf,
#if USE_SLASH
	int initleft,
#endif
	CharHT *cht, THyperLinks *hls )
{
	rc.right = GetLX();
#if SQUONELINE
    if ( !ViewOneLine )
#endif
		rc.bottom = GetY0() + GetLY();
	if ( cht ){
		if ( cht->item != -1 && cht->item != SN_EXP1 ){
			cht = NULL;	// 反転表示は自分のところではない
			if ( dispf & DF_PINPOINT )
				dispf &= ~DF_DISP;
		} else {
        	cht->curitem = SN_EXP1;
		}
	}
#if USE_DT2
	if (hls && (hls->req_parse & HLI_EXP)){
		hls->curitem = HLI_EXP;
		ParseHtmlHitPosition(*heFont, exp, &rc, *hls);
		heFont->Select();	// 上記関数でhwFontが他のfontに使用された後、元のfontに戻すときにSystemFontに戻すTNFONTの（まずい）仕様のため必要
		hls->req_parse &= ~HLI_EXP;
	}
	EnphTextVec et;
	if ( !(dispf & (DF_CALCRECT|DF_UNREDRAW)) && ss.IsTextSearch( SRCH_EXP ) ){
		GetHitPosition( et, exp, _tcslen( exp ) );
	}
	if ( hls && !(dispf&DF_UNREDRAW) ){
		MakeEnph( et, *hls, HLI_EXP );
	}
#endif
	heFont->Select();
#if USE_SLASH
	SetInitLeft( initleft );
#endif
	return AdjDisp( rc, *heFont, exp, dispf, rc.right - rc.left, cht, &et );
}

// rev:
//		0=非反転
//		1=full reverse or gray reverse （自動判別）
//		2=gray reverse
//		3=full reverse
void Squre::_dispStar( int no, int rev )
{
	if ( no < 0 || no > LastIndex+1 )
		return;

	RECT rc;
	CreateTextFonts( );

#if !defined(SMALL) || SQUONELINE
	if (
#ifndef SMALL
		fLongBar ||
#endif
#if SQUONELINE
		(!rev && ViewOneLine)
#endif
		){
		DispOneLine( no, GetWord( no ), rev, GetJapa( no ), DF_DISP );
	} else
#endif
	{
#if USE_DT2
		CharHT *cht = MouseCap->CreateCharHTDispStar();
#endif
		SetRect( &rc, GetOffs( SN_WORD ), GetLocateY( no ), 0, GetLocateY(no)+GetLines(no) );
		COLORREF fore = GET_FORECOLOR( useuniqfont & SNF_WORD ? GetColor( SN_WORD ) : charcolor );
		COLORREF back = GET_BACKCOLOR( backcolor );
		if (rev==1&&cury>=0){
			rev = GetReverseStatus();
		}
		if (rev==2){
			// gray reverse
			fore = graycolor;
		}
		COLORREF oldtext = SetTextColor( hdc, fore );
		COLORREF oldback = SetBkColor( hdc, back );
		DispWord( rc, GetWord( no ), DF_DISP | ( rev ? DF_REVERSE : 0 )
#if USE_DT2
			| (cht ? DF_UNREDRAW : 0), cht
#endif
			);
			// 1999.8.25 (cht ? DF_UNREDRAW : 0)追加
			// cht != NULLのとき、Draw2()でfatal errorが発生するため、
			// 対症療法により、chtのときは必ずDF_UNREDRAWにする。
			// これは、WM_PAINT時に発生する。WM_PAINTではcht=NULLになるように小細工
		SetTextColor( hdc, oldtext );
		SetBkColor( hdc, oldback );
#if USE_DT2
		if ( cht )
			delete cht;
#endif
	}

	DeleteTextFonts( );
}

inline void __SetupColor(HDC hdc, COLORREF charcolor, COLORREF backcolor)
{
	SetTextColor( hdc, GET_FORECOLOR(charcolor) );
	SetBkColor( hdc, GET_BACKCOLOR(backcolor) );
}

#define	_SetupColor(snf_flag, sn_flag) \
	__SetupColor(hdc, useuniqfont & snf_flag ? GetColor(sn_flag) : charcolor, backcolor)

// 返り値：見出語部も含めた表示行数
// no : 相対番号
int Squre::DispOneLine( int no, const tchar *word, int rev, Japa &japa, int dispf, CharHT *cht )
{
	__assert(hdc);

#if USE_DISPLEVEL
	{
		int disp_level = GetDispLevel(no);
		if (disp_level>MaxDispLevel){
			LinesList[no+IndexOffset].NumLines = 0;
			if ( LastIndex < no ){
				LastIndex = no;
			}
			return 0;
		}
	//DBW("no=%d %d : %ws", no, GetDispLevel(no), word);
	}
#endif

	
#if SQUONELINE
	if ( ViewOneLine )
		return DispOneLine1( no, word, rev, japa, dispf, cht );
#endif
	RECT _rc;
	bool fDisp = dispf & DF_DISP ? true : false;
	bool fHitTest = !fDisp && cht;
	wa_t attr = (wa_t)(japa.GetAttr() & ~WA_NORMAL);

	int l = 0;	// 項目の表示に必要なYドット数
	int cyMax = 0;

	int offs;
	if ( dispf == DF_NODISP && !cht ){
		// 画像データの場合、ここですべての単語の行数を検索してしまうため、遅くなってしまう。1996.7.18
		// 描画しない&ヒットテストでない場合はオフセットは関係ない？ >> 動作チェックが必要(1996.8.5)
		offs = 0;
	} else {
		offs = GetOffsY( no );
	}
	int hy = GetHomeY( ) + offs - IndexMiOffset;
	CreateTextFonts( );

#ifndef NOSQUCOLOR
	if ( fDisp ){
		SetTextColor( hdc, GET_FORECOLOR( charcolor ) );
		SetBkColor( hdc, GET_BACKCOLOR( backcolor ) );
	}
#endif

	RECT rc;

	SetRect( &rc, 0, hy, 0, 0 );
#if SQU_DISP_NUM
	int offs_left = GetOffs(SN_NUMBER);
	if (IsVisible(SN_NUMBER)){
		rc.left = offs_left;
		DispNumber( rc, (int)(no+IndexOffset+BaseNum), dispf );
		if ( fHitTest ){
			_rc = rc;
			_rc.right = _rc.left + GetWidth(SN_NUMBER);
			_rc.bottom = _rc.top + cyText;
			if ( PtInRect( &_rc, cht->pt ) ){
				cht->item = SN_NUMBER;
			}
		}
		cyMax = max( cyMax, hnFont->cy );
		offs_left += GetWidth(SN_NUMBER);
	}
#endif

	int audio_right = 0x80000000;	// AUDIO表示直前時のrc.right

	// Display items.
	for (int i=0;;i++){
		int sn = ItemView.GetItemIndex(i);
		if (sn==-1)
			break;
		// rc.top   : 前回のitemの上端。改行した場合は最終行の上端
		// rc.right : 前回のitemの右端。改行した場合は最終行の右端
		// cyMax    : 前回のitemの最終行の最大高さ
		switch (sn){
			case SN_ATTR:
				rc.left = max(GetOffs(SN_ATTR), (int)rc.right);
				rc.right = rc.left+GetWidth(SN_ATTR);
				DispAttr( rc, attr, dispf, cht );
				if ( fHitTest ){
					_rc = rc;
					_rc.right = _rc.left + GetWidth(SN_ATTR);
					_rc.bottom = _rc.top + cyAttr;
					if ( PtInRect( &_rc, cht->pt ) ){
						cht->item = SN_ATTR;
					}
				}
				cyMax = max( cyMax, cyAttr );
				break;
			case SN_WORD:
				if ( fDisp ){
#ifndef NOSQUCOLOR
					SetTextColor( hdc, GET_FORECOLOR( useuniqfont & SNF_WORD ? GetColor( SN_WORD ) : charcolor ) );
#endif
					if ( rev ){
						dispf |= DF_REVERSE;
					}
				}

				rc.left = GetOffs( SN_WORD );
				DispWord( rc, word, dispf, cht, &pool.GetHL(no+IndexOffset) );
				cyMax = GetDrawCYMax();

				if ( fDisp && ( !rev
#ifndef SMALL
					|| !fLongBar
#endif
					) ){
					dispf &= ~DF_REVERSE;
				}
#if 0
#ifndef NOSQUCOLOR
				if ( fDisp ){
					SetTextColor( hdc, GET_FORECOLOR( charcolor ) );
					SetBkColor( hdc, GET_BACKCOLOR( backcolor ) );
				}
#endif
#endif
				break;
			case SN_AUDIO:
				// audio file
				rc.right += GAP_ITEM;
				audio_right = rc.right;
				for (int i=0;i<japa.jlinks.get_num();i++){
					if (!japa.jlinks[i].IsAudio()){
						continue;
					}
					rc.left = max(GetOffs(SN_PRON), (int)rc.right)+1;	// +1 gap
					l = DispAudio(rc, dispf, GetWidth(SN_PRON), i, japa.jlinks[i], cht);	//TODO: swidth is temporary.
					cyMax = max(cyMax, l+1);	//  +1 gap
				}
				break;
			case SN_PRON:
				if ( japa.GetPron()[0] ){
#ifndef NOSQUCOLOR
					if ( fDisp ){
						_SetupColor(SNF_PRON, SN_PRON);
					}
#endif
					rc.left = GetOffs(SN_PRON);
					if (PronCR){
						if (rc.right > rc.left){
							if (audio_right==0x80000000	// no AUDIO
								|| (audio_right > rc.left)){	// the left edge of the AUDIO is already overrun.
								// 強制改行
								rc.top += cyMax;
								rc.right = rc.left;
							}
						}
					}
					SetInitLeft(max(0, (int)(rc.right-rc.left)));
					DispPron( rc, japa.GetPron(), dispf, cht
#if USE_DT2
						, &pool.GetHL(no+IndexOffset)
#endif
						);
					cyMax = max(cyMax,GetDrawCYMax());
				}
				break;
			case SN_LEVEL:
				if ( attr & WA_LEVELMASK || dispf & DF_OPAQUE || fHitTest ){
#ifndef NOSQUCOLOR
					if ( fDisp ){
						_SetupColor(SNF_LEVEL, SN_LEVEL);
					}
#endif
					long _right = rc.right;	// backup
					rc.left = max(GetOffs(SN_LEVEL), (int)rc.right);
					int next_sn = ItemView.GetItemIndex(i+1);
					rc.right = GetOffs(next_sn>=0?next_sn:SN_JAPA);
					l = DispLevel( rc, attr & WA_LEVELMASK, dispf, cyMax, fHitTest ? cht : NULL );
					rc.right = max(_right, rc.right);
				}
				break;
		}
	}

#if USE_SLASH	// " / "付き
	if ( ConcatExp ){
		int nLastRight = rc.right;
		if ( japa.GetJapa()[0] ){
#ifndef NOSQUCOLOR
			if ( fDisp ){
				_SetupColor(SNF_JAPA, SN_JAPA);
			}
#endif
			rc.left = GetOffs(SN_JAPA);
			if ( nLastRight > rc.left ){
				rc.top += cyMax;
				cyMax = 0;
			}
			l = DispJapa( rc, japa.GetJapa(), dispf, cht, &pool.GetHL(no+IndexOffset) );
			cyMax = max( cyMax, hjFont->cy );
			nLastRight = GetLastRight();
		}
		if ( IsVisible( SN_EXP ) && japa.GetExp()[0] ){
#ifndef NOSQUCOLOR
			if ( fDisp ){
				_SetupColor(SNF_EXP1, SN_EXP1);
			}
#endif
			rc.left = GetOffs( SN_JAPA );
			int _cy = heFont->cy;
			if ( l ){
				int cyMin = min( hjFont->cy, _cy );
				rc.top += l - cyMin - CY_DRAW_LINEINTERVAL;
				// draw4に改行処理を任せる
			}
			DispExp( rc, StrExpSepa, dispf, nLastRight-rc.left, NULL );
			nLastRight = GetLastRight();
			if ( rc.right != nLastRight ){
				// " / "で改行していた場合
				rc.top += _cy;
			}
#ifndef NOSQUCOLOR
			if ( fDisp ){
				_SetupColor(SNF_EXP1, SN_EXP1);
			}
#endif
			l = DispExp( rc, japa.GetExp(), dispf, nLastRight-rc.left, cht, &pool.GetHL(no+IndexOffset));
			cyMax = max( cyMax, heFont->cy );
		}
	} else
#endif	// ndef SMALL
	{
		// " / "なし
		if ( japa.GetJapa()[0] ){
#ifndef NOSQUCOLOR
			if ( fDisp ){
				_SetupColor(SNF_JAPA, SN_JAPA);
			}
#endif
			rc.left = GetOffs( SN_JAPA );
			if ( rc.right > rc.left ){
				rc.top += cyMax;
				cyMax = 0;
			}
			l = DispJapa( rc, japa.GetJapa(), dispf, cht
#if USE_DT2
				, &pool.GetHL(no+IndexOffset)
#endif
				);
			cyMax = max( cyMax, hjFont->cy );
		}
		if ( IsVisible( SN_EXP ) && japa.GetExp()[0] ){
#ifndef NOSQUCOLOR
			if ( fDisp ){
				_SetupColor(SNF_EXP1, SN_EXP1);
			}
#endif
			rc.left = GetOffs( SN_JAPA );
			if ( rc.right > rc.left ){
				rc.top += max( cyMax, l );
				cyMax = 0;
			}
			l = DispExp( rc, japa.GetExp(), dispf,
#if USE_SLASH
				0,
#endif
				cht
#if USE_DT2
				, &pool.GetHL(no+IndexOffset)
#endif
				);
			cyMax = max( cyMax, heFont->cy );
		}
	}

	l = max( cyMax, l );
#if 1
   // 見栄えを良くするための補正
   if ( cyText > cyAttr ){
	   if ( l > cyAttr ){
			l += cyText - cyAttr;
		}
	}
#endif

#ifdef USE_JLINK

	// オブジェクトの表示
	if ( japa.jlinks.get_num() ){
		if ( FLIconic == FL_ICON )
			dispf |= DF_DRAWICON;	// icon
		else if ( FLIconic == FL_FILENAME )
			dispf |= DF_DRAWNAME;	// filename only
#ifndef NOSQUCOLOR
		//COLORREF oldtext;
		if ( fDisp ){
			//oldtext = SetTextColor( hdc, GET_FORECOLOR( useuniqfont & SNF_EPWING ? GetColor( SN_EPWING ) : charcolor ) );
			_SetupColor(SNF_EPWING, SN_EPWING);
		}
		hgFont->Select();
#endif

		__EnableDDNBegin(CanDispDicName());
		for ( int i=0;i<japa.jlinks.get_num();i++ ){
			// ２番目以降のオブジェクトの場合は、右側に表示できるかどうか計算する
			// ファイルリンクで内容表示、ファイル名表示の場合は並べない
			// EPWINGも並べない
			JLink &jl = japa.jlinks[i];
			if (jl.IsAudio()){
				continue;
			}
			if ( i != 0 && ( jl.GetType() != JL_FILE || FLIconic == FL_ICON )
#ifdef EPWING
				&& jl.GetType() != JL_EPWING
#endif
				)
			{
				RECT _rc = rc;
				_rc.left = rc.right;
				_rc.right = GetLX();
				jl.SetParent((TWinControl*)View);
				jl.Draw( *hgFont, _rc, dispf & ~DF_DISP);
				if ( _rc.right < GetLX() ){
					// 表示可能
					rc = _rc;
					goto jmp1;
				}
			}
			rc.left = GetOffs( SN_JAPA );
			// 表示開始Y座標の更新
			if ( rc.right > rc.left ){
				rc.top += l;
				l = 0;
#if USE_FASTDISP
				if ( (fDisp || cht) && (rc.top > GetLY()) )
				{
					// 高速化のため表示範囲を超えた場合は中止(1998.12.23)
					break;
				}
#endif
			}
	jmp1:
			DispObject(no, i, jl, dispf, rc, l, cht);
		}
		__EnableDDNEnd();
#ifndef NOSQUCOLOR
		//if ( fDisp )
		//SetTextColor( hdc, oldtext );
#endif
	}
#endif  // USE_JLINK

	DeleteTextFonts( );

#if USE_DT2
	if ( !cht ){
		// 絶対座標からオフセット座標に変換
		THyperLinks &hls = pool.GetHL(no+IndexOffset);
		for ( int i=0;i<hls.get_num();i++ ){
			hls[i].area.rect.top -= hy;
			hls[i].area.rect.bottom -= hy;
//			RECT r = pool.GetHL(no+IndexOffset)[i].area.rect;
//			DBW("pool.GetHL(no+IndexOffset)[i].area.rect=%d %d %d %d", r.left, r.top, r.right, r.bottom );
		}
	}
#endif

	if ( fHitTest ){	// ヒットテストの場合
		return 0;	// 返り値に意味はない
	}


	// 表示行数の計算

	l += rc.top - hy;
	if ( l < cyText )
		l = cyText;

	// ウィンドウの最下行はfDispのときは記憶しない
	// 理由：最下行データが大きい場合、
	//       fDispのときは正確な行数を返さないことに
	//       したため(1999.1.4)、!fDispのときのみ
	// 1999.5.3
	// (rc.top < LY)は外した
	// 最下行を再計算させないことにより高速化させていたつもりだろうが、
	// そうなると、最下行のドット数が不正確になってしまう
	// 2007.12.2
	// !fDispを外した
	// File Link Viewでfittingを採用したため、
	// 再計算なしでresizeすることがあるため。
//	if ( !fDisp /* || (rc.top < LY) */ )
	{
		LinesList[no+IndexOffset].NumLines = l;
	}

	if ( fDisp ){
		if ( offs + l - IndexMiOffset + ItemView.LineInterval <= MaxLines || no == 0 ){
			if ( LastIndex < no ){
				LastIndex = no;
			}
		}
	}
	return l;
}

#if SQUONELINE
// １行表示
int Squre::DispOneLine1( int no, const tchar *word, int rev, Japa &japa, int dispf, CharHT *cht )
{
	RECT _rc;
	bool fDisp = dispf & DF_DISP ? true : false;
	bool fHitTest = !fDisp && cht;
	wa_t attr = (wa_t)(japa.GetAttr() & ~WA_NORMAL);

	int l;	// 項目の表示に必要なYドット数
	int cyMax = cyText;

	int offs;
	if ( dispf == DF_NODISP && !cht ){
		// 画像データの場合、ここですべての単語の行数を検索してしまうため、遅くなってしまう。1996.7.18
		// 描画しない&ヒットテストでない場合はオフセットは関係ない？ >> 動作チェックが必要(1996.8.5)
		offs = 0;
	} else {
		offs = GetOffsY( no );
	}
	int hy = GetHomeY( ) + offs - IndexMiOffset;
	CreateTextFonts( );

//	dispf &= ~DF_PAINT;	// いらない？

#ifndef NOSQUCOLOR
	if ( fDisp ){
		SetTextColor( hdc, GET_FORECOLOR( charcolor ) );
		SetBkColor( hdc, GET_BACKCOLOR( backcolor ) );
	}
#endif

	RECT rc;
	int lines = GetLines(no);
	SetRect( &rc, GetOffs( SN_NUMBER ), hy, GetLX(), lines ? hy+lines : GetLY() );
//	DBW("no:%d rev=%d dispf=%X hy=%d offs=%d (%d,%d,%d,%d)",
//		no,rev,dispf,hy,offs,rc.left,rc.top,rc.right,rc.bottom);
#if SQU_DISP_NUM
	if ( IsVisible( SN_NUMBER ) ){
		DispNumber( rc, (int)(no+IndexOffset+BaseNum), dispf );
		if ( fHitTest ){
			_rc = rc;
			_rc.right = _rc.left + GetOffs( SN_NUMBER );
			_rc.bottom = _rc.top + cyText;
			if ( PtInRect( &_rc, cht->pt ) ){
				cht->item = SN_NUMBER;
			}
		}
		cyMax = max( cyMax, hnFont->cy );
	}
#endif

	if ( IsVisible( SN_ATTR ) ){
		rc.left = GetOffs( SN_ATTR );
		rc.right = rc.left+GetWidth(SN_ATTR);
		DispAttr( rc, attr, dispf, cht );
		if ( fHitTest ){
			_rc = rc;
			_rc.right = _rc.left + GetOffs( SN_ATTR );
			_rc.bottom = _rc.top + cyAttr;
			if ( PtInRect( &_rc, cht->pt ) ){
				cht->item = SN_ATTR;
			}
		}
		cyMax = max( cyMax, cyAttr );
	}

	if ( fDisp ){
#ifndef NOSQUCOLOR
		SetTextColor( hdc, GET_FORECOLOR( useuniqfont & SNF_WORD ? GetColor( SN_WORD ) : charcolor ) );
#endif
		if ( rev ){
			dispf |= DF_REVERSE;
		}
	}

	rc.right = GetLX();
	if ( fDisp ){
		rc.left = GetOffs( SN_WORD );
		hwFont->Select();
		word = find_cword_pos(word);	// 表示用単語のみ
		ExtTextOut( hdc, rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, word, _tcslen(word), NULL );
	}
	cyMax = max( cyMax, hwFont->cy );

	if ( fDisp && ( !rev
#ifndef SMALL
		|| !fLongBar
#endif
		) ){
		dispf &= ~DF_REVERSE;
	}

	if ( japa.GetPron()[0] && IsVisible( SN_PRON ) ){
#ifndef NOSQUCOLOR
		if ( fDisp ){
			SetTextColor( hdc, GET_FORECOLOR( useuniqfont & SNF_PRON ? GetColor( SN_PRON ) : charcolor ) );
			SetBkColor( hdc, GET_BACKCOLOR( backcolor ) );
		}
#endif

		rc.left = GetOffs(SN_PRON);
		ExtTextOut( hdc, rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, japa.GetPron(), _tcslen(japa.GetPron()), NULL );
		cyMax = max( cyMax, hpFont->cy );
	}

#ifndef NOSQUCOLOR
	if ( fDisp ){
		SetTextColor( hdc, GET_FORECOLOR( charcolor ) );
		SetBkColor( hdc, GET_BACKCOLOR( backcolor ) );
	}
#endif

	if ( IsVisible( SN_LEVEL ) ){
		// レベル表示は最適化してあるのでまだ不安
		if ( attr & WA_LEVELMASK || dispf & DF_OPAQUE || fHitTest ){
			rc.left = GetOffs( SN_LEVEL );
			l = DispLevel( rc, attr & WA_LEVELMASK, dispf, cyMax, fHitTest ? cht : NULL );
		}
	}

	SIZE sz;

#if 1	// " / "付き
	int nLastRight = GetOffs( SN_JAPA );
	if ( japa.GetJapa()[0] ){
		if ( fDisp ){
#ifndef NOSQUCOLOR
			SetTextColor( hdc, GET_FORECOLOR( useuniqfont & SNF_JAPA ? GetColor( SN_JAPA ) : charcolor ) );
#endif
            rc.left = GetOffs( SN_JAPA );
            hjFont->Select();
            ExtTextOut( hdc, rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, japa.GetJapa(), _tcslen(japa.GetJapa()), NULL );
		}
		cyMax = max( cyMax, hjFont->cy );
		GetTextExtentPoint32( *hjFont, japa.GetJapa(), _tcslen(japa.GetJapa()), &sz );
		nLastRight = rc.left + sz.cx;
	}
	if ( IsVisible( SN_EXP ) && japa.GetExp()[0] ){
		if ( fDisp ){
#ifndef NOSQUCOLOR
			SetTextColor( hdc, GET_FORECOLOR( useuniqfont & SNF_EXP1 ? GetColor( SN_EXP1 ) : charcolor ) );
#endif
            rc.left = max( GetOffs( SN_JAPA ), nLastRight );
            if ( rc.left < GetLX() ){
				heFont->Select();
				ExtTextOut( hdc, rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, _T(" / "), 3, NULL );
				GetTextExtentPoint32( *hjFont, japa.GetJapa(), _tcslen(japa.GetJapa()), &sz );
                rc.left += sz.cx;
				if ( rc.left < GetLX() ){
            		ExtTextOut( hdc, rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, japa.GetExp(), _tcslen(japa.GetExp()), NULL );
                }
            }
		}
		cyMax = max( cyMax, heFont->cy );
	}
#else	// " / "なし
	if ( japa.GetJapa()[0] ){
#ifndef NOSQUCOLOR
		if ( fDisp ){
			SetTextColor( hdc, GET_FORECOLOR( useuniqfont & SNF_JAPA ? GetColor( SN_JAPA ) : charcolor ) );
		}
#endif
		rc.left = GetOffs( SN_JAPA );
		l = DispJapa( rc, japa.GetJapa(), dispf, cht
#if USE_DT2
        	, &pool.GetHL(no+IndexOffset)
#endif
            );
		cyMax = max( cyMax, hjFont->cy );
	}
	if ( GetView( SN_EXP ) && japa.GetExp()[0] ){
#ifndef NOSQUCOLOR
		if ( fDisp ){
			SetTextColor( hdc, GET_FORECOLOR( useuniqfont & SNF_EXP1 ? GetColor( SN_EXP1 ) : charcolor ) );
		}
#endif
		rc.left = GetOffs( SN_JAPA );
		l = DispExp( rc, japa.GetExp(), dispf, cht
#if USE_DT2
        	, &pool.GetHL(no+IndexOffset)
#endif
            );
		cyMax = max( cyMax, heFont->cy );
	}
#endif

	DeleteTextFonts( );

#if USE_DT2
    if ( !cht ){
        // 絶対座標からオフセット座標に変換
		THyperLinks &hls = pool.GetHL(no+IndexOffset);
		for ( int i=0;i<hls.get_num();i++ ){
			hls[i].area.rect.top -= hy;
			hls[i].area.rect.bottom -= hy;
//			RECT r = pool.GetHL(no+IndexOffset)[i].area.rect;
//			DBW("pool.GetHL(no+IndexOffset)[i].area.rect=%d %d %d %d", r.left, r.top, r.right, r.bottom );
		}
	}
#endif

	if ( fHitTest ){	// ヒットテストの場合
		return 0;	// 返り値に意味はない
	}


	// 表示行数の計算
	l = cyMax;

	// ウィンドウの最下行はfDispのときは記憶しない
	// 理由：最下行データが大きい場合、
	//       fDispのときは正確な行数を返さないことに
	//       したため(1999.1.4)、!fDispのときのみ
	// 1999.5.3
	// (rc.top < LY)は外した
	// 最下行を再計算させないことにより高速化させていたつもりだろうが、
	// そうなると、最下行のドット数が不正確になってしまう
	// 2007.12.2
	// !fDispを外した
	// File Link Viewでfittingを採用したため、
	// 再計算なしでresizeすることがあるため。
//	if ( !fDisp /* || (rc.top < LY) */ )
	{
		LinesList[no+IndexOffset].NumLines = l;
	}

	if ( fDisp ){
		if ( offs + l - IndexMiOffset + ItemView.LineInterval <= MaxLines || no == 0 ){
			if ( LastIndex < no ){
				LastIndex = no;
			}
		}
	}
	return l;
}
#endif

void Squre::DispObject(int no, int i, JLink &jl, int dispf, RECT &rc, int &l, CharHT *cht)
{
	int select = ( (dispf&DF_DISP) && cht ) ? cht->item - SN_OBJECT : -1;

	rc.right = GetLX();
//	rc.bottom = rc.top + GetLY();			// 代わりにこれを追加
	rc.bottom = GetLY();	// 2008.7.29 前行の方法ではmicro scrollがうまくないはず？？
	int _dispf = dispf;
	if ( cht ){
		if (cht->item==i+SN_OBJECT){
			cht->cpos = *(POINT*)&rc;	// 左上座標を返す
		}
		if ( select != i ){	// PINPOINTの場合、PINPOINT以外は非表示にする
			if ( _dispf & DF_PINPOINT )
				_dispf &= ~DF_DISP;
		}
	}
	// -- 実際の表示処理 --
	// PINPOINTの場合は、DF_SELECTEDをセットしていないと選択表示されない！
	bool fAct = jl.IsActivating();	// In-Place Activatingのときは描画しない
	do {
		EnphTextVec et;
#ifdef EPWING
		if ( jl.GetType() == JL_EPWING ){
			MakeEnph( et, pool.GetHL(no+IndexOffset), HLI_EPWING+i );
		}
#endif
		jl.SetParent((TWinControl*)View);
		l = max( l,
			jl.Draw(
				*hgFont,
				rc,
				( (rc.top > GetLY() || fAct) ? ( _dispf & ~DF_DISP ) : _dispf ) | ( select == i && !(dispf&DF_ONLYUNDERLINE) ? DF_SELECTED : 0 ),
				!cht ? &et : NULL
				)
			);
	} while(0);
	const bool fHitTest = (!(dispf & DF_DISP)) && cht;
	if ( fHitTest ){
		if ( PtInRect( &rc, cht->pt ) ){
			cht->item = SN_OBJECT + i;
		}
	}
	if ( select == i ){
		MouseCap->SetCapture(*(POINT*)&rc);	// ここでセット（苦肉の策）
	}
}

#if 0
void Squre::DispState( int y )
{
	static tchar Str[] = _T("-- Search is proceeding --");
	TextOut( hdc, 0, y, Str, tsizeof( Str ) );
}
#endif

#if USE_DT2
// _etを渡すと、追加モード
// Get the match information.
void Squre::GetHitPosition( EnphTextVec &et, const tchar *text, int len, bool useAndSearch )
{
	if (!ss.underline)
		return;	// cannot underline

	SearchCommonStruct scs( &ss, Dic.GetLangProc(), useAndSearch );
	if ( scs.Setup(false) != 1 ){	// 正規表現検索の場合、すでにコンパイルしていることが前提！！
		// error					// そしてなおかつ、正規表現オブジェクトはSrchStatが所有！！
		return;
	}

	if ( scs.fRegular ){
		if ( scs.Search(text) ){
			for (int i=0;i<scs.matches.get_num();i++){
				EnphText item;
				item.start = scs.matches[i].loc;	// charactor count
				item.end = scs.matches[i].loc + scs.matches[i].len;
				et.push_back(item);
			}
			et.Sort();
		}
	} else {
		int index_offset = 0;
		do {
			if ( scs.Search(text+index_offset) ){
				EnphText item;
				item.start = index_offset+ scs.pos;	// charactor count
				item.end = index_offset+ scs.pos + _tcslen(scs.sword);
				if ( item.end > len ){
					item.end = len;
					if ( item.start < item.end ){
						et.push_back(item);
					}
					break;
				}
				index_offset = item.end;
				et.push_back(item);
			} else {
				if ( !et.size() ){
					return;
				}
				break;
			}
		} while ( index_offset < len /* && et.size() < MAXENPHTEXT - 1*/ );
	}
}
#endif

