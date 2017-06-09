#include	"pdclass.h"
#pragma	hdrstop
#include "defs.h"
#include "draw4com.h"
#include	"id.h"
#include	"winsqu.h"
#include "windic.h"	// for only dic.GetDicNum()
#include "squfont.h"
#include "SquItemView.h"

#include "SquView.h"	// for only GetDC()/ReleaseDC()

#include "MouseCapture.h"

//static int hdcRef = 0;

void Squre::Paint( HDC tdc, RECT &rcp )
{
#if USE_BMP
	HDC oldtdc = tdc;
	tdc = CreateCompatibleDC( tdc );
	HBITMAP hm = CreateCompatibleBitmap( oldtdc, GetLX(), GetLY() );
	HBITMAP hold = (HBITMAP)SelectObject( tdc, hm );
#endif

#if SQU_DISP_NUM
	// 番号用縦線
	if ( IsVisible( SN_NUMBER ) && ( Dic.GetDicNum() || bTempClose ) ){
		HPEN hpen = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNHIGHLIGHT ) );
		if ( hpen ){
			HGDIOBJ hold = ::SelectObject( tdc, hpen );
			int x = GetOffs( SN_ATTR ) - 3;
			int top = GetHomeY( );
			int bottom = GetHomeY( ) + GetLY( );
#if defined(__WIN32__)
			MoveToEx( tdc, x, top, NULL );
#else
			MoveTo( tdc, x, top );
#endif
			LineTo( tdc, x, bottom );
			::SelectObject( tdc, hold );
			DeleteObject( hpen );
			hpen = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNFACE ) );
			hold = ::SelectObject( tdc, hpen );
			x++;
#if defined(__WIN32__)
			MoveToEx( tdc, x, top, NULL );
#else
			MoveTo( tdc, x, top );
#endif
			LineTo( tdc, x, bottom );
			::SelectObject( tdc, hold );
			DeleteObject( hpen );
			hpen = CreatePen( PS_SOLID, 1, GetSysColor( COLOR_BTNSHADOW ) );
			hold = SelectObject( tdc, hpen );
			x++;
#if defined(__WIN32__)
			MoveToEx( tdc, x, top, NULL );
#else
			MoveTo( tdc, x, top );
#endif
			LineTo( tdc, x, bottom );
			::SelectObject( tdc, hold );
			DeleteObject( hpen );
		}
#if 0
		HBRUSH hbr = CreateSolidBrush( GetSysColor( COLOR_BTNHIGHLIGHT ) );
		RECT rc;
		rc.left = GetOffs( SN_ATTR ) - 3;
		rc.top = GetHomeY( );
		rc.bottom = GetHomeY( ) + GetLY( );
		rc.right = rc.left + 1;
		FillRect( tdc, &rc, hbr );
		DeleteObject( hbr );
		hbr = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ) );
		rc.left++;
		rc.right++;
		FillRect( tdc, &rc, hbr );
//		MoveTo( tdc, GetOffs( SN_ATTR ) - 2, GetHomeY() );
//		LineTo( tdc, GetOffs( SN_ATTR ) - 2, GetHomeY( ) + GetLY( ) );
		DeleteObject( hbr );
		hbr = CreateSolidBrush( GetSysColor( COLOR_BTNSHADOW ) );
		rc.left++;
		rc.right++;
		FillRect( tdc, &rc, hbr );
//		MoveTo( tdc, GetOffs( SN_ATTR ) - 1, GetHomeY() );
//		LineTo( tdc, GetOffs( SN_ATTR ) - 1, GetHomeY( ) + GetLY( ) );
		DeleteObject( hbr );
#endif
	}
#endif	// SQU_DISP_NUM
	if ( !bAllowDisp ){
		return;
	}

	HDC _oldhdc = hdc;
	hdc = tdc;
	_SetCurrentHDC( hdc );
	CreateTextFonts( );

	int i;

	// 見出語部を選択(色抜き反転)、テキスト範囲選択時、
	// きれいに描画させるため、rvItemを一時的に無効にして、
	// dispStar()で色抜き反転とテキスト範囲選択表示を行わせる
	int _rvItem = MouseCap->BackupSel();

	int _cury = cury;
	cury = -1;

#ifndef NOSQUCOLOR
	if ( hdc ){
		SetTextColor( hdc, GET_FORECOLOR( charcolor ) );
		SetBkColor( hdc, GET_BACKCOLOR( backcolor ) );
	}
#endif
	// 描画の最適化
	if ( GetLY() <= rcp.bottom - rcp.top + 10 ){	// 最後の+1は??? (Invalidate()をしてbottom-topがLY-1のときがあった???)
											// まぁ結局数ドットくらいだったら全体を再描画した方が良い
		// 全体を描画する場合は、linelist[]を再設定する必要があるので
		// 表示項目設定を変更したときにそうなる
		display( );
	} else {
		int top = rcp.top;
		int bottom = rcp.bottom;

		// 行番号から、相対インデックス番号を得る
		int start = 0;
		int end = 0;
		int sum = -IndexMiOffset;	// bug fixed. 2014.9.29
		for ( i=0;i+IndexOffset< get_num();i++ ){
			if ( top >= sum ){
				start = i;
			}
			if ( bottom >= sum ){
				end = i;
			} else break;
			RecalcLine( i+IndexOffset );	// Ver.3.11
			sum += GetLines(i) + ItemView.LineInterval;
		}
		end++;

		display( start, end );
	}
#if USE_BMP
	// コピー
	BitBlt( oldtdc, 0, 0, GetLX(), GetLY(), tdc, 0, 0, SRCCOPY );
	SelectObject( tdc, hold );
	DeleteDC( tdc );
	DeleteObject( hm );

	hdc = tdc = oldtdc;
#endif


	// 反転バーの表示
	if ( _cury != -1 ){
#if 1	// 1998.7.11 (lastindex以降の)最下行にカーソルがきたとき、勝手に補正されては困る
		// 1998.9.5 また復活させたがどこで問題ある？
		// 2008.3.21 curyを変えるのではなく、offsetを変えるようにした（mayを検索するとわかる）
		if ( _cury > LastIndex ){
#if 0	// 2008.3.21 旧式
			_cury = LastIndex;
#else
			int diff = _cury-LastIndex;
			IndexOffset += diff;
			_cury -= diff;
			Invalidate();
#endif
		}
#endif	// 1
#if USE_DT2
		if ( _rvItem == SN_WORD ){
			_dispStar( _cury, 1 );
			MouseCap->SetSelItemIndex(SN_WORD);
			_dispStar( _cury, 1 );
			cury = _cury;
		} else
#endif
		{
			dispStar( _cury );
		}
	}
	MouseCap->RestoreSel(_rvItem);


//	if ( Dic.GetDicNum() )
//		SetVRange( get_num()-1, true );	// 1995.1.13 SetVRange2->SetVRange

	MouseCap->DrawAttr();

#if USE_DT2
#if 1	// EPWINGの枠がおかしくなるため止めた
		// 再現方法：EPWING linkの枠を表示させる(DrawSelection)
		// link先をclick -> Popup Window出る [リンクは左クリックの設定] - このとき、popup windowが枠をoverlapする
		// 別のlinkをclick -> 前のpopup windowがcloseして 枠が一瞬消えるが、再描画されて、再び枠がここの命令に
		// よって、消されてしまう
		// ここで、別のlinkをclickではなく、linkのないEPWING textをclickすると枠は消えない？？？
	int alItem = MouseCap->al.GetItemIndex();
	if ( alItem != -1 ){
		MouseCap->al.DrawUnderline( );
	}
#endif
#endif
	DeleteTextFonts( );
	hdc = _oldhdc;
	_SetCurrentHDC( hdc );

	__assert(!nFontsOpened);
#ifdef _DEBUG
	while (nFontsOpened){
		DeleteTextFonts();
	}
#endif
}

HDC Squre::GetDC( )
{
	return hdc = View->GetDC();
}

// GDI処理
void Squre::ReleaseDC( )
{
	if (View->ReleaseDC(hdc)){
		hdc = NULL;
	}
}

void Squre::ForceDraw( )
{
	bAllowDisp = true;
	cury = 0;
	Invalidate();
}

// start,endは相対インデックス番号
void Squre::display( int start, int end )
{
	int i;

	CreateTextFonts( );

	int selIndex = MouseCap->GetSelIndex();
	int itemIndex = MouseCap->GetSelItemIndex();
	for ( i=start;i+IndexOffset< get_num() && i <= end;i++){
		DispOneLine( i, DF_DISP | DF_PAINT );
#if USE_DT2
		if ( selIndex == i && itemIndex != -1 ){
			MouseCap->DrawSelItem();
		}
#endif
		if ( i > LastIndex )
			break;
	}

	DeleteTextFonts( );
}

//	テキストの表示用フォントの作成
//	参照カウントにより、何回呼び出しても大丈夫
// hdcは必ず有効である事
void Squre::CreateTextFonts( )
{
	_CreateTextFonts( hdc );
}

void Squre::SelectCommonFont( )
{
	SelectObject( hdc, hnFont );
}

// hdcは必ず有効である事
void Squre::DeleteTextFonts( )
{
	if ( nFontsOpened == 1 )
		SelectObject( hdc, GetStockObject( SYSTEM_FONT ) );
	_DeleteTextFonts( );
}


void Squre::Invalidate(bool resend)
{
	InvalidateLinks();
	LastIndex = 0;
	View->Invalidate();
#ifdef __ANDROID__
	if (resend){
		IPoolViewer *viewer = pool.GetViewer();
		viewer->Clear();
		DBW("IndexOffset=%d/%d", IndexOffset, get_num());
		for (int i=IndexOffset+cury;i<get_num();i++){
			viewer->Add(&pool.fw[i], &pool.GetJapaObj(i), pool.GetFDN(i), pool.GetDispLevel(i));
		}
	}
#endif
}

void Squre::InvalidateLinks()
{
#ifdef USE_JLINK
	// 表示されているobjectのみ→はだめ
	for (int i=0;i<get_num();i++){
		JLinks &jls = GetJapa(i-IndexOffset).jlinks;
		for (int j=0;j<jls.get_num();j++){
			jls[j].Invalidate();
		}
	}
#endif
}

// 強制的に消去(なるべく、InvalidateRect()で行なった方が良い）
void Squre::cls( )
{
	LastIndex = 0;
	View->Invalidate();
	pagelines = 0;
}

void Squre::Clear( )
{
	GetDC();
	clear();
	cls();
    ReleaseDC();
}
void Squre::Escape()
{
    EndSearch( );
    GetDC();
	ClsRegion();
	clsStar();
	ClearMessage();
    ReleaseDC();
}

#if 0
void Squre::RecalcAllLines( )
{
	for ( int i=0;i<get_num();i++ ){
		DispOneLine( i-offset, DF_NODISP );
	}
}
#endif

// call from TMouseCapture
void Squre::InvalidateLine(int index)
{
	LinesList[index].NumLines = 0;
	Invalidate();
}

void Squre::InvalidateLines( )
{
	fRecalcLine = true;
	memset( LinesList, 0, sizeof(TSquLineInfo) * MAX_NDISP );
#if 0
	for ( int i=0;i<get_num();i++ ){
		linelist[ i ] = 0;
	}
#endif
}

// linelist[]が０のものを再計算する！
// i は絶対番号
void Squre::RecalcLines( int start, int end )
{
	GetDC( );
	CreateTextFonts( );
	for ( int i=start;i<=end;i++ ){
		if ( !LinesList[ i ].NumLines ){
			DispOneLine( i-IndexOffset, DF_NODISP );
		}
	}
	DeleteTextFonts( );
	ReleaseDC( );
}
void Squre::RecalcLineForce(int i)
{
	LinesList[i].NumLines = 0;
	RecalcLines(i, i);
}

void Squre::DeleteLine( int abs )
{
	memmove( &LinesList[abs], &LinesList[abs+1], (MAX_NDISP - abs - 1)*sizeof(TSquLineInfo) );
}

