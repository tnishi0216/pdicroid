#include "tnlib.h"
#pragma hdrstop
#include "download.h"

static const tchar *MyWinTitle = _T("<Processing...>");

static bool GetRegKey( HKEY Key, const tchar *SubKey, tchar *RetData, long maxlen );

bool download_app(const tchar *appname)
{
	tchar key[256];
	tnstrbuf filename;

	if ( !_tcsncmp(appname, _t("http:"), 5) ){
		filename = appname;
	} else {
		filename += _t("http://pdic.sakura.ne.jp/cgi-bin/download/download.cgi?file=");
		filename += appname;
	}

	int show = SW_SHOW;
#ifdef WINCE
	if ( !_ShellExecute( NULL, _T("open"), filename, NULL, NULL, show ) )
#else
	if ( (UINT)ShellExecute( NULL, _T("open"), filename, NULL, NULL, show ) <= 32 )
#endif
	{
		// もしエラーがでれば
		key[0] = '\0';
		if ( GetRegKey( HKEY_CLASSES_ROOT, _T(".htm"), key, tsizeof(key) ) ){
			_tcscat( key, _T("\\shell\\open\\command") );
			if ( GetRegKey( HKEY_CLASSES_ROOT, key, key, tsizeof(key) ) ){
				tchar *p = _tcsstr( key, _T("\"%1\"") );
				if ( !p ){
					p = _tcsstr( key, _T("%1") );
					if ( p )
						memmove( p-1, p+2, LENTOBYTE(_tcslen(p+2)+1) );
				} else
					memmove( p-1, p+4, LENTOBYTE(_tcslen(p+4)+1) );
				_tcscat( key, _T(" ") );
				_tcscat( key, filename );
				return WinExecEx( key, show ) ? true : false;
			}
		}
	}
	else return true;
	return false;
}

HANDLE WinExecEx( const tchar *cmd, int show, const tchar *dir, const tchar *title )
{
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
	sui.lpTitle = (LPTSTR)(title ? title : MyWinTitle);
	PROCESS_INFORMATION pi;
	if ( !CreateProcess( NULL, (LPTSTR)cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, dir, &sui, &pi ) )
		return NULL;
	return pi.hProcess;
#endif
}

#if 0
// 日本語のparameterを通すために用意
//→関係なさそうなので実験用
#ifdef UNICODE
HANDLE WinExecEx( const char *cmd, int show, const char *dir, const char *title )
{
	STARTUPINFOA sui;
	memset( &sui, 0, sizeof(STARTUPINFOA) );
	sui.cb = sizeof(STARTUPINFOA);
	sui.dwFlags = STARTF_USESHOWWINDOW;
	sui.wShowWindow = (WORD)show;
	sui.lpTitle = (LPSTR)(title ? title : "");
	PROCESS_INFORMATION pi;
	if ( !CreateProcessA( NULL, (LPSTR)cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, dir, &sui, &pi ) )
		return NULL;
	return pi.hProcess;
}
#endif
#endif

static bool GetRegKey( HKEY Key, const tchar *SubKey, tchar *RetData, long maxlen )
{
	HKEY hk;

	int Result = RegOpenKeyEx( Key, SubKey, 0, KEY_QUERY_VALUE, &hk) == ERROR_SUCCESS;

	if ( Result ){
#ifdef WINCE
		DWORD type = REG_SZ;
		RegQueryValueEx( hk, NULL, NULL, &type, (LPBYTE)RetData, (DWORD*)&maxlen );
#else
		RegQueryValue(hk, NULL, RetData, &maxlen);
#endif
		RegCloseKey(hk);
	}
	return Result;
}
