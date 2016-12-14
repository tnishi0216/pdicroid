#include "pdclass.h"
#pragma	hdrstop
#include "pdconfig.h"
#include	"wpdcom.h"
#include	"defs.h"
#include	"filestr.h"
#include	"pdstr.h"
#include "textext.h"
#ifdef WINPPC
#include <sipapi.h>
#endif

#ifdef GUI
#include "UserMsg.h"	// for MoveToTopPdic
#include "ShortCut.h"	// for MoveToTopPdic
#include "pdprof.h"

#if !defined(WINCE)
#include <objbase.h>
#endif
#endif	// GUI

static void _MoveToTop(HWND hwnd, bool restore_invisible);

tnstr CommandPath;	// このプログラムのパス
bool MoveToTopDoing = false;
bool Suspending = false;

///////////////////////////////////////////////////////////////
// ファイル名関連
///////////////////////////////////////////////////////////////

#ifndef SMALL
int GetFileType( const tchar *filename, const tchar *ext )
{
	if ( !ext ){
		if ( !filename )
			return FT_NONE;
		ext = GetFileExtension( filename );
	}
	if ( textext.IsText( ext ) ){
		return FT_TEXT;
	} else
	if ( !_tcsicmp( ext, _T("BMP") ) || !_tcsicmp(ext, _t("dib"))){
		return FT_BMP;
	} else
	if ( !_tcsicmp( ext, _T("RTF") ) ){
		return FT_RTF;
	} else
	if ( !_tcsicmp( ext, _T("TIF") ) || !_tcsicmp(ext, _t("tiff")) ){
		return FT_TIFF;
	} else
	if ( !_tcsicmp( ext, _T("JPG") ) || !_tcsicmp(ext, _t("jpeg")) ){
		return FT_JPEG;
	} else
	if ( !_tcsicmp( ext, _T("GIF") ) ){
		return FT_GIF;
	} else
	if ( !_tcsicmp( ext, _T("MAG") ) ){
		return FT_MAG;
	} else
	if ( !_tcsicmp( ext, _T("LZH") ) ){
		return FT_LZH;
	} else
	if ( !_tcsicmp( ext, _T("wav") ) || !_tcsicmp( ext, _T("mp3") ) || !_tcsicmp( ext, _T("wma") ) || !_tcsicmp( ext, _T("ogg") ) ){
		return FT_WAV;
	} else
	if (!_tcsicmp(ext, _t("png"))){
		return FT_PNG;
	} else
	if (!_tcsicmp(ext, _t("html")) || !_tcsicmp(ext, _t("htm"))){
		return FT_HTML;
	} else
	if (!_tcsicmp(ext, _t("ico"))){
		return FT_ICON;
	} else
	if (!_tcsicmp(ext, _t("wmf")) || !_tcsicmp(ext, _t("wmf"))){
		return FT_WMF;
	} else
	if ( !_tcsicmp( ext, _T("MPG") ) || !_tcsicmp( ext, _T("MPEG") ) || !_tcsicmp(ext, _t("mp4")) || !_tcsicmp(ext, _T("wmv"))){
		return FT_MPEG;
	} else
	if ( !_tcsicmp( ext, _T("AVI") ) ){
		return FT_AVI;
	} else
	if ( !_tcsicmp( ext, _T("MID") ) ){
		return FT_MID;
	}
	return FT_NONE;
}

#endif
///////////////////////////////////////////////////////////////
// ファイル操作関連
///////////////////////////////////////////////////////////////
#if defined(GUI) && !defined(WINCE)
// D&D処理
// 返り値はdeleteする必要あり！！
tnstr_vec *GetDDFiles( HDROP hDrop )
{
	int num = DragQueryFile( hDrop, (UINT)-1, NULL, 0 );	// ドロップされたファイルの個数を得る
	if ( num < 1 ){
		return NULL;
	}
	tnstr_vec *files = new tnstr_vec;
	if (!files)
		return NULL;	// no memory
	int bufsize = _MAX_PATH;
	auto_ptr<TCHAR> name(new TCHAR[ bufsize + 1 ]);
	if (!name.get()){
		delete[] files;
		return NULL;	// no memory
	}
	for ( int i=0;i<num;i++ ){
		int size = DragQueryFile( hDrop, i, NULL, 0 );	// ファイルの長さを得る
		if (size>bufsize){
			name.reset(new TCHAR[size+1]);
			if (!name.get()){
				break;	// not enough memory to get all files.
			}
			bufsize = size;
		}
		DragQueryFile( hDrop, i, name.get(), bufsize );
		files->add( name.get() );
	}

	DragFinish( hDrop );
	return files;
}
#endif	// defined(GUI) && !defined(WINCE)
#ifndef SMALL
bool GetFileInfo( const tchar *filename, ulong *FileSize, FILETIME *FileTime )
{
	OFSTRUCT of;
	memset( &of, 0, sizeof(of) );
	of.cBytes = sizeof(of);
	HANDLE hFile = (HANDLE)OpenFile( _mustr(filename), &of, OF_READ );
	if ( hFile == (HANDLE)HFILE_ERROR )
		return false;
	if ( FileSize )
		*FileSize = GetFileSize( hFile, NULL );
	if ( FileTime ){
		GetFileTime( hFile, NULL, NULL, FileTime );
	}
	CloseHandle( hFile );
	return true;
}
#endif

#if NETDIC || INETDIC
unsigned long GetFileSizeFileName( const tchar *filename )
{
	HANDLE h = CreateFile( filename, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( h == INVALID_HANDLE_VALUE ){
		return 0;
	}
	DWORD high;
	DWORD size = GetFileSize( h, &high );
	CloseHandle( h );
	if ( high ){
		return 0xFFFFFFFF;
	}
	return size;
}
#endif

#ifdef GUI
bool write_bom(TOFile &tof)
{
#ifdef _UNICODE
	long l = tof.tell();
	if ( l == 0 ){
		tof.settextmode(prof.GetTextFileCode());
		if (prof.IsTextFileBOM())
			tof.bom();
	}
#endif
	return true;
}
#endif

///////////////////////////////////////////////////////////////
// ウィンドウ操作
///////////////////////////////////////////////////////////////
#ifdef GUI
// メッセージが到着するまでループする
//	システムをアイドル状態にできる
void WaitDlgMessageProc( )
{
	WaitMessage();
	DlgMessageProc();
}
// メッセージが無ければすぐ戻る
void DlgMessageProc( )
{
	Application->ProcessMessages();
}
bool AppMessageProc( )
{
	MSG Message;
	if ( PeekMessage(&Message, 0, 0, 0, PM_REMOVE) )
	{
//		idleCount = 0;
//	  if ( Message.message == WM_QUIT )
//		break;
//	  if ( !Process(&Message) )
	  {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	  }
	  return true;	// messageがあった
	}
	return false;	// messageがなかった
}
bool AppMessageProc( int msgmin, int msgmax )
{
	MSG Message;
	if ( PeekMessage(&Message, 0, msgmin, msgmax, PM_REMOVE) )
	{
//		idleCount = 0;
//	  if ( Message.message == WM_QUIT )
//		break;
//	  if ( !Process(&Message) )
	  {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	  }
	  return true;	// messageがあった
	}
	return false;	// messageがなかった
}

#if 0
// メッセージが到着するまでループする
//	システムをアイドル状態にできる
// falseが返ってきたときは、プログラムを終了しなければならない
// message queueが空になるまで処理を続ける
void WaitAppMessagesProc( )
{
	::WaitMessage();
	Application->ProcessMessages();
}
#endif

#if 0
// __WaitAppMessagesProc()の単一処理版
// 一つずつ処理する
// Idle処理が実行されない。
void WaitAppMessageProc( )
{
	//Note:
	// いきなりWaitMessage()ではvclProcessMessage()で処理されない場合がある
	MSG msg;
	if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
		::WaitMessage();
	}
	vclProcessMessage();
}
#endif

// __WaitAppMessagesProc()の単一処理版
// 一つずつ処理する
// Idle処理が実行される。
void WaitAppMessageIdle()
{
	vclHandleMessage();	// vclProcessMessage + Idle() + WaitMessage()
}

void EnableItems( HWND hwnd, int *id, BOOL f )
{
	while ( *id ){
		EnableWindow( GetDlgItem( hwnd, *id ), f );
		id++;
	}
}

void ShowItems( HWND hwnd, int *id, BOOL f )
{
	while ( *id ){
		ShowWindow( GetDlgItem( hwnd, *id ), f );
		id++;
	}
}

//#ifndef SMALL
// restore_invisible : VCL main window用
void MoveToTop( HWND hwnd, bool restore_invisible )
{
	MoveToTopDoing = true;
	_MoveToTop(hwnd, restore_invisible);
	MoveToTopDoing = false;
}
static void _MoveToTop(HWND hwnd, bool restore_invisible)
{
#ifdef WINCE
	SetForegroundWindow(hwnd);
#else
	if (IsIconic( hwnd ) || (restore_invisible && !IsWindowVisible(hwnd))){
		WINDOWPLACEMENT wp;
		memset(&wp, 0, sizeof(wp));
		wp.length = sizeof(wp);
		GetWindowPlacement(hwnd, &wp);
		if (wp.showCmd==SW_SHOWMAXIMIZED){
			ShowWindow(hwnd, SW_SHOWMAXIMIZED);
		} else {
			ShowWindow(hwnd, SW_RESTORE);
		}
	}

//	ZOrderInhibit++;
#if 1

	SetForegroundWindow(hwnd);

	if ( GetForegroundWindow() == hwnd ){
//		ZOrderInhibit--;
		return;
	}

#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT
#define SPI_GETFOREGROUNDLOCKTIMEOUT		  0x2000
#endif
#ifndef SPI_SETFOREGROUNDLOCKTIMEOUT
#define SPI_SETFOREGROUNDLOCKTIMEOUT		  0x2001
#endif
	int nTargetID, nForegroundID;
	UINT nTimeout = 0;
//	BOOL ret;

	nForegroundID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	nTargetID = GetWindowThreadProcessId(hwnd, NULL);
	AttachThreadInput(nTargetID, nForegroundID, TRUE);

	SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &nTimeout, 0);
	if (nTimeout!=0){
		if (!SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)0, 0)){
			DWORD err = GetLastError();
			//DBW("MoveToTop:failed 1:%d",err);	//TODO: 管理者権限が必要？
		}
	}

	BringWindowToTop(hwnd);
	/*ret = */ SetForegroundWindow(hwnd);

	if (nTimeout!=0){
		SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)nTimeout, 0);
	}

	AttachThreadInput(nTargetID, nForegroundID, FALSE);

	SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
	//BringWindowToTop(hwnd);

	if (GetForegroundWindow()!=hwnd){
		// why??
		HMODULE hUser32 = GetModuleHandle(_T("user32"));
		if ( hUser32 ){
			typedef void (WINAPI *PROCSWITCHTOTHISWINDOW) (HWND, BOOL);
			// NT5.0 or Win98 or later
			PROCSWITCHTOTHISWINDOW _SwitchToThisWindow = (PROCSWITCHTOTHISWINDOW)GetProcAddress(hUser32, "SwitchToThisWindow");
			if ( _SwitchToThisWindow ){
				// NT5.0 or Win98 or later
				_SwitchToThisWindow( hwnd, true );
			}
		}
		//BringWindowToTop(hwnd);
		if (GetForegroundWindow()!=hwnd){
			// why?? why??
			
		}
	}
#else
	HMODULE hUser32 = GetModuleHandle(_T("user32"));
	if ( hUser32 ){
		typedef void (WINAPI *PROCSWITCHTOTHISWINDOW) (HWND, BOOL);
		// NT5.0 or Win98 or later
		PROCSWITCHTOTHISWINDOW _SwitchToThisWindow = (PROCSWITCHTOTHISWINDOW)GetProcAddress(hUser32, "SwitchToThisWindow");
		if ( _SwitchToThisWindow ){
			// NT5.0 or Win98 or later
			_SwitchToThisWindow( hwnd, true );
		}
	}
	SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

	if ( GetForegroundWindow() == hwnd ){
//		ZOrderInhibit--;
		return;
	}

	int i;
	for ( i=0;i<10;i++ )
	{
		int nForegroundID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
		int nTargetID = GetWindowThreadProcessId(hwnd, NULL );
		AttachThreadInput(nTargetID, nForegroundID, TRUE );

		DWORD locktimeout;
		SystemParametersInfo( SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &locktimeout, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE );
		SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, SPIF_UPDATEINIFILE);
		SetForegroundWindow(hwnd);
		SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, locktimeout, NULL, SPIF_UPDATEINIFILE);

		AttachThreadInput(nTargetID, nForegroundID, FALSE );

		HWND h = GetForegroundWindow();
		if ( h == hwnd )
			break;
		Sleep(1);
	}
#endif
//	ZOrderInhibit--;
#endif
}
//#endif

void MoveToTopNA( HWND hwnd )
{
	if ( GetForegroundWindow() == hwnd ){
		return;
	}

//	ZOrderInhibit++;

#if 0	// 弊害が出そうなのであまりやりたくない。。
	HWND hwndOrg = GetForegroundWindow();
	SetForegroundWindow(hwnd);
	SetForegroundWindow(hwndOrg);
#endif

	//SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE /*| SWP_SHOWWINDOW | SWP_NOACTIVATE*/);
	SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE /*| SWP_SHOWWINDOW | SWP_NOACTIVATE*/);

	//TODO: この処理をどうすべきか迷うところ・・・
	SetWindowPos( hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE /*| SWP_SHOWWINDOW | SWP_NOACTIVATE*/);

//	ZOrderInhibit--;
}

#ifdef GUI
// PDIC専用最前面処理
void MoveToTopPdic(HWND hwnd)
{
#if 1
	PostMessage(hwnd, UM_SHORTCUT, SCINX_TOPMOST, 0);
#else
	MoveToTop(hwnd, !prof.IsTaskTray());
	// 第二引数
	// task tray常駐時にrestoreすると最小化ボタンが効かなくなる問題が発生する（致命的）
	// そのため、task tray常駐時はrestoreしない、という仕様で回避。
#endif
}
#endif

// hwndを現在のscreen内で見えるように移動させる
void MakeWindowVisible(HWND hwnd)
{
	RECT rc;
	GetWindowRect(hwnd, &rc);

	int offx = 0;
	int offy = 0;
	RECT _rc;
	GetScreenSize(hwnd, &_rc);
	int sx = _rc.right - _rc.left;
	int sy = _rc.bottom - _rc.top;

	if ( rc.right > _rc.right ){
		if ( rc.right - rc.left > sx ){
			// 画面より大きい
			offx = -rc.left;
		} else {
			// 画面より小さいが、右端がはみ出る
			offx = _rc.right - rc.right;
		}
	} else if ( rc.left < 0 ){
		// 左端が見えない
		offx = rc.left;
	}
	if ( rc.top < _rc.top ){
		// 上端が見えない
		offy = _rc.top - rc.top;
	} else
	if ( rc.bottom > _rc.bottom ){
		if ( rc.bottom - rc.top > sy ){
			// 画面より大きい
			offy = -rc.top;
		} else {
			// 画面より小さいが、下端がはみ出る
			offy = -(rc.bottom - _rc.bottom);
		}
	}
	OffsetRect( &rc, offx, offy );
	SetWindowPos(hwnd, NULL, rc.left, rc.top, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
}

#endif	// GUI

///////////////////////////////////////////////////////////////
// シェル・アイコン関連
///////////////////////////////////////////////////////////////
#ifndef WINCE
// 拡張子から関連付けアプリケーションを得る
// param1がある場合は、%1を置き換える
bool GetAssociationFile( const tchar *filename, tnstr &app )
{
	tchar *buf = new tchar[ MAXPATH + 1 ];
	HINSTANCE hInst = FindExecutable( filename, NULL, buf );
	if ( (UINT)hInst > 32 ){
		app.setBuf( buf );
		return true;
	} else {
		delete[] buf;
		return false;
	}
}

#define	NUM_ICONCACHE	0

struct ICONCACHE {
	tnstr ext;
	HICON hIcon;
	ICONCACHE( const tchar *_ext, HICON _hIcon )
		:ext( _ext )
	{
		hIcon = _hIcon;
	}
	~ICONCACHE( )
	{
		DestroyIcon( hIcon );
	}
};

#if NUM_ICONCACHE > 0
ObjectArray<ICONCACHE> iconcache( NUM_ICONCACHE );	// キャッシュ
#endif

HICON _GetIcon( HINSTANCE hInstance, const tchar *filename, int inx );

// 拡張子からアイコンを得る キャッシュ付き！
// ext : 拡張子(.を含まない）
// inx : アイコンインデックス
//
// キャッシュ付きの場合の注意事項
// 終了時にclean upする(filenameをNULLにして渡す)
// 拡張子関連付けや、関連付けアプリケーション自体が変更された場合、キャッシュをフラッシュする必要がある
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// 戻り値のhIconは inx=0 以外は削除しないこと！！
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
HICON GetIcon( HINSTANCE hInstance, const tchar *filename, int inx )
{
	if ( !filename ){
#if NUM_ICONCACHE > 0
		iconcache.clear();
#endif
		return NULL;
	}
#if NUM_ICONCACHE > 0
	const tchar *ext = GetFileExtension( filename );
	if ( ext && inx == 0 ){
		// search
		for ( int i=0;i<iconcache.get_num();i++ ){
			if ( !_tcsicmp( iconcache[i].ext, ext ) ){
				return iconcache[i].hIcon;
			}
		}
	}
#endif
	HICON hIcon = _GetIcon( hInstance, filename, inx );
#if NUM_ICONCACHE > 0
	if ( hIcon && inx == 0 ){
		if ( iconcache.get_num() >= NUM_ICONCACHE ){
			// discard
			iconcache.del( 0 );
		}
		// add
		iconcache.add( new ICONCACHE( ext ? ext :StrNull, hIcon ) );
	}
#endif
	return hIcon;
}
void ClearIconCache()
{
#if NUM_ICONCACHE > 0
	iconcache.clear();
#endif

}

// 拡張子からアイコンを得る
// ext : 拡張子(.を含まない）
// inx : アイコンインデックス
HICON _GetIcon( HINSTANCE hInstance, const tchar *filename, int /* inx */ )
{
	return GetAssociatedIcon( hInstance, filename );
}

HICON GetAssociatedIcon( HINSTANCE hInstance, const tchar *filename )
{
	tchar buf[ L_FILENAME ];
	_tcscpy( buf, filename );
	WORD id = 0;
	// なぜか、write protectedされているdiskの中のfileのiconだと、
	// 「write protectされてますよー」とメッセージが出る？？？
	return ExtractAssociatedIcon( hInstance, buf, &id );
}

#endif	// !defined(WINCE)

const tchar *MyWinTitle = _T("<Processing...in PDIC/W>");

HANDLE _WinExec( const tchar *cmd, int show, const tchar *dir )
{
#if 1
	return WinExecEx(cmd, show, dir, MyWinTitle);
#else
#ifdef WINCE
	PROCESS_INFORMATION pi;
	if ( !CreateProcess( NULL, (LPTSTR)cmd, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi ) )
		return NULL;
	return pi.hProcess;
#else
	STARTUPINFO sui;
	memset( &sui, 0, sizeof(STARTUPINFO) );
	sui.cb = sizeof(STARTUPINFO);
	sui.dwFlags = STARTF_USESHOWWINDOW;
	sui.wShowWindow = (WORD)show;
	sui.lpTitle = (LPTSTR)MyWinTitle;
	PROCESS_INFORMATION pi;
	if ( !CreateProcess( NULL, (LPTSTR)cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, dir, &sui, &pi ) )
		return NULL;
	return pi.hProcess;
#endif
#endif
}

#ifdef WINCE
bool _ShellExecute( HWND hwnd, const tchar *verb, const tchar *filename, const tchar *parameter, const tchar *directory, int show )
{
	_SHELLEXECUTEINFO sei;
	memset( &sei, 0, sizeof(sei) );
	sei.cbSize = sizeof(sei);
	sei.hwnd = hwnd;
	sei.lpVerb = verb;
	sei.lpFile = filename;
	sei.lpParameters = parameter;
	sei.lpDirectory = directory;
	sei.nShow = show;
	return !ShellExecuteEx( &sei );
}
#endif	// WINCE
///////////////////////////////////////////////////////////////
// フォント関連
///////////////////////////////////////////////////////////////

#if defined(GUI)
// for あほWin95,NT4.0 //
static int cSysFont = 0;
static HFONT hSysFont;
HFONT CreateSystemFont( )
{
	if ( cSysFont ){
		cSysFont++;
		return hSysFont;
	}
	cSysFont++;

	LOGFONT lf;
#if 1
	memset( &lf, 0, sizeof(lf) );
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfHeight = -GetSystemMetrics( SM_CYCAPTION )*2/3;
	_tcscpy(lf.lfFaceName, prof.GetDefFontName());
#else
	SystemParametersInfo( SPI_GETICONTITLELOGFONT, 0, &lf, FALSE );
#endif
	return hSysFont = CreateFontIndirect( &lf );
}

HFONT SelectSystemFont( HDC hdc )
{
	return (HFONT)SelectObject( hdc, CreateSystemFont( ) );
}

void DeleteSystemFont( HDC hdc, HFONT hOldFont )
{
	SelectObject( hdc, hOldFont );
	if ( --cSysFont == 0 ){
		DeleteObject( hSysFont );
	}
}

static int cySystem = -1;
static int cxSystem = -1;

static void GetSystemFontMetrics()
{
	HDC hdc = GetDC( NULL );
	HFONT hFont = CreateSystemFont();
	HFONT hOld = (HFONT)SelectObject( hdc, hFont );
	TEXTMETRIC tm;
	GetTextMetrics( hdc, &tm );
	cySystem = tm.tmHeight + tm.tmExternalLeading + 2;
	cxSystem = tm.tmAveCharWidth + 2;
	DeleteSystemFont( hdc, hOld );
	ReleaseDC( NULL, hdc );
}

int GetSystemFontHeight()
{
	if ( cySystem == -1 ){
		GetSystemFontMetrics();
	}
	return cySystem;
}

int GetSystemFontWidth()
{
	if ( cxSystem == -1 ){
		GetSystemFontMetrics();
	}
	return cxSystem;
}

int GetTextHeight(HWND hwnd, HFONT hFont)
{
	HDC hdc = GetDC(hwnd);
	HFONT oldFont = (HFONT)SelectObject( hdc, hFont );
	TEXTMETRIC tm;
	GetTextMetrics( hdc, &tm );
	int cyText = tm.tmHeight + tm.tmExternalLeading;
	//cxText = tm.tmAveCharWidth;
	SelectObject( hdc, oldFont );
	ReleaseDC(hwnd, hdc);
	return cyText;
}

///////////////////////////////////////////////////////////////
// Shell関連
///////////////////////////////////////////////////////////////
void InitShell()
{
#ifndef WINCE
	static bool ShellInitialized = false;
	if (ShellInitialized) return;
	CoInitialize(NULL); 
	ShellInitialized = true;
#endif
}
#endif	// GUI

///////////////////////////////////////////////////////////////
// その他
///////////////////////////////////////////////////////////////
#ifdef WINPPC
void SetWindowPosBySip( HWND hwnd )
{
	SIPINFO sip;
	memset( &sip, 0, sizeof(sip) );
	sip.cbSize = sizeof(sip);
	SipGetInfo( &sip );
	MoveWindow( hwnd,
		sip.rcVisibleDesktop.left,
		sip.rcVisibleDesktop.top,
		sip.rcVisibleDesktop.right - sip.rcVisibleDesktop.left,
		sip.rcVisibleDesktop.bottom - sip.rcVisibleDesktop.top
#ifdef PKTPC
		- (sip.fdwFlags & SIPF_ON ? 0 : GetSystemMetrics( SM_CYMENU ))
#endif
		,
		0 );
}
#endif	// WINPPC

void SetWindowLayered(HWND hwnd, int alpha)
{
	LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	if (alpha>=0x100){
		// Not layered.
		SetWindowLong(hwnd, GWL_EXSTYLE, exstyle & ~WS_EX_LAYERED);
	} else {
		if (!(exstyle & WS_EX_LAYERED)){
			SetWindowLong(hwnd, GWL_EXSTYLE, exstyle | WS_EX_LAYERED);
		}
		SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
	}
}

const void *LoadImageFromRes(int resId, int &size)
{
	HRSRC hResource = FindResource(hTNInstance, MAKEINTRESOURCE(resId), _T("IMAGE"));
	if (!hResource){
		return NULL;
	}
	size = SizeofResource(hTNInstance, hResource);
	if (!size){
		return NULL;
	}
	return LockResource(LoadResource(hTNInstance, hResource));
}

#if 0	// 2007.10.30 とりあえずVCLを利用することに変更
// Bitmap //
// --> tnclass
bool TransparentStretchBlt(HDC dstDC, int dstX, int dstY, int dstW, int dstH, HDC srcDC, int srcX, int srcY, int srcW, int srcH, HDC maskDC, int maskX, int maskY)
{
	const int ROP_DstCopy = 0x00AA0029;

	HDC memDC;
	HBITMAP hMemBmp;
	//Save: THandle;
	COLORREF crText, crBack;
	HPALETTE hSavePal;

	bool ret = true;

	if (srcW==dstW && srcH==dstH){
		hMemBmp = CreateCompatibleBitmap(srcDC, 1, 1);
		if (!hMemBmp)
			return false;

		hMemBmp = (HBITMAP)SelectObject(maskDC, hMemBmp);

		MaskBlt(dstDC, dstX, dstY, dstW, dstH, srcDC, srcX, srcY, hMemBmp, maskX,
			maskY, MAKEROP4(ROP_DstCopy, SRCCOPY));

		hMemBmp = (HBITMAP)SelectObject(maskDC, hMemBmp);
		DeleteObject(hMemBmp);
	} else {
		// Please use the below program if you want to use the un-isotropic image.
		__assert(false);
	}
	
	return ret;
}
#endif	// 0

