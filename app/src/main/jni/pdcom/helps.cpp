#include "tnlib.h"
#pragma	hdrstop
#include "pddefs.h"
#include "helps.h"
#include "assexec.h"
#include "hyplink.h"
#include "wpdcom.h"

const tchar *STR_HELPWEB_HOMEPAGE = _T("http://pdic.la.coocan.jp/");
const tchar *STR_HELPWEB_QA = _T("http://pdic.la.coocan.jp/unicode/qa.html");
const tchar *STR_HELPWEB_DOWNLOAD = _T("http://pdic.la.coocan.jp/unicode/");

const TCHAR *HHClassName = _T("HH Parent");
const TCHAR *HelpWindowName = _T("Personal Dictionary Help");

#ifdef WINCE
// No help
static tchar *StrHelpFile = _T("dummy.chm");
#else	// !WINCE
#if defined(_UNICODE)
static tchar *StrHelpFile = _T("PDICU.CHM");
#else	// !_UNICODE
static tchar *StrHelpFile = _T("PDICW32.CHM");
#endif	// !_UNICODE
#endif	// !WINCE

#ifdef WINCE
static HANDLE hProcess;
static HANDLE hThread;
#endif

int _HtmlHelp(HWND hwnd, const tchar *path, int cmd, int data);
#define	__CALLHELP(hwnd, path, winhelpcmd, data) \
	_HtmlHelp(NULL, path, winhelpcmd, data) /* ‘æˆêˆø”‚ðhwnd‚É‚·‚é‚ÆPDIC‚ÌŽè‘O‚ÉHelp‚ª•K‚¸•\Ž¦‚³‚ê‚Ä‚µ‚Ü‚¤ */

int LastHelpId = 0;

static HWND hwndHelp = NULL;

void InitHelp()
{
	TNHtmlHelpInit();
}
void UninitHelp()
{
	TNHtmlHelpUninit();
}

void GetHelpFileName( tnstr &buf )
{
	buf.set( CommandPath );
	buf.cat( StrHelpFile );
}

void HelpContext( HWND hwnd, int id )
{
#ifdef WINCE
#pragma message("Help System is not builded completely")
	PROCESS_INFORMATION p;
	CreateProcess(_T("peghelp.exe"), StrHelpFile, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &p);
	hProcess = p.hProcess;
	hThread = p.hThread;
#else
	tnstr path;
	GetHelpFileName( path );
	__CALLHELP( hwnd, path, HELP_CONTEXT, id );
#endif
}
void HelpQuit( HWND hwnd )
{
#ifdef WINCE
	CloseHandle( hProcess );
	CloseHandle( hThread );
#else
#endif
}

void HelpIndex( HWND hwnd )
{
#ifdef WINCE
#pragma message("Help System is not builded completely")
#else
	tnstr path;
	GetHelpFileName( path );
	__CALLHELP( hwnd, path.c_str(), HELP_FINDER, 0L );
#endif
}
void HelpKey( HWND hwnd )
{
#ifdef WINCE
#pragma message("Help System is not builded completely")
#else
	tnstr path;
	GetHelpFileName( path );
	__CALLHELP( hwnd, path, HELP_KEY, (DWORD)_T("") );
#endif
}
int HelpCommand(HWND hwnd, int cmd, int id)
{
	tnstr path;
	GetHelpFileName(path);
	return __CALLHELP(hwnd, path, cmd, id);
}

#ifndef WINCE
void HelpWeb( const tchar *url )
{
	AssociateExecute( NULL, url, HLT_HTTP );
}
#endif
int _HtmlHelp(HWND hwnd, const tchar *path, int cmd, int data)
{
	int ret = _TNHtmlHelp(hwnd, path, cmd, data);
	hwndHelp = FindHelpWindow();
	return ret;
}

static int WINAPI cbVclHelp(HWND hwnd, int cmd, int id)
{
	return HelpCommand(hwnd, cmd, id);
}

FNHelpXCallback cbHelp = cbVclHelp;

int WinHelpX(HWND hwnd, const tchar *path, int cmd, int id)
{
	hwnd = NULL;	// modeless‚É‚·‚é‚½‚ß
#ifdef UNICODE
	__mustr _path(path);
#endif
#if 1
	if (!cbHelp)
		return 0;
	return cbHelp(hwnd, cmd, id);
#else
	return TNWinHelp(hwnd, _path, cmd, id);
#endif
}
int WinHelpX(HWND hwnd, int cmd, int id)
{
	tnstr helpfile;
	GetHelpFileName( helpfile );
	return WinHelpX(hwnd, helpfile.c_str(), cmd, id);
}

int MessageBoxHelp(HWND hwnd, const tchar *text, const tchar *caption, int type, int helpId)
{
	LastHelpId = helpId;
	return MsgBox(hwnd, text, caption, type|(helpId>0?MB_HELP:0));
}

HWND FindHelpWindow()
{
	return FindWindow(HHClassName, HelpWindowName);
}

void EnableHelpWindow()
{
	if (!hwndHelp || !IsWindow(hwndHelp)){
		hwndHelp = NULL;
		return;
	}
	EnableWindow(hwndHelp, true);
}

