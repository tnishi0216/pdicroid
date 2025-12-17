#include "pdclass.h"
#pragma	hdrstop
#include	"id.h"
#include	"winsqu.h"
#include	"pdicw.h"
#include "SquView.h"
#include "SquItemView.h"
#include "WinSquUI.h"
#include "MouseCapture.h"
#include "SrchMed.h"

#define	SCRIMP		1		// scrollの改善

#if SCRIMP
#define	_ScrollWindow(hwnd, dx, dy)	View->Scroll(dx, dy);
#define	_ScrollWindowR(hwnd, dx, dy, repaint)	View->Scroll(dx, dy, repaint)
#else
#define	_ScrollWindow(hwnd, dx, dy)	View->Scroll(dx, dy, true);
#define	_ScrollWindowR(hwnd, dx, dy, repaint)	View->Scroll(dx, dy, true)
#endif

#define	SCROLL_UNIT	4	// cyTextの倍数

#undef DBX
#if 0
#define	DBX	DBW
#else
inline void DBX(...)	{}
#endif

// Scroll control //
int Squre::GetVPos()
{
	return View->GetScrollPos(SB_VERT);
}

void Squre::SetVPos( int pos, bool fRepaint)
{
	View->SetScrollPos(SB_VERT, pos, fRepaint);
}

void Squre::SetVRange( int iMax, bool fRepaint )
{
#if 0	// removed 2008.7.27
	// 再入防止：Win9x系で落ちるのはこれが原因かも？
#ifdef _DEBUG
	static bool in = false;
	__assert(!in);
	if (in) return;
	in = true;
#endif
#endif

	View->SetScrollRange( SB_VERT, 0, iMax <= 0 ? 1 : iMax, fRepaint );
	LastVRange = get_num();

#if 0
#ifdef _DEBUG
	in = false;
#endif
#endif
}
#if 0
// 検索中のためのVRange
void Squre::SetVRangeSearching()
{
	int iMax = get_num() ? get_num() - 1 : 0;
	if (IndexOffset==0 && iMax<=LastIndex){
		iMax = 0;	// no scroll
	}
	SetVRange(iMax, false);
}
#endif
void Squre::SetHPos( int pos, bool fRepaint)
{
	View->SetScrollPos(SB_HORZ, pos, fRepaint );
}

void Squre::SetHRange( int iMax, bool fRepaint)
		{ View->SetScrollRange( SB_HORZ, 0, iMax, fRepaint ); }
void Squre::EnableVScroll( bool f )
		{ View->ShowScrollBar( SB_VERT, f ); }

void Squre::SetVRange2( bool fRepaint )
{
#if 1
	// １つ以上検索されていて、前方にまだ検索すべきデータが無いとき
	// 最適化を行なう
	int range = get_num()-1;
	if ( range > 0 && !( ss.GetSearchState() & SS_FWD ) ){
		int i = GetLastPageOffset( );
		if ( i != -1 ){
			range = i;
		}
	}
#else	// 工事中 2008.6.3 簡単にできるか？
	int range = get_num() - LastIndex;
	if (range<0)
		range = 0;
#endif
	//DBW("fRepaint=%d range=%d LastIndex=%d IndexOffset=%d", fRepaint, range, LastIndex, IndexOffset);
	SetVRange( range, fRepaint );
}

void Squre::SetUpDown(bool f)
{
	if (f){
		ForceUpdate = false;
	}
}

// reqoffs : absolute index
// return value:
//	true - need to search
//	false - no search
bool Squre::Request( int reqoffs, bool fBack )
{
	//DBW("Request: %d,%d,%d", fBack, reqoffs, get_num());
	if ( fBack ){
		// 後方検索
		if ( ( ss.GetSearchState() & SS_BWD ) && reqoffs <= get_num()/2 ){
			int reqnum = MAX_NDISP/2 - reqoffs + MAX_NDISP/8;
			ReSearch( SrchMed->GetTopSearchWord(), true, reqnum );
			return true;
		}
	} else {
		// 前方検索
		// 強制的に検索できる個数はoffset>=0となるまでである。
		// （Request()が短い間に何度も呼ばれると強制的検索個数が増加するため）
		if ( ( ss.GetSearchState() & SS_FWD ) && reqoffs  >= get_num()/2 ){

			// reqoffsが中心になるように要求
			int reqnum = reqoffs - MAX_NDISP/2 + MAX_NDISP/8;
			ReSearch( SrchMed->GetLastSearchWord(), false, reqnum );
			return true;
		}
	}
	return false;
}

#if NEWINCSRCH
void Squre::StartBackward()
{
	Request( 0, true );
	if ( IsSearching() ){
		SearchProc( true );
	}
}
#endif

// 返り値は相対番号
//	次ページがない場合は、0を返す
int Squre::_GetNextPage( )
{
#if 1
	int i = GetIndexFromLines();
	if (i==IndexOffset)	// １単語が１画面を超えていた場合
		i++;
	if (i>=get_num()){
		return 0;	// 画面内に表示できる
	}
	return i-IndexOffset;
#else	// old code 2009.4.7
	int l = 0;
	int i;
	for ( i=0;i<get_num()-IndexOffset;i++ ){
		RecalcLine( i + IndexOffset );
		l += GetLines(i) + ItemView.LineInterval;
		if (l >= MaxLines){
			break;
		}
	}
	if ( i == 0 )	// 1995.6.29 １単語が１画面を超えていた場合
		i++;
	if ( i + IndexOffset >= get_num() ){
		return 0;
	}
	return i;
#endif
}

// 返り値は相対番号
//	次ページがない場合は、0を返す
int Squre::GetNextPage( )
{
	int i = _GetNextPage( );
	if ( !i )
		return 0;
#if 0	// 1995.5.29 最終頁のスクロール方法変更
	int j = GetLastPageOffset( );
	if ( j != -1 && i+IndexOffset > j )
		return j-IndexOffset;
#else
	if ( !IsSearching( ) ){
		// 最終ページかどうかの判断
		if ( get_num() <= IndexOffset + (LastIndex<<1) ){
			IndexOffset += i;
			if ( !_GetNextPage( ) ){
				// これ以上scrollできない
				IndexOffset -= i;
				int j = GetLastPageOffset( );
				if ( j != -1 && i+IndexOffset > j )
					return j-IndexOffset;
				else
					return i;
			}
			IndexOffset -= i;
		}
	}

#endif
	return i;
}

void Squre::NextPage( )
{
	int n = GetNextPage();
	if ( n ){
		IndexMiOffset = 0;
		IndexOffset += n;
		MouseCap->OffsetIndex(n);
		Request( IndexOffset, false );
		SetVPos( IndexOffset );
		Invalidate( );
		if ( cury + IndexOffset >= get_num() ){
			cury = get_num() - IndexOffset - 1;
			if ( cury < 0 )
				cury = -1;
		}
#if 0
		if ( cury >= 1 ){
			// 反転バーが画面からはみ出していないかチェックする
			// 次ページが遅いオブジェクトのとき時間がかかっちゃうよなぁ・・・
			int l = 0;
			int i;
			for ( i=0;i<get_num()-IndexOffset;i++ ){
				if ( i >= cury ) break;
				RecalcLine( i + IndexOffset );
				l += GetLines(i) + ItemView.LineInterval;
				if ( l >= MaxLines ){
					if ( cury >= i ){
						if ( i == 0 )
							cury = 0;
						else
							cury = i-1;
					}
					break;
				}
			}
		}
#endif
		SetUpDown(true);
	}
}

// 要：GetDC()
void Squre::EndPage( )
{
	int l = 0;
	int i;
	for ( i=get_num()-1;i>=0;i-- ){
		RecalcLine( i );
		l += LinesList[i].NumLines + ItemView.LineInterval;
		if ( l > MaxLines ){
			if ( i != get_num()-1 )
				i++;	// 微調整
			break;
		}
	}
	if ( i <= IndexOffset ){
		// 念のため？
		if ( cury != get_num() - IndexOffset - 1 ){
			dispStar( get_num() - IndexOffset - 1 );
			SetUpDown(true);
		}
		return;
	}

//	StopSearch( );
	IndexMiOffset = 0;
	MouseCap->OffsetIndex(i - IndexOffset);
	IndexOffset = i;
	cury = get_num() - 1 - IndexOffset;
	Request( IndexOffset, false );
	SetVPos( IndexOffset );
	Invalidate( );
	SetUpDown(true);
}

void Squre::PrevPage( )
{
#if NEWINCSRCH
	if ( !IndexOffset && ss.IsIncSearch() ){
		StartBackward();
	}
#endif

	if ( IndexOffset == 0 ){
		return;
	}
	int sum = 0;
	int i;
	for ( i=IndexOffset-1;i>0;i-- ){
		RecalcLine( i );
		sum += LinesList[ i ].NumLines + ItemView.LineInterval;
		if ( sum > MaxLines ){
			i++;
			break;
		}
	}
	if ( i == IndexOffset ){	// １単語が１画面を超えていた場合
		i--;
		if ( cury != -1 ) cury = 0;
	}
	MouseCap->OffsetIndex(i - IndexOffset);
	IndexMiOffset = 0;
	IndexOffset = i;
	Request( IndexOffset, true );
	SetVPos( IndexOffset );
	Invalidate( );
//	UpdateWindow();
	SetUpDown(true);
}

// 先頭ページへ
// 要：GetDC()
void Squre::HomePage( )
{
	if ( IndexOffset == 0 ){
		if ( cury != 0 ){
			dispStar( 0 );
		}
		return;
	}
//	StopSearch( );
	MouseCap->OffsetIndex(IndexOffset);
	IndexMiOffset = 0;
	IndexOffset = 0;
	cury = 0;
	Request( 0, true );
	SetVPos( 0 );
	Invalidate( );
	SetUpDown(true);
}

void Squre::KeyScrollUp( )
{
	bool link_ui = true;
	GetDC();
	if (cury==0){	// <- 2014.9.27 added.
		// MicroScrollであったときは、一度MicroScroll=0の状態を表示してからに変更
		if ( IndexMiOffset ){
			MicroScrollUp( IndexMiOffset );
			dispStar(cury, link_ui);
			goto jexit;
		}
	}
//	IndexMiOffset = 0;
	DBX( "up" );
	if (cury == 0){
		ScrollUp( true );
	} else
	if ( cury == -1){
		ClsRegion( );
		int n = GetNextPage( );
		if ( n == 0 ){
			if (LastIndex<0) goto jexit;	// no data in square
			n = LastIndex;
//			IndexOffset = 0;
			Request( IndexOffset, true );
			SetVPos( IndexOffset );
		}
		dispStar( n - 1, link_ui );
		SetUpDown(true);
	} else {
		ClsRegion( );
		dispStar(cury-1, link_ui);
		SetUpDown(true);
	}
#if !defined(SMALL) && defined(USE_JLINK)
	if ( AutoViewMode ) AutoView();
#endif
jexit:;
	ReleaseDC();
}

int Squre::LineScrollUp()
{
	//DBW("cury=%d %d %d %d", cury, LastIndex, IndexOffset, IndexMiOffset);
	int offs = cyText * SCROLL_UNIT;
	if (MicroScrollUp(offs)==0){
		GetDC();
		CloseAutoLink();
#if SCRIMP
		bool star_erased = false;
#endif
		if (IndexMiOffset<offs){
			if (IndexOffset>0){
				ClsRegion();
#if SCRIMP
				_dispStar(cury, 0);
				star_erased = true;
#endif
				IndexOffset--;
#if USE_FASTDISP
				// wsdisp2.cppで画面の最終行に一度でも描画されたことのある見出し語は、表示行数が正しくない場合がある
				if (cury>0)
					RecalcLineForce(IndexOffset+cury);
#endif
				RecalcLineForce(IndexOffset);
				IndexMiOffset += GetLines(0) + ItemView.LineInterval;
				Request( IndexOffset, true );
				SetVPos( IndexOffset );
			}
		}
		_MicroScrollUp(offs);
#if SCRIMP
		if (star_erased)
			_dispStar(cury, 1);
#endif
		ReleaseDC();
		SetUpDown(true);
	}
	//DBW("cury=%d %d %d %d", cury, LastIndex, IndexOffset, IndexMiOffset);
	return 1;
}
int Squre::LineScrollDown()
{
	if (get_num()==0)
		return 0;

	//DBW("cury=%d %d %d %d", cury, LastIndex, IndexOffset, IndexMiOffset);
	int offs = cyText * SCROLL_UNIT;
	if (MicroScrollDown(offs)==0){
		// 補正
#if 0	// すぐにcursorを移動するversion
		// curyを0にする処理が必要
		int lines = GetLines(cury>=0?cury:0);
		IndexMiOffset -= lines;	// may be negative value
		if (cury>=0) cury++;
		IndexOffset++;
#else
		GetDC();
		CloseAutoLink();

		// - cury直前までの表示行数を求める
		int cury_lines = 0;
		for (int i=0;i<cury;i++){
			int l = GetLines(i) + ItemView.LineInterval;
			if (l==0){
				RecalcLine(i+IndexOffset);
				l = GetLines(i) + ItemView.LineInterval;
			}
			cury_lines += l;
		}
		
		// 現在の表示行数を求め、
		// - すべて表示可能ならreturn
		// - 可能でなければmicro scrollを実施
		int lines = -IndexMiOffset;
		int ai = IndexOffset+(cury>=0?cury:0);
		int si = ai;	// start index
		
		for (;;){
			int l = GetAbsLines(ai);
			if (l==0)
				RecalcLines(ai, ai);	//TODO: lを更新しなくていいの？ 2014.10.20
			lines += l;
			if (ai-IndexOffset<cury)
				cury_lines += l;
			if (lines>GetLY())
				break;	// 表示しきれない
			if (ai+1>=get_num()){
				ReleaseDC();
				return 0;	// すべて表示されている
			}
			ai++;
		}
		// 一単語分調整
		if (IndexMiOffset+offs>=cury_lines+GetAbsLines(si)){
			ClsRegion();
#if SCRIMP
			_dispStar(cury, 0);
#endif
			//do {
				IndexMiOffset -= GetLines(0) + ItemView.LineInterval;
				IndexOffset++;
				//cury--;
			//} while (IndexMiOffset>=GetLines(0));
#if SCRIMP
			_dispStar(cury, 1);
#endif
			Request( IndexOffset, false );
			SetVPos( IndexOffset );
		}
		ReleaseDC();
#endif
		_MicroScrollDown(offs);
		SetUpDown(true);
	}
	//DBW("cury=%d %d %d %d", cury, LastIndex, IndexOffset, IndexMiOffset);
	return 1;
}

//#define	MSCR_UNIT	(cyText*4)	// 微小スクロール単位
#define	MSCR_UNIT	(GetLY()>>2)

// 微少スクロール処理
int Squre::MicroScrollUp( int offs )
{
	if ( !IndexMiOffset )
		return 0;
	return _MicroScrollUp(offs);
}
// no limited scroll
int Squre::_MicroScrollUp(int offs)
{
	if (!offs)
		offs = MSCR_UNIT;
	int mo = IndexMiOffset;
	IndexMiOffset -= offs;
	if ( IndexMiOffset < 0 )
		IndexMiOffset = 0;
	int diff = mo-IndexMiOffset;

	// modify LastIndex
	int abs = GetIndexFromLines(MaxLines-diff);
	if (abs>=get_num())
		abs--;
	LastIndex = abs-IndexOffset;
	if (LastIndex<cury) LastIndex = cury;	// 2015.10.24 帳尻あわせ

	// scroll window
	_ScrollWindow(HWindow, 0, diff);
//	UpdateWindow( );
	ScrolledWindow();
	return 1;
}

int Squre::ScrollUp(bool fClsRegion)
{
	DBX("ScrollUp:%d",fClsRegion);
#if NEWINCSRCH
	if ( IndexOffset == 0 && ss.IsIncSearch() ){
#if 0
		int n = get_num();
#endif
		StartBackward();
#if 0
		if ( n != get_num() ){	// まだ前にデータがある場合
			IndexOffset++;
			BaseNum--;
		}
#endif
	}
#endif

	if ( IndexMiOffset ){
		MicroScrollUp( );
		return 0;
	}

	if ( !IndexOffset ){
		return 0;
	}
#ifndef SMALL
	prevpt.x = prevpt.y = 0;	// スクロールしてもunderlineを出すため@Hyperlink
#endif
#if USE_DT2
	CloseAutoLink();
#endif

	IndexMiOffset = 0;

	GetDC( );
	if ( fClsRegion )
		ClsRegion( );
	if ( cury != -1 ){
		_dispStar( cury, 0 );
//		cury++;
	}
	IndexOffset--;
#if 1	// 2009.4.7 new code
	int i = GetIndexFromLines();
#else	// old code, to be deleted.
	int sum = 0;
	int i;
	for ( i=IndexOffset;i<get_num();i++ ){
		RecalcLine( i );
		sum += LinesList[ i ].NumLines + ItemView.LineInterval;
		if ( sum > MaxLines )
			break;
	}
#endif
	LastIndex = i-1-IndexOffset;
	if ( LastIndex < 0 )
		LastIndex = 0;

	_ScrollWindow( HWindow, 0, LinesList[IndexOffset].NumLines+ItemView.LineInterval );
	MouseCap->OffsetIndex(-1);
	if (cury!=-1){
		if (UIMain) UIMain->SetWordText(GetWord(cury));
		_dispStar( cury, 1 );
	}
	Request( IndexOffset, true );
	SetVPos( IndexOffset );
	SetUpDown(true);
	ReleaseDC( );
	ScrolledWindow();
	return 1;
}

// 微少スクロールダウン
int Squre::MicroScrollDown( int offs )
{
	//dbw("MiOffset=%d %d : %d %d %d", IndexMiOffset, GetLY(), IndexOffset, LinesList[ IndexOffset ].NumLines, cury);
	//Note: cury==0 を前提にしてある。もしcury!=0である場合、IndexOffsetからcuryまでのライン数を計算しなければならない。
	//TODO: 見出し語が変わってもcuryを変更せずにscrollさせるため、poolの一番下の見出し語以降が取得されず、空白がscrollされ続けてしまう
	//      本来この条件文は不要のはず？
	//      cury+1のtopが表示エリアのtop座標より上(minus)に行ったらcuryを更新してはどうだろうか？
#if 0	// 2015.10.15 新方式
#else
	if ( IndexMiOffset + GetLY() >= LinesList[ IndexOffset + (cury>=0?cury:0) ].NumLines )
		return 0;	// IndexOffsetの項目は十分表示可能領域にある
#endif
	return _MicroScrollDown(offs);
}
int Squre::_MicroScrollDown(int offs, bool repaint)
{
	if (!offs)
		offs = MSCR_UNIT;
	_ScrollWindowR( HWindow, 0, -offs, repaint );
	IndexMiOffset += offs;
//	UpdateWindow( );
	ScrolledWindow();
	return 1;
}

// 先頭の次の見出語をウィンドウの先頭にするようにスクロール
// 0 : スクロール無し
// 1 : 微少スクロール
// 2 : SetWordText()が必要ないスクロール
// 3 : SetWordText()が必要なスクロール
// stopatbottom : もし最下行に達した場合は無視する(キーボードでのスクロール)
int Squre::ScrollDown1( bool stopatbottom )
{
	if (get_num()==0)
		return 0;

	DBX("ScrollDown1:%d",stopatbottom);
	// 微小スクロール
	// １ウィンドウに１単語しか表示されていない場合のみ
	if ( LastIndex == 0 ){
		RecalcLine( IndexOffset );
		if ( IndexMiOffset + GetLY() < LinesList[ IndexOffset ].NumLines ){
			MicroScrollDown( );
			return 1;
		}
		if ( stopatbottom )
			return 0;
	}

	if ( LastIndex + IndexOffset >= get_num() - 1 ){
		return 0;
	}

	IndexMiOffset = 0;

#ifndef SMALL
	prevpt.x = prevpt.y = 0;	// スクロールしてもunderlineを出すため@Hyperlink
#endif
#if USE_DT2
	CloseAutoLink();
#endif

	_ScrollWindow( HWindow, 0, -( LinesList[IndexOffset].NumLines + ItemView.LineInterval ) );
	IndexOffset++;
	LastIndex--;
	if ( cury != -1 ){
		cury--;
		if ( cury < 0 )
			cury = 0;
	}
	MouseCap->OffsetIndex(1);
	Request( IndexOffset, false );
	SetVPos( IndexOffset );
	SetUpDown(true);
	ScrolledWindow();
	if ( cury == 0 ){	// 先頭行は消えてしまう
		GetDC( );
		_dispStar( cury, 1 );
		ReleaseDC( );
		return 3;
	}
	return 2;
}

// lastindexの次の見出語がちゃんと表示されるようにスクロール
// 微少スクロールはキャンセルされる
int Squre::ScrollDown(bool fClsRegion, int down )
{
	if ( LastIndex + IndexOffset >= get_num() - 1 ){
		return 0;
	}

	if ( IndexOffset + cury + 1 < get_num() )
		RecalcLine( IndexOffset + cury + 1 );
	else return 0;	// 1998.7.11
	int l = LinesList[ IndexOffset + cury + 1 ].NumLines;	// 次に表示するデータのドット数

	GetDC();
	if ( fClsRegion )
		ClsRegion( );
	_dispStar( cury, 0 );

	// 次に表示するデータのうち、すでに表示されているドット数を求め、
	// 未表示のドット数を l に求める
	int LastY = GetOffsY( LastIndex + 1 );	// 次に表示するデータのY座標
	int i;
	if ( LastY > GetLY() ){
		// lastindexだけで１ウィンドウ表示している
		// scrollはしない
		// 1999.1.4
		i = 0;
		IndexOffset++;
		Invalidate();
	} else {
		int dl = ( GetLY() - LastY + IndexMiOffset )/cyText;	// 既に表示されているドット数
		l -= dl;

		// スクロールする行数の計算
		int sum = 0;
		int sumgap = 0;	// 行間
		for ( i=0;i<=LastIndex;i++ ){
			RecalcLine( IndexOffset + i );
			sum += LinesList[ IndexOffset + i ].NumLines;
			sumgap += ItemView.LineInterval;
			if ( sum >= l )
				break;
		}
		if ( i > LastIndex )	// スクロール後に表示される項目がその項目を過ぎていた
			i = LastIndex;

		IndexOffset += i+1;
		_ScrollWindow( HWindow, 0, -( sum + sumgap ) );
		LastIndex -= i+1;
		if ( LastIndex < 0 )
			LastIndex = 0;	// 1999.1.4
	}
	IndexMiOffset = 0;
	cury -= i+1-down;		// ２行以上スクロールした場合に必要
	if ( cury < 0 )
		cury = 0;
	MouseCap->OffsetIndex(i+1);
	if (cury!=-1){
		if (UIMain) UIMain->SetWordText(GetWord(cury));
		_dispStar( cury, 1 );
	}
	Request( IndexOffset, false );
	SetVPos( IndexOffset );
	SetUpDown(true);
	ScrolledWindow();
	ReleaseDC();
	return i+1;
}

void Squre::KeyScrollDown( )
{
	bool link_ui = true;
	DBX("down");
	GetDC();
	if ( cury >= LastIndex /* || cury == LastIndex-1 */ ){
		ScrollDown( true, 1 );
	} else if ( cury == -1){
		IndexMiOffset = 0;
		ClsRegion();
		dispStar(0, link_ui);
		SetUpDown(true);
	} else {
		if (cury + IndexOffset < get_num() - 1){
			ClsRegion();
			dispStar(cury+1, link_ui);
			SetUpDown(true);
		}
	}
#if !defined(SMALL) && defined(USE_JLINK)
	if ( AutoViewMode ) AutoView();
#endif

	// cury>=0にもかかわらず、half selectionの場合はfull selectionにする
	if (GetReverseStatus()==SQU_REV_GRAY && cury>=0){
		dispStar(cury, link_ui);
	}

	ReleaseDC();
}

// 返り値はドット単位
// yは相対インデックス
int Squre::GetOffsY( int y )
{
#if 0
	int l = y;	// １行１単語
#else
#if 0
	int l = 0;
	for ( int i=0;i<y;i++ ){
		RecalcLine( IndexOffset + i );
		l += GetLines(i);
	}
#else 	// 高速化   1995.5.29
	int l = 0;
	int *ll = &LinesList[IndexOffset].NumLines;
	for ( int i=0;i<y;i++ ){
		RecalcLine( i + IndexOffset );
		l += *ll++;
	}
#endif
#endif
	return l + ItemView.LineInterval*y;
}

// 返り値はドット単位
// yは絶対インデックス
int Squre::GetAbsOffsY( int y )
{
#if 1	// 2009.4.7 bugってると思うので修正した
	int l = 0;
	for ( int i=0;i<y;i++ ){
		RecalcLine( i );
		l += LinesList[ i ].NumLines + ItemView.LineInterval;
	}
	return l;
#else	// 2009.4.7 old code
	int l = 0;
	for ( int i=0;i<y;i++ ){
		RecalcLine( i );
		l += LinesList[ i ].NumLines;
	}
	return l + ItemView.LineInterval;
#endif
}


int Squre::GetLocateY( int y )
{
	return GetHomeY()+GetOffsY( y ) - IndexMiOffset;
}

// 全poolの表示がクライアント領域を超えるかどうかのチェック
bool Squre::IsOver( )
{
	int l = 0;
	for ( int i=0;i<get_num();i++ ){
		RecalcLine( i );
		l += LinesList[ i ].NumLines + ItemView.LineInterval;
		if ( l > MaxLines )
			return true;
	}
	return false;
}

// 上端からlinesドットに該当するindex(absolute)を返す
// lines : 行数(ドット単位) - 省略時 MaxLines
//Note: requires DC
// return:
//	>=IndexOffset, <get_num()
//	==get_num() : 完全に表示しきれる
int Squre::GetIndexFromLines(int lines)
{
	if (lines==0)
		lines = MaxLines;

	int sum = 0;
	int i;
	for ( i=IndexOffset;i<get_num();i++ ){
		RecalcLine( i );
		sum += LinesList[ i ].NumLines + ItemView.LineInterval;
		if (sum >= lines){
			break;
		}
	}
	return i;
}

//void Squre::EvVScroll( UINT code, UINT pos, HWND /* hwnd */, TMessage & )
void Squre::EvVScroll( UINT code, int &pos, HWND )
{
//	UINT code = Msg.WParam;
//	UINT pos = Msg.LP.Lo;
	if ( !get_num() )
		return;
	GetDC( );			// 要らない？
	switch ( code ){
		case SB_LINEDOWN:
			ScrollDown1( false );
			pos = GetVPos();
			break;
		case SB_LINEUP:
			ScrollUp( );
			pos = GetVPos();
			break;
		case SB_PAGEDOWN:
			NextPage( );
			pos = GetVPos();
			break;
		case SB_PAGEUP:
			PrevPage( );
			pos = GetVPos();
			break;
		case SB_THUMBPOSITION:
//		case SB_THUMBTRACK:
			if ( IndexOffset != pos ){
				bool bBack = IndexOffset > pos;
				MouseCap->OffsetIndex(pos - IndexOffset);
				int diff = pos-IndexOffset;
				IndexOffset = pos;
				if (get_num()>0){
					if (IndexOffset>=get_num())
						IndexOffset = get_num()-1;
					//DBW("cury=%d get_num()=%d IndexOffset=%d", cury, get_num(), IndexOffset);
					// curyが画面外(上方）になるときは画面内になるように
					if ((cury<0 || cury-diff<0) && IndexOffset<get_num())
						cury = 0;
					else
					// IndexOffsetの変更によりcuryの調整が必要
					if (cury+IndexOffset>=get_num()){
						cury = get_num()-IndexOffset-1;
					}
				} else {
					// ここには来ないはず
#ifdef _DEBUG
					__assert__;
#endif
					IndexOffset = 0;
					cury = -1;
				}
				SetVPos( IndexOffset );
				Invalidate( );
				Request( IndexOffset, bBack );
				pos = GetVPos();
			}
			break;
	}
	ReleaseDC( );
}

