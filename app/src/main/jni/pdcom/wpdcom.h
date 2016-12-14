#ifndef	__WPDCOM_H
#define	__WPDCOM_H

#include "defs.h"

#ifdef _Windows

// File Types
#define	FT_NONE		0x0000
#define	FT_TEXT		0x0001
#define	FT_BMP		0x0002
#define	FT_RTF		0x0003
#define	FT_TIFF		0x0004
#define	FT_JPEG		0x0005
#define	FT_GIF		0x0006
#define	FT_WAV		0x0007
#define	FT_AVI		0x0008
#define	FT_MID		0x0009
//#define	FT_MP3		0x000A
#define	FT_MPEG		0x000B
#define	FT_PNG		0x000C
#define	FT_HTML		0x000D
#define	FT_ICON		0x000E
#define	FT_WMF		0x000F
#define	FT_MAG		0x0010
#define	FT_LZH		0x0080

// ファイル名 //
bool IsExecuteExtension( const tchar *ext );
bool IsBitmapExtension( const tchar *ext );
int GetFileType( const tchar *filename, const tchar *ext=NULL );
bool IsAudioFile(const tchar *filename, const tchar *ext=NULL);

// File Operations //
#ifndef WINCE
tnstr_vec *GetDDFiles( HDROP hDrop );		// D&Dファイルの取得
#endif
bool GetFileInfo( const tchar *filename, ulong *FileSize, FILETIME *FileTime );
unsigned long GetFileSizeFileName( const tchar *filename );
bool write_bom(TOFile &tof);

// Window Operations //
void SetPdcomMainWindow(HWND hwnd);
void WaitDlgMessageProc( );
void DlgMessageProc( );
bool AppMessageProc( );
bool AppMessageProc( int msgmin, int msgmax );
//void WaitAppMessagesProc( );
void WaitAppMessageProc( );
void WaitAppMessageIdle();
void EnableItems( HWND hwnd, int *id, BOOL f );
void ShowItems( HWND hwnd, int *id, BOOL f );
void MoveToTop( HWND hwnd, bool restore_invisible=false );
void MoveToTopNA( HWND hwnd );
void MoveToTopPdic(HWND hwnd);
void MoveCenter( HWND hwnd, HWND hwndParent );
HWND GetForeWindow();
HWND GetTopParent(HWND hwnd);
void MakeWindowVisible(HWND hwnd);
void MouseWheelScroll( int wParam, class TScroller *Scroller );
void GetScreenSize(HWND hwndBase, RECT *rcWork, RECT *rcScreen=NULL);
bool GetScreenSize(HMONITOR hMonitor, RECT &scr);
bool GetScreenSize(POINT &ref, RECT &scr);
bool GetScreenSize(RECT &ref, RECT &scr);

bool GetAssociationFile( const tchar *ext, tnstr &app );
HICON GetIcon( HINSTANCE hInstance, const tchar *filename, int inx=0 );
void ClearIconCache();
HICON GetAssociatedIcon( HINSTANCE hInstance, const tchar *filename );
HANDLE _WinExec( const tchar *cmd, int show, const tchar *dir=NULL );

bool CopyClipboard( HWND hwnd, const tchar *txt, uint len=(uint)-1 );
bool CopyClipboard( const tchar *text, uint len=(uint)-1 );	// 追加コピー

// CreateSystemFont,SelectSystemFontをしたものは、必ずDeleteSystemFontを呼ぶこと!!
HFONT CreateSystemFont( );
HFONT SelectSystemFont( HDC hdc );
void DeleteSystemFont( HDC hdc, HFONT hOldFont );
int GetSystemFontWidth();
int GetSystemFontHeight();
int GetTextHeight(HWND hwnd, HFONT hFont);

void SetWindowPosBySip( HWND hwnd );

void SetWindowLayered(HWND hwnd, int alpha);

// Resouce Operation //
const void *LoadImageFromRes(int resId, int &size);

// Misc Windows APIs //
tnstr FormatMessage(int errcode);

// Shell関連 //
void InitShell();

// Language //
void SetupLanguage(const char *langid);
WORD GetLangId();

// MessageBox //
#include "msgbox.h"

#define	Beep()	MessageBeep( MB_OK )
#define	DB( m )	MessageBox( NULL, m, "デバッグ", MB_OK )

// Debug //
void InitExceptionInfo();
tnstr GetExceptionInfo();
LONG CALLBACK SWFilter(EXCEPTION_POINTERS *ExInfo);

// Global Variables //

extern tnstr CommandPath;	// このプログラムのパス
extern bool MoveToTopDoing;	// MoveToTop実行中
extern bool Suspending;

/////////////////////////////////////
// Compatiblity for WinCE
/////////////////////////////////////
#ifdef WINCE
bool _ShellExecute( HWND hwnd, const tchar *verb, const tchar *filename, const tchar *parameter, const tchar *directory, int show );
#else
#ifndef _twrite
#define	_twrite		_write
#endif
//#define	_tread		_read
#endif

#endif	// _Windows

#ifdef __ANDROID__
bool write_bom(TOFile &tof);
#endif	// __ANDROID__

#include "wpdcom2.h"

#endif	// __WPDCOM_H

